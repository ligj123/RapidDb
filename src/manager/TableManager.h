#include "../cache/Mallocator.h"
#include "../table/Table.h"

namespace storage {
using namespace std;

class TableManager {
public:
  static bool InitTable(PhysTable *sysTable);
  static bool AddTable(const MString &tblName, PhysTable *table);
  static bool RemoveTable(const MString &tblName);
  static bool FindTable(const MString &tblName, PhysTable *&tbl);
  static bool ListTables(const MString &dbName, MVector<MString> &vctTbl);

protected:
  static MStrTreeMap<PhysTable *> _mapTable;
  static SpinMutex _spinMutex;
  static vector<PhysTable *> _fastTableCache;
  // To temporary save the dropped PhysTable.
  static vector<PhysTable *> _discardTable;
  // True: all tables names will be loaded into _mapTable, the second parameter
  // will be null at first. In this way it can be ensure if a table exist.
  // False: only used table will be loaded
  static bool _bAllInMemory;
};
} // namespace storage