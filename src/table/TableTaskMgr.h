#include "../config/Configure.h"
#include "../core/IndexAction.h"
#include "../core/IndexTree.h"
#include "../table/Table.h"
#include "../utils/RapidQueue.h"
#include "../utils/Utilitys.h"

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
    return _queueTempAction.size() == 0 && _queueSessionAction.RoughSize() == 0;
  }

  // To receive IndexAction from sessions. Its lines equal session groups number
  RapidQueue<IndexAction> _queueSessionAction;
  // Temp to save IndexActions;
  MDeque<IndexAction *> _queueTempAction;
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
    return _queueTempAction.size() == 0 &&
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
  IndexTask(TableTaskMgr *taskMgr, uint16_t indexPos, uint16_t taskSn)
      : _taskMgr(taskMgr), _indexPos(indexPos), _taskSn(taskSn) {}

  TaskStatus Run() override;

protected:
  TableTaskMgr *_taskMgr;
  uint16_t _indexPos;
  // It use to sign which number IndexTask for this IndexTree.
  uint16_t _taskSn;

  friend class TableTaskMgr;
};

class TableTaskMgr {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  /**
   * @brief Constructor
   */
  TableTaskMgr(PhysTable *table, uint16_t sessionGroupNum)
      : _table(table), _sessionGroupNum(sessionGroupNum),
        _priIndexTaskQueue(sessionGroupNum, table->GetPrimaryKey()._tree) {

    MVector<IndexProp> &vctIndex = table->GetVectorIndex();
    _vctSecIndexTaskQueue.reserve(vctIndex.size());
    for (aize_t i = 0; i < vctIndex.size(); i++) {
      auto &prop = vctIndex[i];
      if (i > 0) {
        _vctSecIndexTaskQueue.push_back(
            SecondaryIndexTaskQueue(sessionGroupNum, 1, 1, prop._tree));
      }

      MVector<IndexTask *> vct;
      vct.push_back(new IndexTask(prop._tree, 1, 0, this, table));
      _vctIndexTasks.push_back(move(vct));
    }
  }

  ~TableTaskMgr() {
    for (auto &vtask : _vctIndexTasks) {
      for (auto task : vtask) {
        assert(task->GetStatus() == TaskStatus::FINISHED);
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

  /**
   * @brief Reset the ranges for an IndexTree. It will calcute the actual number
   * of ranges and split the pages into different ranges, then collect all old
   * tasks and create new IndexTask for it.
   * @param idxPos The position of IndexTree in table. It is same with PhysTable
   * @param exptTaskNum The expected number of the ranges to split
   */
  void ResetIndexTaskNum(uint16_t idxPos, uint16_t exptTaskNum);

  void CollectTaskData(uint16_t idxPos);

  bool IsAllTaskFinished(uint16_t idxPos) {
    assert(idxPos < (uint16_t)_vctIndexTasks.size());
    MVector<IndexTask *> &vct = _vctIndexTasks[idxPos];
    for (IndexTask *task : vct) {
      if (task->GetStatus() != TaskStatus::FINISHED) {
        return false;
      }
    }

    return true;
  }

protected:
  /**
   * @brief Calc how to split the IndexTree and split the pages into different
   * ranges.
   * @param indexPos Which index to split, it is same with IndexProp::_position
   * inPhysTable.
   * @param exptTaskNum The expected ranges to split, it will adjust in
   * according to actual conditions.
   * @return The actual range number to split
   */
  uint16_t CalcAndSpliteTaskRanges(uint16_t indexPos, uint16_t exptTaskNum);

protected:
  PhysTable *_table;
  uint16_t _sessionGroupNum;
  MVector<IndexTaskQueue *> _vctIndexTaskQueue;
  MVector<MVector<IndexTask *>> _vctIndexTasks;
  friend class IndexTask;
  friend class IndexAdjustTask;
};

class IndexAdjustTask : public ThreadTask {
public:
  IndexAdjustTask(TableTaskMgr *tableTaskMgr, uint16_t indexPos,
                  int16_t exptTaskNum)
      : _tableTaskMgr(tableTaskMgr), _indexPos(indexPos),
        _exptTaskNum(exptTaskNum) {}
  TaskStatus Run() override;
  bool IsNeedDelete() { return true; }

protected:
  TableTaskMgr *_tableTaskMgr;
  uint16_t _indexPos;
  int16_t _exptTaskNum;
};

} // namespace storage