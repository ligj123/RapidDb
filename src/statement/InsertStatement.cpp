#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
InsertStatement::InsertStatement(ExprInsert *exprInsert, Transaction *tran)
    : Statement(tran, exprInsert->GetParaTemplate()), _exprInsert(exprInsert) {}

int InsertStatement::Execute() {
  if (_startTime == 0) {
    _startTime = MilliSecTime();
  }

  PhysTable *table = _exprInsert->GetTableDesc();
  bool bUpsert = _exprInsert->IsUpsert;

  for (; _currRow < _vctParas.size(); _currRow++) {
    }
  // ExprValueArrayIn *vAin = _exprInsert->GetExprValueArrayIn();
  //  const MHashMap<string, IndexProp>::Type &mIndex = table->GetMapIndex();
  //  IndexTree *priTree = table->GetPrimaryIndexTree();

  for (VectorDataValue *row : _vctRow) {
    VectorDataValue rowPri;
    // table->GenColumsDataValues(rowPri);
    // vAin->Calc(*row, rowPri);

    // LeafRecord *priRec = new LeafRecord(priTree, )
  }

  _startTime = MilliSecTime();
  return 0;
}
} // namespace storage
