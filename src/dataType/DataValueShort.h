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
		DataValueShort(Byte* byArray, bool bKey = false);
		DataValueShort(const DataValueShort& src);
		DataValueShort(std::any val, bool bKey = false);
		~DataValueShort(){}
	public:
		std::any GetValue() const override;
		uint32_t WriteData(Byte* buf);
		uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
		uint32_t GetLength() const override;
		uint32_t GetMaxLength() const override;
		uint32_t GetPersistenceLength() const override;
		void SetMinValue() override;
		void SetMaxValue() override;
		void SetDefaultValue() override;

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
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueShort& dv);
}