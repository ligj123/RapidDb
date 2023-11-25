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

public:
  Session(uint32_t id) : _id(id) {}
  const Database *GetCurrDb() const { return _currDb; }

protected:
  // session id, only valid in this server and to identify the sessions.It will
  // start from 0, and add 1 every time. If exceed 2^32, it will restart from 0.
  uint32_t _id;
  // To record the current database switched by use database name
  Database *_currDb{nullptr};
  // The running statement in this session, or nullptr if not exist.
  Statement *_currStatement{nullptr};
  // The current transaction in this session, or nullptr if not exist.
  Transaction *_currTransaction{nullptr};
  // The parsed ExprStatement in this session
  MHashMap<MString, ExprStatement *> _mapExprStatement;
};

} // namespace storage