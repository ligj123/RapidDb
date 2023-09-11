
#include "SQLParserResult.h"
#include <algorithm>

namespace storage {

SQLParserResult::SQLParserResult(){};

SQLParserResult::SQLParserResult(ExprStatement *stmt) {
  _vctStatement.push_back(stmt);
};

// Move constructor.
SQLParserResult::SQLParserResult(SQLParserResult &&src) { *this = move(src); }

SQLParserResult &SQLParserResult::operator=(SQLParserResult &&src) {
  _isValid = src._isValid;
  _errorMsg = move(src._errorMsg);
  _vctStatement = move(src._vctStatement);
  _errorLine = src._errorLine;
  _errorColumn = src._errorColumn;

  src.Reset();
  return *this;
}

SQLParserResult::~SQLParserResult() { Reset(); }

void SQLParserResult::AddStatement(ExprStatement *stmt) {
  _vctStatement.push_back(stmt);
}

const ExprStatement *SQLParserResult::GetStatement(size_t index) const {
  return _vctStatement[index];
}

size_t SQLParserResult::Size() const { return _vctStatement.size(); }

bool SQLParserResult::IsValid() const { return _isValid; }

const MString &SQLParserResult::ErrorMsg() const { return _errorMsg; }

int SQLParserResult::ErrorLine() const { return _errorLine; }

int SQLParserResult::ErrorColumn() const { return _errorColumn; }

void SQLParserResult::SetIsValid(bool isValid) { _isValid = isValid; }

void SQLParserResult::SetErrorDetails(MString &&errorMsg, int errorLine,
                                      int errorColumn) {
  _errorMsg = move(errorMsg);
  _errorLine = errorLine;
  _errorColumn = errorColumn;
}

const MVectorPtr<ExprStatement *> &SQLParserResult::GetStatements() const {
  return _vctStatement;
}

void SQLParserResult::reset() {
  _vctStatement.clear();
  _isValid = false;
  _errorMsg.clear();
  _errorLine_ = -1;
  _errorColumn_ = -1;
}
} // namespace storage
