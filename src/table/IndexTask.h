#include "../core/IndexTree.h"
#include "../utils/SingleQueue.h"
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

class PriIndexTask : public Task {
public:
  /**
   * @param table The table that this task belong to
   * @param indexPos Which index of the table
   * @param stNum The thread number of session pool
   * @param mtNum The thread number of parmary index task
   */
  IndexTask(PhysTable *table, uint16_t indexPos, uint16_t stNum, uint16_t mtNum)
      : _table(table), _indexPos(indexPos), _fqStmt(stNum), _fqRecord(mtNum) {}

  void Run() override;

protected:
  TableTaskMgr *_taskMgr;
  PhysTable *_table;
  /**The queue of statements waitting to execute */
  MDeque<Statement *> _mqStmt;
  /**The queue of records waitting to execute */
  MDeque<LeafRecordAction *> _mqRecord;

  MDeque<PriKeyStmt *> _mqPriKeyStmt;

  /**The fast queue to receive statements from other thread(ONLY one thread at
   * one time) */
  LineQueue<Statement> _lqStmt;
  /**The fast queue to receive records from other thread(ONLY one thread at
   * one time) */
  LineQueue<LeafRecord> _lqRecord;

  LineQueue<PriKeyStmt> _lqPriKeyStmt;
};

class SecIndexTask : public Task {
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
  // The index order in the table
  uint16_t _indexPos;
  /**The queue of statements waitting to execute */
  MDeque<Statement *> _mqStmt;
  /**The queue of records waitting to execute */
  MDeque<LeafRecordAction *> _mqRecord;

  MDeque<PriKeyStmt *> _mqPriKeyStmt;

  /**The fast queue to receive statements from other thread(ONLY one thread at
   * one time) */
  LineQueue<Statement> _lqStmt;
  /**The fast queue to receive records from other thread(ONLY one thread at
   * one time) */
  LineQueue<LeafRecord> _lqRecord;

  LineQueue<PriKeyStmt> _lqPriKeyStmt;
};

} // namespace storage