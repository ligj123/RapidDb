#pragma once
#include "../expr/ExprStatement.h"
#include "../table/Database.h"
#include "../utils/ResStatus.h"
#include "Transaction.h"

#include <atomic>
#include <unordered_map>

namespace storage {
using namespace std;
class Transaction;
class Statement;

/**
 * The client create a connection and connect to server, the server will create
 * a session to response to this connection. All operations between client and
 * server will communicate by connection and session. All operations will run
 * one by one with order.
 */
struct Session {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  Session(uint32_t id);
  ~Session();
  /**
   * @brief Execute the tasks in this session.
   * @return True: It still has tasks to exec; False: Here has not tasks to run.
   */
  bool Exec();

  bool IsEmpty() {
    return _currStatement == nullptr && _lstWaittingStmt.size() == 0;
  }

public:
  // session id, only valid in this server and to identify the sessions.It will
  // start from 0, and add 1 every time. If exceed 2^32, it will restart from 0.
  uint32_t _id;
  // Auto commit the transaction or need client call commit.
  bool _bAutoCommit{true};
  // The session has been closed or not
  bool _bObsolete{false};
  // If this session is in busy queue of SessionGroup or not
  bool _bBusyQueue{false};
  // The transaction information.
  Transaction _transaction;

  // To record the current database switched by use database name
  Database *_currDb{nullptr};
  // The running statement in this session, or nullptr if not exist.
  Statement *_currStatement{nullptr};

  // The create time for this session
  DT_MicroSec _createTime;
  // The last time to visit this session
  DT_MicroSec _lastVisitTime = 0;
  // The waitting statements,they will be execute one by one when previous
  // statement finished.
  MList<Statement *> _lstWaittingStmt;
};

} // namespace storage