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

enum class TableType {
  PersistTable, // From physical table
  CacheTable,   // The result saved in temp block table
  HashTable     // The result saved in hash table
};

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

} // namespace storage