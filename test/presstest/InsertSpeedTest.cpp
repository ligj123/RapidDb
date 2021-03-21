﻿#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/core/IndexTree.h"
#include "../../src/utils/Utilitys.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/pool/PageDividePool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/utils/BytesConvert.h"

namespace storage {
  BOOST_AUTO_TEST_SUITE(PressTest)

    BOOST_AUTO_TEST_CASE(Press_InsertSpeed_test)
  {
    const uint64_t ROW_COUNT = 10000000;
    const string FILE_NAME = "./dbTest/testInsertSpeed" + utils::StrMSTime() + ".dat";
    const string TABLE_NAME = "testTable";

    DataValueLong* dvKey = new DataValueLong(100, true);
    DataValueLong* dvVal = new DataValueLong(200, false);
    VectorDataValue vctKey = { dvKey->CloneDataValue() };
    VectorDataValue vctVal = { dvVal->CloneDataValue() };
    IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
    indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);

    vctKey.push_back(dvKey->CloneDataValue());
    vctVal.push_back(dvVal->CloneDataValue());

    uint64_t dtStart = utils::MSTime();
    uint64_t dtPrev = dtStart;

    for (uint64_t i = 0; i < ROW_COUNT; i++) {
      uint64_t num = i;
      uint64_t by1 = num & 0xff;
      uint64_t by2 = (num >> 8) & 0xff;
      uint64_t by3 = (num >> 16) & 0xff;
      uint64_t by4 = (num >> 24) & 0xff;
      uint64_t priKey = (((by1 & 0x55) + (by4 & 0xAA)) << 24) + (((by2 & 0x55) + (by3 & 0xAA)) << 16) +
        (((by2 & 0xAA) + (by3 & 0x55)) << 8) + (((by1 & 0xAA) + (by4 & 0x55)));
      *((DataValueLong*)vctKey[0]) = priKey;
      *((DataValueLong*)vctVal[0]) = i;
      LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal,
        indexTree->GetHeadPage()->ReadRecordStamp());
      indexTree->InsertRecord(rr);

      if (i % 1000000 == 0) {
        uint64_t dt = utils::MSTime();
        cout << "i=" << i << "\tTotal Time=" << (dt - dtStart) << "\tGap Time=" << (dt - dtPrev) << endl;
        dtPrev = dt;
      }
    }

    uint64_t dtEnd = utils::MSTime();
    cout << "Used Time: " << (dtEnd - dtStart) << endl;

    indexTree->Close(true);
    delete dvKey;
    delete dvVal;
    PageBufferPool::ClearPool();
    //std::filesystem::remove(std::filesystem::path(FILE_NAME));
  }

  BOOST_AUTO_TEST_SUITE_END()
}
