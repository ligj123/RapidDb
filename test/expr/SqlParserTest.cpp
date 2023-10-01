#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprFunc.h"
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
  MString str = "insert into t1 values(100, 1.5, 'abcdefg', true, null)";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_INSERT);

  ExprInsert *ei = (ExprInsert *)(*result.GetStatements())[0];
  BOOST_TEST(ei->_exprTable->_joinType == JoinType::JOIN_NULL);
  BOOST_TEST(ei->_exprTable->_dbName == nullptr);
  BOOST_TEST(*ei->_exprTable->_tName == "t1");
  BOOST_TEST(ei->_exprTable->_tAlias == nullptr);
  BOOST_TEST(ei->_vctCol == nullptr);
  BOOST_TEST(ei->_bUpsert == false);
  BOOST_TEST(ei->_vctRowData->size() == 1);

  MVectorPtr<ExprElem *> &vct = *ei->_vctRowData->at(0);
  BOOST_TEST(vct[0]->GetType() == ExprType::EXPR_CONST);
  BOOST_TEST(((ExprConst *)vct[0])->_val->GetLong() == 100);
  BOOST_TEST(((ExprConst *)vct[1])->_val->GetDouble() == 1.5);
  BOOST_TEST(((ExprConst *)vct[2])->_val->GetDataType() == DataType::VARCHAR);
  BOOST_TEST(*((ExprConst *)vct[2])->_val == DataValueVarChar("abcdefg", 7));
  BOOST_TEST(*((ExprConst *)vct[3])->_val == DataValueBool(true));
  BOOST_TEST(((ExprConst *)vct[4])->_val->GetDataType() == DataType::VAL_NULL);

  str = "upsert into t1(a, b, c, d, e) values(100, 'abcdefg', c+10, ?, ?)";
  b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());

  ei = (ExprInsert *)(*result.GetStatements())[0];
  BOOST_TEST(ei->_bUpsert == true);
  BOOST_TEST(ei->_vctCol->size() == 5);
  BOOST_TEST(*ei->_vctCol->at(0)->_name == "a");
  BOOST_TEST(ei->_vctCol->at(0)->_exprElem == nullptr);
  BOOST_TEST(ei->_vctCol->at(0)->_alias == nullptr);
  BOOST_TEST(*ei->_vctCol->at(1)->_name == "b");
  BOOST_TEST(*ei->_vctCol->at(2)->_name == "c");
  BOOST_TEST(*ei->_vctCol->at(3)->_name == "d");
  BOOST_TEST(*ei->_vctCol->at(4)->_name == "e");

  MVectorPtr<ExprElem *> &vct2 = *ei->_vctRowData->at(0);
  BOOST_TEST(((ExprConst *)vct2[0])->_val->GetLong() == 100);
  BOOST_TEST(*((ExprConst *)vct2[1])->_val == DataValueVarChar("abcdefg", 7));
  BOOST_TEST(vct2[2]->GetType() == ExprType::EXPR_ADD);
  BOOST_TEST(((ExprAdd *)vct2[2])->_exprLeft->GetType() ==
             ExprType::EXPR_FIELD);
  ExprField *ef = (ExprField *)((ExprAdd *)vct2[2])->_exprLeft;
  BOOST_TEST(*ef->_colName == "c");
  BOOST_TEST(ef->_tableName == nullptr);
  ExprConst *ec = (ExprConst *)((ExprAdd *)vct2[2])->_exprRight;
  BOOST_TEST(ec->_val->GetLong() == 10);

  BOOST_TEST(vct2[3]->GetType() == ExprType::EXPR_PARAMETER);
  BOOST_TEST(((ExprParameter *)vct2[3])->_paraPos == 0);

  BOOST_TEST(vct2[4]->GetType() == ExprType::EXPR_PARAMETER);
  BOOST_TEST(((ExprParameter *)vct2[4])->_paraPos == 1);
}

BOOST_AUTO_TEST_CASE(ParserDelete_test) {
  MString str = "delete from t1";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_DELETE);

  ExprDelete *ed = (ExprDelete *)(*result.GetStatements())[0];
  BOOST_TEST(*ed->_exprTable->_tName == "t1");
  BOOST_TEST(ed->_exprWhere == nullptr);
  BOOST_TEST(ed->_exprOrderBy == nullptr);
  BOOST_TEST(ed->_exprLimit == nullptr);

  str = "delete from t1 where a>10 and b<100 OR c==? ORDER BY a desc, b "
        "limit 10";
  b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_DELETE);

  ed = (ExprDelete *)(*result.GetStatements())[0];
  BOOST_TEST(*ed->_exprTable->_tName == "t1");

  ExprWhere *ew = ed->_exprWhere;
  BOOST_TEST(ew->GetType() == ExprType::EXPR_WHERE);
  BOOST_TEST(ew->_exprLogic->GetType() == ExprType::EXPR_OR);

  ExprOr *eo = (ExprOr *)ew->_exprLogic;
  BOOST_TEST(eo->_vctChild[0]->GetType() == ExprType::EXPR_AND);
  BOOST_TEST(eo->_vctChild[1]->GetType() == ExprType::EXPR_COMP);

  ExprAnd *ea = (ExprAnd *)eo->_vctChild[0];
  BOOST_TEST(ea->_vctChild[0]->GetType() == ExprType::EXPR_COMP);
  BOOST_TEST(ea->_vctChild[1]->GetType() == ExprType::EXPR_COMP);

  ExprComp *ec = (ExprComp *)ea->_vctChild[0];
  BOOST_TEST(ec->_compType == CompType::GT);
  BOOST_TEST(ec->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ec->_exprRight->GetType() == ExprType::EXPR_CONST);
  BOOST_TEST(((ExprField *)ec->_exprLeft)->_colName->compare("a") == 0);
  BOOST_TEST(*((ExprConst *)ec->_exprRight)->_val == DataValueLong(10));

  ec = (ExprComp *)ea->_vctChild[1];
  BOOST_TEST(ec->_compType == CompType::LT);
  BOOST_TEST(ec->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ec->_exprRight->GetType() == ExprType::EXPR_CONST);
  BOOST_TEST(((ExprField *)ec->_exprLeft)->_colName->compare("b") == 0);
  BOOST_TEST(*((ExprConst *)ec->_exprRight)->_val == DataValueLong(100));

  ec = (ExprComp *)eo->_vctChild[1];
  BOOST_TEST(ec->_compType == CompType::EQ);
  BOOST_TEST(ec->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ec->_exprRight->GetType() == ExprType::EXPR_PARAMETER);
  BOOST_TEST(((ExprField *)ec->_exprLeft)->_colName->compare("c") == 0);

  ExprOrderBy *eb = ed->_exprOrderBy;
  BOOST_TEST(eb->GetType() == ExprType::EXPR_ORDER_BY);
  BOOST_TEST(eb->_vctItem->size() == 2);
  BOOST_TEST(eb->_vctItem->at(0)->_colName->compare("a") == 0);
  BOOST_TEST(eb->_vctItem->at(0)->_direct == false);
  BOOST_TEST(eb->_vctItem->at(1)->_colName->compare("b") == 0);
  BOOST_TEST(eb->_vctItem->at(1)->_direct == true);

  ExprLimit *el = ed->_exprLimit;
  BOOST_TEST(el->_rowOffset == 0);
  BOOST_TEST(el->_rowCount == 10);
}

BOOST_AUTO_TEST_CASE(ParserUpdate_test) {
  MString str = "update db1.t1 set a=?, b=?, c=a+b where a=? && b=? && c>=?";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_UPDATE);
  BOOST_TEST(result.GetVctPara()->size() == 5);

  ExprUpdate *eu = (ExprUpdate *)(*result.GetStatements())[0];
  BOOST_TEST(*eu->_exprTable->_dbName == "db1");
  BOOST_TEST(*eu->_exprTable->_tName == "t1");
  BOOST_TEST(eu->_exprOrderBy == nullptr);
  BOOST_TEST(eu->_exprLimit == nullptr);

  ExprWhere *ew = eu->_exprWhere;
  BOOST_TEST(ew->_exprLogic->GetType() == ExprType::EXPR_AND);

  ExprAnd *ea = (ExprAnd *)ew->_exprLogic;
  ExprComp *ec = (ExprComp *)ea->_vctChild[0];
  BOOST_TEST(ec->_compType == CompType::EQ);
  BOOST_TEST(ec->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ec->_exprRight->GetType() == ExprType::EXPR_PARAMETER);
  BOOST_TEST(((ExprField *)ec->_exprLeft)->_colName->compare("a") == 0);
  BOOST_TEST(((ExprParameter *)ec->_exprRight)->_paraPos == 2);

  ec = (ExprComp *)ea->_vctChild[1];
  BOOST_TEST(ec->_compType == CompType::EQ);
  BOOST_TEST(ec->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ec->_exprRight->GetType() == ExprType::EXPR_PARAMETER);
  BOOST_TEST(((ExprField *)ec->_exprLeft)->_colName->compare("b") == 0);
  BOOST_TEST(((ExprParameter *)ec->_exprRight)->_paraPos == 3);

  ec = (ExprComp *)ea->_vctChild[2];
  BOOST_TEST(ec->_compType == CompType::GE);
  BOOST_TEST(ec->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ec->_exprRight->GetType() == ExprType::EXPR_PARAMETER);
  BOOST_TEST(((ExprField *)ec->_exprLeft)->_colName->compare("c") == 0);
  BOOST_TEST(((ExprParameter *)ec->_exprRight)->_paraPos == 4);

  MVectorPtr<ExprColumn *> *vctCol = eu->_vctCol;
  BOOST_TEST(vctCol->size() == 3);

  ExprColumn *col = vctCol->at(0);
  BOOST_TEST(col->_name->compare("a") == 0);
  BOOST_TEST(col->_exprElem->GetType() == ExprType::EXPR_PARAMETER);

  col = vctCol->at(1);
  BOOST_TEST(col->_name->compare("b") == 0);
  BOOST_TEST(col->_exprElem->GetType() == ExprType::EXPR_PARAMETER);

  col = vctCol->at(2);
  BOOST_TEST(col->_name->compare("c") == 0);
  BOOST_TEST(col->_exprElem->GetType() == ExprType::EXPR_ADD);

  ExprAdd *add = (ExprAdd *)col->_exprElem;
  BOOST_TEST(add->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(add->_exprRight->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(((ExprField *)add->_exprLeft)->_colName->compare("a") == 0);
  BOOST_TEST(((ExprField *)add->_exprRight)->_colName->compare("b") == 0);
}

BOOST_AUTO_TEST_CASE(ParserSelect_test) {
  MString str = "select * from t1";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_SELECT);

  ExprSelect *es = (ExprSelect *)(*result.GetStatements())[0];
  BOOST_TEST(es->_bDistinct == false);
  BOOST_TEST(es->_vctTable->size() == 1);
  BOOST_TEST(es->_vctTable->at(0)->_tName->compare("t1") == 0);
  BOOST_TEST(es->_vctCol == nullptr);
  BOOST_TEST(es->_exprWhere == nullptr);
  BOOST_TEST(es->_exprOn == nullptr);
  BOOST_TEST(es->_exprGroupBy == nullptr);
  BOOST_TEST(es->_exprOrderBy == nullptr);
  BOOST_TEST(es->_exprLimit == nullptr);
  BOOST_TEST(es->_lockType == LockType::NO_LOCK);

  str = "select func(c1, c2) as f";
  Parser::Parse(str, result);
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_SELECT);
  es = (ExprSelect *)(*result.GetStatements())[0];
  BOOST_TEST(es->_vctTable == nullptr);
  BOOST_TEST(es->_vctCol->size() == 1);
  ExprColumn *ec = es->_vctCol->at(0);
  BOOST_TEST(ec->GetType() == ExprType::EXPR_COLUMN);
  BOOST_TEST(ec->_name == nullptr);
  BOOST_TEST(ec->_alias->compare("f") == 0);
  BOOST_TEST(ec->_exprElem->GetType() == ExprType::EXPR_FUNCTION);
  ExprFunc *ef = (ExprFunc *)ec->_exprElem;
  BOOST_TEST(ef->_funcName->compare("func") == 0);
  BOOST_TEST(ef->_vctPara->size() == 2);
  BOOST_TEST(ef->_vctPara->at(0)->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(((ExprField *)ef->_vctPara->at(0))->_colName->compare("c1") == 0);
  BOOST_TEST(ef->_vctPara->at(1)->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(((ExprField *)ef->_vctPara->at(1))->_colName->compare("c2") == 0);

  str = "select distinct t1.c1 as a, t2.c2+100 as b, count(t3.c3) as c from "
        "db1.table1 as t1 inner table2 as t2 left table3 as t3 where t1.c1>10 "
        "on t1.id=t2.id && t2.id=t3.id group by a,b having c>5 order by a "
        "desc, b limit 100,200 for update";
  Parser::Parse(str, result);
  BOOST_TEST(result.GetStatements()->at(0)->GetType() == ExprType::EXPR_SELECT);

  es = (ExprSelect *)(*result.GetStatements())[0];
  BOOST_TEST(es->_bDistinct == true);
  BOOST_TEST(es->_vctCol->size() == 3);

  ec = es->_vctCol->at(0);
  BOOST_TEST(ec->_exprElem->GetType() == ExprType::EXPR_FIELD);
  ExprField *ed = (ExprField *)ec->_exprElem;
  BOOST_TEST(ed->_tableName->compare("t1") == 0);
  BOOST_TEST(ed->_colName->compare("c1") == 0);
  BOOST_TEST(ec->_alias->compare("a") == 0);

  ec = es->_vctCol->at(1);
  BOOST_TEST(ec->_exprElem->GetType() == ExprType::EXPR_ADD);
  ExprAdd *ea = (ExprAdd *)ec->_exprElem;
  BOOST_TEST(ea->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  BOOST_TEST(ea->_exprRight->GetType() == ExprType::EXPR_CONST);
  ed = (ExprField *)ea->_exprLeft;
  BOOST_TEST(ed->_tableName->compare("t2") == 0);
  BOOST_TEST(ed->_colName->compare("c2") == 0);
  ExprConst *ect = (ExprConst *)ea->_exprRight;
  BOOST_TEST(((IDataValue *)ect->_val)->GetLong() == 100);
  BOOST_TEST(ec->_alias->compare("b") == 0);

  ec = es->_vctCol->at(2);
  BOOST_TEST(ec->_exprElem->GetType() == ExprType::EXPR_COUNT);
  ExprCount *eo = (ExprCount *)ec->_exprElem;
  BOOST_TEST(eo->_bStar == false);
  BOOST_TEST(eo->_exprData->GetType() == ExprType::EXPR_FIELD);
  ed = (ExprField *)eo->_exprData;
  BOOST_TEST(ed->_tableName->compare("t3") == 0);
  BOOST_TEST(ed->_colName->compare("c3") == 0);
  BOOST_TEST(ec->_alias->compare("c") == 0);

  BOOST_TEST(es->_vctTable->size() == 3);
  ExprTable *et = es->_vctTable->at(0);
  BOOST_TEST(et->_dbName->compare("db1") == 0);
  BOOST_TEST(et->_joinType == JoinType::JOIN_NULL);
  BOOST_TEST(et->_tName->compare("table1") == 0);
  BOOST_TEST(et->_tAlias->compare("t1") == 0);

  et = es->_vctTable->at(1);
  BOOST_TEST(et->_dbName == nullptr);
  BOOST_TEST(et->_joinType == JoinType::INNER_JOIN);
  BOOST_TEST(et->_tName->compare("table2") == 0);
  BOOST_TEST(et->_tAlias->compare("t2") == 0);

  et = es->_vctTable->at(2);
  BOOST_TEST(et->_dbName == nullptr);
  BOOST_TEST(et->_joinType == JoinType::LEFT_JOIN);
  BOOST_TEST(et->_tName->compare("table3") == 0);
  BOOST_TEST(et->_tAlias->compare("t3") == 0);

  BOOST_TEST(es->_exprWhere->_exprLogic->GetType() == ExprType::EXPR_COMP);
  ExprComp *em = (ExprComp *)es->_exprWhere->_exprLogic;
  BOOST_TEST(em->_compType == CompType::GT);
  BOOST_TEST(em->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  ed = (ExprField *)em->_exprLeft;
  BOOST_TEST(ed->_tableName->compare("t1") == 0);
  BOOST_TEST(ed->_colName->compare("c1") == 0);
  ect = (ExprConst *)em->_exprRight;
  BOOST_TEST(((IDataValue *)ect->_val)->GetLong() == 10);

  BOOST_TEST(es->_exprOn->_exprLogic->GetType() == ExprType::EXPR_AND);
  ExprAnd *en = (ExprAnd *)es->_exprOn->_exprLogic;
  BOOST_TEST(en->_vctChild.size() == 2);
  BOOST_TEST(en->_vctChild[0]->GetType() == ExprType::EXPR_COMP);
  BOOST_TEST(en->_vctChild[1]->GetType() == ExprType::EXPR_COMP);

  BOOST_TEST(es->_exprGroupBy->GetType() == ExprType::EXPR_GROUP_BY);
  BOOST_TEST(es->_exprGroupBy->_vctColName->size() == 2);
  BOOST_TEST(es->_exprGroupBy->_vctColName->at(0)->compare("a") == 0);
  BOOST_TEST(es->_exprGroupBy->_vctColName->at(1)->compare("b") == 0);

  ExprHaving *eh = es->_exprGroupBy->_exprHaving;
  BOOST_TEST(eh->GetType() == ExprType::EXPR_HAVING);
  BOOST_TEST(eh->_exprLogic->GetType() == ExprType::EXPR_COMP);

  em = (ExprComp *)eh->_exprLogic;
  BOOST_TEST(em->_compType == CompType::GT);
  BOOST_TEST(em->_exprLeft->GetType() == ExprType::EXPR_FIELD);
  ed = (ExprField *)em->_exprLeft;
  BOOST_TEST(ed->_colName->compare("c") == 0);
  ect = (ExprConst *)em->_exprRight;
  BOOST_TEST(((IDataValue *)ect->_val)->GetLong() == 5);

  BOOST_TEST(es->_exprOrderBy->GetType() == ExprType::EXPR_ORDER_BY);
  BOOST_TEST(es->_exprOrderBy->_vctItem->size() == 2);
  ExprOrderItem *ei = es->_exprOrderBy->_vctItem->at(0);
  BOOST_TEST(ei->_colName->compare("a") == 0);
  BOOST_TEST(ei->_direct == false);
  ei = es->_exprOrderBy->_vctItem->at(1);
  BOOST_TEST(ei->_colName->compare("b") == 0);
  BOOST_TEST(ei->_direct == true);

  BOOST_TEST(es->_exprLimit->GetType() == ExprType::EXPR_LIMIT);
  BOOST_TEST(es->_exprLimit->_rowOffset == 100);
  BOOST_TEST(es->_exprLimit->_rowCount == 200);

  BOOST_TEST(es->_lockType == LockType::WRITE_LOCK);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage