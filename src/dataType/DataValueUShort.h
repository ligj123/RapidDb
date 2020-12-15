#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueUShort : public IDataValue
	{
	public:
		DataValueUShort(bool bKey=false);
		DataValueUShort(uint16_t val, bool bKey = false);
		DataValueUShort(char* byArray, bool bKey = false);
		DataValueUShort(const DataValueUShort& src);
		DataValueUShort(std::any val, bool bKey = false);
		~DataValueUShort(){}
	public:
		virtual std::any GetValue() const;
		virtual uint32_t WriteData(char* buf);
		virtual uint32_t ReadData(char* buf, uint32_t len = 0);
		virtual uint32_t GetLength() const;
		virtual uint32_t GetMaxLength() const;
		virtual uint32_t GetPersistenceLength() const;
		virtual void SetMinValue();
		virtual void SetMaxValue();
		virtual void SetDefaultValue();

		operator uint16_t() const;
		DataValueUShort& operator=(uint16_t val);
		DataValueUShort& operator=(const char* pArr);
		DataValueUShort& operator=(const DataValueUShort& src);

		bool operator > (const DataValueUShort& dv) const;
		bool operator < (const DataValueUShort& dv) const;
		bool operator >= (const DataValueUShort& dv) const;
		bool operator <= (const DataValueUShort& dv) const;
		bool operator == (const DataValueUShort& dv) const;
		bool operator != (const DataValueUShort& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueUShort& dv);

	protected:
		union {
			uint16_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueUShort& dv);
}