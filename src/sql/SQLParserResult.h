#ifndef SQLPARSER_SQLPARSER_RESULT_H
#define SQLPARSER_SQLPARSER_RESULT_H

#include "../cache/Mallocator.h"
#include "../expr/ExprData.h"
#include "../expr/ExprStatement.h"

namespace storage {
// Represents the result of the SQLParser.
// If parsing was successful it contains a list of ExprStatement.
class SQLParserResult {
public:
  SQLParserResult(){};
  SQLParserResult(SQLParserResult &&src) { *this = move(src); }
  virtual ~SQLParserResult() { Reset(); }

  SQLParserResult &operator=(SQLParserResult &&src) {
    _isValid = src._isValid;
    _errorMsg = move(src._errorMsg);
    _vctStatement = move(src._vctStatement);
    _errorLine = src._errorLine;
    _errorColumn = src._errorColumn;

    src.Reset();
    return *this;
  }

  void SetIsValid(bool isValid) { _isValid = isValid; }
  bool IsValid() const { return _isValid; }

  void SetErrorDetails(MString &errorMsg, int errorLine, int errorColumn) {
    _errorMsg = move(errorMsg);
    _errorLine = errorLine;
    _errorColumn = errorColumn;
  }

  const MString &ErrorMsg() const;
  int ErrorLine() const;
  int ErrorColumn() const;

  void AddStatements(MVectorPtr<ExprStatement *> &vct_stmt) {
    for (ExprStatement *stmt : vct_stmt) {
      _vctStatement.push_back(stmt);
    }
    vct_stmt.clear();
  }
  const MVectorPtr<ExprStatement *> &GetStatements() const {
    return _vctStatement;
  }
  void AddParameters(MVectorPtr<ExprParameter *> &&vct_para) {
    size_t ii = _vctPara.size();
    for (ExprParameter *para : vct_para) {
      para->_paraPos = ii;
      ii++;
      _vctPara.push_back(para);
    }
    _vctPara.clear();
  }
  void Reset() {
    _vctStatement = MVectorPtr<ExprStatement *>();
    _vctPara.clear();
    _isValid = true;
    _errorMsg.clear();
    _errorLine_ = -1;
    _errorColumn_ = -1;
  }

private:
  // List of statements within the result. In this version only one statement
  MVectorPtr<ExprStatement *> _vctStatement;
  // To record the parameters information, they have been included in statement,
  // do not need to free.
  MVector<ExprParameter *> _vctPara;

  // Flag indicating the parsing was successful.
  bool _isValid{true};

  // Error message, if an error occurred.
  MString _errorMsg;

  // Line number of the occurrance of the error in the query.
  int _errorLine{-1};

  // Column number of the occurrance of the error in the query.
  int _errorColumn{-1};
};

} // namespace storage

#endif // SQLPARSER_SQLPARSER_RESULT_H