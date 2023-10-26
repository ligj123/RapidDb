#pragma once
#include "../config/Configure.h"
#include "../utils/Utilitys.h"
#include <string>

namespace storage {
using namespace std;

class Database {
public:
  Database(MString dbPath, MString dbName, DT_MilliSec dtCreate,
           DT_MilliSec dtLastUpdate)
      : _dbPath(dbPath), _dbName(dbName), _dtCreate(dtCreate),
        _dtLastUpdate(dtLastUpdate) {
    if (_rootPath.size() == 0)
      _rootPath = Configure::GetDbRootPath();
  }
  const MString GetDbPath() { return _dbPath; }
  const MString &GetRootPath() { return _rootPath; }
  const MString &GetDbName() { return _dbName; }

protected:
  // The root folder to save this database data.
  MString _dbPath;
  // Database name
  MString _dbName;
  // The create time for this database
  DT_MilliSec _dtCreate;
  // The last update time this database
  DT_MilliSec _dtLastUpdate;
  // If this database is valid and not be droped
  bool _bValid{true};
};
} // namespace storage