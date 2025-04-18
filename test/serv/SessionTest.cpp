#include "../../src/serv/Session.h"
#include "../../src/serv/SessionPool.h"
#include "../../src/serv/Transaction.h"
#include "../../src/statement/StmtResult.h"

#include <boost/test/unit_test.hpp>

namespace storage {

BOOST_AUTO_TEST_SUITE(SessionTest)
BOOST_AUTO_TEST_CASE(SessionBasic_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  uint16_t tidOld = ThreadPool::SetThreadId(0);

  ThreadPool *tpool = new ThreadPool("sess", 1, 8);
  tpool->SetStop();
  tpool->WaitStoped();

  SessionPool::InitPool(4, 2, 1, 2, tpool);
  vector<SessionGroup> &vctSessGroup = SessionPool::GetVctSessionGroup();
  BOOST_TEST(vctSessGroup.size() == 4);
  for (uint64_t i = 0; i < 4; i++) {
    SessionGroup &sGroup = vctSessGroup[i];
    BOOST_TEST(sGroup._groupSn == i);
    BOOST_TEST(sGroup._restartNum == 1);
    BOOST_TEST(sGroup._currTranId == (1LL << 48) + (i << 40));
  }

  vector<SessionTask *> &vctTask = SessionPool::GetVctSessionTask();
  BOOST_TEST(vctTask.size() == 2);
  for (size_t i = 0; i < 2; i++) {
    SessionTask *task = vctTask[i];
    MVector<SessionGroup *> &vctGroup = task->GetVctSessionGroup();
    BOOST_TEST(vctGroup.size() == 2);
    BOOST_TEST(vctGroup[0]->_groupSn == i * 2);
    BOOST_TEST(vctGroup[1]->_groupSn == i * 2 + 1);
  }

  for (int i = 0; i < 16; i++) {
    StmtResult sr;
    uint32_t sid = SessionPool::CreateSession(i % 2, &sr);
    BOOST_TEST(sid == i);
    BOOST_TEST(sr._status.load(memory_order_relaxed) == ResultStatus::INIT);
    TaskStatus s = vctTask[(i / 2) % 2]->Run();
    BOOST_TEST(s == TaskStatus::INTERVAL);
    BOOST_TEST(sr._status.load(memory_order_relaxed) == ResultStatus::FINISHED);
    BOOST_TEST(sr._sessionId == i);

    BOOST_TEST(vctSessGroup[i % 4]._mapSession.size() == i / 4 + 1);

    auto iter = vctSessGroup[i % 4]._mapSession.find(sid);
    bool b = iter != vctSessGroup[i % 4]._mapSession.end();
    BOOST_TEST(b);
  }

  for (SessionTask *task : vctTask) {
    task->SetStatus(TaskStatus::FINISHED, false);
    task->SetRemovedPool(true);
  }

  for (SessionGroup &sg : vctSessGroup) {
    for (auto iter = sg._mapSession.begin(); iter != sg._mapSession.end();
         iter++) {
      iter->second->_transaction.SetTranStatus(TranStatus::FINISHED);
    }
  }
  ThreadPool::SetThreadId(tidOld);
  tpool->ClearTasks();
  delete tpool;
  SessionPool::ClearPool();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
