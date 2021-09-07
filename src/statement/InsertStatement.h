#pragma once
#include "../expr/ExprStatement.h"
#include "Statement.h"

namespace storage {
class InsertStatement : public Statement {
public:
  InsertStatement(ExprInsert *exprInsert) : _exprInsert(exprInsert) {
    const VectorDataValue &vctPara = exprInsert->GetParameters();
    _vctPara.reserve(vctPara.size());
    for (auto dv : vctPara) {
      _vctPara.push_back(dv->Clone());
    }
  }
  ~InsertStatement() {}

protected:
  // ExprInsert will be managed unified by a class, do not delete here
  ExprInsert *_exprInsert;
};
} // namespace storage
