#pragma once
#include "../config/Configure.h"
#include "../utils/Utilitys.h"
#include <string>

namespace storage {
using namespace std;

class Database {
public:
  Database(int id, const MString &dbPath, const MString &dbName,
           DT_MilliSec dtCreate, DT_MilliSec dtLastUpdate)
      : _id(id), _dbPath(dbPath), _dbName(dbName), _dtCreate(dtCreate),
        _dtLastUpdate(dtLastUpdate) {
    if (_dbPath.size() == 0)
      _dbPath = Configure::GetDbRootPath();
  }
  const MString GetDbPath() const { return _dbPath; }
  const MString &GetDbName() const { return _dbName; }

protected:
  // The id start from 0 and increase 1 every time
  int _id;
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