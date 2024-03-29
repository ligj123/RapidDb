#pragma once
#include "../cache/Mallocator.h"
#include "SpinMutex.h"
#include <atomic>
#include <cassert>
#include <mutex>
#include <queue>
#include <vector>

namespace storage {
using namespace std;
// Single channel queue.Only two thread can take part in this queue, one
// produces elements and the other consume elements. Can not change their roles
// in short time.
template <class T, uint32_t SZ = 1000> class SingleQueue {
public:
  SingleQueue() {}
  ~SingleQueue() {}

  void Push(T *ele, bool submit = true) {
    uint32_t end = _tail;
    assert(_head - end <= SZ);
    if (_head - end == SZ || _head == UINT32_MAX) [[unlikely]] {
      unique_lock<SpinMutex> lock(_spinMutex);
      _testCount++;
      while (_tail != _head) {
        _queue.push_back(_arrCircle[_tail % SZ]);
        _tail++;
      }

      if (_head == UINT32_MAX) [[unlikely]] {
        _head = 0;
        _tail = 0;
      }
      _submited = _head;
    }

    _arrCircle[_head % SZ] = ele;
    _head++;
    if (submit) {
      atomic_ref<uint32_t>(_submited).store(_head, memory_order_release);
    }
  }

  void Submit() {
    if (_head != _submited) {
      atomic_ref<uint32_t>(_submited).store(_head, memory_order_release);
    }
  }

  void Pop(MDeque<T *> &q) {
    assert(q.size() == 0);
    unique_lock<SpinMutex> lock(_spinMutex);
    uint32_t head = _submited;
    // atomic_ref<uint32_t>(_submited).load(memory_order_acquire);
    while (_tail != head) {
      _queue.push_back(_arrCircle[_tail % SZ]);
      _tail++;
    }

    q.swap(_queue);
  }

  // Due to it used in 2 threads, so it can not sure the size is precise.
  // It must be called in consumer thread.
  size_t Size() {
    uint32_t sz =
        atomic_ref<uint32_t>(_submited).load(memory_order_relaxed) - _tail;
    sz += _queue.size();
    return sz;
  }

  uint32_t _testCount{0};

protected:
  // The circle queue with fixed space to save elements
  array<T *, SZ> _arrCircle;
  //  The head position of circle queue that elements has inserted, if the
  //  client inserted an element and submited at once, it will equal to
  //  _submited, or it will be ahead of _submited
  uint32_t _head{0};
  // The position that has submited elements, it should be equal or behind of
  // _head
  uint32_t _submited{0};
  // The tail of queue that obtain inserted elements.
  uint32_t _tail{0};
  // If _queue != nullptr and the circle queue is full, the elements will moved
  // into _queue and free the circle queue to save new elements.
  MDeque<T *> _queue;
  //
  SpinMutex _spinMutex;
};
} // namespace storage