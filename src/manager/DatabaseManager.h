
#include "../cache/Mallocator.h"
#include "../table/Database.h"
#include "../table/Table.h"
#include "../utils/SpinMutex.h"

namespace storage {
class DatabaseManager {
public:
  static const uint32_t FAST_SIZE;

public:
  /**
   * @brief Init database manager, Load all databases information from disk.
   * @param dbTable The system table database
   * @return true: init successful, false: init fail
   */
  static bool InitDb(PhysTable *dbTable);
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
  static MStrTreeMap<Database *> _mapDb;
  static SpinMutex _spinMutex;
  // Allocate fixed spaces when program initialization and save database pointer
  // into it according db name' hash remainder if the position is nullptr.
  static vector<Database *> _fastDbCache;
  // To temporary save the dropped database.
  static vector<Database *> _discardDb;
};
} // namespace storage