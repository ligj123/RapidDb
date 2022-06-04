#pragma once
#include "SpinMutex.h"
#include "ThreadPool.h"
#include <array>
#include <atomic>
#include <list>
#include <queue>
#include <vector>

namespace storage {
using namespace std;

struct InnerQueue {
  // Here all pointer address will convert int64_t
  array<void *, 100> _arr;
  int _head = 0;
  int _tail = 0;
};
// To decrease lock time, every thread will has a thread_local list. In this
// way it does not need to lock when add an element into thread local list.
// Every instance will use _index to indentify the position in this vector.
static thread_local vector<InnerQueue *> _localInner;
// To record how many instances of FastQueue in this process. Every instance's
// index will get value from it and then it will increase one. The index will
// be used to indentify how to set and get InnerQueue from _localVct
static atomic_int32_t _queueCount{0};

// Here T must be class pointer
// This queue is depend on ThreadPool. The thread id in pool must start from 0
// and must be series. If used in multi instances of ThreadPool. the thread ids
// must be series between two pool.
template <class T> class FastQueue {
public:
  // threadCount: The total threads in all related ThreadPool
  FastQueue(int threadCount) : _threadCount(threadCount) {
    _vctInner.reserve(threadCount);
    for (int i = 0; i < threadCount; i++) {
      _vctInner.push_back(new InnerQueue);
    }

    _index = _queueCount.fetch_add(1, memory_order_relaxed);
  }

  ~FastQueue() {
    assert(_queue.size() == 0);
    for (InnerQueue *q : _vctInner) {
      assert(q->_head == q->_tail);
      delete q;
    }
  }

  // Push an element
  void Push(T *ele) {
    if (_localInner.size() <= (size_t)_index) {
      for (size_t i = _localInner.size(); i <= (size_t)_index; i++) {
        _localInner.push_back(nullptr);
      }

      _localInner.push_back(_vctInner[ThreadPool::_threadID]);
    }

    InnerQueue *q = _localInner[_index];
    if (q == nullptr) {
      q = _vctInner[ThreadPool::_threadID];
      _localInner[_index] = q;
    }

    q->_arr[q->_tail] = ele;
    q->_tail = (q->_tail + 1) % 100;
    if ((q->_tail + 1) % 100 == q->_head) {
      ElementMove(true);
    }
  }

  T *Pop() {
    unique_lock<SpinMutex> lock(_spinMutex);
    if (_queue.size() == 0) {
      ElementMove();
    }

    if (_queue.size() == 0)
      return nullptr;

    T *val = _queue.front();
    _queue.pop();

    return val;
  }

  void swap(queue<T *> &queue) {
    unique_lock<SpinMutex> lock(_spinMutex);
    ElementMove();
    queue.swap(_queue);
  }

protected:
  void ElementMove(bool bLock = false) {
    if (bLock) {
      _spinMutex.lock();
    }

    for (InnerQueue *q : _vctInner) {
      if (q->_head == q->_tail)
        continue;

      while (true) {
        _queue.push((T *)q->_arr[q->_head]);

        q->_head = (q->_head + 1) % 100;
        if (q->_head == q->_tail) {
          break;
        }
      }
    }

    if (bLock) {
      _spinMutex.unlock();
    }
  }

protected:
  queue<T *> _queue;
  SpinMutex _spinMutex;
  // FastQueue depend on ThreadPool and only code run in ThreadPool can push
  // element. Every thread has a thread id and here will use thread id as
  // index of vector
  vector<InnerQueue *> _vctInner;
  // The index in thread_local variable vecotr<InnerQueue>
  int _index;
  // How many threads in thread pool
  int _threadCount;

protected:
};
} // namespace storage
