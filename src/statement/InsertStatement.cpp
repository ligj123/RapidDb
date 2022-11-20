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

  const PhysTable *table = _exprInsert->GetSourTable();
  bool bUpsert = _exprInsert->IsUpsert();

  for (; _currRow < _vctParas.size(); _currRow++) {
    if (_status == InsertStatus::PRIMARY_START) {
      
    }
  }

  return 0;
}
} // namespace storage
