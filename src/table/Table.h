#pragma once
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../core/IndexTree.h"
#include "../core/IndexType.h"
#include "../core/LeafRecord.h"
#include "../dataType/IDataValue.h"
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
  inline const string &GetName() const { return _name; }
  inline void SetName(string name) {
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
  IndexColumn() {}
  IndexColumn(const string &name, uint32_t pos) : colName(name), colPos(pos) {}

  string colName;
  uint32_t colPos = 0;
};

struct IndexProp {
  IndexProp() : _position(UINT32_MAX), _type(IndexType::UNKNOWN) {}
  IndexProp(string &name, uint32_t pos, IndexType type,
            MVector<IndexColumn> &vctCol)
      : _name(name), _position(pos), _type(type) {
    _vctCol.swap(vctCol);
  }

  uint32_t Write(Byte *bys, uint32_t bysLen);
  uint32_t Read(Byte *bys);
  uint32_t CalcSize();

  // Index name
  string _name;
  // The position of this index, start from 0 and primary key must be 0. Table
  // id + this position will be the file id in IndxTree.
  uint32_t _position;
  // This index is primary key, unique key or nonunique key.
  IndexType _type;
  // The columns that composit this index
  MVector<IndexColumn> _vctCol;
  // Index tree,
  IndexTree *_tree = nullptr;
};

class PhysTable : public BaseTable {
public:
  PhysTable(string &dbName, string &tableName, string &desc, uint32_t tid)
      : _dbName(dbName), _name(name), _desc(desc), _tid(tid){};
  PhysTable(){};
  ~PhysTable() {}

  const string &GetTableName() const { return _name; }
  const string &GetDescription() const { return _desc; }
  const string &GetDbName() const { return _dbName; }
  const string GetFullName() const { return _dbName + "/" + _name; }
  uint32_t TableID() { return _tid; }
  const char *GetPrimaryName() const { return PRIMARY_KEY; }
  const IndexProp &GetPrimaryKey() const { return _vctIndex[0]; }
  const MVector<IndexProp> &GetVectorIndex() const { return _vctIndex; }
  IndexType GetIndexType(string &indexName) const {
    auto iter = _mapIndexNamePos.find(indexName);
    if (iter == _mapIndexNamePos.end())
      return IndexType::UNKNOWN;
    return _vctIndex[iter->second]._type;
  }

  const MVector<PhysColumn> &GetColumnArray() const { return _vctColumn; }
  const PhysColumn *GetColumn(string &fieldName) const;
  const PhysColumn *GetColumn(int pos);
  const MHashMap<string, uint32_t> GetMapColumnPos() { return _mapColumnPos; }
  const unordered_multimap<uint32_t, uint32_t> &GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  bool AddColumn(string &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, string &comment, Charsets charset,
                 any &valDefault);

  bool AddIndex(IndexType indexType, string &indexName,
                MVector<string> &colNames);
  /**
   * @brief Load this table information from the byte array.
   * @param bys The byte array saved the information.
   * @param len the length of byte array.
   * @return True: passed to load table information; False: failed to load. When
   * False, the error information will be saved into _threadErrorMsg
   */
  bool LoadData(Byte *bys, uint32_t len);
  /**
   * @brief Save this table information into the byte array.
   * @param bys The byte array used to save the table information.
   * @param len the length of byte array.
   * @return The length of the byte array occupied by table information
   */
  uint32_t SaveData(Byte *bys, uint32_t len);

  bool OpenIndex(size_t idx, bool bCreate);
  void CloseIndex(size_t idx) {
    assert(idx > 0 && idx < _vctIndex.size());
    IndexProp &prop = _vctIndex[idx];
    prop._tree->Close();
    prop._tree = nullptr;
  }

  DT_MilliSec GetCreateTime() { return _dtCreate; }
  DT_MilliSec GetLastUpdateTime() { return _dtLastUpdate; }

  void GenSecondaryRecords(const LeafRecord *lrSrc, const LeafRecord *lrDst,
                           const VectorDataValue &dstVd, ActionType type,
                           Statement *stmt, VectorLeafRecord &vctRec);
  int32_t GetRefCount() { return _refCount.load(memory_order_relaxed); }
  int32_t IncRef(int32_t i = 1) {
    return _refCount.fetch_add(i, memory_order_relaxed);
  }
  int32_t DecRef(int32_t i = 1) {
    return _refCount.fetch_sub(i, memory_order_relaxed);
  }

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }

protected:
  /** The database name this table belong to*/
  string _dbName;
  /**Table name*/
  string _name;
  /**Table describer*/
  string _desc;
  // How much time that this instance has been referenced.
  atomic_int32_t _refCount{0};
  // Auto increment id, every time add 256.
  // The high 3 bytes used for table id, and the last byte used for index id.
  // Primary key id is 0 and other index ids order by index order.
  uint32_t _tid;
  // The date time to create this table
  DT_MilliSec _dtCreate;
  // The last date time to update this table
  DT_MilliSec _dtLastUpdate;
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  MVector<PhysColumn> _vctColumn;
  /** The map for column name and their position in column list */
  MHashMap<string, uint32_t> _mapColumnPos;
  /**All index, the primary key must be the first.*/
  MVector<IndexProp> _vctIndex;
  // The map for index with index name and position
  MHashMap<string, uint32_t> _mapIndexNamePos;
  /**The map for index with first column's position in _vctColumn and index
   * position in _vctIndex*/
  unordered_multimap<uint32_t, uint32_t> _mapIndexFirstField;
  // The positions of all columns that constitute the all secondary index.
  MVector<int> _vctIndexPos;
  // The last time to be visited.
  DT_MilliSec _dtLastVisit;
};

} // namespace storage
