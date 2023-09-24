#include "Parser.h"
#include "../utils/ErrorMsg.h"
#include "./sql_parser.h"
#include "flex_lexer.h"

namespace storage {

bool Parser::Parse(const MString &sql, ParserResult &result) {
  yyscan_t scanner;
  YY_BUFFER_STATE state;

  if (db_lex_init(&scanner)) {
    _threadErrorMsg.reset(new ErrorMsg(SQL_PARSER_INIT_FAILED));
    return false;
  }
  const char *text = sql.c_str();
  state = db__scan_string(text, scanner);

  int ret = db_parse(&result, scanner);
  result.SetIsValid(ret == 0);

  db__delete_buffer(state, scanner);
  db_lex_destroy(scanner);

  return true;
}

bool Parser::Tokenize(const MString &sql, MVector<int16_t> &tokens) {
  // Initialize the scanner.
  yyscan_t scanner;
  if (db_lex_init(&scanner)) {
    _threadErrorMsg.reset(new ErrorMsg(SQL_PARSER_INIT_FAILED));
    return false;
  }

  YY_BUFFER_STATE state;
  state = db__scan_string(sql.c_str(), scanner);

  YYSTYPE yylval;
  YYLTYPE yylloc;

  // Step through the string until EOF is read.
  // Note: hsql_lex returns int, but we know that its range is within 16 bit.
  int16_t token = db_lex(&yylval, &yylloc, scanner);
  while (token != 0) {
    tokens.push_back(token);
    token = db_lex(&yylval, &yylloc, scanner);

    if (token == SQL_IDENTIFIER || token == SQL_STRING) {
      free(yylval.sval);
    }
  }

  db__delete_buffer(state, scanner);
  db_lex_destroy(scanner);
  return true;
}
} // namespace storage