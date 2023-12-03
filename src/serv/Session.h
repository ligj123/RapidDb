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
enum class SessionStatus {

};

/**
 * The client create a connection and connect to server, the server will create
 * a session to response to this connection. All operations between client and
 * server will communicate by connection and session. All operations will run
 * one by one with order.
 */
class Session {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  Session(uint32_t id, function<void()> hookFunc) : _id(id) {
    _resStatus = ResStatus::Valid;
  }
  uint32_t SessionID() { return _id; }
  const Database *GetCurrDb() const { return _currDb; }
  void SetCurrDb(Database *db) { _currDb = db; }
  Statement *GetCurrStatement() { return _currStatement; }
  Transaction *GetCurrTransaction() { return _currTransaction; }
  void SetAutoCommit(bool b) { _bAutoCommit = b; }

  /** @brief Parse sql to ExprStatement, assign a new id to this statement. Then
   * create a new statement to execute the sql.
   * @param sql The sql string
   * @param paras One or multi group of parameters that wait to fill statement.
   * @return True, this session is free and passed to parse this ql, False,
   * failed to parse this sql or the session is busy.
   */
  bool CreateStatement(MString &&sql, VectorRow &paras);

  /**
   * @brief The sql has been parse in previous call, this time will reuse the
   * statement.
   * @param sid The statement id.
   * @param paras One or multi group of parameters that wait to fill statement.
   * @return True, this session is free and passed to parse this ql, False,
   * failed to parse this sql or the session is busy.
   */
  bool AddStatement(uint64_t stmt_id, VectorRow &paras);
  /**
   * @brief To judge if this session is running statement or has finished,
   * without statement.
   * @return True: No statement is running, False: One statement is running.
   */
  bool IsFree();

  bool BeginTran();
  bool CommitTran();
  bool RollbackTran();

  void Close();

protected:
  // session id, only valid in this server and to identify the sessions.It will
  // start from 0, and add 1 every time. If exceed 2^32, it will restart from 0.
  uint32_t _id;
  // Auto commit the transaction or need client call commit.
  bool _bAutoCommit;
  // The session status.
  ResStatus _resStatus;
  // To record the current database switched by use database name
  Database *_currDb{nullptr};
  // The running statement in this session, or nullptr if not exist.
  Statement *_currStatement{nullptr};
  // The current transaction in this session, or nullptr if not exist.
  Transaction *_currTransaction{nullptr};
  // The map of <Sql, parsed ExprStatement> in this session
  MStrHashMap<ExprStatement *> _mapSqlExprStatement;
  // The map of <id, parsed ExprStatement> in this session, duplicate of above
  // map.
  MHashMap<uint32_t, ExprStatement *> _mapIdExprStatement;
  // The hook function that willbe called when the statement finished.
  function<void()> _hookFunc;
};

} // namespace storage