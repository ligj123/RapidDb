#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../serv/SessionPool.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

namespace storage {
bool InsertStatement::SessionExec() {
  PhysTable *table = _exprInsert->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  auto &priIndex = table->GetVectorIndex()[0];
  MVectorPtr<InsertRecord *> &vctRec =
      _mapInsert.emplace(-1, MVectorPtr<InsertRecord *>()).first->second;

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
    vctRec.push_back(insr);
  }

  StatementAction *action = new StatementAction(priIndex._tree, this);
  mgr->AddSessionAction(0, GetSessionId(), action);
  _status = StmtStatus::Executing;
  return true;
}

bool InsertStatement::PrimaryKeyExec(int rangePos) {
  auto iter = _mapInsert.find(rangePos);
  assert(iter != _mapInsert.end());

  PhysTable *table = _exprInsert->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  MVector<storage::IndexProp> &vctProp = table->GetVectorIndex();

  MVectorPtr<InsertRecord *> &vctRec = iter->second;
  for (InsertRecord *iRec : vctRec) {
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

StmtStatus InsertStatement::CheckStatus() {
  if (_status == StmtStatus::Initialized) {
    PhysTable *table = _exprInsert->_physTable;
    if (_stmtFailed.load(memory_order_relaxed)) {
      if (_lstFinshRecord.size() + _lstWaitRecord.size() ==
          _vctParas.size() * table->GetVectorIndex().size()) {
        _lstFinshRecord.insert(_lstFinshRecord.end(), _lstWaitRecord.begin(),
                               _lstWaitRecord.end());
        _lstWaitRecord.clear();
        _status = StmtStatus::Executed;
        return _status;
      }
    }

    for (auto iter = _lstWaitRecord.begin(); iter != _lstWaitRecord.end();) {
      if ((*iter)->GetLock()->GetRecordResult() != RecordResult::INIT) {
        _lstFinshRecord.push_back(*iter);
        iter = _lstWaitRecord.erase(iter);
      } else {
        iter++;
      }
    }

    if (_lstFinshRecord.size() ==
        _vctParas.size() * table->GetVectorIndex().size()) {
      _status = StmtStatus::Executed;
    }
  }

  return _status;
}

void InsertStatement::CollectLogRecords(
    MTreeSet<LeafRecord *, LeafRecordCmp> &setRec) {
  for (LeafRecord *lr : _lstFinshRecord) {
    setRec.insert(lr);
  }
}

void InsertStatement::Commit() {
  for (LeafRecord *lr : _lstFinshRecord) {
    lr->SubmitStatement(*this, RecordStatus::COMMITED);
  }
}

void InsertStatement::Rollback() {
  for (LeafRecord *lr : _lstFinshRecord) {
    lr->SubmitStatement(*this, RecordStatus::ROLLBACKED);
  }
}

MVector<int> InsertStatement::CalcIndexRanges(IndexTree *idxTree) {
  MHashMap<int, MVectorPtr<InsertRecord *>> map;
  map.swap(_mapInsert);
  assert(map.size() == 1 && map.begin()->first == -1);
  MVectorPtr<InsertRecord *> &vctRec = _mapInsert.begin()->second;
  PhysTable *table = _exprInsert->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  auto &priIndex = table->GetVectorIndex()[0];
  MVector<int> vctPos;

  for (InsertRecord *insrRec : vctRec) {
    int pos = idxTree->CalcIndexRange(insrRec->_priKey);
    auto iter = _mapInsert.find(pos);
    if (iter == _mapInsert.end()) {
      iter = _mapInsert.emplace(pos, MVectorPtr<InsertRecord *>()).first;
      vctPos.push_back(pos);
    }

    iter->second.push_back(insrRec);
  }

  vctRec.clear();
  return vctPos;
}
} // namespace storage
