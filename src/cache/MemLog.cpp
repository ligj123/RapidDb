#include "MemLog.h"
#include "../utils/Utilitys.h"

#include <mutex>

namespace storage {
thread_local MemLogMap MemLog::_mapLocalLog;
unordered_map<Byte *, unordered_map<DT_MilliSec, std::string>>
    MemLog::_mapTotalLog;
set<MemLogMap *> MemLog::_setAddr;

MemLogMap::MemLogMap() {
  std::unique_lock<SpinMutex> lock(MemLog::_spinMutex);
  MemLog::_setAddr.insert(this);
}

MemLogMap::~MemLogMap() {
  std::unique_lock<SpinMutex> lock(MemLog::_spinMutex);
  for (auto itLocal = begin(); itLocal != end(); itLocal++) {
    unordered_map<DT_MilliSec, std::string> &mapTs =
        MemLog::_mapTotalLog.try_emplace(itLocal->first).first->second;
    for (auto itTs = itLocal->second.begin(); itTs != itLocal->second.end();
         itTs++) {
      auto res = mapTs.emplace(itTs->first, itTs->second);
      assert(res.second);
    }
  }
}

void MemLog::AddLog(Byte *addr, string &log) {
  DT_MicroSec ts = MilliSecTime();
  auto &mapTs = _mapLocalLog.try_emplace(addr).first->second;
  auto res = mapTs.emplace(ts, std::move(log));
  assert(res.second);
}

/**
 * @brief Collect logs from thread local map and save them into total map. It
 * should be called when NO logs are inserted into thread local map by AddLog.
 */
void MemLog::CollectLog() {
  std::unique_lock<SpinMutex> lock(_spinMutex);
  for (MemLogMap *mlm : _setAddr) {
    for (auto itLocal = mlm->begin(); itLocal != mlm->end(); itLocal++) {
      unordered_map<DT_MilliSec, std::string> &mapTs =
          MemLog::_mapTotalLog.try_emplace(itLocal->first).first->second;
      for (auto itTs = itLocal->second.begin(); itTs != itLocal->second.end();
           itTs++) {
        auto res = mapTs.emplace(itTs->first, itTs->second);
        assert(res.second);
      }
    }
  }
}

void MemLog::ClearObsoleLog() {
  std::unique_lock<SpinMutex> lock(_spinMutex);
  for (auto itAddr = _mapTotalLog.begin(); itAddr != _mapTotalLog.end();) {
    auto &mapTs = itAddr->second;

    auto itTs = mapTs.begin();
    auto itTs2 = itTs;
    itTs++;
    for (; itTs != mapTs.end();) {
      if (itTs2->second.find("Apply") != string::npos &&
          itTs->second.find("Release") != string::npos) {
        mapTs.erase(itTs2);
        itTs2 = mapTs.erase(itTs);
        if (itTs2 == mapTs.end()) {
          break;
        }

        itTs = itTs2;
        itTs++;
      } else {
        itTs2++;
        itTs++;
      }
    }
    itAddr++;
  }
}
} // namespace storage