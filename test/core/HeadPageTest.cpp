#include "../../src/core/HeadPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)

BOOST_AUTO_TEST_CASE(HeadPage_test) {
  const string FILE_NAME = ROOT_PATH + "/testHeadPage" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  VectorDataValue vctKey;
  VectorDataValue vctVal;
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1,
                         IndexType::PRIMARY);

  HeadPage *headPage = indexTree->GetHeadPage();
  headPage->WriteFileVersion();
  headPage->WriteIndexType(IndexType::PRIMARY);
  headPage->AddNewRecordVersion(100, 0);
  headPage->AddNewRecordVersion(200, 1);
  headPage->WriteKeyVariableFieldCount(100);
  headPage->WriteValueVariableFieldCount(100);

  headPage->WriteTotalPageCount(100);
  BOOST_TEST(100 == headPage->GetAndIncTotalPageCount());

  headPage->WriteRootPagePointer(100);
  headPage->WriteBeginLeafPagePointer(100);
  headPage->WriteEndLeafPagePointer(100);

  headPage->WriteTotalRecordCount(100);
  BOOST_TEST(100 == headPage->GetAndIncTotalRecordCount());

  headPage->WriteRecordStamp(100);
  BOOST_TEST(100 == headPage->GetAndIncRecordStamp());

  headPage->WriteAutoIncrementKey(100);
  BOOST_TEST(100 == headPage->GetAndAddAutoIncrementKey(3));
  headPage->WriteAutoIncrementKey2(100);
  BOOST_TEST(100 == headPage->GetAndAddAutoIncrementKey2(3));
  headPage->WriteAutoIncrementKey3(100);
  BOOST_TEST(100 == headPage->GetAndAddAutoIncrementKey3(3));

  headPage->WritePage();

  headPage = new HeadPage(indexTree);
  headPage->ReadPage();
  FileVersion fv = headPage->ReadFileVersion();
  BOOST_TEST(fv == CURRENT_FILE_VERSION);

  BOOST_TEST(IndexType::PRIMARY == headPage->ReadIndexType());
  BOOST_TEST(2 == headPage->ReadRecordVersionCount());
  BOOST_TEST(100 == headPage->GetRecordVersionStamp(0));
  BOOST_TEST(200 == headPage->GetRecordVersionStamp(1));
  BOOST_TEST(100 == headPage->ReadKeyVariableFieldCount());
  BOOST_TEST(100 == headPage->ReadValueVariableFieldCount());

  BOOST_TEST(101 == headPage->ReadTotalPageCount());
  BOOST_TEST(100 == headPage->ReadRootPagePointer());
  BOOST_TEST(100 == headPage->ReadBeginLeafPagePointer());
  BOOST_TEST(100 == headPage->ReadEndLeafPagePointer());
  BOOST_TEST(101 == headPage->ReadTotalRecordCount());
  BOOST_TEST(101 == headPage->ReadRecordStamp());

  BOOST_TEST(103 == headPage->ReadAutoIncrementKey());
  BOOST_TEST(103 == headPage->ReadAutoIncrementKey2());
  BOOST_TEST(103 == headPage->ReadAutoIncrementKey3());

  delete headPage;
  indexTree->Close(nullptr);
  // PageBufferPool::ClearPool();
  // fs::remove(fs::path(FILE_NAME));
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
