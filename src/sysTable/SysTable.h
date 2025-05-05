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

  static bool LoadTable(PhysTable *table);

  static bool LoadSystemParameter(MString &name);

  static inline const char *SystemDbTableName() { return "rapid.sys_dbs"; }
  static inline const char *SystemTblTableName() { return "rapid.sys_tables"; }
};
} // namespace storage