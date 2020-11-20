#pragma once
#include "TDataValue.h"
#include <ostream>

namespace storage {
	class DataValueLong : public TDataValue<int64_t>
	{
	public:
		DataValueLong();
		DataValueLong(int64_t val);
		DataValueLong(char* byArray);
	public:
		virtual void SetMinValue();
		virtual void SetMaxValue();
		virtual void SetDefaultValue();

		operator int64_t() const;
		bool operator > (const DataValueLong& dv) const { return Lg(dv); }
		bool operator < (const DataValueLong& dv) const { return !Le(dv); }
		bool operator >= (const DataValueLong& dv) const {return Le(dv); }
		bool operator <= (const DataValueLong& dv) const { return !Lg(dv); }
		bool operator == (const DataValueLong& dv) const { return Equal(dv); }
		bool operator != (const DataValueLong& dv) const { return !Equal(dv); }
		friend std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
	};
	std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
}