
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
    LOG_INFO << "Suite core setup.";

    TimerThread::Start();
    _threadPool = ThreadPool::InitMain();
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

    PageDividePool::StopPool();
    StoragePool::StopPool();
    PageBufferPool::ClearPool();
    TimerThread::Stop();
    delete _threadPool;

    LOG_INFO << "Suite core tear down.";
  }

  ThreadPool *_threadPool;
};
} // namespace storage