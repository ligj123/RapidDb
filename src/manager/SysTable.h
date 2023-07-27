#pragma once
#include "../table/Table.h"

namespace storage {
using namespace std;

class SysTable {
public:
protected:
  PhysTable *_schema;
};
} // namespace storage