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
                                 "table_desc varchar(200),"
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
                                 MHashMap<MString, PhyTable *> &hmap) {
  uint32_t tid = 0;

  for (string &str : SYS_TABLE_SQL) {
    ParserResult result;
    Parser::Parse(str.c_str(), result);
    assert(result.GetStatements()->size() == 1);

    ExprCreateTable *ect = (ExprCreateTable *)result.GetStatements()->at(0);
    bool b = ect->Preprocess();
    assert(b);

    ExprTable *et = ect->_table;
    assert(*et->_dbName == sysDb->GetDbName());

    PhysTable *table = new PhysTable(sysDb, *et->_tName, tid, MilliSecTime());
  }
}

bool SysTable::CreateSystemTable() {
  fs::path path = Configure::GetDbRootPath() + "/sys";
  if (!fs::exists(path)) {
    fs::create_directories(path);
  } else if (!fs::is_empty(path)) {
    LOG_ERROR << "The system database root path is not empty, path="
              << path.c_str();
    abort();
  }

  for (string &str : SYS_TABLE_SQL) {
    ParserResult result;
    Parser::Parse(str.c_str(), result);
    assert(result.GetStatements()->size() == 1);

    ExprCreateTable *ect = (ExprCreateTable *)result.GetStatements()->at(0);
    bool b = ect->Preprocess();
    assert(b);
  }

  return false;
}
} // namespace storage