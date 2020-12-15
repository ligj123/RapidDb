#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueShort : public IDataValue
	{
	public:
		DataValueShort(bool bKey=false);
		DataValueShort(int16_t val, bool bKey = false);
		DataValueShort(char* byArray, bool bKey = false);
		DataValueShort(const DataValueShort& src);
		DataValueShort(std::any val, bool bKey = false);
		~DataValueShort(){}
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

		operator int16_t() const;
		DataValueShort& operator=(int16_t val);
		DataValueShort& operator=(const DataValueShort& src);

		bool operator > (const DataValueShort& dv) const;
		bool operator < (const DataValueShort& dv) const;
		bool operator >= (const DataValueShort& dv) const;
		bool operator <= (const DataValueShort& dv) const;
		bool operator == (const DataValueShort& dv) const;
		bool operator != (const DataValueShort& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueShort& dv);

	protected:
		union {
			int16_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueShort& dv);
}