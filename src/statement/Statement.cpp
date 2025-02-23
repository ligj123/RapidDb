#include "Statement.h"

#include "../core/BranchRecord.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../serv/SessionAction.h"
#include "../serv/SessionPool.h"
#include "../table/TableTaskMgr.h"

namespace storage {
bool CheckQueryRangeOrder(MVector<QueryRange> &vctQR) {
  if (vctQR.size() == 0) {
    return true;
  }

  auto iter = vctQR.begin();
  while (true) {
    if (*iter->_dvLeft > *iter->_dvRight) {
      return false;
    } else if (*iter->_dvLeft == *iter->_dvRight) {
      if (iter->_bRange) {
        return false;
      } else if (!iter->_bIncLeft || !iter->_bIncRight) {
        return false;
      }
    }

    auto itPrev = iter;
    iter++;
    if (iter == vctQR.end()) {
      break;
    }

    if (*itPrev->_dvRight > *iter->_dvLeft) {
      return false;
    } else if (*itPrev->_dvRight == *iter->_dvLeft) {
      if (itPrev->_bIncRight || iter->_bIncLeft) {
        return false;
      }
    }
  }

  return true;
}

void Statement::AddLeafRecords(VectorLeafRecord &vctLr) {
  for (LeafRecord *lr : vctLr) {
    if (lr->GetLock()->GetRecordResult() == RecordResult::INIT) {
      _lstWaitRecord.push_back(lr);
    } else {
      _lstFinishRecord.push_back(lr);
      if (lr->GetLock()->GetRecordResult() == RecordResult::ERROR) {
        _stmtResult->_vctError.push_back(
            move(lr->GetLock()->_errMsg->GetErrorMsg()));
        _stmtFailed.store(true, memory_order_relaxed);
      }
    }
  }
}

void Statement::AddLeafRecord(LeafRecord *lr) {
  if (lr->GetLock()->GetRecordResult() == RecordResult::INIT) {
    _lstWaitRecord.push_back(lr);
  } else {
    _lstFinishRecord.push_back(lr);
    if (lr->GetLock()->GetRecordResult() == RecordResult::ERROR) {
      _stmtResult->_vctError.push_back(
          move(lr->GetLock()->_errMsg->GetErrorMsg()));
      _stmtFailed.store(true, memory_order_relaxed);
    }
  }
}

/**
 * @brief Merge two MVector<QueryRange> into one by AND operation， ASSUME every
 * vector is sorted.
 * @param vctLeft
 * @param vctRight
 * @return The merged result
 */
MVector<QueryRange>
Statement::MergeAndQueryRange(MVector<QueryRange> &vctLeft,
                              MVector<QueryRange> &vctRight) {
  assert(CheckQueryRangeOrder(vctLeft));
  assert(CheckQueryRangeOrder(vctRight));
  MVector<QueryRange> vctVal;

  size_t lpos = 0, rpos = 0;
  while (true) {
    if (lpos >= vctLeft.size() || rpos >= vctRight.size()) {
      break;
    }

    QueryRange *v1 = &vctLeft[lpos];
    QueryRange *v2 = &vctRight[rpos];
    assert(v1->_bRange || v1->_dvLeft == v1->_dvRight);
    assert(v2->_bRange || v2->_dvLeft == v2->_dvRight);

    if (*v1->_dvRight > *v2->_dvRight) {
      rpos++;
    } else {
      lpos++;
    }

    if (*v1->_dvLeft > *v2->_dvLeft) {
      QueryRange *tmp = v1;
      v1 = v2;
      v2 = tmp;
    }

    QueryRange val;

    if (*v1->_dvRight >= *v2->_dvLeft) {
      val._dvLeft = v2->_dvLeft->AddRef();
      if (*v1->_dvLeft == *v2->_dvLeft) {
        val._bIncLeft = (v1->_bIncLeft && v2->_bIncLeft);
      } else {
        val._bIncLeft = v2->_bIncLeft;
      }

      if (*v1->_dvRight > *v2->_dvRight) {
        val._dvRight = v2->_dvRight->AddRef();
        val._bIncRight = v2->_bIncRight;
      } else {
        val._dvRight = v1->_dvRight->AddRef();
        if (*v1->_dvRight == *v2->_dvRight) {
          val._bIncRight = (v1->_bIncRight && v2->_bIncRight);
        } else {
          val._bIncRight = v1->_bIncRight;
        }
      }

      if (*val._dvLeft == *val._dvRight) {
        if (!val._bIncLeft || !val._bIncRight) {
          continue;
        } else {
          val._bRange = false;
        }
      }

      vctVal.push_back(move(val));
    }
  }

  return vctVal;
}

void Statement::MergeOrQueryRange(MVector<QueryRange> &vctResult,
                                  MVector<QueryRange> &vctSrc) {
  assert(CheckQueryRangeOrder(vctSrc));

  size_t rpos = 0, spos = 0;
  while (true) {
    if (rpos >= vctResult.size() || spos >= vctSrc.size()) {
      break;
    }

    QueryRange &v1 = vctResult[rpos];
    QueryRange &v2 = vctSrc[spos];

    if (*v1._dvLeft > *v2._dvLeft) {
      vctResult.insert(vctResult.begin() + rpos, move(v2));
      rpos++;
      spos++;
    } else {
      rpos++;
    }
  }

  if (spos < vctSrc.size()) {
    vctResult.insert(vctResult.end(), vctSrc.begin() + spos, vctSrc.end());
  }

  assert(vctResult.size() > 0);
  auto iter = vctResult.begin();
  while (true) {
    auto itprev = iter;
    iter++;
    if (iter == vctResult.end()) {
      break;
    }

    if (*itprev->_dvRight > *iter->_dvLeft ||
        (*itprev->_dvRight == *iter->_dvLeft &&
         (itprev->_bIncRight || iter->_bIncLeft))) {
      if (*itprev->_dvRight < *iter->_dvRight) {
        itprev->_dvRight->DecRef();
        itprev->_dvRight = iter->_dvRight->AddRef();
        itprev->_bIncRight = iter->_bIncRight;
      } else if (*itprev->_dvRight == *iter->_dvRight) {
        itprev->_bIncRight |= iter->_bIncRight;
      }

      vctResult.erase(iter);
      iter = itprev;
    }
  }

  assert(CheckQueryRangeOrder(vctResult));
}

MVector<QueryRange> Statement::ConditionConvert(ExprLogic *logic,
                                                VectorDataValue &paras) {
  assert(logic != nullptr);
  MVector<QueryRange> vctVal;

  switch (logic->GetType()) {
  case ExprType::EXPR_COMP: {
    ExprComp *ecmp = dynamic_cast<ExprComp *>(logic);
    assert(ecmp->_exprLeft->GetType() == ExprType::EXPR_FIELD);
    vctVal.resize(1);
    VectorDataValue tmp;
    IDataValue *dv = ecmp->_exprRight->Calc(paras, tmp);

    switch (ecmp->_compType) {
    case CompType::EQ:
      vctVal[0]._dvLeft = dv;
      vctVal[0]._dvRight = dv->AddRef();
      vctVal[0]._bRange = false;
      break;
    case CompType::GT: {
      vctVal[0]._dvLeft = dv;
      IDataValue *border = dv->Clone(false);
      border->SetMaxValue();
      vctVal[0]._dvRight = border;
      vctVal[0]._bIncLeft = false;
      break;
    }
    case CompType::GE: {
      vctVal[0]._dvLeft = dv;
      IDataValue *border = dv->Clone(false);
      border->SetMaxValue();
      vctVal[0]._dvRight = border;
      break;
    }
    case CompType::LT: {
      IDataValue *border = dv->Clone(false);
      border->SetMinValue();
      vctVal[0]._dvLeft = border;
      vctVal[0]._dvRight = dv;
      vctVal[0]._bIncRight = false;
      break;
    }
    case CompType::LE: {
      IDataValue *border = dv->Clone(false);
      border->SetMinValue();
      vctVal[0]._dvLeft = border;
      vctVal[0]._dvRight = dv;
      break;
    }
    case CompType::NE: {
      IDataValue *border = dv->Clone(false);
      border->SetMinValue();
      vctVal[0]._dvLeft = border;
      vctVal[0]._dvRight = dv;
      vctVal[0]._bIncRight = false;

      vctVal.emplace_back();
      border = dv->Clone(false);
      border->SetMaxValue();
      vctVal[1]._dvLeft = dv;
      vctVal[1]._dvRight = border;
      vctVal[1]._bIncLeft = false;
      break;
    }
    default:
      LOG_ERROR << "Unsupport compare type : " << ecmp->_compType;
      abort();
    }

    break;
  }
  case ExprType::EXPR_IN_OR_NOT: {
    ExprInNot *exprIn = dynamic_cast<ExprInNot *>(logic);
    assert(exprIn->_bIn);
    assert(exprIn->_exprData->GetType() == ExprType::EXPR_FIELD);
    vctVal.resize(exprIn->_exprArray->_setVal.size());

    size_t pos = 0;
    for (IDataValue *dv : exprIn->_exprArray->_setVal) {
      QueryRange &idxVal = vctVal[pos];
      pos++;
      idxVal._bRange = false;
      idxVal._dvLeft = dv;
      idxVal._dvRight = dv->AddRef();
    }

    exprIn->_exprArray->_setVal.clear();
    break;
  }
  case ExprType::EXPR_BETWEEN: {
    ExprBetween *exprBwn = dynamic_cast<ExprBetween *>(logic);
    assert(exprBwn->_child->GetType() == ExprType::EXPR_FIELD);
    vctVal.resize(1);
    VectorDataValue tmp;
    IDataValue *dvL = exprBwn->_exprLeft->Calc(paras, tmp);
    IDataValue *dvR = exprBwn->_exprRight->Calc(paras, tmp);
    if (*dvL > *dvR) {
      dvL->DecRef();
      dvR->DecRef();
      vctVal.clear();
    } else {
      vctVal[0]._dvLeft = dvL;
      vctVal[0]._dvRight = dvR;
      if (*dvL == *dvR) {
        vctVal[0]._bRange = false;
      }
    }

    break;
  }
  case ExprType::EXPR_AND: {
    ExprAnd *exprAnd = dynamic_cast<ExprAnd *>(logic);
    for (ExprLogic *logic : exprAnd->_vctChild) {
      MVector<QueryRange> vctRes = ConditionConvert(logic, paras);
      if (vctVal.size() > 0) {
        MVector<QueryRange> vct = MergeAndQueryRange(vctVal, vctRes);
        vctVal.swap(vct);
      } else {
        vctVal = move(vctRes);
      }
    }

    break;
  }
  case ExprType::EXPR_OR: {
    ExprOr *exprOr = dynamic_cast<ExprOr *>(logic);
    for (ExprLogic *logic : exprOr->_vctChild) {
      MVector<QueryRange> vctRes = ConditionConvert(logic, paras);
      MergeOrQueryRange(vctVal, vctRes);
    }
    break;
  }
  default:
    LOG_ERROR << "Unsupport Expr type : " << logic->GetType();
    abort();
  }

  return vctVal;
}

std::ostream &operator<<(std::ostream &os, const StmtStatus &s) {
  os << "StmtStatus::";
  switch (s) {
  case StmtStatus::Created:
    os << "Created(" << (int)StmtStatus::Created << ")";
    break;
  case StmtStatus::Executing:
    os << "Executing(" << (int)StmtStatus::Executing << ")";
    break;
  case StmtStatus::Executed:
    os << "Executed(" << (int)StmtStatus::Executed << ")";
    break;
  case StmtStatus::Logging:
    os << "Logging(" << (int)StmtStatus::Logging << ")";
    break;
  case StmtStatus::Logged:
    os << "Logged(" << (int)StmtStatus::Logged << ")";
    break;
  case StmtStatus::Finished:
    os << "Finished(" << (int)StmtStatus::Finished << ")";
    break;
  }

  return os;
}

ExprField *Statement::GetFieldFromExprLogic(ExprLogic *logic) {
  switch (logic->GetType()) {
  case ExprType::EXPR_COMP: {
    ExprComp *ecmp = dynamic_cast<ExprComp *>(logic);
    return dynamic_cast<ExprField *>(ecmp->_exprLeft);
  }
  case ExprType::EXPR_IN_OR_NOT: {
    ExprInNot *exprIn = dynamic_cast<ExprInNot *>(logic);
    return dynamic_cast<ExprField *>(exprIn->_exprData);
  }
  case ExprType::EXPR_BETWEEN: {
    ExprBetween *exprBwn = dynamic_cast<ExprBetween *>(logic);
    return dynamic_cast<ExprField *>(exprBwn->_child);
  }
  case ExprType::EXPR_AND: {
    ExprAnd *exprAnd = dynamic_cast<ExprAnd *>(logic);
    return GetFieldFromExprLogic(exprAnd->_vctChild[0]);
  }
  case ExprType::EXPR_OR: {
    ExprOr *exprOr = dynamic_cast<ExprOr *>(logic);
    return GetFieldFromExprLogic(exprOr->_vctChild[0]);
  }
  default:
    LOG_ERROR << "Unsupport Expr type : " << logic->GetType();
    abort();
  }
  return nullptr;
}

KeyRange Statement::GenIndexSearchKey(IndexTree *idxTree, ExprField *field,
                                      QueryRange *qRange) {
  RawKey *sKey = nullptr;
  RawKey *eKey = nullptr;
  VectorDataValue vdv;

  idxTree->CloneKeys(vdv);
  vdv[0]->Copy(*(qRange->_dvLeft), true);
  for (size_t i = 1; i < vdv.size(); i++) {
    vdv[i]->SetMinValue();
  }

  sKey = new RawKey(vdv);

  if (qRange->_bRange || vdv.size() > 1) {
    if (qRange->_bRange) {
      vdv[0]->Copy(*(qRange->_dvRight), true);
    }

    for (size_t i = 1; i < vdv.size(); i++) {
      vdv[i]->SetMaxValue();
    }

    eKey = new RawKey(vdv);
  }

  return KeyRange(sKey, eKey, qRange->_bRange, qRange->_bIncLeft,
                  qRange->_bIncRight);
}

void Statement::SendStmtRecord(int idxPos, int rangePos, PhysTable *table,
                               Statement *stmt, LeafRecord *lr,
                               IndexTree *idxTree) {
  StmtSecRecord *stmtRec = new StmtSecRecord();
  stmtRec->_table = table;
  stmtRec->_stmt = stmt;
  stmtRec->_secLr = lr;

  TableTaskMgr *mgr = table->GetTableTaskMgr();
  StmtSecRecordAction *action =
      new StmtSecRecordAction(table->GetVectorIndex()[0]._tree, stmtRec);
  mgr->AddToPrimaryAction(idxPos, rangePos, action);
}

void Statement::CollectLogRecords(TreeSetRecord &setRec) {
  assert(!_stmtFailed.load(memory_order_relaxed) && !IsReadonly());

  for (LeafRecord *lr : _lstFinishRecord) {
    assert(lr->GetLock()->_recResult != RecordResult::INIT);
    if (lr->GetLock()->_recResult == RecordResult::ERROR) {
      continue;
    }

    setRec.insert(lr);
  }
}

void Statement::Commit() {
  assert(!_stmtFailed.load(memory_order_relaxed));

  for (LeafRecord *lr : _lstFinishRecord) {
    lr->SubmitStatement(*this, (lr->GetLock()->_actType & READ_LOCK_MASK) == 0
                                   ? RecordStatus::COMMITED
                                   : RecordStatus::FREEED);
  }
}

void Statement::Rollback() {
  for (LeafRecord *lr : _lstFinishRecord) {
    lr->SubmitStatement(*this, (lr->GetLock()->_actType & READ_LOCK_MASK) == 0
                                   ? RecordStatus::ROLLBACKED
                                   : RecordStatus::FREEED);
  }
}

int Statement::CalcIndexRanges(IndexTree *idxTree) {
  assert(_midVar->_keyPos >= 0 &&
         _midVar->_keyPos < _midVar->_vctKeyRange.size());
  assert(_midVar->_indexPos >= 0 &&
         _midVar->_indexPos < _midVar->_table->GetVectorIndex().size());
  RawKey *key = _midVar->_vctKeyRange[_midVar->_keyPos]._startKey;
  return _midVar->_table->GetVectorIndex()[_midVar->_indexPos]
      ._tree->CalcIndexRange(*key);
}

bool Statement::SacnIndex(int rangePos) {
  assert(_midVar->_indexPos >= 0 &&
         _midVar->_indexPos <= _midVar->_table->GetVectorIndex().size());
  if (IsStmtFailed()) {
    return true;
  }

  _midVar->_rangePos = rangePos;
  IndexTree *idxTree =
      _midVar->_table->GetVectorIndex()[_midVar->_indexPos]._tree;
  IndexRange &idxRange = idxTree->GetVctRange()[_midVar->_rangePos];

  while (true) {
    KeyRange *keyRange = &_midVar->_vctKeyRange[_midVar->_keyPos];
    if (_midVar->_midPage == nullptr ||
        _midVar->_midPage->GetPageType() != PageType::LEAF_PAGE) {
      if (_midVar->_bFromRangeBegin) {
        _midVar->_midPage = idxRange._startPage;
      } else {
        if (_midVar->_midPage == nullptr) {
          if (idxTree->GetVctRange().size() > 1) {
            _midVar->_midPage = idxRange.GetTopPage(*keyRange->_startKey);
          } else {
            _midVar->_midPage = idxTree->GetRootPage();
          }
        }

        if (!idxTree->SearchPage(*keyRange->_startKey, _midVar->_midPage)) {
          return false;
        }
      }
    }

    int pos = 0;
    bool bFind = true;

    LeafPage *lpage = dynamic_cast<LeafPage *>(_midVar->_midPage);
    if (!_midVar->_bFromPageBegin) {
      pos = lpage->SearchKey(*keyRange->_startKey, bFind);
      if (!keyRange->_bIncLeft && bFind) {
        pos++;
      }
    }

    bool bend = false;
    if (!keyRange->_bRange &&
        idxTree->GetIndexType() != IndexType::NON_UNIQUE) {
      bend = true;
      if (bFind) {
        LeafRecord *lr = &lpage->GetRecord(pos);
        if (_midVar->_indexPos == 0) {
          TriBool tb = HandleLeafRecord(lpage, pos, rangePos);
          if (tb == TriBool::Error) {
            SetFinished(true);
            return true;
          }
        } else {
          SendStmtRecord(_midVar->_indexPos, rangePos, _midVar->_table, this,
                         lr, idxTree);
        }
      }
    } else {
      for (; pos < lpage->GetRecordNumber(); pos++) {
        LeafRecord *lr = &lpage->GetRecord(pos);
        int res = lr->CompareKey(*keyRange->_endKey);
        if ((res == 0 && !keyRange->_bIncRight) || res > 0) {
          bend = true;
          break;
        }
        if (_midVar->_indexPos == 0) {
          TriBool tb = HandleLeafRecord(lpage, pos, rangePos);
          if (tb == TriBool::Error) {
            SetFinished(true);
            return true;
          }
        } else {
          SendStmtRecord(_midVar->_indexPos, rangePos, _midVar->_table, this,
                         lr, idxTree);
        }
      }
    }

    if (IsStmtFailed()) {
      SetFinished(true);
      return true;
    }

    if (bend) {
      _midVar->_keyPos++;
      if (_midVar->_keyPos < _midVar->_vctKeyRange.size()) {
        _midVar->_midPage = nullptr;
        _midVar->_bFromPageBegin = false;
        if (idxRange._borderRecord->CompareKey(
                *_midVar->_vctKeyRange[_midVar->_keyPos]._startKey) < 0) {
          StatementAction *action = new StatementAction(idxTree, this);
          TableTaskMgr *mgr = _midVar->_table->GetTableTaskMgr();
          _midVar->_rangePos = idxTree->CalcIndexRange(
              *_midVar->_vctKeyRange[_midVar->_keyPos]._startKey);
          mgr->AddIndexRangeAction(_midVar->_indexPos, _midVar->_rangePos,
                                   action);
        }
      } else {
        SetFinished(true);
        return true;
      }
    } else {
      _midVar->_bFromPageBegin = true;
      if (lpage->IsRangEndPage()) {
        _midVar->_midPage = nullptr;
        StatementAction *action = new StatementAction(idxTree, this);
        TableTaskMgr *mgr = _midVar->_table->GetTableTaskMgr();
        _midVar->_rangePos++;
        mgr->AddIndexRangeAction(_midVar->_indexPos, _midVar->_rangePos,
                                 action);
        return true;
      } else {
        _midVar->_midPage = lpage->GetNextPage();
      }
    }
  }

  return true;
}

void Statement::SendErrMsg(MString &&errMsg) {
  if (_midVar->_indexPos == 0) {
    _stmtResult->_vctError.push_back(move(errMsg));
  } else {
    SessionErrMsgAction *eAction = new SessionErrMsgAction(this, move(errMsg));
    SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), eAction);
  }
  SetStmtFailed(true);
}
} // namespace storage
