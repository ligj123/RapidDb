
#include "../serv/Session.h"
#include "../utils/ErrorMsg.h"
#include "ExprDdl.h"
#include "ExprStatement.h"

namespace storage {
using namespace std;

bool ExprCreateTable::Preprocess(Session *session) {
  if (_table->_dbName == nullptr) {
    assert(session != nullptr);
    const Database *db = session->GetCurrDb();
    if (db == nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(SESSION_NO_CURR_DB, {}));
      return false;
    }

    _table->_dbName = new MString(db->GetDbName());
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

  return true;
}

bool ExprDropTable::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprShowTables::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprTrunTable::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprTransaction::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprSelect::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprInsert::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprUpdate::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprDelete::Preprocess(Session *session) {
  // TO DO
  return false;
}
} // namespace storage