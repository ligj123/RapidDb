#include "BaseExpr.h"

#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../serv/Session.h"

using namespace std;
namespace storage {
bool ExprTable::Preprocess(Database *currDb) {
  if (_dbName == nullptr) {
    if (currDb != nullptr) {
      _dbName = new MString(currDb->GetDbName());
      _db = currDb;
    } else {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }
  } else {
    _db = DatabaseManager::FindDb(*_dbName);
    if (_db == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(DB_NOT_FOUNF, {*_dbName}));
      return false;
    }
  }

  MString fname = *_dbName + "." + *_tName;
  if (!TableManager::FindTable(fname, _physTable)) {
    _threadErrorMsg.reset(new ErrorMsg(TB_INVALID_TABLE_NAME, {*_tName}));
    return false;
  }

  return true;
}

std::ostream &operator<<(std::ostream &os, const TriBool &b) {
  switch (b) {
  case TriBool::Error:
    os << "Error(" << (int)TriBool::Error << ")";
    break;
  case TriBool::False:
    os << "False(" << (int)TriBool::False << ")";
    break;
  case TriBool::True:
    os << "True(" << (int)TriBool::True << ")";
    break;
  default:
    os << "UNKNOWN TriBool";
    break;
  }
  return os;
}
} // namespace storage
