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
  /**
   * @param table The table that this task belong to
   * @param indexPos Which index of the table
   * @param stNum The thread number of session pool
   * @param mtNum The thread number of parmary index task
   */
  IndexTask(PhysTable *table, uint16_t indexPos, uint16_t stNum, uint16_t mtNum)
      : _table(table), _indexPos(indexPos), _fqStmt(stNum), _fqRecord(mtNum) {}

  virtual IndexTaskType TaskType() = 0;
  virtual bool AddStatement(uint16_t tNum, Statement *stmt, bool bSubmit) = 0;
  virtual bool AddLeafRecord(uint16_t tNum, LeafRecord *lr, bool bSubmit) = 0;

protected:
  PhysTable *_table;
  uint16_t _indexPos; // The index order in the table
  /**The queue of statements waitting to execute */
  MDeque<Statement *> _mqStmt;
  /**The queue of records waitting to execute */
  MDeque<LeafRecordAction *> _mqRecord;
  /**The fast queue to receive statements from other thread(ONLY one thread at
   * one time) */
  FastQueue<Statement *, 100> _fqStmt;
  /**The fast queue to receive records from other thread(ONLY one thread at
   * one time) */
  FastQueue<LeafRecord *, 100> _fqRecord;
};

// Every index tree will create a task, no matter what primary index or
// secondary index.
class TableSingleTask : public TableTask {
public:
  TableSingleTask(PhysTable *table, uint16_t indexPos)
      : IndexTask(table, indexPos) {}

  IndexTaskType TaskType() override { return IndexTaskType::SINGLE; }
  void Run() override;
  bool AddStatement(uint16_t tNum, Statement *stmt, bool bSubmit) override;
  bool AddLeafRecord(uint16_t tNum, LeafRecord *lr, bool bSubmit) override;
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