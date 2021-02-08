#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/core/LeafRecord.h"
#include "../../src/utils/Utilitys.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/core/IndexTree.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/dataType/DataValueBlob.h"

namespace storage {
	namespace fs = std::filesystem;
	BOOST_AUTO_TEST_SUITE(CoreTest)

	BOOST_AUTO_TEST_CASE(LeafRecord_test)
	{
		const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		DataValueLong* dvKey = new DataValueLong(100LL, true);
		DataValueLong* dvVal = new DataValueLong(200LL, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);

		vctKey.push_back(dvKey->CloneDataValue(true));
		vctVal.push_back(dvVal->CloneDataValue(true));
		LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal);
		BOOST_TEST(8 ==lr->GetKeyLength());
		BOOST_TEST(9 == lr->GetValueLength());
		BOOST_TEST(23 == lr->GetTotalLength());

		VectorDataValue vctKey2;
		lr->GetListKey(vctKey2);
		BOOST_TEST(1 == vctKey2.size());
		BOOST_TEST(*vctKey2[0] == *dvKey);

		VectorDataValue vctVal2;
		lr->GetListValue(vctVal2);
		BOOST_TEST(1 == vctVal2.size());
		BOOST_TEST(*dvVal == *vctVal[0]);

		Byte buff[100] = {};
		int pos = 0;
		utils::UInt16ToBytes((uint16_t)(sizeof(uint16_t) * 3 + dvKey->GetPersistenceLength(true) +
			dvVal->GetPersistenceLength(false)), buff + pos);
		pos += 2;
		utils::UInt16ToBytes(dvKey->GetPersistenceLength(true), buff + pos);
		pos += 2;
		utils::UInt16ToBytes(1, buff + pos);
		pos += 2;

		pos += dvKey->WriteData(buff + pos, true);
		pos += dvVal->WriteData(buff + pos, false);
		lr->ReleaseRecord();

		lr = new LeafRecord(indexTree, buff);
		BOOST_TEST(buff == lr->GetBysValue());
		BOOST_TEST(8 == lr->GetKeyLength());
		BOOST_TEST(9 == lr->GetValueLength());
		BOOST_TEST(23 == lr->GetTotalLength());

		VectorDataValue vctKey3;
		lr->GetListKey(vctKey3);
		BOOST_TEST(vctKey3.size() == 1);
		BOOST_TEST(*dvKey == *vctKey3[0]);

		VectorDataValue vctVal3;
		lr->GetListValue(vctVal3);
		BOOST_TEST(1 == vctVal3.size());
		BOOST_TEST(*dvVal == *vctVal3[0]);
		
		lr->ReleaseRecord();
		delete indexTree;
		delete dvKey;
		delete dvVal;

		fs::remove(fs::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(LeafRecord_Equal_test)
	{
		const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue(false) };
		VectorDataValue vctVal = { dvVal->CloneDataValue(false) };

		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		LeafPage* lp = (LeafPage*)indexTree->AllocateNewPage(HeadPage::NO_PARENT_POINTER, (Byte)0);

		Byte buff1[100] = { 0 };
		uint32_t pos = 0;
		utils::UInt16ToBytes((uint16_t)(sizeof(uint16_t) * 3 + dvKey->GetPersistenceLength(true) +
			dvVal->GetPersistenceLength(false)), buff1 + pos);
		pos += 2;
		utils::UInt16ToBytes((uint16_t)dvKey->GetPersistenceLength(true), buff1 + pos);
		pos += 2;
		utils::UInt16ToBytes(1, buff1 + pos);
		pos += 2;

		pos += dvKey->WriteData(buff1 + pos, true);
		pos += dvVal->WriteData(buff1 + pos, false);

		Byte buff2[100] = { 0 };
		pos = 0;
		utils::UInt16ToBytes((uint16_t)(sizeof(uint16_t) * 3 + dvKey->GetPersistenceLength(true) +
			dvVal->GetPersistenceLength(false)), buff2 + pos);
		pos += 2;
		utils::UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
		pos += 2;
		utils::UInt16ToBytes(1, buff2 + pos);
		pos += 2;

		pos += dvKey->WriteData(buff2 + pos, true);
		pos += dvVal->WriteData(buff2 + pos, false);

		LeafRecord* rr1 = new LeafRecord(lp, buff1);
		LeafRecord* rr2 = new LeafRecord(lp, buff2);

		BOOST_TEST(rr1->CompareTo(*rr2) == 0);
		BOOST_TEST(rr1->CompareKey(*rr2) == 0);
		RawKey* key = rr2->GetKey();
		BOOST_TEST(rr1->CompareKey(*key) == 0);
		delete key;

		delete dvVal;
		dvVal = new DataValueLong(210, false);
		pos = 0;
		utils::UInt16ToBytes((uint16_t)(sizeof(uint16_t) * 3 + dvKey->GetPersistenceLength(true) +
			dvVal->GetPersistenceLength(false)), buff2 + pos);
		pos += 2;
		utils::UInt16ToBytes((uint16_t)dvKey->GetPersistenceLength(true), buff2 + pos);
		pos += 2;
		utils::UInt16ToBytes(1, buff2 + pos);
		pos += 2;

		pos += dvKey->WriteData(buff2 + pos, true);
		pos += dvVal->WriteData(buff2 + pos, false);

		rr2->ReleaseRecord();
		rr2 = new LeafRecord(lp, buff2);

		BOOST_TEST(rr1->CompareTo(*rr2) != 0);
		BOOST_TEST(rr1->CompareKey(*rr2) == 0);
		key = rr2->GetKey();
		BOOST_TEST(rr1->CompareKey(*key) == 0);
		delete key;
		BOOST_TEST(0 > rr1->CompareTo(*rr2));

		delete dvKey;
		dvKey = new DataValueLong(110, true);
		pos = 0;
		utils::UInt16ToBytes((uint16_t)(sizeof(uint16_t) * 3 + dvKey->GetPersistenceLength(true) +
			dvVal->GetPersistenceLength(false)), buff2 + pos);
		pos += 2;
		utils::UInt16ToBytes((uint16_t)dvKey->GetPersistenceLength(true), buff2 + pos);
		pos += 2;
		utils::UInt16ToBytes(1, buff2 + pos);
		pos += 2;

		pos += dvKey->WriteData(buff2 + pos, true);
		pos += dvVal->WriteData(buff2 + pos, false);

		rr2->ReleaseRecord();
		rr2 = new LeafRecord(lp, buff2);

		BOOST_TEST(rr1->CompareKey(*rr2) != 0);
		BOOST_TEST(0 > rr1->CompareTo(*rr2));
		key = rr2->GetKey();
		BOOST_TEST(0 > rr1->CompareKey(*key));

		vctKey.push_back(dvKey->CloneDataValue(true));
		RawKey* key2 = new RawKey(vctKey);
		BOOST_TEST(0 == key2->CompareTo(*key));

		delete key2;
		delete lp;
		delete indexTree;
		delete dvKey;
		delete dvVal;
		fs::remove(fs::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(LeafRecord_Block_test)
	{
		const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		char* blockData = new char[1024 * 10];
		for (int i = 0; i < 1024 * 10; i++) {
			blockData[i] = (Byte)i;
		}

		char* blockData2 = new char[1024 * 20];
		for (int i = 0; i < 1024 * 20; i++) {
			blockData2[i] = (Byte)i;
		}

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal1 = new DataValueLong(200, false);
		DataValueBlob* dvVal2 = new DataValueBlob(1024 * 20, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue(false) };
		VectorDataValue vctVal = { dvVal1->CloneDataValue(false), dvVal2->CloneDataValue(false) };

		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);

		vctKey.push_back(dvKey->CloneDataValue(true));
		vctVal.push_back(dvVal1->CloneDataValue(true));
		vctVal.push_back(new DataValueBlob(blockData, 1024 * 10, 1024 * 20, false));

		LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal);

		PageFile* ovf = indexTree->GetOverflowFile();
		uint64_t lenOvf = ovf->Length();

		LeafRecord* rr2 = new LeafRecord(indexTree, vctKey, vctVal);
		uint64_t lenOvf2 = ovf->Length();
		BOOST_TEST(lenOvf * 2 <= lenOvf2);

		LeafRecord* rr3 = new LeafRecord(indexTree, vctKey, vctVal, rr2->GetOvfOffset(), rr2->GetSizeOverflow());
		uint64_t lenOvf3 = ovf->Length();
		BOOST_TEST(lenOvf2 == lenOvf3);

		delete vctVal[1];
		vctVal[1] = new DataValueBlob(blockData2, 1024 * 20, 1024 * 20, false);
		LeafRecord* rr4 = new LeafRecord(indexTree, vctKey, vctVal, rr->GetOvfOffset(), rr->GetSizeOverflow());
		uint64_t lenOvf4 = ovf->Length();
		BOOST_TEST(lenOvf3 < lenOvf4);

		delete indexTree;
		delete dvKey;
		delete dvVal1;
		delete dvVal2;
		rr->ReleaseRecord();
		rr2->ReleaseRecord();
		rr3->ReleaseRecord();
		rr4->ReleaseRecord();

		fs::remove(fs::path(FILE_NAME));
		string name = FILE_NAME.substr(0, FILE_NAME.find_last_of('/')) + "/overfile.dat";
		fs::remove(fs::path(name));
	}
	BOOST_AUTO_TEST_SUITE_END()
}