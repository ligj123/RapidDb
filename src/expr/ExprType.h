#include "../header.h"

namespace storage {
enum class ExprType {
  EXPR_BASE,
  EXPR_STAR,

  // const type
  EXPR_ARRAY,

  // data value type
  EXPR_CONST,
  EXPR_PARAMETER,
  EXPR_FIELD,
  EXPR_ADD,
  EXPR_SUB,
  EXPR_MUL,
  EXPR_DIV,
  EXPR_MINUS,
  EXPR_DATA_END,

  // Aggr
  EXPR_COUNT,
  EXPR_SUM,
  EXPR_MAX,
  EXPR_MIN,
  EXPR_AVG,
  EXPR_AGGR_END,

  // bool value type
  EXPR_COMP,
  EXPR_IN_OR_NOT,
  EXPR_IS_NULL_NOT,
  EXPR_BETWEEN,
  EXPR_LIKE,
  EXPR_EXIST,
  EXPR_NOT_EXIST,
  EXPR_AND,
  EXPR_OR,
  EXPR_NOT,
  EXPR_LOGIC_END,

  // Statement part
  EXPR_WHERE,
  EXPR_ON,
  EXPR_HAVING,
  EXPR_JOIN,
  EXPR_GROUP_BY,
  EXPR_LIMIT,
  EXPR_ORDER_ITEM,
  EXPR_ORDER_BY,

  // Input or oupt value and table
  EXPR_COLUMN,
  EXPR_RESULT_COLUMN,
  EXPR_TABLE,
  EXPR_JOIN_TABLE,

  // DDL
  EXPR_CREATE_DATABASE,
  EXPR_DROP_DATABASE,
  EXPR_SHOW_DATABASES,
  EXPR_USE_DATABASE,
  EXPR_CREATE_TABLE,
  EXPR_DROP_TABLE,
  EXPR_SHOW_TABLES,
  EXPR_TRUN_TABLE,
  EXPR_DATA_TYPE,
  EXPR_COLUMN_INFO,
  EXPR_CONSTRAINT,
  EXPR_TRANSACTION,

  // Statement
  EXPR_SELECT,
  EXPR_TABLE_SELECT,
  EXPR_INSERT,
  EXPR_UPDATE,
  EXPR_DELETE,

  // Internal function fro SQL. In this version, all function input ExprData*
  // and output ExprData*
  EXPR_FUNCTION,

  EXPR_LAST
};
static const char ExprStr[][32] = {"EXPR_BASE",
                                   "EXPR_STAR",
                                   "EXPR_ARRAY",
                                   "EXPR_CONST",
                                   "EXPR_PARAMETER",
                                   "EXPR_FIELD",
                                   "EXPR_ADD",
                                   "EXPR_SUB",
                                   "EXPR_MUL",
                                   "EXPR_DIV",
                                   "EXPR_MINUS",
                                   "EXPR_DATA_END",
                                   "EXPR_COUNT",
                                   "EXPR_SUM",
                                   "EXPR_MAX",
                                   "EXPR_MIN",
                                   "EXPR_AVG",
                                   "EXPR_AGGR_END",
                                   "EXPR_COMP",
                                   "EXPR_IN_OR_NOT",
                                   "EXPR_IS_NULL_NOT",
                                   "EXPR_BETWEEN",
                                   "EXPR_LIKE",
                                   "EXPR_EXIST",
                                   "EXPR_NOT_EXIST",
                                   "EXPR_AND",
                                   "EXPR_OR",
                                   "EXPR_NOT",
                                   "EXPR_LOGIC_END",
                                   "EXPR_WHERE",
                                   "EXPR_ON",
                                   "EXPR_HAVING",
                                   "EXPR_JOIN",
                                   "EXPR_GROUP_BY",
                                   "EXPR_LIMIT",
                                   "EXPR_ORDER_ITEM",
                                   "EXPR_ORDER_BY",
                                   "EXPR_COLUMN",
                                   "EXPR_RESULT_COLUMN",
                                   "EXPR_TABLE",
                                   "EXPR_JOIN_TABLE",
                                   "EXPR_CREATE_DATABASE",
                                   "EXPR_DROP_DATABASE",
                                   "EXPR_SHOW_DATABASES",
                                   "EXPR_USE_DATABASE",
                                   "EXPR_CREATE_TABLE",
                                   "EXPR_DROP_TABLE",
                                   "EXPR_SHOW_TABLES",
                                   "EXPR_TRUN_TABLE",
                                   "EXPR_DATA_TYPE",
                                   "EXPR_COLUMN_INFO",
                                   "EXPR_CONSTRAINT",
                                   "EXPR_TRANSACTION",
                                   "EXPR_SELECT",
                                   "EXPR_TABLE_SELECT",
                                   "EXPR_INSERT",
                                   "EXPR_UPDATE",
                                   "EXPR_DELETE",
                                   "EXPR_FUNCTION",
                                   "EXPR_LAST"};

inline std::ostream &operator<<(std::ostream &os, const ExprType &type) {

  os << "ExprType::";
  int tp = (int)type;
  if (tp >= 0 && tp < (int)ExprType::EXPR_LAST) {
    os << ExprStr[tp] << "(" << tp << ")";
  } else {
    os << "UNKNOWN(" << tp << ")";
  }

  return os;
}

} // namespace storage