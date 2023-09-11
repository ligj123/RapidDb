#ifndef SQLPARSER_SQLPARSER_RESULT_H
#define SQLPARSER_SQLPARSER_RESULT_H

#include "../cache/Mallocator.h"
#include "../expr/ExprStatement.h"

namespace storage {
// Represents the result of the SQLParser.
// If parsing was successful it contains a list of ExprStatement.
class SQLParserResult {
public:
  // Initialize with empty statement list.
  SQLParserResult();

  // Initialize with a single statement.
  // Takes ownership of the statement.
  SQLParserResult(ExprStatement *stmt);

  // Move constructor.
  SQLParserResult(SQLParserResult &&moved);
  SQLParserResult &operator=(SQLParserResult &&moved);

  // Deletes all statements in the result.
  virtual ~SQLParserResult();

  // Set whether parsing was successful.
  void SetIsValid(bool isValid);

  // Returns true if parsing was successful.
  bool IsValid() const;

  // Returns the number of statements in the result.
  size_t Size() const;

  // Set the details of the error, if available.
  // Takes ownership of errorMsg.
  void SetErrorDetails(MString &errorMsg, int errorLine, int errorColumn);

  // Returns the error message, if an error occurred.
  const MString &ErrorMsg() const;

  // Returns the line number of the occurrance of the error in the query.
  int ErrorLine() const;

  // Returns the column number of the occurrance of the error in the query.
  int ErrorColumn() const;

  // Adds a statement to the result list of statements.
  // SQLParserResult takes ownership of the statement.
  void AddStatement(ExprStatement *stmt);

  // Gets the SQL statement with the given index.
  const ExprStatement *GetStatement(size_t index) const;

  // Get the list of all statements.
  const MVectorPtr<ExprStatement *> &GetStatements() const;

  // Deletes all statements and other data within the result.
  void Reset();

private:
  // List of statements within the result. In this version only one statement
  MVectorPtr<ExprStatement *> _vctStatement;

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