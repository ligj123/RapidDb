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
template <class T, uint32_t SZ = 1000> class STQueue {
public:
  STQueue() {}
  ~STQueue() {}

  bool Push(T *ele, bool submit = true) {
    if (_head == UINT32_MAX) [[unlikely]] {
      uint32_t tail = _tail;
      _tail %= SZ;
      tail -= _tail;
      _head -= tail;
      _submited -= tail;
    }

    uint32_t end = _tail;
    assert(_head - end <= SZ);
    if (_head - end == SZ) [[unlikely]] {
      end = atomic_ref<uint32_t>(_tail).load(memory_order_relaxed);
      if (_head - end == SZ) {
        return false;
      }
    }

    _arrCircle[_head % SZ] = ele;
    _head++;
    if (submit) {
      atomic_ref<uint32_t>(_submited).store(_head, memory_order_release);
    }
    return true;
  }

  void Submit() {
    if (_head != _submited) {
      atomic_ref<uint32_t>(_submited).store(_head, memory_order_release);
    }
  }

  void Pop(MVector<T *> &vct) {
    assert(vct.size() == 0);
    unique_lock<SpinMutex> lock(_spinMutex);
    uint32_t head = _submited;
    vct.reserve(head - _tail);
    while (_tail != head) {
      vct.push_back(_arrCircle[_tail % SZ]);
      _tail++;
    }
  }

  // Due to it used in 2 threads, so it can not sure the size is precise.
  // It must be called in consumer thread.
  size_t Size() {
    uint32_t sz =
        atomic_ref<uint32_t>(_submited).load(memory_order_relaxed) - _tail;
    return sz;
  }

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
  SpinMutex _spinMutex;
};
} // namespace storage