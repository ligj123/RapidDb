#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../serv/SessionPool.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

namespace storage {
bool InsertStatement::SessionExec() {
  PhysTable *table = _exprInsert->_physTable;
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
    vctKey._bDecrease = false;
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
      iter = _mapInsert.emplace(range, RangeRecord()).first;
    }

    iter->second._vctRecord.push_back(insr);
  }

  _status = StmtStatus::Initialized;
  return true;
}

bool InsertStatement::PrimaryKeyExec(int rangePos) {
  auto iter = _mapInsert.find(rangePos);
  assert(iter != _mapInsert.end());

  PhysTable *table = _exprInsert->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  MVector<storage::IndexProp> &vctProp = table->GetVectorIndex();

  RangeRecord &rangeRecord = iter->second;
  for (InsertRecord *iRec : rangeRecord._vctRecord) {
    VersionStamp stamp = vctProp[0]._tree->ApplyStamp(rangePos);
    LeafRecord *lrPri = new LeafRecord(vctProp[0]._tree, iRec->_priKey,
                                       iRec->_vctParas, stamp, this);
    if (!lrPri->IsValid()) {
      SessionErrMsgAction *eAction =
          new SessionErrMsgAction(this, move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      delete lrPri;
      break;
    }

    SessionRecordAction *action = new SessionRecordAction(this, lrPri);
    SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), action);
    RecordAction *rAction = new RecordAction(vctProp[0]._tree, lrPri);
    vctProp[0]._tree->AddActionFromLocal(rangePos, rAction);

    for (size_t i = 1; i < vctProp.size(); i++) {
      IndexTree *secTree = vctProp[i]._tree;
      VectorDataValue vctKey;
      vctKey._bDecrease = false;
      vctKey.reserve(vctProp[i]._vctCol.size());

      for (IndexColumn &col : vctProp[i]._vctCol) {
        IDataValue *dv = iRec->_vctParas[col.colPos];
        vctKey.push_back(dv);
      }
      LeafRecord *lrSec = new LeafRecord(
          secTree, vctKey, lrPri->GetBysValue() + UI16_2_LEN,
          lrPri->GetKeyLength(), ActionType::INSERT, stamp, this);

      if (!lrSec->IsValid()) {
        SessionErrMsgAction *eAction =
            new SessionErrMsgAction(this, move(_threadErrorMsg->GetErrorMsg()));
        SetStmtFailed(true);
        delete lrSec;
        break;
      }

      SessionRecordAction *sAction = new SessionRecordAction(this, lrSec);
      SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), sAction);

      RecordAction *rAction = new RecordAction(secTree, lrSec);
      mgr->AddFromPrimaryAction(i, rangePos, rAction);
    }
  }

  return true;
}

StmtStatus InsertStatement::CheckStatus() { return StmtStatus::Created; }

void InsertStatement::CollectLogRecords(MTreeSet<LeafRecord *> &setRec) {}

void InsertStatement::Commit() {}

void InsertStatement::Rollback() {}
} // namespace storage
