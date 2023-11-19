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
template <class T, uint16_t SZ> struct InnerQueue {
  array<T *, SZ> _arr{};
  uint16_t _head{0};
  uint16_t _submited{0};
  uint16_t _tail{0};
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
template <class T, uint16_t SZ = 100> class FastQueue {
public:
  // threadCount: The total threads in all related ThreadPool
  FastQueue(uint16_t tNum) : _aliveMaxThreads(tNum), _threadTotalNum(tNum) {
    assert(tNum > 0);
    _vctInner.reserve(tNum);
    for (int i = 0; i < tNum; i++) {
      _vctInner.push_back(new InnerQueue<T, SZ>);
    }
  }

  FastQueue(ThreadPool *tp = nullptr)
      : _aliveMaxThreads(tp->GetMaxThreads()),
        _threadTotalNum(tp->GetMaxThreads()) {
    _vctInner.reserve(tp->GetMaxThreads());
    for (int i = 0; i < tp->GetMaxThreads(); i++) {
      _vctInner.push_back(new InnerQueue<T, SZ>);
    }

    tp->PushLambda([this](uint16_t threads) {
      unique_lock<SpinMutex> lock(_spinMutex);
      _aliveLastThreads = threads;
      if (_aliveMaxThreads < threads) {
        _aliveMaxThreads = threads;
      }
    });
  }

  ~FastQueue() {
    assert(_queue.size() == 0);
    for (auto q : _vctInner) {
      assert(q->_head == q->_tail);
      delete q;
    }
  }

  void UpdateLiveThreads(uint16_t threads) {
    unique_lock<SpinMutex> lock(_spinMutex);
    _aliveLastThreads = threads;
    if (_aliveMaxThreads < threads) {
      _aliveMaxThreads = threads;
    }
  }

  // Push an element
  void Push(T *ele, uint16_t tid = UINT16_MAX, bool submit = true) {
    assert(tid < _threadTotalNum);
    if (tid == UINT16_MAX) {
      unique_lock<SpinMutex> lock(_spinMutex);
      _queue.push(ele);
      return;
    }

    auto q = _vctInner[tid];
    q->_arr[q->_head] = ele;
    q->_head = (q->_head + 1) % SZ;
    if (submit) {
      atomic_ref<uint16_t>(q->_submited).store(q->_head, memory_order_release);
    }
    if ((q->_head + 1) % SZ == q->_tail) {
      unique_lock<SpinMutex> lock(_spinMutex);
      q->_submited = q->_head;
      ElementMove();
    }
  }

  void Submit(uint16_t tid) {
    assert(tid >= 0 && tid < _threadTotalNum);
    auto q = _vctInner[tid];
    if (q->_head != q->_submited) {
      atomic_ref<uint16_t>(q->_submited).store(q->_head, memory_order_release);
    }
  }

  void Pop(queue<T *> &queue) {
    assert(queue.size() == 0);
    unique_lock<SpinMutex> lock(_spinMutex);
    ElementMove();
    queue.swap(_queue);
  }

  // Here does not consider thread safe, so the result can not sure accurate, it
  // is just rough result
  bool RoughEmpty() {
    if (_queue.size() > 0)
      return false;
    for (auto q : _vctInner) {
      if (q->_submited != q->_tail || q->_head != q->_submited)
        return false;
    }

    return true;
  }

  // No consider thread safe, only return elements number in queue, neglect the
  // elements in InnerQueue.
  size_t RoughSize() { return _queue.size(); }

protected:
  void ElementMove() {
    for (int i = 0; i < _aliveMaxThreads; i++) {
      auto q = _vctInner[i];
      if (q->_submited == q->_tail)
        continue;

      while (true) {
        _queue.push((T *)q->_arr[q->_tail]);

        q->_tail = (q->_tail + 1) % 100;
        if (q->_tail == q->_submited) {
          break;
        }
      }
    }

    _aliveMaxThreads = _aliveLastThreads;
  }

protected:
  queue<T *> _queue;
  SpinMutex _spinMutex;
  // FastQueue depend on ThreadPool and only run in ThreadPool can push
  // element. Every thread has a thread id and here will use thread id as
  // index of vector
  vector<InnerQueue<T, SZ> *> _vctInner;
  // The max number of threads in thread pool or used sole threads
  uint16_t _threadTotalNum;
  // The max alive threads number since previous to call ElementMove
  uint16_t _aliveMaxThreads;
  // The thread number of last time to change threads.
  uint16_t _aliveLastThreads;
};
} // namespace storage
