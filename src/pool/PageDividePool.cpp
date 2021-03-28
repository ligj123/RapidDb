#include "PageDividePool.h"
#include "../config/Configure.h"

namespace storage {
const uint64_t PageDividePool::MAX_QUEUE_SIZE =
Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
const uint32_t PageDividePool::BUFFER_FLUSH_INTEVAL_MS = 1 * 1000;
const int PageDividePool::SLEEP_INTEVAL_MS = 100;

map<uint64_t, IndexPage*> PageDividePool::_mapPage;
unordered_map<uint64_t, IndexPage*> PageDividePool::_mapTmp1;
thread* PageDividePool::_treeDivideThread = PageDividePool::CreateThread();
bool PageDividePool::_bSuspend = false;
utils::SpinMutex PageDividePool::_spinMutex;

thread* PageDividePool::CreateThread() {
  thread* t = new thread([]() {
    utils::ThreadPool::_threadName = "PageDividePool";

    while (true) {
      if (_bSuspend || (_mapPage.size() + _mapTmp1.size() == 0)) {
        this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      unordered_map<uint64_t, IndexPage*> mapTmp2;
      {
        unique_lock< utils::SpinMutex> lock(_spinMutex);
        mapTmp2.swap(_mapTmp1);
      }

      for (auto iter = mapTmp2.cbegin(); iter != mapTmp2.cend(); iter++) {
        if (_mapPage.find(iter->first) != _mapPage.end()) {
          iter->second->DecRefCount();
          continue;
        }

        _mapPage.insert(pair<uint64_t, IndexPage*>(iter->first, iter->second));
      }

      if (_mapPage.size() < 1000) {
        this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      for (auto iter = _mapPage.begin(); iter != _mapPage.end(); ) {
        auto iter2 = iter++;
        auto page = iter2->second;
        if (page->GetRecordRefCount() > 0 ||
          (!page->IsOverTime(BUFFER_FLUSH_INTEVAL_MS) &&
            !page->IsOverlength())) {
          continue;
        }

        bool b = page->TryUpdateLock();
        if (!b || page->GetRecordRefCount() > 0) {
          if (b) page->UpdateUnlock();
          continue;
        }

        bool bPassed = false;
        if (page->GetTotalDataLength() > page->GetMaxDataLength()) {
          bPassed = page->PageDivide();
        } else {
          bPassed = page->SaveRecords();
        }

        page->UpdateUnlock();
        if (bPassed) {
          page->DecRefCount();
          _mapPage.erase(iter2);
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

  page->IncRefCount();
  page->SetPageLastUpdateTime();
  _mapTmp1.insert(pair<uint64_t, IndexPage*>(page->GetPageId(), page));
}
}
