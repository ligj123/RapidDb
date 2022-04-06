#pragma once
#include "../file/PageFile.h"
#include "../header.h"
#include "../utils/ErrorMsg.h"
#include "../utils/SpinMutex.h"
#include "HeadPage.h"
#include "LeafRecord.h"
#include "RawKey.h"
#include <atomic>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace storage {
using namespace std;

struct PageLock {
  PageLock() : _sm(new SpinMutex), _refCount(0) {}

  ~PageLock() { delete _sm; }

  SpinMutex *_sm;
  int _refCount;
};

class IndexTree {
public:
  IndexTree(const string &tableName, const string &fileName,
            VectorDataValue &vctKey, VectorDataValue &vctVal);
  ErrorMsg *InsertRecord(LeafRecord *rr);
  void UpdateRootPage(IndexPage *root);
  IndexPage *AllocateNewPage(uint32_t parentId, Byte pageLevel);
  void CloneKeys(VectorDataValue &vct);
  void CloneValues(VectorDataValue &vct);
  IndexPage *GetPage(uint32_t pageId, bool bLeafPage);
  PageFile *ApplyPageFile();

  /**
   * @brief Read a record's data values by key, only used for primary key.
   * @param key The primary key to search.
   * @param recStamp The record stamp, If not care it , input ULLONG_MAX
   * @param vctVal To save result of the data values.
   * @param bOvf Include overflow fileds or not.
   * @return Return if find the record and record stamp is right
   */
  bool ReadRecord(const RawKey &key, uint64_t verStamp, VectorDataValue &vctVal,
                  bool bOvf = false);
  /**Only to read overflow fileds for the last stamp version */
  bool ReadRecordOverflow(const RawKey &key, VectorDataValue &vctVal);
  /**Read the primary keys by secondary key from secondary index record*/
  bool ReadPrimaryKeys(const RawKey &key, VectorRawKey &vctKey);
  /**
   * @brief To query a secondary index from start key to end key.
   * @param keyStart The start key.
   * @param keyEnd The end key.
   * @param bIncLeft If include the records with start key.
   * @param bIncRight If include the record with the end key.
   * @param vctKey
   * @return Return if find the record and record stamp is right
   */
  void QueryIndex(const RawKey *keyStart, const RawKey *keyEnd, bool bIncLeft,
                  bool bIncRight, VectorRawKey &vctKey);
  /**
   * @brief To query a primary index from start key to end key.
   * @param keyStart The start key.
   * @param keyEnd The end key.
   * @param bIncLeft If include the records with start key.
   * @param bIncRight If include the record with the end key.
   * @param vctKey
   * @return Return if find the record and record stamp is right
   */
  void QueryIndex(const RawKey *keyStart, const RawKey *keyEnd, bool bIncLeft,
                  bool bIncRight, VectorRow &vctRow);

  LeafRecord *GetRecord(const RawKey &key);
  void GetRecords(const RawKey &key, VectorLeafRecord &vct);
  void QueryRecord(RawKey *keyStart, RawKey *keyEnd, bool bIncLeft,
                   bool bIncRight, VectorLeafRecord &vct);

  inline uint64_t GetRecordsCount() {
    return _headPage->ReadTotalRecordCount();
  }
  inline string &GetFileName() { return _fileName; }
  inline uint16_t GetFileId() { return _fileId; }
  inline bool IsClosed() { return _bClosed; }
  inline void SetClose() { _bClosed = true; }
  void Close(bool bWait);
  inline void ReleasePageFile(PageFile *rpf) {
    lock_guard<SpinMutex> lock(_fileMutex);
    _fileQueue.push(rpf);
  }
  inline PageFile *GetOverflowFile() {
    if (_ovfFile == nullptr) {
      unique_lock<SpinMutex> lock(_fileMutex);
      if (_ovfFile == nullptr) {
        string name =
            _fileName.substr(0, _fileName.find_last_of('.')) + "_ovf.dat";
        _ovfFile = new PageFile(name, true);
      }
    }
    return _ovfFile;
  }
  inline HeadPage *GetHeadPage() { return _headPage; }
  inline void IncPages() {
    _pageCountInPool.fetch_add(1, memory_order_relaxed);
  }
  inline void DecPages() {
    if (_pageCountInPool.fetch_add(-1, memory_order_relaxed) == 1)
      delete this;
  }
  // inline void IncTask() { _taskWaiting.fetch_add(1, memory_order_relaxed); }
  // inline void DecTask() { _taskWaiting.fetch_sub(1, memory_order_relaxed); }
  // inline int64_t GetTaskWaiting() {
  //   return _taskWaiting.load(memory_order_relaxed);
  // }

  // inline void InitStamp(uint64_t val) {
  //   _stampGen.store(val, memory_order_relaxed);
  // }
  // inline uint64_t GetStamp() {
  //   return _stampGen.fetch_add(1, memory_order_relaxed);
  // }

  inline uint16_t GetKeyVarLen() { return _keyVarLen; }
  inline uint16_t GetValVarLen() { return _valVarLen; }
  inline uint16_t GetKeyOffset() { return _keyOffset; }
  inline uint16_t GetValOffset() { return _valOffset; }

protected:
  ~IndexTree();
  LeafPage *SearchRecursively(const RawKey &key);
  LeafPage *SearchRecursively(const LeafRecord &lr);

protected:
  static std::atomic<uint32_t> _atomicFileId;
  std::string _tableName;
  std::string _fileName;
  std::queue<PageFile *> _fileQueue;
  SpinMutex _fileMutex;
  /**How much page files were opened for this index tree*/
  uint32_t _rpfCount = 0;
  uint16_t _fileId;
  PageFile *_ovfFile = nullptr;
  bool _bClosed = false;
  /** Head page */
  HeadPage *_headPage = nullptr;

  /** To record how much pages are in index page pool */
  atomic<int64_t> _pageCountInPool = 0;
  /** How much pages are waiting to read or write */
  // atomic<int64_t> _taskWaiting = 0;
  /**To generate new stamp for record, every time increase 1*/
  // atomic<int64_t> _stampGen = 0;

  VectorDataValue _vctKey;
  VectorDataValue _vctValue;
  SpinMutex _pageMutex;
  MHashMap<uint64_t, PageLock *>::Type _mapMutex;
  queue<PageLock *> _queueMutex;
  /**To lock for root page*/
  SharedSpinMutex _rootSharedMutex;
  IndexPage *_rootPage = nullptr;

  uint16_t _keyVarLen; // KeyVarFieldNum * sizeof(uint16_t)
  uint16_t _valVarLen; // ValVarFieldNum * sizeof(uint16_t)
  uint16_t _keyOffset; //(KeyVarFieldNum + 2) * sizeof(uint16_t)
  uint16_t _valOffset; //(ValVarFieldNum + 2) * sizeof(uint16_t)
protected:
  static MHashSet<uint16_t>::Type _setFiledId;
  static uint16_t _currFiledId;
  static SpinMutex _spinMutex;
  friend class HeadPage;
};
} // namespace storage
