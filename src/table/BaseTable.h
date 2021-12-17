#pragma once
#include "../config/ErrorID.h"
#include "../core/IndexTree.h"
#include "../core/IndexType.h"
#include "../table/Column.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include "Column.h"
#include <any>
#include <string>

namespace storage {
using namespace std;
static const char *COLUMN_CONNECTOR_CHAR = "|";
static const char *PRIMARY_KEY = "PARMARYKEY";

class BaseTable {
public:
  BaseTable() {}
  BaseTable(const string &name, const string &alias, const string &desc)
      : _name(name), _alias(alias), _desc(desc) {}
  virtual ~BaseTable() {}
  inline const string &GetTableName() const { return _name; }
  inline void SetTableName(string name) {
    IsValidName(name);
    _name = name;
  }
  inline const string GetTableAlias() { return _alias; }
  inline void SetTableAlias(string alias) {
    IsValidName(alias);
    _alias = alias;
  }
  inline const string &GetDescription() const { return _desc; }
  inline void SetTableDesc(const string &desc) { _desc = desc; }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /**Table name*/
  string _name;
  /**Table alias*/
  string _alias;
  /**Table describer*/
  string _desc;
};

class TempTable : public BaseTable {};

class HashTable : public BaseTable {};
} // namespace storage
