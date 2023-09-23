#pragma once
#include "BytesFuncs.h"
#include <boost/locale/encoding.hpp>
#include <cstring>
#include <iostream>
#include <unordered_map>

namespace storage {
using namespace std;
enum class Charsets : uint16_t { UNKNOWN = 0, UTF8, UTF16, UTF32, GBK };

inline std::ostream &operator<<(std::ostream &os, const Charsets &vt) {
  switch (vt) {
  case Charsets::UNKNOWN:
    os << "UNKNOWN(" << (int)Charsets::UNKNOWN << ")";
    break;
  case Charsets::UTF8:
    os << "UTF8(" << (int)Charsets::UTF8 << ")";
    break;
  case Charsets::UTF16:
    os << "UTF16(" << (int)Charsets::UTF16 << ")";
    break;
  case Charsets::UTF32:
    os << "UTF32(" << (int)Charsets::UTF32 << ")";
    break;
  case Charsets::GBK:
    os << "GBK(" << (int)Charsets::GBK << ")";
    break;
  }

  return os;
}

enum class ConvResult { OK = 0, PARTIAL, ERROR, IGNORE };

class CodeConvert {
public:
  static ConvResult AsciiToUTF8(const char *inFrom, const int inLen,
                                char *outFrom, int &outLen,
                                const Charsets cset = Charsets::GBK,
                                const bool bZero = true) {
    string str = boost::locale::conv::between(inFrom, inFrom + inLen,
                                              mapCharset[Charsets::UTF8],
                                              mapCharset[cset])
                     .c_str();
    if (str.size() > outLen) {
      BytesCopy(outFrom, str.c_str(), outLen);
      return ConvResult::PARTIAL;
    }

    BytesCopy(outFrom, str.c_str(), str.size());
    if (bZero) {
      std::memset(outFrom + str.size(), 0, outLen - str.size());
    }
    outLen = (int)str.size();
    return ConvResult::OK;
  }

  static ConvResult AsciiFromUTF8(const char *inFrom, const int inLen,
                                  char *outFrom, int &outLen,
                                  const Charsets cset = Charsets::GBK,
                                  const bool bZero = true) {
    string str =
        boost::locale::conv::between(inFrom, inFrom + inLen, mapCharset[cset],
                                     mapCharset[Charsets::UTF8])
            .c_str();
    if (str.size() > outLen) {
      BytesCopy(outFrom, str.c_str(), outLen);
      return ConvResult::PARTIAL;
    }

    BytesCopy(outFrom, str.c_str(), str.size());
    if (bZero) {
      std::memset(outFrom + str.size(), 0, outLen - str.size());
    }
    outLen = (int)str.size();
    return ConvResult::OK;
  }

  static ConvResult MultiCharToWideChar(const char *inFrom, const int inLen,
                                        wchar_t *outFrom, int &outLen,
                                        const Charsets cset = Charsets::GBK,
                                        const bool bZero = true) {
    static const int WCHAR_SIZE = sizeof(wchar_t);
    wstring str = boost::locale::conv::to_utf<wchar_t>(inFrom, inFrom + inLen,
                                                       mapCharset[cset]);
    if (str.size() > outLen) {
      BytesCopy(outFrom, str.c_str(), (size_t)outLen * WCHAR_SIZE);
      return ConvResult::PARTIAL;
    }

    BytesCopy(outFrom, str.c_str(), str.size() * WCHAR_SIZE);
    if (bZero) {
      std::memset((char *)outFrom + str.size() * WCHAR_SIZE, 0,
                  (outLen - str.size()) * WCHAR_SIZE);
    }
    outLen = (int)str.size();
    return ConvResult::OK;
  }

  static ConvResult MultiCharFromWideChar(const wchar_t *inFrom,
                                          const int inLen, char *outFrom,
                                          int &outLen,
                                          const Charsets cset = Charsets::GBK,
                                          const bool bZero = true) {
    string str =
        boost::locale::conv::from_utf(inFrom, inFrom + inLen, mapCharset[cset]);
    if (str.size() > outLen) {
      BytesCopy(outFrom, str.c_str(), outLen);
      return ConvResult::PARTIAL;
    }

    BytesCopy(outFrom, str.c_str(), str.size());
    if (bZero) {
      std::memset(outFrom + str.size(), 0, outLen - str.size());
    }
    outLen = (int)str.size();
    return ConvResult::OK;
  }

protected:
  static unordered_map<Charsets, string> mapCharset;
};

ostream &operator<<(ostream &os, const ConvResult &rst);

} // namespace storage
