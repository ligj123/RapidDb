
#include "TableManager.h"

#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../dataType/DataValueBlob.h"

namespace storage {
const uint32_t TableManager::FAST_SIZE = 1023;
MStrTreeMap<PhysTable *> TableManager::_mapTable;
SpinMutex TableManager::_spinMutex;
vector<PhysTable *> TableManager::_fastTableCache(FAST_SIZE, nullptr);
vector<PhysTable *> TableManager::_discardTable;
bool TableManager::_bAllInMemory{true};

bool TableManager::InitTable(PhysTable *sysTable) {
  if (!_bAllInMemory)
    return;

  IndexTree *ptree = dbTable->GetPrimaryKey()._tree;
  LeafPage *lp = ptree->GetBeginPage();

  while (lp != nullptr) {
    uint32_t num = lp->GetRecordNumber();
    for (uint32_t i = 0; i < num; i++) {
      LeafRecord &lr = lp->GetRecord(i);
      VectorDataValue vdv;
      ReadResult rst = lr.ReadListValue({}, vdv, lp);
      if (rst != ReadResult::OK) {
        LOG_FATAL << "Failed to read list value for table information!";
        return false;
      }

      PhysTable *tbl = new PhysTable();
      Byte *bys = ((DataValueBlob *)vdv[2])->GetBuff();
      if (!tbl->LoadData(bys)) {
        delete tbl;
        LOG_FATAL << "Failed to load data for table information!";
        return false;
      }
      _mapTable.insert({tbl->GetFullName(), tbl});
      size_t hash = MStrHash{}(db->GetDbName());
      _fastTableCache[hash % FAST_SIZE] = tbl;
    }

    PageID pid = lp->GetNextPageId();
    lp->DecRef();
    if (pid == PAGE_NULL_POINTER)
      break;

    lp = (LeafPage *)ptree->GetPage(pid, PageType::LEAF_PAGE, true);
  }

  return true;
}

bool TableManager::AddTable(const MString &tblName, PhysTable *table) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(tblName);

  if (iter != _mapTable.end()) {
    LOG_ERROR << "The table has exist, name = " << tblName;
    return false;
  }

  _mapTable.insert({tblName, table});
  size_t hash = MStrHash{}(tblName);
  _fastTableCache[hash % FAST_SIZE] = table;

  return true;
}

bool TableManager::RemoveTable(const MString &tblName) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(tblName);
  if (iter == _mapTable.end())
    return false;

  PhysTable *tbl = iter->second;
  tbl->SetTableStatus(ResStatus::Obsolete);
  size_t hash = MStrHash{}(tblName);
  if (_fastTableCache[hash % FAST_SIZE] == tbl)
    _fastTableCache[hash % FAST_SIZE] = nullptr;

  _discardTable.push_back(tbl);
  _mapDb.erase(iter);
  return true;
}

bool TableManager::FindTable(const MString &tblName, PhysTable *&tbl) {
  size_t hash = MStrHash{}(dbName);
  if (_fastTableCache[hash % FAST_SIZE] != nullptr &&
      _fastTableCache[hash % FAST_SIZE]->GetFullName() == tblName) {
    tbl = _fastDbCache[hash % FAST_SIZE];
    return true;
  }

  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(dbName);
  if (iter == _mapTable.end()) {
    if (_bAllInMemory) {
      tbl = nullptr;
      return false;
    } else {
      tbl = new PhysTable();
      tbl->SetTableStatus(ResStatus::Uninit);
      _mapTable.insert({tblName, tbl});
      return true;
    }
  } else {
    tbl = iter->second;
    return true;
  }
}

bool TableManager::ListTables(const MString &dbName, MVector<MString> &vctTbl) {
  assert(vctTbl.size() == 0);
  if (!_bAllInMemory) {
    return false;
  }

  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto iter = _mapTable.begin(); iter != _mapTable.end(); iter++) {
    if (iter->second->GetDbName() != dbName)
      continue;

    vctTbl.push_back(iter->second->GetTableName());
  }

  return true;
}
} // namespace storage