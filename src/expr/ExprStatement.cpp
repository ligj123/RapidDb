#include "ExprStatement.h"

#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../serv/Session.h"
#include "../table/Table.h"

namespace storage {

bool FillElemFiled(MVector<ExprElem *> &vctElem, PhysTable *table) {
  const storage::MStrHashMap<uint32_t> &mapPos =
      _exprTable->_physTable->GetMapColumnPos();
  for (ExprElem *elem : vctElem) {
    ExprField *field = (ExprField *)elem;
    auto iter = mapPos.find(field->_colName);
    if (iter == mapPos.end()) {
      _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {*ecol->_name}));
      return false;
    }

    field->_rowPos = iter->second;
  }

  return true;
}

ExprInsert::~ExprInsert() {
  delete _exprTable;
  delete _vctCol;
  delete _vctRowData;
  delete _exprSelect;

  if (_physTable != nullptr)
    _physTable->DecRef();
}

ExprUpdate::~ExprUpdate() {
  delete _exprTable;
  delete _vctCol;
  delete _exprWhere;
  delete _exprOrderBy;
  delete _exprLimit;

  if (_physTable != nullptr)
    _physTable->DecRef();
}

ExprDelete::~ExprDelete() {
  delete _exprTable;
  delete _exprWhere;
  delete _exprOrderBy;
  delete _exprLimit;

  if (_physTable != nullptr)
    _physTable->DecRef();
}

bool ExprWhere::Preprocess(PhysTable *table) {
  MVector<ExprElem *> vctElem;
  vctElem.reserve(32);
  _exprLogic->CollectElem(ExprType::EXPR_FIELD, vctElem);
  if (!FillElemFiled(vctElem, table)) {
    return false;
  }

  // if (_exprLogic->GetType()==ExprType::EXPR_FIELD)
}

bool ExprSelect::Preprocess(Database *currDb) {
  // TO DO
  return false;
}

bool ExprInsert::Preprocess(Database *currDb) {
  if (!_exprTable->Preprocess(session)) {
    return false;
  }

  if (_vctCol->size() == 0) {
    const MVector<PhysColumn> &vctPCol = _physTable->GetColumnArray();
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
      const PhysColumn *pcol = _physTable->GetColumn(*ecol->_name);
      if (pcol == nullptr) {
        _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {*ecol->_name}));
        return false;
      }
      ecol->_pos = pcol->GetIndex();
      ecol->_dataType = pcol->GetDataType();
      ecol->_dataLength = pcol->GetMaxLength();
    }
  }

  MVector<ExprElem *> vctElem;
  vctElem.reserve(32);
  for (MVectorPtr<ExprElem *> *vctElem : *_vctRowData) {
    if (vctElem->size() != _vctCol->size()) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
      return false;
    }

    for (ExprElem *elem : *vctElem) {
      elem->CollectElem(ExprType::EXPR_FIELD, vctElem);
    }
  }

  if (!FillElemFiled(vctElem, table)) {
    return false;
  }

  return true;
}

bool ExprUpdate::Preprocess(Database *currDb) {
  // TO DO
  return false;
}

bool ExprDelete::Preprocess(Database *currDb) {
  // TO DO
  return false;
}
} // namespace storage
