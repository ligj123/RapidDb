#pragma once
#include "../cache/Mallocator.h"
#include "SpinMutex.h"
#include <mutex>

namespace storage {
using namespace std;
template <class Key, class Val> class ConcurrentHashMap {
public:
  typedef std::unordered_map<Key, Val> ConHashMap;
  using Iterator = typename std::unordered_map<Key, Val>::iterator;

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

  int GetGroupCount() { return _groupCount; }

  bool Insert(Key key, Val val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(*_vctLock[pos]);
    return _vctMap[pos]->insert({key, val}).second;
  }

  bool Find(Key key, Val &val) {
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

  Iterator Begin(int pos) { return _vctMap[pos]->begin(); }
  Iterator End(int pos) { return _vctMap[pos]->end(); }
  void Erase(int pos, Iterator iter) { _vctMap[pos]->erase(iter); }
  void Erase(int pos, Key key) { _vctMap[pos]->erase(key); }

  void Lock(int pos) { _vctLock[pos]->lock(); }
  void Unlock(int pos) { _vctLock[pos]->unlock(); }

protected:
  vector<ConHashMap *> _vctMap;
  vector<SpinMutex *> _vctLock;
  int _groupCount;
};
} // namespace storage
