#include "DatabaseManager.h"

namespace storage {
const uint32_t DatabaseManager::FAST_SIZE = 256;
static inline Database *GenNullPointer() {
  Database *db =
      (Database *)new char[sizeof(Database *) * DatabaseManager::FAST_SIZE];
  memset(db, 0, sizeof(Database *) * DatabaseManager::FAST_SIZE);
  return db;
}

MHashMap<MString, Database *> DatabaseManager::_mapDb;
SpinMutex DatabaseManager::_spinMutex;
Database *DatabaseManager::_fastDbCache = GenNullPointer();
} // namespace storage