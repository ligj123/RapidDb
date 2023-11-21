#pragma once
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
template <class T, uint16_t SZ = 1000> class SingleQueue {
public:
  SingleQueue() {}
  ~SingleQueue() {}

  bool Push(T *ele, bool submit = true) {
    uint16_t end =
        (submit ? atomic_ref<uint16_t>(_tail).load(memory_order_relaxed)
                : _tail);

    if ((_head + 1) % SZ == end) {
      unique_lock<SpinMutex> lock(_spinMutex);
      while (_tail != _submited) {
        _queue.push(_arrCircle[_tail]);
        _tail++;
        _tail %= SZ;
      }

      assert((_head + 1) % SZ != _tail);
      if (_queue.size() > SZ * 10) {
        return false;
      }
    }

    _arrCircle[_head] = ele;
    _head++;
    _head %= SZ;
    if (submit) {
      atomic_ref<uint16_t>(_submited).store(_head, memory_order_release);
    }

    return true;
  }

  void Submit() {
    if (_head != _submited) {
      atomic_ref<uint16_t>(_submited).store(_head, memory_order_release);
    }
  }

  void Pop(queue<T *> &q) {
    assert(q.size() == 0);
    unique_lock<SpinMutex> lock(_spinMutex);
    uint16_t head = atomic_ref<uint16_t>(_submited).load(memory_order_acquire);
    while (_tail != head) {
      _queue.push(_arrCircle[_tail]);
      _tail++;
      _tail %= SZ;
    }

    q.swap(_queue);
  }

  // Due to it used in 2 threads, so it can not sure the size is precise.
  size_t Size() {
    uint32_t sz = (_tail < _head ? _head - _tail : SZ + _head - _tail);
    sz += _queue.size();
    return sz;
  }

protected:
  // The circle queue with fixed space to save elements
  array<T *, SZ> _arrCircle;
  //  The head position of circle queue that elements has inserted, if the
  //  client inserted an element and submited at once, it will equal to
  //  _submited, or it will be ahead of _submited
  uint16_t _head{0};
  // The position that has submited elements, it should be equal or behind of
  // _head
  uint16_t _submited{0};
  // The tail of queue that obtain inserted elements.
  uint16_t _tail{0};
  // If _queue != nullptr and the circle queue is full, the elements will moved
  // into _queue and free the circle queue to save new elements.
  queue<T *> _queue;
  // Only used when _queue != nullptr
  SpinMutex _spinMutex;
};
} // namespace storage