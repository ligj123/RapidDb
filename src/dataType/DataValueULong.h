#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueULong : public IDataValue
	{
	public:
		DataValueULong(bool bKey=false);
		DataValueULong(uint64_t val, bool bKey = false);
		DataValueULong(char* byArray, bool bKey = false);
		DataValueULong(const DataValueULong& src);
		DataValueULong(std::any val, bool bKey = false);
		~DataValueULong(){}
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

		operator uint64_t() const;
		DataValueULong& operator=(uint64_t val);
		DataValueULong& operator=(const char* pArr);
		DataValueULong& operator=(const DataValueULong& src);

		bool operator > (const DataValueULong& dv) const;
		bool operator < (const DataValueULong& dv) const;
		bool operator >= (const DataValueULong& dv) const;
		bool operator <= (const DataValueULong& dv) const;
		bool operator == (const DataValueULong& dv) const;
		bool operator != (const DataValueULong& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueULong& dv);

	protected:
		union {
			uint64_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueULong& dv);
}