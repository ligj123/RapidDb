#pragma once
#include "../cache/Mallocator.h"
#include "../utils/SpinMutex.h"

using namespace std;
namespace storage {
class IndexTree;
/**This class use to collect garbage page of index page and overflow page. The
 * first garbage page id is saved in head page and this garbage page will save
 * other garbage pages. In this page, the first byte is page type, then the
 * second */
class GarbageOwner {
public:
  GarbageOwner(IndexTree *indexTree);
  ~GarbageOwner() {}

  void ReleasePage(PageID pid, uint16_t num);
  PageID ApplyPage(uint16_t num);
  void SavePage();

protected:
  void InsertPage(PageID pageId, int16_t num);
  void ErasePage(PageID pageId, int16_t num);

protected:
  /** The total number of garbage pages, include the paged used to save garbage
   * page ids*/
  uint32_t _totalGarbagePages = 0;
  /**The first page id that used to save grabage page ids.*/
  PageID _firstPageId;
  /** The number that used to save garbage page ids */
  uint16_t _usedPageNum;
  /**How many a series pages to save the garbage page ids.*/
  uint16_t _pageUsedCount = 0;
  /**If it has changed since previous save time*/
  bool _bDirty;
  /**The first garbage page id, how many series */
  MTreeMap<PageID, uint16_t>::Type _treeFreePage;
  /**To find the free pages by this map. The key is the free pages number of
   * therange, the value is the first page id of the range with free page number
   * = key*/
  MTreeMap<uint16_t, MHashSet<PageID>::Type>::Type _rangePage;
  /**SpinMutex*/
  ReentrantSpinMutex _spinMutex;
  /**IndexTree*/
  IndexTree *_indexTree;
};
} // namespace storage
