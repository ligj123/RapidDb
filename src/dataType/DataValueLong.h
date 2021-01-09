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
		DataValueLong(Byte* byArray, bool bKey = false);
		DataValueLong(const DataValueLong& src);
		DataValueLong(std::any val, bool bKey = false);
		~DataValueLong(){}
	public:
		DataValueLong* CloneDataValue(bool incVal = false) override;
		uint32_t WriteData(Byte* buf, bool key) override;
		uint32_t GetPersistenceLength(bool key) const override;

		std::any GetValue() const override;
		uint32_t WriteData(Byte* buf);
		uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
		uint32_t GetDataLength() const override;
		uint32_t GetMaxLength() const override;
		uint32_t GetPersistenceLength() const override;
		void SetMinValue() override;
		void SetMaxValue() override;
		void SetDefaultValue() override;

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
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
}