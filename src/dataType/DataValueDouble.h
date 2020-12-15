#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueDouble : public IDataValue
	{
	public:
		DataValueDouble(bool bKey=false);
		DataValueDouble(double val, bool bKey = false);
		DataValueDouble(char* byArray, bool bKey = false);
		DataValueDouble(const DataValueDouble& src);
		DataValueDouble(std::any val, bool bKey = false);
		~DataValueDouble(){}
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

		operator double() const;
		DataValueDouble& operator=(double val);
		DataValueDouble& operator=(const DataValueDouble& src);

		bool operator > (const DataValueDouble& dv) const;
		bool operator < (const DataValueDouble& dv) const;
		bool operator >= (const DataValueDouble& dv) const;
		bool operator <= (const DataValueDouble& dv) const;
		bool operator == (const DataValueDouble& dv) const;
		bool operator != (const DataValueDouble& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueDouble& dv);

	protected:
		union {
			double soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueDouble& dv);
}