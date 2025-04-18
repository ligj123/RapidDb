
#include "TableManager.h"

#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../dataType/DataValueBlob.h"
#include "../dataType/DataValueVarChar.h"
#include "../utils/Log.h"
#include "DatabaseManager.h"

namespace storage {
const uint32_t TableManager::FAST_SIZE = 255;
MTreeMap<MString, PhysTable *> TableManager::_mapTable;
SpinMutex TableManager::_spinMutex;
PhysTable *TableManager::_fastTableCache[FAST_SIZE][4] = {};
vector<PhysTable *> TableManager::_discardTable;
bool TableManager::_bAllInMemory{true};

bool TableManager::InitTable(PhysTable *sysTable) {
  IndexTree *ptree = sysTable->GetPrimaryKey()._tree;
  BranchPage *bp = dynamic_cast<BranchPage *>(ptree->GetRootPage());
  LeafPage *lp = bp->GetLeftLeafChild();

  while (lp != nullptr) {
    uint32_t num = lp->GetRecordNumber();
    for (uint32_t i = 0; i < num; i++) {
      LeafRecord &lr = lp->GetRecord(i);
      VectorDataValue vdv;
      ReadResult rst = lr.ReadListValue({}, vdv, ptree);
      if (rst != ReadResult::OK_NOLOCK) {
        LOG_FATAL << "Failed to read list value for table information!";
        abort();
        return false;
      }

      PhysTable *tbl;
      if (_bAllInMemory) {
        tbl = new PhysTable();
        Byte *bys = ((DataValueBlob *)vdv[3])->GetBuff();
        if (!tbl->LoadData(bys)) {
          delete tbl;
          LOG_FATAL << "Failed to load data for table information!";
          return false;
        }
      } else {
        MString dbName = (MString)(*dynamic_cast<DataValueVarChar *>(vdv[1]));
        MString tblName = (MString)(*dynamic_cast<DataValueVarChar *>(vdv[2]));
        Database *db = DatabaseManager::FindDb(db_name);
        assert(db != nullptr);

        tbl = new PhysTable(db, tblName, vdv[0]->GetLong(), 0, 0,
                            ResStatus::Uninit);
      }

      _mapTable.insert({tbl->GetFullName(), tbl});
      AddFastTable(tbl);
    }

    lp = lp->GetNextPage();
    if (lp == nullptr) {
      break;
    }

    while (lp->GetPageStatus() != PageStatus::VALID) {
      this_thread::yield();
    }
  }

  return true;
}

bool TableManager::AddTable(PhysTable *table) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(table->GetFullName());

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
  _mapTable.erase(iter);
  return true;
}

bool TableManager::FindTable(const MString &tblName, PhysTable *&tbl) {
  size_t hash = MStrHash{}(tblName);
  if (_fastTableCache[hash % FAST_SIZE] != nullptr &&
      _fastTableCache[hash % FAST_SIZE]->GetFullName() == tblName) {
    tbl = _fastTableCache[hash % FAST_SIZE];
    return true;
  }

  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(tblName);
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

void TableManager::ClearTable() {
  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto iter = _mapTable.begin(); iter != _mapTable.end(); iter++) {
    delete iter->second;
  }

  _mapTable.clear();
  for (size_t i = 0; i < _fastTableCache.size(); i++) {
    _fastTableCache[i] = nullptr;
  }

  for (PhysTable *table : _discardTable) {
    delete table;
  }

  _discardTable.clear();
}

void TableManager::AddFastTable(PhysTable *table) {
  DT_MicroSec lv = UINT64_MAX;
  size_t lvPos = 0;
  PhysTable **pArrTbl = _fastTableCache[table->Hash() % FAST_SIZE];
  for (int i = 0; i < 4; i++) {
    if (pArrTbl[i] == nullptr) {
      pArrTbl[i] = table;
      return;
    } else if (lv > pArrTbl[i]->GetLastVisitTime()) {
      lvPos = i;
      lv = pArrTbl[i]->GetLastVisitTime();
    }
  }

  assert(lv != UINT64_MAX);
  LOG_WARN << "Replace fast table cache " << pArrTbl[lvPos]->GetFullName()
           << " by " << table->GetFullName();
  pArrDb[lvPos] = db;
}
} // namespace storage