#include "DataValueChar.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"

namespace storage {
	DataValueChar::DataValueChar(bool bKey)
		:IDataValue(DataType::CHAR, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueChar::DataValueChar(int8_t val, bool bKey)
		: IDataValue(DataType::CHAR, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueChar::DataValueChar(char* byArray, bool bKey)
		: IDataValue(DataType::CHAR, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueChar::DataValueChar(std::any val, bool bKey)
		: IDataValue(DataType::CHAR, ValueType::SOLE_VALUE, bKey)
	{
		if (val.type() == typeid(int64_t)) soleValue_ = (int8_t)std::any_cast<int64_t>(val);
		else if (val.type() == typeid(int32_t)) soleValue_ = (int8_t)std::any_cast<int32_t>(val);
		else if (val.type() == typeid(int16_t)) soleValue_ = (int8_t)std::any_cast<int16_t>(val);
		else if (val.type() == typeid(uint64_t)) soleValue_ = (int8_t)std::any_cast<uint64_t>(val);
		else if (val.type() == typeid(uint32_t)) soleValue_ = (int8_t)std::any_cast<uint32_t>(val);
		else if (val.type() == typeid(uint16_t)) soleValue_ = (int8_t)std::any_cast<uint16_t>(val);
		else if (val.type() == typeid(int8_t)) soleValue_ = std::any_cast<int8_t>(val);
		else if (val.type() == typeid(uint8_t)) soleValue_ = std::any_cast<uint8_t>(val);
		else if (val.type() == typeid(std::string)) soleValue_ = (int8_t)std::stoll(std::any_cast<std::string>(val));
		else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueChar" });
	}

	DataValueChar::DataValueChar(const DataValueChar& src)
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

	std::any DataValueChar::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::Int8FromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueChar::WriteData(char* buf)
	{
		if (bKey_)
		{
			if (valType_ == ValueType::NULL_VALUE)
			{
				std::memset(buf, 0, sizeof(int8_t));
			}
			else if (valType_ == ValueType::BYTES_VALUE)
			{
				std::memcpy(buf, byArray_, sizeof(int8_t));
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::Int8ToBytes(soleValue_, buf, bKey_);
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
				std::memcpy(buf, byArray_, sizeof(int8_t));
				return sizeof(int8_t) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				utils::Int8ToBytes(soleValue_, buf, bKey_);
				return sizeof(int8_t) + 1;
			}
		}
	}

	uint32_t DataValueChar::ReadData(char* buf, uint32_t len)
	{
		if (bKey_)
		{
			if (len == -1)
			{
				valType_ = ValueType::NULL_VALUE;
				return sizeof(int8_t);
			}

			valType_ = ValueType::BYTES_VALUE;
			byArray_ = buf;
			return sizeof(int8_t);
		}
		else
		{
			valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			byArray_ = buf;
			return sizeof(int8_t) + 1;
		}
	}

	uint32_t DataValueChar::GetLength() const
	{
		return bKey_ ? sizeof(int8_t) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(int8_t));
	}

	uint32_t DataValueChar::GetMaxLength() const
	{
		return sizeof(int8_t) + (bKey_ ? 0 : 1);
	}
	
	uint32_t DataValueChar::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(int8_t) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(int8_t));
	}

	void DataValueChar::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = CHAR_MIN;
	}
	void DataValueChar::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = CHAR_MAX;
	}
	void DataValueChar::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}

	DataValueChar::operator int8_t() const
	{
		switch (valType_) 
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return  utils::Int8FromBytes(byArray_, bKey_);
		}

		return 0;
	}

	DataValueChar& DataValueChar::operator=(int8_t val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueChar& DataValueChar::operator=(const DataValueChar& src)
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

	bool DataValueChar::operator > (const DataValueChar& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int8_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int8FromBytes(byArray_, bKey_));
		int8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int8FromBytes(dv.byArray_, dv.bKey_));
		return v1 > v2;
	}

	bool DataValueChar::operator < (const DataValueChar& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueChar::operator >= (const DataValueChar& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		int8_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int8FromBytes(byArray_, bKey_));
		int8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int8FromBytes(dv.byArray_, dv.bKey_));
		return v1 >= v2;
	}

	bool DataValueChar::operator <= (const DataValueChar& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueChar::operator == (const DataValueChar& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		int8_t v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::Int8FromBytes(byArray_, bKey_));
		int8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::Int8FromBytes(dv.byArray_, dv.bKey_));
		return v1 == v2;
	}

	bool DataValueChar::operator != (const DataValueChar& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueChar& dv)
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
			os << utils::Int8FromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}