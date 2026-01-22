#include "../header.h"
#include "../utils/SpinMutex.h"

#include <set>
#include <unordered_map>

namespace storage {
using namespace std;

class MemLogMap
    : public unordered_map<Byte *, unordered_map<DT_MilliSec, std::string>> {
public:
  MemLogMap();
  ~MemLogMap();
};

class MemLog {
public:
  static void AddLog(Byte *addr, string &log);
  static void CollectLog();
  static void ClearObsoleLog();
  static unordered_map<Byte *, unordered_map<DT_MilliSec, std::string>>
  GetMapTotalLog() {
    return _mapTotalLog;
  }

protected:
  static thread_local MemLogMap _mapLocalLog;
  static unordered_map<Byte *, unordered_map<DT_MilliSec, std::string>>
      _mapTotalLog;

  static set<MemLogMap *> _setAddr;
  static SpinMutex _spinMutex;
  friend class MemLogMap;
};
} // namespace storage
