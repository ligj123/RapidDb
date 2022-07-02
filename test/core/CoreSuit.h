
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
    std::cout << "Suite core setup, MemoryUsed: " << std::endl;

    TimerThread::Start();
    _threadPool = ThreadPool::InitMain();
    StoragePool::InitPool(_threadPool);
    StoragePool::AddTimerTask();
    PageDividePool::InitPool(_threadPool);
    PageDividePool::AddTimerTask();
    PageBufferPool::AddTimerTask();
  }
  ~SuiteFixture() {
    PageBufferPool::RemoveTimerTask();
    PageDividePool::RemoveTimerTask();
    StoragePool::RemoveTimerTask();
    delete _threadPool;
    PageDividePool::StopPool();
    StoragePool::StopPool();
    PageBufferPool::ClearPool();
    TimerThread::Stop();

    std::cout << "Suite core tear down, MemoryUsed: " << std::endl;
  }

  ThreadPool *_threadPool;
};

static void ClearCase() {
  PageDividePool::PushTask();
  StoragePool::PushTask();
  PageBufferPool::ClearPool();
}
} // namespace storage