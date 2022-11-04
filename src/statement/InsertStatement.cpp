#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
InsertStatement::InsertStatement(ExprInsert *exprInsert, Transaction *tran)
    : Statement(tran, exprInsert->GetParaTemplate()), _exprInsert(exprInsert) {}

int InsertStatement::ExecuteUpdate() {
  _startTime = TimerThread::GetCurrTime();
  if (!_tran->AbleAddTask()) {
    _errorMsg->SetMsg(TRAN_ADD_TASK_FAILED, {to_string(_tran->GetTranId())});
    LOG_ERROR
        << "Try to insert a record into table with invalid transaction. ID="
        << _tran->GetTranId();
    return -1;
  }

  // PhysTable *table = _exprInsert->GetTableDesc();
  // ExprValueArrayIn *vAin = _exprInsert->GetExprValueArrayIn();
  //  const MHashMap<string, IndexProp>::Type &mIndex = table->GetMapIndex();
  //  IndexTree *priTree = table->GetPrimaryIndexTree();

  for (VectorDataValue *row : _vctRow) {
    VectorDataValue rowPri;
    // table->GenColumsDataValues(rowPri);
    // vAin->Calc(*row, rowPri);

    // LeafRecord *priRec = new LeafRecord(priTree, )
  }

  _startTime = TimerThread::GetCurrTime();
  return 0;
}
} // namespace storage
