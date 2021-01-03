#pragma once
#include "../header.h"
#include "../dataType/IDataValue.h"
#include "../cache/CachePool.h"
#include <vector>
#include <cstdint>

namespace storage {
  class RawKey
  {
	protected:
		Byte* _bysVal;
		uint32_t _length;
		bool _bSole;

	public:
		RawKey() : _bysVal(nullptr), _length(0), _bSole(false) { }
		RawKey(vector<IDataValue> vctKey);
		RawKey(Byte* bys, uint32_t len);
		~RawKey();

		Byte* GetBysVal() { return _bysVal;	}
		uint32_t GetLength() { return _length; }
		
		void* operator new(size_t size)
		{
			return CachePool::Apply((uint32_t)size);
		}

		void operator delete(void* ptr, size_t size)
		{
			CachePool::Release((Byte*)ptr, (uint32_t)size);
		}

		bool operator > (const RawKey& key) const;
		bool operator < (const RawKey& key) const;
		bool operator >= (const RawKey& key) const;
		bool operator <= (const RawKey& key) const;
		bool operator == (const RawKey& key) const;
		bool operator != (const RawKey& key) const;
		/*@Override
			public boolean equals(Object o) {
			if (this == o)
				return true;

			RawKey other = (RawKey)o;
			if (length != other.length) {
				return false;
			}

			return BytesUtils.equals(bysVal, startPos, other.bysVal, other.startPos, length);
		}

		@Override
			public int compareTo(RawKey target) {
			if (this == target)
				return 0;

			return BytesUtils.compareTo(bysVal, startPos, length, target.bysVal, target.startPos, target.length);
		}*/

		/*public void release() {
			if (bysVal.length < Configuration.getCachePageSize() && startPos == 0) {
				ObjectPool.releaseByteArray(bysVal);
			}
			ObjectPool.releaseRawKey(this);
		}*/

	

		@Override
			public String toString() {
			return "startPos=" + startPos + "\tlength=" + length +
				"\tkey=" + HexStringUtil.toHexString(bysVal, startPos, length);
		}
  };
}

