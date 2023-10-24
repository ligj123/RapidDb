#pragma once
#include "../config/Configure.h"
#include "../utils/Utilitys.h"
#include <string>

namespace storage {
using namespace std;

class Database {
public:
  Database(MString rootPath, MString dbName, DT_MilliSec dtCreate,
           DT_MilliSec dtLastUpdate)
      : _rootPath(rootPath), _dbName(dbName), _dtCreate(dtCreate),
        _dtLastUpdate(dtLastUpdate) {
    if (_rootPath.size() == 0)
      _rootPath = Configure::GetDbRootPath();
  }
  const MString GetDbPath() { return _rootPath + "/" + _dbName; }
  const MString &GetRootPath() { return _rootPath; }
  const MString &GetDbName() { return _dbName; }

protected:
  // The root folder to save this database data.
  MString _rootPath;
  // Database name
  MString _dbName;
  // The create time for this database
  DT_MilliSec _dtCreate;
  // The last update time this database
  DT_MilliSec _dtLastUpdate;
};
} // namespace storage