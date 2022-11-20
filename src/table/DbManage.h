#pragma once
#include "../expr/ExprStatement.h"
#include "../utils/ConcurrentHashMap.h"
#include "Database.h"
#include "Session.h"
#include "Table.h"

using namespace std;
// Mutex need very much
namespace storage {
// This is a static class, to manage all database resources, include tables etc.
class DbManage {
public:
  static PhysTable *GetTable(uint32_t id) {
    PhysTable *t;
    _inst->_mapIdTable.Find(id, t);
    return t;
  }
  static PhysTable *GetTable(const string &name) {
    PhysTable *t;
    _inst->_mapNameTable.Find(name, t);
    return t;
  }
  static Database *GetDb(string name) {
    Database *db;
    _inst->_mapDatabase.Find(name, db);
    return db;
  }
  static ExprStatement *GetExprStat(uint64_t id) {
    ExprStatement *es;
    _inst->_mapExprStat.Find(id, es);
    return es;
  }
  static Session *GetSession(uint64_t id) {
    Session *s;
    _inst->_mapSession.Find(id, s);
    return s;
  }

  static void AddTable(PhysTable *table) {
    _inst->_mapIdTable.Insert(table->TableID(), table);
    _inst->_mapNameTable.Insert(table->GetFullName(), table);
  }
  static void RemoveTable(PhysTable *table) {
    _inst->_mapIdTable.Erase(table->TableID());
    _inst->_mapNameTable.Erase(table->GetFullName());
  }
  static void AddDb(Database *db) {
    _inst->_mapDatabase.Insert(db->_dbName, db);
  }
  static void RemoveDb(Database *db) { _inst->_mapDatabase.Erase(db->_dbName); }
  static void AddExprStatement(uint64_t id, ExprStatement *es) {
    _inst->_mapExprStat.Insert(id, es);
  }
  static void RemoveExprStatement(uint64_t id) {
    _inst->_mapExprStat.Erase(id);
  }
  static void AddSession(uint64_t id, Session *s) {
    _inst->_mapSession.Insert(id, s);
  }
  static void RemoveSession(uint64_t id) { _inst->_mapSession.Erase(id); }

protected:
  DbManage()
      : _mapIdTable(10, 10000), _mapNameTable(10, 10000), _mapDatabase(1, 100),
        _mapExprStat(10, 10000), _mapSession(10, 10000) {}

protected:
  // The map for all opened table ids and themselves
  ConcurrentHashMap<uint32_t, PhysTable *> _mapIdTable;
  // The map for all opened table names and themselves, name should be 'db name'
  // + '/' + 'table name'
  ConcurrentHashMap<string, PhysTable *> _mapNameTable;
  // The map for all database names and databases. They should be loaded when
  // initializing
  ConcurrentHashMap<string, Database *> _mapDatabase;
  // The map for its id and ExprStatement.
  ConcurrentHashMap<uint64_t, ExprStatement *> _mapExprStat;
  // Every connection fron the client will create a session and assign a unique
  // id
  ConcurrentHashMap<uint64_t, Session *> _mapSession;
  static DbManage *_inst;
};
} // namespace storage