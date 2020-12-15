#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueInt : public IDataValue
	{
	public:
		DataValueInt(bool bKey=false);
		DataValueInt(int32_t val, bool bKey = false);
		DataValueInt(char* byArray, bool bKey = false);
		DataValueInt(const DataValueInt& src);
		DataValueInt(std::any val, bool bKey = false);
		~DataValueInt(){}
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

		operator int32_t() const;
		DataValueInt& operator=(int32_t val);
		DataValueInt& operator=(const DataValueInt& src);

		bool operator > (const DataValueInt& dv) const;
		bool operator < (const DataValueInt& dv) const;
		bool operator >= (const DataValueInt& dv) const;
		bool operator <= (const DataValueInt& dv) const;
		bool operator == (const DataValueInt& dv) const;
		bool operator != (const DataValueInt& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueInt& dv);

	protected:
		union {
			int32_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueInt& dv);
}