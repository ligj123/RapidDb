
#include "../cache/Mallocator.h"
#include "../table/Database.h"
#include "../utils/SpinMutex.h"

namespace storage {
class DatabaseManager {
public:
  static const uint32_t FAST_SIZE;

public:
protected:
  static MHashMap<MString, Database *> _mapDb;
  static SpinMutex _spinMutex;
  static Database *_fastDbCache;
};
} // namespace storage