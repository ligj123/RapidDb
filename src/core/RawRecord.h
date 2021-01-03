#pragma once
#include "../header.h"
#include "../dataType/IDataValue.h"
#include <vector>
#include <cstdint>

namespace storage {
  class RawRecord
  {
  public:
    virtual Byte* GetBysValue() = 0;
    virtual uint32_t GetStartPos() = 0;
		virtual uint32_t GetTotalLength() = 0;
		virtual uint32_t GetKeyLength() = 0;
		virtual uint32_t GetValueLength() = 0;
		virtual vector<IDataValue> GetListKey() = 0;
		virtual vector<IDataValue> GetListValue() = 0;
		
		RawKey getKey();

		void setParentPage(IndexPage page);

		void release(boolean releaseBuf, boolean releaseRef);

		void saveData(byte[] bysPage, int start);

		IndexPage getIndexPage();

		TreeFile getTreeeFile();

		RawRecord cloneRecord();
  };
}

