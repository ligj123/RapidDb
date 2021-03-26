#include "PageDividePool.h"
#include "../config/Configure.h"

namespace storage {
const uint64_t PageDividePool::MAX_QUEUE_SIZE =
Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
const uint32_t PageDividePool::BUFFER_FLUSH_INTEVAL_MS = 1 * 1000;
const int PageDividePool::SLEEP_INTEVAL_MS = 100;

map<uint64_t, IndexPage*> PageDividePool::_mapPage;
unordered_map<uint64_t, IndexPage*> PageDividePool::_mapTmp1;
unordered_map<uint64_t, IndexPage*> PageDividePool::_mapTmp2;
thread* PageDividePool::_treeDivideThread = PageDividePool::CreateThread();
bool PageDividePool::_bSuspend = false;
utils::SpinMutex PageDividePool::_spinMutex;

thread* PageDividePool::CreateThread() {
  thread* t = new thread([]() {
    utils::ThreadPool::_threadName = "PageDividePool";

    while (true) {
      if (_bSuspend || (_mapPage.size() == 0 && _mapTmp1.size() == 0)) {
        this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      {
        unique_lock< utils::SpinMutex> lock(_spinMutex);
        _mapTmp2.swap(_mapTmp1);
      }

      for (auto iter = _mapTmp2.cbegin(); iter != _mapTmp2.cend(); iter++) {
        if (_mapPage.find(iter->first) != _mapPage.end()) {
          continue;
        }

        iter->second->IncRefCount();
        iter->second->SetPageLastUpdateTime();
        _mapPage.insert(pair<uint64_t, IndexPage*>(iter->first, iter->second));
      }

      _mapTmp2.clear();

      for (auto iter = _mapPage.begin(); iter != _mapPage.end(); ) {
        auto iter2 = iter++;
        auto page = iter2->second;
        if (page->GetRecordRefCount() > 0 ||
          (!page->IsOverTime(BUFFER_FLUSH_INTEVAL_MS) &&
            !page->IsOverlength())) {
          continue;
        }

        bool b = page->WriteTryLock();
        if (!b || page->GetRecordRefCount() > 0) {
          if (b) page->WriteUnlock();
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
        }
      }
    }
    });
  return t;
}

void PageDividePool::AddCachePage(IndexPage* page) {
  lock_guard<utils::SpinMutex> lock(_spinMutex);
  if (_mapTmp1.find(page->GetPageId()) != _mapTmp1.end()) {
    return;
  }

  _mapTmp1.insert(pair<uint64_t, IndexPage*>(page->GetPageId(), page));
}
}
