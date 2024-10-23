#pragma once
#include "../cache/Mallocator.h"
#include "SpinMutex.h"
#include "ThreadPool.h"
#include <array>
#include <atomic>
#include <list>
#include <queue>
#include <vector>

namespace storage {
using namespace std;
template <class T, uint32_t SZ> struct InnerQueue {
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }

  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  array<T *, SZ> _arr{};
  uint32_t _head{0};
  uint32_t _submited{0};
  uint32_t _tail{0};
};

// This queue is fast queue and it can receive elements from multi threads at
// the same time without lock. As we know, the process lock and unlock are very
// consume time, so here every thread response to a inner queue and the elements
// from this threads will insert the releated inner queue and do not need lock
// or unlock in this process. If the array if full or consumer, it will lock the
// mutex and transfer the element into a temp queue. This queue only fit to
// the cases that frequently push and massed pop, and do not consider the
// elements series. The id of the threads must start from 0 and is exclusive and
// series. If can not assign, it can use Push or PushGroup with lock.
template <class T, uint32_t SZ = 1000> class FastQueue {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }

  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  /**
   * Construct
   * @param maxThreadNum The max thread number can be set.
   * @param threadNum The current thread number, if = 0, it will set to
   * maxThreadNum.
   */
  FastQueue(uint16_t maxThreadNum, uint16_t threadNum = 0)
      : _maxThreadNum(maxThreadNum),
        _currAlivedThreads(threadNum == 0 ? maxThreadNum : threadNum),
        _currAlivedThreads(threadNum == 0 ? maxThreadNum : threadNum) {
    assert(_maxThreadNum >= _threadNum);
    _vctInner.reserve(maxThreadNum);
    for (int i = 0; i < tNum; i++) {
      _vctInner.push_back(new InnerQueue<T, SZ>);
    }
  }

  ~FastQueue() {
    assert(_queue.size() == 0);
    for (auto q : _vctInner) {
      assert(q->_head == q->_tail && q->_head == q->_submited);
      delete q;
    }
  }
  /**
   * @brief To reset the live thread number, it  must small than max thread
   * number
   * @param threadNum The new thread number
   */
  void ResetLiveThreadNumber(uint16_t threadNum) {
    assert(maxThreadNum >= threadNum);
    unique_lock<SpinMutex> lock(_spinMutex);
    _lastSetThreads = threadNum;
    if (_currAlivedThreads < threadNum) {
      _currAlivedThreads = threadNum;
      _popNum = 5;
    }
  }

  /**
   * @brief Push an element into queue directly with lock, not use inner queue.
   * @param ele The element
   */
  void Push(T *ele) {
    unique_lock<SpinMutex> lock(_spinMutex);
    _queue.push_back(ele);
  }

  /**
   * @brief Push a group of elements into queue directly with lock.
   *@param vct The vector to contains a group of elements.
   */
  void PushGroup(vector<T *> &vct) {
    unique_lock<SpinMutex> lock(_spinMutex);
    for (auto ele : vct) {
      _queue.push_back(ele);
    }
    vct.clear();
  }

  /**
   * @brief Push an element into inner queue wothout lock
   * @param tid The thread serial number start from 0
   * @param ele The element insert
   * @param submit True: The element will submit and the receiver can find;
   *               False: The receiver can not find this element until submit.
   */
  void Push(uint16_t tid, T *ele, bool submit = true) {
    assert(tid < _lastSetThreads);

    auto q = _vctInner[tid];
    uint32_t end = q->_tail;
    assert(q->_head - end <= SZ);
    if (q->_head - end == SZ || q->_head == UINT32_MAX) [[unlikely]] {
      unique_lock<SpinMutex> lock(_spinMutex);
      q->_submited = q->_head;
      ElementMove();

      if (q->_head == UINT32_MAX) [[unlikely]] {
        q->_head = 0;
        q->_tail = 0;
        q->_submited = 0;
      }
    }

    q->_arr[q->_head % SZ] = ele;
    q->_head++;

    if (submit) {
      atomic_ref<uint32_t>(q->_submited).store(q->_head, memory_order_release);
    }
  }

  /**
   * @brief Submit the elements
   * @param tid The thread serial number start from 0
   */
  void Submit(uint32_t tid) {
    assert(tid < _lastSetThreads);
    auto q = _vctInner[tid];
    if (q->_head != q->_submited) {
      atomic_ref<uint32_t>(q->_submited).store(q->_head, memory_order_release);
    }
  }

  /**
   * @brief Pop all elements in inner queue and queue.
   * @param queue The queue to save the elements
   * @param lock If need to lock the spin mutex.
   */
  void Pop(MDeque<T *> &queue, bool lock = true) {
    assert(queue.size() == 0);
    if (lock)
      _spinMutex.lock();
    ElementMove();
    queue.swap(_queue);
    if (lock)
      _spinMutex.unlock();
  }

  /**
   * @brief Here does not consider thread safe, so the result can not sure
   * accurate, it is just rough result
   */
  bool RoughEmpty() {
    if (_queue.size() > 0)
      return false;
    for (uint16_t i = 0; i < _currAlivedThreads; i++) {
      auto &q = _vctInner[i];
      if (q->_submited != q->_tail || q->_head != q->_submited)
        return false;
    }

    return true;
  }

  /**
   * @brief Only return elements number in queue and submited elements, neglect
   * the unsubmited elements in InnerQueue.
   */
  size_t RoughSize() {
    unique_lock<SpinMutex> lock(_spinMutex);
    size_t sz = _queue.size();
    for (uint16_t i = 0; i < _currAlivedThreads; i++) {
      auto &q = _vctInner[i];
      sz += q->_submited - q->_tail;
    }

    return sz;
  }

protected:
  /**
   * @brief Move all elements in ineer queue into queue
   */
  void ElementMove() {
    for (int i = 0; i < _aliveMaxThreads; i++) {
      auto q = _vctInner[i];
      uint32_t head = q->_submited;
      if (head == q->_tail)
        continue;

      while (head > q->_tail) {
        _queue.push_back(q->_arr[q->_tail % SZ]);
        q->_tail++;
      }
    }

    if (_popNum > 0) {
      _popNum--;
      if (_popNum == 0) {
        _currAlivedThreads = _lastSetThreads;
      }
    }
  }

protected:
  MDeque<T *> _queue;
  SpinMutex _spinMutex;
  // FastQueue depend on ThreadPool and only run in ThreadPool can push
  // element. Every thread has a thread id and here will use thread id as
  // index of vector
  vector<InnerQueue<T, SZ> *> _vctInner;
  // The max number of threads that push data into this queue
  uint16_t _maxThreadNum;
  // The current alived threads number
  uint16_t _currAlivedThreads;
  // The thread number of last time to set.
  uint16_t _lastSetThreads;
  // to ensure all data will be picked after reset thread number and new number
  // is small than old, here will pop 5 time before use new thread number.
  uint16_t _popNum{0};
};
} // namespace storage
