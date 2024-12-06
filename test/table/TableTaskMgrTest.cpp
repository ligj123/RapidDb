#include "../../src/table/TableTaskMgr.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/manager/DatabaseManager.h"
#include "../../src/table/Column.h"
#include "../../src/table/Database.h"
#include "../../src/table/Table.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>

namespace storage {
PhysTable *CreateTestTable(Database *db, const MString &tableName, int rowNum) {
  MTreeMap<uint64_t, CachePage *> pageMap;
  PhysTable *ptable =
      new PhysTable(db, tableName, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::FIXCHAR, false, 1000, "primary key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c2", DataType::VARCHAR, false, 1000, "Unique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c3", DataType::FIXCHAR, true, 50, "NonUnique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});
  ptable->AddIndex(IndexType::UNIQUE, "c2_unique", {"c2"});
  ptable->AddIndex(IndexType::NON_UNIQUE, "c3_non_unique", {"c3"});

  ptable->OpenIndex(0, true);
  ptable->OpenIndex(1, true);
  ptable->OpenIndex(2, true);

  MString fix1000 = "FIXCHAR_1000_" + MString(970, 'a');
  DataValueFixChar dvFix1000(fix1000.c_str(), 982);
  dvFix1000.SetConstRef();

  MString var1000 = "VARCHAR_1000_" + MString(980, 'a');
  DataValueVarChar dvVar1000(var1000.c_str(), 982);
  dvVar1000.SetConstRef();

  MString fix50 = "FIXCHAR_50_" + MString(28, 'a');
  DataValueFixChar dvFix50(fix50.c_str(), 38);
  dvFix50.SetConstRef();

  IndexTree *priTree = ptable->GetVectorIndex()[0]._tree;
  IndexTree *uniTree = ptable->GetVectorIndex()[1]._tree;
  IndexTree *nonTree = ptable->GetVectorIndex()[2]._tree;

  for (int i = 0; i < rowNum; i++) {
    stringstream ss;
    ss << "_0x" << std::setfill('0') << std::setw(8) << std::hex << i;
    memcpy(dvFix1000.GetBuff() + 982, ss.str().c_str(), 11);
    memcpy(dvVar1000.GetBuff() + 982, ss.str().c_str(), 12);
    memcpy(dvFix50.GetBuff() + 38, ss.str().c_str(), 12);

    VectorDataValue vctKey = {&dvFix1000};
    VectorDataValue vctVal = {&dvFix1000, &dvVar1000, &dvFix50};
    VersionStamp stamp = priTree->GetHeadPage()->GetAndIncRecordStamp();
    LeafRecord *lr = new LeafRecord(priTree, vctKey, vctVal, stamp);

    IndexPage *idxPage = nullptr;
    bool b = priTree->SearchPage(*lr, idxPage);
    assert(b && idxPage->GetPageType() == PageType::LEAF_PAGE);
    LeafPage *lp = (LeafPage *)idxPage;
    bool bFind;
    int32_t pos = lp->SearchRecord(*lr, bFind);
    lp->InsertRecord(lr, pos);
    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap, UINT8_MAX);
    }

    vctKey = {&dvVar1000};
    LeafRecord *ulr =
        new LeafRecord(uniTree, vctKey, lr->GetBysValue() + UI16_2_LEN,
                       lr->GetKeyLength(), ActionType::INSERT, stamp);

    idxPage = nullptr;
    b = uniTree->SearchPage(*ulr, idxPage);
    assert(b && idxPage->GetPageType() == PageType::LEAF_PAGE);
    lp = (LeafPage *)idxPage;
    pos = lp->SearchRecord(*ulr, bFind);
    lp->InsertRecord(ulr, pos);
    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap, UINT8_MAX);
    }

    vctKey = {&dvFix50};
    LeafRecord *nlr =
        new LeafRecord(nonTree, vctKey, lr->GetBysValue() + UI16_2_LEN,
                       lr->GetKeyLength(), ActionType::INSERT, stamp);

    idxPage = nullptr;
    b = nonTree->SearchPage(*nlr, idxPage);
    assert(b && idxPage->GetPageType() == PageType::LEAF_PAGE);
    lp = (LeafPage *)idxPage;
    pos = lp->SearchRecord(*nlr, bFind);
    lp->InsertRecord(nlr, pos);
    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap, UINT8_MAX);
    }
  }

  IndexTree::SettleUpdatedPages(pageMap);
  assert(pageMap.size() == 0);
  return ptable;
}

BOOST_AUTO_TEST_SUITE(TableTest)

// BOOST_AUTO_TEST_CASE(TableTaskMgr_test) {
//   LOG_INFO << "Run testcase: "
//            << boost::unit_test::framework::current_test_case().p_name;
//   const MString TABLE_NAME = "testTableMgr";
//   const MString DB_NAME = "testDbMgr";

//   Database *db = new Database(1, ROOT_PATH.c_str(), DB_NAME, MilliSecTime(),
//                               MicroSecTime());
//   DatabaseManager::AddDb(db);
//   PhysTable *table = CreateTestTable(db, TABLE_NAME, 1000);

//   // ThreadPool::CreateMainPool()
// }

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
