#pragma once
#include "../cache/Mallocator.h"
#include "../header.h"
#include "../utils/ErrorMsg.h"
#include "../utils/FileHandle.h"
#include "../utils/SpinMutex.h"
#include "GarbageOwner.h"
#include "HeadPage.h"
#include "IndexAction.h"
#include "LeafRecord.h"
#include "RawKey.h"

#include <atomic>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace storage {
using namespace std;
class LeafPage;
class BranchPage;

struct IndexRange {
  // The end border of this range
  LeafRecord _lrBorder;
  // The top level BrangePages assigned to this range
  MVector<BranchPage *> _vctRangePage;
  // The Start leafPage of this range
  LeafPage *_startPage{nullptr};
  // The end LeafPage of this range
  LeafPage *_endPage{nullptr};
  // The queue to save running IndexActions
  MDeque<IndexAction *> _queueAction;
  // The queue to temp save IndexActions that insert from other threads and will
  // be moved into _queueAction before run.
  MDeque<IndexAction *> _queueTempAction;
  // The SpinMutex used for _queueTempAction
  SpinMutex _mutex;

  void AddAction(IndexAction *act) {
    unique_lock<SpinMutex> lock(_mutex);
    _queueTempAction.push_back(act);
  }
  void AddActions(MDeque<IndexAction *> &queue) {
    unique_lock<SpinMutex> lock(_mutex);
    _queueTempAction.insert(_queueTempAction.end(), queue.begin(), queue.end());
    queue.clear();
  }
};

class IndexTree {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  IndexTree() {}
  ~IndexTree();

  bool CreateIndexTree(const MString &indexName, const MString &fileName,
                       VectorDataValue &vctKey, VectorDataValue &vctVal,
                       uint32_t indexId, IndexType iType,
                       const MString &tableName = "");
  bool LoadIndexTree(const MString &indexName, const MString &fileName,
                     VectorDataValue &vctKey, VectorDataValue &vctVal,
                     uint32_t indexId, const MString &tableName = "");
  void CloneKeys(VectorDataValue &vct);
  void CloneValues(VectorDataValue &vct);
  /**
   * @brief Apply one or more index page when an index page is been split.
   * @param parentPage The parent page, if nullptr, means it is root page.
   * @param pageLevel which level for this page
   * @param pnum The number of pages applied this time.
   * @param block If add lock when apply pages. True if there have multi thread
   * tasks for this index.
   */
  MVector<IndexPage *> ApplyIndexPages(BranchPage *parentPage, Byte pageLevel,
                                       uint32_t pnum, bool block);
  /**
   * @brief Apply a series of pages for overflow pages. It will search Garbage
   * Pages first. If no suitable, it will apply new page id.
   * @param num The number of pages
   * @param block If add lock when apply pages. True if there have multi thread
   * tasks for this index.
   * @return The created OverflowPage
   */
  OverflowPage *ApplyOvfPage(uint16_t num, bool block);

  IndexPage *GetPage(PageID pageId, PageType type,
                     BranchPage *parentPage = nullptr);
  LeafPage *GetLeafPage(PageID pageId, BranchPage *parentPage, LeafPage *prev,
                        LeafPage *next);
  /**
   * @brief Recycle the unused pages into garbage owner
   * @param firstId The first page id of a series of pages.
   * @param num The number of the series of pages.
   * @param bBlock True: There have multi thread tasks to run this index tree's
   *                     tasks.
   *               False: There only has one thread task for this index tree.
   */
  inline void RecyclePageId(PageID firstId, uint16_t num, bool bBlock) {
    _garbageOwner->RecyclePage(firstId, num, bBlock);
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
   * @brief To split the overlength page and save the contents into page buffer,
   * then push the pages into write queue
   * @param pageMap The map of waitting pages
   * @param lockPageLevel The page level that the BranchRecords in those pages
   * will be as borders that split the statements into different index task. If
   * =0xFF, means only one index task to run.
   */
  void SettleUpdatedPages(MTreeMap<uint64_t, CachePage *> &pageMap,
                          Byte lockPageLevel = UINT8_MAX);
  /**
   * @brief Remove a IndexPage and its child and all their relationships from
   * IndexTree, include parent page, prev page, next page, child page.
   * @param idxPage The index page that will be removed from index tree
   * @param bParent True: needs to set _childPage=nullptr in parent page's
   * BranchRecord
   * @param lockPageLevel The page level that the BranchRecords in those pages
   * will be as borders that split the statements into different index task. If
   * =0xFF, means only one index task to run.
   */
  void ReleaseIndexPage(IndexPage *idxPage, bool bParent = false,
                        Byte lockPageLevel = UINT8_MAX);

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
  inline void UpdateRootPage(IndexPage *root, bool block) {
    if (block) {
      _spinMutex.lock();
    }

    _rootPage = root;

    if (block) {
      _spinMutex.unlock();
    }
  }

  LeafRecord MakeMaxLeafRecord();
  LeafRecord MakeMinLeafRecord();

  MVector<IndexRange> &GetVctRange() { return _vctRange; }
  int CalcIndexRange(LeafRecord &lr);
  int CalcIndexRange(RawKey &key);
  bool IsMultiRange() { return _vctRange.size() > 1; }

protected:
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

  VectorDataValue _vctKey;
  VectorDataValue _vctValue;
  SpinMutex _spinMutex;

  // To record how much pages of this index tree are in CachePagePool.
  atomic_uint32_t _pagesInMem{0};
  // Every index will assign a unique id, it is table id + index  seriel number
  uint32_t _fileId{0};
  atomic_bool _bClosed{false};
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t)
  // Other: 0
  uint16_t _valVarLen{0};
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t) + Field Null bits
  // Other: 0
  uint16_t _valOffset{0};

  MVector<IndexRange> _vctRange;

  IndexType _indexType;
  friend class HeadPage;
};
} // namespace storage
