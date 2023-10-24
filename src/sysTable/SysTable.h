#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataType.h"
#include "../table/Table.h"

using namespace std;
namespace storage {
class SysTable {
public:
  static bool CreateSystemTable();
  static bool GenerateSysTables(MHashMap<MString, PhyTable *> &hmap);
};
} // namespace storage