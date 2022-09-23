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

struct IndexColumn {
  string colName;
  uint16_t colPos = 0;
};

struct IndexProp {
  IndexProp() : _position(UINT16_MAX), _type(IndexType::UNKNOWN) {}
  IndexProp(string &name, uint16_t pos, IndexType type,
            MVector<IndexColumn>::Type &vctCol)
      : _name(name), _position(pos), _type(type) {
    _vctCol.swap(vctCol);
  }

  void Write(Byte *bys, uint32_t &bysLen);
  bool Read(Byte *bys);
  uint32_t CalcSize();

  // Index name
  string _name;
  // The position of this index, start from 0 and primary key must be 0. Table
  // id + this position will be the file id in IndxTree.
  uint16_t _position;
  // This index is primary key, unique key or nonunique key.
  IndexType _type;
  // The columns that composit this index
  MVector<IndexColumn>::Type _vctCol;
  // Index tree,
  IndexTree *_tree;
};

class PhysTable : public BaseTable {
public:
  PhysTable(string &rootPath, string &dbName, string &tableName,
            string &tableAlias, string &description);
  PhysTable(){};
  ~PhysTable();

  const char *GetPrimaryName() const { return PRIMARY_KEY; }
  const IndexProp *GetPrimaryKey() const { return _vctIndex[0]; }
  const MVector<IndexProp *>::Type &GetVectorIndex() const { return _vctIndex; }
  IndexType GetIndexType(string &indexName) const {
    auto iter = _mapIndexNamePos.find(indexName);
    if (iter == _mapIndexNamePos.end())
      return IndexType::UNKNOWN;
    return _vctIndex[iter->second]->_type;
  }

  const MVector<PhysColumn *>::Type &GetColumnArray() const {
    return _vctColumn;
  }

  const PhysColumn *GetColumn(string &fieldName) const;
  const PhysColumn *GetColumn(int pos);
  const MHashMap<string, int>::Type GetMapColumnPos() { return _mapColumnPos; }
  const unordered_multimap<int, int> &GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  void AddColumn(string &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, string &comment, Charsets charset,
                 any &valDefault);
  void SetPrimaryKey(MVector<string>::Type &priCols);

  void AddSecondaryKey(string &indexName, IndexType indexType,
                       MVector<string>::Type &colNames);
  // Only used in developing time, in the future will save table schema to
  // system table
  void ReadData();
  void WriteData();

  void OpenTable();
  void CloseTable();
  void GenIndexDataValues(IndexProp &prop, VectorDataValue &vct) const;
  void GenColumsDataValues(VectorDataValue &vct) const;

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }
  void Clear();

protected:
  // Auto increment id, every time add 256.
  // The high 3 bytes used for table id, and the last byte used for index id.
  // Primary key id is 0 and other index ids order by index order.
  uint32_t _tid;
  /**The root path for the database.*/
  string _rootPath;
  /**The datebase name that this table belong to*/
  string _dbName;
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  MVector<PhysColumn *>::Type _vctColumn;
  /** The map for column name and their position in column list */
  MHashMap<string, int>::Type _mapColumnPos;
  /**All index, the primary key must be the first.*/
  MVector<IndexProp *>::Type _vctIndex;
  // The map for index with index name and position
  MHashMap<string, int>::Type _mapIndexNamePos;
  /**The map for index with first column's position in _vctColumn and index
   * position in _vctIndex*/
  unordered_multimap<int, int> _mapIndexFirstField;
};

class ResultTable : public BaseTable {};
} // namespace storage
