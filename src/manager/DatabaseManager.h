
#include "../cache/Mallocator.h"
#include "../table/Database.h"
#include "../utils/SpinMutex.h"

namespace storage {
class DatabaseManager {
public:
  static const uint32_t FAST_SIZE;

public:
  static bool LoadDb();
  static bool AddDb();
  static bool DelDb();
  static bool ListDb(MVector<MString> &vctDb);
  static bool FindDb(MString db);
  static bool LockDb(bool bWait = true) {
    if (bWait) {
      _spinMutex.lock();
      return true;
    } else {
      return _spinMutex.try_lock();
    }
  }
  static void UnlockDb() {
    assert(_spinMutex.is_locked());
    _spinMutex.unlock();
  }

protected:
  static MTreeMap<MString, Database *> _mapDb;
  static SpinMutex _spinMutex;
  static Database *_fastDbCache;
};
} // namespace storage