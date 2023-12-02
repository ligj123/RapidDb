#include "../cache/Mallocator.h"
#include "../table/Table.h"

namespace storage {
using namespace std;
class TableManager {
public:
protected:
  static MStrTreeMap<PhysTable *> _mapTable;
  static SpinMutex _spinMutex;
  static vector<PhysTable *> _fastTableCache;
  // To temporary save the dropped PhysTable.
  static vector<PhysTable *> _discardTable;
};
} // namespace storage