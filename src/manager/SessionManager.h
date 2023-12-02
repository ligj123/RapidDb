#include "../cache/Mallocator.h"
#include "../serv/Session.h"

namespace storage {
using namespace std;

class TableManager {
public:
protected:
  static MTreeMap<uint32_t, Session *> _mapTable;
  static SpinMutex _spinMutex;
  static vector<Session *> _fastTableCache;
  // To temporary save the dropped PhysTable.
  static vector<Session *> _discardTable;
};
} // namespace storage