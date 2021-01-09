#pragma once
#include <string>
#include <regex>
#include "../utils/ErrorMsg.h"
#include "../config/ErrorID.h"

namespace storage {
  using namespace utils;
  using namespace std;

  class FileVersion {
  public:
    FileVersion() 
      : _majorVer(0), _minorVer(0), _patchVer(0)
    { }
    FileVersion(int16_t majorVer, uint8_t minorVer, uint8_t patchVer)
      : _majorVer(majorVer), _minorVer(minorVer), _patchVer(patchVer)
    { }

    void ParseVersion(string text)
    {
      regex reg("([0-9]+)\\.([0-9]+)\\.([0-9]+)");
      cmatch mt;
      if (!regex_match(text.c_str(), mt, reg)) {
        throw ErrorMsg(TB_INVALID_FILE_VERSION, { text });
      }

      _majorVer = std::stoi(mt[1]);
      _minorVer = std::stoi(mt[2]);
      _patchVer = std::stoi(mt[3]);
    }

    short GetMajorVersion() {
      return _majorVer;
    }

    void SetMajorVersion(short ver) {
      _majorVer = ver;
    }

    uint8_t GetMinorVersion() {
      return _minorVer;
    }

    void SetMinorVersion(uint8_t ver) {
      _minorVer = ver;
    }

    uint8_t GetPatchVersion() {
      return _patchVer;
    }

    void GetPatchVersion(uint8_t ver) {
      _patchVer = ver;
    }

    bool operator=(FileVersion fv) {
      return _majorVer == fv._majorVer &&
        _minorVer == fv._minorVer &&
        _patchVer == fv._patchVer;
    }
  protected:
    uint16_t _majorVer;
    uint8_t _minorVer;
    uint8_t _patchVer;
  };

  static FileVersion INDEX_FILE_VERSION = { 1, 1, 0 };
}