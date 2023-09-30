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

enum class TranAction : int8_t { TRAN_BEGIN = 0, TRAN_COMMIT, TRAN_ROLLBACK };

enum class JoinType {
  JOIN_NULL,
  INNER_JOIN,
  LEFT_JOIN,
  RIGHT_JOIN,
  OUTTER_JOIN
};

enum class CompType { EQ, GT, GE, LT, LE, NE };

enum class LockType { NO_LOCK, SHARE_LOCK, WRITE_LOCK };

inline std::ostream &operator<<(std::ostream &os, const TranAction &action) {
  switch (action) {
  case TranAction::TRAN_BEGIN:
    os << "TRAN_BEGIN(" << (int)TranAction::TRAN_BEGIN << ")";
    break;
  case TranAction::TRAN_COMMIT:
    os << "TRAN_COMMIT(" << (int)TranAction::TRAN_COMMIT << ")";
    break;
  case TranAction::TRAN_ROLLBACK:
    os << "TRAN_ROLLBACK(" << (int)TranAction::TRAN_ROLLBACK << ")";
    break;
  default:
    os << "UNKNOWN Action";
    break;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const JoinType &type) {
  switch (type) {
  case JoinType::JOIN_NULL:
    os << "JOIN_NULL(" << (int)JoinType::JOIN_NULL << ")";
    break;
  case JoinType::INNER_JOIN:
    os << "INNER_JOIN(" << (int)JoinType::INNER_JOIN << ")";
    break;
  case JoinType::LEFT_JOIN:
    os << "LEFT_JOIN(" << (int)JoinType::LEFT_JOIN << ")";
    break;
  case JoinType::RIGHT_JOIN:
    os << "RIGHT_JOIN(" << (int)JoinType::RIGHT_JOIN << ")";
    break;
  case JoinType::OUTTER_JOIN:
    os << "OUTTER_JOIN(" << (int)JoinType::OUTTER_JOIN << ")";
    break;
  default:
    os << "UNKNOWN Action";
    break;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const CompType &type) {
  switch (type) {
  case CompType::EQ:
    os << "EQ(" << (int)CompType::EQ << ")";
    break;
  case CompType::GT:
    os << "GT(" << (int)CompType::GT << ")";
    break;
  case CompType::GE:
    os << "GE(" << (int)CompType::GE << ")";
    break;
  case CompType::LT:
    os << "LT(" << (int)CompType::LT << ")";
    break;
  case CompType::LE:
    os << "LE(" << (int)CompType::LE << ")";
    break;
  case CompType::NE:
    os << "NE(" << (int)CompType::NE << ")";
    break;
  default:
    os << "UNKNOWN CompType";
    break;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const LockType &type) {
  switch (type) {
  case LockType::NO_LOCK:
    os << "NO_LOCK(" << (int)LockType::NO_LOCK << ")";
    break;
  case LockType::SHARE_LOCK:
    os << "SHARE_LOCK(" << (int)LockType::SHARE_LOCK << ")";
    break;
  case LockType::WRITE_LOCK:
    os << "WRITE_LOCK(" << (int)LockType::WRITE_LOCK << ")";
    break;
  default:
    os << "UNKNOWN LockType";
    break;
  }
  return os;
}

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