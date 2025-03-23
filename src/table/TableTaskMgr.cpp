#include "TableTaskMgr.h"
#include "../core/BranchPage.h"
#include "../core/BranchRecord.h"
#include "../core/LeafPage.h"
#include "../pool/FilePagePool.h"
#include "../serv/SessionPool.h"
#include "../utils/ThreadPool.h"

namespace storage {
DT_MicroSec TableTaskMgr::_dtLastWriteDisk{0};

IndexTask::IndexTask(ThreadPool *pool, TableTaskMgr *taskMgr, uint16_t indexPos,
                     uint16_t taskPos)
    : ThreadTask(pool), _taskMgr(taskMgr), _indexPos(indexPos),
      _taskPos(taskPos) {
  _taskName = "IndexTask_" + _taskMgr->_table->GetFullName() + "_" +
              ToMString(indexPos) + "_" + ToMString(taskPos);
  _taskMask = _taskMgr->_table->TableID();
}

TaskStatus IndexTask::Run() {
  IndexTree *idxTree = _taskMgr->_table->GetVectorIndex().at(_indexPos)._tree;

  if (_taskMgr->GetMgrStatus() == MgrStatus::ADJUSTING) [[unlikely]] {
    if (idxTree->IsReranging()) {
      SetStatus(TaskStatus::FINISHED, true);
      return TaskStatus::FINISHED;
    } else {
      return TaskStatus::INTERVAL;
    }
  }

  SetStatus(TaskStatus::RUNNING, false);

  IndexRange &range = idxTree->GetVctRange().at(_taskPos);
  MList<IndexAction *> &lstAction = range._actionQueue->_lstAction;
  uint64_t oldNum = lstAction.size();
  _taskMgr->CollectTaskData(_indexPos, _taskPos);
  _actionCount += lstAction.size() - oldNum;

  for (auto iter = lstAction.begin(); iter != lstAction.end();) {
    TaskStatus status = (*iter)->Exec();
    if (status == TaskStatus::FINISHED) {
      delete (*iter);
      iter = lstAction.erase(iter);
    } else {
      iter++;
    }
  }

  for (auto iter = range._lstErrRecord.begin();
       iter != range._lstErrRecord.end();) {
    if ((*iter)->GetLock()->GetRecordStatus() >= RecordStatus::COMMITED) {
      (*iter)->ReleaseLock(idxTree);
      delete (*iter);
      iter = range._lstErrRecord.erase(iter);
    } else {
      iter++;
    }
  }

  if (TableTaskMgr::_dtLastWriteDisk > range._dtLastWriteDisk) {
    idxTree->SettleUpdatedPages(range._pageMap);
    range._dtLastWriteDisk = TableTaskMgr::_dtLastWriteDisk + 1;
  } else if (_taskMgr->GetMgrStatus() == MgrStatus::SET_STOP) {
    if (lstAction.size() == 0 && (_taskPos == 0 && range._pageMap.size() <= 1 ||
                                  _taskPos > 0 && range._pageMap.size() == 0)) {
      if (range._dtTaskStop == 0) {
        range._dtTaskStop = MicroSecTime();
      } else if (MicroSecTime() - range._dtTaskStop > 100000 &&
                 range._lstErrRecord.size() == 0) {
        _taskMgr->CheckMgrStatus();
        SetStatus(TaskStatus::FINISHED, true);
        return TaskStatus::FINISHED;
      }
    } else if (MicroSecTime() - range._dtLastWriteDisk > 100000) {
      idxTree->SettleUpdatedPages(range._pageMap);
      range._dtLastWriteDisk = MicroSecTime();
    }
  }

  SetStatus(TaskStatus::INTERVAL, true);
  return TaskStatus::INTERVAL;
}

void TableTaskMgr::CollectTaskData(uint16_t idxPos, int iRange) {
  assert(idxPos < _table->GetVectorIndex().size());
  IndexTree *idxTree = _table->GetVectorIndex().at(idxPos)._tree;

  IndexRange &idxRange = idxTree->GetVctRange()[iRange];
  IndexActionQueue *iaQueue = idxRange._actionQueue;

  iaQueue->_queueSessionAction.Pop(iaQueue->_lstAction);
  if (iaQueue->_lstRangeAction.size() > 0) {
    unique_lock<SpinMutex> lock(idxTree->GetRangeMutex());
    iaQueue->_lstAction.insert(iaQueue->_lstAction.end(),
                               iaQueue->_lstRangeAction.begin(),
                               iaQueue->_lstRangeAction.end());
    iaQueue->_lstRangeAction.clear();
  }

  if (idxPos == 0) {
    MVector<IndexProp> &vctProp = _table->GetVectorIndex();
    for (size_t i = 1; i < vctProp.size(); i++) {
      MVector<IndexRange> &vctRange = vctProp[i]._tree->GetVctRange();
      for (IndexRange &range : vctRange) {
        SecIndexActionQueue *secQueue =
            dynamic_cast<SecIndexActionQueue *>(range._actionQueue);
        secQueue->_toPrimaryQueue[iRange].Pop(iaQueue->_lstAction);
      }
    }
  } else {
    SecIndexActionQueue *secQueue =
        dynamic_cast<SecIndexActionQueue *>(iaQueue);
    secQueue->_fromPrimaryQueue.Pop(iaQueue->_lstAction);
  }
}

TaskStatus IndexAdjustTask::Run() {
  IndexTree *idxTree =
      _tableTaskMgr->_table->GetVectorIndex().at(_indexPos)._tree;

  if (!idxTree->IsReranging()) {
    MgrStatus s = _tableTaskMgr->_mgrStatus.exchange(MgrStatus::ADJUSTING,
                                                     memory_order_acquire);
    if (s == MgrStatus::ADJUSTING) {
      SetStatus(TaskStatus::INTERVAL, false);
      return TaskStatus::INTERVAL;
    } else if (s != MgrStatus::RUNNING) {
      SetStatus(TaskStatus::FINISHED, false);
      return TaskStatus::FINISHED;
    }

    idxTree->SetReRanging(true);
    SetStatus(TaskStatus::INTERVAL, false);
    return TaskStatus::INTERVAL;
  }

  for (size_t i = 0; i < _tableTaskMgr->_vctIndexTasks.size(); i++) {
    MVector<IndexTask *> &vctTask = _tableTaskMgr->_vctIndexTasks[i];
    if (i == _indexPos) {
      for (auto iter = vctTask.begin(); iter != vctTask.end(); iter++) {
        if (*iter == nullptr)
          continue;

        if ((*iter)->GetStatus(true) == TaskStatus::FINISHED) {
          delete *iter;
          *iter = nullptr;
        } else {
          SetStatus(TaskStatus::INTERVAL, false);
          return TaskStatus::INTERVAL;
        }
      }
    } else {
      for (auto iter = vctTask.begin(); iter != vctTask.end(); iter++) {
        if ((*iter)->GetStatus(true) == TaskStatus::RUNNING) {
          SetStatus(TaskStatus::INTERVAL, false);
          return TaskStatus::INTERVAL;
        }
      }
    }
  }

  MVector<IndexRange> &vctRange = idxTree->GetVctRange();
  MTreeMap<uint64_t, CachePage *> pageMap;
  MList<IndexAction *> lstAction;
  MVector<IndexProp> &vctProp = _tableTaskMgr->_table->GetVectorIndex();

  for (size_t i = 0; i < vctRange.size(); i++) {
    IndexRange &range = vctRange[i];
    pageMap.insert(range._pageMap.begin(), range._pageMap.end());
    range._pageMap.clear();

    IndexActionQueue *iaQueue = range._actionQueue;
    lstAction.insert(lstAction.end(), iaQueue->_lstAction.begin(),
                     iaQueue->_lstAction.end());
    iaQueue->_lstAction.clear();
    iaQueue->_queueSessionAction.Pop(lstAction);

    if (iaQueue->_lstRangeAction.size() > 0) {
      lstAction.insert(lstAction.end(), iaQueue->_lstRangeAction.begin(),
                       iaQueue->_lstRangeAction.end());
      iaQueue->_lstRangeAction.clear();
    }

    if (vctRange.size() > 1) {
      range._startPage->SetRangeBeginPage(false);
      range._endPage->SetRangeEndPage(false);
    }
  }

  if (_indexPos == 0) {
    for (size_t j = 1; j < vctProp.size(); j++) {
      MVector<IndexRange> &vctRange = vctProp[j]._tree->GetVctRange();
      for (IndexRange &range : vctRange) {
        SecIndexActionQueue *secQueue =
            dynamic_cast<SecIndexActionQueue *>(range._actionQueue);
        for (LineQueue<IndexAction> &line : secQueue->_toPrimaryQueue) {
          line.Pop(lstAction);
        }

        secQueue->_fromPrimaryQueue.Pop(range._actionQueue->_lstAction);
      }
    }
  } else {
    MVector<IndexRange> &vctPriRange = vctProp[0]._tree->GetVctRange();
    for (size_t i = 0; i < vctRange.size(); i++) {
      IndexRange &range = vctRange[i];
      IndexActionQueue *iaQueue = range._actionQueue;
      SecIndexActionQueue *secQueue =
          dynamic_cast<SecIndexActionQueue *>(iaQueue);
      secQueue->_fromPrimaryQueue.Pop(lstAction);
      assert(vctPriRange.size() == secQueue->_toPrimaryQueue.size());

      for (size_t j = 0; j < vctPriRange.size(); j++) {
        secQueue->_toPrimaryQueue[j].Pop(
            vctPriRange[j]._actionQueue->_lstAction);
      }
    }
  }

  MVector<IndexTask *> &vctTask = _tableTaskMgr->_vctIndexTasks[_indexPos];
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

  int priRangeNum = (_indexPos == 0 ? _exptTaskNum
                                    : _tableTaskMgr->_table->GetVectorIndex()[0]
                                          ._tree->GetVctRange()
                                          .size());
  if (_exptTaskNum > 1) {
    BranchPage *bpRoot = (BranchPage *)rootPage;
    int num = rootPage->GetRecordNumber();
    int a = num / _exptTaskNum;
    int b = num % _exptTaskNum;
    int pos = 0;
    vctRange.resize(_exptTaskNum);

    for (uint16_t i = 0; i < _exptTaskNum; i++) {
      IndexRange &range = vctRange[i];
      int n = a + (b >= _exptTaskNum - i ? 1 : 0);
      int end = pos + n;
      range._vctRangePage.reserve(n);

      for (; pos < end; pos++) {
        BranchRecord &br = bpRoot->GetRecord(pos, false);
        BranchPage *child = (BranchPage *)br.GetChildPage();
        if (child == nullptr) {
          child = (BranchPage *)idxTree->GetPage(
              br.GetChildPageId(), PageType::BRANCH_PAGE, bpRoot, true);
          br.SetChildPage(child);
        }

        range._vctRangePage.push_back(child);
      }

      BranchPage *child = range._vctRangePage[0];
      range._startPage = ((BranchPage *)child)->GetLeftLeafChild();
      range._startPage->SetRangeBeginPage(true);

      child = range._vctRangePage[range._vctRangePage.size() - 1];
      range._endPage = ((BranchPage *)child)->GetRightLeafChild();
      range._endPage->SetRangeEndPage(true);

      range._borderRecord = new BranchRecord();
      range._borderRecord->Copy(
          ((BranchPage *)child)->GetRecord(INT32_MAX, true));
      range._dtLastWriteDisk = TableTaskMgr::_dtLastWriteDisk;

      if (_indexPos == 0) {
        range._actionQueue =
            new IndexActionQueue(SessionPool::GetVctSessionGroup().size());
      } else {
        range._actionQueue = new SecIndexActionQueue(
            SessionPool::GetVctSessionGroup().size(), priRangeNum);
      }
    }

    assert(pos == num);
    idxTree->SetSplitPageLevel(rootPage->GetPageLevel() - 1);

    for (auto iter = pageMap.begin(); iter != pageMap.end(); iter++) {
      if (iter->second->GetPageType() == PageType::OVERFLOW_PAGE) {
        FilePagePool::AddWritePage(ThreadPool::GetThreadId(), iter->second,
                                   false);
      } else if (iter->second->GetPageType() == PageType::HEAD_PAGE) {
        vctRange[0]._pageMap.insert(*iter);
      } else {
        assert(iter->second->GetPageType() == PageType::BRANCH_PAGE ||
               iter->second->GetPageType() == PageType::LEAF_PAGE);
        int rpos = idxTree->CalcIndexRange((IndexPage *)iter->second);
        vctRange[rpos]._pageMap.insert(*iter);
      }
    }

    FilePagePool::SubmitWritePage(ThreadPool::GetThreadId());

    for (auto iter = lstAction.begin(); iter != lstAction.end(); iter++) {
      int pos = (*iter)->JudgeRange();
      vctRange[pos]._actionQueue->_lstAction.push_back(*iter);
    }
  } else {
    vctRange.resize(1);
    IndexRange &range = vctRange[0];
    if (_indexPos == 0) {
      range._actionQueue =
          new IndexActionQueue(SessionPool::GetVctSessionGroup().size());
    } else {
      range._actionQueue = new SecIndexActionQueue(
          SessionPool::GetVctSessionGroup().size(), priRangeNum);
    }

    idxTree->SetSplitPageLevel(UINT8_MAX);
    range._pageMap.swap(pageMap);
    MList<storage::IndexAction *> &lst = range._actionQueue->_lstAction;
    for (auto iter = lstAction.begin(); iter != lstAction.end(); iter++) {
      (*iter)->SetRangePos(0);
      lst.push_back(*iter);
    }

    range._dtLastWriteDisk = TableTaskMgr::_dtLastWriteDisk;
  }

  if (_indexPos == 0) {
    for (size_t i = 1; i < vctProp.size(); i++) {
      MVector<IndexRange> &vctSecRange = vctProp[i]._tree->GetVctRange();
      for (IndexRange &range : vctSecRange) {
        SecIndexActionQueue *secQueue =
            dynamic_cast<SecIndexActionQueue *>(range._actionQueue);
        assert(secQueue->IsQueueEmpty());

        secQueue->_fromPrimaryQueue.Resize(_exptTaskNum);
        secQueue->_toPrimaryQueue.resize(_exptTaskNum);
      }
    }
  }

  vctTask.reserve(_exptTaskNum);
  MVector<ThreadTask *> vct;
  vct.reserve(_exptTaskNum);

  for (int64_t i = 0; i < _exptTaskNum; i++) {
    IndexTask *task = new IndexTask(_threadPool, _tableTaskMgr, _indexPos, i);
    task->SetExclusiveTask(_bExclusive);
    vctTask.push_back(task);
    vct.push_back(task);
  }

  idxTree->SetReRanging(false);
  _threadPool->AddTasks(ThreadPool::GetThreadId(), vct);

  {
    unique_lock<SpinMutex> lock(idxTree->GetRangeMutex());
    for (IndexAction *action : _tableTaskMgr->_lstTmpAction) {
      int pos = action->JudgeRange();
      idxTree->AddActionFromLocal(pos, action);
    }

    _tableTaskMgr->_lstTmpAction.clear();
  }

  MgrStatus s = _tableTaskMgr->_mgrStatus.exchange(MgrStatus::RUNNING,
                                                   memory_order_acquire);
  assert(s == MgrStatus::ADJUSTING);
  SetStatus(TaskStatus::FINISHED, true);
  return TaskStatus::FINISHED;
}

} // namespace storage