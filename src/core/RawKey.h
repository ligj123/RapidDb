#pragma once
#include "../header.h"
#include "../dataType/IDataValue.h"
#include "../cache/CachePool.h"
#include <vector>
#include <cstdint>
#include <cstring>

namespace storage {
  class RawKey
  {
	public:
		RawKey() : _bysVal(nullptr), _length(0), _bSole(false) { }
		RawKey(VectorDataValue& vctKey);
		RawKey(Byte* bys, uint32_t len);
		~RawKey();

		Byte* GetBysVal() const { return _bysVal;	}
		uint32_t GetLength() const { return _length; }
		
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
		int CompareTo(const RawKey& key) const;
		
	protected:
		Byte* _bysVal;
		uint32_t _length;
		bool _bSole;

		friend std::ostream& operator<< (std::ostream& os, const RawKey& key);
  };

	std::ostream& operator<< (std::ostream& os, const RawKey& dv);
}

