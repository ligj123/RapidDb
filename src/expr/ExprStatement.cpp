#include "ExprStatement.h"

#include "../expr/ExprLogic.h"
#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../serv/Session.h"
#include "../table/Table.h"
#include "../utils/ErrorMsg.h"

namespace storage {
bool FillElemFiled(const MStrHashMap<uint32_t> &mapColPos,
                   MVector<ExprElem *> &vctElem) {
  for (ExprElem *elem : vctElem) {
    ExprField *field = (ExprField *)elem;
    auto iter = mapColPos.find(*field->_colName);
    if (iter != mapColPos.end()) {
      field->_rowPos = iter->second;
    } else {
      _threadErrorMsg.reset(
          new ErrorMsg(TB_UNEXIST_COLUMN, {*field->_colName}));
      return false;
    }
  }

  return true;
}

bool ExprWhere::Preprocess(PhysTable *table,
                           const MStrHashMap<uint32_t> &mapColPos) {
  MVector<ExprElem *> vctElem;
  vctElem.reserve(32);
  _exprLogic->CollectElem(ExprType::EXPR_FIELD, vctElem);
  if (!FillElemFiled(mapColPos, vctElem)) {
    return false;
  }

  int idxPos;
  MVectorPtr<ExprLogic *> vctExpr;
  TriBool b = _exprLogic->PickIndexCondition(table, idxPos, &vctExpr);
  if (b == TriBool::Error) {
    return false;
  } else if (b == TriBool::False) {
    return true;
  }

  if (_exprLogic->GetType() == ExprType::EXPR_AND) {
    if (vctExpr.size() == 1) {
      _indexSearch = new IndexSearch(*vctExpr.begin(), idxPos);
      vctExpr.clear();
    } else {
      ExprAnd *expr = new ExprAnd();
      expr->_vctChild = move(vctExpr);
      _indexSearch = new IndexSearch(expr, idxPos);
    }

    ExprAnd *eand = dynamic_cast<ExprAnd *>(_exprLogic);
    if (eand->_vctChild.size() == 0) {
      delete _exprLogic;
      _exprLogic = nullptr;
    } else if (eand->_vctChild.size() == 1) {
      ExprLogic *logic = *eand->_vctChild.begin();
      eand->_vctChild.clear();
      delete _exprLogic;
      _exprLogic = logic;
    }

  } else {
    _indexSearch = new IndexSearch(_exprLogic, idxPos);
    _exprLogic = nullptr;
  }

  bool bPoint = false;
  ExprType etype = _indexSearch->_idxLogic->GetType();
  if (etype == ExprType::EXPR_COMP) {
    ExprComp *ecmp = dynamic_cast<ExprComp *>(_indexSearch->_idxLogic);
    if (ecmp->_compType == CompType::EQ) {
      bPoint = true;
    }
  } else if (etype == ExprType::EXPR_IN_OR_NOT) {
    ExprInNot *exprIn = dynamic_cast<ExprInNot *>(_indexSearch->_idxLogic);
    bPoint = exprIn->_bIn;
  }

  if (bPoint) {
    _indexSearch->_bPointQuery = true;
    _indexSearch->_vctPointCond = new MVectorPtr<ExprLogic *>();
    _indexSearch->_vctPointCond->push_back(_indexSearch->_idxLogic);
    _indexSearch->_idxLogic = nullptr;

    IndexProp &prop = table->GetVectorIndex()[idxPos];
    if (prop._vctCol.size() > 1 && _exprLogic != nullptr) {
      if (_exprLogic->GetType() == ExprType::EXPR_AND) {
        ExprAnd *eand = dynamic_cast<ExprAnd *>(_exprLogic);
        MTreeMap<int, ExprLogic *> map;
        for (ExprLogic *logic : eand->_vctChild) {
          int pos = logic->CombinedIndexCondition(prop._vctCol);
          if (pos >= 0) {
            map.emplace(pos, logic);
          }
        }

        int pos = 1;
        while (true) {
          auto iter = map.find(pos);
          if (iter != map.end()) {
            _indexSearch->_vctPointCond->push_back(iter->second);
            for (auto it = eand->_vctChild.begin(); it != eand->_vctChild.end();
                 it++) {
              if (*it == iter->second) {
                eand->_vctChild.erase(it);
                break;
              }
            }
          }
        }
      } else {
        if (_exprLogic->CombinedIndexCondition(prop._vctCol) == 1) {
          _indexSearch->_vctPointCond->push_back(_exprLogic);
          _exprLogic = nullptr;
        }
      }
    }
  }

  return true;
}

bool ExprGroupBy::Preprocess(const MStrHashMap<uint32_t> &mapColPos) {
  assert(_vctColPos.size() == 0);
  for (MString *name : *_vctColName) {
    auto iter = mapColPos.find(*name);
    if (iter != mapColPos.end()) {
      _vctColPos.push_back(iter->second);
    } else {
      _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {*name}));
      return false;
    }
  }

  if (_exprHaving != nullptr) {
    return _exprHaving->Preprocess(mapColPos);
  } else {
    return true;
  }
}

bool ExprOrderBy::Preprocess(const MStrHashMap<uint32_t> &mapColPos) {
  for (ExprOrderItem *item : *_vctItem) {
    auto iter = mapColPos.find(*item->_colName);
    if (iter != mapColPos.end()) {
      item->_pos = iter->second;
    } else {
      _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {*item->_colName}));
      return false;
    }
  }

  return true;
}

bool ExprSelect::Preprocess(Database *currDb) {
  if (_vctTable->size() == 1) {
    ExprTableSelect *tsel = new ExprTableSelect();
    tsel->_vctPara = move(_vctPara);
    tsel->_exprId = _exprId;
    tsel->_bDistinct = _bDistinct;
    tsel->_vctCol = _vctCol;
    _vctCol = nullptr;
    tsel->_exprTable = _vctTable->at(0);
    _vctTable->clear();
    tsel->_exprWhere = _exprWhere;
    _exprWhere = nullptr;
    tsel->_exprGroupBy = _exprGroupBy;
    _exprGroupBy = nullptr;
    tsel->_exprOrderBy = _exprOrderBy;
    _exprOrderBy = nullptr;
    tsel->_exprLimit = _exprLimit;
    _exprLimit = nullptr;
    _exprDestSelect = tsel;
    if (!tsel->Preprocess(currDb)) {
      return false;
    }
  } else {
    _threadErrorMsg.reset(new ErrorMsg(
        SYS_UNSUPPORT_OPERATION, {"This version does not support table join"}));
    return false;
  }

  return true;
}

bool ExprTableSelect::Preprocess(Database *currDb) {
  if (!_exprTable->Preprocess(currDb)) {
    return false;
  }

  const MStrHashMap<uint32_t> &mapTPos =
      _exprTable->_physTable->GetMapColumnPos();
  MStrHashMap<uint32_t> mapRPos;

  MVector<ExprElem *> vctField;
  vctField.reserve(32);
  for (size_t i = 0; i < _vctCol->size(); i++) {
    ExprColumn *ecol = _vctCol->at(i);
    bool bAlias = true;
    if (ecol->_exprElem->GetType() == ExprType::EXPR_FIELD) {
      ecol->_name =
          new MString(*(dynamic_cast<ExprField *>(ecol->_exprElem)->_colName));
    } else if (ecol->_alias != nullptr) {
      ecol->_name = new MString(*ecol->_alias);
      bAlias = false;
    } else {
      ecol->_name = new MString("col" + ToMString(i));
    }

    ecol->_exprElem->CollectElem(ExprType::EXPR_FIELD, vctField);
    mapRPos.emplace(*ecol->_name, i);
    if (bAlias && ecol->_alias != nullptr) {
      mapRPos.emplace(*ecol->_alias, i);
    }
  }

  if (!FillElemFiled(mapTPos, vctField)) {
    return false;
  }

  if (!_exprWhere->Preprocess(_exprTable->_physTable, mapTPos)) {
    return false;
  }

  if (!_exprGroupBy->Preprocess(mapRPos)) {
    return false;
  }
  if (!_exprOrderBy->Preprocess(mapRPos)) {
    return false;
  }

  return true;
}

bool ExprInsert::Preprocess(Database *currDb) {
  if (!_exprTable->Preprocess(currDb)) {
    return false;
  }

  if (_vctCol == nullptr) {
    _vctCol = new MVectorPtr<ExprColumn *>();
    const MVector<PhysColumn> &vctPCol =
        _exprTable->_physTable->GetColumnArray();
    for (const PhysColumn &pcol : vctPCol) {
      ExprColumn *ecol = new ExprColumn(new MString(pcol.GetName()), nullptr,
                                        new MString(pcol.GetName()));
      ecol->_pos = pcol.GetIndex();
      ecol->_dataType = pcol.GetDataType();
      ecol->_dataLength = pcol.GetMaxLength();
      _vctCol->push_back(ecol);
    }
  } else {
    for (ExprColumn *ecol : *_vctCol) {
      const PhysColumn *pcol = _exprTable->_physTable->GetColumn(*ecol->_name);
      if (pcol == nullptr) {
        _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {*ecol->_name}));
        return false;
      }
      ecol->_pos = pcol->GetIndex();
      ecol->_dataType = pcol->GetDataType();
      ecol->_dataLength = pcol->GetMaxLength();
    }
  }

  MVector<ExprElem *> vctExpr;
  vctExpr.reserve(32);
  for (MVectorPtr<ExprElem *> *vctElem : *_vctRowData) {
    if (vctElem->size() != _vctCol->size()) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
      return false;
    }

    for (ExprElem *elem : *vctElem) {
      elem->CollectElem(ExprType::EXPR_FIELD, vctExpr);
    }
  }

  if (!FillElemFiled(_exprTable->_physTable->GetMapColumnPos(), vctExpr)) {
    return false;
  }

  return true;
}

bool ExprUpdate::Preprocess(Database *currDb) {
  if (!_exprTable->Preprocess(currDb)) {
    return false;
  }

  const MStrHashMap<uint32_t> &mapPos =
      _exprTable->_physTable->GetMapColumnPos();
  const storage::MVector<storage::PhysColumn> &vctCmn =
      _exprTable->_physTable->GetColumnArray();
  MVector<ExprElem *> vctElem;
  for (ExprColumn *ecol : *_vctCol) {
    auto iter = mapPos.find(*ecol->_name);
    if (iter != mapPos.end()) {
      ecol->_pos = iter->second;
      ecol->_dataType = vctCmn[iter->second].GetDataType();
      ecol->_dataLength = vctCmn[iter->second].GetMaxLength();
    } else {
      _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {*ecol->_name}));
      return false;
    }

    ecol->_exprElem->CollectElem(ExprType::EXPR_FIELD, vctElem);
  }

  if (!FillElemFiled(mapPos, vctElem)) {
    return false;
  }

  if (_exprWhere != nullptr &&
      !_exprWhere->Preprocess(_exprTable->_physTable, mapPos)) {
    return false;
  }
  if (_exprOrderBy != nullptr && !_exprOrderBy->Preprocess(mapPos)) {
    return false;
  }

  return true;
}

bool ExprDelete::Preprocess(Database *currDb) {
  if (!_exprTable->Preprocess(currDb)) {
    return false;
  }

  const MStrHashMap<uint32_t> &mapPos =
      _exprTable->_physTable->GetMapColumnPos();
  if (_exprWhere != nullptr &&
      !_exprWhere->Preprocess(_exprTable->_physTable, mapPos)) {
    return false;
  }
  if (_exprOrderBy != nullptr && !_exprOrderBy->Preprocess(mapPos)) {
    return false;
  }

  return true;
}
} // namespace storage
