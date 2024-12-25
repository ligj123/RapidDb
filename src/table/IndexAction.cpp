#include "IndexAction.h"

#include "../core/BranchPage.h"
#include "../core/BranchRecord.h"
#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../serv/SessionPool.h"
#include "../statement/InsertStatement.h"
#include "../statement/Statement.h"
#include "../utils/ThreadPool.h"
#include "Table.h"
#include "TableTaskMgr.h"

namespace storage {
int PrevPageAction::JudgeRange(bool recalc) {
  if (!recalc) {
    return _rangePos;
  }

  if (_page == nullptr) {
    _page = (LeafPage *)_indexTree->GetPage(_pageId, PageType::LEAF_PAGE,
                                            nullptr, true);
  }

  if (_page->GetRecordNumber() > 0) {
    LeafRecord &lr = _page->GetRecord(0);
    _rangePos = _indexTree->CalcIndexRange(lr);
  } else {
    BranchPage *parent = _page->GetParentPage();
    if (parent == nullptr) {
      parent = (BranchPage *)_indexTree->GetPage(
          _page->GetParentPageId(), PageType::BRANCH_PAGE, nullptr, true);
    }

    BranchRecord &br = parent->GetRecord(0, false);
    _rangePos = _indexTree->CalcIndexRange(br);
  }

  return _rangePos;
}

TaskStatus PrevPageAction::Exec() {
  assert(_rangePos >= 0 && _rangePos < _indexTree->GetVctRange().size());
  if (_page == nullptr) {
    _page = (LeafPage *)_indexTree->GetPage(_pageId, PageType::LEAF_PAGE,
                                            nullptr, true);
  }

  PageStatus s = _page->GetPageStatus();
  if (s == PageStatus::READING) {
    _page->SetPageStatus(PageStatus::VALID, true);
  } else if (s != PageStatus::VALID) {
    return TaskStatus::RUNNING;
  }

  _page->SetPrevPageId(_prevPageId);
  return TaskStatus::FINISHED;
}

TaskStatus RecordAction::Exec() {
  if (_idxPage == nullptr) {
    _idxPage = _indexTree->GetRootPage();
  }
  if (_idxPage->GetPageType() != PageType::LEAF_PAGE) {
    bool b = _indexTree->SearchPage(*_lr, _idxPage);
    if (!b) {
      return TaskStatus::RUNNING;
    }
  }

  RecordLock *lock = _lr->GetLock();
  if (lock->GetRecordStatus() != RecordStatus::INIT) {
    assert(lock->GetRecordResult() != RecordResult::IN_PAGE &&
           lock->GetRecordStatus() == RecordStatus::ROLLBACKED);
    delete _lr;
    _lr = nullptr;
    return TaskStatus::FINISHED;
  }

  LeafPage *lp = (LeafPage *)_idxPage;
  lp->UpdateAction(_lr);
  if (_lr->GetLock()->_recResult == RecordResult::ERROR) {
    _indexTree->GetVctRange()[_rangePos]._vctErrRecord.push_back(_lr);
  }

  _lr = nullptr;
  return TaskStatus::FINISHED;
}

int RecordAction::JudgeRange(bool recalc) {
  if (!recalc) {
    return _rangePos;
  }

  _rangePos = _indexTree->CalcIndexRange(*_lr);
  return _rangePos;
}

TaskStatus StatementAction::Exec() {
  bool b;
  if (_indexTree->GetIndexType() == IndexType::PRIMARY) {
    b = _stmt->PrimaryKeyExec(_rangePos);
  } else {
    b = _stmt->SecondaryKeyExec(_rangePos);
  }

  return b ? TaskStatus::FINISHED : TaskStatus::RUNNING;
}

int StatementAction::JudgeRange(bool recalc) {
  if (!recalc) {
    return _rangePos;
  }

  _rangePos = _stmt->GetIndexRange(_indexTree);
  return _rangePos;
}
} // namespace storage