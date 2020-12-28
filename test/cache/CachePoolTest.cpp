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
			uint32_t GetEleSize(){ return _eleSize; }
			uint32_t GetMaxEle() { return _maxEle; }
			queue<uint16_t> GetQueueFree() { return _queueFree; }
		};

		uint32_t maxEle = (uint32_t)Configure::GetCacheBlockSize() / 32;
		BufferEx buff(32);
		BOOST_TEST(true == buff.IsEmpty());
		BOOST_TEST(false == buff.IsFull());

		BOOST_TEST(32 == buff.GetEleSize());
		BOOST_TEST(maxEle == buff.GetMaxEle());
		BOOST_TEST(buff.GetBuf() == buff.Apply());
		BOOST_TEST(buff.GetBuf() + 32 == buff.Apply());
		BOOST_TEST(buff.GetBuf() == Buffer::CalcAddr(buff.Apply()));
		BOOST_TEST(3 == buff.CalcPos(buff.Apply()));
		BOOST_TEST(maxEle - 4 == buff.GetQueueFree().size());
		
		for (uint32_t i = 4; i < maxEle; i++)
		{
			BOOST_TEST(buff.GetBuf() + i * 32 == buff.Apply());
		}

		BOOST_TEST(true == buff.IsFull());
		BOOST_TEST(false == buff.IsEmpty());

		for (int32_t i = maxEle - 1; i >= 0; i--)
		{
			buff.Release(buff.GetBuf() + (i * 32));
		}

		BOOST_TEST(true == buff.IsEmpty());

		buff.Init(40);
		maxEle = (uint32_t)Configure::GetCacheBlockSize() / 40;
		BOOST_TEST(40 == buff.GetEleSize());
		BOOST_TEST(maxEle == buff.GetMaxEle());
	}

	BOOST_AUTO_TEST_CASE(BufferPool_test)
	{
		class BufferPoolEx : public BufferPool
		{
		public:
			BufferPoolEx(uint32_t eleSize) :BufferPool(eleSize) {}
			unordered_map<Byte*, Buffer*> GetMapBuffer() { return _mapBuffer; }
			unordered_map<Byte*, Buffer*> GetMapFree() { return _mapFreeBuffer; }
			uint32_t GetEleSize() { return _eleSize; }
		};

		BufferPoolEx pool(32);
		BOOST_TEST(32 == pool.GetEleSize());
		uint32_t maxEle = (uint32_t)Configure::GetCacheBlockSize() / 32;
		for (uint32_t i = 0; i < maxEle; i++)
		{
			pool.Apply();
		}

		BOOST_TEST(1 == pool.GetMapBuffer().size());
		BOOST_TEST(0 == pool.GetMapFree().size());

		Byte* bys1 = pool.Apply();
		Byte* bys2 = pool.Apply();
		BOOST_TEST(2 == pool.GetMapBuffer().size());
		BOOST_TEST(1 == pool.GetMapFree().size());

		pool.Release(bys1);
		pool.Release(bys2);
		BOOST_TEST(1 == pool.GetMapBuffer().size());
		BOOST_TEST(0 == pool.GetMapFree().size());

	}

	BOOST_AUTO_TEST_CASE(CachePool_test)
	{
		class CachePoolEx : public CachePool
		{
		public:
			using CachePool::_mapPool;
			using CachePool::_queueFreeBuf;
			using CachePool::_gCachePool;
		};

		CachePoolEx::_gCachePool = new CachePoolEx();
		uint32_t maxEle = (uint32_t)Configure::GetCacheBlockSize() / 32;
		for (uint32_t i = 0; i < maxEle; i++)
		{
			CachePoolEx::Apply(32);
		}

		BOOST_TEST(1 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_mapPool.size());

		Byte* bys = CachePoolEx::Apply(32);
		BOOST_TEST(0 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_queueFreeBuf.size());

		CachePoolEx::Release(bys, 32);
		BOOST_TEST(1 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_queueFreeBuf.size());

		bys = CachePoolEx::Apply(32);
		BOOST_TEST(0 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_queueFreeBuf.size());

		Byte* bys2 = CachePoolEx::Apply(40);
		BOOST_TEST(2 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_mapPool.size());
		BOOST_TEST(0 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_queueFreeBuf.size());

		CachePoolEx::Release(bys, 32);
		CachePoolEx::Release(bys2, 40);
		BOOST_TEST(2 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_mapPool.size());
		BOOST_TEST(2 == ((CachePoolEx*)CachePoolEx::_gCachePool)->_queueFreeBuf.size());

	}

	BOOST_AUTO_TEST_SUITE_END()
}