#pragma once
#include "../cache/Mallocator.h"
#include "../file/PageFile.h"
#include "../header.h"
#include "../utils/ErrorMsg.h"
#include "../utils/SpinMutex.h"
#include "GarbageOwner.h"
#include "HeadPage.h"
#include "LeafRecord.h"
#include "RawKey.h"
#include <atomic>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace storage {
using namespace std;
class LeafPage;

struct PageLock {
  PageLock() : _sm(new SpinMutex), _refCount(0) {}
  ~PageLock() { delete _sm; }

  SpinMutex *_sm;
  int _refCount;
};

class IndexTree {
public:
  IndexTree(const string &tableName, const string &fileName,
            VectorDataValue &vctKey, VectorDataValue &vctVal,
            IndexType iType = IndexType::UNKNOWN);

  void UpdateRootPage(IndexPage *root);
  IndexPage *AllocateNewPage(PageID parentId, Byte pageLevel);
  IndexPage *GetPage(PageID pageId, bool bLeafPage);
  void CloneKeys(VectorDataValue &vct);
  void CloneValues(VectorDataValue &vct);
  // Apply a series of pages for overflow pages. It will search Garbage Pages
  // first. If no suitable, it will apply new page id
  PageID ApplyPageId(uint16_t num) {
    PageID pid = _garbageOwner->ApplyPage(num);
    if (pid == HeadPage::PAGE_NULL_POINTER) {
      pid = _headPage->GetAndIncTotalPageCount(num);
    }
    return pid;
  }

  void ReleasePageId(PageID firstId, uint16_t num) {
    _garbageOwner->ReleasePage(firstId, num);
  }

  PageFile *ApplyPageFile();
  bool SearchRecursively(const RawKey &key, bool bEdit, IndexPage *&page);
  bool SearchRecursively(const LeafRecord &lr, bool bEdit, IndexPage *&page);

  // ErrorMsg *InsertRecord(LeafRecord *rr);
  /**
   * @brief Read a record's data values by key, only used for primary key.
   * @param key The primary key to search.
   * @param recStamp The record stamp, If not care it , input ULLONG_MAX
   * @param vctVal To save result of the data values.
   * @param bOvf Include overflow fileds or not.
   * @return Return if find the record and record stamp is right
   */
  // bool ReadRecord(const RawKey &key, uint64_t verStamp, VectorDataValue
  // &vctVal,
  //                 bool bOvf = false);
  /**Read the primary keys by secondary key from secondary index record*/
  // bool ReadPrimaryKeys(const RawKey &key, VectorRawKey &vctKey);
  /**
   * @brief To query a secondary index from start key to end key.
   * @param keyStart The start key.
   * @param keyEnd The end key.
   * @param bIncLeft If include the records with start key.
   * @param bIncRight If include the record with the end key.
   * @param vctKey
   * @return Return if find the record and record stamp is right
   */
  // void QueryIndex(const RawKey *keyStart, const RawKey *keyEnd, bool
  // bIncLeft,
  //                 bool bIncRight, VectorRawKey &vctKey);
  /**
   * @brief To query a primary index from start key to end key.
   * @param keyStart The start key.
   * @param keyEnd The end key.
   * @param bIncLeft If include the records with start key.
   * @param bIncRight If include the record with the end key.
   * @param vctKey
   * @return Return if find the record and record stamp is right
   */
  // void QueryIndex(const RawKey *keyStart, const RawKey *keyEnd, bool
  // bIncLeft,
  //                 bool bIncRight, VectorRow &vctRow);

  // LeafRecord *GetRecord(const RawKey &key);
  // void GetRecords(const RawKey &key, VectorLeafRecord &vct);
  // void QueryRecord(RawKey *keyStart, RawKey *keyEnd, bool bIncLeft,
  //                  bool bIncRight, VectorLeafRecord &vct);

  inline uint64_t GetRecordsCount() {
    return _headPage->ReadTotalRecordCount();
  }
  inline string &GetFileName() { return _fileName; }
  inline uint16_t GetFileId() { return _fileId; }
  inline bool IsClosed() { return _bClosed; }
  inline void SetClose() { _bClosed = true; }
  void Close();
  inline void ReleasePageFile(PageFile *rpf) {
    lock_guard<SpinMutex> lock(_fileMutex);
    _fileQueue.push(rpf);
  }

  inline HeadPage *GetHeadPage() { return _headPage; }
  inline void IncPages() { _pagesInMem.fetch_add(1, memory_order_relaxed); }
  inline void DecPages() {
    int ii = _pagesInMem.fetch_sub(1, memory_order_relaxed);
    if (ii == 1) {
      delete this;
    }
  }

  inline uint16_t GetKeyVarLen() { return _keyVarLen; }
  inline uint16_t GetValVarLen() { return _valVarLen; }
  inline uint16_t GetKeyOffset() { return _keyOffset; }
  inline uint16_t GetValOffset() { return _valOffset; }
  inline const VectorDataValue &GetVctKey() const { return _vctKey; }
  inline const VectorDataValue &GetVctValue() const { return _vctValue; }

protected:
  ~IndexTree();

protected:
  string _tableName;
  string _fileName;
  std::queue<PageFile *> _fileQueue;
  SpinMutex _fileMutex;
  condition_variable_any _fileCv;
  /**How much page files were opened for this index tree*/
  uint32_t _rpfCount = 0;
  uint32_t _fileId;
  bool _bClosed = false;
  /** Head page */
  HeadPage *_headPage = nullptr;
  GarbageOwner *_garbageOwner;

  /** To record how much pages of this index tree are in memory */
  atomic<int32_t> _pagesInMem = 0;

  VectorDataValue _vctKey;
  VectorDataValue _vctValue;
  SpinMutex _pageMutex;
  MHashMap<uint64_t, PageLock *>::Type _mapMutex;
  queue<PageLock *> _queueMutex;
  /**To lock for root page*/
  SharedSpinMutex _rootSharedMutex;
  IndexPage *_rootPage = nullptr;

  // KeyVarFieldNum * sizeof(uint16_t)
  uint16_t _keyVarLen;
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t)
  // Other: ValVarFieldNum * sizeof(uint16_t)
  uint16_t _valVarLen;
  //(KeyVarFieldNum + 2) * sizeof(uint16_t)
  uint16_t _keyOffset;
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t) + Field Null bits
  // Other: ValVarFieldNum * sizeof(uint16_t) + sizeof(uint16_t)
  uint16_t _valOffset;

protected:
  // The ids have been used in this process
  static unordered_set<uint32_t> _setFiledId;
  // Used to find free id circled
  static uint32_t _currFiledId;
  // Used to static File IDs
  static SpinMutex _fileIdMutex;
  friend class HeadPage;
};
} // namespace storage
