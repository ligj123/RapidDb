#include "../core/IndexTree.h"
#include "../table/Table.h"
#include "../utils/FastQueue.h"
#include "../utils/ThreadPool.h"

namespace storage {
enum class IndexTaskType {
  SINGLE = -1, // only one task for current index tree
  MAIN = 0,    // The main task for current index tree, it will assign statement
               // and LeafRecord into different task
  SECOND       // The second task to run statement or LeafRecord
};

struct LeafRecordAction {
public:
  // LeafRecord to insert or delete
  LeafRecord *_leafRecord;
  // When search the position of the record in btree, if the related page is not
  // in memory, here is used to save the page addr and as start page in next
  // procedure.
  IndexPage *_midPage{nullptr};
};

class IndexTask : public Task {
public:
  void Run() override;

protected:
  void ExecStmts();
  void HandleRecords();
  virtual IndexTaskType TaskType() = 0;

protected:
  PhysTable *_table;
  uint16_t _indexPos; // The index order in the table
  /**The queue of statements waitting to execute */
  MDeque<Statement *> _mqStmt;
  /**The queue of records waitting to execute */
  MDeque<LeafRecordAction *> _mqRecord;
  /**The fast queue to receive statements from other thread(ONLY one thread at
   * one time) */
  FastQueue<Statement *> _fqStmt;
  /**The fast queue to receive records from other thread(ONLY one thread at
   * one time) */
  FastQueue<LeafRecord *> _fqRecord;
};

// Every index tree will create a task, no matter what primary index or
// secondary index.
class TableSingleTask : public TableTask {
public:
  IndexTaskType TaskType() override { return IndexTaskType::SINGLE; }

protected:
  // Receive the statement from session
  FastQueue<Statement, 100> _queueStmt;
  // For secondary index, receive LeafRecord from primary index; No used for
  // primary index.
  FastQueue<LeafRecordAction, 100> _queueRecord;
};

// If a table has too much statements to execute, it will create more than one
// task to run at the same time. The first task is main task, it has the
// function to dispatch and assign the statements and LeafRecords into the
// related tasks.
class TableMainTask : public TableTask {
public:
  IndexTaskType TaskType() override { return IndexTaskType::MAIN; }

protected:
};

class TableSecondTask : public TableTask {
public:
  IndexTaskType TaskType() override { return IndexTaskType::SECOND; }

protected:
};
} // namespace storage