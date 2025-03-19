#include "DatabaseManager.h"

#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../dataType/DataValueDateTime.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueVarChar.h"
#include "../utils/Log.h"

namespace storage {
const uint32_t DatabaseManager::FAST_SIZE = 127;
MStrTreeMap<Database *> DatabaseManager::_mapDb;
SpinMutex DatabaseManager::_spinMutex;
Database *DatabaseManager::_fastDbCache[FAST_SIZE] = {};
vector<Database *> DatabaseManager::_discardDb;

bool DatabaseManager::InitDb(PhysTable *dbTable) {
  IndexTree *ptree = dbTable->GetPrimaryKey()._tree;
  LeafPage *lp = ptree->GetBeginPage();

  while (lp != nullptr) {
    uint32_t num = lp->GetRecordNumber();
    for (uint32_t i = 0; i < num; i++) {
      // const LeafRecord &lr = lp->GetRecord(i);
      // VectorDataValue vdv;

      // ReadResult rst = lr.ReadListValue({}, vdv, lp);
      // if (rst != ReadResult::OK) {
      //   LOG_FATAL << "Failed to read list value for database information!";
      //   return false;
      // }

      // Database *db = new Database((int)*(DataValueInt *)vdv[0],
      //                             (MString) * (DataValueVarChar *)vdv[1],
      //                             (MString) * (DataValueVarChar *)vdv[2],
      //                             (DT_MilliSec) * (DataValueDateTime
      //                             *)vdv[3], (DT_MilliSec) *
      //                             (DataValueDateTime *)vdv[4]);
      // _mapDb.insert({db->GetDbName(), db});
      // size_t hash = MStrHash{}(db->GetDbName());
      // _fastDbCache[hash % FAST_SIZE] = db;
    }

    PageID pid = lp->GetNextPageId();
    // lp->DecRef();
    if (pid == PAGE_NULL_POINTER)
      break;

    lp = (LeafPage *)ptree->GetPage(pid, PageType::LEAF_PAGE);
  }

  return true;
}

bool DatabaseManager::AddDb(Database *db) {
  unique_lock<SpinMutex> lock(_spinMutex);
  if (_mapDb.find(db->GetDbName()) != _mapDb.end())
    return false;

  _mapDb.insert({db->GetDbName(), db});
  size_t hash = MStrHash{}(db->GetDbName());
  _fastDbCache[hash % FAST_SIZE] = db;

  return true;
}

bool DatabaseManager::DelDb(const MString &dbName) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapDb.find(dbName);
  if (iter == _mapDb.end())
    return false;

  Database *db = iter->second;
  db->SetResStatus(ResStatus::Obsolete);
  size_t hash = MStrHash{}(dbName);
  if (_fastDbCache[hash % FAST_SIZE] == db)
    _fastDbCache[hash % FAST_SIZE] = nullptr;

  _discardDb.push_back(db);
  _mapDb.erase(iter);
  return true;
}

bool DatabaseManager::ListDb(MVector<MString> &vctDb) {
  assert(vctDb.size() == 0);
  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto &iter : _mapDb) {
    vctDb.push_back(iter.first);
  }

  return true;
}

Database *DatabaseManager::FindDb(const MString &dbName) {
  size_t hash = MStrHash{}(dbName);
  if (_fastDbCache[hash % FAST_SIZE] != nullptr &&
      _fastDbCache[hash % FAST_SIZE]->GetDbName() == dbName) {
    return _fastDbCache[hash % FAST_SIZE];
  }

  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapDb.find(dbName);
  if (iter == _mapDb.end())
    return nullptr;
  else
    return iter->second;
}

void DatabaseManager::ClearDB() {
  for (auto iter = _mapDb.begin(); iter != _mapDb.end(); iter++) {
    delete iter->second;
  }

  _mapDb.clear();
  for (size_t i = 0; i < FAST_SIZE; i++) {
    _fastDbCache[i] = nullptr;
  }

  _discardDb.clear();
}
} // namespace storage