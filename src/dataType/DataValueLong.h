#pragma once
#include "IDataValue.h"
#include <ostream>

namespace storage {
	class DataValueLong : public IDataValue
	{
	public:
		DataValueLong(ColumnBase* pColumn, bool bKey=false);
		DataValueLong(int64_t val, ColumnBase* pColumn, bool bKey = false);
		DataValueLong(char* byArray, ColumnBase* pColumn, bool bKey = false);
	public:
		virtual std::any GetValue() const;
		virtual uint32_t WriteData(char* buf);
		virtual uint32_t ReadData(char* buf, uint32_t len = 0);
		virtual uint32_t GetLength() const;
		virtual uint32_t GetMaxLength() const;
		virtual void SetMinValue();
		virtual void SetMaxValue();
		virtual void SetDefaultValue();

		operator int64_t() const;
		bool operator > (const DataValueLong& dv) const;
		bool operator < (const DataValueLong& dv) const;
		bool operator >= (const DataValueLong& dv) const;
		bool operator <= (const DataValueLong& dv) const;
		bool operator == (const DataValueLong& dv) const;
		bool operator != (const DataValueLong& dv) const;
		friend std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
	protected:
		static int64_t BytesToInt64(char* byArr, bool bKey)
		{
			int64_t val;
			if (bKey && !IsBigEndian()) {
				char* ar = (char*)&val;
				ar[0] = byArr[7];
				ar[1] = byArr[6];
				ar[2] = byArr[5];
				ar[3] = byArr[4];
				ar[4] = byArr[3];
				ar[5] = byArr[2];
				ar[6] = byArr[1];
				ar[7] = byArr[0];
			}
			else
			{
				val = *((int64_t*)byArr);
			}

			return val;
		}
	protected:
		union {
			int64_t soleValue_;
			char* byArray_;
		};
	};
	std::ostream& operator<< (std::ostream& os, const DataValueLong& dv);
}