#pragma once
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../core/IndexTree.h"
#include "../core/IndexType.h"
#include "../table/Column.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include "Column.h"
#include <any>

namespace storage {
using namespace std;
static const char *COLUMN_CONNECTOR_CHAR = "|";
static const char *PRIMARY_KEY = "PARMARYKEY";

class BaseTable {
public:
  BaseTable() {}
  BaseTable(const MString &name, const MString &alias, const MString &desc)
      : _name(name), _alias(alias), _desc(desc) {}
  virtual ~BaseTable() {}
  inline const MString &GetTableName() const { return _name; }
  inline void SetTableName(MString name) {
    IsValidName(name);
    _name = name;
  }
  inline const MString GetTableAlias() { return _alias; }
  inline void SetTableAlias(MString alias) {
    IsValidName(alias);
    _alias = alias;
  }
  inline const MString &GetDescription() const { return _desc; }
  inline void SetTableDesc(const MString &desc) { _desc = desc; }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /**Table name*/
  MString _name;
  /**Table alias*/
  MString _alias;
  /**Table describer*/
  MString _desc;
};

class TempTable : public BaseTable {};

class HashTable : public BaseTable {};
} // namespace storage
