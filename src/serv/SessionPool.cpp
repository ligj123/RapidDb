#include "SessionPool.h"
#include "bit"

namespace storage {
bool SessionPool::_bStopped{false};
uint16_t SessionPool::_threadNum{0};
vector<SessionGroup> SessionPool::_vctGroup;
atomic<uint64_t> SessionPool::_sessionId{0};
atomic<uint64_t> SessionPool::_tranId;
uint64_t SessionPool::_tranInitId;
SpinMutex SessionPool::_spinMutex;

TaskStatus SessionTask::Run() { return TaskStatus::FINISHED; }

bool SessionPool::InitPool(uint16_t groupNum, uint16_t taskNum,
                           uint16_t startNum) {
  // Make sure it is this method is only called one time
  assert(_vctGroup.size() == 0);
  assert(popcount(groupNum) == 1 && popcount(taskNum) == 1);
  assert(taskNum <= groupNum);
  _vctGroup.resize(groupNum);
  startNum %= 16;
  _vctTask.reserve(taskNum);
  SessionTask *task = nullptr;
  uint16_t num = groupNum / taskNum;

  for (uint16_t i = 0; i < groupNum; i++) {
    SessionGroup &group = _vctGroup[i];
    group._currTranId = (startNum << 48) + (i << 40);

    if (i % num == 0) {
      task = new SessionTask;
      _vctTask.push_back(task);
    }

    task->AddSessionGroup(&group);
  }

  return true;
}

uint32_t SessionPool::CreateSession(function<void()> hookFunc) {}

void SessionPool::CloseSession(uint32_t sid) {}

} // namespace storage