#pragma once
#include "../config/Configure.h"
#include "../utils/ResStatus.h"
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
    _hash = MStrHash{}(_dbName);
    _dtLastVisit = MilliSecTime();
    if (_dbPath.size() == 0) {
      _dbPath = Configure::GetDbRootPath();
    }
  }

  const MString GetDbPath() const { return _dbPath + "/" + _dbName; }
  const MString &GetDbName() const { return _dbName; }
  void SetResStatus(ResStatus sts) { _dbStatus = sts; }
  ResStatus GetResStatus() const { return _dbStatus; }
  DT_MilliSec GetLastVisitTime() const { return _dtLastVisit; }
  void SetLastVisitTime() { _dtLastVisit = MilliSecTime(); }
  size_t Hash() const { return _hash; }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  // The id start from 0 and increase 1 every time
  int _id;
  // The root folder to save this database data.
  MString _dbPath;
  // Database name
  MString _dbName;
  // The create time for this database
  DT_MilliSec _dtCreate;
  // The last update time for this database
  DT_MilliSec _dtLastUpdate;
  // The last visit time this database
  DT_MilliSec _dtLastVisit;
  // If this database is valid and not be droped
  bool _bValid{true};
  // Database status
  ResStatus _dbStatus{ResStatus::Valid};
  // string hash of db name
  size_t _hash;
};
} // namespace storage