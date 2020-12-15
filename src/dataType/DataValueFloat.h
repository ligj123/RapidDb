#pragma once
#include "IDataValue.h"
#include <ostream>
#include <any>

namespace storage {
	class DataValueFloat : public IDataValue
	{
	public:
		DataValueFloat(bool bKey=false);
		DataValueFloat(float val, bool bKey = false);
		DataValueFloat(char* byArray, bool bKey = false);
		DataValueFloat(const DataValueFloat& src);
		DataValueFloat(std::any val, bool bKey = false);
		~DataValueFloat(){}
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
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueFloat& dv);
}