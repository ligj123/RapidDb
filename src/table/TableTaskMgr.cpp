#include "TableTaskMgr.h"
#include "../core/BranchPage.h"
#include "../core/BranchRecord.h"
#include "../core/LeafPage.h"
#include "../utils/ThreadPool.h"

namespace storage {
TaskStatus IndexTask::Run() {
  _taskStatus = TaskStatus::RUNNING;

  IndexTree *idxTree = _taskMgr->_table->GetVectorIndex().at(_indexPos)._tree;
  IndexRange &range = idxTree->GetVctRange().at(_taskSn);

  if (idxTree->IsReranging()) [[unlikely]] {
    MDeque<IndexAction *> &queue =
        _taskMgr->_vctIndexTaskQueue[_indexPos]->_queueTempAction;
    unique_lock<SpinMutex> lock(idxTree->GetRangeMutex());

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

void TableTaskMgr::CollectTaskData(uint16_t idxPos) {
  assert(idxPos < _vctIndexTaskQueue.size());
  IndexTree *idxTree = _table->GetVectorIndex().at(idxPos)._tree;
  IndexTaskQueue *itq = _vctIndexTaskQueue[idxPos];

  if (!idxTree->GetRangeMutex().try_lock()) {
    return;
  }

  MDeque<IndexAction *> qs;
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
    int pos = (*iter)->JudgeRange();
    vctRange[pos]._queueTempAction.push_back(*iter);
  }

  idxTree->GetRangeMutex().unlock();
}

uint16_t TableTaskMgr::CalcAndSpliteTaskRanges(uint16_t indexPos,
                                               uint16_t exptTaskNum) {
  assert(exptTaskNum > 0 && exptTaskNum <= Configure::GetMaxIndexTaskNum());
  IndexTree *idxTree = _table->GetVectorIndex().at(indexPos)._tree;
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
        child = (BranchPage *)idxTree->GetPage(br.GetChildPageId(),
                                               PageType::BRANCH_PAGE, bpRoot);
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

  IndexTree *idxTree =
      _tableTaskMgr->_table->GetVectorIndex().at(_indexPos)._tree;
  MVector<IndexRange> &vctRange = idxTree->GetVctRange();
  for (IndexRange &range : vctRange) {
    assert(range._queueAction.size() == 0);
    assert(range._queueTempAction.size() == 0);

    range._startPage->SetRangeBeginPage(false);
    range._endPage->SetRangeEndPage(false);
    (*range._vctRangePage.begin())->SetRangeBeginPage(false);
    (*range._vctRangePage.rbegin())->SetRangeEndPage(false);
    range._brBorder = new BranchRecord();
    range._brBorder->Copy(
        (*range._vctRangePage.rbegin())->GetRecord(INT32_MAX, true));
  }

  vctTask.clear();
  vctRange.clear();
  _exptTaskNum =
      _tableTaskMgr->CalcAndSpliteTaskRanges(_indexPos, _exptTaskNum);
  _tableTaskMgr->CalcAndSpliteTaskRanges(_indexPos, _exptTaskNum);

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