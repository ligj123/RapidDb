#pragma once
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include <regex>

namespace storage {
using namespace std;

class FileVersion {
public:
  FileVersion() : _majorVer(0), _minorVer(0), _patchVer(0) {}
  FileVersion(int16_t majorVer, uint8_t minorVer, uint8_t patchVer)
      : _majorVer(majorVer), _minorVer(minorVer), _patchVer(patchVer) {}

  void ParseVersion(MString text) {
    regex reg("([0-9]+)\\.([0-9]+)\\.([0-9]+)");
    cmatch mt;
    if (!regex_match(text.c_str(), mt, reg)) {
      throw ErrorMsg(TB_INVALID_FILE_VERSION, {text});
    }

    _majorVer = std::stoi(mt[1]);
    _minorVer = std::stoi(mt[2]);
    _patchVer = std::stoi(mt[3]);
  }

  short GetMajorVersion() { return _majorVer; }

  void SetMajorVersion(short ver) { _majorVer = ver; }

  uint8_t GetMinorVersion() { return _minorVer; }

  void SetMinorVersion(uint8_t ver) { _minorVer = ver; }

  uint8_t GetPatchVersion() { return _patchVer; }

  void GetPatchVersion(uint8_t ver) { _patchVer = ver; }

  bool operator==(const FileVersion &fv) const {
    return _majorVer == fv._majorVer && _minorVer == fv._minorVer &&
           _patchVer == fv._patchVer;
  }

protected:
  uint16_t _majorVer;
  uint8_t _minorVer;
  uint8_t _patchVer;

  friend std::ostream &operator<<(std::ostream &os, const FileVersion &fv);
};

inline std::ostream &operator<<(std::ostream &os, const FileVersion &fv) {
  os << "MajerVer: " << fv._majorVer << "\tMinorVer: " << fv._minorVer
     << "\tPatchVer:" << fv._patchVer;
  return os;
}
static FileVersion CURRENT_FILE_VERSION = {1, 0, 0};
} // namespace storage
