#pragma once
#include "CachePool.h"
#include <cstdlib>
#include <limits>
#include <list>
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
    return (T *)(CachePool::Apply((uint32_t)(n * sizeof(T))));
  }

  void deallocate(T *p, std::size_t n) noexcept {
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

template <class V> using MVector = std::vector<V, Mallocator<V>>;

template <class Key, class T>
using MHashMap = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>,
                                    Mallocator<std::pair<const Key, T>>>;

template <class Key>
using MHashSet = std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>,
                                    Mallocator<Key>>;

template <class Key, class T>
using MTreeMap =
    std::map<Key, T, std::less<Key>, Mallocator<std::pair<const Key, T>>>;

template <class Key>
using MTreeSet = std::set<Key, std::less<Key>, Mallocator<Key>>;

template <class T> using MList = std::list<T, Mallocator<T>>;
template <class T> using MDeque = std::deque<T, Mallocator<T>>;

using MString = basic_string<char, std::char_traits<char>, Mallocator<char>>;

inline MString ToMString(int value) {
  char buf[32];
  std::sprintf(buf, "%d", value);
  return MString(buf);
}
inline MString ToMString(long value) {
  char buf[32];
  std::sprintf(buf, "%ld", value);
  return MString(buf);
}
inline MString ToMString(long long value) {
  char buf[32];
  std::sprintf(buf, "%lld", value);
  return MString(buf);
}
inline MString ToMString(unsigned value) {
  char buf[32];
  std::sprintf(buf, "%u", value);
  return MString(buf);
}
inline MString ToMString(unsigned long value) {
  char buf[32];
  std::sprintf(buf, "%lu", value);
  return MString(buf);
}
inline MString ToMString(unsigned long long value) {
  char buf[32];
  std::sprintf(buf, "%llu", value);
  return MString(buf);
}
inline MString ToMString(float value) {
  char buf[32];
  std::sprintf(buf, "%f", value);
  return MString(buf);
}
inline MString ToMString(double value) {
  char buf[32];
  std::sprintf(buf, "%f", value);
  return MString(buf);
}
} // namespace storage
