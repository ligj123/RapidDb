#include "../cache/Mallocator.h"
#include "BaseExpr.h"

using namespace std;
namespace storage {
struct FuncStru {
  MString _funcName;
  MVector<DataType> _parasType;
  DataType _rtType;
};
/*
 * @brief The parent class for all function expression.
 */
class ExprFunc : public BaseExpr {
public:
  ExprFunc(std::initializer_list<BaseExpr *> paras) : _vctPara(paras) {}
  ~ExprFunc() {
    for (auto iter : _vctPara)
      delete iter;
  }

  ExprType GetType() { return ExprType::EXPR_FUNCTION; }

protected:
  // The function name, must convert to upper case
  MString _funcName;
  // The parameters for this function
  MVector<BaseExpr *> _vctPara;
};
} // namespace storage
