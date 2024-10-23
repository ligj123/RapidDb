#pragma once
#include "../cache/Mallocator.h"

#include <atomic>

#define BLOCK_SIZE 256;
#define ELE_SIZE 32
using namespace std;

namespace storage {
template <class T> struct LinkNode {
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  LinkNode() { _block = (T *)CachePool::Apply(BLOCK_SIZE); }
  ~LinkNode() { CachePool::Release((Byte *)_block, BLOCK_SIZE); }

  T *_block;
  LinkNode *_next{nullptr};
};

template <class T> class LineQueue {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  LineQueue(uint64_t max_elements) : _max_elements(max_elements) {
    _startNode = new LinkNode();
    _endNode = _startNode;
  }

  ~LineQueue() {
    _submited.load(memory_order_acquire);
    while (_endNode != nullptr) {
      LinkNode *node = _endNode;
      _endNode = _endNode->next;
      delete node;
    }
  }

  bool Push(T *ele, bool submit = true) {
    if (RoughSize() > _max_elements) [[unlikely]] {
      return false;
    }

    uint64_t pos = _head % ELE_SIZE;
    if (pos == 0) {
      assert(_startNode->_next == nullptr);
      submit = true;
      _startNode->_next = new LinkNode();
      _startNode = _startNode->_next;
    }

    _startNode->_block[pos] = ele;
    _head++;

    if (submit) {
      _submited.store(_head, memory_order_release);
    }

    return true;
  }

  void Submit() {
    if (_head != _submited.load(memory_order_relax)) {
      _submited.store(_head, memory_order_release);
    }
  }

  void Pop(MDeque<T *> &dq) {
    uint64_t head = _submited.load(memory_order_acquire);
    uint64_t tail = _tail.load(memory_order_relax);

    while (_endNode->_next != nullptr) {
      for (uint64_t i = tail % ELE_SIZE; i < ELE_SIZE; i++) {
        dq.push_back(_endNode->_block[i]);
        tail++;
      }

      LinkNode *node = _endNode;
      _endNode = _endNode->_next;
      delete node;
    }

    if (head < ELE_SIZE && tail > UINT64_MAX - ELE_SIZE) [[unlikely]] {
      tail = tail % ELE_SIZE;
    }

    assert(head - tail <= ELE_SIZE);
    for (; tail < head; tail++) {
      dq.push_back(_endNode->_block[tail % ELE_SIZE]);
    }

    _tail.store(tail, memory_order_relax);
  }

  bool IsEmpty() {
    uint64_t tail = _tail.load(memory_order_relax);
    uint64_t submit = _submited.load(memory_order_acquire);
    return tail == submit && _head == submit;
  }

  size_t RoughSize() {
    uint64_t tail = _tail.load(memory_order_relax);
    uint64_t submit = _submited.load(memory_order_relax);
    if (tail > submit) [[unlikely]] {
      return submit + (UINT64_MAX - tail + 1);
    } else {
      return submit - tail;
    }
  }

protected:
  LinkNode *_startNode;
  LinkNode *_endNode;

  //  The head position of circle queue that elements has inserted, if the
  //  client inserted an element and submited at once, it will equal to
  //  _submited, or it will be ahead of _submited
  uint64_t _head{0};
  // The position that has submited elements, it should be equal or behind of
  // _head
  atomic_uint64_t _submited{0};
  // The tail of queue that obtain inserted elements.
  atomic_uint64_t _tail{0};
  // The max blocks can be assigned for this line.
  uint64_t _max_elements;
};

template <class T> class RapidQueue {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  RapidQueue(uint16_t maxThreadNum, uint16_t threadNum = 0,
             uint64_t max_elements = 32 * 1024 * 1024)
      : _maxThreadNum(maxThreadNum),
        _currAlivedThreads(threadNum == 0 ? maxThreadNum : threadNum),
        _currAlivedThreads(threadNum == 0 ? maxThreadNum : threadNum),
        _vctLine(maxThreadNum, LineQueue(max_elements)) {
    assert(_maxThreadNum >= _threadNum);
  }
  /**
   * @brief To reset the live thread number, it must be not large than max
   * thread number
   * @param threadNum The new thread number
   */
  void ResetLiveThreadNumber(uint16_t threadNum) {
    assert(maxThreadNum >= threadNum);
    unique_lock<SpinMutex> lock(_spinMutex);
    _lastSetThreads = threadNum;
    if (_currAlivedThreads > threadNum) {
      _popNum = 5;
    } else {
      _currAlivedThreads = threadNum;
    }
  }

  /**
   * @brief Push an element into line queue wothout lock
   * @param tid The thread serial number start from 0
   * @param ele The element that will be inserted
   * @param submit True: The element will submit and the receiver can find;
   *               False: The receiver can not find this element until submit.
   * @return True: The element has been inserted into queue
   *         False: The element failed to insert into the queue due to The line
   * queue own too much elements.
   */
  bool Push(uint16_t tid, T *ele, bool submit = true) {
    assert(tid < _lastSetThreads);
    return _vctLine[tid].Push(ele, submit);
  }

  /**
   * @brief Submit the elements
   * @param tid The thread serial number start from 0
   */
  void Submit(uint32_t tid) {
    assert(tid < _lastSetThreads);
    _vctLine[tid].Submit();
  }

  /**
   * @brief Pop all elements in line queues.
   * @param queue The queue to save the elements
   * @param lock If need to lock the spin mutex.
   */
  void Pop(MDeque<T *> &queue) {
    assert(queue.size() == 0);

    for (uint16_t i = 0; i < _currAlivedThreads; i++) {
      _vctLine[i].Pop(queue);
    }

    if (_popNum > 0) {
      _popNum--;
      if (_popNum == 0) {
        _currAlivedThreads = _lastSetThreads;
      }
    }
  }

  /**
   * @brief Here does not consider thread safe, so the result can not sure
   * accurate, it is just rough result
   */
  bool IsEmpty() {
    for (uint16_t i = 0; i < _currAlivedThreads; i++) {
      if (!_vctLine[i].IsEmpty())
        return false;
    }

    return true;
  }

  /**
   * @brief Only return elements number in queue and submited elements, neglect
   * the unsubmited elements in InnerQueue.
   */
  size_t RoughSize() {
    size_t sz = 0;
    for (uint16_t i = 0; i < _currAlivedThreads; i++) {
      sz += _vctLine[i].RoughSize();
    }

    return sz;
  }

protected:
  // The max number of threads that push data into this queue
  uint16_t _maxThreadNum;
  // The current alived threads number
  uint16_t _currAlivedThreads;
  // The thread number of last time to set.
  uint16_t _lastSetThreads;
  // to ensure all data will be picked after reset thread number and new number
  // is small than old, here will pop 5 time before use new thread number.
  int16_t _popNum{0};
  // The array of line queue , every line response to a thread.
  MVector<LineQueue> _vctLine;
};
} // namespace storage