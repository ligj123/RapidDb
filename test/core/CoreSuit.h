
#include "../../src/pool/PageBufferPool.h"
#include "../../src/pool/PageDividePool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/utils/ThreadPool.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace std;
namespace storage {
struct SuiteFixture {
  SuiteFixture() {
    LOG_INFO << "Suite core setup: "
             << boost::unit_test::framework::current_test_case().p_name;
    _threadPool = ThreadPool::InitMain(100000, 1, 1);
    TimerThread::Start();
    StoragePool::InitPool(_threadPool);
    StoragePool::AddTimerTask();
    PageDividePool::InitPool(_threadPool);
    PageDividePool::AddTimerTask();
    PageBufferPool::InitPool(_threadPool);
    PageBufferPool::AddTimerTask();
  }
  ~SuiteFixture() {
    PageBufferPool::RemoveTimerTask();
    PageDividePool::RemoveTimerTask();
    StoragePool::RemoveTimerTask();
    TimerThread::Stop();

    ThreadPool::StopMain();
    _threadPool = nullptr;

    PageDividePool::StopPool();
    StoragePool::StopPool();
    PageBufferPool::StopPool();

    LOG_INFO << "Suite core tear down.";
  }

  ThreadPool *_threadPool;
};
} // namespace storage