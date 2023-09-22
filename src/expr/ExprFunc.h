#include "../cache/Mallocator.h"
#include "ExprData.h"

using namespace std;
namespace storage {
struct FuncStru {
  MString _funcName;
  MVector<DataType> _parasType;
  DataType _rtType;
};

/*
 * @brief The function expression.
 */
class ExprFunc : public ExprData {
public:
  ExprFunc(MString *funcName, MVectorPtr<ExprData *> *paras)
      : _funcName(funcName), _vctPara(paras) {}
  ~ExprFunc() {
    delete _funcName;
    delete _vctPara;
  }

  ExprType GetType() override { return ExprType::EXPR_FUNCTION; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    return nullptr;
  }

protected:
  // The function name, must convert to upper case
  MString *_funcName;
  // The parameters for this function
  MVectorPtr<ExprData *> *_vctPara;
};
} // namespace storage
