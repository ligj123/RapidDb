#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueByte : public IDataValue
	{
	public:
		DataValueByte(bool bKey=false);
		DataValueByte(uint8_t val, bool bKey = false);
		DataValueByte(char* byArray, bool bKey = false);
		DataValueByte(const DataValueByte& src);
		DataValueByte(std::any val, bool bKey = false);
		~DataValueByte(){}
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

		operator uint8_t() const;
		DataValueByte& operator=(uint8_t val);
		DataValueByte& operator=(const DataValueByte& src);

		bool operator > (const DataValueByte& dv) const;
		bool operator < (const DataValueByte& dv) const;
		bool operator >= (const DataValueByte& dv) const;
		bool operator <= (const DataValueByte& dv) const;
		bool operator == (const DataValueByte& dv) const;
		bool operator != (const DataValueByte& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueByte& dv);
	protected:
		union {
			uint8_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueByte& dv);
}