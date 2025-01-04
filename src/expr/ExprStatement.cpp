#include "ExprStatement.h"

#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../serv/Session.h"
#include "../table/Table.h"

namespace storage {
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

bool ExprSelect::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprInsert::Preprocess(Session *session) {
  if (_exprTable->_dbName == nullptr) {
    if (session->_currDb != nullptr) {
      _exprTable->_dbName = new MString(session->_currDb->GetDbName());
    } else {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }
  } else {
    Database *db = DatabaseManager::FindDb(*_exprTable->_dbName);
    if (db == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(DB_NOT_FOUNF, {*_exprTable->_dbName}));
      return false;
    }
  }

  MString tname = *_exprTable->_dbName + "." + *_exprTable->_tName;
  if (!TableManager::FindTable(tname, _physTable)) {
    // In following time, add the code to load table from system table. Now only
    // consider the condition that all tables in memory.

    _threadErrorMsg.reset(new ErrorMsg(TB_INVALID_TABLE_NAME, {tname}));
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

  for (MVectorPtr<ExprElem *> *vctElem : *_vctRowData) {
    if (vctElem->size() != _vctCol->size()) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
      return false;
    }
  }

  return true;
}

bool ExprUpdate::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprDelete::Preprocess(Session *session) {
  // TO DO
  return false;
}
} // namespace storage
