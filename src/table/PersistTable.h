#pragma once
#include "../cache/Mallocator.h"
#include "../core/IndexTree.h"
#include "BaseTable.h"

namespace storage {
struct IndexProp {
  IndexType type;
  struct Column {
    MString colName;
    int colPos = 0;
  };
  MVector<Column>::Type vctCol;
};

class PersistTable : public BaseTable {
public:
  PersistTable(MString &rootPath, MString &dbName, MString &tableName,
               MString &tableAlias, MString &description);
  PersistTable(){};
  ~PersistTable();

  const char *GetPrimaryKey() const { return PRIMARY_KEY; }
  const IndexProp &GetPrimaryKeyColumns() const {
    return _mapIndex.at(PRIMARY_KEY);
  }
  const MHashMap<MString, IndexProp>::Type &GetMapIndex() const {
    return _mapIndex;
  }
  IndexType GetIndexType(MString &indexName) const {
    auto iter = _mapIndex.find(indexName);
    if (iter == _mapIndex.end())
      return IndexType::UNKNOWN;
    return iter->second.type;
  }

  const MVector<PersistColumn *>::Type &GetColumnArray() const {
    return _vctColumn;
  }

  const PersistColumn *GetColumn(MString &fieldName) const;
  const PersistColumn *GetColumn(int pos);
  MHashMap<MString, int>::Type GetMapColumnPos() { return _mapColumnPos; }
  MHashMap<int, MString>::Type GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  void AddColumn(MString &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, MString &comment, Charsets charset,
                 any &valDefault);
  void SetPrimaryKey(MVector<MString>::Type &priCols);

  void AddSecondaryKey(MString &indexName, IndexType indexType,
                       MVector<MString>::Type &colNames);
  // Only used in developing time, in the future will save table schema to
  // system table
  void ReadData();
  void WriteData();
  IndexTree *GetPrimaryIndexTree() const { return _primaryTree; }
  const MHashMap<MString, IndexTree *>::Type &GetMapTree() const {
    return _mapTree;
  }
  const IndexTree *GetIndexTree(const MString &indexName) const {
    auto iter = _mapTree.find(indexName);
    assert(iter != _mapTree.end());
    return iter->second;
  }

  void OpenTable();
  void CloseTable();
  void GenIndexDataValues(IndexProp &prop, VectorDataValue &vct) const;
  void GenColumsDataValues(VectorDataValue &vct) const;

protected:
  inline bool IsExistedColumn(MString name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }
  void Clear();

protected:
  /**The primary index tree for this table*/
  IndexTree *_primaryTree = nullptr;
  /**The map for index tree, include the primary index tree. The keys is index
   * name. */
  MHashMap<MString, IndexTree *>::Type _mapTree;
  /**The root path for the database.*/
  MString _rootPath;
  /**The datebase name that this table belong to*/
  MString _dbName;
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  MVector<PersistColumn *>::Type _vctColumn;
  /** The map for column name and their position in column list */
  MHashMap<MString, int>::Type _mapColumnPos;
  /**The first parameter is the unique name for a index and the primary key's
  name is fixed, must be PRIMARY_KEY. The second parameter is IndexProp.*/
  MHashMap<MString, IndexProp>::Type _mapIndex;
  /**The first column position to constitute the index;
  The second parameter,  the unique name for a index;*/
  MHashMap<int, MString>::Type _mapIndexFirstField;
};

} // namespace storage
