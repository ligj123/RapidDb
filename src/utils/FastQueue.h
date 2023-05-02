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
template <class T> struct InnerQueue {
  // Here all pointer address will convert int64_t
  array<T *, 100> _arr{};
  int _head = 0;
  int _tail = 0;
};

// This queue is fast queue and it depend on ThreadPool. As we know, the lock
// and unlock are very consume time, so here every thread response to a inner
// queue and the elements from this threads will insert the releated inner queue
// and do not need lock or unlock in this process. If the array if full or
// consumer, it will lock the mutex and transfer the element to consumer's
// queue. This queue only fit to the cases that frequently push and massed pull,
// and do not consider the elements series. The id from thread pool must start
// from 0 and is series. If there have more than one thread pool, the ids in
// this pools must series and threadCount must be the sum of all threads in all
// pools. For all single threads, for example main or timer thread, the id must
// be -1 and do not use inner queue to avlid lock.
template <class T> class FastQueue {
public:
  // threadCount: The total threads in all related ThreadPool
  FastQueue(int threadCount) : _threadCount(threadCount) {
    _vctInner.reserve(_threadCount);
    for (int i = 0; i < _threadCount; i++) {
      _vctInner.push_back(new InnerQueue<T>);
    }
  }

  ~FastQueue() {
    assert(_queue.size() == 0);
    for (InnerQueue<T> *q : _vctInner) {
      assert(q->_head == q->_tail);
      delete q;
    }
  }

  // Push an element
  void Push(T *ele) {
    assert(ThreadPool::GetThreadId() < _threadCount);
    if (ThreadPool::GetThreadId() < 0) {
      unique_lock<SpinMutex> lock(_spinMutex);
      _queue.push(ele);
      return;
    }

    InnerQueue<T> *q = _vctInner[ThreadPool::GetThreadId()];
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

  void Swap(queue<T *> &queue) {
    unique_lock<SpinMutex> lock(_spinMutex);
    ElementMove();
    queue.swap(_queue);
  }

  bool Empty() {
    if (_queue.size() > 0)
      return false;
    for (InnerQueue<T> *q : _vctInner) {
      if (q->_head != q->_tail)
        return false;
    }
    return true;
  }

  // No consider thread safe, only return elements number in queue, neglect the
  // elements in InnerQueue.
  size_t RoughSize() { return _queue.size(); }

protected:
  void ElementMove(bool bLock = false) {
    if (bLock) {
      _spinMutex.lock();
    }

    for (InnerQueue<T> *q : _vctInner) {
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
  // FastQueue depend on ThreadPool and only run in ThreadPool can push
  // element. Every thread has a thread id and here will use thread id as
  // index of vector
  vector<InnerQueue<T> *> _vctInner;
  // How many threads in thread pool
  int _threadCount;

protected:
};
} // namespace storage
