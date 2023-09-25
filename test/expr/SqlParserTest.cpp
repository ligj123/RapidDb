#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprLogic.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/sql/Parser.h"
#include <boost/test/unit_test.hpp>

#define PRINT_FUNC(str) #str

namespace storage {
BOOST_AUTO_TEST_SUITE(SqlParserTest)

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

BOOST_AUTO_TEST_CASE(ParserBasic_test) {
  MString str = "create database db1;";
  ParserResult result;
  bool b = Parser::Parse(str, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());
  const MVectorPtr<ExprStatement *> &vct = result.GetStatements();
  BOOST_TEST(vct.size() == 1);
  BOOST_TEST(vct[0]->GetType() == ExprType::EXPR_CREATE_DATABASE);

  ExprCreateDatabase *expr = (ExprCreateDatabase *)vct[0];
  BOOST_TEST(expr->_ifNotExist == false);
  BOOST_TEST(*expr->_dbName == "db1");

  // "drop database db1;", "show databases", "use db1",
  //     "create table db1.t1(c1 long primary key AUTO_INCREMENT, c2 "
  //     "varchar(100), c3 char(10), )";
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage