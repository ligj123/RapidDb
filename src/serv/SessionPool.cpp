#include "SessionPool.h"

#include <bit>

namespace storage {
MVector<SessionGroup> SessionPool::_vctGroup;
MVector<SessionTask *> SessionPool::_vctTask;
atomic_bool SessionPool::_bStop{false};
ThreadPool *SessionPool::_threadPool{nullptr};
atomic_uint32_t SessionPool::_currSessionId{0};

TaskStatus SessionTask::Run() { return TaskStatus::FINISHED; }

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
  MVector<ThreadTask> vct;

  for (uint64_t i = 0; i < groupNum; i++) {
    _vctGroup.emplace_back(threadPool->GetMaxThreads(), outsiteThreadNum);
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

bool SessionPool::AdjustTaskNumber(uint16_t newTaskNum) {
  for (SessionTask *task : _vctTask) {
    task->SetStop();
  }

  _vctTask.clear();

  _vctTask.reserve(newTaskNum);
  SessionTask *task = nullptr;
  uint16_t num = _vctGroup.size() / newTaskNum;
  MVector<ThreadTask> vct;

  for (uint64_t i = 0; i < groupNum; i++) {
    SessionGroup &group = _vctGroup[i];

    if (i % num == 0) {
      task = new SessionTask(threadPool);
      _vctTask.push_back(task);
      vct.push_back(task);
    }

    task->AddSessionGroup(&group);
  }

  threadPool->AddTasks(vct);
}

uint32_t SessionPool::CreateSession(StmtResult *result) { return 0; }

void SessionPool::CloseSession(uint32_t sid, StmtResult *result) {}

} // namespace storage