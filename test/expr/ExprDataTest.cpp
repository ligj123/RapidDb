#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprFunc.h"
#include "../../src/expr/ExprLogic.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/sql/Parser.h"
#include "../../src/utils/Log.h"

#include <boost/test/unit_test.hpp>

namespace storage {

BOOST_AUTO_TEST_SUITE(ExprTest)
BOOST_AUTO_TEST_CASE(ExprConst_test) {
  ExprConst ec1(1.123);
  BOOST_TEST(ec1._val->GetDataType() == DataType::DOUBLE);
  BOOST_TEST(ec1._val->IsConstRef());

  int64_t ival = 123;
  ExprConst ec2(ival);
  BOOST_TEST(ec2._val->GetDataType() == DataType::LONG);
  BOOST_TEST(ec2._val->IsConstRef());

  MString *str = new MString("abcdefg");
  ExprConst ec3(str);
  BOOST_TEST(ec3._val->GetDataType() == DataType::VARCHAR);
  BOOST_TEST(ec3._val->IsConstRef());

  ExprConst ec4(true);
  BOOST_TEST(ec4._val->GetDataType() == DataType::BOOL);
  BOOST_TEST(ec4._val->IsConstRef());

  IDataValue *dv = new DataValueFixChar("abcdefgh", 8);
  ExprConst ec5(dv);
  BOOST_TEST((void *)ec5._val == (void *)dv);
  BOOST_TEST(ec5._val->IsConstRef());
}

BOOST_AUTO_TEST_CASE(ExprDataCalc_test) {
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

  ExprConst *ec = new ExprConst(new DataValueFixChar("abcdefgh", 8));
  IDataValue *dvRes = ec->Calc(vdParas, vdRow);
  BOOST_TEST((void *)dvRes == (void *)ec->_val);
  dvRes->DecRef();
  delete ec;

  ExprField *ef = new ExprField(nullptr, new MString("c1"));
  ef->_rowPos = 1;
  dvRes = ef->Calc(vdParas, vdRow);
  BOOST_TEST((void *)dvRes == (void *)vdRow[1]);
  dvRes->DecRef();

  ExprParameter *ep = new ExprParameter();
  ep->_paraPos = 2;
  dvRes = ep->Calc(vdParas, vdRow);
  BOOST_TEST((void *)dvRes == (void *)vdParas[2]);
  dvRes->DecRef();

  ExprAdd eadd(ef, ep);
  ef->_rowPos = 3;
  ep->_paraPos = 1;
  dvRes = eadd.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(dvRes->GetLong() == 2468);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 3;
  ep->_paraPos = 2;
  dvRes = eadd.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::DOUBLE);
  BOOST_TEST(dvRes->GetDouble() == 1235.234);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 0;
  ep->_paraPos = 3;
  dvRes = eadd.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::VARCHAR);
  const char *tmp = "abcdefghijklmnabcdefghijklmn     ";
  BOOST_TEST(
      BytesEqual(dvRes->GetBuff(), dvRes->GetDataLength(), (Byte *)tmp, 34));
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 3;
  ep->_paraPos = 4;
  dvRes = eadd.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::VARCHAR);
  const char *tmp2 = "1234abcdefghijklmn";
  BOOST_TEST(
      BytesEqual(dvRes->GetBuff(), dvRes->GetDataLength(), (Byte *)tmp2, 19));
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 3;
  ep->_paraPos = 5;
  dvRes = eadd.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(dvRes->GetLong() == 1234);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  eadd._exprLeft = nullptr;
  eadd._exprRight = nullptr;

  ExprSub esub(ef, ep);
  ef->_rowPos = 3;
  ep->_paraPos = 1;
  dvRes = esub.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(dvRes->GetLong() == 0);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 2;
  ep->_paraPos = 1;
  dvRes = esub.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::DOUBLE);
  BOOST_TEST(dvRes->GetDouble() == -1232.766);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 1;
  ep->_paraPos = 1;
  dvRes = esub.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(*dvRes == *vdRow[1]);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  esub._exprLeft = nullptr;
  esub._exprRight = nullptr;

  ExprMul emul(ep, ef);
  ef->_rowPos = 3;
  ep->_paraPos = 1;
  dvRes = emul.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(dvRes->GetLong() == 1234 * 1234);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 2;
  ep->_paraPos = 1;
  dvRes = emul.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::DOUBLE);
  BOOST_TEST(dvRes->GetDouble() == 1234 * 1.234);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 1;
  ep->_paraPos = 1;
  dvRes = emul.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(*dvRes == *vdParas[1]);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  emul._exprLeft = nullptr;
  emul._exprRight = nullptr;

  ExprDiv ediv(ep, ef);
  ef->_rowPos = 3;
  ep->_paraPos = 1;
  dvRes = ediv.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(dvRes->GetLong() == 1234 / 1234);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 2;
  ep->_paraPos = 1;
  dvRes = ediv.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::DOUBLE);
  BOOST_TEST(dvRes->GetDouble() == 1234 / 1.234);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();

  ef->_rowPos = 1;
  ep->_paraPos = 1;
  dvRes = ediv.Calc(vdParas, vdRow);
  BOOST_TEST(dvRes->GetDataType() == DataType::LONG);
  BOOST_TEST(*dvRes == *vdParas[1]);
  BOOST_TEST(dvRes->GetRef() == 1);
  dvRes->DecRef();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
