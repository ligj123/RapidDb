#include "SysTable.h"

namespace storage {

static string CREATE_DB_SQL = "create table schemas("
                              "id int auto_increment primary key,"
                              "db_name varchar(50) not null unique key,"
                              "db_path varchar(100),"
                              "create_time datetime,"
                              "update_time datetime)";
static string CREATE_TABLE_SQL = "create table tables("
                                 "id int auto_increment(256,256) primary key,"
                                 "table_name varchar(50) not null unique key,"
                                 "table_desc varchar(200),"
                                 "table_info blob(65536),"
                                 "create_time datetime,"
                                 "update_time datetime)";
static string CREATE_VARS_SQL = "create table variables("
                                "var_name varchar(50) primary key,"
                                "system_var bool,"
                                "resident_memory bool,"
                                "data_type varchar(20),"
                                "value blob(65536))";

static string SYS_TABLE_SQL[] = {CREATE_DB_SQL, CREATE_TABLE_SQL,
                                 CREATE_VARS_SQL};

bool Systable::CreateSystemTable() {
  return false;
}
} // namespace storage