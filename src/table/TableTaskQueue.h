#include "../table/Table.h"
#include "../utils/FastQueue.h"
#include "../utils/Utilitys.h"

#include <vector>

namespace storage {
/** The queues for an index task */
struct TaskQueue {
  /**
   * @param stCnt Session group threads count
   * @param ptCnt Parmary key's tasks count
   */
  TaskQueue(uint16_t stCnt, uint16_t ptCnt)
      : _fqStmt(stCnt), _fqRecord(ptCnt) {}
  /**The fast queue to receive statements from session group threads*/
  FastQueue<Statement *, 100> _fqStmt;
  /**The fast queue to receive records from pariamry key tasks */
  FastQueue<LeafRecord *, 100> _fqRecord;
};

/**Response for a index in the table */
struct IndexTaskQueue {
  IndexTaskQueue(uint16_t tCnt, uint16_t stCnt, uint16_t ptCnt,
                 vector<LeafRecord> vctRecBorder)
      : _vctQueue(tCnt, TaskQueue(stCnt, ptCnt)), _createTime(MilliSecTime()),
        _vctRecBorder(vctRecBorder) {}

  // Every task response a TaskQueue
  vector<TaskQueue> _vctQueue;
  // The time of old IndexTaskQueue moved into obsole _vctObsoleTableQueue, if
  // 0, means the related value in _vctObsoleTableQueue is null.
  DT_MilliSec _createTime;
  // The border LeafRecords for the index tasks
  vector<LeafRecord> _vctRecBorder;
};

class TableQueueMgr {
public:
  TableQueueMgr(uint16_t idxSz)
      : _vctTableQueue(idxSz, nullptr), _vctObsoleTableQueue(idxSz, nullptr) {}
  ~TableQueueMgr() {
    for (auto tq : _vctTableQueue) {
      if (tq != nullptr)
        delete tq;
    }
    for (auto tq : _vctObsoleTableQueue) {
      if (tq != nullptr)
        delete tq;
    }
  }

  bool InitIndexTaskQueue(uint16_t indexPos, uint16_t tCnt, uint16_t stCnt,
                          uint16_t ptCnt, vector<LeafRecord> vctRecBorder) {
    IndexTaskQueue *old = _vctTableQueue[indexPos];
    if (old != nullptr) {
      if (MilliSecTime() - old->_createTime < 1000)
        return false;

      if (_vctObsoleTableQueue[indexPos] != nullptr) {
        delete _vctObsoleTableQueue[indexPos];
      }

      _vctObsoleTableQueue[indexPos] = old;
    }

    IndexTaskQueue *itq = new IndexTaskQueue(tCnt, stCnt, ptCnt, vctRecBorder);
    _vctTableQueue[indexPos] = itq;
  }

protected:
  vector<IndexTaskQueue *> _vctTableQueue;
  vector<IndexTaskQueue *> *_vctObsoleTableQueue;
};

} // namespace storage