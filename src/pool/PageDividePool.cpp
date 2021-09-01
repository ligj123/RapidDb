#include "PageDividePool.h"
#include "../config/Configure.h"

namespace storage {
const uint64_t PageDividePool::MAX_QUEUE_SIZE =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
const uint32_t PageDividePool::BUFFER_FLUSH_INTEVAL_MS = 1 * 1000;
const int PageDividePool::SLEEP_INTEVAL_MS = 100;

MTreeMap<uint64_t, IndexPage *>::Type PageDividePool::_mapPage;
MHashMap<uint64_t, IndexPage *>::Type PageDividePool::_mapTmp;
thread *PageDividePool::_treeDivideThread = PageDividePool::CreateThread();
bool PageDividePool::_bSuspend = false;
SpinMutex PageDividePool::_spinMutex;
bool PageDividePool::_bStop = false;

thread *PageDividePool::CreateThread() {
  thread *t = new thread([]() {
    ThreadPool::_threadName = "PageDividePool";
    _mapTmp.reserve(100000);

    while (true) {
      if (_bSuspend || (_mapPage.size() == 0 && _mapTmp.size() == 0)) {
        this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      MHashMap<uint64_t, IndexPage *>::Type mapTmp;
      {
        unique_lock<SpinMutex> lock(_spinMutex);
        _mapTmp.swap(mapTmp);
      }

      for (auto iter = mapTmp.cbegin(); iter != mapTmp.cend(); iter++) {
        if (_mapPage.find(iter->first) != _mapPage.end()) {
          iter->second->DecRefCount();
          continue;
        }

        _mapPage.insert(pair<uint64_t, IndexPage *>(iter->first, iter->second));
      }

      if (_mapPage.size() < 1000) {
        this_thread::sleep_for(std::chrono::milliseconds(10));
        if (_mapPage.size() == 0 && _mapTmp.size() == 0 && _bStop)
          break;
      }

      for (auto iter = _mapPage.begin(); iter != _mapPage.end();) {
        auto iter2 = iter++;
        auto page = iter2->second;
        if (page->GetRecordTranCount() > 0 ||
            (!page->IsOverTime(BUFFER_FLUSH_INTEVAL_MS) &&
             !page->IsOverlength())) {
          continue;
        }

        bool b = page->WriteTryLock();
        if (!b || page->GetRecordTranCount() > 0) {
          if (b)
            page->WriteUnlock();
          continue;
        }

        bool bPassed = false;
        if (page->GetTotalDataLength() > page->GetMaxDataLength()) {
          bPassed = page->PageDivide();
        } else {
          bPassed = page->SaveRecords();
        }

        page->WriteUnlock();
        if (bPassed) {
          page->DecRefCount();
          _mapPage.erase(iter2);
        }
      }
    }
  });
  return t;
}

void PageDividePool::AddCachePage(IndexPage *page) {
  lock_guard<SpinMutex> lock(_spinMutex);
  if (_mapTmp.find(page->GetPageId()) != _mapTmp.end()) {
    return;
  }

  page->IncRefCount();
  page->SetPageLastUpdateTime();
  _mapTmp.insert(pair<uint64_t, IndexPage *>(page->GetPageId(), page));
}
} // namespace storage
