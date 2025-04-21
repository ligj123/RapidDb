
#include "TableManager.h"

#include "../core/BranchPage.h"
#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../dataType/DataValueBlob.h"
#include "../dataType/DataValueVarChar.h"
#include "../serv/SessionPool.h"
#include "../table/TableTaskMgr.h"
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
        const Byte *bys = dynamic_cast<DataValueBlob *>(vdv[3])->GetBuff();
        if (!tbl->LoadData(bys)) {
          delete tbl;
          LOG_FATAL << "Failed to load data for table information!";
          return false;
        }

        TableTaskMgr *tmgr =
            new TableTaskMgr(ThreadPool::GetMainPool(), tbl,
                             SessionPool::GetVctSessionGroup().size(), false);
        tbl->SetTableTaskMgr(tmgr);

      } else {
        MString dbName = (MString)(*dynamic_cast<DataValueVarChar *>(vdv[1]));
        MString tblName = (MString)(*dynamic_cast<DataValueVarChar *>(vdv[2]));
        Database *db = DatabaseManager::FindDb(dbName);
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
    LOG_ERROR << "The table has exist, name = " << table->GetFullName();
    return false;
  }

  _mapTable.insert({table->GetFullName(), table});
  AddFastTable(table);
  return true;
}

bool TableManager::RemoveTable(const MString &tblFullName) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(tblFullName);
  if (iter == _mapTable.end()) {
    return false;
  }

  PhysTable *tbl = iter->second;
  tbl->SetTableStatus(ResStatus::Obsolete);

  PhysTable **pArrTbl = _fastTableCache[tbl->Hash() % FAST_SIZE];
  for (int i = 0; i < 4; i++) {
    if (pArrTbl[i] == tbl) {
      pArrTbl[i] = nullptr;
      break;
    }
  }

  _discardTable.push_back(tbl);
  _mapTable.erase(iter);
  return true;
}

bool TableManager::FindTable(const MString &tblFullName, PhysTable *&tbl) {
  size_t hash = MStrHash{}(tblFullName);
  PhysTable **pArrTbl = _fastTableCache[hash % FAST_SIZE];
  int epos = -1;
  for (int i = 0; i < 4; i++) {
    if (pArrTbl[i] != nullptr) {
      if (pArrTbl[i]->GetFullName() == tblFullName) {
        tbl = pArrTbl[i];
        if (tbl->GetTableStatus() == ResStatus::Uninit) {
          LoadTable(tbl);
        }

        return true;
      }
    } else if (epos < 0) {
      epos = i;
    }
  }

  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapTable.find(tblFullName);
  if (iter == _mapTable.end()) {
    return false;
  } else {
    tbl = iter->second;
    if (epos >= 0) {
      pArrTbl[epos] = tbl;
    }

    if (tbl->GetTableStatus() == ResStatus::Uninit) {
      LoadTable(tbl);
    }
    return true;
  }
}

bool TableManager::ListTables(const MString &dbName, MVector<MString> &vctTbl) {
  assert(vctTbl.size() == 0);
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
  for (size_t i = 0; i < FAST_SIZE; i++) {
    _fastTableCache[i][0] = nullptr;
    _fastTableCache[i][1] = nullptr;
    _fastTableCache[i][2] = nullptr;
    _fastTableCache[i][3] = nullptr;
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
  pArrTbl[lvPos] = table;
}

void TableManager::LoadTable(PhysTable *table) {
  // TO DO
  abort();
}
} // namespace storage