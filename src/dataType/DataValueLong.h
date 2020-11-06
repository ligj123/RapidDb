#pragma once
#include "TDataValue.h"
#include <ostream>

namespace storage {
	class DataValueLong : public TDataValue<int64_t>
	{
	public:
		DataValueLong();
		DataValueLong(int64_t val);
		DataValueLong(char* byArray, int spos);
	public:
		operator int64_t();
		virtual void SetMinValue();
		virtual void SetMaxValue();
		virtual void SetDefaultValue();

		friend std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
	};
	std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
}