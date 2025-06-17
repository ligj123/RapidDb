#pragma once
#include "../cache/Mallocator.h"
#include "../core/CoreEnum.h"
#include "../core/IndexTree.h"
#include "../core/LeafRecord.h"
#include "../dataType/IDataValue.h"
#include "../serv/Transaction.h"
#include "../table/Column.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/ResStatus.h"
#include "../utils/SpinMutex.h"
#include "../utils/Utilitys.h"
#include "Column.h"
#include "Database.h"

#include <any>

namespace storage {
using namespace std;
static const char *COLUMN_CONNECTOR_CHAR = "|";
static const char *PRIMARY_KEY = "PARMARYKEY";

class TableTaskMgr;

struct IndexColumn {
  IndexColumn() {}
  IndexColumn(const MString &name, uint32_t pos) : colName(name), colPos(pos) {}

  MString colName;
  uint32_t colPos = 0;
};

struct IndexProp {
  IndexProp() : _position(UINT32_MAX), _type(IndexType::UNKNOWN) {}
  IndexProp(const MString &name, uint32_t pos, IndexType type,
            MVector<IndexColumn> &vctCol)
      : _name(name), _position(pos), _type(type) {
    _vctCol.swap(vctCol);
  }
  IndexProp(const IndexProp &src) = delete;
  IndexProp(IndexProp &&src) {
    _name = move(src._name);
    _position = src._position;
    _type = src._type;
    _vctCol = move(src._vctCol);
    _tree = src._tree;
    src._tree = nullptr;
  }
  ~IndexProp();

  IndexProp &operator=(const IndexProp &src) = delete;
  IndexProp &operator=(IndexProp &&src) {
    _name = move(src._name);
    _position = src._position;
    _type = src._type;
    _vctCol = move(src._vctCol);
    _tree = src._tree;
    src._tree = nullptr;
    return *this;
  }

  uint32_t Write(Byte *bys);
  uint32_t Read(const Byte *bys, uint32_t pos,
                const MStrHashMap<uint32_t> &mapColumnPos);
  /** @brief To calculate the length to save this index
   * 1) 2 + n bytes: index name length + contents
   * 2) 1 byte: Index type
   * 3) 2 bytes: The number of columns to composite this index.
   * 4) (2 + n) * m: The column name length + contents * number
   */
  uint32_t CalcSize();

  VectorDataValue GenSecondaryData(VectorDataValue &vctPri) {
    VectorDataValue vctSec;
    vctSec.reserve(_vctCol.size());
    for (IndexColumn &icol : _vctCol) {
      vctSec.push_back(vctPri[icol.colPos]->AddRef());
    }

    return vctSec;
  }
  // Index name
  MString _name;
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
  PhysTable(Database *db, const MString &tableName, uint32_t tid,
            const MString &folder, DT_MilliSec dtCreate,
            DT_MilliSec dtLastUpdate, ResStatus tblStatus = ResStatus::Valid)
      : _db(db), _name(tableName),
        _fullName(_db->GetDbName() + "." + tableName),
        _folder(folder.size() > 0 ? folder : tableName), _tid(tid),
        _dtCreate(dtCreate), _dtLastUpdate(dtLastUpdate),
        _dtLastVisit(MilliSecTime()), _tableStatus(tblStatus) {
    _hash = MStrHash{}(_fullName);
  }

  PhysTable(uint32_t tid, const MString &folder)
      : _db(nullptr), _name(), _fullName(), _folder(folder), _tid(tid),
        _dtCreate(0), _dtLastUpdate(0), _dtLastVisit(0){};
  ~PhysTable() { Clear(); }

  const MString &GetTableName() const { return _name; }
  const MString &GetDbName() const { return _db->GetDbName(); }
  const MString &GetFullName() const { return _fullName; }
  uint32_t TableID() { return _tid; }
  void SetID(uint32_t id) { _tid = id; }
  void SetFolder(const MString &folder) { _folder = folder; }
  const MString GetPath() { return _db->GetDbPath() + "/" + _folder; }
  Database *GetDb() { return _db; }
  const char *GetPrimaryName() const { return PRIMARY_KEY; }
  IndexProp &GetPrimaryKey() { return _vctIndex[0]; }
  MVector<IndexProp> &GetVectorIndex() { return _vctIndex; }
  IndexType GetIndexType(const MString &indexName) const {
    auto iter = _mapIndexNamePos.find(indexName);
    if (iter == _mapIndexNamePos.end())
      return IndexType::UNKNOWN;
    return _vctIndex[iter->second]._type;
  }
  const MVector<PhysColumn> &GetColumnArray() const { return _vctColumn; }
  const PhysColumn *GetColumn(const MString &fieldName) const {
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
  const MStrHashMap<uint32_t> &GetMapColumnPos() { return _mapColumnPos; }
  const MHashMap<uint32_t, uint32_t> &GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  // Add normal column
  bool AddColumn(const MString &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, const MString &comment, Charsets charset,
                 IDataValue *valDefault);
  // Add auto increment column. In this version, only one auto increment column
  // in a table and must be first column and as primary key, maybe update in
  // future.
  bool AddColumn(const MString &columnName, DataType dataType,
                 const MString &comment, int64_t initVal, int64_t incStep);
  bool AddIndex(IndexType indexType, const MString &indexName,
                const MVector<MString> &colNames);
  /**
   * @brief Load this table information from the byte array.
   * @param bys The byte array saved the information.
   * @return The length of byte array to load, If error, return UINT32_MAX, the
   * detail information saved in _threadErrorMsg.
   */
  uint32_t LoadData(const Byte *bys);
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
  void Clear();

  bool OpenIndex(size_t idx, bool bCreate = false);
  /**
   * @brief To check if the values meet the columns' rule.
   * @param vctDv The values of columns
   */
  bool CheckColumnValues(VectorDataValue &vctDv);

  void CloseIndex(uint16_t idx = UINT16_MAX) {
    if (idx == UINT16_MAX) {
      for (IndexProp &prop : _vctIndex) {
        if (prop._tree != nullptr) {
          prop._tree->Close();
          prop._tree = nullptr;
        }
      }
    } else {
      assert(idx >= 0 && idx < _vctIndex.size());
      IndexProp &prop = _vctIndex[idx];
      if (prop._tree != nullptr) {
        prop._tree->Close();
        prop._tree = nullptr;
      }
    }
  }

  DT_MilliSec GetCreateTime() { return _dtCreate; }
  DT_MilliSec GetLastUpdateTime() { return _dtLastUpdate; }
  DT_MilliSec GetLastVisitTime() { return _dtLastVisit; }
  void SetLastVisitTime() { _dtLastVisit = MilliSecTime(); }

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
      bool b = OpenIndex(i, true);
      if (!b) {
        return false;
      }
    }
    return true;
  }
  bool OpenTable() {
    for (size_t i = 0; i < _vctIndex.size(); i++) {
      assert(_vctIndex[i]._tree == nullptr);
      bool b = OpenIndex(i, false);
      if (!b) {
        return false;
      }
    }
    return true;
  }

  inline ResStatus GetTableStatus() { return _tableStatus; }
  inline void SetTableStatus(ResStatus sts) { _tableStatus = sts; }
  inline void SetTableTaskMgr(TableTaskMgr *mgr) { _tableTaskMgr = mgr; }
  inline TableTaskMgr *GetTableTaskMgr() { return _tableTaskMgr; }
  inline IndexTree *GetIndexTree(uint16_t indexPos) {
    assert(indexPos >= 0 && indexPos < _vctIndex.size());
    return _vctIndex[indexPos]._tree;
  }

  size_t Hash() { return _hash; }
  DT_MilliSec GetLastCheckTime() { return _dtLastChecked; }
  void SetCheckTime() { _dtLastChecked = MilliSecTime(); }

protected:
  inline bool IsExistedColumn(MString &name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }

protected:
  /** The database of this table belong to*/
  Database *_db;
  /**Table name*/
  MString _name;
  /**db name + '.' + table name*/
  MString _fullName;
  // The folder to save this table, not include database path
  MString _folder;
  // How much time that this instance has been referenced.
  atomic_int32_t _refCount{0};
  // Auto increment id, every time add 256.
  // The high 3 bytes used for table id, and the last byte used for index id.
  // Primary key id is 0 and other index ids order by index order.
  uint32_t _tid;
  // The date time to create this table
  DT_MilliSec _dtCreate;
  // The last date time to update this table
  DT_MilliSec _dtLastUpdate{0};
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  MVector<PhysColumn> _vctColumn;
  /** The map for column name and their position in column list */
  MStrHashMap<uint32_t> _mapColumnPos;
  /**All index, the primary key must be the first.*/
  MVector<IndexProp> _vctIndex;
  // The map for index with index name and position
  MStrHashMap<uint32_t> _mapIndexNamePos;
  /**The map for index with first column's position in _vctColumn and index
   * position in _vctIndex*/
  MHashMap<uint32_t, uint32_t> _mapIndexFirstField;
  //  The positions of all columns that constitute the all secondary index. This
  //  variable is used to know which columns are compose secondary index.
  MVector<int> _vctIndexPos;
  //  The last time to be visited.
  DT_MilliSec _dtLastVisit{0};
  // This table status.
  ResStatus _tableStatus{ResStatus::Uninit};
  // The microsecond to check this table. Only used when it is obsolete.
  DT_MilliSec _dtLastChecked{0};
  // The transaction to lock this table
  Transaction *_lockTran{nullptr};
  // The mutex for table lock
  SpinMutex _spinMutex;

  size_t _hash{0};

  TableTaskMgr *_tableTaskMgr{nullptr};
};

} // namespace storage
