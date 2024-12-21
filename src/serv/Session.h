#pragma once
#include "../expr/ExprStatement.h"
#include "../statement/Statement.h"
#include "../table/Database.h"
#include "../utils/ResStatus.h"
#include "Transaction.h"

#include <atomic>
#include <unordered_map>

namespace storage {
using namespace std;
class SessionStatementAction;
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
  Session(uint16_t sessionGroupId, uint32_t id)
      : _id(id), _transaction(this, sessionGroupId) {}

  ~Session() { assert(_transaction.) }

public:
  // session id, only valid in this server and to identify the sessions.It will
  // start from 0, and add 1 every time. If exceed 2^32, it will restart from 0.
  uint32_t _id;
  // Auto commit the transaction or need client call commit.
  bool _bAutoCommit{true};
  // The session has been closed or not
  bool _bObsolete{false};
  // The transaction information.
  Transaction _transaction;

  // To record the current database switched by use database name
  Database *_currDb{nullptr};
  // The running statement in this session, or nullptr if not exist.
  Statement *_currStatement{nullptr};
  // The map of <Sql, parsed ExprStatement> in this session
  MStrHashMap<ExprStatement *> _mapSqlExprStatement;
  // The map of <exprstatement id, parsed ExprStatement> in this session,
  // duplicate of _mapSqlExprStatement.
  MHashMap<uint32_t, ExprStatement *> _mapIdExprStatement;
  // The create time for this session
  DT_MicroSec _createTime;
  // The last time to visit this session
  DT_MicroSec _lastVisitTime = 0;
  // The waitting statements,they will be execute one by one when previous
  // statement finished.
  MList<SessionStatementAction> _lstWaittingStmt;
};

} // namespace storage