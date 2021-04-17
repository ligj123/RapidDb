#pragma once
#include "SpinMutex.h"
#include <unordered_map>
#include <vector>

namespace utils {
using namespace std;
template <class Key, class T> class ConcurrentHashMap {
public:
  ConcurrentHashMap(int groupCount, size_type bucketCount)
      : _vctMap(gCount, unordered_map<key, T>(bucket_count / gCount)),
        _vctLock(gCount, SpinMutex()), _groupCount(groupCount) {}
  size_type size() {
    size_type sz = 0;
    for (auto iter = _vctMap.begin(); iter != _vctMap.end(); iter++) {
      sz += iter->size();
    }

    return sz;
  }

  bool insert(Key key, T val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(_vctLock[pos]);
    return _vctMap[pos].insert({key, val}).second;
  }

  bool &find(Key key, T &val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(_vctLock[pos]);
    auto iter = _vctMap[pos].find(key);
    if (iter == _vctMap[pos].end()) {
      return false;
    } else {
      val = iter->second;
      return true;
    }
  }

  iterator begin(int pos) {
    _vctLock[pos].lock();
    return _vctMap[pos].begin();
  }

  iterator end(int pos) { return _vctMap[pos].end(); }

  void unlock(int pos) { _vctLock[pos].unlock(); }

protected:
  vector<unordered_map<key, T>> _vctMap;
  vector<SpinMutex> _vctLock;
  int _groupCount;
};
} // namespace utils
