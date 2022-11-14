#pragma once
#include <string>

namespace storage {
using namespace std;
struct Database {
  Database(string dbPath, string dbName) : _dbPath(dbPath), _dbName(dbName) {}
  // The path to save this database path.
  string _dbPath;
  // Database name
  string _dbName;
};
} // namespace storage