#pragma once
#include "../cache/Mallocator.h"
#include "../header.h"
#include "../table/IndexAction.h"
#include "../utils/ErrorMsg.h"
#include "../utils/FileHandle.h"
#include "../utils/SpinMutex.h"
#include "GarbageOwner.h"
#include "HeadPage.h"
#include "LeafRecord.h"
#include "RawKey.h"

#include <atomic>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#define STAMP_BATCH 16

namespace storage {
using namespace std;
class LeafPage;
class BranchPage;
class BranchRecord;

struct IndexRange {
  IndexRange() {}
  IndexRange(IndexRange &&src) {}
  ~IndexRange();

  IndexPage *GetTopPage(IndexType type, RawRecord &rr);
  IndexPage *GetTopPage(IndexType type, RawKey &key);
  BranchRecord *GetLastRecord() {
    assert(_borderRecord != nullptr);
    return _borderRecord;
  }

  // The right border of record for this range
  BranchRecord *_borderRecord{nullptr};
  // The top level BrangePages assigned to this range
  MVector<BranchPage *> _vctRangePage;
  // The Start leafPage of this range
  LeafPage *_startPage{nullptr};
  // The end LeafPage of this range
  LeafPage *_endPage{nullptr};
  // The queue to save running IndexActions
  MList<IndexAction *> _queueAction;
  // The queue to temp save IndexActions that collected from session and other
  // IndexTree.
  LineQueue<IndexAction> _queueActionFromCollect;
  // The queue to temp save IndexActions that send from previous range.
  LineQueue<IndexAction> _queueActionFromPrev;
  // To save the increase-decrease of records in current range, it will be added
  // into the total record number in the HeadPage when write disk.
  int64_t _recordNumber{0};
  // To decrease atomic operation, every range will apply a batch of stamp one
  // time. Only used when multi ranges.
  VersionStamp _recordStampStart{0};
  VersionStamp _recordStampEnd{0};
  // The updated CachePages in this range that need to write into disk
  MTreeMap<uint64_t, CachePage *> _pageMap;
  // The last time to write updated CachePages into disk.
  DT_MicroSec _dtLastWriteDisk{1};
  // The datetime that the task has received stop signal and all actions has
  // been finished. After 100 milliseconds the task will stop if no more actions
  // com.
  DT_MicroSec _dtTaskStop{0};

  // To temp save the failed insert LeafRecord, it will delete when the
  // statement has been rollbacked
  MList<LeafRecord *> _lstErrRecord;
};

class IndexTree {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  /**
   * @brief To split the overlength page and save the contents into page buffer,
   * then push the pages into write queue
   * @param pageMap The map of waitting pages
   */
  void SettleUpdatedPages(MTreeMap<uint64_t, CachePage *> &pageMap);

public:
  IndexTree() {}
  ~IndexTree();

  bool CreateIndexTree(const MString &tableName, const MString &indexName,
                       const MString &fileName, VectorDataValue &vctKey,
                       VectorDataValue &vctVal, uint32_t indexId,
                       IndexType iType);
  bool LoadIndexTree(const MString &tableName, const MString &indexName,
                     const MString &fileName, VectorDataValue &vctKey,
                     VectorDataValue &vctVal, uint32_t indexId);
  void CloneKeys(VectorDataValue &vct);
  void CloneValues(VectorDataValue &vct);
  /**
   * @brief Apply one or more index page when an index page is been split.
   * @param parentPage The parent page, if nullptr, means it is root page.
   * @param pageLevel which level for this page
   * @param pnum The number of pages applied this time.
   */
  MVector<IndexPage *> ApplyIndexPages(BranchPage *parentPage, Byte pageLevel,
                                       uint32_t pnum);
  /**
   * @brief Apply a series of pages for overflow pages. It will search Garbage
   * Pages first. If no suitable, it will apply new page id.
   * @param num The number of pages
   * @return The created OverflowPage
   */
  OverflowPage *ApplyOvfPage(uint16_t num);

  IndexPage *GetPage(PageID pageId, PageType type,
                     BranchPage *parentPage = nullptr, bool bSyncRead = false);

  /**
   * @brief Recycle the unused pages into garbage owner
   * @param firstId The first page id of a series of pages.
   * @param num The number of the series of pages.
   * @param bBlock True: There have multi thread tasks to run this index tree's
   *                     tasks.
   *               False: There only has one thread task for this index tree.
   */
  inline void RecyclePageId(PageID firstId, uint16_t num) {
    _garbageOwner->RecyclePage(firstId, num, GetSplitPageLevel() != UINT8_MAX);
  }

  /** @brief Search B+ tree from an index page according record's key, util find
   * the LeafPage or the related index page not in memory cache.
   * @param key The record's key to search
   * @param page The start page for search, then return the result page. If page
   * Not in memory cache, it will return the middle level page and put the page
   * into read queue.
   * @return True: All related IndexPages are in memory and success to find the
   * LeafPage, False: One of related IndexPages is not in memory cache and need
   * to rerun after read.
   */
  bool SearchPage(const RawKey &key, IndexPage *&page);

  /** @brief Search B+ tree from index page according record, util find the
   * LeafPage. If primary or unique key, only compare key, or Nonunique key,
   * compare key and value at the same time.
   * @param lr The record for search
   * @param page The start page for search, then return the result page. If page
   * Not in memory cache, it will return the middle level page and put the page
   * into read queue.
   * @return True: All related IndexPages are in memory, False: One of related
   * IndexPages is not in memory and will load in a read task, it will search
   * again after loaded.
   */
  bool SearchPage(const LeafRecord &lr, IndexPage *&page);
  void Close();

  /**
   * @brief Remove a IndexPage and its child and all their relationships from
   * IndexTree, include parent page, prev page, next page, child page.
   * @param idxPage The index page that will be removed from index tree
   */
  void ReleaseIndexPage(IndexPage *idxPage);

  inline uint64_t GetRecordsCount() const {
    return _headPage->GetTotalRecordCount();
  }
  inline const MString &GetTableName() { return _tableName; }
  inline const MString &GetIndexName() { return _indexName; }
  inline const MString &GetFileName() const { return _fileName; }
  inline uint16_t GetFileId() const { return _fileId; }
  inline bool IsClosed() const { return _bClosed.load(memory_order_relaxed); }

  inline HeadPage *GetHeadPage() const { return _headPage; }
  // To inc pages in memory cache. It must be called in CachePagePool to ensure
  // thread safe.
  inline void IncPages(uint32_t pnum = 1) { _pagesInMem.fetch_add(pnum); }
  // To dec pages in memory cache. It must be called in CachePagePool to ensure
  // thread safe.
  inline void DecPages(uint32_t pnum = 1) {
    uint32_t old = _pagesInMem.fetch_sub(pnum);
    assert(old >= pnum);
    if (old == pnum) {
      delete this;
    }
  }

  inline uint16_t GetValVarLen() { return _valVarLen; }
  inline uint16_t GetValOffset() { return _valOffset; }
  inline const VectorDataValue &GetVctKey() const { return _vctKey; }
  inline const VectorDataValue &GetVctValue() const { return _vctValue; }
  inline LeafPage *GetBeginPage() {
    PageID pid = _headPage->GetBeginLeafPageID();
    return (LeafPage *)GetPage(pid, PageType::LEAF_PAGE);
  }
  inline FILE_HANDLE GetFileHandle() { return _fileHandle->FileDescriptor(); }
  inline IndexType GetIndexType() { return _indexType; }
  inline IndexPage *GetRootPage() { return _rootPage; }
  inline void UpdateRootPage(IndexPage *root) {
    bool block = (GetSplitPageLevel() != UINT8_MAX);
    if (block) {
      _spinMutex.lock();
    }

    _rootPage = root;
    _headPage->SetRootPageID(root->GetPageId());

    if (block) {
      _spinMutex.unlock();
    }
  }

  LeafRecord MakeMaxLeafRecord();
  LeafRecord MakeMinLeafRecord();

  MVector<IndexRange> &GetVctRange() { return _vctRange; }
  int CalcIndexRange(const RawRecord &rr);
  int CalcIndexRange(const RawKey &key);
  inline int CalcIndexRange(IndexPage *page) {
    return CalcIndexRange(*page->_vctRecord[0]);
  }

  bool IsMultiRange() { return _vctRange.size() > 1; }
  void UpdateRecordNumber(int iRange, int64_t recNum);
  VersionStamp ApplyStamp(int iRange);

  void AddActionFromPrev(int iRange, IndexAction *act) {
    assert(iRange >= 0 && iRange < _vctRange.size());
    IndexRange &range = _vctRange[iRange];
    range._queueActionFromPrev.Push(act);
  }

  // Add IndexAction that generate from current range. The producer and consumer
  // are in same thread.
  void AddActionFromLocal(int iRange, IndexAction *act) {
    _vctRange[iRange]._queueAction.push_back(act);
  }

  bool IsReranging() { return _bReranging; }
  void SetReRanging(bool b) { _bReranging = b; }
  Byte GetSplitPageLevel() { return _splitPageLevel; }
  void SetSplitPageLevel(Byte n) { _splitPageLevel = n; }
  SpinMutex &GetRangeMutex() { return _rangMutex; }

protected:
  // To record how much pages of this index tree are in CachePagePool.
  atomic_uint32_t _pagesInMem{0};
  MString _tableName;
  MString _indexName;
  MString _fileName;
  // The file handle for tree file
  FileHandle *_fileHandle{nullptr};
  /** Head page */
  HeadPage *_headPage = nullptr;
  // The manager for garbage page
  GarbageOwner *_garbageOwner = nullptr;
  IndexPage *_rootPage = nullptr;

  SpinMutex _spinMutex;
  VectorDataValue _vctKey;
  VectorDataValue _vctValue;

  // Every index will assign a unique id, it is table id + index  seriel number
  uint32_t _fileId{0};

  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t)
  // Other: 0
  uint16_t _valVarLen{0};
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t) + Field Null bits
  // Other: 0
  uint16_t _valOffset{0};
  atomic_bool _bClosed{false};
  IndexType _indexType;
  // The IndexTree is reranging or not
  atomic_bool _bReranging{false};
  // The page level to split range
  Byte _splitPageLevel{UINT8_MAX};
  // The vector of IndexRange
  MVector<IndexRange> _vctRange;

  SpinMutex _rangMutex;
  friend class HeadPage;
};
} // namespace storage
