#include "IndexAction.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include "LeafRecord.h"

namespace storage {
TaskStatus PrevPageAction::Run() {
  assert(_rangePos >= 0 && _rangePos < _indexTree->GetVctRange().size());
  if (_idxPage == nullptr) {
    _idxPage = _indexTree->GetVctRange()[_rangePos]._vctRangePage[0];
  }

  while (true) {
    PageStatus ps = _idxPage->GetPageStatus();
    if (ps == PageStatus::READING) {
      return TaskStatus::RUNNING;
    }

    if (ps == PageStatus::READED) {
      _idxPage->SetPageStatus(PageStatus::VALID, memory_order_acquire);
    }

    if (_idxPage->GetPageType() == PageType::LEAF_PAGE) {
      LeafPage *lp = (LeafPage *)_idxPage;
      assert(lp->GetPageId() == _pageId);
      lp->SetPrevPageId(_prevPageId);
      return TaskStatus::FINISHED;
    } else {
      BranchPage *bp = (BranchPage *)_idxPage;
      _idxPage = bp->GetChild(0);
      if (_idxPage == nullptr) {
        BranchRecord &br = bp->GetRecord(0, false);
        PageType type = bp->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                                : PageType::BRANCH_PAGE;
        _idxPage = _indexTree->GetPage(br.GetChildPageId(), type, bp);
      }
    }
  }
}

TaskStatus InsertAction::Run() {
  _idxPage = _indexTree->GetRootPage();
  bool b = _indexTree->SearchPage(*_lr, _idxPage);
  if (!b) {
    return TaskStatus::RUNNING;
  }
  assert(_idxPage->GetPageType() == PageType::LEAF_PAGE);
  LeafPage *lp = (LeafPage *)_idxPage;
  bool bFind;
  int pos = lp->SearchRecord(*_lr, bFind);
  if (_lr->GetAction() == ActionType::DELETE) {
    assert(bFind);
    // LeafRecord
  } else {
  }

  return TaskStatus::FINISHED;
}

int InsertAction::JudgeRange() { return _indexTree->CalcIndexRange(*_lr); }

TaskStatus PriKeyAction::Run() { return TaskStatus::FINISHED; }

int PriKeyAction::JudgeRange() { return _indexTree->CalcIndexRange(_key); }

} // namespace storage