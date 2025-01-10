#include "SessionPool.h"

#include <bit>

namespace storage {
MVector<SessionGroup> SessionPool::_vctGroup;
MVectorPtr<SessionTask *> SessionPool::_vctTask;
atomic_bool SessionPool::_bStop{false};
ThreadPool *SessionPool::_threadPool{nullptr};
atomic_uint32_t SessionPool::_currSessionId{0};

TaskStatus SessionTask::Run() {
  for (SessionGroup *group : _vctGroup) {
    group->_runTimes++;
    MList<SessionAction *> &lst = group->_lstAction;
    group->_threaPoolQueue.Pop(lst);
    group->_outerQueue.Pop(lst);

    for (auto iter = lst.begin(); iter != lst.end(); iter++) {
      TaskStatus s = (*iter)->Exec(*group);
      if (s == TaskStatus::FINISHED) {
        delete (*iter);
        iter = lst.erase(iter);
      } else {
        iter++;
      }
    }

    for (auto iter = group->_mapSession.begin();
         iter != group->_mapSession.end(); iter++) {
      iter->second->Exec();
    }

    for (auto iter = group->_obsoleteSession.begin();
         iter != group->_obsoleteSession.end();) {
      Session *sess = *iter;
      assert(sess->_bObsolete);
      sess->Exec();
      if (sess->_lstWaittingStmt.size() == 0 &&
          sess->_currStatement == nullptr) {
        delete sess;
        iter = group->_obsoleteSession.erase(iter);
      } else {
        iter++;
      }
    }
  }

  if (_bStop) {
    if (SessionPool::IsPoolStop()) {
      bool empty = true;
      for (SessionGroup *group : _vctGroup) {
        MList<SessionAction *> &lst = group->_lstAction;
        group->_threaPoolQueue.Pop(lst);
        group->_outerQueue.Pop(lst);
        if (lst.size() > 0) {
          return TaskStatus::RUNNING;
        }

        for (auto iter = group->_mapSession.begin();
             iter != group->_mapSession.end(); iter++) {
          if (!iter->second->IsEmpty()) {
            return TaskStatus::RUNNING;
          }
        }

        if (group->_obsoleteSession.size() > 0) {
          return TaskStatus::RUNNING;
        }
      }

      _tryStopTime--;
      if (_tryStopTime > 0) {
        return TaskStatus::RUNNING;
      } else {
        return TaskStatus::FINISHED;
      }
    } else {
      return TaskStatus::FINISHED;
    }
  } else {
    return TaskStatus::RUNNING;
  }
}

TaskStatus SessionAdjustTask::Run() {
  bool empty = true;
  if (_vctOldTask.size() == 0) {
    MVectorPtr<SessionTask *> &vctTask = SessionPool::GetVctSessionTask();
    _vctOldTask.swap(vctTask);
    vctTask.resize(_newTaskNum);
    for (SessionTask *task : _vctOldTask) {
      task->SetStop();
    }
  }

  MVectorPtr<SessionTask *> &vctNewTask = SessionPool::GetVctSessionTask();
  for (auto iter = _vctOldTask.begin(); iter != _vctOldTask.end(); iter++) {
    SessionTask *task = *iter;
    if (task == nullptr) {
      continue;
    } else if (task->GetStatus(false) != TaskStatus::FINISHED) {
      empty = false;
      continue;
    }

    MVector<SessionGroup *> &vctGroup = (*iter)->GetVctSessionGroup();
    for (SessionGroup *group : vctGroup) {
      vctNewTask[group->_groupSn / _newTaskNum]->AddSessionGroup(group);
    }
  }

  return (empty ? TaskStatus::FINISHED : TaskStatus::RUNNING);
}

bool SessionPool::InitPool(uint16_t groupNum, uint16_t taskNum,
                           uint16_t restartNum, uint16_t outsiteThreadNum,
                           ThreadPool *threadPool) {
  // Make sure it is this method is only called one time
  assert(_vctGroup.size() == 0);
  assert(popcount(groupNum) == 1 && popcount(taskNum) == 1);
  assert(taskNum <= groupNum);
  _vctGroup.reserve(groupNum);
  restartNum %= 16;
  _vctTask.reserve(taskNum);
  SessionTask *task = nullptr;
  uint16_t num = groupNum / taskNum;
  MVector<ThreadTask *> vct;

  for (uint64_t i = 0; i < groupNum; i++) {
    _vctGroup.emplace_back(i, 0, 0, threadPool->GetMaxThreads(),
                           outsiteThreadNum);
    SessionGroup &group = _vctGroup[i];
    group._currTranId = ((uint64_t)restartNum << 48) + (i << 40);

    if (i % num == 0) {
      task = new SessionTask(threadPool);
      _vctTask.push_back(task);
      vct.push_back(task);
    }

    task->AddSessionGroup(&group);
  }

  threadPool->AddTasks(vct);
  return true;
}

uint32_t SessionPool::CreateSession(uint16_t outerTid, StmtResult *result) {
  uint32_t sid = _currSessionId.fetch_add(1, memory_order_relaxed);
  SessionCreateAction *action = new SessionCreateAction(sid, result);
  _vctGroup[sid % _vctGroup.size()]._outerQueue.Push(outerTid, action);
  return sid;
}

void SessionPool::CloseSession(uint16_t outerTid, uint32_t sid,
                               StmtResult *result) {
  SessionCloseAction *action = new SessionCloseAction(sid, result);
  _vctGroup[sid % _vctGroup.size()]._outerQueue.Push(outerTid, action);
}

void SessionPool::AddStatement(uint16_t outerTid, uint32_t sid, uint32_t stmtId,
                               uint32_t exprId, MString &&sql,
                               VectorRow &&paras, StmtResult *result) {
  SessionStatementAction *action = new SessionStatementAction(
      sid, stmtId, exprId, move(sql), move(paras), result);
  _vctGroup[sid % _vctGroup.size()]._outerQueue.Push(outerTid, action);
}
} // namespace storage