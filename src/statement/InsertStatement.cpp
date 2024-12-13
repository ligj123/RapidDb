#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

namespace storage {
bool InsertStatement::InitData() {
  _status = StmtStatus::Created;
  PhysTable *table = _exprInsert->GetSourTable();
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  auto vctIndex = table->GetVectorIndex();

  for (VectorDataValue *pvct : _vctParas) {
    VectorDataValue vctVal;
    vctIndex[0]._tree->CloneValues(vctVal);

    for (size_t i = 0; i < _exprInsert->_rowData->size(); i++) {
      ExprElem *elem = _exprInsert->_rowData->at(i);
      ExprColumn *col = _exprInsert->_vctCol->at(i);
      IDataValue *dv = ((ExprData *)elem)->Calc(*pvct, vctVal);
      bool b = vctVal[col->_pos]->Copy(*dv, true);
      dv->DecRef();

      if (!b) {
        _stmtResult._error = move(_threadErrorMsg->GetErrorMsg());
        _status = StmtStatus::Finished;
        return false;
      }
    }

    if (!table->CheckColumnValues(vctVal)) {
      _stmtResult._error = move(_threadErrorMsg->GetErrorMsg());
      return false;
    }

    VectorDataValue vctKey;
    vctKey._bDec = false;
    vctKey.reserve(vctIndex[0]._vctCol.size());

    for (IndexColumn &col : vctIndex[0]._vctCol) {
      IDataValue *dv = vctVal[col.colPos];
      vctKey.push_back(dv);
    }

    RawKey key(vctKey);
    InsertAction *action = new InsertAction(table, vctKey, vctVal, this);
    mgr->AddSessionAction(0, /*SessionID*/, action);
  }
}
} // namespace storage
