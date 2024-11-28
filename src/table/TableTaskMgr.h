#include "../config/Configure.h"
#include "../core/IndexAction.h"
#include "../core/IndexTree.h"
#include "../table/Table.h"
#include "../utils/RapidQueue.h"
#include "../utils/Utilitys.h"

#include <vector>

namespace storage {

/**Response for a primary index in the table */
struct PrimaryIndexTaskQueue {
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
  PrimaryIndexTaskQueue(uint16_t sessionGroupNum, IndexTree *idxTree)
      : _sessionQueue(Configure::GetMaxSessionGroupNum(), sessionGroupNum),
        _indexTree(idxTree), _createTime(MilliSecTime()) {}

  // To receive IndexAction from sessions. Its lines equal session groups number
  RapidQueue<IndexAction> _sessionQueue;
  // The IndexTree that this queue belong to.
  IndexTree *_indexTree;
  // The time of this IndexTaskQueue created
  DT_MilliSec _createTime;
};

/**Response for a secondary index in the table */
struct SecondaryIndexTaskQueue {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  /**
   * Construct for secondary index tasks queues
   * @param sessionGroupCount The session groups number.
   * @param maxTaskNum The max task number for a index,
   * @param secTaskNum The secondary index task number.
   * @param priTaskNum The primary index task number.
   */
  SecondaryIndexTaskQueue(uint16_t sessionGroupNum, uint16_t secTaskNum,
                          uint16_t priTaskNum, IndexTree *idxTree)
      : _sessionQueue(Configure::GetMaxSessionGroupNum(), sessionGroupNum),
        _fromPrimaryQueue(Configure::GetMaxIndexTaskNum(), priTaskNum),
        _toPrimaryQueue(Configure::GetMaxIndexTaskNum(), secTaskNum),
        _indexTree(idxTree), _createTime(MilliSecTime()) {}
  // To receive IndexAction from sessions. Its lines equal session groups number
  RapidQueue<IndexAction> _sessionQueue;
  // To receive the IndexAction from primary index tasks. Its lines equal to the
  // primary index tasks number.
  RapidQueue<IndexAction> _fromPrimaryQueue;
  // To send the IndexAction to primary index tasks.Its lines equal to current
  // index tasks number.
  RapidQueue<IndexAction> _toPrimaryQueue;
  // The IndexTree that this queue belong to.
  IndexTree *_indexTree;
  // The time of this IndexTaskQueue created
  DT_MilliSec _createTime;
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
  IndexTask(IndexTree *indexTree, uint16_t taskCnt, uint16_t taskSn,
            TableTaskMgr *taskMgr, PhysTable *table)
      : _indexTree(indexTree), _taskCnt(taskCnt), _taskSn(taskSn),
        _taskMgr(taskMgr), _table(table) {}

  TaskStatus Run() override;

protected:
  IndexTree *_indexTree;
  // The total number of IndexTask's for this IndexTree.
  uint16_t _taskCnt;
  // It use to sign which number IndexTask for this IndexTree.
  uint16_t _taskSn;

  TableTaskMgr *_taskMgr;
  PhysTable *_table;

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
    _vctPriIndexTask.push_back(
        new IndexTask(table->GetPrimaryKey()._tree, 1, 0, this, table));

    MVector<IndexProp> &vctIndex = table->GetVectorIndex();
    _vctSecIndexTaskQueue.reserve(vctIndex.size());
    for (auto &prop : vctIndex) {
      _vctSecIndexTaskQueue.push_back(
          SecondaryIndexTaskQueue(sessionGroupNum, 1, 1, prop._tree));
      MVector<IndexTask *> vct;
      vct.push_back(new IndexTask(prop._tree, 1, 0, this, table));
      _vctSecIndexTasks.push_back(move(vct));
    }
  }

  void ResetSessionGroupNum(uint16_t sessionGroupNum) {
    _sessionGroupNum = sessionGroupNum;
    _priIndexTaskQueue._sessionQueue.ResetLiveThreadNumber(sessionGroupNum);

    for (SecondaryIndexTaskQueue &itq : _vctSecIndexTaskQueue) {
      itq._sessionQueue.ResetLiveThreadNumber(sessionGroupNum);
    }
  }

  void ResetPriIndexTaskNum(uint16_t priTaskNum) {
    uint16_t actualNum = CalcTaskRanges(0, priTaskNum);

    for (SecondaryIndexTaskQueue &itq : _vctSecIndexTaskQueue) {
      itq._fromPrimaryQueue.ResetLiveThreadNumber(actualNum);
    }
  }

  void ResetSecIndexTaskNum(uint16_t idxPos, uint16_t secTaskNum) {
    assert(idxPos <= _vctSecIndexTaskQueue.size());
    uint16_t actualNum = CalcTaskRanges(idxPos, secTaskNum);

    SecondaryIndexTaskQueue &itq = _vctSecIndexTaskQueue[idxPos - 1];
    itq._toPrimaryQueue.ResetLiveThreadNumber(secTaskNum);
  }

  void CollectPrimaryTaskData();
  void CollectSecondaryTaskData(uint16_t idxPos);

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
  uint16_t CalcTaskRanges(uint16_t indexPos, uint16_t exptTaskNum);

protected:
  PhysTable *_table;
  uint16_t _sessionGroupNum;
  PrimaryIndexTaskQueue _priIndexTaskQueue;
  MVector<SecondaryIndexTaskQueue> _vctSecIndexTaskQueue;
  MVector<IndexTask *> _vctPriIndexTask;
  MVector<MVector<IndexTask *>> _vctSecIndexTasks;
};

} // namespace storage