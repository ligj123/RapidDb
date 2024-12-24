#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

namespace storage {
bool InsertStatement::SessionExec() {
  PhysTable *table = _exprInsert->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  auto &priIndex = table->GetVectorIndex()[0];

  for (VectorDataValue *pvct : _vctParas) {
    VectorDataValue vctVal;
    priIndex._tree->CloneValues(vctVal);

    for (size_t i = 0; i < _exprInsert->_rowData->size(); i++) {
      ExprElem *elem = _exprInsert->_rowData->at(i);
      ExprColumn *col = _exprInsert->_vctCol->at(i);
      IDataValue *dv = ((ExprData *)elem)->Calc(*pvct, vctVal);
      bool b = vctVal[col->_pos]->Copy(*dv, true);
      dv->DecRef();

      if (!b) {
        _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
        SetStmtFailed(true);
        _status = StmtStatus::Initialized;
        return true;
      }
    }

    if (!table->CheckColumnValues(vctVal)) {
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      _status = StmtStatus::Initialized;
      return true;
    }

    VectorDataValue vctKey;
    vctKey._bDec = false;
    vctKey.reserve(priIndex._vctCol.size());

    for (IndexColumn &col : priIndex._vctCol) {
      IDataValue *dv = vctVal[col.colPos];
      vctKey.push_back(dv);
    }

    InsertRecord *insr = new InsertRecord();
    insr->_priKey = RawKey(vctKey);
    insr->_vctParas = move(vctVal);
    int range = priIndex._tree->CalcIndexRange(insr->_priKey);
    auto iter = _mapInsert.find(range);
    if (iter == _mapInsert.end()) {
      iter = _mapInsert.emplace(range, MVectorPtr<InsertRecord *>()).first;
    }

    iter->second.push_back(insr);
  }

  return true;
}

bool InsertStatement::PrimaryKeyExec() { return true; }
} // namespace storage
