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

  PhysTable *table = _exprInsert->GetSourTable();
  bool bUpsert = _exprInsert->IsUpsert();

  if (_status == InsertStatus::PRIMARY_PAGE_LOAD)
    goto PRIMARY_PAGE_LOAD;
  if (_status == InsertStatus::SECONDARY_PAGE_LOAD)
    goto SECONDARY_PAGE_LOAD;

// Start to insert a new row.
INSERT_START : {
  if (_currRow >= _vctParas.size()) {
    return TaskStatus::FINISHED;
  }

  VectorDataValue *vdv = _vctParas[_currRow];
  const IndexProp &priProp = table->GetPrimaryKey();
  VectorDataValue pdv;
  pdv.SetRef(true);
  pdv.reserve(priProp._vctCol.size());
  for (const IndexColumn &col : priProp._vctCol) {
    pdv.push_back(vdv->at(col.colPos));
  }

  LeafRecord *lr =
      new LeafRecord(priProp._tree, pdv, *vdv,
                     priProp._tree->GetHeadPage()->GetAndIncRecordStamp(), this,
                     table->GetKeySavePosition());
  _vctRecord.push_back(lr);
  _currRow++;
  _indexPage = nullptr;
}
// After load primary page and restart this task, goto here
PRIMARY_PAGE_LOAD : {
  LeafRecord *lr = _vctRecord[_currRec];
  IndexTree *tree = lr->GetTreeFile();
  bool b = tree->SearchRecursively(*lr, true, _indexPage, false);
  if (!b) {
    _status = InsertStatus::PRIMARY_PAGE_LOAD;
    return TaskStatus::PAUSE_WITHOUT_ADD;
  }

  assert(_indexPage->GetPageType() == PageType::LEAF_PAGE);
  LeafPage *lp = (LeafPage *)_indexPage;
  bool bFind;
  int32_t pos = lp->SearchRecord(*lr, bFind);
  VectorLeafRecord vctRec;

  if (bFind) {
    if (bUpsert) {
      LeafRecord *lrSrc = lp->GetRecord(pos);
      table->GenSecondaryRecords(lrSrc, lr, *_vctParas[_currRow],
                                 ActionType::INSERT, this, vctRec);
      int32_t len = lrSrc->UpdateRecord(
          *_vctParas[_currRow],
          table->GetPrimaryKey()._tree->GetHeadPage()->GetAndIncRecordStamp(),
          this, ActionType::UPSERT, false);
      lp->UpdateTotalLength(len);
      lr->DecRef();
      _vctRecord[_currRec] = lrSrc;
    } else {
      LeafPage::RollbackLeafRecords(_vctRecord, _currRec);
      _errorMsg.reset(new ErrorMsg(CORE_REPEATED_RECORD, {}));
      return TaskStatus::FINISHED;
    }
  } else {
    table->GenSecondaryRecords(nullptr, lr, *_vctParas[_currRow],
                               ActionType::INSERT, this, vctRec);
    lp->InsertRecord(lr, pos, true);
  }

  for (LeafRecord *slr : vctRec) {
    _vctRecord.push_back(slr);
  }
  vctRec.clear();
  _currRec++;
}

// Loop to insert secondary lead records.
SECONDARY_RECORD:
  if (_currRow >= _vctRecord.size()) {
    goto INSERT_START;
  }
  _indexPage = nullptr;

// After load secondary page and restart this task, goto here
SECONDARY_PAGE_LOAD : {
  LeafRecord *lr = _vctRecord[_currRec];
  IndexTree *tree = lr->GetTreeFile();
  bool b = tree->SearchRecursively(*lr, true, _indexPage, false);
  if (!b) {
    _status = InsertStatus::PRIMARY_PAGE_LOAD;
    return TaskStatus::PAUSE_WITHOUT_ADD;
  }

  assert(_indexPage->GetPageType() == PageType::LEAF_PAGE);
  LeafPage *lp = (LeafPage *)_indexPage;
  bool bFind;
  int32_t pos = lp->SearchRecord(*lr, bFind);
  if (bFind) {
    if (lr->GetAction() == ActionType::DELETE) {
      LeafRecord *lrSrc = lp->GetRecord(pos);
      lr->DecRef();
      _vctRecord[_currRec] = lrSrc;
    } else {
      // For ActionType::INSERT, Here should not find the record or has error
      // code
      abort();
    }
  } else {
    if (lr->GetAction() == ActionType::INSERT) {
      lp->InsertRecord(lr, pos, true);
    } else {
      abort();
    }
  }
}

  return TaskStatus::FINISHED;
}
} // namespace storage
