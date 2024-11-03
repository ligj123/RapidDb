#include "../core/IndexTree.h"
#include "../utils/RapidQueue.h"
#include "../utils/ThreadPool.h"
#include "Table.h"

namespace storage {
struct PriKeyStmt;
class TableTaskMgr;

struct LeafRecordAction {
public:
  // LeafRecord to insert or delete
  LeafRecord *_leafRecord;
  // When search the position of the record in b+ tree, if the related page is
  // not in memory, here is used to save the page addr and as start page in next
  // procedure.
  IndexPage *_midPage{nullptr};
};

class PriIndexTask : public ThreadTask {
public:
  /**
   * @param table The table that this task belong to
   * @param indexPos Which index of the table
   * @param stNum The thread number of session pool
   * @param mtNum The thread number of parmary index task
   */
  PriIndexTask(uint16_t sn, uint16_t task_cnt, TableTaskMgr *taskMgr,
               PhysTable *table)
      : _sn(sn), _task_cnt(task_cnt), _taskMgr(taskMgr), _table(table) {}

  TaskStatus Run() override;

protected:
  // There maybe has more than 1 index tasks to execute the statement at the
  // same time, use it as the seriel number start from 0.
  uint16_t _sn;
  // The total index task for this index.
  uint16_t _task_cnt;

  TableTaskMgr *_taskMgr;
  PhysTable *_table;
  /**The queue of statements waitting to execute */
  MDeque<Statement *> _mqStmt;

  MDeque<PriKeyStmt *> _mqPriKeyStmt;

  /**The fast queue to receive statements from other thread(ONLY one thread at
   * one time) */
  LineQueue<Statement> _lqStmt;

  LineQueue<PriKeyStmt> _lqPriKeyStmt;

  friend class TableTaskMgr;
};

class SecIndexTask : public ThreadTask {
public:
  /**
   * @param table The table that this task belong to
   * @param indexPos Which index of the table
   * @param stNum The thread number of session pool
   * @param mtNum The thread number of parmary index task
   */
  SecIndexTask(uint16_t sn, uint16_t task_cnt, TableTaskMgr *taskMgr,
               PhysTable *table, uint16_t indexPos)
      : _sn(sn), _task_cnt(task_cnt), _taskMgr(taskMgr), _table(table),
        _indexPos(indexPos) {}

  TaskStatus Run() override;

protected:
  // There maybe has more than 1 index tasks to execute the statement at the
  // same time, use it as the seriel number start from 0.
  uint16_t _sn;
  // The total index task for this index.
  uint16_t _task_cnt;
  // The index order in the table
  uint16_t _indexPos;

  TableTaskMgr *_taskMgr;

  PhysTable *_table;

  /**The queue of statements waitting to execute */
  MDeque<Statement *> _mqStmt;
  /**The queue of records waitting to execute */
  MDeque<LeafRecordAction *> _mqRecord;

  /**The fast queue to receive statements from other thread(ONLY one thread at
   * one time) */
  LineQueue<Statement> _lqStmt;
  /**The fast queue to receive records from other thread(ONLY one thread at
   * one time) */
  LineQueue<LeafRecord> _lqRecord;

  friend class TableTaskMgr;
};

} // namespace storage