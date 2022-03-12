#pragma once
#include "../cache/Mallocator.h"
#include "SpinMutex.h"
#include <mutex>

namespace storage {
using namespace std;
template <class Key, class T> class ConcurrentHashMap {
public:
  typedef std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>,
                             Mallocator<std::pair<const Key, T>>>
      ConHashMap;
      
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
  size_t Size() {
    size_t sz = 0;
    for (int i = 0; i < _vctMap.size(); i++) {
      sz += _vctMap[i]->size();
    }

    return sz;
  }

  bool Insert(Key key, T val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(*_vctLock[pos]);
    return _vctMap[pos]->insert({key, val}).second;
  }

  bool Find(Key key, T &val) {
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

  ConHashMap::iterator Begin(int pos) {
    _vctLock[pos]->lock();
    return _vctMap[pos]->begin();
  }

  ConHashMap::iterator End(int pos) { return _vctMap[pos]->end(); }

  void Unlock(int pos) { _vctLock[pos]->unlock(); }

protected:
  vector<ConHashMap *> _vctMap;
  vector<SpinMutex *> _vctLock;
  int _groupCount;
};
} // namespace storage
