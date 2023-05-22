#pragma once
#include "../utils/CharsetConvert.h"
#include <string>

namespace storage {
using namespace std;

class Database {
public:
  Database(string rootPath, string dbName) : _dbPath(dbPath), _dbName(dbName) {}
  const string GetDbPath() { return _rootPath + "/" + _dbName; }
  const string &GetRootPath() { return _rootPath; }
  const string &GetDbName() { return _dbName; }

protected:
  // The root folder to save this database data.
  string _rootPath;
  // Database name
  string _dbName;
};
} // namespace storage