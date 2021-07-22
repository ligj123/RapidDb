#include "../../src/table/Column.h"
#include "../../src/dataType/DataValueUInt.h"
#include "../../src/utils/Utilitys.h"
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <filesystem>
namespace storage {
namespace fs = std::filesystem;
BOOST_AUTO_TEST_SUITE(ColumnTest)

BOOST_AUTO_TEST_CASE(PersistColumn_test) {
  PersistColumn clm("abcd", 10, DataType::UINT, "test", false, 100, 1, 2,
                    Charsets::UNKNOWN, new DataValueUInt(20u, false));

  BOOST_TEST(strcmp("abcd", clm.GetName().c_str()) == 0);
  BOOST_TEST(clm.GetPosition() == 10);
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
  clm.WriteData(bys);

  PersistColumn clm2;
  clm2.ReadData(bys);

  BOOST_TEST(strcmp("abcd", clm2.GetName().c_str()) == 0);
  BOOST_TEST(clm2.GetPosition() == 10);
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

BOOST_AUTO_TEST_CASE(MiddleColumn_test) {
  MiddleColumn clm("abcd", 10, DataType::VARCHAR, "a-2", 10, 1, 1);
  BOOST_TEST(strcmp("abcd", clm.GetName().c_str()) == 0);
  BOOST_TEST(clm.GetPosition() == 10);
  BOOST_TEST(clm.GetDataType() == DataType::VARCHAR);
  BOOST_TEST(strcmp("a-2", clm.GetAlias().c_str()) == 0);
  BOOST_TEST(clm.GetDataBasicStart() == 10);
  BOOST_TEST(clm.GetPrevVarCols() == 1);
  BOOST_TEST(clm.GetColNullPlace() == 1);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
