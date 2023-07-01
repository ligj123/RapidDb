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
  const char *var_default = "this is default value for the varchr column";
  const char *fix_default = "this is default value for the fixchar column";
  PhysTable ptable("testdb", "testtable", "PhysTable_test", 0x100,
                   MilliSecTime());
  ptable.AddColumn("c1", DataType::LONG, "primary key", 100, 2);
  ptable.AddColumn("c2", DataType::VARCHAR, false, 100, "varchar test",
                   Charsets::UTF8, var_default);
  ptable.AddColumn("c3", DataType::DOUBLE, true, -1, "double column test",
                   Charsets::UTF8, 1.23456);
  ptable.AddColumn("c4", DataType::FIXCHAR, true, 50, "Fixchar test",
                   Charsets::UTF8, fix_default);
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

  BOOST_TEST(ptable.GetIndexType("c2_unique") == IndexType::NON_UNIQUE);

  const MVector<PhysColumn> &vct_col = ptable.GetColumnArray();
  BOOST_TEST(vct_col.size() == 4);
  BOOST_TEST(vct_col[0].GetName() == "c1");
  BOOST_TEST(vct_col[0].GetIndex() == 0);
  BOOST_TEST(vct_col[0].GetDataType() == DataType::LONG);
  BOOST_TEST(vct_col[0].IsNullable() == false);
  BOOST_TEST(vct_col[0].GetMaxLength() == -1);
  BOOST_TEST(vct_col[0].GetInitVal() == 100);
  BOOST_TEST(vct_col[0].GetIncStep() == 2);
  BOOST_TEST(vct_col[0].GetCharset() == Charsets::UTF8);
  BOOST_TEST(vct_col[0].GetComments() == "primary key");
  BOOST_TEST(vct_col[0].GetDefaultVal() == nullptr);

  BOOST_TEST(vct_col[1].GetName() == "c2");
  BOOST_TEST(vct_col[1].GetIndex() == 1);
  BOOST_TEST(vct_col[1].GetDataType() == DataType::VARCHAR);
  BOOST_TEST(vct_col[1].IsNullable() == false);
  BOOST_TEST(vct_col[1].GetMaxLength() == 100);
  BOOST_TEST(vct_col[1].GetInitVal() == -1);
  BOOST_TEST(vct_col[1].GetIncStep() == -1);
  BOOST_TEST(vct_col[1].GetCharset() == Charsets::UTF8);
  BOOST_TEST(vct_col[1].GetComments() == "varchar test");
  BOOST_TEST(*(const DataValueVarChar *)vct_col[1].GetDefaultVal() ==
             DataValueVarChar(var_default, (uint32_t)strlen(var_default)));

  BOOST_TEST(vct_col[2].GetName() == "c3");
  BOOST_TEST(vct_col[2].GetIndex() == 2);
  BOOST_TEST(vct_col[2].GetDataType() == DataType::DOUBLE);
  BOOST_TEST(vct_col[2].IsNullable() == true);
  BOOST_TEST(vct_col[2].GetMaxLength() == 0);
  BOOST_TEST(vct_col[2].GetInitVal() == -1);
  BOOST_TEST(vct_col[2].GetIncStep() == -1);
  BOOST_TEST(vct_col[2].GetCharset() == Charsets::UTF8);
  BOOST_TEST(vct_col[2].GetComments() == "double column test");
  BOOST_TEST(vct_col[2].GetDefaultVal()->GetDouble() == 1.23456);

  BOOST_TEST(vct_col[3].GetName() == "c4");
  BOOST_TEST(vct_col[3].GetIndex() == 3);
  BOOST_TEST(vct_col[3].GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(vct_col[3].IsNullable() == true);
  BOOST_TEST(vct_col[3].GetMaxLength() == 0);
  BOOST_TEST(vct_col[3].GetInitVal() == -1);
  BOOST_TEST(vct_col[3].GetIncStep() == -1);
  BOOST_TEST(vct_col[3].GetCharset() == Charsets::UTF8);
  BOOST_TEST(vct_col[3].GetComments() == "Fixchar test");
  BOOST_TEST(vct_col[3].GetDefaultVal()->GetDataType() == DataType::FIXCHAR);

  Byte buf[10000];
  uint32_t sz = ptable.CalcSize();
  uint32_t sz2 = ptable.SaveData(buf);
  BOOST_TEST(sz = sz2);

  PhysTable ptable2;
  sz2 = ptable2.LoadData(buf);
  BOOST_TEST(sz = sz2);
  BOOST_TEST(ptable2.GetIndexType(PRIMARY_KEY) == IndexType::PRIMARY);
  BOOST_TEST(ptable2.GetIndexType("c2_unique") == IndexType::UNIQUE);
  BOOST_TEST(ptable2.GetIndexType("c3_c4_non_unique") == IndexType::NON_UNIQUE);

  const PhysColumn *col = ptable2.GetColumn("c1");
  BOOST_TEST(col == ptable2.GetColumn(0));
  BOOST_TEST(col->GetName() == "c1");
  BOOST_TEST(col->GetIndex() == 0);
  BOOST_TEST(col->GetDataType() == DataType::LONG);
  BOOST_TEST(col->IsNullable() == false);
  BOOST_TEST(col->GetMaxLength() == -1);
  BOOST_TEST(col->GetInitVal() == 100);
  BOOST_TEST(col->GetIncStep() == 2);
  BOOST_TEST(col->GetCharset() == Charsets::UTF8);
  BOOST_TEST(col->GetComments() == "primary key");
  BOOST_TEST(col->GetDefaultVal() == nullptr);

  col = ptable2.GetColumn("c2");
  BOOST_TEST(col == ptable2.GetColumn(1));
  BOOST_TEST(col->GetName() == "c2");
  BOOST_TEST(col->GetIndex() == 1);
  BOOST_TEST(col->GetDataType() == DataType::VARCHAR);
  BOOST_TEST(col->IsNullable() == false);
  BOOST_TEST(col->GetMaxLength() == 100);
  BOOST_TEST(col->GetInitVal() == -1);
  BOOST_TEST(col->GetIncStep() == -1);
  BOOST_TEST(col->GetCharset() == Charsets::UTF8);
  BOOST_TEST(col->GetComments() == "varchar test");
  BOOST_TEST(*(const DataValueVarChar *)col->GetDefaultVal() ==
             DataValueVarChar(var_default, (uint32_t)strlen(var_default)));

  col = ptable2.GetColumn("c3");
  BOOST_TEST(col == ptable2.GetColumn(2));
  BOOST_TEST(col->GetName() == "c3");
  BOOST_TEST(col->GetIndex() == 2);
  BOOST_TEST(col->GetDataType() == DataType::DOUBLE);
  BOOST_TEST(col->IsNullable() == true);
  BOOST_TEST(col->GetMaxLength() == 0);
  BOOST_TEST(col->GetInitVal() == -1);
  BOOST_TEST(col->GetIncStep() == -1);
  BOOST_TEST(col->GetCharset() == Charsets::UTF8);
  BOOST_TEST(col->GetComments() == "double column test");
  BOOST_TEST(col->GetDefaultVal()->GetDouble() == 1.23456);

  col = ptable2.GetColumn("c4");
  BOOST_TEST(col == ptable2.GetColumn(3));
  BOOST_TEST(col->GetName() == "c4");
  BOOST_TEST(col->GetIndex() == 3);
  BOOST_TEST(col->GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(col->IsNullable() == true);
  BOOST_TEST(col->GetMaxLength() == 0);
  BOOST_TEST(col->GetInitVal() == -1);
  BOOST_TEST(col->GetIncStep() == -1);
  BOOST_TEST(col->GetCharset() == Charsets::UTF8);
  BOOST_TEST(col->GetComments() == "Fixchar test");
  BOOST_TEST(col->GetDefaultVal()->GetDataType() == DataType::FIXCHAR);

  const MVector<IndexProp> &vct_ip2 = ptable2.GetVectorIndex();
  BOOST_TEST(vct_ip2.size() == 3);
  BOOST_TEST(vct_ip2[0]._name == PRIMARY_KEY);
  BOOST_TEST(vct_ip2[0]._position == 0);
  BOOST_TEST(vct_ip2[0]._type == IndexType::PRIMARY);
  BOOST_TEST(vct_ip2[0]._vctCol.size() == 1);
  BOOST_TEST(vct_ip2[0]._vctCol[0].colName == "c1");
  BOOST_TEST(vct_ip2[0]._vctCol[0].colPos == 0);

  BOOST_TEST(vct_ip2[1]._name == "c2_unique");
  BOOST_TEST(vct_ip2[1]._position == 1);
  BOOST_TEST(vct_ip2[1]._type == IndexType::UNIQUE);
  BOOST_TEST(vct_ip2[1]._vctCol.size() == 1);
  BOOST_TEST(vct_ip2[1]._vctCol[0].colName == "c2");
  BOOST_TEST(vct_ip2[1]._vctCol[0].colPos == 0);

  BOOST_TEST(vct_ip2[2]._name == "c3_c4_non_unique");
  BOOST_TEST(vct_ip2[2]._position == 2);
  BOOST_TEST(vct_ip2[2]._type == IndexType::NON_UNIQUE);
  BOOST_TEST(vct_ip2[2]._vctCol.size() == 2);
  BOOST_TEST(vct_ip2[2]._vctCol[0].colName == "c3");
  BOOST_TEST(vct_ip2[2]._vctCol[0].colPos == 0);
  BOOST_TEST(vct_ip2[2]._vctCol[1].colName == "c4");
  BOOST_TEST(vct_ip2[2]._vctCol[1].colPos == 1);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
