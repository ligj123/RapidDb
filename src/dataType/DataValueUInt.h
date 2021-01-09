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
		DataValueUInt(Byte* byArray, bool bKey = false);
		DataValueUInt(const DataValueUInt& src);
		DataValueUInt(std::any val, bool bKey = false);
		~DataValueUInt(){}
	public:
		DataValueUInt* CloneDataValue(bool incVal = false) override;
		uint32_t WriteData(Byte* buf, bool key) override;
		uint32_t GetPersistenceLength(bool key) const override;

		std::any GetValue() const override;
		uint32_t WriteData(Byte* buf) override;
		uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
		uint32_t GetDataLength() const override;
		uint32_t GetMaxLength() const override;
		uint32_t GetPersistenceLength() const override;
		void SetMinValue() override;
		void SetMaxValue() override;
		void SetDefaultValue() override;

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
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueUInt& dv);
}