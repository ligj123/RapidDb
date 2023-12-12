#pragma once
#include "../expr/ExprStatement.h"
#include "../statement/Statement.h"
#include "../table/Database.h"
#include "../transaction/Transaction.h"
#include "../utils/ResStatus.h"

#include <atomic>
#include <unordered_map>

namespace storage {
using namespace std;
enum class SessionStatus : uint8_t {
  // No statement or transaction in this session
  Free = 0,
  // The sql has been added and waiting to execute.
  Added,
  // The statement is running
  Executing,
  // The statement has executed and waiting to write log.
  Executed,
  // Writting log. If autocommit, need commit, then finish and return
  // result to client,
  Logging,
  // Finished log
  Logged,
  // Commit this transaction
  Committing,
  // The statement has finished and wait client to get result.
  Finished,
  // Waitting next statement or commit transaction
  Waiting,
  // The session has been closed and obsolete.
  Obsolete
};

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
  Session(uint32_t id, function<void()> hookFunc = nullptr)
      : _id(id), _hookFunc(hookFunc) {}

  /**
   * @brief Add a statement into session.
   * @param sql The sql string to executed. If the second time to run this sql
   * and client has got the statement id, this value should be empty.
   * @param sid The statement id, it was create the first time. The id only
   * valid in this session.
   * @param paras One or multi group of parameters that wait to fill statement.
   * @return True: this session is free and added this statement into session;
   * False, failed to add into session.
   */
  bool AddStatement(MString &&sql, uint32_t stmtId, VectorRow &&paras) {
    if (_status != SessionStatus::Free && _status != SessionStatus::Waiting) {
      return false;
    }

    _sql = move(sql);
    _stmtId = stmtId;
    _paras = paras;
    _status = SessionStatus::Waiting;
    return true;
  }

  void GenStatement() {}

public:
  // session id, only valid in this server and to identify the sessions.It will
  // start from 0, and add 1 every time. If exceed 2^32, it will restart from 0.
  uint32_t _id;
  // Auto commit the transaction or need client call commit.
  bool _bAutoCommit{true};
  // After create this session, it will always true to sign this session is
  // valid. If it will set false if the session is droped and the session will
  // be move to obsolete vector.
  SessionStatus _status{SessionStatus::Free};

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
  // The hook function that will be called when the statement finished.
  function<void()> _hookFunc;
  // The create time for this session
  DT_MicroSec _createTime;
  // The last time to visit this session
  DT_MicroSec _lastVisitTime = 0;
  // The current statement id that will assign to new statement in this session.
  uint32_t _currStatementId;

  // Below 3 variable are only used for client and server in one process.
  //  The sql string to parse and execute.
  MString _sql;
  // The statement id that just added if it is parpre statement and second
  // input.
  uint32_t _stmtId;
  // Multi rows of the parameters
  VectorRow _paras;
};

} // namespace storage