#pragma once
#include "../expr/ExprStatement.h"
#include "../statement/Statement.h"
#include "../table/Database.h"
#include "../transaction/Transaction.h"
#include <atomic>
#include <unordered_map>

namespace storage {
using namespace std;

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

protected:
  // session id, only valid in this server and to identify the sessions.It will
  // start from 0, and add 1 every time. If goto 2^32, it will restart from 0.
  uint32_t _id;
  // To save the sql parse time, every sql statement will parse one time. In
  // follow time reuse this ExprStatement and identify it by id.
  unordered_map<uint32_t, ExprStatement *> _mapExprStat;
  // To generate id for _mapExprStat.
  atomic_int32_t _exprStatId;
  // To record the current database switched by use database name
  Database *_currDb;
  // The running statement in this session, or nullptr if not exist.
  Statement *_currStatement;
  // The current transaction in this session, or nullptr if not exist.
  Transaction *_currTransaction;
};

} // namespace storage