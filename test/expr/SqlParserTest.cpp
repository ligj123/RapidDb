#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprLogic.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/sql/Parser.h"
#include <boost/test/unit_test.hpp>

#define PRINT_FUNC(str) #str

namespace storage {
BOOST_AUTO_TEST_SUITE(SqlParserBasicTest)

BOOST_AUTO_TEST_CASE(ExprType_test) {
  BOOST_TEST("EXPR_BASE" == ExprStr[(int)ExprType::EXPR_BASE]);
  BOOST_TEST("EXPR_COUNT" == ExprStr[(int)ExprType::EXPR_COUNT]);
  BOOST_TEST("EXPR_COMP" == ExprStr[(int)ExprType::EXPR_COMP]);
  BOOST_TEST("EXPR_WHERE" == ExprStr[(int)ExprType::EXPR_WHERE]);
  BOOST_TEST("EXPR_COLUMN" == ExprStr[(int)ExprType::EXPR_COLUMN]);
  BOOST_TEST("EXPR_CREATE_DATABASE" ==
             ExprStr[(int)ExprType::EXPR_CREATE_DATABASE]);
  BOOST_TEST("EXPR_SELECT" == ExprStr[(int)ExprType::EXPR_SELECT]);
  BOOST_TEST("EXPR_LAST" == ExprStr[(int)ExprType::EXPR_LAST]);
}

BOOST_AUTO_TEST_CASE(ParserDatabase_test) {
  MString str = "create database db1;";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  const MVectorPtr<ExprStatement *> *vct = result.GetStatements();
  BOOST_TEST(vct->size() == 1);
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_CREATE_DATABASE);

  ExprCreateDatabase *expr = (ExprCreateDatabase *)(*vct)[0];
  BOOST_TEST(expr->_ifNotExist == false);
  BOOST_TEST(*expr->_dbName == "db1");

  str = "create database if not exists db1;";
  b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_CREATE_DATABASE);
  expr = (ExprCreateDatabase *)(*vct)[0];
  BOOST_TEST(expr->_ifNotExist == true);
  BOOST_TEST(*expr->_dbName == "db1");

  str = "drop database db1;";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_DROP_DATABASE);
  ExprDropDatabase *dropDb = (ExprDropDatabase *)(*vct)[0];
  BOOST_TEST(dropDb->_ifExist == false);
  BOOST_TEST(*dropDb->_dbName == "db1");

  str = "drop database if exists db1;";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_DROP_DATABASE);
  dropDb = (ExprDropDatabase *)(*vct)[0];
  BOOST_TEST(dropDb->_ifExist == true);
  BOOST_TEST(*dropDb->_dbName == "db1");

  str = "show databases";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_SHOW_DATABASES);

  str = "use db1;";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_USE_DATABASE);
  ExprUseDatabase *useDb = (ExprUseDatabase *)(*vct)[0];
  BOOST_TEST(*useDb->_dbName == "db1");

  str = "BEGIN";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_TRANSACTION);
  ExprTransaction *tran = (ExprTransaction *)(*vct)[0];
  BOOST_TEST(tran->_tranAction == TranAction::TRAN_BEGIN);

  str = "start transaction";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_TRANSACTION);
  tran = (ExprTransaction *)(*vct)[0];
  BOOST_TEST(tran->_tranAction == TranAction::TRAN_BEGIN);

  str = "ROLLBACK";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_TRANSACTION);
  tran = (ExprTransaction *)(*vct)[0];
  BOOST_TEST(tran->_tranAction == TranAction::TRAN_ROLLBACK);

  str = "COMMIT";
  Parser::Parse(str, result);
  vct = result.GetStatements();
  BOOST_TEST((*vct)[0]->GetType() == ExprType::EXPR_TRANSACTION);
  tran = (ExprTransaction *)(*vct)[0];
  BOOST_TEST(tran->_tranAction == TranAction::TRAN_COMMIT);
}

BOOST_AUTO_TEST_CASE(ParserCreateTable_test) {
  MString str = "create table if not exists t1("
                "i int AUTO_INCREMENT primary key,"
                "j varchar(100) not null default 'abcd',"
                "k char(10) null comment 'comment column',"
                "KEY idx_j_k(j,k));";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_CREATE_TABLE);
  ExprCreateTable *ct = (ExprCreateTable *)result.GetStatements()->at(0);
  BOOST_TEST(ct->_table->_dbName == nullptr);
  BOOST_TEST(*ct->_table->_tName == "t1");
  BOOST_TEST(ct->_table->_tAlias == nullptr);
  BOOST_TEST(ct->_ifNotExist);
  MVectorPtr<ExprCreateTableItem *> &vctItem = *ct->_vctItem;
  BOOST_TEST(vctItem.size() == 4);
  BOOST_TEST(vctItem[0]->GetType() == ExprType::EXPR_COLUMN_INFO);
  BOOST_TEST(vctItem[1]->GetType() == ExprType::EXPR_COLUMN_INFO);
  BOOST_TEST(vctItem[2]->GetType() == ExprType::EXPR_COLUMN_INFO);
  BOOST_TEST(vctItem[3]->GetType() == ExprType::EXPR_CONSTRAINT);

  ExprColumnItem *ci = (ExprColumnItem *)vctItem[0];
  BOOST_TEST(*ci->_colName == "i");
  BOOST_TEST(ci->_dataType == DataType::INT);
  BOOST_TEST(ci->_maxLength == -1);
  BOOST_TEST(ci->_nullable == true);
  BOOST_TEST(ci->_defaultVal == nullptr);
  BOOST_TEST(ci->_autoInc == true);
  BOOST_TEST(ci->_indexType == IndexType::PRIMARY);
  BOOST_TEST(ci->_comment == nullptr);

  ci = (ExprColumnItem *)vctItem[1];
  BOOST_TEST(*ci->_colName == "j");
  BOOST_TEST(ci->_dataType == DataType::VARCHAR);
  BOOST_TEST(ci->_maxLength == 100);
  BOOST_TEST(ci->_nullable == false);
  BOOST_TEST(*ci->_defaultVal == DataValueVarChar("abcd", 4));
  BOOST_TEST(ci->_autoInc == false);
  BOOST_TEST(ci->_indexType == IndexType::UNKNOWN);
  BOOST_TEST(ci->_comment == nullptr);

  ci = (ExprColumnItem *)vctItem[2];
  BOOST_TEST(*ci->_colName == "k");
  BOOST_TEST(ci->_dataType == DataType::FIXCHAR);
  BOOST_TEST(ci->_maxLength == 10);
  BOOST_TEST(ci->_nullable == true);
  BOOST_TEST(ci->_defaultVal == nullptr);
  BOOST_TEST(ci->_autoInc == false);
  BOOST_TEST(ci->_indexType == IndexType::UNKNOWN);
  BOOST_TEST(*ci->_comment == "comment column");

  ExprTableConstraint *tc = (ExprTableConstraint *)vctItem[3];
  BOOST_TEST(*tc->_idxName == "idx_j_k");
  BOOST_TEST(tc->_idxType == IndexType::NON_UNIQUE);
  BOOST_TEST(tc->_vctColName->size() == 2);
  BOOST_TEST(*tc->_vctColName->at(0) == "j");
  BOOST_TEST(*tc->_vctColName->at(1) == "k");
}

BOOST_AUTO_TEST_CASE(ParserDropTable_test) {
  MString str = "drop table if exists t1";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_DROP_TABLE);

  ExprDropTable *dt = (ExprDropTable *)(*result.GetStatements())[0];
  BOOST_TEST(dt->_ifExist == true);
  BOOST_TEST(dt->_table->_dbName == nullptr);
  BOOST_TEST(*dt->_table->_tName == "t1");
  BOOST_TEST(dt->_table->_tAlias == nullptr);

  str = "drop table db1.t1";
  b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_DROP_TABLE);

  dt = (ExprDropTable *)(*result.GetStatements())[0];
  BOOST_TEST(dt->_ifExist == false);
  BOOST_TEST(*dt->_table->_dbName == "db1");
  BOOST_TEST(*dt->_table->_tName == "t1");
  BOOST_TEST(dt->_table->_tAlias == nullptr);
}

BOOST_AUTO_TEST_CASE(ParserShowTables_test) {
  MString str = "show tables";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_SHOW_TABLES);

  ExprShowTables *st = (ExprShowTables *)(*result.GetStatements())[0];
  BOOST_TEST(st->_dbName == nullptr);

  str = "show tables from db1";
  b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_SHOW_TABLES);

  st = (ExprShowTables *)(*result.GetStatements())[0];
  BOOST_TEST(*st->_dbName == "db1");
}

BOOST_AUTO_TEST_CASE(ParserTrunTable_test) {
  MString str = "truncate table t1";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_TRUN_TABLE);

  ExprTrunTable *dt = (ExprTrunTable *)(*result.GetStatements())[0];
  BOOST_TEST(dt->_table->_dbName == nullptr);
  BOOST_TEST(*dt->_table->_tName == "t1");
  BOOST_TEST(dt->_table->_tAlias == nullptr);

  str = "truncate table db1.t1";
  b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() ==
             ExprType::EXPR_TRUN_TABLE);

  dt = (ExprTrunTable *)(*result.GetStatements())[0];
  BOOST_TEST(*dt->_table->_dbName == "db1");
  BOOST_TEST(*dt->_table->_tName == "t1");
  BOOST_TEST(dt->_table->_tAlias == nullptr);
}


BOOST_AUTO_TEST_CASE(ParserInsert_test) {
  MString str = "truncate table t1";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage