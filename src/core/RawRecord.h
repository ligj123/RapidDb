#pragma once
#include "../header.h"
#include "../dataType/IDataValue.h"
#include "RawKey.h"
#include <vector>
#include <cstdint>

namespace storage {
	class IndexPage;
	class IndexTree;

  class RawRecord
  {
  public:
    virtual Byte* GetBysValue() const  = 0;
		virtual uint16_t GetTotalLength() const = 0;
		virtual uint16_t GetKeyLength() const = 0;
		virtual uint16_t GetValueLength() const = 0;
		virtual RawKey* GetKey() const = 0;
		virtual void SetParentPage(IndexPage* page) = 0;
		virtual IndexPage* GetParentPage() const = 0;
		virtual vector<IDataValue*>& GetListKey() const = 0;
		virtual vector<IDataValue*>& GetListValue() const = 0;
		virtual void SaveData(Byte* bysPage) = 0;
		virtual IndexTree* GetTreeeFile() const = 0;
	public:
		static void* operator new(size_t size)
		{
			return CachePool::Apply((uint32_t)size);
		}
		static void operator delete(void* ptr, size_t size)
		{
			CachePool::Release((Byte*)ptr, (uint32_t)size);
		}
  };
}

