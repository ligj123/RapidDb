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
    } else {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }
  } else {
    Database *db = DatabaseManager::FindDb(*_dbName);
    if (db == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(DB_NOT_FOUNF, {*_dbName}));
      return false;
    }
  }

  MString tname = *_dbName + "." + *_tName;
  if (!TableManager::FindTable(tname, _physTable)) {
    // In following time, add the code to load table from system table. Now only
    // consider the condition that all tables in memory.

    _threadErrorMsg.reset(new ErrorMsg(TB_INVALID_TABLE_NAME, {tname}));
    return false;
  }

  return true;
}
} // namespace storage
