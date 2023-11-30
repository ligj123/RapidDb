#include "DatabaseManager.h"
#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../dataType/DataValueDateTime.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueVarChar.h"

namespace storage {
const uint32_t DatabaseManager::FAST_SIZE = 127;
MTreeMap<MString, Database *> DatabaseManager::_mapDb;
SpinMutex DatabaseManager::_spinMutex;
vector<Database *> DatabaseManager::_fastDbCache(FAST_SIZE, nullptr);
vector<Database *> DatabaseManager::_discardDb;

bool DatabaseManager::LoadDb(PhysTable *dbTable) {
  IndexTree *ptree = dbTable->GetPrimaryKey()._tree;
  LeafPage *lp = ptree->GetBeginPage();

  while (lp != nullptr) {
    uint32_t num = lp->GetRecordNumber();
    for (uint32_t i = 0; i < num; i++) {
      LeafRecord *lr = lp->GetRecord(i);
      VectorDataValue vdv;
      if (lr->GetListValue(vdv) != 0) {
        abort();
      }

      Database *db = new Database((int)*(DataValueInt *)vdv[0],
                                  (MString) * (DataValueVarChar *)vdv[1],
                                  (MString) * (DataValueVarChar *)vdv[2],
                                  (DT_MilliSec) * (DataValueDateTime *)vdv[3],
                                  (DT_MilliSec) * (DataValueDateTime *)vdv[4]);
      _mapDb.insert({db->GetDbName(), db});
      size_t hash = MStrHash{}(db->GetDbName());
      assert(_fastDbCache[hash % FAST_SIZE] == nullptr);
      _fastDbCache[hash % FAST_SIZE] = db;
    }

    PageID pid = lp->GetNextPageId();
    lp->DecRef();
    if (pid == PAGE_NULL_POINTER)
      break;

    lp = (LeafPage *)ptree->GetPage(pid, PageType::LEAF_PAGE, true);
  }

  return true;
}

bool DatabaseManager::AddDb(Database *db) {
  unique_lock<SpinMutex> lock(_spinMutex);
  if (_mapDb.find(db->GetDbName()) != _mapDb.end())
    return false;

  _mapDb.insert({db->GetDbName(), db});
  size_t hash = MStrHash{}(db->GetDbName());
  if (_fastDbCache[hash % FAST_SIZE] == nullptr)
    _fastDbCache[hash % FAST_SIZE] = db;

  return true;
}

bool DatabaseManager::DelDb(MString dbName) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapDb.find(dbName);
  if (iter == _mapDb.end())
    return false;

  Database *db = iter->second;
  db->SetDropped();
  size_t hash = MStrHash{}(dbName);
  if (_fastDbCache[hash % FAST_SIZE] == nullptr)
    _fastDbCache[hash % FAST_SIZE] = db;

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

Database *DatabaseManager::FindDb(MString dbName) {
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

} // namespace storage