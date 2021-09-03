#pragma once
#include "../cache/Mallocator.h"
#include "SpinMutex.h"
#include <mutex>
#include <unordered_map>

namespace storage {
using namespace std;
template <class Key, class T> class ConcurrentHashMap {
public:
  typedef unordered_map<Key, T> ConHashMap;
  ConcurrentHashMap(int groupCount, uint64_t maxCount)
      : _groupCount(groupCount) {
    _vctMap.reserve(groupCount);
    _vctLock.reserve(groupCount);
    for (int i = 0; i < groupCount; i++) {
      _vctMap.push_back(new ConHashMap(maxCount / groupCount));
      _vctLock.push_back(new SpinMutex());
    }
  }

  ~ConcurrentHashMap() {
    for (int i = 0; i < _groupCount; i++) {
      delete _vctMap[i];
      delete _vctLock[i];
    }
  }
  size_t size() {
    size_t sz = 0;
    for (int i = 0; i < _vctMap.size(); i++) {
      sz += _vctMap[i]->size();
    }

    return sz;
  }

  bool insert(Key key, T val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(*_vctLock[pos]);
    return _vctMap[pos]->insert({key, val}).second;
  }

  bool find(Key key, T &val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(*_vctLock[pos]);
    auto iter = _vctMap[pos]->find(key);
    if (iter == _vctMap[pos]->end()) {
      return false;
    } else {
      val = iter->second;
      return true;
    }
  }

  auto begin(int pos) {
    _vctLock[pos]->lock();
    return _vctMap[pos]->begin();
  }

  auto end(int pos) { return _vctMap[pos]->end(); }

  void unlock(int pos) { _vctLock[pos]->unlock(); }

protected:
  vector<ConHashMap *> _vctMap;
  vector<SpinMutex *> _vctLock;
  int _groupCount;
};
} // namespace storage
