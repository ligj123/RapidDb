#include "../../src/pool/FilePagePool.h"

#include "../../src/core/CachePage.h"
#include "../../src/core/IndexTree.h"
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

  void AfterRead() override {
    _pageStatus.store(PageStatus::READED, memory_order_release);
  }
};
BOOST_AUTO_TEST_SUITE(PoolTest)
BOOST_AUTO_TEST_CASE(FilePagePool_test) {
  const string FILE_NAME = ROOT_PATH + "/testPool" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  string strTest = "abcdefg1234567890中文测试abcdefghigjlmnopqrstuvwrst";
  strTest += strTest;
  strTest += strTest;
  size_t sz = strTest.size() + 1;
  const char *pStrTest = strTest.c_str();

  VectorDataValue vctKey;
  VectorDataValue vctVal;

  IndexTree idxTree;
  idxTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey, vctVal,
                          g_atmFileId.fetch_add(1, memory_order_relaxed),
                          IndexType::PRIMARY);

  CachePageEx page(&idxTree, 1, PageType::LEAF_PAGE);
  page.WriteInt(100);
  Byte *bys = page.GetBysPage();
  BytesCopy(bys + 4, pStrTest, sz);
  page->SetDirty(true);

  FilePagePool::SyncWritePage(page);
  CachePageEx page2(&idxTree, 1, PageType::LEAF_PAGE);
  FilePagePool::SyncReadPage(page);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
