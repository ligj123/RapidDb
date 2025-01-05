#include "../header.h"

namespace storage {
enum class ExprType : uint16_t {
  EXPR_BASE = 0,
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
  EXPR_CREATE_DATABASE = 0x100,
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
  EXPR_SELECT = 0x200,
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

static const char *ExprTypeToStr(ExprType type) {
  switch (type) {
  case ExprType::EXPR_BASE:
    return "EXPR_BASE";
  case ExprType::EXPR_STAR:
    return "EXPR_STAR";
  case ExprType::EXPR_ARRAY:
    return "EXPR_ARRAY";
  case ExprType::EXPR_CONST:
    return "EXPR_CONST";
  case ExprType::EXPR_PARAMETER:
    return "EXPR_PARAMETER";
  case ExprType::EXPR_FIELD:
    return "EXPR_FIELD";
  case ExprType::EXPR_ADD:
    return "EXPR_ADD";
  case ExprType::EXPR_SUB:
    return "EXPR_SUB";
  case ExprType::EXPR_MUL:
    return "EXPR_MUL";
  case ExprType::EXPR_DIV:
    return "EXPR_DIV";
  case ExprType::EXPR_MINUS:
    return "EXPR_MINUS";
  case ExprType::EXPR_DATA_END:
    return "EXPR_DATA_END";
  case ExprType::EXPR_COUNT:
    return "EXPR_COUNT";
  case ExprType::EXPR_SUM:
    return "EXPR_SUM";
  case ExprType::EXPR_MAX:
    return "EXPR_MAX";
  case ExprType::EXPR_MIN:
    return "EXPR_MIN";
  case ExprType::EXPR_AVG:
    return "EXPR_AVG";
  case ExprType::EXPR_AGGR_END:
    return "EXPR_AGGR_END";
  case ExprType::EXPR_COMP:
    return "EXPR_COMP";
  case ExprType::EXPR_IN_OR_NOT:
    return "EXPR_IN_OR_NOT";
  case ExprType::EXPR_IS_NULL_NOT:
    return "EXPR_IS_NULL_NOT";
  case ExprType::EXPR_BETWEEN:
    return "EXPR_BETWEEN";
  case ExprType::EXPR_LIKE:
    return "EXPR_LIKE";
  case ExprType::EXPR_EXIST:
    return "EXPR_EXIST";
  case ExprType::EXPR_NOT_EXIST:
    return "EXPR_NOT_EXIST";
  case ExprType::EXPR_AND:
    return "EXPR_AND";
  case ExprType::EXPR_OR:
    return "EXPR_OR";
  case ExprType::EXPR_NOT:
    return "EXPR_NOT";
  case ExprType::EXPR_LOGIC_END:
    return "EXPR_LOGIC_END";
  case ExprType::EXPR_WHERE:
    return "EXPR_WHERE";
  case ExprType::EXPR_ON:
    return "EXPR_ON";
  case ExprType::EXPR_HAVING:
    return "EXPR_HAVING";
  case ExprType::EXPR_JOIN:
    return "EXPR_JOIN";
  case ExprType::EXPR_GROUP_BY:
    return "EXPR_GROUP_BY";
  case ExprType::EXPR_LIMIT:
    return "EXPR_LIMIT";
  case ExprType::EXPR_ORDER_ITEM:
    return "EXPR_ORDER_ITEM";
  case ExprType::EXPR_ORDER_BY:
    return "EXPR_ORDER_BY";
  case ExprType::EXPR_COLUMN:
    return "EXPR_COLUMN";
  case ExprType::EXPR_RESULT_COLUMN:
    return "EXPR_RESULT_COLUMN";
  case ExprType::EXPR_TABLE:
    return "EXPR_TABLE";
  case ExprType::EXPR_JOIN_TABLE:
    return "EXPR_JOIN_TABLE";
  case ExprType::EXPR_CREATE_DATABASE:
    return "EXPR_CREATE_DATABASE";
  case ExprType::EXPR_DROP_DATABASE:
    return "EXPR_DROP_DATABASE";
  case ExprType::EXPR_SHOW_DATABASES:
    return "EXPR_SHOW_DATABASES";
  case ExprType::EXPR_USE_DATABASE:
    return "EXPR_USE_DATABASE";
  case ExprType::EXPR_CREATE_TABLE:
    return "EXPR_CREATE_TABLE";
  case ExprType::EXPR_DROP_TABLE:
    return "EXPR_DROP_TABLE";
  case ExprType::EXPR_SHOW_TABLES:
    return "EXPR_SHOW_TABLES";
  case ExprType::EXPR_TRUN_TABLE:
    return "EXPR_TRUN_TABLE";
  case ExprType::EXPR_DATA_TYPE:
    return "EXPR_DATA_TYPE";
  case ExprType::EXPR_COLUMN_INFO:
    return "EXPR_COLUMN_INFO";
  case ExprType::EXPR_CONSTRAINT:
    return "EXPR_CONSTRAINT";
  case ExprType::EXPR_TRANSACTION:
    return "EXPR_TRANSACTION";
  case ExprType::EXPR_SELECT:
    return "EXPR_SELECT";
  case ExprType::EXPR_TABLE_SELECT:
    return "EXPR_TABLE_SELECT";
  case ExprType::EXPR_INSERT:
    return "EXPR_INSERT";
  case ExprType::EXPR_UPDATE:
    return "EXPR_UPDATE";
  case ExprType::EXPR_DELETE:
    return "EXPR_DELETE";
  case ExprType::EXPR_FUNCTION:
    return "EXPR_FUNCTION";
  case ExprType::EXPR_LAST:
    return "EXPR_LAST";
  default:
    abort();
    return "UNKNOWN TYPE";
  }
}

inline std::ostream &operator<<(std::ostream &os, const ExprType &type) {
  os << "ExprType::" << ExprTypeToStr(type) << "(" << (int)type << ")";
  return os;
}

} // namespace storage