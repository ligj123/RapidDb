#include "TableTestLib.h"
#include "../src/manager/DatabaseManager.h"
#include "../src/manager/TableManager.h"
#include "../src/serv/SessionPool.h"
#include "../src/statement/StmtResult.h"
#include "../src/table/TableTaskMgr.h"

namespace storage {
const char *ROOT_PATH = "./TestPress";
const char *DB_NAME = "dbTest";
const char *TBL_NAME = "tableTest";
const char *DB_TBL_NAME = "dbTest.tableTest";
const char *INSERT_STMT = "insert into tableTest values(?, ?, ?)";
const char *UPDATE_STMT = "update tableTest set c2=c2+1 where c1=?";
const char *UPDATE_STMT2 = "update tableTest set c2=c2-1 where c1=?";
const char *DELETE_STMT = "delete from tableTest where c1=?";
const char *SELECT_STMT = "select * from tableTest where c1=?";
thread_local string varchar = "VARCHAR_50_" + string(40, 'a');

OpRedio arrRadio[] = {OpRedio::INS, OpRedio::UPD, OpRedio::DEL, OpRedio::SEL,
                      OpRedio::SEL, OpRedio::SEL, OpRedio::SEL, OpRedio::SEL,
                      OpRedio::SEL, OpRedio::SEL};
int redioCount = sizeof(arrRadio);

void CheckSelectResult(uint32_t num, VectorDataValue &vctDv) {
  uint32_t val = GenTestKey(num);
  int64_t pkval = GenPrimaryKey(num);

  if (vctDv[0]->GetLong() != pkval) {
    LOG_ERROR << num << "  Error first field value, expect value: " << pkval
              << "  actual value: " << vctDv[0]->GetLong();
  }

  int secVal = (int32_t)BytesSwap32(val) + (arrResult[num] & 0x7f);
  if (vctDv[1]->GetLong() != secVal) {
    LOG_ERROR << num
              << "  Error secondary field value, expect value: " << secVal
              << "  actual value: " << vctDv[1]->GetLong();
  }

  sprintf(varchar.data() + 30, "0x%08X", (val / 10));
  const Byte *p = (const Byte *)varchar.c_str();
  if (BytesCompare(p, strlen(varchar.c_str()) + 1, vctDv[2]->GetBuff(),
                   vctDv[2]->GetDataLength()) != 0) {
    LOG_ERROR << num
              << "  Error third field value, expect value: " << varchar.c_str()
              << "  actual value: " << (const char *)vctDv[2]->GetBuff();
  }
}

void GetRecordValue(const string &fullTblName, int num) {
  PhysTable *table;
  bool b = TableManager::FindTable(fullTblName.c_str(), table);
  assert(b);
  int64_t val = GenTestKey(num);
  DataValueLong *dvLong = new DataValueLong(val * val + val);
  RawKey key({dvLong});

  IndexTree *idxTree = table->GetVectorIndex()[0]._tree;
  IndexPage *idxPage = idxTree->GetRootPage();
  b = idxTree->SearchPage(key, idxPage);
  assert(b);

  LeafPage *lpage = dynamic_cast<LeafPage *>(idxPage);
  bool bFind;
  int pos = lpage->SearchKey(key, bFind);
  if (!bFind) {
    LOG_INFO << "Failed to find num: " << num;
    return;
  }

  LeafRecord &lr = lpage->GetRecord(pos);
  VectorDataValue vctDv;
  ReadResult rr = lr.ReadListValue({}, vctDv, idxTree);
  if (rr == ReadResult::REC_DELETE) {
    LOG_INFO << "Delete Num: " << num;
    return;
  }
  LOG_INFO << "C1: " << vctDv[0]->GetLong() << "\tC2: " << vctDv[1]->GetLong()
           << "\tc3: " << (const char *)vctDv[2]->GetBuff();
}

void CheckAllRecord(const string &fullTblName, int rowNum) {
  PhysTable *table;
  bool b = TableManager::FindTable(fullTblName.c_str(), table);
  assert(b);

  IndexTree *idxTree = table->GetVectorIndex()[0]._tree;
  LeafPage *lpage = idxTree->GetBeginPage();

  MTreeMap<int64_t, int> map;
  for (int i = 0; i < rowNum; i++) {
    map.emplace(GenPrimaryKey(i), i);
  }

  int cnt = 0;
  auto iter = map.begin();
  auto itOld = iter;

  while (lpage != nullptr) {
    for (int i = 0; i < lpage->GetRecordNumber(); i++) {
      LeafRecord &lr = lpage->GetRecord(i);
      if (lr.ReleaseLockAble()) {
        lr.ReleaseLock(idxTree);
      }

      while (true) {
        RawKey key({new DataValueLong(iter->first)});
        int hr = lr.CompareKey(key);
        assert(hr >= 0);
        if (hr == 0)
          break;

        iter++;
        cnt++;
      }

      VectorDataValue vctDv;
      ReadResult rr = lr.ReadListValue({}, vctDv, idxTree);
      if (rr == ReadResult::REC_DELETE) {
        assert((arrResult[iter->second] & 0x80) == 0);
      } else {
        CheckSelectResult(iter->second, vctDv);
      }
      cnt++;
      itOld = iter;
      iter++;
    }

    lpage = lpage->GetNextPage();
  }

  LOG_INFO << "cnt: " << cnt;
}

void CreateDbTable(const string &dbName, bool bExclusive, int sessionGroup) {
  Database *db = new Database(1, ROOT_PATH, dbName.c_str(), MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);

  PhysTable *ptable =
      new PhysTable(db, TBL_NAME, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::LONG, false, -1, "primary key",
                    Charsets::UNKNOWN, nullptr);
  ptable->AddColumn("c2", DataType::INT, false, -1, "Unique Key",
                    Charsets::UNKNOWN, nullptr);
  ptable->AddColumn("c3", DataType::VARCHAR, true, 50, "NonUnique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});

  bool b = ptable->OpenIndex(0, true);
  assert(b);

  TableTaskMgr *tmgr = new TableTaskMgr(ThreadPool::GetMainPool(), ptable,
                                        sessionGroup, bExclusive);
  ptable->SetTableTaskMgr(tmgr);
  string fullName = dbName + "." + TBL_NAME;
  TableManager::AddTable(fullName.c_str(), ptable);
}

void InsertProc(uint16_t tid, const MVector<uint32_t> &vctSessId, int recStart,
                int recNum) {

  MVector<StmtResult> vctResult(vctSessId.size());
  MVector<int> vctStmtId(vctSessId.size());

  int cnt = 0;
  int times = 0;
  while (true) {
    int unfinished = 0;
    times++;
    for (size_t i = 0; i < vctResult.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs == ResultStatus::FILLING) {
        unfinished++;
        continue;
      }

      if (rs == ResultStatus::FINISHED) {
        assert(vctResult[i]._rowNum == 1 && vctResult[i]._vctError.size() == 0);
        assert(vctResult[i]._stmtId == vctStmtId[i]);
      }

      if (cnt < recNum) {
        arrResult[recStart + cnt] = 0x80;
        VectorRow vctRow = GenRow(recStart + cnt);
        SessionPool::AddStatement(tid, vctSessId[i], cnt, 1, INSERT_STMT,
                                  move(vctRow), &vctResult[i]);
        vctStmtId[i] = cnt;
        unfinished++;
        cnt++;
      }
    }

    if (unfinished == 0) {
      break;
    }
  }

  LOG_INFO << "Times: " << times;
}

} // namespace storage