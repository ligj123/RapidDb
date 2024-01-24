#include "../../src/file/PageFile.h"
#include "../../src/config/Configure.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_AUTO_TEST_SUITE(PageFileTest)

BOOST_AUTO_TEST_CASE(PageFile_test) {
  namespace fs = std::filesystem;
  const string pageName = ROOT_PATH + "/testPageFile" + StrMSTime() + ".dat";
  const string ovfName = ROOT_PATH + "/testOverflow" + StrMSTime() + ".dat";

  uint32_t pageLen = (uint32_t)Configure::GetIndexPageSize();
  string nofile = "./unexistdir/filename";
  PageFile errFile(nofile.c_str());
  BOOST_TEST(errFile.IsValid() == false);
  LOG_INFO << _threadErrorMsg->what();

  fs::path path(ROOT_PATH);
  if (!fs::exists(path))
    fs::create_directories(path);
  PageFile pf(pageName.c_str());

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

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
