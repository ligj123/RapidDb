#include "TableTestLib.h"

#include "../src/binlog/LogTask.h"
#include "../src/cache/Mallocator.h"
#include "../src/core/BranchPage.h"
#include "../src/core/IndexTree.h"
#include "../src/core/LeafPage.h"
#include "../src/core/LeafRecord.h"
#include "../src/manager/DatabaseManager.h"
#include "../src/manager/TableManager.h"
#include "../src/pool/CachePagePool.h"
#include "../src/pool/FilePagePool.h"
#include "../src/serv/Session.h"
#include "../src/serv/SessionPool.h"
#include "../src/statement/StmtResult.h"
#include "../src/table/Table.h"
#include "../src/table/TableTaskMgr.h"

namespace storage {
void SelectTestProc(int thd, MVector<uint32_t> vctSessId, int recStart,
                    int recNum, int opTimes) {
  MVector<StmtResult> vctResult(vctSessId.size());
  MVector<int> vctId(vctSessId.size());
  int cnt = 0;
  int times = 0;
  srand(thd);
  for (size_t i = 0; i < vctId.size(); i++) {
    vctId[i] = -1;
  }

  while (true) {
    bool empty = true;
    times++;
    for (size_t i = 0; i < vctSessId.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs != ResultStatus::FINISHED) {
        empty = false;
        continue;
      }

      int id = vctId[i];
      if (id < 0) {
        continue;
      }
      StmtResult &rst = vctResult[i];
      assert(!rst._bFailed && rst._rowNum == 1);
      rst._resultSet->First();
      VectorDataValue vctDv;
      rst._resultSet->GetCurrDataValueRow(vctDv);
      CheckSelectResult(id, vctDv);
    }

    if (cnt >= opTimes) {
      if (empty) {
        break;
      }

      continue;
    }

    for (size_t i = 0; i < vctSessId.size(); i++) {
    }
  }
}
} // namespace storage