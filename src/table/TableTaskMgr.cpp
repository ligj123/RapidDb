#include "TableTaskMgr.h"
#include "../core/BranchPage.h"
#include "../core/BranchRecord.h"
#include "../core/LeafPage.h"
#include "../utils/ThreadPool.h"

namespace storage {
DT_MicroSec TableTaskMgr::_dtLastWriteDisk{0};

TaskStatus IndexTask::Run() {
  SetStatus(TaskStatus::RUNNING, false);

  IndexTree *idxTree = _taskMgr->_table->GetVectorIndex().at(_indexPos)._tree;
  IndexRange &range = idxTree->GetVctRange().at(_taskPos);

  if (idxTree->IsReranging()) [[unlikely]] {
    SetStatus(TaskStatus::FINISHED, true);
    return TaskStatus::FINISHED;
  }

  if (range._queueActionFromCollect.RoughSize() == 0) {
    _taskMgr->CollectTaskData(_indexPos);
  }

  range._queueActionFromPrev.Pop(range._queueAction);
  range._queueActionFromCollect.Pop(range._queueAction);

  for (auto iter = range._queueAction.begin();
       iter != range._queueAction.end();) {
    TaskStatus status = (*iter)->Exec();
    if (status == TaskStatus::FINISHED) {
      iter = range._queueAction.erase(iter);
    } else {
      iter++;
    }
  }

  if (TableTaskMgr::_dtLastWriteDisk > range._dtLastWriteDisk) {
    IndexTree::SettleUpdatedPages(range._pageMap, idxTree->GetSplitPageLevel());
    range._dtLastWriteDisk = TableTaskMgr::_dtLastWriteDisk;
  }

  if (range._queueAction.size() == 0 && range._pageMap.size() == 0) {
    if (_taskMgr->GetMgrStatus() == MgrStatus::SET_STOP) {
      if (range._dtTaskStop == 0) {
        range._dtTaskStop = MicroSecTime();
      } else if (MicroSecTime() - range._dtTaskStop > 100000) {
        _taskMgr->CheckMgrStatus();
        SetStatus(TaskStatus::FINISHED, true);
        return TaskStatus::FINISHED;
      }
    }
  }

  return TaskStatus::RUNNING;
}

void TableTaskMgr::CollectTaskData(uint16_t idxPos) {
  assert(idxPos < _vctIndexTaskQueue.size());
  IndexTree *idxTree = _table->GetVectorIndex().at(idxPos)._tree;
  IndexTaskQueue *itq = _vctIndexTaskQueue[idxPos];

  if (!idxTree->GetRangeMutex().try_lock()) {
    return;
  }

  MList<IndexAction *> qs;
  itq->_queueSessionAction.Pop(qs);

  if (idxPos == 0) {
    // Primary Key
    for (size_t i = 1; i < _vctIndexTaskQueue.size(); i++) {
      SecondaryIndexTaskQueue *tqueue =
          (SecondaryIndexTaskQueue *)_vctIndexTaskQueue[i];
      tqueue->_toPrimaryQueue.Pop(qs);
    }
  } else {
    ((SecondaryIndexTaskQueue *)itq)->_fromPrimaryQueue.Pop(qs);
  }

  MVector<IndexRange> &vctRange = idxTree->GetVctRange();
  for (auto iter = qs.begin(); iter != qs.end(); iter++) {
    MVector<int> vctPos = (*iter)->JudgeRange();
    for (int pos : vctPos) {
      vctRange[pos]._queueActionFromCollect.Push(*iter, false);
    }
  }

  for (size_t i = 0; i <= vctRange.size(); i++) {
    vctRange[i]._queueActionFromCollect.Submit();
  }

  idxTree->GetRangeMutex().unlock();
}

TaskStatus IndexAdjustTask::Run() {
  MVector<IndexTask *> &vctTask = _tableTaskMgr->_vctIndexTasks[_indexPos];
  for (auto iter = vctTask.begin(); iter != vctTask.end(); iter++) {
    if (*iter != nullptr) {
      if ((*iter)->GetStatus(false) != TaskStatus::FINISHED) {
        return TaskStatus::RUNNING;
      } else {
        delete *iter;
        *iter = nullptr;
      }
    }
  }

  IndexTree *idxTree =
      _tableTaskMgr->_table->GetVectorIndex().at(_indexPos)._tree;
  unique_lock<SpinMutex> lock(idxTree->GetRangeMutex());
  MVector<IndexRange> &vctRange = idxTree->GetVctRange();

  MTreeMap<uint64_t, CachePage *> pageMap;
  MList<IndexAction *> queueAction;

  if (vctRange.size() > 1) {
    for (IndexRange &range : vctRange) {
      for (auto iter = range._pageMap.begin(); iter != range._pageMap.end();
           iter++) {
        pageMap.insert(*iter);
      }
      range._pageMap.clear();

      queueAction.insert(queueAction.end(), range._queueAction.begin(),
                         range._queueAction.end());
      range._queueActionFromPrev.Pop(queueAction);
      range._queueActionFromPrev.Pop(queueAction);

      range._startPage->SetRangeBeginPage(false);
      range._endPage->SetRangeEndPage(false);
      range._vctRangePage[0]->SetRangeBeginPage(false);
      range._vctRangePage[range._vctRangePage.size() - 1]->SetRangeEndPage(
          false);
    }
  } else {
    IndexRange &range = vctRange[0];
    for (auto iter = range._pageMap.begin(); iter != range._pageMap.end();
         iter++) {
      pageMap.insert(*iter);
    }
    range._pageMap.clear();
    queueAction.insert(queueAction.end(), range._queueAction.begin(),
                       range._queueAction.end());
    range._queueActionFromPrev.Pop(queueAction);
    range._queueActionFromPrev.Pop(queueAction);
  }

  vctTask.clear();
  vctRange.clear();

  IndexPage *rootPage = idxTree->GetRootPage();
  if (rootPage->GetPageLevel() < 2) {
    _exptTaskNum = 1;
  } else {
    if (rootPage->GetRecordNumber() < _exptTaskNum) {
      _exptTaskNum = rootPage->GetRecordNumber();
    }
  }

  if (_exptTaskNum > 1) {
    BranchPage *bpRoot = (BranchPage *)rootPage;
    int num = rootPage->GetRecordNumber();
    int a = num / _exptTaskNum;
    int b = num % _exptTaskNum;
    int pos = 0;
    vctRange.resize(_exptTaskNum);

    for (uint16_t i = 0; i < _exptTaskNum; i++) {
      IndexRange &range = vctRange[i];
      int end = pos + a + (b < i ? 1 : 0);
      for (; pos < end; pos++) {
        BranchRecord &br = bpRoot->GetRecord(pos, false);
        BranchPage *child = (BranchPage *)br.GetChildPage();
        if (child == nullptr) {
          child = (BranchPage *)idxTree->GetPage(br.GetChildPageId(),
                                                 PageType::BRANCH_PAGE, bpRoot);
          br.SetChildPage(child);
        }

        range._vctRangePage.push_back(child);
      }

      BranchPage *child = range._vctRangePage[0];
      child->SetRangeBeginPage(true);
      range._startPage = ((BranchPage *)child)->GetLeftLeafChild();
      range._startPage->SetRangeBeginPage(true);

      child = range._vctRangePage[range._vctRangePage.size()];
      child->SetRangeEndPage(true);
      range._endPage = ((BranchPage *)child)->GetRightLeafChild();
      range._endPage->SetRangeEndPage(true);

      range._borderRecord = &((BranchPage *)child)->GetRecord(INT32_MAX, true);
    }

    idxTree->SetSplitPageLevel(rootPage->GetPageLevel() - 1);

    for (auto iter = pageMap.begin(); iter != pageMap.end(); iter++) {
      assert(iter->second->GetPageType() == PageType::BRANCH_PAGE ||
             iter->second->GetPageType() == PageType::LEAF_PAGE);
      int rpos = idxTree->CalcIndexRange((IndexPage *)iter->second);
      vctRange[rpos]._pageMap.insert(*iter);
    }

    for (auto iter = queueAction.begin(); iter != queueAction.end(); iter++) {
      MVector<int> vctPos = (*iter)->JudgeRange();
      for (int rpos : vctPos) {
        vctRange[rpos]._queueAction.push_back(*iter);
      }
    }
  } else {
    idxTree->SetSplitPageLevel(UINT8_MAX);
    IndexRange &range = vctRange[0];
    range._pageMap.swap(pageMap);
    range._queueAction.swap(queueAction);
  }

  if (_indexPos == 0) {
    for (size_t i = 1; i < _tableTaskMgr->_vctIndexTaskQueue.size(); i++) {
      SecondaryIndexTaskQueue *itq =
          (SecondaryIndexTaskQueue *)&_tableTaskMgr->_vctIndexTaskQueue[i];
      itq->_fromPrimaryQueue.ResetLiveThreadNumber(_exptTaskNum);
    }
  } else {
    SecondaryIndexTaskQueue *itq = (SecondaryIndexTaskQueue *)&_tableTaskMgr
                                       ->_vctIndexTaskQueue[_indexPos];
    itq->_toPrimaryQueue.ResetLiveThreadNumber(_exptTaskNum);
  }

  vctTask.reserve(_exptTaskNum);
  for (int64_t i = 0; i < _exptTaskNum; i++) {
    vctTask.push_back(
        new IndexTask(_threadPool, _tableTaskMgr, _exptTaskNum, i));
  }

  idxTree->SetReRanging(false);

  MVector<ThreadTask *> vct;
  vct.reserve(vctTask.size());
  for (auto t : vctTask) {
    vct.push_back(t);
  }

  _threadPool->AddTasks(vct);
  return TaskStatus::FINISHED;
}

} // namespace storage