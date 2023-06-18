#include "../../src/table/Column.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/utils/Utilitys.h"
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <filesystem>
namespace storage {
namespace fs = std::filesystem;
BOOST_AUTO_TEST_SUITE(ColumnTest)

BOOST_AUTO_TEST_CASE(PersistColumn_test) {
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

  PhysColumn clm2;
  sz2 = clm2.ReadData(bys, sz);
  BOOST_TEST(sz == sz2);

  BOOST_TEST(strcmp("abcd", clm2.GetName().c_str()) == 0);
  BOOST_TEST(clm2.GetIndex() == 10);
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
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
