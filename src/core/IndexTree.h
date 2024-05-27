#pragma once
#include "../cache/Mallocator.h"
#include "../header.h"
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

namespace storage {
using namespace std;
class LeafPage;

class IndexTree {
public:
  IndexTree() {}
  bool CreateIndexTree(const MString &indexName, const MString &fileName,
                       VectorDataValue &vctKey, VectorDataValue &vctVal,
                       uint32_t indexId, IndexType iType);
  bool LoadIndexTree(const MString &indexName, const MString &fileName,
                     VectorDataValue &vctKey, VectorDataValue &vctVal,
                     uint32_t indexId);
  vector<IndexPage *> ApplyIndexPages(PageID parentId, Byte pageLevel,
                                      uint32_t pnum, bool block);
  IndexPage *GetPage(PageID pageId, PageType type);
  void CloneKeys(VectorDataValue &vct);
  void CloneValues(VectorDataValue &vct);
  // Apply a series of pages for overflow pages. It will search Garbage Pages
  // first. If no suitable, it will apply new page id
  PageID ApplyOvfPageId(uint16_t num, bool block) {
    PageID pid = (_garbageOwner == nullptr)
                     ? PAGE_NULL_POINTER
                     : _garbageOwner->ApplyPage(num, block);
    if (pid == PAGE_NULL_POINTER) {
      pid = _headPage->GetAndIncTotalPageCount(num, block);
    }
    return pid;
  }

  void RecyclePageId(PageID firstId, uint16_t num) {
    _garbageOwner->RecyclePage(firstId, num);
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

  inline uint64_t GetRecordsCount() const {
    return _headPage->ReadTotalRecordCount();
  }
  inline MString &GetFileName() const { return _fileName; }
  inline uint16_t GetFileId() const { return _fileId; }
  inline bool IsClosed() const { return _bClosed; }
  inline void SetClose() { _bClosed = true; }

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
    PageID pid = _headPage->ReadBeginLeafPagePointer();
    return (LeafPage *)GetPage(pid, PageType::LEAF_PAGE, true);
  }
  inline FILE_HANDLE GetFileHandle() { return _fileHandle.FileDescriptor(); }

protected:
  ~IndexTree();

protected:
  MString _indexName;
  MString _fileName;
  // The file handle for tree file
  FileHandle _fileHandle;
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
  bool _bClosed{false};
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t)
  // Other: 0
  uint16_t _valVarLen{0};
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t) + Field Null bits
  // Other: 0
  uint16_t _valOffset{0};

  friend class HeadPage;
};
} // namespace storage
