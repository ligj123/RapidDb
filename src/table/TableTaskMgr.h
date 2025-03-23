#include "../config/Configure.h"
#include "../core/IndexTree.h"
#include "../table/Table.h"
#include "../utils/Log.h"
#include "../utils/RapidQueue.h"
#include "../utils/ThreadPool.h"
#include "../utils/Utilitys.h"
#include "IndexAction.h"

#include <vector>

namespace storage {

class TableTaskMgr;
class IndexTask : public ThreadTask {
public:
  /**
   * @param pool The ThreadPool to run this ThreadTask
   * @param taskMgr The TableTaskMgr that belong this task
   * @param indexPos The position of index in the table
   * @param taskPos The task position in task array of the index
   */
  IndexTask(ThreadPool *pool, TableTaskMgr *taskMgr, uint16_t indexPos,
            uint16_t taskPos);

  TaskStatus Run() override;

  uint16_t GetTaskPos() { return _taskPos; }

  uint64_t GetActionCount() {
    uint64_t tmp = _actionCount;
    _actionCount = 0;
    return tmp;
  }

protected:
  TableTaskMgr *_taskMgr;
  uint16_t _indexPos;
  // Which IndexRange that this IndexTask belong to.
  uint16_t _taskPos;

  uint64_t _actionCount{0};
  friend class TableTaskMgr;
};

enum class MgrStatus {
  INIT = 0, // Just create and not start TableTaskMgr tasks
  RUNNING,  // The tasks of this TableTaskMgr are running.
  SET_STOP, // The TableTaskMgr has been set to stop.
  STOPED,   // The TableTaskMgr has stoped
  ADJUSTING // One of IndexTree is adjust the IndexRanges
};

class TableTaskMgr {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
  // The datatime that the last time write updated CachePages into disk
  static DT_MicroSec _dtLastWriteDisk;

public:
  /**
   * @brief Constructor
   */
  TableTaskMgr(ThreadPool *pool, PhysTable *table, uint16_t sessionGroupNum,
               bool bExclusive = false)
      : _threadPool(pool), _table(table), _sessionGroupNum(sessionGroupNum) {
    MVector<IndexProp> &vctIndex = table->GetVectorIndex();
    MVector<ThreadTask *> vctTask;
    vctTask.reserve(vctIndex.size());

    for (size_t i = 0; i < vctIndex.size(); i++) {
      MVector<IndexRange> &vctRange =
          _table->GetVectorIndex()[i]._tree->GetVctRange();
      vctRange.clear();
      vctRange.resize(1);

      vctRange[0]._pageMap.emplace(UINT64_MAX,
                                   vctIndex[i]._tree->GetHeadPage());
      if (i == 0) {
        vctRange[0]._actionQueue = new IndexActionQueue(sessionGroupNum);
      } else {
        vctRange[0]._actionQueue = new SecIndexActionQueue(sessionGroupNum, 1);
      }

      MVector<IndexTask *> vct;
      IndexTask *task = new IndexTask(pool, this, i, 0);
      task->SetExclusiveTask(bExclusive);
      vct.push_back(task);
      vctTask.push_back(task);
      _vctIndexTasks.push_back(move(vct));
    }

    pool->AddTasks(ThreadPool::GetThreadId(), vctTask);
    SetMgrStatus(MgrStatus::RUNNING);
  }

  ~TableTaskMgr() {
    for (auto &vtask : _vctIndexTasks) {
      for (auto task : vtask) {
        assert(task->GetStatus(true) == TaskStatus::FINISHED);
        delete task;
      }
    }
  }

  void CollectTaskData(uint16_t idxPos, int iRange);

  /**
   * @brief The session group generate IndexActions and add them into action
   * queues of related index.
   * @param indexPos The position of IndexTree in table
   * @param sessionId session group id
   * @param action The IndexAction will be inserted
   */
  void AddSessionAction(uint16_t indexPos, uint16_t sessionGroupId,
                        IndexAction *action) {
    IndexTree *idxTree = _table->GetIndexTree(indexPos);
    if (idxTree->IsReranging()) [[unlikely]] {
      unique_lock<SpinMutex> lock(_spinMutex);
      if (idxTree->IsReranging()) {
        _lstTmpAction.push_back(action);
        return;
      }
    }

    idxTree->AddSessionAction(sessionGroupId, action);
  }

  MgrStatus GetMgrStatus() { return _mgrStatus.load(memory_order_relaxed); }
  void SetMgrStatus(MgrStatus s) {
    if (_mgrStatus.load(memory_order_relaxed) == MgrStatus::ADJUSTING) {
      LOG_WARN << "Failed to change table " << _table->GetFullName()
               << " MgrStatus, due to it is adjust IndexTask.";
      return;
    }
    _mgrStatus.store(s, memory_order_relaxed);
  }

  // To check if all IndexTasks have finished
  void CheckMgrStatus() {
    assert(_mgrStatus.load(memory_order_relaxed) == MgrStatus::SET_STOP);

    for (auto &vct : _vctIndexTasks) {
      for (auto task : vct) {
        if (task->GetStatus(true) != TaskStatus::FINISHED) {
          return;
        }
      }
    }

    _mgrStatus.store(MgrStatus::STOPED, memory_order_relaxed);
  }

  // Only used for testcase

  MVector<MVector<IndexTask *>> &GetVctIndexTasks() { return _vctIndexTasks; }

protected:
  ThreadPool *_threadPool;
  PhysTable *_table;
  uint16_t _sessionGroupNum;
  atomic<MgrStatus> _mgrStatus{MgrStatus::INIT};
  MVector<MVector<IndexTask *>> _vctIndexTasks;

  SpinMutex _spinMutex;
  // Temporarily to save the actions when adjust current
  MList<IndexAction *> _lstTmpAction;

  friend class IndexTask;
  friend class IndexAdjustTask;
};

class IndexAdjustTask : public ThreadTask {
public:
  IndexAdjustTask(ThreadPool *pool, TableTaskMgr *tableTaskMgr,
                  uint16_t indexPos, uint16_t exptTaskNum,
                  bool bExclusive = false)
      : ThreadTask(pool), _tableTaskMgr(tableTaskMgr), _indexPos(indexPos),
        _exptTaskNum(exptTaskNum), _bExclusive(bExclusive) {
    _taskName = "IndexAdjustTask" + _tableTaskMgr->_table->GetFullName() + "_" +
                ToMString(indexPos);
    assert(_exptTaskNum > 0);
  }

  TaskStatus Run() override;
  bool IsNeedDelete() const override { return true; }

protected:
  TableTaskMgr *_tableTaskMgr;
  uint16_t _indexPos;
  uint16_t _exptTaskNum;
  bool _bExclusive;
};

} // namespace storage