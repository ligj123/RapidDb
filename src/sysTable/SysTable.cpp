#include "SysTable.h"
#include "../config/Configure.h"
#include "../expr/ExprDdl.h"
#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../serv/SessionPool.h"
#include "../sql/Parser.h"
#include "../table/Table.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace storage {
const char *CREATE_DB_SQL = "create table rapid.sys_dbs("
                            "id int auto_increment primary key,"
                            "db_name varchar(50) not null,"
                            "folder varchar(100),"
                            "create_time datetime,"
                            "update_time datetime)";
const char *CREATE_TABLE_SQL = "create table rapid.sys_tables("
                               "id int auto_increment(25600,256) primary key,"
                               "db_name varchar(50) not null,"
                               "table_name varchar(50) not null,"
                               "folder varchar(100),"
                               "table_info blob(65536),"
                               "create_time datetime,"
                               "update_time datetime)";
const char *CREATE_VARS_SQL = "create table rapid.sys_vars("
                              "var_name varchar(50) primary key,"
                              "system_var boolean,"
                              "resident_memory boolean,"
                              "var_type varchar(20),"
                              "var_value blob(65536))";

const char *SYS_TABLE_SQLS[] = {CREATE_DB_SQL, CREATE_TABLE_SQL,
                                CREATE_VARS_SQL};

const char *SEL_DB_SQL = "select * from rapid.sys_dbs";
const char *SEL_TBL_SQL_ALL = "select * from rapid.sys_tables";
const char *SEL_TBL_SQL_ID = "select * from rapid.sys_tables where id=?";

bool SysTable::InitSystemTable() {
  fs::path path = Configure::GetDbRootPath() + "rapid";
  bool bCreate = false;
  if (!fs::exists(path)) {
    bool b = fs::create_directories(path);
    if (!b) {
      LOG_ERROR << "Failed to create system database path " << path.c_str();
      return false;
    }

    bCreate = true;
  } else if (fs::is_empty(path)) {
    bCreate = true;
  }

  Database *dbSys =
      new Database(0, "rapid", "rapid", MilliSecTime(), MilliSecTime());
  DatabaseManager::AddDb(dbSys);
  uint32_t tid = 0x100;

  for (int i = 0; i < 3; i++) {
    const char *sql = SYS_TABLE_SQLS[i];
    ParserResult result;
    Parser::Parse(sql, result);
    if (!result.IsValid() || result.GetStatements()->size() != 1) {
      LOG_ERROR << "Failed to parse system table ExprStatement. ErrMsg: "
                << result.ErrorMsg();
      return false;
    }

    ExprCreateTable *ect =
        dynamic_cast<ExprCreateTable *>(result.GetStatements()->at(0));
    bool b = ect->Preprocess(dbSys);
    assert(b);

    ExprTable *et = ect->_table;
    assert(*et->_dbName == dbSys->GetDbName());

    PhysTable *table = new PhysTable(dbSys, *et->_tName, tid, *et->_tName,
                                     MilliSecTime(), MilliSecTime());
    tid += 0x100;

    for (ExprColumnItem *citem : ect->_vctColumn) {
      if (citem->_autoInc) {
        table->AddColumn(*citem->_colName, citem->_dataType,
                         citem->_comment == nullptr ? "" : *citem->_comment,
                         citem->_initVal, citem->_incStep);
      } else {
        table->AddColumn(
            *citem->_colName, citem->_dataType, citem->_nullable,
            citem->_maxLength,
            citem->_comment == nullptr ? "" : *citem->_comment, Charsets::UTF8,
            citem->_defaultVal == nullptr ? nullptr
                                          : citem->_defaultVal->Clone());
      }
    }

    for (ExprTableIndex *eidx : ect->_vctIndex) {
      MVector<MString> vct;
      for (MString *cn : *(eidx->_vctColName)) {
        vct.push_back(*cn);
      }

      MString iname = (eidx->_idxName == nullptr) ? "" : *eidx->_idxName;
      table->AddIndex(eidx->_idxType, iname, vct);
    }

    if (bCreate) {
      if (!table->CreateTable()) {
        LOG_ERROR << "Failed to create system table, name: " << et->_tName;
        return false;
      }
    } else {
      if (!table->OpenTable()) {
        LOG_ERROR << "Failed to open system table, name: " << et->_tName;
        return false;
      }

      bool b = true;
      switch (i) {
      case 0:
        b = DatabaseManager::InitDb(table);
        break;
      case 1:
        b = TableManager::InitTable(table);
        break;
      case 2:
        break;
      default:
        break;
      }

      if (!b) {
        LOG_ERROR << "Failed to initliaze system table, name: " << et->_tName;
        return false;
      }
    }
    TableTaskMgr *tmgr = new TableTaskMgr(
        ThreadPool::GetMainPool(), table,
        static_cast<uint16_t>(SessionPool::GetVctSessionGroup().size()), false);
    table->SetTableTaskMgr(tmgr);
    TableManager::AddTable(table);
  }

  return true;
}

bool SysTable::LoadTable(PhysTable *table) {
  // TO DO
  return false;
}

bool SysTable::LoadSystemParameter(MString &name) {
  // TO DO
  return false;
}
} // namespace storage