#include "ExprStatement.h"
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
  // TO DO
  return false;
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
