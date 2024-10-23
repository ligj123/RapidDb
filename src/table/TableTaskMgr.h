#include "../table/Table.h"
#include "../utils/RapidQueue.h"
#include "../utils/Utilitys.h"

#include <vector>

namespace storage {
class IndexTask;

// To save the selected primay key from secondary index by where conditions,
// they and related statement will be send into primary index tasks for
// following actions.
struct PriKeyStmt {
  // Primary key from secondary index
  RawKey _key;
  // The statement owned the key
  Statement *_stmt;
  // When search the position of the key in b+ tree, if the related page is not
  // in memory, here is used to save the page addr and as start page in next
  // procedure.
  IndexPage *_midPage{nullptr};
};

/**Response for a primary index in the table */
struct PriIndexTaskQueue {
  /**
   * Construct for primary index tasks queues
   * @param maxSessionGroupNum The max session groups number, one group response
   * a thread.
   * @param sessionGroupCount The session groups number.
   */
  PriIndexTaskQueue(uint16_t maxSessionGroupNum, uint16_t sessionGroupNum,
                    MVector<LeafRecord> &&vctRecBorder)
      : _fqStmt(maxSessionGroupNum, sessionGroupNum),
        _vctRecBorder(move(vctRecBorder)), _createTime(MilliSecTime()) {
    assert(maxSessionGroupNum >= sessionGroupNum);
  }

  // To receive statement from sessions. Its size equal session groups number
  RapidQueue<Statement> _fqStmt;
  // The border LeafRecords for this index tasks, its size equal this index
  // tasks number -1.
  MVector<LeafRecord> _vctRecBorder;
  // The time of this IndexTaskQueue created
  DT_MilliSec _createTime;
};

/**Response for a secondary index in the table */
struct SecIndexTaskQueue {
  /**
   * Construct for secondary index tasks queues
   * @param maxSessionGroupNum The max session groups number, one group response
   * a thread.
   * @param sessionGroupCount The session groups number.
   * @param maxTaskNum The max task number for a index,
   * @param secTaskNum The secondary index task number.
   * @param priTaskNum The primary index task number.
   */
  SecIndexTaskQueue(uint16_t maxSessionGroupNum, uint16_t sessionGroupNum,
                    uint16_t maxTaskNum, uint16_t secTaskNum,
                    uint16_t priTaskNum, MVector<LeafRecord> &&vctRecBorder)
      : _fqStmt(maxSessionGroupNum, sessionGroupNum),
        _fqRec(maxTaskNum, priTaskNum), _fqPriKeyStmt(maxTaskNum, secTaskNum),
        _createTime(MilliSecTime()), _vctRecBorder(move(vctRecBorder)) {
    assert(maxSessionGroupNum >= sessionGroupNum);
    assert(maxTaskNum >= secTaskNum && maxTaskNum >= priTaskNum);
  }

  // To receive statement from sessions. Its size equal session groups number
  RapidQueue<Statement> _fqStmt;
  // To receive the leaf records from primary index tasks. Its size equal the
  // primary index tasks number. For primary index, it should be empty.
  RapidQueue<LeafRecord> _fqRec;
  // To send the selected pri keys by where conditions to primary index tasks.
  // For primary index, it should be empty.
  RapidQueue<PriKeyStmt> _fqPriKeyStmt;
  // The border LeafRecords for this index tasks, its size equal this index
  // tasks number -1.
  MVector<LeafRecord> _vctRecBorder;
  // The time of this IndexTaskQueue created
  DT_MilliSec _createTime;
};

class TableTaskMgr {
public:
  /**
   * @brief Constructor
   */
  TableTaskMgr(uint16_t secIdxSz, uint16_t maxSessionGroupNum,
               uint16_t sessionGroupNum, uint16_t maxTaskNum)
      : _priIndexTaskQueue(maxSessionGroupNum, sessionGroupNum, {}),
        _vctTableQueue(secIdxSz,
                       SecIndexTaskQueue(maxSessionGroupNum, sessionGroupNum,
                                         maxTaskNum, 1, 1, {})) {}

  void ResetSessionGroupNum(uint16_t sessionGroupNum) {
    _priIndexTaskQueue._fqStmt.ResetLiveThreadNumber(sessionGroupNum);

    for (SecIndexTaskQueue &itq : _vctSecIndexTaskQueue) {
      itq._fqStmt(sessionGroupNum);
    }
  }

  bool ResetPriIndexTaskNum(uint16_t priTaskNum,
                            vector<LeafRecord> &&vctRecBorder) {
    assert(priTaskNum - 1 == vctRecBorder.size());
    _priIndexTaskQueue._vctRecBorder = move(vctRecBorder);

    for (SecIndexTaskQueue &itq : _vctSecIndexTaskQueue) {
      itq._fqRec.ResetLiveThreadNumber(priTaskNum);
    }
  }

  bool ResetSecIndexTaskNum(uint16_t idxPos, uint16_t secTaskNum,
                            MVector<LeafRecord> &&vctRecBorder) {
    assert(idxPos <= _vctSecIndexTaskQueue.size());
    assert(secTaskNum - 1 == vctRecBorder.size());

    SecIndexTaskQueue &itq = _vctSecIndexTaskQueue[idxPos - 1];
    itq._fqPriKeyStmt.ResetLiveThreadNumber(secTaskNum);
    itq._vctRecBorder = move(vctRecBorder);
  }

  void CollectPrimaryTaskData();
  void CollectSecondaryTaskData();

protected:
  PriIndexTaskQueue _priIndexTaskQueue;
  MVector<SecIndexTaskQueue> _vctSecIndexTaskQueue;
  MVector<PriIndexTask> _vctPriIndexTask;
  MVector<MVector<SecIndexTask>> _vctSecIndexTasks;
  SpinMutex _spinMutex;
};

} // namespace storage