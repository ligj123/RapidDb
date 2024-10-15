#include "SysTable.h"
#include "../config/Configure.h"
#include "../expr/ExprDdl.h"
#include "../sql/Parser.h"
#include "../table/Table.h"
#include "../utils/Log.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace storage {
static string CREATE_DB_SQL = "create table rapid.schemas("
                              "id int auto_increment primary key,"
                              "db_name varchar(50) not null unique key,"
                              "db_path varchar(100),"
                              "create_time datetime,"
                              "update_time datetime)";
static string CREATE_TABLE_SQL = "create table rapid.tables("
                                 "id int auto_increment(25600,256) primary key,"
                                 "table_name varchar(50) not null unique key,"
                                 "table_info blob(65536),"
                                 "create_time datetime,"
                                 "update_time datetime)";
static string CREATE_VARS_SQL = "create table rapid.variables("
                                "var_name varchar(50) primary key,"
                                "system_var bool,"
                                "resident_memory bool,"
                                "data_type varchar(20),"
                                "value blob(65536))";

static string SYS_TABLE_SQL[] = {CREATE_DB_SQL, CREATE_TABLE_SQL,
                                 CREATE_VARS_SQL};

bool SysTable::GenerateSysTables(Database *sysDb,
                                 MStrHashMap<PhysTable *> &mapTable) {
  uint32_t tid = 0;

  for (string &str : SYS_TABLE_SQL) {
    ParserResult result;
    Parser::Parse(str.c_str(), result);
    if (result.GetStatements()->size() != 1) {
      LOG_ERROR << "Failed to parse system table ExprStatement. ErrMsg: "
                << result.ErrorMsg();
      return false;
    }

    ExprCreateTable *ect = (ExprCreateTable *)result.GetStatements()->at(0);
    bool b = ect->Preprocess();
    assert(b);

    ExprTable *et = ect->_table;
    assert(*et->_dbName == sysDb->GetDbName());

    PhysTable *table = new PhysTable(sysDb, *et->_tName, tid, MilliSecTime());
    tid += 0xff;

    for (ExprColumnItem *citem : ect->_vctColumn) {
      if (citem->_autoInc) {
        table->AddColumn(*citem->_colName, citem->_dataType,
                         citem->_comment == nullptr ? "" : *citem->_comment,
                         citem->_initVal, citem->_incStep);
      } else {
        table->AddColumn(*citem->_colName, citem->_dataType, citem->_nullable,
                         citem->_maxLength,
                         citem->_comment == nullptr ? "" : *citem->_comment,
                         Charsets::UTF8, citem->_defaultVal);
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

    if (!table->CreateTable()) {
      LOG_ERROR << "Failed to create system table, name: " << et->_tName;
    }

    mapTable.insert({table->GetFullName(), table});
  }

  return true;
}

bool SysTable::InitSystemTable() {
  fs::path path = Configure::GetDbRootPath() + "/rapid";
  if (!fs::exists(path)) {
    fs::create_directories(path);
  } else if (!fs::is_empty(path)) {
    LOG_ERROR << "The system database root path is not empty, path="
              << path.c_str();
    abort();
  }

  Database dbSys(0, path.string().c_str(), "rapid", MilliSecTime(),
                 MilliSecTime());
  MStrHashMap<PhysTable *> mapTable;
  bool b = GenerateSysTables(&dbSys, mapTable);
  if (!b) {
    LOG_ERROR << "Failed to generate system table.";
    abort();
  }

  for (auto iter = mapTable.begin(); iter != mapTable.end(); iter++) {
    iter->second->CreateTable();
  }

  return false;
}
} // namespace storage