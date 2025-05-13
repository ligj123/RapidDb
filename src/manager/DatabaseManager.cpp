#include "DatabaseManager.h"

#include "../core/BranchPage.h"
#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../dataType/DataValueDateTime.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueVarChar.h"
#include "../utils/Log.h"

namespace storage {
const uint32_t DatabaseManager::FAST_SIZE = 31;
MTreeMap<MString, Database *> DatabaseManager::_mapDb;
SpinMutex DatabaseManager::_spinMutex;
Database *DatabaseManager::_fastDbCache[FAST_SIZE][4] = {};
vector<Database *> DatabaseManager::_discardDb;

bool DatabaseManager::InitDb(PhysTable *dbTable) {
  IndexTree *ptree = dbTable->GetPrimaryKey()._tree;
  BranchPage *bp = dynamic_cast<BranchPage *>(ptree->GetRootPage());
  LeafPage *lp = bp->GetLeftLeafChild();

  while (lp != nullptr) {
    uint32_t num = lp->GetRecordNumber();
    for (uint32_t i = 0; i < num; i++) {
      LeafRecord &lr = lp->GetRecord(i);
      VectorDataValue vdv;

      ReadResult rst = lr.ReadListValue({}, vdv, ptree);
      if (rst != ReadResult::OK_NOLOCK) {
        LOG_FATAL << "Failed to read list value for database information!";
        abort();
        return false;
      }

      Database *db = new Database((int)*(DataValueInt *)vdv[0],
                                  (MString) * (DataValueVarChar *)vdv[1],
                                  (MString) * (DataValueVarChar *)vdv[2],
                                  (DT_MilliSec) * (DataValueDateTime *)vdv[3],
                                  (DT_MilliSec) * (DataValueDateTime *)vdv[4]);
      _mapDb.insert({db->GetDbName(), db});
      AddFastDB(db);
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

bool DatabaseManager::AddDb(Database *db) {
  unique_lock<SpinMutex> lock(_spinMutex);
  if (_mapDb.find(db->GetDbName()) != _mapDb.end())
    return false;

  _mapDb.insert({db->GetDbName(), db});
  AddFastDB(db);
  return true;
}

bool DatabaseManager::RemoveDb(const MString &dbName, bool bDroped) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapDb.find(dbName);
  if (iter == _mapDb.end())
    return false;

  Database *db = iter->second;
  db->SetResStatus(bDroped ? ResStatus::Droped : ResStatus::Obsolete);

  Database **pArrDb = _fastDbCache[db->Hash() % FAST_SIZE];
  for (size_t i = 0; i < 4; i++) {
    if (pArrDb[i] == db) {
      pArrDb[i] = nullptr;
      break;
    }
  }

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
  Database **pArrDb = _fastDbCache[hash % FAST_SIZE];
  int epos = -1;
  for (int i = 0; i < 4; i++) {
    if (pArrDb[i] != nullptr) {
      if (pArrDb[i]->GetDbName() == dbName) {
        pArrDb[i]->SetLastVisitTime();
        return pArrDb[i];
      }
    } else if (epos < 0) {
      epos = i;
    }
  }

  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapDb.find(dbName);
  if (iter == _mapDb.end()) {
    return nullptr;
  } else {
    if (epos >= 0) {
      pArrDb[epos] = iter->second;
    }

    iter->second->SetLastVisitTime();
    return iter->second;
  }
}

void DatabaseManager::ClearDB() {
  for (auto iter = _mapDb.begin(); iter != _mapDb.end(); iter++) {
    delete iter->second;
  }

  _mapDb.clear();
  for (int i = 0; i < FAST_SIZE; i++) {
    _fastDbCache[i][0] = nullptr;
    _fastDbCache[i][1] = nullptr;
    _fastDbCache[i][2] = nullptr;
    _fastDbCache[i][3] = nullptr;
  }

  _discardDb.clear();
  _discardDb.resize(0);
}

void DatabaseManager::AddFastDB(Database *db) {
  DT_MicroSec lv = UINT64_MAX;
  size_t lvPos = 0;
  Database **pArrDb = _fastDbCache[db->Hash() % FAST_SIZE];
  for (int i = 0; i < 4; i++) {
    if (pArrDb[i] == nullptr) {
      pArrDb[i] = db;
      return;
    } else if (lv > pArrDb[i]->GetLastVisitTime()) {
      lvPos = i;
      lv = pArrDb[i]->GetLastVisitTime();
    }
  }

  assert(lv != UINT64_MAX);
  LOG_WARN << "Replace fast db cache " << pArrDb[lvPos]->GetDbName() << " by "
           << db->GetDbName();
  pArrDb[lvPos] = db;
}
} // namespace storage