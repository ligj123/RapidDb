#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataType.h"
#include "../table/Table.h"

using namespace std;
namespace storage {
class SysTable {
public:
  /**
   * @brief Initialize the system table when first start. It will create a
   * folder for database and create system tables.
   */
  static bool InitSystemTable();
  /**
   * @brief Generate system tables
   */
  static bool GenerateSysTables(Database *sysDb,
                                MStrHashMap<PhysTable *> &hmap);

  static bool LoadSystemTable();
};
} // namespace storage