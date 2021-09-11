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

struct IndexProp {
  IndexType type;
  struct Column {
    string colName;
    int colPos = 0;
  };
  MVector<Column>::Type vctCol;
};

class BaseTableDesc {
public:
  BaseTableDesc() {}
  BaseTableDesc(const string &name, const string &desc)
      : _name(name), _desc(desc) {}
  virtual ~BaseTableDesc() {}
  inline const string GetTableName() const { return _name; }
  inline void SetTableName(string name) {
    IsValidName(name);
    _name = name;
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
  /**Table describer*/
  string _desc;
};

class PersistTableDesc : public BaseTableDesc {
public:
  PersistTableDesc(string tableName, string description)
      : BaseTableDesc(tableName, description){};
  PersistTableDesc(){};
  ~PersistTableDesc();

  const string GetPrimaryKey() const { return PRIMARY_KEY; }
  const IndexProp &GetPrimaryKeyColumns() const {
    return _mapIndex.at(PRIMARY_KEY);
  }
  const MHashMap<string, IndexProp>::Type &GetMapIndex() const {
    return _mapIndex;
  }
  IndexType GetIndexType(string indexName) const {
    auto iter = _mapIndex.find(indexName);
    if (iter == _mapIndex.end())
      return IndexType::UNKNOWN;
    return iter->second.type;
  }

  const MVector<PersistColumn *>::Type &GetColumnArray() const {
    return _vctColumn;
  }

  const PersistColumn *GetColumn(string fieldName) const;
  const PersistColumn *GetColumn(int pos);
  MHashMap<string, int>::Type GetMapColumnPos() { return _mapColumnPos; }
  MHashMap<int, string>::Type GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  void AddColumn(string &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, string &comment, Charsets charset,
                 any &valDefault);
  void SetPrimaryKey(MVector<string>::Type priCols);

  void AddSecondaryKey(string indexName, IndexType indexType,
                       MVector<string>::Type colNames);
  static PersistTableDesc *ReadData(string rootPath, string dbName,
                                    string tblName);
  void WriteData(string rootPath, string dbName);

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }

protected:
  //
  MVector<TempColumn *>::Type _vctColm;
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  MVector<PersistColumn *>::Type _vctColumn;
  /** The map for column name and their position in column list */
  MHashMap<string, int>::Type _mapColumnPos;
  /**The first parameter is the unique name for a index and the primary key's
  name is fixed, must be PRIMARY_KEY. The second parameter is IndexProp.*/
  MHashMap<string, IndexProp>::Type _mapIndex;
  /**The first column position to constitute the index;
  The second parameter,  the unique name for a index;*/
  MHashMap<int, string>::Type _mapIndexFirstField;
};

class TempTableDesc : public BaseTableDesc {};

class HashTableDesc : public BaseTableDesc {};
} // namespace storage
