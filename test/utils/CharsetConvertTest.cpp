#include "../../src/utils/CharsetConvert.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)
//
// BOOST_AUTO_TEST_CASE(CharsetConvert_test) {
//  const char *pAsc = "\xd6\xd0\xb9\xfa\xc4\xe3\xba\xc3HelloChina!!@";
//  const char *pUtf8 = (const char *)u8"中国你好HelloChina!!@";
//
//  char tmp[100];
//  int len = 100;
//
//  ConvResult rt = CodeConvert::AsciiToUTF8(pAsc, (int)std::strlen(pAsc), tmp,
//                                           len, Charsets::GBK, true);
//  BOOST_TEST(rt == ConvResult::OK);
//  BOOST_TEST(strcmp(tmp, pUtf8) == 0);
//  BOOST_TEST(tmp[strlen(tmp) + 1] == 0);
//
//  len = 10;
//  rt = CodeConvert::AsciiToUTF8(pAsc, (int)std::strlen(pAsc), tmp, len,
//                                Charsets::GBK, false);
//  BOOST_TEST(rt == ConvResult::PARTIAL);
//  BOOST_TEST(len == 10);
//  BOOST_TEST(tmp[len] != '\0');
//
//  len = 100;
//  rt = CodeConvert::AsciiFromUTF8(pUtf8, (int)std::strlen(pUtf8), tmp, len,
//                                  Charsets::GBK, true);
//  BOOST_TEST(rt == ConvResult::OK);
//  BOOST_TEST(strcmp(tmp, pAsc) == 0);
//  BOOST_TEST(tmp[strlen(tmp) + 1] == 0);
//
//  len = 10;
//  rt = CodeConvert::AsciiFromUTF8(pUtf8, (int)std::strlen(pUtf8), tmp, len,
//                                  Charsets::GBK, false);
//  BOOST_TEST(rt == ConvResult::PARTIAL);
//  BOOST_TEST(len == 10);
//  BOOST_TEST(tmp[len] != '\0');
//
//  const wchar_t *pUncode = L"中国你好HelloChina!!@";
//  wchar_t tmp2[100];
//  len = 100;
//  rt = CodeConvert::MultiCharToWideChar(pAsc, (int)std::strlen(pAsc), tmp2,
//  len,
//                                        Charsets::GBK, true);
//  BOOST_TEST(rt == ConvResult::OK);
//  BOOST_TEST(std::wcscmp(tmp2, pUncode) == 0);
//  BOOST_TEST((uint32_t)tmp2[len] == 0);
//
//  len = 100;
//  rt = CodeConvert::MultiCharFromWideChar(pUncode, (int)wcslen(pUncode), tmp,
//                                          len, Charsets::GBK, true);
//  BOOST_TEST(rt == ConvResult::OK);
//  BOOST_TEST(strcmp(tmp, pAsc) == 0);
//  BOOST_TEST(tmp[len] == 0);
//}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
