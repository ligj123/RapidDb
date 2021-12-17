﻿#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
InsertStatement::InsertStatement(ExprInsert *exprInsert, Transaction *tran)
    : Statement(tran, &exprInsert->GetParameters(), exprInsert->IsStatTime()),
      _exprInsert(exprInsert) {}

int InsertStatement::ExecuteUpdate() {
  if (!_transaction->AbleAddTask()) {
    _errorMsg =
        ErrorMsg(TRAN_ADD_TASK_FAILED, {to_string(_transaction->GetTranId())});
    LOG_ERROR
        << "Try to insert a record into table with invalid transaction. ID="
        << _transaction->GetTranId();
    return -1;
  }

  PersistTable *table = _exprInsert->GetTableDesc();
  ExprValueArrayIn *vAin = _exprInsert->GetExprValueArrayIn();
  const MHashMap<string, IndexProp>::Type &mIndex = table->GetMapIndex();
  IndexTree *priTree = table->GetPrimaryIndexTree();

  for (VectorDataValue *row : _vctRow) {
    VectorDataValue &rowPri = table->GenColumsDataValues();
    vAin->Calc(*row, rowPri);

   // LeafRecord *priRec = new LeafRecord(priTree, )
  }
}

future<int> InsertStatement::ExecuteUpdateAsyn() {
  return std::promise<int>().get_future();
}
} // namespace storage
