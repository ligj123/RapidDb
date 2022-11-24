#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
InsertStatement::InsertStatement(ExprInsert *exprInsert, Transaction *tran)
    : Statement(tran, exprInsert->GetParaTemplate()), _exprInsert(exprInsert) {}

TaskStatus InsertStatement::Execute() {
  if (_status == InsertStatus::INSERT_START) {
    _startTime = MilliSecTime();
  }

  const PhysTable *table = _exprInsert->GetSourTable();
  bool bUpsert = _exprInsert->IsUpsert();

  if (_status == InsertStatus::PRIMARY_PAGE_LOAD)
    goto PRIMARY_PAGE_LOAD;
  if (_status == InsertStatus::SECONDARY_PAGE_LOAD)
    goto SECONDARY_PAGE_LOAD;

// Start to insert a new row.
INSERT_START : {
  VectorDataValue *vdv = _vctParas[_currRow];
  const IndexProp *priProp = table->GetPrimaryKey();
  VectorDataValue pdv;
  pdv.SetRef(true);
  pdv.reserve(priProp->_vctCol.size());
  for (const IndexColumn &col : priProp->_vctCol) {
    pdv.push_back(vdv->at(col.colPos));
  }

  LeafRecord *lr = new LeafRecord(
      priProp->_tree, pdv, *vdv,
      priProp->_tree->GetHeadPage()->GetAndIncRecordStamp(), this);
  _vctRec.push_back(lr);
  _currRec++;
}
// After load primary page and restart this task, goto here
PRIMARY_PAGE_LOAD : {
  LeafRecord *lr = _vctRec[_currRec];
  IndexTree *tree = lr->GetTreeFile();
  bool b = tree->SearchRecursively(*lr, true, _indexPage, false);
  if (!b) {
    _status = InsertStatus::PRIMARY_PAGE_LOAD;
    return TaskStatus::PAUSE_WITHOUT_ADD;
  }
  
}
// Loop to insert secondary lead records.
SECONDARY_RECORD:
// After load secondary page and restart this task, goto here
SECONDARY_PAGE_LOAD:

  return 0;
}
} // namespace storage
