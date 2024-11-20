#include "../../src/pool/FilePagePool.h"
#include "../../src/core/CachePage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/utils/Log.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <cstring>
#include <filesystem>

namespace storage {
class CachePageEx : public CachePage {
public:
  CachePageEx(IndexTree *indexTree, PageID pageId)
      : CachePage(indexTree, pageId, PageType::LEAF_PAGE) {
    _bysPage = CachePool::ApplyPage();
    _pageStatus.store(PageStatus::VALID, memory_order_relaxed);
  }

  ~CachePageEx() { CachePool::ReleasePage(_bysPage); }

  uint32_t PageSize() const override { return CachePage::INDEX_PAGE_SIZE; }
  void AfterRead() override {
    _pageStatus.store(PageStatus::READED, memory_order_release);
  }
};

BOOST_AUTO_TEST_SUITE(PoolTest)
BOOST_AUTO_TEST_CASE(FilePagePoolSync_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testPoolSync" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testSyncTable";

  string strTest = "abcdefg1234567890中文测试abcdefghigjlmnopqrstuvwrst";
  strTest += strTest;
  strTest += strTest;
  size_t sz = strTest.size() + 1;
  const char *pStrTest = strTest.c_str();

  VectorDataValue vctKey;
  VectorDataValue vctVal;

  IndexTree idxTree;
  idxTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey, vctVal,
                          GetFileId(), IndexType::PRIMARY);

  CachePageEx page(&idxTree, 1);
  page.WriteInt(0, 100);
  Byte *bys = page.GetBysPage();
  BytesCopy(bys + 4, pStrTest, sz);
  page.WriteInt(CachePage::INDEX_PAGE_SIZE - 4, 0x5A5A5A5A);
  page.SetDirty();

  bool b = FilePagePool::SyncWritePage(&page);
  BOOST_TEST(b);
  CachePageEx page2(&idxTree, 1);
  b = FilePagePool::SyncReadPage(&page2);
  BOOST_TEST(b);
  BOOST_TEST(100 == page2.ReadInt(0));
  BOOST_TEST(
      BytesEqual((const Byte *)pStrTest, sz, page2.GetBysPage() + 4, sz));
  BOOST_TEST(0x5A5A5A5A == page2.ReadInt(CachePage::INDEX_PAGE_SIZE - 4));

  idxTree.Close();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(FilePagePoolAsync_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testPoolSync" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testSyncTable";

  string strTest = "abcdefg1234567890中文测试abcdefghigjlmnopqrstuvwrst";
  strTest += strTest;
  strTest += strTest;
  size_t sz = strTest.size() + 1;
  const char *pStrTest = strTest.c_str();
  FilePagePool::Start(1);

  VectorDataValue vctKey;
  VectorDataValue vctVal;

  IndexTree idxTree;
  idxTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey, vctVal,
                          GetFileId(), IndexType::PRIMARY);

  MVector<CachePageEx *> vctPage;
  vctPage.reserve(1000);
  for (uint32_t i = 1; i <= 1000; i++) {
    CachePageEx *page = new CachePageEx(&idxTree, i);
    page->WriteInt(0, i + 100);
    Byte *bys = page->GetBysPage();
    BytesCopy(bys + 4, pStrTest, sz);
    page->WriteInt(CachePage::INDEX_PAGE_SIZE - 4, 0x5A5A5A5A);
    page->SetDirty();
    FilePagePool::AddWritePage(0, page, true);
    vctPage.push_back(page);
  }

  for (uint32_t i = 0; i < 1000; i++) {
    while (vctPage[i]->GetPageStatus() != PageStatus::VALID) {
      std::this_thread::yield();
    }

    delete vctPage[i];
  }

  vctPage.clear();
  for (uint32_t i = 1; i <= 1000; i++) {
    CachePageEx *page = new CachePageEx(&idxTree, i);
    page->SetPageStatus(PageStatus::EMPTY);
    FilePagePool::AddReadPage(0, page, true);
    vctPage.push_back(page);
  }

  for (uint32_t i = 0; i < 1000; i++) {
    while (vctPage[i]->GetPageStatus() != PageStatus::READED) {
      std::this_thread::yield();
    }

    CachePageEx *page = vctPage[i];
    BOOST_TEST(i + 101 == page->ReadInt(0));
    BOOST_TEST(
        BytesEqual((const Byte *)pStrTest, sz, page->GetBysPage() + 4, sz));
    BOOST_TEST(0x5A5A5A5A == page->ReadInt(CachePage::INDEX_PAGE_SIZE - 4));
    delete page;
  }

  vctPage.clear();
  idxTree.Close();
  CachePagePool::ClearPool();
  FilePagePool::Stop();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
