#include "DataValueLong.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"

namespace storage {
	DataValueLong::DataValueLong(bool bKey)
		:IDataValue(DataType::LONG, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueLong::DataValueLong(int64_t val, bool bKey)
		: IDataValue(DataType::LONG, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueLong::DataValueLong(Byte* byArray, bool bKey)
		: IDataValue(DataType::LONG, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueLong::DataValueLong(std::any val, bool bKey)
		: IDataValue(DataType::LONG, ValueType::SOLE_VALUE, bKey)
	{
		if (val.type() == typeid(int64_t)) soleValue_ = std::any_cast<int64_t>(val);
		else if (val.type() == typeid(int32_t)) soleValue_ = std::any_cast<int32_t>(val);
		else if (val.type() == typeid(int16_t)) soleValue_ = std::any_cast<int16_t>(val);
		else if (val.type() == typeid(uint64_t)) soleValue_ = std::any_cast<uint64_t>(val);
		else if (val.type() == typeid(uint32_t)) soleValue_ = std::any_cast<uint32_t>(val);
		else if (val.type() == typeid(uint16_t)) soleValue_ = std::any_cast<uint16_t>(val);
		else if (val.type() == typeid(int8_t)) soleValue_ = std::any_cast<int8_t>(val);
		else if (val.type() == typeid(uint8_t)) soleValue_ = std::any_cast<uint8_t>(val);
		else if (val.type() == typeid(std::string)) soleValue_ = std::stoll(std::any_cast<std::string>(val));
		else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueLong" });
	}

	DataValueLong::DataValueLong(const DataValueLong& src)
		: IDataValue(src)
	{
		switch (src.valType_)
		{
		case ValueType::SOLE_VALUE:
			soleValue_ = src.soleValue_;
			break;
		case ValueType::BYTES_VALUE:
			byArray_ = src.byArray_;
			break;
		case ValueType::NULL_VALUE:
			break;
		}
	}

	std::any DataValueLong::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::Int64FromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueLong::WriteData(Byte* buf)
	{
		if (bKey_)
		{
			if (valType_ == ValueType::NULL_VALUE)
			{
				std::memset(buf, 0, sizeof(int64_t));
			}
			else if (valType_ == ValueType::BYTES_VALUE)
			{
				std::memcpy(buf, byArray_, sizeof(int64_t));
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::Int64ToBytes(soleValue_, buf, true);
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
				utils::Int64ToBytes(soleValue_, buf, false);
				return sizeof(int64_t) + 1;
			}
		}
	}

	uint32_t DataValueLong::ReadData(Byte* buf, uint32_t len)
	{
		if (bKey_)
		{
			if (len == -1)
			{
				valType_ = ValueType::NULL_VALUE;
				return sizeof(int64_t);
			}

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

	uint32_t DataValueLong::GetDataLength() const
	{
		return bKey_ ? sizeof(int64_t) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(int64_t));
	}

	uint32_t DataValueLong::GetMaxLength() const
	{
		return sizeof(int64_t);
	}
	
	uint32_t DataValueLong::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(int64_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(int64_t));
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
		soleValue_ = 0;
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
			return utils::Int64FromBytes(byArray_, bKey_);
		}

		return 0;
	}

	DataValueLong& DataValueLong::operator=(int64_t val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueLong& DataValueLong::operator=(const DataValueLong& src)
	{
		dataType_ = src.dataType_;
		valType_ = src.valType_;
		bKey_ = src.bKey_;
		switch (src.valType_)
		{
		case ValueType::SOLE_VALUE:
			soleValue_ = src.soleValue_;
			break;
		case ValueType::BYTES_VALUE:
			byArray_ = src.byArray_;
			break;
		case ValueType::NULL_VALUE:
			break;
		}

		return *this;
	}

	bool DataValueLong::operator > (const DataValueLong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int64FromBytes(byArray_, bKey_));
		int64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int64FromBytes(dv.byArray_, dv.bKey_));
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

		int64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int64FromBytes(byArray_, bKey_));
		int64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int64FromBytes(dv.byArray_, dv.bKey_));
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

		int64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int64FromBytes(byArray_, bKey_));
		int64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int64FromBytes(dv.byArray_, dv.bKey_));
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
			os << utils::Int64FromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}