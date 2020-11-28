#include "DataValueLong.h"
#include <stdexcept>

namespace storage {
	DataValueLong::DataValueLong(ColumnBase* pColumn, bool bKey)
		:IDataValue(DataType::LONG, ValueType::NULL_VALUE, pColumn, bKey)
	{
	}

	DataValueLong::DataValueLong(int64_t val, ColumnBase* pColumn, bool bKey)
		: IDataValue(DataType::LONG, ValueType::SOLE_VALUE, pColumn, bKey), soleValue_(val)
	{
	}

	DataValueLong::DataValueLong(char* byArray, ColumnBase* pColumn, bool bKey)
		: IDataValue(DataType::LONG, ValueType::BYTES_VALUE, pColumn, bKey), byArray_(byArray)
	{
	}

	std::any DataValueLong::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return BytesToInt64(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueLong::WriteData(char* buf)
	{
		if (bKey_)
		{
			if (valType_ == ValueType::BYTES_VALUE)
				{
					std::memcpy(buf, byArray_, sizeof(int64_t));
				}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				if (IsBigEndian())
					std::memcpy(buf, &soleValue_, sizeof(int64_t));
				else {
					char* ar = (char*)&soleValue_;
					buf[7] = ar[0];
					buf[6] = ar[1];
					buf[5] = ar[2];
					buf[4] = ar[3];
					buf[3] = ar[4];
					buf[2] = ar[5];
					buf[1] = ar[6];
					buf[0] = ar[7];
				}
			}
			
			return sizeof(int64_t);
		}
		else
		{
			if (valType_ == ValueType::NULL_VALUE)
			{
				*buf = 0;
				return 1;
			}
			else if (valType_ == ValueType::BYTES_VALUE)
			{
				*buf = 1;
				buf++;
				std::memcpy(buf, byArray_, sizeof(int64_t));
				return sizeof(int64_t) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				std::memcpy(buf, reinterpret_cast<char*>(&soleValue_), sizeof(int64_t));
				return sizeof(int64_t) + 1;
			}
		}
	}

	uint32_t DataValueLong::ReadData(char* buf, uint32_t len)
	{
		if (bKey_)
		{
			valType_ = ValueType::BYTES_VALUE;
			byArray_ = buf;
			return sizeof(int64_t);
		}
		else
		{
			valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			byArray_ = buf;
			return sizeof(int64_t) + 1;
		}
	}

	uint32_t DataValueLong::GetLength() const
	{
		return sizeof(int64_t) + (bKey_ ? 0 : 1);
	}

	uint32_t DataValueLong::GetMaxLength() const
	{
		return sizeof(int64_t) + (bKey_ ? 0 : 1);
	}

	void DataValueLong::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = LLONG_MIN;
	}
	void DataValueLong::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = LLONG_MAX;
	}
	void DataValueLong::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		ColumnInTable* p = static_cast<ColumnInTable*>(pColumn_);
		soleValue_ = (p != nullptr && p->getDefaultVal() != nullptr ?
				(int64_t)(*(DataValueLong*)(p->getDefaultVal())) : 0);
	}

	DataValueLong::operator int64_t() const
	{
		switch (valType_) 
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return  *(reinterpret_cast<long*>(byArray_));
		}

		return 0;
	}

	bool DataValueLong::operator > (const DataValueLong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : BytesToInt64(byArray_, bKey_));
		int64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : BytesToInt64(dv.byArray_, dv.bKey_));
		return v1 > v2;
	}

	bool DataValueLong::operator < (const DataValueLong& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueLong::operator >= (const DataValueLong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : BytesToInt64(byArray_, bKey_));
		int64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : BytesToInt64(dv.byArray_, dv.bKey_));
		return v1 >= v2;
	}

	bool DataValueLong::operator <= (const DataValueLong& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueLong::operator == (const DataValueLong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		int64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : BytesToInt64(byArray_, bKey_));
		int64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : BytesToInt64(dv.byArray_, dv.bKey_));
		return v1 == v2;
	}

	bool DataValueLong::operator != (const DataValueLong& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueLong& dv)
	{
		switch (dv.valType_)
		{
		case ValueType::NULL_VALUE:
			os << "nullptr";
			break;
		case ValueType::SOLE_VALUE:
			os << dv.soleValue_;
			break;
		case ValueType::BYTES_VALUE:
			os << *(reinterpret_cast<int64_t*>(dv.byArray_));
			break;
		}

		return os;
	}
}