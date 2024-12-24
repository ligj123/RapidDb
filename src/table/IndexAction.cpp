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
int PrevPageAction::JudgeRange() {
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

TaskStatus StatementAction::Exec() {
  bool b;
  if (_indexTree->GetIndexType() == IndexType::PRIMARY) {
    b = _stmt->PrimaryKeyExec();
  } else {
    b = _stmt->SecondaryKeyExec();
  }

  return b ? TaskStatus::FINISHED : TaskStatus::RUNNING;
}

int StatementAction::JudgeRange() { return _stmt->GetIndexRange(_indexTree); }
} // namespace storage