#include "TableTaskMgr.h"
#include "../core/BranchPage.h"
#include "../core/BranchRecord.h"
#include "../core/LeafPage.h"

namespace storage {
TaskStatus IndexTask::Run() {
  _taskStatus = TaskStatus::RUNNING;

  IndexTree *idxTree = _taskMgr->_table->GetVectorIndex().at(idxPos)._tree;
  if (idxTree->IsReranging()) [[unlikely]] {
    MDeque<IndexAction *> &queue =
        _taskMgr->_vctSecIndexTaskQueue[_indexPos]._queueTempAction;
    unique_lock<SpinMutex> lock(idxTree->GetRangeMutex());
    IndexRange &range = idxTree->GetVctRange().at(_taskSn);
    if (range._queueTempAction.size() > 0) {
      range._queueAction.insert(range._queueAction.end(),
                                range._queueTempAction.begin(),
                                range._queueTempAction.end());
    }

    for (auto iter = range._queueAction.begin();
         iter != range._queueAction.end();) {
      if ((*iter)->RearrangeAble()) {
        queue.push_back(*iter);
        iter = range._queueAction.erase(iter);
      } else {
        iter++;
      }
    }

    if (range._queueAction.size() == 0) {
      return TaskStatus::FINISHED;
    }
  } else {
    unique_lock<SpinMutex> lock(idxTree->GetRangeMutex());

    if (range._queueTempAction.size() == 0) {
      _taskMgr->CollectTaskData(_taskSn);
    }

    range._queueAction.insert(range._queueAction.end(),
                              range._queueTempAction.begin(),
                              range._queueTempAction.end());
  }

  IndexRange &range = idxTree->GetVctRange().at(_taskSn);
  for (auto iter = range._queueAction.begin();
       iter != range._queueAction.end();) {
    TaskStatus status = (*iter)->Exec();
    if (status == TaskStatus::FINISHED) {
      iter = range._queueAction.erase(iter);
    } else {
      iter++;
    }
  }
  return TaskStatus::RUNNING;
}

void TableTaskMgr::ResetIndexTaskNum(uint16_t idxPos, uint16_t secTaskNum) {
  // assert(idxPos <= _vctIndexTaskQueue.size());
  // uint16_t actualNum = CalcTaskRanges(idxPos, secTaskNum);

  // SecondaryIndexTaskQueue &itq = _vctSecIndexTaskQueue[idxPos - 1];
  // itq._toPrimaryQueue.ResetLiveThreadNumber(secTaskNum);
  // for (SecondaryIndexTaskQueue &itq : _vctSecIndexTaskQueue) {
  //     itq._fromPrimaryQueue.ResetLiveThreadNumber(actualNum);
  //   }
}

void TableTaskMgr::CollectTaskData(uint16_t idxPos) {
  assert(idxPos < _vctSecIndexTaskQueue.size());
  IndexTree *idxTree = _table->GetVectorIndex().at(idxPos)._tree;
  IndexTaskQueue *itq = _vctIndexTaskQueue[idxPos];

  if (!idxTree->GetRangeMutex().try_lock()) {
    return;
  }

  MDeque<IndexAction *> qs;
  itq->_queueSessionAction.Pop(qs);

  if (idxPos == 0) {
    // Primary Key
    for (size_t i = 1; i < _vctSecIndexTaskQueue.size(); i++) {
      SecondaryIndexTaskQueue *tqueue =
          (SecondaryIndexTaskQueue *)_vctSecIndexTaskQueue[i];
      tqueue->_toPrimaryQueue.Pop(qs);
    }
  } else {
    ((SecondaryIndexTaskQueue *)itq)->_fromPrimaryQueue.Pop(qs);
  }

  MVector<IndexRange> &vctRange = idxTree->GetVctRange();
  for (auto iter = qs.begin(); iter != qs.end(); iter++) {
    int pos = (*iter)->JudgeRange();
    vctRange[pos]._queueTempAction.push_back(*iter);
  }

  idxTree->GetRangeMutex().unlock();
}

uint16_t TableTaskMgr::CalcAndSpliteTaskRanges(uint16_t indexPos,
                                               uint16_t exptTaskNum) {
  assert(exptTaskNum > 0 && exptTaskNum <= Configure::GetMaxIndexTaskNum());
  IndexTree *idxTree = _tableTaskMgr->_table->GetVectorIndex().at(_indexPos);
  MVector<IndexRange> &vctRange = idxTree->GetVctRange();
  assert(idxTree->GetVctRange().size() == 0);
  if (exptTaskNum == 1) {
    return 1;
  }

  IndexPage *root = idxTree->GetRootPage();
  assert(root->GetPageLevel() > 1);
  int num = root->GetRecordNumber();
  BranchPage *bpRoot = (BranchPage *)root;

  if (num < exptTaskNum) {
    exptTaskNum = num;
  }

  int a = num / exptTaskNum;
  int b = num % exptTaskNum;
  int pos = 0;
  vctRange.resize(exptTaskNum);

  for (uint16_t i = 0; i < exptTaskNum; i++) {
    IndexRange &range = vctRange[i];
    int end = pos + a + (b < i ? 1 : 0);
    for (; pos < end; pos++) {
      BranchRecord &br = bpRoot->GetRecord(pos, false);
      BranchPage *child = (BranchPage *)br.GetChildPage();
      if (child == nullptr) {
        child = idxTree->GetPage(br.GetChildPageId(), PageType::BRANCH_PAGE,
                                 bpRoot);
        br.SetChildPage(child);
      }

      range._vctRangePage.push_back(child);
    }

    range._vctRangePage[0]->SetRangeBeginPage(true);
    (*range._vctRangePage.rbegin())->SetRangeEndPage(true);

    range._startPage = (LeafPage *)range._vctRangePage[0]->RecursiveLeftChild();
    range._endPage =
        (LeafPage *)(*range._vctRangePage.rbegin())->RecursiveRightChild();
  }

  return exptTaskNum;
}

TaskStatus IndexAdjustTask::Run() {
  MVector<IndexTask *> &vctTask = _tableTaskMgr->_vctIndexTasks[_indexPos];
  for (auto iter = vctTask.begin(); iter != vctTask.end(); iter++) {
    if (*iter != nullptr) {
      if ((*iter)->GetStatus() != TaskStatus::FINISHED) {
        return TaskStatus::RUNNING;
      } else {
        delete *iter;
        *iter = nullptr;
      }
    }
  }

  IndexTree *idxTree = _tableTaskMgr->_table->GetVectorIndex().at(_indexPos);
  MVector<IndexRange> &vctRange = idxTree->GetVctRange();
  for (IndexRange &range : vctRange) {
    assert(range._queueAction.size() == 0);
    assert(range._queueTempAction.size() == 0);

    range._startPage->SetRangeBeginPage(false);
    range._endPage->SetRangeEndPage(false);
    (*range._vctRangePage.begin())->SetRangeBeginPage(false);
    (*range._vctRangePage.rbegin())->SetRangeEndPage(false);
  }

  vctTask.clear();
  vctRange.clear;

  idxTree->SetReRanging(false);
  return TaskStatus::FINISHED;
}
} // namespace storage