#include "DataValueShort.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"
#include "../config/ErrorID.h"
#include <cstring>
#include <limits.h>

namespace storage {
	DataValueShort::DataValueShort(bool bKey)
		:IDataValue(DataType::SHORT, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueShort::DataValueShort(int16_t val, bool bKey)
		: IDataValue(DataType::SHORT, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueShort::DataValueShort(Byte* byArray, bool bKey)
		: IDataValue(DataType::SHORT, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueShort::DataValueShort(std::any val, bool bKey)
		: IDataValue(DataType::SHORT, ValueType::SOLE_VALUE, bKey)
	{
		if (val.type() == typeid(int64_t)) soleValue_ = (uint16_t)std::any_cast<int64_t>(val);
		else if (val.type() == typeid(int32_t)) soleValue_ = (uint16_t)std::any_cast<int32_t>(val);
		else if (val.type() == typeid(int16_t)) soleValue_ = std::any_cast<int16_t>(val);
		else if (val.type() == typeid(uint64_t)) soleValue_ = (uint16_t)std::any_cast<uint64_t>(val);
		else if (val.type() == typeid(uint32_t)) soleValue_ = (uint16_t)std::any_cast<uint32_t>(val);
		else if (val.type() == typeid(uint16_t)) soleValue_ = std::any_cast<uint16_t>(val);
		else if (val.type() == typeid(int8_t)) soleValue_ = std::any_cast<int8_t>(val);
		else if (val.type() == typeid(uint8_t)) soleValue_ = std::any_cast<uint8_t>(val);
		else if (val.type() == typeid(std::string)) soleValue_ = (uint16_t)std::stoi(std::any_cast<std::string>(val));
		else throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT, { val.type().name(), "DataValueShort" });
	}

	DataValueShort::DataValueShort(const DataValueShort& src)
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

	DataValueShort* DataValueShort::CloneDataValue(bool incVal)
	{
		return new DataValueShort(*this);
	}

	std::any DataValueShort::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::Int16FromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueShort::WriteData(Byte* buf)
	{
		return WriteData(buf, bKey_);
	}

	uint32_t DataValueShort::GetPersistenceLength(bool key) const
	{
		return key ? sizeof(int16_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(int16_t));
	}

	uint32_t DataValueShort::WriteData(Byte* buf, bool key)
	{
		if (key)
		{
			assert(valType_ != ValueType::NULL_VALUE);
			if (valType_ == ValueType::BYTES_VALUE)
			{
				std::memcpy(buf, byArray_, sizeof(int16_t));
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::Int16ToBytes(soleValue_, buf, bKey_);
			}

			return sizeof(int16_t);
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
				std::memcpy(buf, byArray_, sizeof(int16_t));
				return sizeof(int16_t) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				utils::Int16ToBytes(soleValue_, buf, bKey_);
				return sizeof(int16_t) + 1;
			}
		}
	}

	uint32_t DataValueShort::ReadData(Byte* buf, uint32_t len)
	{
		if (bKey_)
		{
			valType_ = ValueType::SOLE_VALUE;
			soleValue_ = utils::Int16FromBytes(buf, bKey_);
			return sizeof(int16_t);
		}
		else
		{
			valType_ = (*buf ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			soleValue_ = utils::Int16FromBytes(buf, bKey_);
			return sizeof(int16_t) + 1;
		}
	}

	uint32_t DataValueShort::GetDataLength() const
	{
		return bKey_ ? sizeof(int16_t) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(int16_t));
	}

	uint32_t DataValueShort::GetMaxLength() const
	{
		return sizeof(int16_t);
	}
	
	uint32_t DataValueShort::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(int16_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(int16_t));
	}

	void DataValueShort::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = SHRT_MIN;
	}
	void DataValueShort::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = SHRT_MAX;
	}
	void DataValueShort::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}

	DataValueShort::operator int16_t() const
	{
		switch (valType_) 
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::Int16FromBytes(byArray_, bKey_);
		}

		return 0;
	}

	DataValueShort& DataValueShort::operator=(int16_t val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueShort& DataValueShort::operator=(const DataValueShort& src)
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

	bool DataValueShort::operator > (const DataValueShort& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int16_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int16FromBytes(byArray_, bKey_));
		int16_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int16FromBytes(dv.byArray_, dv.bKey_));
		return v1 > v2;
	}

	bool DataValueShort::operator < (const DataValueShort& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueShort::operator >= (const DataValueShort& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int16_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int16FromBytes(byArray_, bKey_));
		int16_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int16FromBytes(dv.byArray_, dv.bKey_));
		return v1 >= v2;
	}

	bool DataValueShort::operator <= (const DataValueShort& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueShort::operator == (const DataValueShort& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		int16_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int16FromBytes(byArray_, bKey_));
		int16_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int16FromBytes(dv.byArray_, dv.bKey_));
		return v1 == v2;
	}

	bool DataValueShort::operator != (const DataValueShort& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueShort& dv)
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
			os << utils::Int16FromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}