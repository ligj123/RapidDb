#include "../core/IndexTree.h"
#include "../utils/SingleQueue.h"
#include "../utils/ThreadPool.h"
#include "Table.h"

namespace storage {
enum class TableTaskIdx {
  SINGLE = -1, // only one task for current index tree
  MAIN = 0,    // The main task for current index tree, it will assign statement
               // and LeafRecord into different task
  SECOND       // The second task to run statement or LeafRecord
};

class TableTask : public Task {
public:
  void Run() override;

protected:
  void ExecStmts();
  void HandleRecords();

protected:
  Table *_table;
  IndexTree *_indexTree;
  MDeque<Statement *> _vctStmt;
  MDeque<LeafRecord *> _vctRecord;
};

// Every index tree will create a task, no matter what primary index or
// secondary index.
class TableSingleTask : public TableTask {
public:
protected:
  // Receive the statement from session
  SingleQueue<Statement *> _queueStmt;
  // For secondary index, receive LeafRecord from primary index; No used for
  // primary index.
  SingleQueue<LeafRecord *> _queueRecord;
};

// If a table has too much statements to execute, it will create more than one
// task to run at the same time. The first task is main task, it has the
// function to dispatch and assign the statements and LeafRecords into the
// related tasks.
class TableMainTask : public TableTask {
public:
protected:
};

class TableSecondTask : public TableTask {
public:
protected:
};
} // namespace storage