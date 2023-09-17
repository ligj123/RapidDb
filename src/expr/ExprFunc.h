#include "../cache/Mallocator.h"
#include "BaseExpr.h"

using namespace std;
namespace storage {
struct FuncStru {
  MString _funcName;
  MVector<DataType> _parasType;
  DataType _rtType;
};
} // namespace storage
