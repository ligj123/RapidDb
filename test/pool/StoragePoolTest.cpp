#include "../../src/pool/StoragePool.h"
#include "../../src/core/IndexTree.h"
#include "../../src/utils/Utilitys.h"
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;
atomic_int32_t atm(0);

BOOST_AUTO_TEST_SUITE(PoolTest)

BOOST_AUTO_TEST_CASE(StoragePool_test) {
  const string FILE_NAME = "./dbTest/testStoragePool" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int NUM = 10000;

  class StorageTask : public Task {
  public:
    StorageTask(IndexTree *indexTree, const char *pStrTest, size_t sz)
        : _indexTree(indexTree), _pStrTest(pStrTest), _strSize(sz) {}

    bool IsSmallTask() override { return false; }
    void Run() {
      while (true) {
        int ii = atm.fetch_add(1, memory_order_relaxed);
        if (ii >= NUM) {
          break;
        }

        CachePage *page = new CachePage(_indexTree, ii, PageType::UNKNOWN);
        page->WriteInt(0, ii);
        BytesCopy(page->GetBysPage() + 4, _pStrTest, _strSize);
        BytesCopy(page->GetBysPage() + Configure::GetCachePageSize() - _strSize,
                  _pStrTest, _strSize);
        page->SetDirty(true);
        StoragePool::WriteCachePage(page);
      }
    }

    IndexTree *_indexTree;
    const char *_pStrTest;
    size_t _strSize;
  };

  ThreadPool *tp = new ThreadPool("StorageTest");
  StoragePool::InitPool(tp);
  string strTest = "abcdefg1234567890中文测试abcdefghigjlmnopqrstuvwrst";
  strTest += strTest;
  strTest += strTest;
  size_t sz = strTest.size() + 1;
  const char *pStrTest = strTest.c_str();

  VectorDataValue vctKey;
  VectorDataValue vctVal;
  IndexTree *indexTree =
      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal, IndexType::PRIMARY);

  int hc = std::thread::hardware_concurrency();
  for (int i = 1; i <= hc; i++) {
    StorageTask *task = new StorageTask(indexTree, pStrTest, sz);
    tp->AddTask(task);
  }

  while (atm.load(memory_order_relaxed) < NUM &&
         StoragePool::GetWaitingPageCount() > 0) {
    this_thread::sleep_for(1ms);
  }
  indexTree->Close();
  delete indexTree;

  class PageCmpTask : public Task {
  public:
    PageCmpTask(CachePage *page, int id, const char *pStrTest, size_t sz)
        : _page(page), _id(id), _pStrTest(pStrTest), _strSize(sz) {}
    bool IsSmallTask() override { return false; }
    void Run() {
      BOOST_TEST(_id == _page->ReadInt(0));
      BOOST_TEST(memcmp(_pStrTest, _page->GetBysPage() + 4, _strSize) == 0);
      BOOST_TEST(
          memcmp(_pStrTest,
                 _page->GetBysPage() + Configure::GetCachePageSize() - _strSize,
                 _strSize) == 0);
    }

    CachePage *_page;
    int _id;
    const char *_pStrTest;
    size_t _strSize;
  };

  indexTree =
      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal, IndexType::UNKNOWN);
  for (int i = 1; i <= NUM; i++) {
    CachePage *page = new CachePage(indexTree, i, PageType::UNKNOWN);
    PageCmpTask *ctask = new PageCmpTask(page, i, pStrTest, sz);
    page->PushWaitTask(ctask);
    ReadPageTask *rtask = new ReadPageTask(page);
    tp->AddTask(rtask);
  }

  indexTree->Close();
  delete indexTree;
  fs::remove(fs::path(FILE_NAME));
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
