#include "IndexAction.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include "LeafRecord.h"

namespace storage {
TaskStatus PrevPageAction::Exec() {
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
      _idxPage->SetPageStatus(PageStatus::VALID, true);
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

TaskStatus InsertAction::Exec() {
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
  bool bFind;
  int pos = lp->SearchRecord(*_lr, bFind);
  if (bFind) {
    LeafRecord &old = lp->GetRecord(pos);
    if (old.GetLock() != nullptr && old.ReleaseLockAble()) {
      int32_t commLen1, commLen2, tempLen1, tempLen2;
      old.GetLength(tempLen1, commLen1);
      old.ReleaseLock(_indexTree, _indexTree->IsMultiRange());
      old.GetLength(tempLen2, commLen2);

      lp->_tempDataLength += tempLen2 - tempLen1;
      lp->_committedDataLength += commLen2 - commLen1;
    }

    if (old.IsDelete()) {
      lp->_vctRecord.erase(lp->_vctRecord.begin() + pos);
      bFind = false;
    }
  }

  if (_lr->GetAction() == ActionType::INSERT) {
    if (bFind) {
      lock->_errMsg =
          new ErrorMsg(STMT_DUPLICATE_ENTRY,
                       {_lr->GetKeyString(), _indexTree->GetTableName(),
                        _indexTree->GetIndexName()});
      lock->_recResult.store(RecordResult::ERROR, memory_order_release);
      return TaskStatus::RUNNING;
    }

    lp->InsertRecord(_lr, pos);
    _lr = nullptr;
    return TaskStatus::FINISHED;
  } else {
    assert(_lr->GetAction() == ActionType::DELETE);
    assert(bFind);
    LeafRecord &old = lp->GetRecord(pos);

    if (old.IsConflict(lock->TxID(), lock->_actType)) {
      lock->_errMsg = new ErrorMsg(STMT_LOCK_CONFLICT, {});
      lock->_recResult.store(RecordResult::ERROR, memory_order_release);
      return TaskStatus::RUNNING;
    }

    lp->DeleteRecord(_lr, pos);
    return TaskStatus::FINISHED;
  }
}

int InsertAction::JudgeRange() { return _indexTree->CalcIndexRange(*_lr); }

TaskStatus PriKeyAction::Exec() { return TaskStatus::FINISHED; }

int PriKeyAction::JudgeRange() { return _indexTree->CalcIndexRange(_key); }

} // namespace storage