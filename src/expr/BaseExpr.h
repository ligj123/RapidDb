#pragma once
namespace storage {
class BaseExpr {
public:
protected:
};

class ValueExpr : BaseExpr {};
class ParameterExpr : BaseExpr {};

class ColumnExpr : BaseExpr {};

class FunctionExpr : BaseExpr {};

class OperatorExpr : BaseExpr {};

class SelectExpr : BaseExpr {};

class ArrayExpr : BaseExpr {};

class CaseExpr : BaseExpr {};
} // namespace storage
