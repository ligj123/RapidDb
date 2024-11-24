#pragma once

#include "../cache/Mallocator.h"
#include "../utils/ThreadPool.h"
#include "RawKey.h"

namespace storage {
class IndexTree;
class IndexPage;
class Statement;
class LeafRecord;

class IndexAction {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  IndexAction(IndexTree *idxTree) : _indexTree(idxTree) {}
  virtual ~IndexAction() {}
  /**
   * @brief Run this action, and return the status to know if it has finished.
   */
  virtual TaskStatus Run() = 0;
  /**
   * @brief After the index range has been adjusted, calc again its index range
   * @return If pass, return its new range, or abort
   */
  virtual int JudgeRange() {
    abort();
    return -1;
  }
  /**
   * @brief If the index range need to adjust, to judge if this action can be
   * run after adjusted
   */
  virtual bool RearrangeAble() { return true; }

protected:
  IndexTree *_indexTree;
};

/**
 * @brief When split a LeafPage and the next page is in next index range, it can
 * not set the prev page id for next page
 */
class PrevPageAction : public IndexAction {
public:
  PrevPageAction(IndexTree *idxTree, int rangePos, PageID pageId,
                 PageID prevpageId)
      : IndexAction(idxTree), _rangePos(rangePos), _pageId(pageId),
        _prevPageId(prevpageId) {}
  TaskStatus Run() override;

  bool RearrangeAble() override { return false; }

protected:
  int _rangePos;                // The range position thia action belong to
  PageID _pageId;               // The page need to update previous page
  PageID _prevPageId;           // The new previous page id
  IndexPage *_idxPage{nullptr}; // Temp save
};

class InsertAction : public IndexAction {
public:
  InsertAction(IndexTree *idxTree, LeafRecord *lr)
      : IndexAction(idxTree), _lr(lr) {}
  TaskStatus Run() override;
  int JudgeRange() override;

protected:
  LeafRecord *_lr;
  IndexPage *_idxPage{nullptr}; // Temp save
};

class PriKeyAction : public IndexAction {
public:
  PriKeyAction(IndexTree *idxTree, RawKey &&key, Statement *stmt)
      : IndexAction(idxTree), _key(move(key)), _stmt(stmt) {}
  TaskStatus Run() override;
  int JudgeRange() override;

protected:
  RawKey _key;
  Statement *_stmt;
  IndexPage *_idxPage{nullptr}; // Temp save
};
} // namespace storage