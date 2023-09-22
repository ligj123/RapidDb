#ifndef __PARSER_TYPEDEF_H__
#define __PARSER_TYPEDEF_H__

#include "../cache/Mallocator.h"
#include "../expr/ExprData.h"
#include <vector>

#ifndef YYtypeDEF_YY_SCANNER_T
#define YYtypeDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#define YYSTYPE DB_STYPE
#define YYLTYPE DB_LTYPE

struct DB_CUST_LTYPE {
  int first_line;
  int first_column;
  int last_line;
  int last_column;

  int total_column;

  // Length of the string in the SQL query string
  int string_length;

  // Parameters.
  // int param_id;
  storage::MVector<storage::ExprParameter *> param_list;
};

#define DB_LTYPE DB_CUST_LTYPE
#define DB_LTYPE_IS_DECLARED 1

#endif