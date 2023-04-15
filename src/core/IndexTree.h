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
  // For test purpose, close the index tree and wait until all pages are saved.
  static void TestCloseWait(IndexTree *indexTree);

public:
  IndexTree() {}
  bool CreateIndex(const string &indexName, const string &fileName,
                   VectorDataValue &vctKey, VectorDataValue &vctVal,
                   uint32_t indexId, IndexType iType);
  bool InitIndex(const string &indexName, const string &fileName,
                 VectorDataValue &vctKey, VectorDataValue &vctVal,
                 uint32_t indexId);

  void UpdateRootPage(IndexPage *root);
  IndexPage *AllocateNewPage(PageID parentId, Byte pageLevel);
  IndexPage *GetPage(PageID pageId, bool bLeafPage);
  void CloneKeys(VectorDataValue &vct);
  void CloneValues(VectorDataValue &vct);
  // Apply a series of pages for overflow pages. It will search Garbage Pages
  // first. If no suitable, it will apply new page id
  PageID ApplyPageId(uint16_t num) {
    PageID pid = _garbageOwner->ApplyPage(num);
    if (pid == PAGE_NULL_POINTER) {
      pid = _headPage->GetAndIncTotalPageCount(num);
    }
    return pid;
  }

  void ReleasePageId(PageID firstId, uint16_t num) {
    _garbageOwner->ReleasePage(firstId, num);
  }

  PageFile *ApplyPageFile();
  /** @brief Search B+ tree from root according record's key, util find the
   * LeafPage.
   * @param key The record's key for search
   * @param bEdit if edit the LeafPage, true: WriteLock, false: ReadLock
   * @param page The inputed and returned page. Null: start from root, or start
   * from this page.
   * @param bWait True: wait when load IndexPage from disk until find the
   * LeafPage, False: return directly when the IndexPage is not in memory.
   * @return True: found the LeafPage and in memory, False: Need to seach again.
   */
  bool SearchRecursively(const RawKey &key, bool bEdit, IndexPage *&page,
                         bool bWait = false);
  /** @brief Search B+ tree from root according record, util find the
   * LeafPage. If primary or unique key, only compare key, or Nonunique key,
   * compare key and value too.
   * @param key The record's key for search
   * @param bEdit if edit the LeafPage, true: WriteLock, false: ReadLock
   * @param page The inputed and returned page. Null: start from root, or start
   * from this page.
   * @param bWait True: wait when load IndexPage from disk until find the
   * LeafPage, False: return directly when the IndexPage is not in memory.
   * @return True: found the LeafPage and in memory, False: Need to seach again.
   */
  bool SearchRecursively(const LeafRecord &lr, bool bEdit, IndexPage *&page,
                         bool bWait = false);

  inline uint64_t GetRecordsCount() {
    return _headPage->ReadTotalRecordCount();
  }
  inline string &GetFileName() { return _fileName; }
  inline uint16_t GetFileId() { return _fileId; }
  inline bool IsClosed() { return _bClosed; }
  inline void SetClose() { _bClosed = true; }
  void Close(function<void()> funcDestory = nullptr);
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

  inline uint16_t GetKeyVarLen() { return 0; }
  inline uint16_t GetValVarLen() { return _valVarLen; }
  inline uint16_t GetKeyOffset() { return UI16_2_LEN; }
  inline uint16_t GetValOffset() { return _valOffset; }
  inline const VectorDataValue &GetVctKey() const { return _vctKey; }
  inline const VectorDataValue &GetVctValue() const { return _vctValue; }
  inline LeafPage *GetBeginPage() {
    PageID pid = _headPage->ReadBeginLeafPagePointer();
    return (LeafPage *)GetPage(pid, true);
  }

protected:
  ~IndexTree();

protected:
  string _indexName;
  string _fileName;
  std::queue<PageFile *> _fileQueue;
  SpinMutex _fileMutex;
  condition_variable_any _fileCv;
  /**How much page files were opened for this index tree*/
  uint32_t _rpfCount = 0;
  uint32_t _fileId = 0;
  bool _bClosed = false;
  /** Head page */
  HeadPage *_headPage = nullptr;
  GarbageOwner *_garbageOwner = nullptr;

  /** To record how much pages of this index tree are in memory */
  atomic<int32_t> _pagesInMem = 0;

  VectorDataValue _vctKey;
  VectorDataValue _vctValue;
  SpinMutex _pageMutex;
  MHashMap<uint64_t, PageLock *> _mapMutex;
  queue<PageLock *> _queueMutex;
  /**To lock for root page*/
  SharedSpinMutex _rootSharedMutex;
  IndexPage *_rootPage = nullptr;

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
