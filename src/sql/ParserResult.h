#pragma once

#include "../cache/Mallocator.h"
#include "../expr/ExprData.h"
#include "../expr/ExprStatement.h"

namespace storage {
// Represents the result of the SQLParser.
// If parsing was successful it contains a list of ExprStatement.
class ParserResult {
public:
  ParserResult(){};
  ParserResult(ParserResult &&src) { *this = move(src); }
  virtual ~ParserResult() { Reset(); }

  ParserResult &operator=(ParserResult &&src) {
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

  void SetErrorDetails(const char *errorMsg, int errorLine, int errorColumn) {
    _errorMsg = errorMsg;
    _errorLine = errorLine;
    _errorColumn = errorColumn;
  }

  const MString &ErrorMsg() const;
  int ErrorLine() const;
  int ErrorColumn() const;

  void AddStatements(MVectorPtr<ExprStatement *> *vct_stmt) {
    if (_vctStatement != nullptr)
      delete _vctStatement;

    _vctStatement = vct_stmt;
  }
  const MVectorPtr<ExprStatement *> &GetStatements() const {
    return *_vctStatement;
  }
  void AddParameters(MVector<ExprParameter *> &vct_para) {
    size_t ii = _vctPara.size();
    for (ExprParameter *para : vct_para) {
      para->_paraPos = ii;
      ii++;
      _vctPara.push_back(para);
    }
    _vctPara.clear();
  }
  void Reset() {
    if (_vctStatement != nullptr)
      delete _vctStatement;

    _vctStatement = new MVectorPtr<ExprStatement *>();
    _vctPara.clear();
    _isValid = true;
    _errorMsg.clear();
    _errorLine = -1;
    _errorColumn = -1;
  }

private:
  // List of statements within the result. In this version only one statement
  MVectorPtr<ExprStatement *> *_vctStatement;
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
