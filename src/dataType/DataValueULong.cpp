#include "DataValueULong.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"
#include "../config/ErrorID.h"
#include <cstring>
#include <limits.h>

namespace storage {
	DataValueULong::DataValueULong(bool bKey)
		:IDataValue(DataType::ULONG, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueULong::DataValueULong(uint64_t val, bool bKey)
		: IDataValue(DataType::ULONG, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueULong::DataValueULong(Byte* byArray, bool bKey)
		: IDataValue(DataType::ULONG, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueULong::DataValueULong(std::any val, bool bKey)
		: IDataValue(DataType::ULONG, ValueType::SOLE_VALUE, bKey)
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
		else throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT, { val.type().name(), "DataValueULong" });
	}

	DataValueULong::DataValueULong(const DataValueULong& src)
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

	DataValueULong* DataValueULong::CloneDataValue(bool incVal)
	{
		return new DataValueULong(*this);
	}

	std::any DataValueULong::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::UInt64FromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueULong::WriteData(Byte* buf)
	{
		return WriteData(buf, bKey_);
	}

	uint32_t DataValueULong::GetPersistenceLength(bool key) const
	{
		return key ? sizeof(uint64_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(uint64_t));
	}

	uint32_t DataValueULong::WriteData(Byte* buf, bool key)
	{
		if (key)
		{
			assert(valType_ != ValueType::NULL_VALUE);
			if (valType_ == ValueType::BYTES_VALUE)
			{
				std::memcpy(buf, byArray_, sizeof(uint64_t));
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::UInt64ToBytes(soleValue_, buf, bKey_);
			}

			return sizeof(uint64_t);
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
				std::memcpy(buf, byArray_, sizeof(uint64_t));
				return sizeof(uint64_t) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				utils::UInt64ToBytes(soleValue_, buf, bKey_);
				return sizeof(uint64_t) + 1;
			}
		}
	}

	uint32_t DataValueULong::ReadData(Byte* buf, uint32_t len)
	{
		if (bKey_)
		{
			valType_ = ValueType::SOLE_VALUE;
			soleValue_ = utils::UInt64FromBytes(buf, bKey_);
			return sizeof(uint64_t);
		}
		else
		{
			valType_ = (*buf ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			soleValue_ = utils::UInt64FromBytes(buf, bKey_);
			return sizeof(uint64_t) + 1;
		}
	}

	uint32_t DataValueULong::GetDataLength() const
	{
		return bKey_ ? sizeof(uint64_t) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(uint64_t));
	}

	uint32_t DataValueULong::GetMaxLength() const
	{
		return sizeof(uint64_t);
	}
	
	uint32_t DataValueULong::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(uint64_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(uint64_t));
	}

	void DataValueULong::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}
	void DataValueULong::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = ULLONG_MAX;
	}
	void DataValueULong::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}

	DataValueULong::operator uint64_t() const
	{
		switch (valType_) 
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::UInt64FromBytes(byArray_, bKey_);
		}

		return 0;
	}

	DataValueULong& DataValueULong::operator=(uint64_t val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueULong& DataValueULong::operator=(const char* pArr)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = std::atoll(pArr);
		return *this;
	}

	DataValueULong& DataValueULong::operator=(const DataValueULong& src)
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

	bool DataValueULong::operator > (const DataValueULong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		uint64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::UInt64FromBytes(byArray_, bKey_));
		uint64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::UInt64FromBytes(dv.byArray_, dv.bKey_));
		return v1 > v2;
	}

	bool DataValueULong::operator < (const DataValueULong& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueULong::operator >= (const DataValueULong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		uint64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::UInt64FromBytes(byArray_, bKey_));
		uint64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::UInt64FromBytes(dv.byArray_, dv.bKey_));
		return v1 >= v2;
	}

	bool DataValueULong::operator <= (const DataValueULong& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueULong::operator == (const DataValueULong& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		uint64_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::UInt64FromBytes(byArray_, bKey_));
		uint64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::UInt64FromBytes(dv.byArray_, dv.bKey_));
		return v1 == v2;
	}

	bool DataValueULong::operator != (const DataValueULong& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueULong& dv)
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
			os << utils::UInt64FromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}