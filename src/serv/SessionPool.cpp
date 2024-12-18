#include "SessionPool.h"

#include <bit>

namespace storage {
MVector<SessionGroup> SessionPool::_vctGroup;
MVector<ThreadTask *> SessionPool::_vctTask;
atomic_bool SessionPool::_bStop{false};
ThreadPool *SessionPool::_threadPool{nullptr};

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

  for (uint64_t i = 0; i < groupNum; i++) {
    _vctGroup.emplace_back(threadPool->GetMaxThreads(), outsiteThreadNum);
    SessionGroup &group = _vctGroup[i];
    group._currTranId = ((uint64_t)restartNum << 48) + (i << 40);

    if (i % num == 0) {
      task = new SessionTask(threadPool);
      _vctTask.push_back(task);
    }

    task->AddSessionGroup(&group);
  }

  threadPool->AddTasks(_vctTask);
  return true;
}

uint32_t SessionPool::CreateSession(function<void()> hookFunc) { return 0; }

void SessionPool::CloseSession(uint32_t sid) {}

} // namespace storage