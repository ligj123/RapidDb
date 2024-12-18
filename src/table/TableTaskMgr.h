#include "../config/Configure.h"
#include "../core/IndexTree.h"
#include "../table/Table.h"
#include "../utils/RapidQueue.h"
#include "../utils/ThreadPool.h"
#include "../utils/Utilitys.h"
#include "IndexAction.h"

#include <vector>

namespace storage {

/**Response for a primary index in the table */
struct IndexTaskQueue {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  /**
   * Construct for primary index tasks queues
   * @param sessionGroupCount The session groups number.
   * @param idxTree The primary index tree
   */
  IndexTaskQueue(uint16_t sessionGroupNum)
      : _queueSessionAction(Configure::GetMaxSessionGroupNum(),
                            sessionGroupNum),
        _createTime(MilliSecTime()) {}
  virtual ~IndexTaskQueue() {}

  virtual bool IsQueueEmpty() {
    return _queueRangeAction.size() == 0 &&
           _queueSessionAction.RoughSize() == 0;
  }

  // To receive IndexAction from sessions. Its lines equal session groups number
  RapidQueue<IndexAction> _queueSessionAction;
  // Temp to save IndexActions from obsolete IndexTasks when rerange the
  // IndexTasks;
  MList<IndexAction *> _queueRangeAction;
  // The time of this IndexTaskQueue created
  DT_MilliSec _createTime;
};

/**Response for a secondary index in the table */
struct SecondaryIndexTaskQueue : public IndexTaskQueue {
public:
  /**
   * Construct for secondary index tasks queues
   * @param sessionGroupCount The session groups number.
   * @param secTaskNum The secondary index task number.
   * @param priTaskNum The primary index task number.
   */
  SecondaryIndexTaskQueue(uint16_t sessionGroupNum, uint16_t secTaskNum,
                          uint16_t priTaskNum)
      : IndexTaskQueue(sessionGroupNum),
        _fromPrimaryQueue(Configure::GetMaxIndexTaskNum(), priTaskNum),
        _toPrimaryQueue(Configure::GetMaxIndexTaskNum(), secTaskNum) {}

  bool IsQueueEmpty() override {
    return _queueRangeAction.size() == 0 &&
           _queueSessionAction.RoughSize() == 0 &&
           _fromPrimaryQueue.RoughSize() == 0 &&
           _toPrimaryQueue.RoughSize() == 0;
  }

  // To receive the IndexAction from primary index tasks. Its lines equal to the
  // primary index tasks number.
  RapidQueue<IndexAction> _fromPrimaryQueue;
  // To send the IndexAction to primary index tasks.Its lines equal to current
  // index tasks number.
  RapidQueue<IndexAction> _toPrimaryQueue;
};

class TableTaskMgr;
class IndexTask : public ThreadTask {
public:
  /**
   * @param table The table that this task belong to
   * @param indexPos Which index of the table
   * @param stNum The thread number of session pool
   * @param mtNum The thread number of parmary index task
   */
  IndexTask(ThreadPool *pool, TableTaskMgr *taskMgr, uint16_t indexPos,
            uint16_t taskPos)
      : ThreadTask(pool), _taskMgr(taskMgr), _indexPos(indexPos),
        _taskPos(taskPos) {}

  TaskStatus Run() override;

protected:
  TableTaskMgr *_taskMgr;
  uint16_t _indexPos;
  // It use to sign which number IndexTask for this IndexTree.
  uint16_t _taskPos;

  friend class TableTaskMgr;
};

enum class MgrStatus {
  INIT = 0, // Just create and not start TableTaskMgr tasks
  RUNNING,  // The tasks of this TableTaskMgr are running.
  SET_STOP, // The TableTaskMgr has been set to stop.
  STOPED    // The TableTaskMgr has stoped
};

class TableTaskMgr {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
  // The datatime the last time to set to write updated pages into disk
  static DT_MicroSec _dtLastWriteDisk;

public:
  /**
   * @brief Constructor
   */
  TableTaskMgr(ThreadPool *pool, PhysTable *table, uint16_t sessionGroupNum)
      : _threadPool(pool), _table(table), _sessionGroupNum(sessionGroupNum) {
    MVector<IndexProp> &vctIndex = table->GetVectorIndex();
    _vctIndexTaskQueue.reserve(vctIndex.size());
    MVector<ThreadTask *> vctTask;
    vctTask.reserve(vctIndex.size());

    for (size_t i = 0; i < vctIndex.size(); i++) {
      auto &prop = vctIndex[i];
      if (i == 0) {
        _vctIndexTaskQueue.push_back(new IndexTaskQueue(sessionGroupNum));
      } else {
        _vctIndexTaskQueue.push_back(
            new SecondaryIndexTaskQueue(sessionGroupNum, 1, 1));
      }

      MVector<IndexRange> &vctRange =
          _table->GetVectorIndex()[i]._tree->GetVctRange();
      vctRange.resize(1);

      MVector<IndexTask *> vct;
      IndexTask *task = new IndexTask(pool, this, 1, 0);
      vct.push_back(task);
      vctTask.push_back(task);
      _vctIndexTasks.push_back(move(vct));
    }

    pool->AddTasks(vctTask);
  }

  ~TableTaskMgr() {
    for (auto &vtask : _vctIndexTasks) {
      for (auto task : vtask) {
        assert(task->GetStatus(true) == TaskStatus::FINISHED);
        delete task;
      }
    }

    for (auto queue : _vctIndexTaskQueue) {
      assert(queue->IsQueueEmpty());
      delete queue;
    }
  }
  void ResetSessionGroupNum(uint16_t sessionGroupNum) {
    _sessionGroupNum = sessionGroupNum;

    for (size_t i = 0; i < _vctIndexTaskQueue.size(); i++) {
      IndexTaskQueue *tq = _vctIndexTaskQueue[i];
      tq->_queueSessionAction.ResetLiveThreadNumber(sessionGroupNum);
    }
  }

  void CollectTaskData(uint16_t idxPos);

  /**
   * @brief The session group generate IndexActions and add them into action
   * queues of related index.
   * @param indexPos The position of IndexTree in table
   * @param sessionId session id
   * @param action The IndexAction will be inserted
   */
  void AddSessionAction(uint16_t indexPos, uint16_t sessionId,
                        IndexAction *action) {
    assert(indexPos < _vctIndexTaskQueue.size());
    _vctIndexTaskQueue[indexPos]->_queueSessionAction.Push(sessionId, action);
  }
  /**
   *@brief The IndexActions that generate by primary index and will insert into
   *the action queue of secondary index.
   * @param indexPos The position of IndexTree in table that will accept the
   *action
   * @param rangeId Which range to generate this action from primary index.
   * @param action The IndexAction will be inserted
   */
  void AddFromPrimaryAction(uint16_t indexPos, uint16_t rangeId,
                            IndexAction *action) {
    assert(indexPos > 0 && indexPos < _vctIndexTaskQueue.size());
    SecondaryIndexTaskQueue *sitq =
        (SecondaryIndexTaskQueue *)_vctIndexTaskQueue[indexPos];
    sitq->_fromPrimaryQueue.Push(rangeId, action);
  }
  /**
   *@brief The IndexActions that generate by secondary index and will insert
   *into action queue of primary index.
   * @param indexPos The position of IndexTree in table that generate the action
   * @param rangeId Which range to generate this action from secondary index.
   * @param action The IndexAction will be inserted
   */
  void AddToPrimaryAction(uint16_t indexPos, uint16_t rangeId,
                          IndexAction *action) {
    assert(indexPos > 0 && indexPos < _vctIndexTaskQueue.size());
    SecondaryIndexTaskQueue *sitq =
        (SecondaryIndexTaskQueue *)_vctIndexTaskQueue[indexPos];
    sitq->_toPrimaryQueue.Push(rangeId, action);
  }

  MgrStatus GetMgrStatus() { return _mgrStatus; }
  void SetMgrStatus(MgrStatus s) { _mgrStatus = s; }
  // To check if all IndexTasks have finished
  void CheckMgrStatus() {
    assert(_mgrStatus == MgrStatus::SET_STOP);

    for (auto &vct : _vctIndexTasks) {
      for (auto task : vct) {
        if (task->GetStatus(true) != TaskStatus::FINISHED) {
          return;
        }
      }
    }

    _mgrStatus = MgrStatus::STOPED;
  }

protected:
  ThreadPool *_threadPool;
  PhysTable *_table;
  uint16_t _sessionGroupNum;
  MgrStatus _mgrStatus{MgrStatus::INIT};
  MVector<IndexTaskQueue *> _vctIndexTaskQueue;
  MVector<MVector<IndexTask *>> _vctIndexTasks;

  friend class IndexTask;
  friend class IndexAdjustTask;
};

class IndexAdjustTask : public ThreadTask {
public:
  IndexAdjustTask(ThreadPool *pool, TableTaskMgr *tableTaskMgr,
                  uint16_t indexPos, int16_t exptTaskNum)
      : ThreadTask(pool), _tableTaskMgr(tableTaskMgr), _indexPos(indexPos),
        _exptTaskNum(exptTaskNum) {}
  TaskStatus Run() override;
  bool IsNeedDelete() { return true; }

protected:
  TableTaskMgr *_tableTaskMgr;
  uint16_t _indexPos;
  int16_t _exptTaskNum;
};

} // namespace storage