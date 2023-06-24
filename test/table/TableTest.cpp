#include "../../src/table/Table.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/table/Column.h"
#include "../../src/utils/Utilitys.h"

#include <boost/test/unit_test.hpp>
#include <cstring>
#include <filesystem>
namespace storage {
namespace fs = std::filesystem;
BOOST_AUTO_TEST_SUITE(TableTest)

BOOST_AUTO_TEST_CASE(PhyColumn_test) {
  PhysColumn clm("abcd", 10, DataType::UINT, "test", false, 100, 1, 2,
                 Charsets::UNKNOWN, new DataValueUInt(20u));

  BOOST_TEST(strcmp("abcd", clm.GetName().c_str()) == 0);
  BOOST_TEST(clm.GetIndex() == 10);
  BOOST_TEST(clm.GetDataType() == DataType::UINT);
  BOOST_TEST(strcmp("test", clm.GetComments().c_str()) == 0);
  BOOST_TEST(!clm.IsNullable());
  BOOST_TEST(clm.GetMaxLength() == 100);
  BOOST_TEST(clm.GetInitVal() == 1);
  BOOST_TEST(clm.GetIncStep() == 2);
  BOOST_TEST(clm.GetCharset() == Charsets::UNKNOWN);
  BOOST_TEST(clm.GetDefaultVal()->GetDataType() == DataType::UINT);
  BOOST_TEST((uint32_t)(*(DataValueUInt *)clm.GetDefaultVal()) == 20);

  Byte bys[2000];
  uint32_t sz = clm.CalcSize();
  uint32_t sz2 = clm.WriteData(bys);
  BOOST_TEST(sz == sz2);

  PhysColumn clm2(1);
  sz2 = clm2.ReadData(bys);
  BOOST_TEST(sz == sz2);

  BOOST_TEST(strcmp("abcd", clm2.GetName().c_str()) == 0);
  BOOST_TEST(clm2.GetIndex() == 1);
  BOOST_TEST(clm2.GetDataType() == DataType::UINT);
  BOOST_TEST(strcmp("test", clm2.GetComments().c_str()) == 0);
  BOOST_TEST(!clm2.IsNullable());
  BOOST_TEST(clm2.GetMaxLength() == 100);
  BOOST_TEST(clm2.GetInitVal() == 1);
  BOOST_TEST(clm2.GetIncStep() == 2);
  BOOST_TEST(clm2.GetCharset() == Charsets::UNKNOWN);
  BOOST_TEST(clm2.GetDefaultVal()->GetDataType() == DataType::UINT);
  BOOST_TEST((uint32_t)(*(DataValueUInt *)clm2.GetDefaultVal()) == 20);
}

BOOST_AUTO_TEST_CASE(PhysTable_test) {
  PhysTable ptable("testdb", "testtable", "PhysTable_test", 0x100,
                   MilliSecTime());
  ptable.AddColumn("c1", DataType::LONG, "primary key", 100, 2);
  ptable.AddColumn("c2", DataType::VARCHAR, false, 100, "varchar test",
                   Charsets::UTF8,
                   "this is default value for the varchr column");
  ptable.AddColumn("c3", DataType::DOUBLE, true, 0, "double column test",
                   Charsets::UTF8, 1.23456);
  ptable.AddColumn("c4", DataType::FIXCHAR, true, 50, "Fixchar test",
                   Charsets::UTF8,
                   "this is default value for the fixchar column");
  ptable.AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});
  ptable.AddIndex(IndexType::UNIQUE, "c2_unique", {"c2"});
  ptable.AddIndex(IndexType::NON_UNIQUE, "c3_c4_non_unique", {"c3", "c4"});

  BOOST_TEST(ptable.GetDbName() == "testdb");
  BOOST_TEST(ptable.GetTableName() == "testtable");
  BOOST_TEST(ptable.GetFullName() == "testdb.testtable");
  BOOST_TEST(ptable.TableID() == 0x100);

  const MVector<IndexProp> &vct_ip = ptable.GetVectorIndex();
  BOOST_TEST(vct_ip.size() == 3);
  BOOST_TEST(vct_ip[0]._name == PRIMARY_KEY);
  BOOST_TEST(vct_ip[0]._position == 0);
  BOOST_TEST(vct_ip[0]._type == IndexType::PRIMARY);
  BOOST_TEST(vct_ip[0]._vctCol.size() == 1);
  BOOST_TEST(vct_ip[0]._vctCol[0].colName == "c1");
  BOOST_TEST(vct_ip[0]._vctCol[0].colPos == 0);

  BOOST_TEST(vct_ip[1]._name == "c2_unique");
  BOOST_TEST(vct_ip[1]._position == 1);
  BOOST_TEST(vct_ip[1]._type == IndexType::UNIQUE);
  BOOST_TEST(vct_ip[1]._vctCol.size() == 1);
  BOOST_TEST(vct_ip[1]._vctCol[0].colName == "c2");
  BOOST_TEST(vct_ip[1]._vctCol[0].colPos == 0);

  BOOST_TEST(vct_ip[2]._name == "c3_c4_non_unique");
  BOOST_TEST(vct_ip[2]._position == 2);
  BOOST_TEST(vct_ip[2]._type == IndexType::NON_UNIQUE);
  BOOST_TEST(vct_ip[2]._vctCol.size() == 2);
  BOOST_TEST(vct_ip[2]._vctCol[0].colName == "c3");
  BOOST_TEST(vct_ip[2]._vctCol[0].colPos == 0);
  BOOST_TEST(vct_ip[2]._vctCol[1].colName == "c4");
  BOOST_TEST(vct_ip[2]._vctCol[1].colPos == 1);

  BOOST_TEST(ptable.GetIndexType(c3_c4_non_unique) == IndexType::NON_UNIQUE);

  const MVector<PhysColumn> &vct_col = ptable.GetColumnArray();
  BOOST_TEST()
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
