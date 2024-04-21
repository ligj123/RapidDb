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

class IndexTask : public Task {
public:
  void Run() override;

protected:
  void ExecStmts();
  void HandleRecords();
  virtual IndexTaskType TaskType() = 0;

protected:
  PhysTable *_table;
  IndexTree *_indexTree;
  MDeque<Statement *> _vctStmt;
  MDeque<LeafRecord *> _vctRecord;
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