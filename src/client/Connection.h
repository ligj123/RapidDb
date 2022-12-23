#pragma once
#include "../utils/ErrorMsg.h"
#include <string>

namespace storage {
using namespace std;

class Connection {
public:
  // create a new connection
  static Connection *CreateConnection();

public:
  Connection(uint64_t sessionId) : _sessionId(sessionId), _bAutoCommit(false) {}
  ~Connection() {}
  bool Close();
  bool Commit();
  bool Rollback();

  void SetSchema(const string &schema) { _currSchema = schema; }
  string &GetSchema() { return _currSchema; }
  void SetAutoCommit(bool b) { _bAutoCommit = b; }
  bool GetAutoCommit() { return _bAutoCommit; }

protected:
  // Every connection will create a session in server, this id will be the
  // identity for this session.
  uint64_t _sessionId;
  // The current schema;
  string _currSchema;
  // The timeout seconds to run a statement;
  uint32_t _secQueryTimeout;
  // Error message in last statement
  vector<ErrorMsg *> _vctErrMsg;
  // True: auto transaction, false: manual transaction
  bool _bAutoCommit;
};
} // namespace storage