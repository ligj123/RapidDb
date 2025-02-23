#include "../../src/expr/ExprStatement.h"
#include "../../src/manager/DatabaseManager.h"
#include "../../src/manager/TableManager.h"
#include "../../src/statement/Statement.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>

namespace storage {
class StatementEx : public Statement {
public:
  using Statement::Statement;
  ExprType GetType() override { return ExprType::EXPR_SELECT; }
  bool IsReadonly() override { return false; }

  using Statement::ConditionConvert;
  using Statement::GenIndexSearchKey;
  using Statement::MergeAndQueryRange;
  using Statement::MergeOrQueryRange;
};

PhysTable *CreateTestTable(Database *db, const MString &tableName) {
  PhysTable *ptable =
      new PhysTable(db, tableName, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::LONG, false, -1, "primary key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c2", DataType::VARCHAR, false, 10, "Unique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c3", DataType::FIXCHAR, true, 10, "NonUnique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});
  ptable->AddIndex(IndexType::UNIQUE, "c2_unique", {"c2", "c3"});
  ptable->AddIndex(IndexType::NON_UNIQUE, "c3_non_unique", {"c3"});
  return ptable;
}

void CompareQueryResult(QueryRange &qr, const IDataValue &dvLeft,
                        const IDataValue &dvRight, bool bRange, bool incLeft,
                        bool incRight) {
  BOOST_TEST(*qr._dvLeft == dvLeft);
  BOOST_TEST(*qr._dvRight == dvRight);
  BOOST_TEST(qr._bRange == bRange);
  BOOST_TEST(qr._bIncLeft == incLeft);
  BOOST_TEST(qr._bIncRight == incRight);
}

BOOST_AUTO_TEST_SUITE(StatementTest)
BOOST_AUTO_TEST_CASE(MergeAndQueryRange_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;

  MVector<QueryRange> vctLeft;
  MVector<QueryRange> vctRight;
  vctLeft.emplace_back(new DataValueLong(INT64_MIN), new DataValueLong(100),
                       true, true, true);
  IDataValue *dv1 = new DataValueLong(200);
  vctLeft.emplace_back(dv1, dv1->AddRef(), false, true, true);
  IDataValue *dv2 = new DataValueLong(10);
  vctRight.emplace_back(dv2, dv2->AddRef(), false, true, true);
  vctRight.emplace_back(new DataValueLong(50), new DataValueLong(1000), true,
                        false, true);

  StatementEx stmt(1, 1);
  MVector<QueryRange> vctRange = stmt.MergeAndQueryRange(vctLeft, vctRight);
  BOOST_TEST(vctRange.size() == 3);
  BOOST_TEST(!vctRange[0]._bRange);
  BOOST_TEST(vctRange[0]._dvLeft->GetLong() == 10);
  BOOST_TEST(vctRange[0]._dvRight->GetLong() == 10);
  BOOST_TEST(vctRange[0]._bIncLeft);
  BOOST_TEST(vctRange[0]._bIncRight);

  BOOST_TEST(vctRange[1]._bRange);
  BOOST_TEST(vctRange[1]._dvLeft->GetLong() == 50);
  BOOST_TEST(vctRange[1]._dvRight->GetLong() == 100);
  BOOST_TEST(vctRange[1]._bIncLeft == false);
  BOOST_TEST(vctRange[1]._bIncRight == true);

  BOOST_TEST(!vctRange[2]._bRange);
  BOOST_TEST(vctRange[2]._dvLeft->GetLong() == 200);
  BOOST_TEST(vctRange[2]._dvRight->GetLong() == 200);
  BOOST_TEST(vctRange[2]._bIncLeft);
  BOOST_TEST(vctRange[2]._bIncRight);

  vctLeft.clear();
  vctLeft.emplace_back(new DataValueLong(-100), new DataValueLong(0), true,
                       true, false);

  vctRight.clear();
  vctRight.emplace_back(new DataValueLong(0), new DataValueLong(100), true,
                        true, true);
  vctRange = stmt.MergeAndQueryRange(vctLeft, vctRight);
  BOOST_TEST(vctRange.size() == 0);
}

BOOST_AUTO_TEST_CASE(MergeOrQueryRange_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;

  MVector<QueryRange> vctLeft;
  MVector<QueryRange> vctRight;
  vctLeft.emplace_back(new DataValueLong(INT64_MIN), new DataValueLong(30),
                       true, true, true);
  IDataValue *dv1 = new DataValueLong(200);
  vctLeft.emplace_back(dv1, dv1->AddRef(), false, true, true);
  vctLeft.emplace_back(new DataValueLong(1000), new DataValueLong(2000), true,
                       false, true);

  IDataValue *dv2 = new DataValueLong(10);
  vctRight.emplace_back(dv2, dv2->AddRef(), false, true, true);
  vctRight.emplace_back(new DataValueLong(50), new DataValueLong(1000), true,
                        false, false);

  StatementEx stmt(1, 1);
  stmt.MergeOrQueryRange(vctLeft, vctRight);
  BOOST_TEST(vctLeft.size() == 3);
  BOOST_TEST(vctLeft[0]._bRange);
  BOOST_TEST(vctLeft[0]._dvLeft->GetLong() == INT64_MIN);
  BOOST_TEST(vctLeft[0]._dvRight->GetLong() == 30);
  BOOST_TEST(vctLeft[0]._bIncLeft);
  BOOST_TEST(vctLeft[0]._bIncRight);

  BOOST_TEST(vctLeft[1]._bRange);
  BOOST_TEST(vctLeft[1]._dvLeft->GetLong() == 50);
  BOOST_TEST(vctLeft[1]._dvRight->GetLong() == 1000);
  BOOST_TEST(vctLeft[1]._bIncLeft == false);
  BOOST_TEST(vctLeft[1]._bIncRight == false);

  BOOST_TEST(vctLeft[2]._bRange);
  BOOST_TEST(vctLeft[2]._dvLeft->GetLong() == 1000);
  BOOST_TEST(vctLeft[2]._dvRight->GetLong() == 2000);
  BOOST_TEST(vctLeft[2]._bIncLeft == false);
  BOOST_TEST(vctLeft[2]._bIncRight == true);
}

BOOST_AUTO_TEST_CASE(ConditionConvert_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;

  DataValueLong dv(1000);
  VectorDataValue vctEmpty;
  StatementEx stmt(1, 1);
  ExprComp exprCmp(CompType::EQ,
                   new ExprField(new MString("t1"), new MString("c1")),
                   new ExprConst(int64_t(1000)));
  MVector<QueryRange> vctQR = stmt.ConditionConvert(&exprCmp, vctEmpty);
  BOOST_TEST(vctQR.size() == 1);
  CompareQueryResult(vctQR[0], dv, dv, false, true, true);

  exprCmp._compType = CompType::GT;
  vctQR = stmt.ConditionConvert(&exprCmp, vctEmpty);
  CompareQueryResult(vctQR[0], dv, DataValueLong(INT64_MAX), true, false, true);

  exprCmp._compType = CompType::GE;
  vctQR = stmt.ConditionConvert(&exprCmp, vctEmpty);
  CompareQueryResult(vctQR[0], dv, DataValueLong(INT64_MAX), true, true, true);

  exprCmp._compType = CompType::LT;
  vctQR = stmt.ConditionConvert(&exprCmp, vctEmpty);
  CompareQueryResult(vctQR[0], DataValueLong(INT64_MIN), dv, true, true, false);

  exprCmp._compType = CompType::LE;
  vctQR = stmt.ConditionConvert(&exprCmp, vctEmpty);
  CompareQueryResult(vctQR[0], DataValueLong(INT64_MIN), dv, true, true, true);

  exprCmp._compType = CompType::NE;
  vctQR = stmt.ConditionConvert(&exprCmp, vctEmpty);
  BOOST_TEST(vctQR.size() == 2);
  CompareQueryResult(vctQR[0], DataValueLong(INT64_MIN), dv, true, true, false);
  CompareQueryResult(vctQR[1], dv, DataValueLong(INT64_MAX), true, false, true);

  ExprArray *exprArr = new ExprArray();
  exprArr->AddElem(new DataValueLong(10));
  exprArr->AddElem(new DataValueLong(20));
  exprArr->AddElem(new DataValueLong(30));
  ExprInNot exprIn(new ExprField(new MString("t1"), new MString("c1")),
                   exprArr);
  vctQR = stmt.ConditionConvert(&exprIn, vctEmpty);

  BOOST_TEST(vctQR.size() == 3);
  CompareQueryResult(vctQR[0], DataValueLong(10), DataValueLong(10), false,
                     true, true);
  CompareQueryResult(vctQR[1], DataValueLong(20), DataValueLong(20), false,
                     true, true);
  CompareQueryResult(vctQR[2], DataValueLong(30), DataValueLong(30), false,
                     true, true);

  ExprBetween exprBwn(new ExprField(new MString("t1"), new MString("c1")),
                      new ExprConst(int64_t(0)), new ExprConst(int64_t(1000)));
  vctQR = stmt.ConditionConvert(&exprBwn, vctEmpty);
  CompareQueryResult(vctQR[0], DataValueLong(0), dv, true, true, true);

  ExprAnd exprAnd;
  exprAnd._vctChild.push_back(new ExprComp(
      CompType::LT, new ExprField(new MString("t1"), new MString("c1")),
      new ExprConst(int64_t(1000))));
  exprAnd._vctChild.push_back(new ExprComp(
      CompType::GE, new ExprField(new MString("t1"), new MString("c1")),
      new ExprConst(int64_t(100))));
  exprAnd._vctChild.push_back(
      new ExprBetween(new ExprField(new MString("t1"), new MString("c1")),
                      new ExprConst(int64_t(0)), new ExprConst(int64_t(500))));
  vctQR = stmt.ConditionConvert(&exprAnd, vctEmpty);
  BOOST_TEST(vctQR.size() == 1);
  CompareQueryResult(vctQR[0], DataValueLong(100), DataValueLong(500), true,
                     true, true);

  exprAnd._vctChild.push_back(new ExprBetween(
      new ExprField(new MString("t1"), new MString("c1")),
      new ExprConst(int64_t(1000)), new ExprConst(int64_t(1500))));
  vctQR = stmt.ConditionConvert(&exprAnd, vctEmpty);
  BOOST_TEST(vctQR.size() == 0);

  ExprOr exprOr;
  exprOr._vctChild.push_back(new ExprComp(
      CompType::LT, new ExprField(new MString("t1"), new MString("c1")),
      new ExprConst(int64_t(10))));
  exprOr._vctChild.push_back(
      new ExprBetween(new ExprField(new MString("t1"), new MString("c1")),
                      new ExprConst(int64_t(0)), new ExprConst(int64_t(500))));
  vctQR = stmt.ConditionConvert(&exprOr, vctEmpty);
  BOOST_TEST(vctQR.size() == 1);
  CompareQueryResult(vctQR[0], DataValueLong(INT64_MIN), DataValueLong(500),
                     true, true, true);
}

BOOST_AUTO_TEST_CASE(IndexScan_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const MString TABLE_NAME = "testTable";
  const MString DB_NAME = "testDb";
  Database *db = new Database(1, ROOT_PATH.c_str(), DB_NAME, MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *ptable = CreateTestTable(db, TABLE_NAME);
  TableManager::AddTable(DB_NAME + "." + TABLE_NAME, ptable);

  ExprDelete exprDel;
  exprDel._exprTable =
      new ExprTable(new MString(DB_NAME), new MString(TABLE_NAME), nullptr);

  // Test CompType::EQ
  ExprComp *exprComp = new ExprComp(
      CompType::EQ, new ExprField(new MString("t1"), new MString("c1")),
      new ExprConst(int64_t(10)));
  ExprWhere *exprWhere = new ExprWhere(exprComp);
  exprDel._exprWhere = exprWhere;
  bool b = exprDel.Preprocess(db);
  BOOST_TEST(b);
  BOOST_TEST(exprWhere->_exprLogic == nullptr);
  BOOST_TEST(exprWhere->_indexSearch->_bPointQuery);
  BOOST_TEST(exprWhere->_indexSearch->_indexPos == 0);
  BOOST_TEST(exprWhere->_indexSearch->_idxLogic == nullptr);
  BOOST_TEST(exprWhere->_indexSearch->_vctPointCond->size() == 1);
  ExprLogic *logic = exprWhere->_indexSearch->_vctPointCond->at(0);
  BOOST_TEST(logic == exprComp);
  delete exprWhere;

  exprComp = new ExprComp(CompType::EQ,
                          new ExprField(new MString("t1"), new MString("c1")),
                          new ExprConst(int64_t(10)));
  ExprComp *exprComp2 = new ExprComp(
      CompType::EQ, new ExprField(new MString("t1"), new MString("c2")),
      new ExprConst(new MString("abcdefg")));
  ExprAnd *exprAnd = new ExprAnd();
  exprAnd->_vctChild.push_back(exprComp);
  exprAnd->_vctChild.push_back(exprComp2);
  exprWhere = new ExprWhere(exprAnd);
  exprDel._exprWhere = exprWhere;
  b = exprDel.Preprocess(db);
  BOOST_TEST(b);
  BOOST_TEST(exprWhere->_exprLogic == exprComp2);
  BOOST_TEST(exprWhere->_indexSearch->_bPointQuery);
  BOOST_TEST(exprWhere->_indexSearch->_indexPos == 0);
  BOOST_TEST(exprWhere->_indexSearch->_idxLogic == nullptr);
  BOOST_TEST(exprWhere->_indexSearch->_vctPointCond->size() == 1);
  logic = exprWhere->_indexSearch->_vctPointCond->at(0);
  BOOST_TEST(logic == exprComp);
  delete exprWhere;

  exprComp = new ExprComp(CompType::EQ,
                          new ExprField(new MString("t1"), new MString("c3")),
                          new ExprConst(new MString("qqqq")));
  exprComp2 = new ExprComp(CompType::EQ,
                           new ExprField(new MString("t1"), new MString("c2")),
                           new ExprConst(new MString("abcdefg")));
  exprAnd = new ExprAnd();
  exprAnd->_vctChild.push_back(exprComp);
  exprAnd->_vctChild.push_back(exprComp2);
  exprWhere = new ExprWhere(exprAnd);
  exprDel._exprWhere = exprWhere;
  b = exprDel.Preprocess(db);
  BOOST_TEST(b);

  BOOST_TEST(exprWhere->_indexSearch->_bPointQuery);
  BOOST_TEST(exprWhere->_indexSearch->_indexPos == 1);
  BOOST_TEST(exprWhere->_indexSearch->_idxLogic == nullptr);
  BOOST_TEST(exprWhere->_indexSearch->_vctPointCond->size() == 2);
  auto vct = exprWhere->_indexSearch->_vctPointCond;
  BOOST_TEST(vct->at(0) == exprComp2);
  BOOST_TEST(vct->at(1) == exprComp);
  delete exprWhere;

  // Test NOT CompType::EQ
  exprComp = new ExprComp(CompType::GT,
                          new ExprField(new MString("t1"), new MString("c1")),
                          new ExprConst(int64_t(10)));
  exprWhere = new ExprWhere(exprComp);
  exprDel._exprWhere = exprWhere;
  b = exprDel.Preprocess(db);
  BOOST_TEST(b);
  BOOST_TEST(exprWhere->_exprLogic == nullptr);
  BOOST_TEST(exprWhere->_indexSearch->_bPointQuery == false);
  BOOST_TEST(exprWhere->_indexSearch->_idxLogic == exprComp);
  BOOST_TEST(exprWhere->_indexSearch->_vctPointCond == nullptr);
  delete exprWhere;

  exprComp = new ExprComp(CompType::LE,
                          new ExprField(new MString("t1"), new MString("c1")),
                          new ExprConst(int64_t(10)));
  exprComp2 = new ExprComp(CompType::GE,
                           new ExprField(new MString("t1"), new MString("c2")),
                           new ExprConst(new MString("abcdefg")));
  exprAnd = new ExprAnd();
  exprAnd->_vctChild.push_back(exprComp);
  exprAnd->_vctChild.push_back(exprComp2);
  exprWhere = new ExprWhere(exprAnd);
  exprDel._exprWhere = exprWhere;
  b = exprDel.Preprocess(db);
  BOOST_TEST(b);
  BOOST_TEST(exprWhere->_exprLogic == exprComp2);
  BOOST_TEST(exprWhere->_indexSearch->_bPointQuery == false);
  BOOST_TEST(exprWhere->_indexSearch->_indexPos == 0);
  BOOST_TEST(exprWhere->_indexSearch->_idxLogic == exprComp);
  BOOST_TEST(exprWhere->_indexSearch->_vctPointCond == nullptr);

  TableManager::ClearTable();
  DatabaseManager::ClearDB();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
