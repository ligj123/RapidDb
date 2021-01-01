#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueBool : public IDataValue
	{
	public:
		DataValueBool(bool bKey = false);
		DataValueBool(bool val, bool bKey = false);
		DataValueBool(Byte* byArray, bool bKey = false);
		DataValueBool(const DataValueBool& src);
		DataValueBool(std::any val, bool bKey = false);
		~DataValueBool() {}
	public:
		std::any GetValue() const override;
		uint32_t WriteData(Byte* buf) override;
		uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
		uint32_t GetDataLength() const override;
		uint32_t GetMaxLength() const override;
		uint32_t GetPersistenceLength() const override;
		void SetMinValue() override;
		void SetMaxValue() override;
		void SetDefaultValue()override;

		operator bool() const;
		DataValueBool& operator=(bool val);
		DataValueBool& operator=(const DataValueBool& src);

		bool operator > (const DataValueBool& dv) const;
		bool operator < (const DataValueBool& dv) const;
		bool operator >= (const DataValueBool& dv) const;
		bool operator <= (const DataValueBool& dv) const;
		bool operator == (const DataValueBool& dv) const;
		bool operator != (const DataValueBool& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueBool& dv);
	protected:
		union {
			bool soleValue_;
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueBool& dv);
}