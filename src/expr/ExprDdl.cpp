
#include "ExprDdl.h"
#include "../manager/DatabaseManager.h"
#include "../serv/Session.h"
#include "../utils/ErrorMsg.h"
#include "ExprStatement.h"

namespace storage {
using namespace std;

bool ExprCreateTable::Preprocess(Database *currDb) {
  if (_table->_dbName == nullptr) {
    if (currDb == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }

    _table->_dbName = new MString(currDb->GetDbName());
    _table->_db = currDb;
  } else {
    _table->_db = DatabaseManager::FindDb(*_table->_dbName);
    if (_table->_db == nullptr) {
      if (currDb == nullptr) {
        _threadErrorMsg.reset(
            new ErrorMsg(DDL_DATABASE_NOT_EXIST, {*_table->_dbName}));
        return false;
      }
    }
  }

  for (ExprCreateTableItem *item : *_vctItem) {
    if (item->GetType() == ExprType::EXPR_COLUMN_INFO) {
      ExprColumnItem *col = (ExprColumnItem *)item;
      _vctColumn.push_back(col);

      if (col->_indexType != IndexType::UNKNOWN) {
        MString *name = new MString(*col->_colName);
        MVectorPtr<MString *> *vct = new MVectorPtr<MString *>();
        vct->push_back(new MString(*col->_colName));
        ExprTableIndex *tindex = new ExprTableIndex(name, col->_indexType, vct);
        if (tindex->_idxType == IndexType::PRIMARY)
          _vctIndex.insert(_vctIndex.begin(), tindex);
        else
          _vctIndex.push_back(tindex);
      }
    } else {
      ExprTableIndex *tindex = (ExprTableIndex *)item;
      if (tindex->_idxType == IndexType::PRIMARY)
        _vctIndex.insert(_vctIndex.begin(), tindex);
      else
        _vctIndex.push_back(tindex);
    }
  }

  _vctItem->clear();
  _physTable = new PhysTable(currDb, *_table->_tName, UINT32_MAX,
                             *_table->_tName, MilliSecTime(), MilliSecTime());

  for (ExprColumnItem *col : _vctColumn) {
    if (col->_autoInc) {
      _physTable->AddColumn(*col->_colName, col->_dataType,
                            col->_comment == nullptr ? "" : *col->_comment,
                            col->_initVal, col->_incStep);
    } else {
      _physTable->AddColumn(
          *col->_colName, col->_dataType, col->_nullable, col->_maxLength,
          col->_comment == nullptr ? "" : *col->_comment, Charsets::UTF8,
          col->_defaultVal == nullptr ? nullptr
                                      : col->_defaultVal->Clone(true));
    }
  }

  for (ExprTableIndex *tindex : _vctIndex) {
    MVector<MString> vct;
    vct.reserve(tindex->_vctColName->size());
    for (MString *pstr : *tindex->_vctColName) {
      vct.push_back(*pstr);
    }
    _physTable->AddIndex(tindex->_idxType, *tindex->_idxName, vct);
  }

  return true;
}

bool ExprDropTable::Preprocess(Database *currDb) {
  if (_table->_dbName == nullptr) {
    if (currDb == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }

    _table->_dbName = new MString(currDb->GetDbName());
    _table->_db = currDb;
  } else {
    _table->_db = DatabaseManager::FindDb(*_table->_dbName);
  }

  return true;
}

bool ExprTrunTable::Preprocess(Database *currDb) {
  if (_table->_dbName == nullptr) {
    if (currDb == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }

    _table->_dbName = new MString(currDb->GetDbName());
  }

  return true;
}

} // namespace storage