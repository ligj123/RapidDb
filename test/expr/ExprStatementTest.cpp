#include "../../src/expr/ExprStatement.h"

#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprFunc.h"
#include "../../src/expr/ExprLogic.h"
#include "../../src/manager/DatabaseManager.h"
#include "../../src/manager/TableManager.h"
#include "../../src/sql/Parser.h"
#include "../../src/utils/Log.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>

namespace storage {
void GenerateParas(VectorDataValue &vdParas) {
  vdParas.push_back(new DataValueBool(true));
  vdParas.push_back(new DataValueLong(1234LL));
  vdParas.push_back(new DataValueDouble(1.234));
  vdParas.push_back(new DataValueFixChar("abcdefghijklmn", 14, 20));
  vdParas.push_back(new DataValueVarChar("abcdefghijklmn", 14, 20));
  vdParas.push_back(new DataValueBlob("abcdefgh", 8, 8));
};

void GenerateRows(VectorDataValue &vdRow) {
  vdRow.push_back(new DataValueVarChar("abcdefghijklmn", 14, 20));
  vdRow.push_back(new DataValueFixChar("abcdefghijklmn", 14, 20));
  vdRow.push_back(new DataValueDouble(1.234));
  vdRow.push_back(new DataValueLong(1234LL));
  vdRow.push_back(new DataValueBool(true));
};

PhysTable *CreateTable(Database &db, const MString &tableName) {
  PhysTable *ptable =
      new PhysTable(&db, tableName, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::FIXCHAR, false, 1000, "primary key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c2", DataType::VARCHAR, false, 1000, "Unique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c3", DataType::FIXCHAR, true, 50, "NonUnique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});
  ptable->AddIndex(IndexType::UNIQUE, "c2_unique", {"c2"});
  ptable->AddIndex(IndexType::NON_UNIQUE, "c3_non_unique", {"c3"});

  return ptable;
}

BOOST_AUTO_TEST_SUITE(ExprTest)
BOOST_AUTO_TEST_CASE(ExprHaving_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database db(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(), MicroSecTime());
  PhysTable *table = CreateTable(db, "testTable");

  ExprHaving exprHaving(nullptr);
  TriBool b = exprHaving.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  ExprField *ef = new ExprField(new MString("testTable"), new MString("c2"));
  ExprParameter *ep = new ExprParameter();
  ep->_paraPos = 4;
  exprHaving._exprLogic = new ExprComp(CompType::GE, ef, ep);
  const MStrHashMap<uint32_t> &mapColPos = table->GetMapColumnPos();
  bool bl = exprHaving.Preprocess(mapColPos);
  BOOST_TEST(bl);
  BOOST_TEST(ef->_rowPos == 1);

  b = exprHaving.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  delete table;
}

BOOST_AUTO_TEST_CASE(ExprWhere_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database db(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(), MicroSecTime());
  PhysTable *table = CreateTable(db, "testTable");

  ExprWhere exprWhere(nullptr);
  TriBool b = exprWhere.Calc(vdParas, vdRow);
  BOOST_TEST(b == TriBool::True);

  ExprField *ef1 = new ExprField(new MString("testTable"), new MString("c2"));
  ExprParameter *ep1 = new ExprParameter();
  ep1->_paraPos = 4;

  ExprField *ef2 = new ExprField(new MString("testTable"), new MString("c3"));
  ExprParameter *ep2 = new ExprParameter();
  ep2->_paraPos = 5;

  ExprComp *exprCmp1 = new ExprComp(CompType::GE, ef1, ep1);
  ExprComp *exprCmp2 = new ExprComp(CompType::GT, ef2, ep2);
  ExprAnd *eand = new ExprAnd();
  eand->_vctChild.push_back(exprCmp1);
  eand->_vctChild.push_back(exprCmp2);

  exprWhere._exprLogic = eand;
  const MStrHashMap<uint32_t> &mapColPos = table->GetMapColumnPos();
  bool bl = exprWhere.Preprocess(table, mapColPos);
  BOOST_TEST(bl);
  BOOST_TEST(ef1->_rowPos == 1);
  BOOST_TEST(ef2->_rowPos == 2);
  BOOST_TEST(exprWhere._exprLogic == exprCmp2);

  BOOST_TEST(exprWhere._indexSearch->_idxLogic == exprCmp1);
  BOOST_TEST(exprWhere._indexSearch->_bPointQuery == false);
  BOOST_TEST(exprWhere._indexSearch->_vctPointCond == nullptr);
  BOOST_TEST(exprWhere._indexSearch->_indexPos == 1);

  delete table;
}

BOOST_AUTO_TEST_CASE(ExprGroupBy_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database db(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(), MicroSecTime());
  PhysTable *table = CreateTable(db, "testTable");

  MVectorPtr<MString *> *vctColName = new MVectorPtr<MString *>();
  vctColName->push_back(new MString("c1"));
  vctColName->push_back(new MString("c3"));

  ExprField *ef = new ExprField(new MString("testTable"), new MString("c2"));
  ExprParameter *ep = new ExprParameter();
  ep->_paraPos = 4;
  ExprHaving *exprHaving = new ExprHaving(new ExprComp(CompType::GE, ef, ep));

  ExprGroupBy exprGb(vctColName, exprHaving);
  const MStrHashMap<uint32_t> &mapColPos = table->GetMapColumnPos();
  bool bl = exprGb.Preprocess(mapColPos);
  BOOST_TEST(bl);
  BOOST_TEST(exprGb._vctColPos.size() == 2);
  BOOST_TEST(exprGb._vctColPos[0] == 0);
  BOOST_TEST(exprGb._vctColPos[1] == 2);

  BOOST_TEST(exprGb._exprHaving->_exprLogic->GetType() == ExprType::EXPR_COMP);
  BOOST_TEST(ef->_rowPos == 1);

  delete table;
}

BOOST_AUTO_TEST_CASE(ExprOrderBy_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database db(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(), MicroSecTime());
  PhysTable *table = CreateTable(db, "testTable");

  MVectorPtr<ExprOrderItem *> *vctItem = new MVectorPtr<ExprOrderItem *>();
  vctItem->push_back(new ExprOrderItem(new MString("c1"), true));
  vctItem->push_back(new ExprOrderItem(new MString("c2"), false));
  vctItem->push_back(new ExprOrderItem(new MString("c3"), true));

  ExprOrderBy exprOb(vctItem);
  const MStrHashMap<uint32_t> &mapColPos = table->GetMapColumnPos();
  bool bl = exprOb.Preprocess(mapColPos);
  BOOST_TEST(bl);

  BOOST_TEST(vctItem->at(0)->_pos == 0);
  BOOST_TEST(vctItem->at(1)->_pos == 1);
  BOOST_TEST(vctItem->at(2)->_pos == 2);

  delete table;
}

BOOST_AUTO_TEST_CASE(ExprInsert_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database *db = new Database(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *table = CreateTable(*db, "testTable");
  TableManager::AddTable(table);

  ExprTable *exprTable =
      new ExprTable(new MString("testDb"), new MString("testTable"));
  MVectorPtr<ExprColumn *> *vctCol = new MVectorPtr<ExprColumn *>();
  vctCol->push_back(new ExprColumn(new MString("c1"), nullptr, nullptr));
  vctCol->push_back(new ExprColumn(new MString("c2"), nullptr, nullptr));
  vctCol->push_back(new ExprColumn(new MString("c3"), nullptr, nullptr));

  MVectorPtr<ExprElem *> *rowData = new MVectorPtr<ExprElem *>();
  rowData->push_back(new ExprParameter());
  rowData->push_back(new ExprParameter());
  rowData->push_back(
      new ExprField(new MString("testTable"), new MString("c2")));

  MVectorPtr<MVectorPtr<ExprElem *> *> *vctRowData =
      new MVectorPtr<MVectorPtr<ExprElem *> *>();
  vctRowData->push_back(rowData);

  ExprInsert exprInsert;
  exprInsert._exprTable = exprTable;
  exprInsert._vctCol = vctCol;
  exprInsert._vctRowData = vctRowData;
  bool bl = exprInsert.Preprocess(db);
  BOOST_TEST(bl);

  BOOST_TEST(exprTable->_physTable == table);
  BOOST_TEST(vctCol->at(0)->_pos == 0);
  BOOST_TEST(vctCol->at(0)->_dataLength == 1000);
  BOOST_TEST(vctCol->at(0)->_dataType == DataType::FIXCHAR);
  BOOST_TEST(vctCol->at(1)->_pos == 1);
  BOOST_TEST(vctCol->at(1)->_dataLength == 1000);
  BOOST_TEST(vctCol->at(1)->_dataType == DataType::VARCHAR);
  BOOST_TEST(vctCol->at(2)->_pos == 2);
  BOOST_TEST(vctCol->at(2)->_dataLength == 50);
  BOOST_TEST(vctCol->at(2)->_dataType == DataType::FIXCHAR);

  BOOST_TEST(dynamic_cast<ExprField *>(rowData->at(2))->_rowPos == 1);

  exprTable->_physTable = nullptr;
  delete exprInsert._vctCol;
  exprInsert._vctCol = nullptr;

  bl = exprInsert.Preprocess(db);
  BOOST_TEST(bl);

  vctCol = exprInsert._vctCol;
  BOOST_TEST(exprTable->_physTable == table);
  BOOST_TEST(vctCol->at(0)->_pos == 0);
  BOOST_TEST(vctCol->at(0)->_dataLength == 1000);
  BOOST_TEST(vctCol->at(0)->_dataType == DataType::FIXCHAR);
  BOOST_TEST(vctCol->at(1)->_pos == 1);
  BOOST_TEST(vctCol->at(1)->_dataLength == 1000);
  BOOST_TEST(vctCol->at(1)->_dataType == DataType::VARCHAR);
  BOOST_TEST(vctCol->at(2)->_pos == 2);
  BOOST_TEST(vctCol->at(2)->_dataLength == 50);
  BOOST_TEST(vctCol->at(2)->_dataType == DataType::FIXCHAR);

  TableManager::ClearTable();
  DatabaseManager::ClearDB();
}

BOOST_AUTO_TEST_CASE(ExprUpdate_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database *db = new Database(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *table = CreateTable(*db, "testTable");
  TableManager::AddTable(table);

  ExprTable *exprTable =
      new ExprTable(new MString("testDb"), new MString("testTable"));
  MVectorPtr<ExprColumn *> *vctCol = new MVectorPtr<ExprColumn *>();
  vctCol->push_back(
      new ExprColumn(new MString("c1"), new ExprParameter(), nullptr));
  vctCol->push_back(
      new ExprColumn(new MString("c2"), new ExprParameter(), nullptr));
  vctCol->push_back(new ExprColumn(
      new MString("c3"),
      new ExprField(new MString("testTable"), new MString("c2")), nullptr));

  ExprField *ef1 = new ExprField(new MString("testTable"), new MString("c2"));
  ExprParameter *ep1 = new ExprParameter();
  ep1->_paraPos = 4;
  ExprComp *exprComp = new ExprComp(CompType::GT, ef1, ep1);
  ExprWhere *exprWhere = new ExprWhere(exprComp);

  MVectorPtr<ExprOrderItem *> *vctItem = new MVectorPtr<ExprOrderItem *>();
  vctItem->push_back(new ExprOrderItem(new MString("c2"), true));
  ExprOrderBy *exprOb = new ExprOrderBy(vctItem);

  ExprLimit *exprLimit = new ExprLimit(1, 2);
  ExprUpdate exprUp;
  exprUp._exprTable = exprTable;
  exprUp._vctCol = vctCol;
  exprUp._exprWhere = exprWhere;
  exprUp._exprOrderBy = exprOb;
  exprUp._exprLimit = exprLimit;

  bool bl = exprUp.Preprocess(db);
  BOOST_TEST(exprUp._exprTable->_physTable == table);
  ExprColumn *ecol = dynamic_cast<ExprColumn *>(exprUp._vctCol->at(0));
  BOOST_TEST(ecol->_pos == 0);
  BOOST_TEST(ecol->_dataType == DataType::FIXCHAR);
  BOOST_TEST(ecol->_dataLength == 1000);

  ecol = dynamic_cast<ExprColumn *>(exprUp._vctCol->at(1));
  BOOST_TEST(ecol->_pos == 1);
  BOOST_TEST(ecol->_dataType == DataType::VARCHAR);
  BOOST_TEST(ecol->_dataLength == 1000);

  ecol = dynamic_cast<ExprColumn *>(exprUp._vctCol->at(2));
  BOOST_TEST(ecol->_pos == 2);
  BOOST_TEST(ecol->_dataType == DataType::FIXCHAR);
  BOOST_TEST(ecol->_dataLength == 50);
  ExprField *efd = dynamic_cast<ExprField *>(ecol->_exprElem);
  BOOST_TEST(efd->_rowPos == 1);

  BOOST_TEST(ef1->_rowPos == 1);
  BOOST_TEST(vctItem->at(0)->_pos == 1);

  TableManager::ClearTable();
  DatabaseManager::ClearDB();
}

BOOST_AUTO_TEST_CASE(ExprDelete_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database *db = new Database(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *table = CreateTable(*db, "testTable");
  TableManager::AddTable(table);

  ExprTable *exprTable =
      new ExprTable(new MString("testDb"), new MString("testTable"));

  ExprField *ef1 = new ExprField(new MString("testTable"), new MString("c2"));
  ExprParameter *ep1 = new ExprParameter();
  ep1->_paraPos = 4;
  ExprComp *exprComp = new ExprComp(CompType::GT, ef1, ep1);
  ExprWhere *exprWhere = new ExprWhere(exprComp);

  MVectorPtr<ExprOrderItem *> *vctItem = new MVectorPtr<ExprOrderItem *>();
  vctItem->push_back(new ExprOrderItem(new MString("c2"), true));
  ExprOrderBy *exprOb = new ExprOrderBy(vctItem);

  ExprLimit *exprLimit = new ExprLimit(1, 2);
  ExprDelete exprDel;
  exprDel._exprTable = exprTable;
  exprDel._exprWhere = exprWhere;
  exprDel._exprOrderBy = exprOb;
  exprDel._exprLimit = exprLimit;

  bool bl = exprDel.Preprocess(db);
  BOOST_TEST(exprDel._exprTable->_physTable == table);
  BOOST_TEST(ef1->_rowPos == 1);
  BOOST_TEST(vctItem->at(0)->_pos == 1);

  TableManager::ClearTable();
  DatabaseManager::ClearDB();
}

BOOST_AUTO_TEST_CASE(ExprSelect_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;

  VectorDataValue vdParas;
  GenerateParas(vdParas);
  VectorDataValue vdRow;
  GenerateRows(vdRow);
  Database *db = new Database(1, ROOT_PATH.c_str(), "testDb", MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *table = CreateTable(*db, "testTable");
  TableManager::AddTable(table);

  ExprTable *exprTable =
      new ExprTable(new MString("testDb"), new MString("testTable"));
  MVectorPtr<ExprColumn *> *vctCol = new MVectorPtr<ExprColumn *>();
  ExprField *el1 = new ExprField(nullptr, new MString("c1"));
  vctCol->push_back(new ExprColumn(nullptr, el1, new MString("ff1")));

  ExprField *el2 = new ExprField(nullptr, new MString("c2"));
  vctCol->push_back(new ExprColumn(nullptr, el2, new MString("ff2")));

  ExprField *el3 = new ExprField(nullptr, new MString("c3"));
  vctCol->push_back(new ExprColumn(
      nullptr, new ExprAdd(el3, new ExprConst(new MString("abcdefg"))),
      nullptr));

  ExprField *ef1 = new ExprField(new MString("testTable"), new MString("c2"));
  ExprComp *exprComp =
      new ExprComp(CompType::GT, ef1, new ExprConst(new MString("abcdefg")));
  ExprWhere *exprWhere = new ExprWhere(exprComp);

  MVectorPtr<ExprOrderItem *> *vctItem = new MVectorPtr<ExprOrderItem *>();
  vctItem->push_back(new ExprOrderItem(new MString("c2"), true));
  ExprOrderBy *exprOb = new ExprOrderBy(vctItem);

  MVectorPtr<MString *> *vctGbName = new MVectorPtr<MString *>();
  vctGbName->push_back(new MString("c1"));
  vctGbName->push_back(new MString("c2"));
  ExprGroupBy *exprGb = new ExprGroupBy(vctGbName, nullptr);

  ExprLimit *exprLimit = new ExprLimit(1, 2);
  ExprSelect exprSel;
  exprSel._vctTable = new MVectorPtr<storage::ExprTable *>();
  exprSel._vctTable->push_back(exprTable);
  exprSel._vctCol = vctCol;
  exprSel._exprWhere = exprWhere;
  exprSel._exprGroupBy = exprGb;
  exprSel._exprOrderBy = exprOb;
  exprSel._exprLimit = exprLimit;

  bool bl = exprSel.Preprocess(db);
  BOOST_TEST(bl);

  ExprTableSelect *tsel =
      dynamic_cast<ExprTableSelect *>(exprSel._exprDestSelect);
  BOOST_TEST(tsel != nullptr);
  BOOST_TEST(tsel->_exprTable->_physTable == table);

  BOOST_TEST(el1->_rowPos == 0);
  BOOST_TEST(*vctCol->at(0)->_name == "c1");
  BOOST_TEST(*vctCol->at(0)->_alias == "ff1");

  BOOST_TEST(el2->_rowPos == 1);
  BOOST_TEST(*vctCol->at(1)->_name == "c2");
  BOOST_TEST(*vctCol->at(1)->_alias == "ff2");

  BOOST_TEST(el3->_rowPos == 2);
  BOOST_TEST(*vctCol->at(2)->_name == "col2");
  BOOST_TEST(vctCol->at(2)->_alias == nullptr);

  BOOST_TEST(ef1->_rowPos == 1);
  BOOST_TEST(vctItem->at(0)->_pos == 1);

  BOOST_TEST(exprGb->_vctColPos.size() == 2);
  BOOST_TEST(exprGb->_vctColPos[0] == 0);
  BOOST_TEST(exprGb->_vctColPos[1] == 1);

  TableManager::ClearTable();
  DatabaseManager::ClearDB();
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
