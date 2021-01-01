#include "DataValueByte.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"

namespace storage {
	DataValueByte::DataValueByte(bool bKey)
		:IDataValue(DataType::BYTE, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueByte::DataValueByte(uint8_t val, bool bKey)
		: IDataValue(DataType::BYTE, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueByte::DataValueByte(Byte* byArray, bool bKey)
		: IDataValue(DataType::BYTE, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueByte::DataValueByte(std::any val, bool bKey)
		: IDataValue(DataType::BYTE, ValueType::SOLE_VALUE, bKey)
	{
		if (val.type() == typeid(int64_t)) soleValue_ = (uint8_t)std::any_cast<int64_t>(val);
		else if (val.type() == typeid(int32_t)) soleValue_ = (uint8_t)std::any_cast<int32_t>(val);
		else if (val.type() == typeid(int16_t)) soleValue_ = (uint8_t)std::any_cast<int16_t>(val);
		else if (val.type() == typeid(uint64_t)) soleValue_ = (uint8_t)std::any_cast<uint64_t>(val);
		else if (val.type() == typeid(uint32_t)) soleValue_ = (uint8_t)std::any_cast<uint32_t>(val);
		else if (val.type() == typeid(uint16_t)) soleValue_ = (uint8_t)std::any_cast<uint16_t>(val);
		else if (val.type() == typeid(int8_t)) soleValue_ = std::any_cast<int8_t>(val);
		else if (val.type() == typeid(uint8_t)) soleValue_ = std::any_cast<uint8_t>(val);
		else if (val.type() == typeid(std::string)) soleValue_ = (uint8_t)std::stoi(std::any_cast<std::string>(val));
		else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueByte" });
	}

	DataValueByte::DataValueByte(const DataValueByte& src)
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

	std::any DataValueByte::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::UInt8FromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueByte::WriteData(Byte* buf)
	{
		if (bKey_)
		{
			if (valType_ == ValueType::NULL_VALUE)
			{
				*buf = 0;
			}
			else if (valType_ == ValueType::BYTES_VALUE)
			{
				*buf = *byArray_;
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::UInt8ToBytes(soleValue_, buf, true);
			}

			return sizeof(int8_t);
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
				*buf = *byArray_;
				return sizeof(int8_t) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				utils::UInt8ToBytes(soleValue_, buf, false);
				return sizeof(int8_t) + 1;
			}
		}
	}

	uint32_t DataValueByte::ReadData(Byte* buf, uint32_t len)
	{
		if (bKey_)
		{
			if (len == -1)
			{
				valType_ = ValueType::NULL_VALUE;
				return sizeof(uint8_t);
			}

			valType_ = ValueType::BYTES_VALUE;
			byArray_ = buf;
			return sizeof(uint8_t);
		}
		else
		{
			valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			byArray_ = buf;
			return sizeof(uint8_t) + 1;
		}
	}

	uint32_t DataValueByte::GetDataLength() const
	{
		return bKey_ ? sizeof(uint8_t) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(uint8_t));
	}

	uint32_t DataValueByte::GetMaxLength() const
	{
		return sizeof(int8_t);
	}

	uint32_t DataValueByte::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(uint8_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(uint8_t));
	}

	void DataValueByte::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}
	void DataValueByte::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = UCHAR_MAX;
	}
	void DataValueByte::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}

	DataValueByte::operator uint8_t() const
	{
		switch (valType_)
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return  utils::UInt8FromBytes(byArray_, bKey_);;
		}

		return 0;
	}

	DataValueByte& DataValueByte::operator=(uint8_t val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueByte& DataValueByte::operator=(const DataValueByte& src)
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

	bool DataValueByte::operator > (const DataValueByte& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		uint8_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::UInt8FromBytes(byArray_, bKey_));
		uint8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::UInt8FromBytes(dv.byArray_, bKey_));
		return v1 > v2;
	}

	bool DataValueByte::operator < (const DataValueByte& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueByte::operator >= (const DataValueByte& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		uint8_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::UInt8FromBytes(byArray_, bKey_));
		uint8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::UInt8FromBytes(dv.byArray_, bKey_));
		return v1 >= v2;
	}

	bool DataValueByte::operator <= (const DataValueByte& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueByte::operator == (const DataValueByte& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		uint8_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::UInt8FromBytes(byArray_, bKey_));
		uint8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::UInt8FromBytes(dv.byArray_, bKey_));
		return v1 == v2;
	}

	bool DataValueByte::operator != (const DataValueByte& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueByte& dv)
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
			os << utils::UInt8FromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}