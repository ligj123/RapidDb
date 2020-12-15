#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueUInt : public IDataValue
	{
	public:
		DataValueUInt(bool bKey=false);
		DataValueUInt(uint32_t val, bool bKey = false);
		DataValueUInt(char* byArray, bool bKey = false);
		DataValueUInt(const DataValueUInt& src);
		DataValueUInt(std::any val, bool bKey = false);
		~DataValueUInt(){}
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

		operator uint32_t() const;
		DataValueUInt& operator=(uint32_t val);
		DataValueUInt& operator=(const DataValueUInt& src);

		bool operator > (const DataValueUInt& dv) const;
		bool operator < (const DataValueUInt& dv) const;
		bool operator >= (const DataValueUInt& dv) const;
		bool operator <= (const DataValueUInt& dv) const;
		bool operator == (const DataValueUInt& dv) const;
		bool operator != (const DataValueUInt& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueUInt& dv);

	protected:
		union {
			uint32_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueUInt& dv);
}