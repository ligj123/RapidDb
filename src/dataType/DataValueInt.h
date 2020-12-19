#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueInt : public IDataValue
	{
	public:
		DataValueInt(bool bKey = false);
		DataValueInt(int32_t val, bool bKey = false);
		DataValueInt(Byte* byArray, bool bKey = false);
		DataValueInt(const DataValueInt& src);
		DataValueInt(std::any val, bool bKey = false);
		~DataValueInt() {}
	public:
		std::any GetValue() const override;
		uint32_t WriteData(Byte* buf) override;
		uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
		uint32_t GetLength() const override;
		uint32_t GetMaxLength() const override;
		uint32_t GetPersistenceLength() const override;
		void SetMinValue() override;
		void SetMaxValue() override;
		void SetDefaultValue() override;

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
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueInt& dv);
}