#include "SessionPool.h"
#include "utils/"
namespace storage {
atomic<uint64_t> SessionPool::_sessionId{0};
atomic<uint64_t> SessionPool::_tranId{0};
uint16_t SessionPool::_threadNum{0};
vector<SessionGroup> SessionPool::_vctGroup;
uint64_t SessionPool::_sessionRangeId{0xff};
uint64_t SessionPool::_tranRangeId{0xff};

void CreateSession::Exec() {
  SessionGroup &sg = GetSessionGroup(_sid);
  _session = new Session(_sid);
  sg._vctTask.push_back(_session);
}

void CloseSession::Exec() {
  SessionGroup &sg = GetSessionGroup(_sid);
  auto iter = sg._mapSession.find(_sid);
  assert(iter != sg._mapSession.end());
  Session *session = iter->second;
  session->_lastVisitTime = utils::MicroSecTime();
  sg._discardSession.push_back(session);
}

bool SessionPool::InitPool(uint16_t threadNum) {
  // Make sure it is this method is only called one time
  assert(_threadNum == 0);
  assert(threadNum > 0);
  _threadNum = threadNum;
  _vctGroup.resize(_threadNum);

  for (uint16_t i = 0; i < _threadNum; i++) {
    _vctGroup[i]._thread = new thread([i]() { Run(i); });
  }
}

void SessionPool::Run(uint16_t thdId) {
  SessionGroup &sg = _vctGroup[thdId];
  sg._currTranId = _tranId.fetch_add(_tranRangeId, memory_order_relaxed);

  while (true) {
  }
}
} // namespace storage