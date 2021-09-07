#pragma once
#include "CachePool.h"
#include <cstdlib>
#include <limits>
#include <map>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace storage {
template <class T> class Mallocator {
public:
  typedef T value_type;

  Mallocator() = default;
  template <class U> constexpr Mallocator(const Mallocator<U> &) noexcept {}
  T *allocate(std::size_t n) {
    // return (T *)::operator new(n * sizeof(T));
    return (T *)(CachePool::Apply((uint32_t)(n * sizeof(T))));
  }

  void deallocate(T *p, std::size_t n) noexcept {
    //::operator delete((void *)p);
    CachePool::Release((Byte *)p, (uint32_t)(n * sizeof(T)));
  }
};

template <class T, class U>
inline bool operator==(const Mallocator<T> &, const Mallocator<U> &) {
  return true;
}
template <class T, class U>
inline bool operator!=(const Mallocator<T> &, const Mallocator<U> &) {
  return false;
}

template <class V> struct MVector {
public:
  typedef std::vector<V, Mallocator<V>> Type;
};

template <class Key, class T> struct MHashMap {
public:
 typedef std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>,
                            Mallocator<std::pair<const Key, T>>>
     Type;
};

template <class Key> struct MHashSet {
public:
 typedef std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>,
                            Mallocator<Key>>
     Type;
};

template <class Key, class T> struct MTreeMap {
public:
 typedef std::map<Key, T, std::less<Key>, Mallocator<std::pair<const Key,
 T>>>
     Type;
};

// template <class V> struct MVector {
// public:
//  typedef std::vector<V> Type;
//};

// template <class Key, class T> struct MHashMap {
// public:
//   typedef std::unordered_map<Key, T> Type;
// };

// template <class Key> struct MHashSet {
// public:
//   typedef std::unordered_set<Key> Type;
// };

// template <class Key, class T> struct MTreeMap {
// public:
//   typedef std::map<Key, T> Type;
// };
} // namespace storage
