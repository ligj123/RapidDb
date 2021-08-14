#include "BaseExpr.h"
#include <vector>

using namespace std;
namespace storage {
struct FuncStru {
  string _funcName;
  vector<DataType> _parasType;
  DataType _rtType;
}
/*
 * @brief The parent class for all function expression.
 */
class ExprFunc : public BaseExpr {
public:
  ExprFunc(std::initializer_list<BaseExpr *> paras)
      : BaseExpr(ExprType::EXPR_FUNCTION), _vctPara(paras) {}
  ~ExprFunc() {
    for (auto iter : _vctPara)
      delete iter;
  }

protected:
  // The function name, must convert to upper case
  string _funcName;
  // The parameters for this function
  vector<BaseExpr *> _vctPara;
};
} // namespace storage