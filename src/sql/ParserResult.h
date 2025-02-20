#pragma once

#include "../cache/Mallocator.h"
#include "../expr/ExprData.h"
#include "../expr/ExprStatement.h"
#include "../utils/Log.h"

namespace storage {
// Represents the result of the SQLParser.
// If parsing was successful it contains a list of ExprStatement.
class ParserResult {
public:
  ParserResult(){};
  ParserResult(ParserResult &&src) { *this = std::move(src); }
  virtual ~ParserResult() { delete _vctStatement; }

  ParserResult &operator=(ParserResult &&src) {
    _isValid = src._isValid;
    _errorMsg = std::move(src._errorMsg);
    _vctStatement = src._vctStatement;
    src._vctStatement = nullptr;
    _errorLine = src._errorLine;
    _errorColumn = src._errorColumn;

    src.Reset();
    return *this;
  }

  void SetIsValid(bool isValid) { _isValid = isValid; }
  bool IsValid() const { return _isValid; }

  void SetErrorDetails(const char *errorMsg, int errorLine, int errorColumn) {
    _errorMsg = errorMsg;
    _errorMsg += "  errLine: " + ToMString(_errorLine) +
                 "  errCol: " + ToMString(_errorColumn);
    _errorLine = errorLine;
    _errorColumn = errorColumn;
  }

  MString &ErrorMsg() { return _errorMsg; }
  int ErrorLine() const { return _errorLine; }
  int ErrorColumn() const { return _errorColumn; }

  void AddStatements(MVectorPtr<ExprStatement *> *vct_stmt) {
    if (_vctStatement != nullptr)
      delete _vctStatement;

    _vctStatement = vct_stmt;
  }
  MVectorPtr<ExprStatement *> *GetStatements() { return _vctStatement; }
  void AddParameters(MVector<ExprParameter *> &vct_para) {
    size_t ii = _vctPara.size();
    for (ExprParameter *para : vct_para) {
      para->_paraPos = ii;
      ii++;
      _vctPara.push_back(para);
    }
    vct_para.clear();

    if (_vctStatement != nullptr) {
      for (ExprStatement *es : *_vctStatement) {
        es->_vctPara = _vctPara;
      }
    }
  }
  const MVector<ExprParameter *> *GetVctPara() { return &_vctPara; }
  void Reset() {
    if (_vctStatement != nullptr) {
      delete _vctStatement;
      _vctStatement = nullptr;
    }

    _vctPara.clear();
    _isValid = true;
    _errorMsg.clear();
    _errorLine = -1;
    _errorColumn = -1;
  }

private:
  // List of statements within the result. In this version only one statement
  MVectorPtr<ExprStatement *> *_vctStatement{nullptr};
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
