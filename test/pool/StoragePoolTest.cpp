#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/pool/StoragePool.h"
#include "../../src/core/IndexTree.h"
#include "../../src/utils/Utilitys.h"
#include <cstring>

namespace storage {
	namespace fs = std::filesystem;
	BOOST_AUTO_TEST_SUITE(PoolTest)

	BOOST_AUTO_TEST_CASE(StoragePool_test)
	{
		const string FILE_NAME = "./dbTest/testStoragePool" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int NUM = 10000;
		VectorDataValue vctKey;
		VectorDataValue vctVal;
		string strTest = "abcdefg1234567890÷–Œƒ≤‚ ‘abcdefghigjlmnopqrstuvwrst";
		strTest += strTest;
		strTest += strTest;
		size_t size = strTest.size() + 1;
		const char* pStrTest = strTest.c_str();
		vector<CachePage*> vctPage;

		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		for (int i = 0; i < NUM; i++) {
			CachePage* page = new CachePage(indexTree, i);
			page->WriteInt(0, i);
			memcpy(page->GetBysPage() + 4, pStrTest, size);
			memcpy(page->GetBysPage() + Configure::GetCachePageSize() - size, pStrTest, size);
			page->SetDirty(true);
			StoragePool::WriteCachePage(page);
			vctPage.push_back(page);
		}

		StoragePool::FlushWriteCachePage();
		indexTree->Close(true);

		indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		for (int i = 0; i < NUM; i++) {
			CachePage* page = new CachePage(indexTree, i);
			future<int> fut = StoragePool::ReadCachePage(page);
			fut.get();
			BOOST_TEST(i == page->ReadInt(0));
			BOOST_TEST(memcmp(pStrTest, page->GetBysPage() + 4, size) == 0);
			BOOST_TEST(memcmp(pStrTest, page->GetBysPage() + Configure::GetCachePageSize() - size, size) == 0);
			vctPage.push_back(page);
		}

		for (CachePage* page : vctPage) {
			delete page;
		}
		
		indexTree->Close(true);
		fs::remove(fs::path(FILE_NAME));
	}
	BOOST_AUTO_TEST_SUITE_END()
}