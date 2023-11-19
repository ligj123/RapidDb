
#include "../cache/Mallocator.h"
#include "../table/Database.h"
#include "../table/Table.h"
#include "../utils/SpinMutex.h"

namespace storage {
class DatabaseManager {
public:
  static const uint32_t FAST_SIZE;

public:
  static bool LoadDb(PhysTable *dbTable);
  static bool AddDb(Database *db);
  static bool DelDb(MString dbName);
  static bool ListDb(MVector<MString> &vctDb);
  static Database *FindDb(MString db);
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
  // Allocate fixed spaces when program initialization and save database pointer
  // into it according db name' hash remainder if the position is nullptr.
  static vector<Database *> _fastDbCache;
  // To temporary save the dropped database.
  static vector<Database *> _discardDb;
};
} // namespace storage