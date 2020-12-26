#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueFloat : public IDataValue
	{
	public:
		DataValueFloat(bool bKey = false);
		DataValueFloat(float val, bool bKey = false);
		DataValueFloat(Byte* byArray, bool bKey = false);
		DataValueFloat(const DataValueFloat& src);
		DataValueFloat(std::any val, bool bKey = false);
		~DataValueFloat() {}
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

		operator float() const;
		DataValueFloat& operator=(float val);
		DataValueFloat& operator=(const DataValueFloat& src);

		bool operator > (const DataValueFloat& dv) const;
		bool operator < (const DataValueFloat& dv) const;
		bool operator >= (const DataValueFloat& dv) const;
		bool operator <= (const DataValueFloat& dv) const;
		bool operator == (const DataValueFloat& dv) const;
		bool operator != (const DataValueFloat& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueFloat& dv);

	protected:
		union {
			float soleValue_;
			Byte* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueFloat& dv);
}