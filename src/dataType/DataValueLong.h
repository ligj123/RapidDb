#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueLong : public IDataValue
	{
	public:
		DataValueLong(bool bKey=false);
		DataValueLong(int64_t val, bool bKey = false);
		DataValueLong(char* byArray, bool bKey = false);
		DataValueLong(const DataValueLong& src);
		DataValueLong(std::any val, bool bKey = false);
		~DataValueLong(){}
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

		operator int64_t() const;
		DataValueLong& operator=(int64_t val);
		DataValueLong& operator=(const DataValueLong& src);

		bool operator > (const DataValueLong& dv) const;
		bool operator < (const DataValueLong& dv) const;
		bool operator >= (const DataValueLong& dv) const;
		bool operator <= (const DataValueLong& dv) const;
		bool operator == (const DataValueLong& dv) const;
		bool operator != (const DataValueLong& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
	protected:
		union {
			int64_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
}