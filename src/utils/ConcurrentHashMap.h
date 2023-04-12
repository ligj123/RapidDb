#pragma once
#include "../cache/Mallocator.h"
#include "SpinMutex.h"
#include <functional>
#include <mutex>

namespace storage {
using namespace std;
template <class Key, class Val, bool UseFunc = true> class ConcurrentHashMap {
public:
  using ConHashMap = std::unordered_map<Key, Val>;
  using Iterator = typename std::unordered_map<Key, Val>::iterator;

  ConcurrentHashMap(int groupCount, uint64_t maxCount,
                    function<void(Val)> funcFind = nullptr,
                    function<void(Val)> funcErase = nullptr)
      : _groupCount(groupCount), _funcFind(funcFind), _funcErase(funcErase) {
    assert((UseFunc && funcFind != nullptr && funcErase != nullptr) ||
           (!UseFunc && funcFind == nullptr && funcErase == nullptr));
    _vctMap.reserve(groupCount);
    _vctLock.reserve(groupCount);
    for (int i = 0; i < groupCount; i++) {
      _vctMap.push_back(new ConHashMap(maxCount / groupCount));
      _vctLock.push_back(new SpinMutex());
    }
  }

  ~ConcurrentHashMap() {
    assert(Size() == 0);

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

  bool Find(const Key &key, Val &val) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(*_vctLock[pos]);
    auto iter = _vctMap[pos]->find(key);
    if (iter == _vctMap[pos]->end()) {
      return false;
    } else {
      val = iter->second;

      if constexpr (UseFunc) {
        _funcFind(val);
      }
      return true;
    }
  }

  Iterator Begin(int pos) { return _vctMap[pos]->begin(); }
  Iterator End(int pos) { return _vctMap[pos]->end(); }
  Iterator Erase(int pos, Iterator iter) {
    if constexpr (UseFunc) {
      _funcErase(iter->second);
    }
    return _vctMap[pos]->erase(iter);
  }
  bool Erase(int pos, const Key &key) {
    if constexpr (UseFunc) {
      auto iter = _vctMap[pos]->find(key);
      if (iter == _vctMap[pos]->end())
        return false;

      _funcErase(iter->second);
      _vctMap[pos]->erase(iter);
      return true;
    } else {
      size_t n = _vctMap[pos]->erase(key);
      return (n > 0);
    }
  }
  bool Erase(const Key &key) {
    int pos = std::hash<Key>{}(key) % _groupCount;
    unique_lock<SpinMutex> lock(*_vctLock[pos]);
    return Erase(pos, key);
  }
  void Clear(int pos) {
    _vctLock[pos]->lock();
    ConHashMap *map = _vctMap[pos];
    if constexpr (UseFunc) {
      for (auto iter = map->begin(); iter != map->end(); iter++) {
        _funcErase(iter->second);
      }
    }
    map->clear();
    _vctLock[pos]->unlock();
  }

  void Lock(int pos) { _vctLock[pos]->lock(); }
  void Unlock(int pos) { _vctLock[pos]->unlock(); }

protected:
  vector<ConHashMap *> _vctMap;
  vector<SpinMutex *> _vctLock;
  int _groupCount;
  // The lambad called in method find when find and return the key
  function<void(Val)> _funcFind = nullptr;
  // The lambad called in method find when erase the key
  function<void(Val)> _funcErase = nullptr;
};
} // namespace storage
