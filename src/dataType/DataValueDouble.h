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
		DataValueDouble(Byte* byArray, bool bKey = false);
		DataValueDouble(const DataValueDouble& src);
		DataValueDouble(std::any val, bool bKey = false);
		~DataValueDouble(){}
	public:
		 std::any GetValue() const override;
		 uint32_t WriteData(Byte* buf) override;
		 uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
		 uint32_t GetDataLength() const override;
		 uint32_t GetMaxLength() const override;
		 uint32_t GetPersistenceLength() const override;
		 void SetMinValue() override;
		 void SetMaxValue() override;
		 void SetDefaultValue() override;

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
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueDouble& dv);
}