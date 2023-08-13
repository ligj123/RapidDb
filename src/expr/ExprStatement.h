#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../table/Table.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprAggr.h"
#include "ExprData.h"
#include "ExprLogic.h"
#include <unordered_set>

using namespace std;
namespace storage {
enum class TableType {
  PersistTable, // From physical table
  CacheTable,   // The result saved in temp block table
  HashTable     // The result saved in hash table
};

enum class JoinType { INNER_JOIN, LEFT_JOIN, RIGHT_JOIN, OUTTER_JOIN };

// To query physical table, point out which index will be used. Only one index
// can be selected.
struct SelIndex {
  // If the primary or secondary index can be used to query, copy the query
  // conditions to here. Only one index can be used. Only valid for physical
  // table select
  ExprLogic *_indexExpr;
  // which index used, The position of index that start from 0(primary key)
  int _indexPos;
};

template <ExprType ET> class ExprCondition : public BaseExpr {
public:
  ExprCondition(ExprLogic *exprLogic) : _exprLogic(exprLogic) {}
  ~ExprCondition() { delete _exprLogic; }
  ExprType GetDataType() const { return ET; }

  TriBool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_exprLogic == nullptr)
      return TriBool::True;
    return _exprLogic->Calc(vdPara, vdRow);
  }

public:
  ExprLogic *_exprLogic;
};

typedef ExprCondition<ExprType::EXPR_WHERE> ExprWhere;
typedef ExprCondition<ExprType::EXPR_WHERE> ExprOn;
typedef ExprCondition<ExprType::EXPR_WHERE> ExprHaving;

struct GroupItem {
  MString _colName; // The column name, from sql statement
  int _pos;         // The position of column in result set
};

class ExprGroupBy : public BaseExpr {
public:
  ExprGroupBy(vector<GroupItem> vct) { _vctItem.swap(vct); }

public:
  vector<GroupItem> _vctItem;
};

struct OrderTerm {
  MString _colName;
  bool _direct; // True: ASC; False: DESC
  int _pos;
};

class ExprOrderBy : public BaseExpr {
public:
  ExprOrderBy(vector<OrderTerm> &vct) { _vctItem.swap(vct); }

public:
  vector<OrderTerm> _vctItem;
};

// Base class for all statement
class ExprStatement : public BaseExpr {
public:
  ExprStatement(VectorDataValue *paraTmpl) : _paraTmpl(paraTmpl) {}
  ~ExprStatement() { delete _paraTmpl; }
  const VectorDataValue *GetParaTemplate() const { return _paraTmpl; }
  bool Preprocess() = 0;

public:
  // The template for paramters, only need by top statement
  VectorDataValue *_paraTmpl;
};

// Base class for all select
class ExprSelect : public ExprStatement {
public:
  ExprSelect(ExprTable *destTable, ExprLogic *where,
             MVector<OrderCol> *vctOrder, bool bDistinct, int offset,
             int rowCount, bool bCacheResult, VectorDataValue *paraTmpl,
             ExprStatement *parent)
      : ExprStatement(paraTmpl), _destTable(destTable), _where(where),
        _vctOrder(vctOrder), _offset(offset), _rowCount(rowCount),
        _bDistinct(bDistinct), _bCacheResult(bCacheResult), _parent(parent) {}
  ~ExprSelect() {
    delete _destTable;
    delete _where;
    delete _vctOrder;
  }

  ExprTable *GetDestTable() { return _destTable; }
  ExprLogic *GetWhere() { return _where; }
  MVector<OrderCol> *GetVctOrder() { return _vctOrder; }
  int GetOffset() { return _offset; }
  int GetRowCount() { return _rowCount; }
  bool IsDistinct() { return _bDistinct; }
  bool IsCacheResult() { return _bCacheResult; }

  ExprStatement *GetParent() { return _parent; }

public:
  // Parent statement for this select statement
  ExprStatement *_parent;
  ExprTable *_destTable; // The columns of selected result
  ExprLogic *_where;     // The where conditions
  // The columns idx and order type for order by, NO order with nullptr
  MVector<OrderCol> *_vctOrder;
  // offset for return rows, default 0. Only valid for top select result.
  int _offset;
  // The max rows to return, -1 means return all. Only valid for top select
  // result.
  int _rowCount;
  bool _bDistinct; // If remove repeated rows
  // If cache result for future query, only valid for top select result.
  bool _bCacheResult;
};

class ExprCreateDatabase : public BaseExpr {
public:
  ExprCreateDatabase(MString name, bool ifNotExist)
      : _dbName(name), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_CREATE_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
}

class ExprDropDatabase : public BaseExpr {
public:
  ExprDropDatabase(MString name, bool ifNotExist)
      : _dbName(name), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_DROP_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
}

class ExprCreateTable : public BaseExpr {
public:
public:
}

class ExprTableSelect : public ExprSelect {
public:
  ExprTableSelect(PhysTable *physTable, ExprTable *destTable, ExprLogic *where,
                  SelIndex *selIndex, MVector<OrderCol> *vctOrder,
                  bool bDistinct, int offset, int rowCount, bool bCacheResult,
                  VectorDataValue *paraTmpl, ExprStatement *parent)
      : ExprSelect(destTable, where, vctOrder, bDistinct, offset, rowCount,
                   bCacheResult, paraTmpl, parent),
        _physTable(physTable), _selIndex(selIndex) {
    _physTable->IncRef();
  }
  ~ExprTableSelect() {
    _physTable->DecRef();
    delete _selIndex;
  }

  ExprType GetType() { return ExprType::EXPR_TABLE_SELECT; }
  PhysTable *GetSourTable() const { return _physTable; }
  const SelIndex *GetSelIndex() const { return _selIndex; }

public:
  // The source table information.
  PhysTable *_physTable;
  // Which index used to search. Null means traverse all table.
  SelIndex *_selIndex;
};

class ExprInsert : public ExprStatement {
public:
  ExprInsert(PhysTable *physTable, ExprTable *exprTable,
             VectorDataValue *paraTmpl, ExprSelect *exprSelect = nullptr,
             bool bUpsert = false)
      : ExprStatement(paraTmpl), _physTable(physTable), _exprTable(exprTable),
        _exprSelect(exprSelect), _bUpsert(bUpsert) {
    _physTable->IncRef();
  }

  ~ExprInsert() {
    _physTable->DecRef();
    delete _exprTable;
    delete _exprSelect;
  }

  ExprType GetType() { return ExprType::EXPR_INSERT; }
  PhysTable *GetSourTable() const { return _physTable; }
  const ExprTable *GetExprTable() { return _exprTable; }
  const ExprSelect *GetExprSelect() { return _exprSelect; }

  bool IsUpsert() { return _bUpsert; }

public:
  // The destion physical table information
  PhysTable *_physTable;
  // The expression that how to calc values from input or select
  ExprTable *_exprTable;
  // Used for insert into TABLE A select from TABLE B
  ExprSelect *_exprSelect;
  // True, update if the key has exist
  bool _bUpsert;
};

class ExprUpdate : public ExprStatement {
public:
  ExprUpdate(PhysTable *physTable, ExprTable *exprTable, ExprLogic *where,
             SelIndex *selIndex, VectorDataValue *paraTmpl)
      : ExprStatement(paraTmpl), _physTable(physTable), _exprTable(exprTable),
        _where(where), _selIndex(selIndex) {
    _physTable->IncRef();
  }

  ~ExprUpdate() {
    _physTable->DecRef();
    delete _exprTable;
    delete _where;
  }

  ExprType GetType() { return ExprType::EXPR_UPDATE; }
  PhysTable *GetSourTable() const { return _physTable; }
  const ExprTable *GetExprTable() { return _exprTable; }
  const ExprLogic *GetWhere() { return _where; }
  const SelIndex *GetSelIndex() const { return _selIndex; }

public:
  // The destion physical table information
  PhysTable *_physTable;
  // The expression that how to calc values
  ExprTable *_exprTable;
  // Where condition
  ExprLogic *_where;
  // Which index used to search. Null means traverse all table.
  SelIndex *_selIndex;
};

class ExprDelete : public ExprStatement {
public:
  ExprDelete(PhysTable *physTable, ExprLogic *where, SelIndex *selIndex,
             VectorDataValue *paraTmpl)
      : ExprStatement(paraTmpl), _physTable(physTable), _where(where),
        _selIndex(selIndex) {
    _physTable->IncRef();
  }
  ~ExprDelete() {
    _physTable->DecRef();
    delete _where;
    delete _paraTmpl;
  }

  ExprType GetType() { return ExprType::EXPR_DELETE; }
  PhysTable *GetSourTable() const { return _physTable; }
  const ExprLogic *GetWhere() { return _where; }
  const SelIndex *GetSelIndex() const { return _selIndex; }

public:
  // The destion persistent table information
  PhysTable *_physTable;
  // Where condition
  ExprLogic *_where;
  // Which index used to search. Null means traverse all table.
  SelIndex *_selIndex;
};

// class ExprJoin : public ExprSelect {
// public:
//   ExprJoin(BaseTable *sourTable, BaseTable *destTable,
//            ExprValueArrayOut *exprVAout, ExprCondition *where,
//            MVector<OrderCol> &vctOrder, bool bDistinct, bool
//            bCacheResult, BaseTable *rightTable, ExprOn *exprOn, JoinType
//            jType, VectorDataValue &vctPara)
//       : ExprSelect(sourTable, destTable, exprVAout, where, vctOrder,
//       bDistinct,
//                    bCacheResult, vctPara),
//         _rightTable(rightTable), _exprOn(exprOn), _joinType(jType) {}
//   ~ExprJoin() {
//     delete _rightTable;
//     delete _exprOn;
//   }

//   ExprType GetType() { return ExprType::EXPR_JOIN; }
//   const BaseTable *GetRightTable() const { return _rightTable; }
//   const ExprOn *GetExprOn() const { return _exprOn; }
//   JoinType GetJoinType() { return _joinType; }

// protected:
//   // The right source table information for join
//   BaseTable *_rightTable;
//   // On condition for join
//   ExprOn *_exprOn;
//   // Join type
//   JoinType _joinType;
// };

// class ExprGroupBy : public ExprSelect {
// public:
//   ExprGroupBy(BaseTable *sourTable, BaseTable *destTable,
//               MVector<ExprAggr *> &vctAggr, ExprCondition *where,
//               MVector<OrderCol> &vctOrder, bool bDistinct,
//               bool bCacheResult, ExprCondition *having,
//               VectorDataValue &vctPara)
//       : ExprSelect(sourTable, destTable, nullptr, where, vctOrder, bDistinct,
//                    bCacheResult, vctPara),
//         _having(having) {
//     _vctChild.swap(vctAggr);
//   }
//   ~ExprGroupBy() {
//     for (auto child : _vctChild)
//       delete child;

//     _vctChild.clear();
//     delete _having;
//   }

//   ExprType GetType() { return ExprType::EXPR_GROUP_BY; }
//   MVector<ExprAggr *> GetVctChild() { return _vctChild; }
//   ExprCondition *GetHaving() { return _having; }

// protected:
//   // This variable will replace _exprVAout to calc the values.
//   MVector<ExprAggr *> _vctChild;
//   ExprCondition *_having;
// };
} // namespace storage
