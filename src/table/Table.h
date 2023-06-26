#pragma once
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../core/IndexTree.h"
#include "../core/IndexType.h"
#include "../core/LeafRecord.h"
#include "../dataType/IDataValue.h"
#include "../table/Column.h"
#include "../transaction/Transaction.h"
#include "../utils/ErrorMsg.h"
#include "../utils/SpinMutex.h"
#include "../utils/Utilitys.h"
#include "Column.h"

#include <any>

namespace storage {
using namespace std;
static const char *COLUMN_CONNECTOR_CHAR = "|";
static const char *PRIMARY_KEY = "PARMARYKEY";

enum class TableStatus : uint8_t {
  Normal,  // This table is opening with normal status
  Locking, // This table has been locked by a transaction and other transaction
           // can not visit it.
  Preparing, // This table is opening or fix data
  Droped     // This table has been droped and it is only remainder and will be
             // remove in near future
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

  uint32_t Write(Byte *bys);
  uint32_t Read(Byte *bys, uint32_t pos,
                const MHashMap<string, uint32_t> &mapColumnPos);
  /** @brief To calculate the length to save this index
   * 1) 2 + n bytes: index name length + contents
   * 2) 1 byte: Index type
   * 3) 2 bytes: The number of columns to composite this index.
   * 4) (2 + n) * m: The column name length + contents * number
   */
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

class PhysTable {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  PhysTable(const string &dbName, const string &tableName, const string &desc,
            uint32_t tid, DT_MilliSec dtCreate)
      : _dbName(dbName), _name(tableName), _fullName(_dbName + "." + tableName),
        _desc(desc), _tid(tid), _dtCreate(dtCreate) {
    _dtLastUpdate = MilliSecTime();
  };
  PhysTable()
      : _dbName(), _name(), _fullName(), _desc(), _tid(0), _dtCreate(0){};
  ~PhysTable() {}

  const string &GetTableName() const { return _name; }
  const string &GetDescription() const { return _desc; }
  const string &GetDbName() const { return _dbName; }
  const string &GetFullName() const { return _fullName; }
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
  const PhysColumn *GetColumn(string &fieldName) const {
    auto iter = _mapColumnPos.find(fieldName);
    if (iter == _mapColumnPos.end()) {
      return nullptr;
    } else {
      return &_vctColumn[iter->second];
    }
  }
  const PhysColumn *GetColumn(int pos) {
    if (pos < 0 || pos > _vctColumn.size()) {
      return nullptr;
    } else {
      return &_vctColumn[pos];
    }
  }
  const MHashMap<string, uint32_t> GetMapColumnPos() { return _mapColumnPos; }
  // const unordered_multimap<uint32_t, uint32_t> &GetIndexFirstFieldMap() {
  //   return _mapIndexFirstField;
  // }

  // Add normal column
  bool AddColumn(const string &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, const string &comment, Charsets charset,
                 const any &valDefault);
  // Add auto increment column. In this version, only one auto increment column
  // in a table and must be first column and as primary key, maybe update in
  // future.
  bool AddColumn(const string &columnName, DataType dataType,
                 const string &comment, int64_t initVal, int64_t incStep);
  bool AddIndex(IndexType indexType, const string &indexName,
                const MVector<string> &colNames);
  /**
   * @brief Load this table information from the byte array.
   * @param bys The byte array saved the information.
   * @return The length of byte array to load, If error, return UINT32_MAX, the
   * detail information saved in _threadErrorMsg.
   */
  uint32_t LoadData(Byte *bys);
  /**
   * @brief Save this table information into the byte array.
   * @param bys The byte array used to save the table information.
   * @return The length of the byte array occupied by table information
   */
  uint32_t SaveData(Byte *bys);
  /**
   * @brief Calculate the byte array length to save this table information.
   * 1) 4 bytes: The length of byte array to save this table information
   * 2) 4 bytes: File version CURRENT_FILE_VERSION
   * 3) 4 bytes: table id
   * 4) 2 + n bytes: table name length and contents.
   * 5) 2 + n bytes: table describer length and contents.
   * 6) 8 bytes: table create time
   * 7) 8 bytes: table last update time
   * 8) 2 + n bytes: columns number and contents
   * 9) 2 + n bytes: Index number and contents, include primary key
   */
  uint32_t CalcSize();

  bool OpenIndex(size_t idx, bool bCreate = false);
  void CloseIndex(size_t idx) {
    assert(idx >= 0 && idx < _vctIndex.size());
    IndexProp &prop = _vctIndex[idx];
    if (prop._tree != nullptr) {
      prop._tree->Close();
      prop._tree = nullptr;
    }
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
  bool CreateTable() {
    for (size_t i = 0; i < _vctIndex.size(); i++) {
      assert(_vctIndex[i]._tree == nullptr);
      OpenIndex(i, true);
    }
  }
  bool LockTable(Transaction *tran) {
    if (!_spinMutex.try_lock())
      return false;
    if (_tableStatus != TableStatus::Normal) {
      _spinMutex.unlock();
      return false;
    }

    _lockTran = tran;
    _tableStatus = TableStatus::Locking;
    _spinMutex.unlock();
  }
  void UnlockTable() {
    _spinMutex.lock();
    _lockTran = nullptr;
    _tableStatus = TableStatus::Normal;
    _spinMutex.unlock();
  }
  inline TableStatus GetTableStatus() { return _tableStatus; }

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }
  inline void Clear() {
    _vctColumn.clear();
    _mapColumnPos.clear();
    _vctIndex.clear();
    _mapIndexNamePos.clear();
    _vctIndexPos.clear();
  }

protected:
  /** The database name this table belong to*/
  string _dbName;
  /**Table name*/
  string _name;
  /**db name + '.' + table name*/
  string _fullName;
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
  // MHashMap<uint32_t, uint32_t> _mapIndexFirstField;
  //  The positions of all columns that constitute the all secondary index.
  MVector<int> _vctIndexPos;
  //  The last time to be visited.
  DT_MilliSec _dtLastVisit{0};
  // This table status.
  TableStatus _tableStatus{TableStatus::Normal};
  // The transaction to lock this table
  Transaction *_lockTran{nullptr};
  // The mutex for table lock
  SpinMutex _spinMutex;
};

} // namespace storage
