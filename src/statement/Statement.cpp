#include "Statement.h"

#include "../core/BranchRecord.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../table/TableTaskMgr.h"

namespace storage {

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
  MVector<QueryRange> vctVal;

  size_t lpos = 0, rpos = 0;
  while (true) {
    if (lpos >= vctLeft.size() || rpos >= vctRight.size()) {
      return vctVal;
    }

    QueryRange *v1 = &vctLeft[lpos];
    QueryRange *v2 = &vctRight[rpos];
    assert(v1->_bRange || v1->_dvLeft == v1->_dvRight);
    assert(v2->_bRange || v2->_dvLeft == v2->_dvRight);

    if (*v1->_dvLeft > *v2->_dvLeft) {
      QueryRange *tmp = v1;
      v1 = v2;
      v2 = tmp;
      rpos++;
    } else {
      lpos++;
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
        val._dvRight = v1->_dvRight;
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
                                  MVector<QueryRange> &vctRight) {
  for (QueryRange &rVal : vctRight) {
    bool bFinished = false;

    size_t i;
    for (i = 0; i < vctResult.size(); i++) {
      QueryRange &lVal = vctResult[i];
      if (*lVal._dvRight < *rVal._dvLeft) {
        continue;
      }

      if (*lVal._dvLeft < *rVal._dvRight) {
        vctResult.insert(vctResult.begin() + i, move(rVal));
        bFinished = true;
        break;
      }

      if (*lVal._dvLeft > *rVal._dvLeft) {
        lVal._dvLeft->DecRef();
        lVal._dvLeft = rVal._dvLeft->AddRef();
        lVal._bIncLeft = rVal._bIncLeft;
      } else if (*lVal._dvLeft == *rVal._dvLeft && rVal._bIncLeft) {
        lVal._bIncLeft = true;
      }

      if (*lVal._dvRight < *rVal._dvRight) {
        lVal._dvRight->DecRef();
        lVal._dvRight = rVal._dvRight->AddRef();
        lVal._bIncRight = rVal._bIncRight;
      } else if (*lVal._dvRight == *rVal._dvRight && rVal._bIncRight) {
        lVal._bIncRight = true;
      }

      if (i < vctResult.size() - 1) {
        QueryRange &valNext = vctResult[i + 1];
        if (*valNext._dvLeft < *lVal._dvRight) {
          lVal._dvRight->DecRef();
          lVal._dvRight = valNext._dvRight->AddRef();
          lVal._bIncRight = valNext._bIncRight;
          vctResult.erase(vctResult.begin() + i + 1);
        } else if (*valNext._dvLeft == *lVal._dvRight &&
                   (lVal._bIncRight || valNext._bIncLeft)) {
          lVal._bIncRight = true;
          vctResult.erase(vctResult.begin() + i + 1);
        }
      }
      bFinished = true;
      break;
    }

    if (!bFinished) {
      vctResult.push_back(move(rVal));
    }
  }
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
    case CompType::NE:
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

ExprField *Statement::GetFrieldFromExprLogic(ExprLogic *logic) {
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
    return GetFrieldFromExprLogic(exprAnd->_vctChild[0]);
  }
  case ExprType::EXPR_OR: {
    ExprOr *exprOr = dynamic_cast<ExprOr *>(logic);
    return GetFrieldFromExprLogic(exprOr->_vctChild[0]);
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
  assert(rangePos == _midVar->_rangePos);
  assert(_midVar->_indexPos &&
         _midVar->_indexPos <= _midVar->_table->GetVectorIndex().size());
  if (IsStmtFailed()) {
    return true;
  }

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
          _midVar->_midPage = idxRange.GetTopPage(*keyRange->_startKey);
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

} // namespace storage
