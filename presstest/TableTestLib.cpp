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

OpRedio arrRadio[] = {OpRedio::SEL, OpRedio::SEL, OpRedio::SEL, OpRedio::SEL,
                      OpRedio::SEL, OpRedio::SEL, OpRedio::SEL, OpRedio::SEL,
                      OpRedio::SEL, OpRedio::SEL};
int redioCount = sizeof(arrRadio);

uint32_t tableId = 0;
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
  int recCount = 0;

  while (lpage != nullptr) {
    recCount += lpage->GetRecordNumber();
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

  LOG_INFO << "cnt: " << cnt << "   recCount: " << recCount;
}

void CreateDbTable(const string &dbName, bool bExclusive, int sessionGroup) {
  Database *db = new Database(1, ROOT_PATH, dbName.c_str(), MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);

  tableId += 0x100;
  PhysTable *ptable =
      new PhysTable(db, TBL_NAME, tableId, MilliSecTime(), MilliSecTime());
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

void InsertProc1(uint16_t tid, const MVector<uint32_t> &vctSessId, int recStart,
                 int recNum, int multi) {
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

      // if (rs == ResultStatus::FINISHED) {
      // assert(vctResult[i]._rowNum == 1 && vctResult[i]._vctError.size()
      //== 0);
      // assert(vctResult[i]._stmtId == vctStmtId[i]);
      // }

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

void InsertProc2(uint16_t tid, const MVector<uint32_t> &vctSessId, int recStart,
                 int recNum, int multi) {
  // LOG_INFO << "tid: " << tid << "  SessNum: " << vctSessId.size()
  //          << "   recStart: " << recStart << "   recNum: " << recNum;
  MVector<StmtResultEx> vctResult(vctSessId.size() * multi);
  int currRst = -1;
  int waitRst = recNum > vctResult.size() ? vctResult.size() : recNum;
  // int32_t poolSz = SessionPool::GetVctSessionGroup().size();
  int cnt = 0;
  int times = 0;

  while (true) {
    // MTreeMap<uint32_t, MVector<SessionStatementAction *>> mapAct;
    times++;

    for (size_t i = 0; i < vctSessId.size(); i++) {
      while (true) {
        currRst++;
        if (currRst >= vctResult.size()) {
          currRst = 0;
        }

        ResultStatus rs = vctResult[currRst].GetResultStatus();
        if (rs == ResultStatus::FILLING) {
          continue;
        }

        // if (rs == ResultStatus::FINISHED) {
        //   if (vctResult[currRst]._rowNum != 1 ||
        //       vctResult[currRst]._vctError.size() != 0) {
        //     LOG_INFO << "_rowNum: " << vctResult[currRst]._rowNum
        //              << "\tError: " <<
        // vctResult[currRst]._vctError.size();
        //   }
        // }

        break;
      }

      if (cnt >= recNum) {
        if (vctResult[currRst]._currVal >= 0) {
          waitRst--;
          vctResult[currRst]._currVal = -1;
        }

        if (waitRst == 0) {
          break;
        } else {
          continue;
        }
      }

      int currVal = recStart + cnt;
      arrResult[currVal] = 0x80;
      vctResult[currRst]._currVal = currVal;
      VectorRow vctRow = GenRow(currVal);
      SessionPool::AddStatement(tid, vctSessId[i], cnt, 1, INSERT_STMT,
                                move(vctRow), &vctResult[currRst]);

      // SessionStatementAction *action = new SessionStatementAction(
      //     vctSessId[i], cnt + recStart, 1, INSERT_STMT, move(vctRow),
      //     &vctResult[currRst]);
      // uint32_t key = ((vctSessId[i] % poolSz) << 16) + tid;
      // auto iter = mapAct.try_emplace(key, MVector<SessionStatementAction
      // *>()); iter.first->second.push_back(action);
      cnt++;
    }

    // SessionPool::AddStatements(mapAct);

    if (waitRst == 0) {
      break;
    }
  }

  LOG_INFO << tid << ": Times: " << times;
}

void InsertProc3(uint16_t tid, const MVector<uint32_t> &vctSessId, int recStart,
                 int recNum, int multi) {
  MList<StmtResultEx *> lstResult;
  int cnt = -1;
  int times = 0;

  while (true) {
    times++;
    while (lstResult.size() > 0) {
      StmtResultEx *res = lstResult.front();
      ResultStatus s = res->GetResultStatus();
      if (s == ResultStatus::FILLING) {
        break;
      }

      delete res;
      lstResult.pop_front();
      assert(s == ResultStatus::FINISHED);
      //   if (vctResult[currRst]._rowNum != 1 ||
      //       vctResult[currRst]._vctError.size() != 0) {
      //     LOG_INFO << "_rowNum: " << vctResult[currRst]._rowNum
      //              << "\tError: " <<
      //  vctResult[currRst]._vctError.size();
      //   }
    }

    if (cnt >= recNum) {
      if (lstResult.size() == 0) {
        break;
      } else {
        this_thread::yield();
        continue;
      }
    }

    if (lstResult.size() > 200) {
      this_thread::yield();
      continue;
    }

    for (size_t i = 0; i < 100; i++) {
      cnt++;

      if (cnt >= recNum) {
        break;
      }

      int currVal = recStart + cnt;
      arrResult[currVal] = 0x80;
      StmtResultEx *rst = new StmtResultEx();
      rst->_currVal = currVal;
      lstResult.push_back(rst);
      VectorRow vctRow = GenRow(currVal);
      SessionPool::AddStatement(tid, vctSessId[cnt % vctSessId.size()], cnt, 1,
                                INSERT_STMT, move(vctRow), rst);
    }
  }

  LOG_INFO << tid << ": Times: " << times;
}

} // namespace storage