#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataType.h"

using namespace std;
namespace storage {
struct SysColumn {

  MString _name;
  DataType _dtType;
  int _maxLength;
};

class Systable {
public:

protected:
  MVector<SysColumn> _vctColumn;
};
} // namespace storage