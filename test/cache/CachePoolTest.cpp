#include <boost/test/unit_test.hpp>
#include "../../src/cache/CachePool.h"
#include "../../src/cache/BufferPool.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(CachePoolTest)

	BOOST_AUTO_TEST_CASE(Buffer_test)
	{
		class BufferEx : public Buffer
		{
		public:
			BufferEx(uint32_t eleSize) : Buffer(eleSize){}
			Byte* GetBuf() { return _pBuf; }
			uint32_t GetEleSize(){ return _eleSize; }
			uint32_t GetMaxEle() { return _maxEle; }
			set<uint16_t> GetSetFree() { return _setFree; }
		};

		uint32_t maxEle = (uint32_t)Configure::GetCacheBlockSize() / 32;
		BufferEx buff(32);
		BOOST_TEST(true, buff.IsEmpty());
		BOOST_TEST(false, buff.IsFull());

		BOOST_TEST(32 == buff.GetEleSize());
		BOOST_TEST(maxEle == buff.GetMaxEle());
		BOOST_TEST(buff.GetBuf() == buff.Apply());
		BOOST_TEST(buff.GetBuf() + 32 == buff.Apply());
		BOOST_TEST(buff.GetBuf() == Buffer::CalcAddr(buff.Apply()));
		BOOST_TEST(3 == buff.CalcPos(buff.Apply()));
		BOOST_TEST(maxEle - 4, buff.GetSetFree().size());
		
		for (uint32_t i = 4; i < maxEle; i++)
		{
			BOOST_TEST(buff.GetBuf() + i * 32 == buff.Apply());
		}

		BOOST_TEST(true, buff.IsFull());
		BOOST_TEST(false, buff.IsEmpty());

		for (int32_t i = maxEle - 1; i >= 0; i--)
		{
			buff.Release(buff.GetBuf() + (i * 32));
			BOOST_TEST(maxEle - i, buff.GetSetFree().size());
		}

		BOOST_TEST(true, buff.IsEmpty());

		buff.Init(40);
		maxEle = (uint32_t)Configure::GetCacheBlockSize() / 40;
		BOOST_TEST(40 == buff.GetEleSize());
		BOOST_TEST(maxEle == buff.GetMaxEle());
	}
	BOOST_AUTO_TEST_SUITE_END()
}