#pragma once
#include "../cache/Mallocator.h"
#include "../utils/SpinMutex.h"

using namespace std;
namespace storage {
class IndexTree;
class OverflowPage;

/**This class use to collect garbage page of index page and overflow page. The
 * first garbage page id is saved in head page and this garbage page will save
 * other garbage pages. In this page, the first byte is page type, then the
 * second */
class GarbageOwner {
public:
  GarbageOwner(IndexTree *indexTree);
  ~GarbageOwner() {}

  void RecyclePage(PageID pid, uint16_t num, bool block);
  PageID ApplyPage(uint16_t num, bool block);
  bool SavePage(bool block);

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
  /**If it has changed since previous save time*/
  bool _bDirty;
  /**map<first page id in garbage, page number>*/
  MTreeMap<PageID, uint16_t> _treeFreePage;
  /**To find the free pages by this map. The key is the free pages number of
   * the range, the value is the first page id of the range with free page
   * map<page number, set<first page id>>*/
  MTreeMap<uint16_t, MHashSet<PageID>> _rangePage;
  /**SpinMutex*/
  SpinMutex _spinMutex;
  /**IndexTree*/
  IndexTree *_indexTree;
  /**The overflow page to save garbage pages*/
  OverflowPage *_ovfPage{nullptr};
};
} // namespace storage
