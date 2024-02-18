﻿#pragma once
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
  // For test purpose, close the index tree and wait until all pages are saved.
  static void TestCloseWait(IndexTree *indexTree);

public:
  IndexTree() {}
  bool CreateIndexTree(const MString &indexName, const MString &fileName,
                       VectorDataValue &vctKey, VectorDataValue &vctVal,
                       uint32_t indexId, IndexType iType);
  bool LoadIndexTree(const MString &indexName, const MString &fileName,
                     VectorDataValue &vctKey, VectorDataValue &vctVal,
                     uint32_t indexId);

  void UpdateRootPage(IndexPage *root);

  vector<IndexPage *> ApplyIndexPages(PageID parentId, Byte pageLevel,
                                      uint32_t pnum, bool block);
  IndexPage *GetPage(PageID pageId, PageType type, bool wait = false);
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

  /** @brief Search B+ tree from root according record's key, util find the
   * LeafPage.
   * @param key The record's key to search
   * @param bEdit if edit the LeafPage, true: WriteLock, false: ReadLock
   * @param page The start page for search, if Null, start from root page, then
   * return the result page after search, maybe the BranchPage if bWait=False.
   * @param bWait True: wait when load IndexPage from disk until find and return
   * the LeafPage, False: return directly when the related IndexPage is not in
   * memory.
   * @return True: All related IndexPages are in memory, False: One of related
   * IndexPages is not in memory and will load in a read task, it will search
   * again after loaded.
   */
  bool SearchRecursively(const RawKey &key, bool bEdit, IndexPage *&page,
                         bool bWait = false);
  /** @brief Search B+ tree from root according record, util find the
   * LeafPage. If primary or unique key, only compare key, or Nonunique key,
   * compare key and value at the same time.
   * @param key The record's key for search
   * @param bEdit if edit the LeafPage, true: WriteLock, false: ReadLock
   * @param page The start page for search, if Null, start from root page, then
   * return the result page after search, maybe the BranchPage if bWait=False.
   * @param bWait True: wait when load IndexPage from disk until find the
   * LeafPage, False: return directly when the IndexPage is not in memory.
   * @return True: All related IndexPages are in memory, False: One of related
   * IndexPages is not in memory and will load in a read task, it will search
   * again after loaded.
   */
  bool SearchRecursively(const LeafRecord &lr, bool bEdit, IndexPage *&page,
                         bool bWait = false);

  inline uint64_t GetRecordsCount() const {
    return _headPage->ReadTotalRecordCount();
  }
  inline MString &GetFileName() const { return _fileName; }
  inline uint16_t GetFileId() const { return _fileId; }
  inline bool IsClosed() const { return _bClosed; }
  inline void SetClose() { _bClosed = true; }
  void Close(function<void()> funcDestory = nullptr);

  inline HeadPage *GetHeadPage() const { return _headPage; }
  inline void IncPages(uint32_t pnum = 1) {
    _pagesInMem.fetch_add(pnum, memory_order_relaxed);
  }
  inline void DecPages(uint32_t pnum = 1) {
    int ii = _pagesInMem.fetch_sub(pnum, memory_order_relaxed);
    assert(ii >= 1);
    if (ii == 1) {
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
  FileHandle _fileHandle;
  // Every index will assign a unique id, it is table id + index  seriel number
  uint32_t _fileId = 0;
  bool _bClosed = false;
  /** Head page */
  HeadPage *_headPage = nullptr;
  GarbageOwner *_garbageOwner = nullptr;
  IndexPage *_rootPage = nullptr;

  /** To record how much pages of this index tree are in memory */
  atomic<uint32_t> _pagesInMem = 0;

  VectorDataValue _vctKey;
  VectorDataValue _vctValue;
  SpinMutex _spinMutex;

  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t)
  // Other: 0
  uint16_t _valVarLen = 0;
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t) + Field Null bits
  // Other: 0
  uint16_t _valOffset = 0;
  // Set this function when close tree and call it in destory method
  function<void()> _funcDestory = nullptr;

  friend class HeadPage;
};
} // namespace storage
