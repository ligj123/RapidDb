#pragma once
#include "../core/IndexTree.h"
#include "BaseTable.h"

namespace storage {
struct IndexProp {
  IndexType type;
  struct Column {
    string colName;
    int colPos = 0;
  };
  MVector<Column>::Type vctCol;
};

class PersistTable : public BaseTable {
public:
  PersistTable(string &rootPath, string &dbName, string &tableName,
               string &tableAlias, string &description);
  PersistTable(){};
  ~PersistTable();

  const string &GetPrimaryKey() const { return PRIMARY_KEY; }
  const IndexProp &GetPrimaryKeyColumns() const {
    return _mapIndex.at(PRIMARY_KEY);
  }
  const MHashMap<string, IndexProp>::Type &GetMapIndex() const {
    return _mapIndex;
  }
  IndexType GetIndexType(string &indexName) const {
    auto iter = _mapIndex.find(indexName);
    if (iter == _mapIndex.end())
      return IndexType::UNKNOWN;
    return iter->second.type;
  }

  const MVector<PersistColumn *>::Type &GetColumnArray() const {
    return _vctColumn;
  }

  const PersistColumn *GetColumn(string &fieldName) const;
  const PersistColumn *GetColumn(int pos);
  MHashMap<string, int>::Type GetMapColumnPos() { return _mapColumnPos; }
  MHashMap<int, string>::Type GetIndexFirstFieldMap() {
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
  IndexTree *GetPrimaryIndexTree() const { return _primaryTree; }
  const MHashMap<string, IndexTree *>::Type &GetMapTree() const {
    return _mapTree;
  }
  const IndexTree *GetIndexTree(const string &indexName) const {
    auto iter = _mapTree.find(indexName);
    assert(iter != _mapTree.end());
    return iter->second;
  }

  void OpenTable();
  void CloseTable();
  VectorDataValue &GenIndexDataValues(IndexProp prop);
  VectorDataValue &GenColumsDataValues();

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }
  void Clear();

protected:
  /**The primary index tree for this table*/
  IndexTree *_primaryTree = nullptr;
  /**The map for index tree, include the primary index tree. The keys is index
   * name. */
  MHashMap<string, IndexTree *>::Type _mapTree;
  /**The root path for the database.*/
  string _rootPath;
  /**The datebase name that this table belong to*/
  string _dbName;
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

} // namespace storage
