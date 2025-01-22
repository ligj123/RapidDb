#include "../../src/expr/ExprLogic.h"

#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprFunc.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/sql/Parser.h"
#include "../../src/utils/Log.h"

#include <boost/test/unit_test.hpp>

namespace storage {

BOOST_AUTO_TEST_SUITE(ExprTest)
BOOST_AUTO_TEST_CASE(ExprLogicBase_test) {
  VectorDataValue vdParas;
  vdParas.push_back(new DataValueBool(true));
  vdParas.push_back(new DataValueLong(1234LL));
  vdParas.push_back(new DataValueDouble(1.234));
  vdParas.push_back(new DataValueFixChar("abcdefghijklmn", 14, 20));
  vdParas.push_back(new DataValueVarChar("abcdefghijklmn", 14, 20));
  vdParas.push_back(new DataValueBlob("abcdefgh", 8, 8));

  VectorDataValue vdRow;
  vdRow.push_back(new DataValueVarChar("abcdefghijklmn", 14, 20));
  vdRow.push_back(new DataValueFixChar("abcdefghijklmn", 14, 20));
  vdRow.push_back(new DataValueDouble(1.234));
  vdRow.push_back(new DataValueLong(1234LL));
  vdRow.push_back(new DataValueBool(true));

  ExprField ef(nullptr, new MString("c1"));
  ExprParameter ep;

  ExprComp exprComp(CompType::EQ, &ep, &ef);
  ef._rowPos = 0;
  ep._paraPos = 4;
  TriBool b = exprComp.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  ep._paraPos = 0;
  b = exprComp.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::Error);
  BOOST_TEST(_threadErrorMsg->getErrId() == DT_UNSUPPORT_COMPARE);

  exprComp._compType = CompType::GE;
  exprComp.Reverse();
  BOOST_TEST(exprComp._exprLeft == &ef);
  BOOST_TEST(exprComp._exprRight == &ep);

  ep._paraPos = 3;
  b = exprComp.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  exprComp._exprLeft = nullptr;
  exprComp._exprRight = nullptr;

  // ExprBetween
  ExprConst ec(int64_t(1235));
  ExprBetween exprBtn(&ef, &ep, &ec);
  b = exprBtn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::Error);

  ef._rowPos = 3;
  ep._paraPos = 2;
  b = exprBtn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  ef._rowPos = 2;
  ep._paraPos = 1;
  b = exprBtn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::False);
  exprBtn._child = nullptr;
  exprBtn._exprLeft = nullptr;
  exprBtn._exprRight = nullptr;

  // ExprInNot
  ExprArray *exprArr = new ExprArray();
  exprArr->AddElem(new DataValueLong(1));
  exprArr->AddElem(new DataValueLong(3));
  ExprInNot exprIn(&ef, exprArr);
  ef._rowPos = 3;
  b = exprIn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::False);

  exprIn._bIn = false;
  b = exprIn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  exprArr->AddElem(new DataValueLong(1234));
  b = exprIn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::False);

  exprIn._bIn = true;
  b = exprIn.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);
  exprIn._exprData = nullptr;

  // ExprIsNullNot
  ExprIsNullNot exprNull(&ef, true);
  vdRow.push_back(new DataValueNull());
  b = exprNull.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::False);
  ef._rowPos = 5;
  b = exprNull.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  exprNull._bNull = false;
  b = exprNull.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::False);

  ef._rowPos = 4;
  b = exprNull.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  // ExprNot
  ExprNot exprNot(&exprNull);
  b = exprNot.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::False);

  exprNot._child = nullptr;
  exprNull._child = nullptr;
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
