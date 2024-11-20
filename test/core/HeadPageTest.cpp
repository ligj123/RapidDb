#include "../../src/core/HeadPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/dataType/DataValueFixChar.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/pool/FilePagePool.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;
BOOST_AUTO_TEST_SUITE(CoreTest)
BOOST_AUTO_TEST_CASE(HeadPage_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testHeadPage" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  VectorDataValue vctKey;
  VectorDataValue vctVal;
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::NON_UNIQUE);

  VectorDataValue vctDv;
  vctDv.push_back(new DataValueInt(1));
  vctDv.push_back(new DataValueFixChar(15));
  vctDv.push_back(new DataValueVarChar(100));

  HeadPage *headPage = indexTree.GetHeadPage();
  headPage->InitHeadPage(IndexType::NON_UNIQUE, vctDv);

  BOOST_TEST(headPage->GetValueVariableFieldCount() == 1);
  BOOST_TEST(headPage->GetKeyVariableFieldCount() == 0);
  BOOST_TEST(headPage->GetIndexType() == IndexType::NON_UNIQUE);

  BOOST_TEST(headPage->GetTotalPageCount() == 1);
  headPage->SetTotalPageCount(100);
  BOOST_TEST(100 == headPage->GetAndIncTotalPageCount(3));
  BOOST_TEST(103 == headPage->GetTotalPageCount());

  BOOST_TEST(headPage->GetRootPageID() == 0);
  BOOST_TEST(headPage->GetBeginLeafPageID() == 0);
  BOOST_TEST(headPage->GetEndLeafPageID() == 0);
  headPage->SetRootPageID(100);
  headPage->SetBeginLeafPageID(200);
  headPage->SetEndLeafPageID(300);
  BOOST_TEST(headPage->GetRootPageID() == 100);
  BOOST_TEST(headPage->GetBeginLeafPageID() == 200);
  BOOST_TEST(headPage->GetEndLeafPageID() == 300);

  BOOST_TEST(headPage->GetTotalRecordCount() == 0);
  headPage->SetTotalRecordCount(100);
  BOOST_TEST(100 == headPage->GetAndIncTotalRecordCount(3));
  BOOST_TEST(103 == headPage->GetTotalRecordCount());

  BOOST_TEST(headPage->GetRecordStamp() == 0);
  headPage->SetRecordStamp(100);
  BOOST_TEST(100 == headPage->GetAndIncRecordStamp(3));
  BOOST_TEST(103 == headPage->GetRecordStamp());

  BOOST_TEST(headPage->GetAutoIncrementKey() == 0);
  headPage->SetAutoIncrementKey(100);
  BOOST_TEST(100 == headPage->GetAndIncAutoIncrementKey(3));
  BOOST_TEST(103 == headPage->GetAutoIncrementKey());

  headPage->SaveToBuffer();
  FilePagePool::SyncWritePage(headPage);

  headPage = new HeadPage(&indexTree);
  FilePagePool::SyncReadPage(headPage);
  headPage->InitParameters();

  FileVersion fv = headPage->ReadFileVersion();
  BOOST_TEST(fv == CURRENT_FILE_VERSION);

  BOOST_TEST(IndexType::NON_UNIQUE == headPage->GetIndexType());
  BOOST_TEST(0 == headPage->GetKeyVariableFieldCount());
  BOOST_TEST(1 == headPage->GetValueVariableFieldCount());
  BOOST_TEST(103 == headPage->GetTotalPageCount());
  BOOST_TEST(100 == headPage->GetRootPageID());
  BOOST_TEST(200 == headPage->GetBeginLeafPageID());
  BOOST_TEST(300 == headPage->GetEndLeafPageID());
  BOOST_TEST(103 == headPage->GetTotalRecordCount());
  BOOST_TEST(103 == headPage->GetAutoIncrementKey());
  BOOST_TEST(103 == headPage->GetRecordStamp());

  delete headPage;
  indexTree.Close();
  CachePagePool::ClearPool();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
