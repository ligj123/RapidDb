#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueDate : public IDataValue
	{
	public:
		DataValueDate(bool bKey = false);
		DataValueDate(uint64_t msVal, bool bKey = false);
		DataValueDate(Byte* byArray, bool bKey = false);
		DataValueDate(const DataValueDate& src);
		DataValueDate(std::any val, bool bKey = false);
		~DataValueDate() {}
	public:
		DataValueDate* CloneDataValue(bool incVal = false) override;
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
		void SetDefaultValue()override;

		/**millisecond since 1970/1/1*/
		operator uint64_t() const;
		/**millisecond since 1970/1/1*/
		DataValueDate& operator=(uint64_t msVal);
		DataValueDate& operator=(const DataValueDate& src);

		bool operator > (const DataValueDate& dv) const;
		bool operator < (const DataValueDate& dv) const;
		bool operator >= (const DataValueDate& dv) const;
		bool operator <= (const DataValueDate& dv) const;
		bool operator == (const DataValueDate& dv) const;
		bool operator != (const DataValueDate& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueDate& dv);
	protected:
		union {
			/**millisecond since 1970/1/1*/
			uint64_t soleValue_;
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueDate& dv);
}