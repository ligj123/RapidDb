#pragma once
#include "../utils/CharsetConvert.h"
#include <string>

namespace storage {
using namespace std;

class Database {
public:
  Database(MString rootPath, MString dbName)
      : _rootPath(rootPath), _dbName(dbName) {}
  const MString GetDbPath() { return _rootPath + "/" + _dbName; }
  const MString &GetRootPath() { return _rootPath; }
  const MString &GetDbName() { return _dbName; }

protected:
  // The root folder to save this database data.
  MString _rootPath;
  // Database name
  MString _dbName;
};
} // namespace storage