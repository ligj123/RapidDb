#include "../../src/utils/ThreadPool.h"
#include "../../src/utils/Log.h"
#include <boost/bind/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ThreadPool_test) {
  class TestTask : public ThreadTask {
  public:
    TestTask() { _bExclusive = true; }
    TaskStatus Run() override {
      _val = ThreadPool::GetThreadId();
      LOG_INFO << "thread id: " << _val;
      while (!ThreadPool::IsStoped()) {
        this_thread::sleep_for(1ms);
      }

      _bExclusive = false;
      return TaskStatus::FINISHED;
    }

  public:
    int _val = 0;
  };

  ThreadPool *tp = new ThreadPool("TestPool", 8, 8);
  TestTask arr[8];
  for (int i = 0; i < 8; i++) {
    tp->AddTask(&arr[i]);
    this_thread::sleep_for(1ms);
  }

  this_thread::sleep_for(10ms);
  tp->SetStop();
  delete tp;

  int count = 0;
  for (int i = 0; i < 8; i++) {
    count += arr[i]._val;
  }

  LOG_INFO << "count: " << count;
  BOOST_TEST(count == 28);
}

BOOST_AUTO_TEST_CASE(ThreadPoolEx_test) {
  class TestTask : public ThreadTask {
  public:
    TaskStatus Run() override {
      this_thread::sleep_for(chrono::microseconds(_usSleep));

      if (ThreadPool::IsStoped() || _bStop) {
        if (IsExclusiveTask()) {
          ThreadTask::SetExclusiveTask(false);
        }
        return TaskStatus::FINISHED;
      } else {
        return TaskStatus::INTERVAL;
      }
    }

    DT_MicroSec _usSleep{100};
    bool _bStop{false};
  };

  class ThreadPoolEx : public ThreadPool {
  public:
    using ThreadPool::_poolBusyDegree;
    using ThreadPool::_queueTask;
    using ThreadPool::_vctThreadPara;
    using ThreadPool::ThreadPool;
  };

  ThreadPoolEx tp("Test_Pool", 1, 8);
  BOOST_TEST(tp.GetMinThreads() == tp.GetAliveThreadCount());
  BOOST_TEST(ThreadTask::GetExclusiveTaskCount() == 0);

  TestTask arrExcTask[5];
  for (int i = 0; i < 5; i++) {
    arrExcTask[i].SetExclusiveTask(true);
    tp.AddTask(&arrExcTask[i]);
  }

  this_thread::sleep_for(1000ms);
  BOOST_TEST(6 == tp.GetAliveThreadCount());
  BOOST_TEST(ThreadTask::GetExclusiveTaskCount() == 5);

  int count = 0;
  for (auto &tpara : tp._vctThreadPara) {
    if (tpara._vctTask.size() == 0) {
      continue;
    }

    BOOST_TEST(tpara._vctTask.size() == 1);
    count++;
  }
  BOOST_TEST(count == 5);

  TestTask arrNormalTask[30];
  for (int i = 0; i < 30; i++) {
    tp.AddTask(&arrNormalTask[i]);
  }

  this_thread::sleep_for(1000ms);
  BOOST_TEST(8 == tp.GetAliveThreadCount());
  BOOST_TEST(ThreadTask::GetExclusiveTaskCount() == 5);

  int excCount = 0;
  int norCount = 0;
  for (auto &tpara : tp._vctThreadPara) {
    if (tpara._vctTask.size() == 1) {
      excCount++;
    } else {
      norCount += tpara._vctTask.size();
    }
  }
  BOOST_TEST(excCount == 5);
  BOOST_TEST(norCount == 30);

  for (int i = 0; i < 30; i++) {
    arrNormalTask[i]._bStop = true;
  }

  this_thread::sleep_for(1000ms);
  BOOST_TEST(6 == tp.GetAliveThreadCount());
  BOOST_TEST(ThreadTask::GetExclusiveTaskCount() == 5);

  for (int i = 0; i < 5; i++) {
    arrExcTask[i].SetExclusiveTask(false);
    arrExcTask[i]._bStop = true;
  }

  this_thread::sleep_for(1000ms);
  BOOST_TEST(1 == tp.GetAliveThreadCount());
  BOOST_TEST(ThreadTask::GetExclusiveTaskCount() == 0);

  tp.SetStop();
  this_thread::sleep_for(1000ms);
  BOOST_TEST(tp.GetAliveThreadCount() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
