#pragma once

#include "../cache/Mallocator.h"
#include "../core/RawKey.h"
#include "../utils/ThreadPool.h"

namespace storage {
class IndexTree;
class IndexPage;
class Statement;
class InsertStatement;
class LeafRecord;
class IndexRange;
class LeafPage;
class StmtInsertRecord;
class StmtPriKey;

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
  virtual TaskStatus Exec() = 0;
  /**
   * @brief After the index range has been adjusted, calc again its index range
   * @return If pass, return its new range, or abort
   */
  virtual int JudgeRange() {
    abort();
    return -1;
  }

  /**
   * @brief Set the range position that this action belong to
   * @param pos The range position
   */
  void SetRangePos(int pos) { _rangePos = pos; }

  virtual const char *GetActionName() = 0;

protected:
  IndexTree *_indexTree;
  IndexPage *_idxPage{nullptr};
  int _rangePos{0}; // The range position thia action belong to
};

/**
 * @brief When split a LeafPage and the next page is in next index range, it can
 * not set the prev page id for next page
 */
class PrevPageAction : public IndexAction {
public:
  PrevPageAction(IndexTree *idxTree, int rangePos, PageID pageId,
                 PageID prevpageId)
      : IndexAction(idxTree), _pageId(pageId), _prevPageId(prevpageId) {
    _rangePos = rangePos;
  }
  TaskStatus Exec() override;

  int JudgeRange() override;

  const char *GetActionName() override { return "PrevPageAction"; }

protected:
  LeafPage *_page{nullptr}; // The page will update  previous page id.
  PageID _pageId;           // The page need to update previous page id.
  PageID _prevPageId;       // The new previous page id
};

class RecordAction : public IndexAction {
public:
  RecordAction(IndexTree *idxTree, LeafRecord *lr)
      : IndexAction(idxTree), _lr(lr) {}
  TaskStatus Exec() override;
  int JudgeRange() override;

  const char *GetActionName() override { return "RecordAction"; }

protected:
  LeafRecord *_lr;
};

class StatementAction : public IndexAction {
public:
  StatementAction(IndexTree *idxTree, Statement *stmt)
      : IndexAction(idxTree), _stmt(stmt) {}

  TaskStatus Exec() override;
  int JudgeRange() override;

  const char *GetActionName() override { return "StatementAction"; }

protected:
  Statement *_stmt;
};

class StmtInsertAction : public IndexAction {
public:
  StmtInsertAction(IndexTree *idxTree, StmtInsertRecord *stmtRecord)
      : IndexAction(idxTree), _stmtRecord(stmtRecord) {}
  ~StmtInsertAction();
  TaskStatus Exec() override;
  int JudgeRange() override;
  const char *GetActionName() override { return "StmtInsertAction"; }

protected:
  StmtInsertRecord *_stmtRecord;
};

class StmtPriKeyAction : public IndexAction {
public:
  StmtPriKeyAction(IndexTree *idxTree, StmtPriKey *stmtPriKey)
      : IndexAction(idxTree), _stmtPriKey(stmtPriKey) {}
  ~StmtPriKeyAction();
  TaskStatus Exec() override;
  int JudgeRange() override;
  const char *GetActionName() override { return "StmtPriKeyAction"; }

protected:
  StmtPriKey *_stmtPriKey;
};

} // namespace storage