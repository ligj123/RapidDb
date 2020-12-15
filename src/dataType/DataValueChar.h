#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueChar : public IDataValue
	{
	public:
		DataValueChar(bool bKey=false);
		DataValueChar(int8_t val, bool bKey = false);
		DataValueChar(char* byArray, bool bKey = false);
		DataValueChar(const DataValueChar& src);
		DataValueChar(std::any val, bool bKey = false);
		~DataValueChar(){}
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

		operator int8_t() const;
		DataValueChar& operator=(int8_t val);
		DataValueChar& operator=(const DataValueChar& src);

		bool operator > (const DataValueChar& dv) const;
		bool operator < (const DataValueChar& dv) const;
		bool operator >= (const DataValueChar& dv) const;
		bool operator <= (const DataValueChar& dv) const;
		bool operator == (const DataValueChar& dv) const;
		bool operator != (const DataValueChar& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueChar& dv);

	protected:
		union {
			int8_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueChar& dv);
}