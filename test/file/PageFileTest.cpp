#include "../../src/file/PageFile.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;
string rootPath = "./dbTest";
string pageName = rootPath + "/testPageFile" + utils::StrMSTime() + ".dat";
string ovfName = rootPath + "/testOverflow" + utils::StrMSTime() + ".dat";

BOOST_AUTO_TEST_SUITE(PageFileTest)

BOOST_AUTO_TEST_CASE(PageFile_test) {
  uint32_t pageLen = (uint32_t)Configure::GetCachePageSize();
  try {
    string nofile = "./unexistdir/filename";
    PageFile errFile(nofile);
    throw "Here should have an exception!";
  } catch (utils::ErrorMsg &msg) {
    LOG_INFO << msg.what();
  }

  fs::path path(rootPath);
  if (!fs::exists(path))
    fs::create_directories(path);
  PageFile pf(pageName);

  char *bys1 = new char[pageLen];
  char *bys2 = new char[pageLen];
  for (uint32_t i = 0; i < pageLen; i++)
    bys1[i] = (char)i;

  pf.WritePage(0, bys1, pageLen);
  pf.WritePage(pageLen, bys1, pageLen);
  pf.WritePage(pageLen * 2, bys1, pageLen);
  pf.WritePage(pageLen * 3, bys1, pageLen);

  pf.ReadPage(pageLen, bys2, pageLen);
  BOOST_TEST(memcmp(bys1, bys2, pageLen) == 0);

  for (uint32_t i = 0; i < pageLen; i++)
    bys1[i] = (char)(i + 10);
  pf.WritePage(pageLen * 2, bys1, pageLen);
  pf.ReadPage(pageLen * 2, bys2, pageLen);
  BOOST_TEST(memcmp(bys1, bys2, pageLen) == 0);
  pf.close();
  fs::remove(fs::path(pageName));

  delete[] bys1;
  delete[] bys2;
}

BOOST_AUTO_TEST_CASE(OverflowFile_test) {
  PageFile pf(ovfName, true);
  vector<IDataValue *> vctDv;
  vctDv.push_back(DataValueFactory(DataType::LONG, false, 0, 0x12345678));
  vctDv.push_back(
      DataValueFactory(DataType::FIXCHAR, false, 100, "abcdefghigklmn"));
  vctDv.push_back(DataValueFactory(DataType::VARCHAR, false, 100,
                                   "1234567890abcdefghijklmn"));
  uint32_t len = 0;
  for (IDataValue *dv : vctDv) {
    len += dv->GetPersistenceLength();
  }
  pf.WriteDataValue(vctDv, 0, 0);
  pf.close();

  PageFile pf2(ovfName, true);
  vector<IDataValue *> vctDv2;
  vctDv2.push_back(DataValueFactory(DataType::LONG, false, 0));
  vctDv2.push_back(DataValueFactory(DataType::FIXCHAR, false, 100));
  vctDv2.push_back(DataValueFactory(DataType::VARCHAR, false, 100));
  pf2.ReadDataValue(vctDv2, 0, 0, len);
  pf2.close();

  for (int i = 0; i < 3; i++) {
    BOOST_TEST(*vctDv[i] == *vctDv2[i]);
    delete vctDv[i];
    delete vctDv2[i];
  }

  fs::remove(fs::path(ovfName));
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage