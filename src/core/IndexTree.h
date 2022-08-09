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
    if (pid == PAGE_NULL_POINTER) {
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
  GarbageOwner *_garbageOwner = nullptr;

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

  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t)
  // Other: 0
  uint16_t _valVarLen;
  // PrimaryKey: ValVarFieldNum * sizeof(uint32_t) + Field Null bits
  // Other: 0
  uint16_t _valOffset;
  // Set this function when close tree and call it in destory method
  function<void()> _funcDestory = nullptr;

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
