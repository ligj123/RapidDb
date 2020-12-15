#include "DataValueFloat.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"

namespace storage {
	DataValueFloat::DataValueFloat(bool bKey)
		:IDataValue(DataType::FLOAT, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueFloat::DataValueFloat(float val, bool bKey)
		: IDataValue(DataType::FLOAT, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueFloat::DataValueFloat(char* byArray, bool bKey)
		: IDataValue(DataType::FLOAT, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueFloat::DataValueFloat(std::any val, bool bKey)
		: IDataValue(DataType::FLOAT, ValueType::SOLE_VALUE, bKey)
	{
		if (val.type() == typeid(double)) soleValue_ = (float)std::any_cast<double>(val);
		else if (val.type() == typeid(float)) soleValue_ = std::any_cast<float>(val);
		else if (val.type() == typeid(int64_t)) soleValue_ = (float)std::any_cast<int64_t>(val);
		else if (val.type() == typeid(int32_t)) soleValue_ = (float)std::any_cast<int32_t>(val);
		else if (val.type() == typeid(int16_t)) soleValue_ = (float)std::any_cast<int16_t>(val);
		else if (val.type() == typeid(uint64_t)) soleValue_ = (float)std::any_cast<uint64_t>(val);
		else if (val.type() == typeid(uint32_t)) soleValue_ = (float)std::any_cast<uint32_t>(val);
		else if (val.type() == typeid(uint16_t)) soleValue_ = (float)std::any_cast<uint16_t>(val);
		else if (val.type() == typeid(int8_t)) soleValue_ = (float)std::any_cast<int8_t>(val);
		else if (val.type() == typeid(uint8_t)) soleValue_ = (float)std::any_cast<uint8_t>(val);
		else if (val.type() == typeid(std::string)) soleValue_ = std::stof(std::any_cast<std::string>(val));
		else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueFloat" });
	}

	DataValueFloat::DataValueFloat(const DataValueFloat& src)
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

	std::any DataValueFloat::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::FloatFromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueFloat::WriteData(char* buf)
	{
		if (bKey_)
		{
			if (valType_ == ValueType::NULL_VALUE)
			{
				std::memset(buf, 0, sizeof(float));
			}
			else if (valType_ == ValueType::BYTES_VALUE)
			{
				std::memcpy(buf, byArray_, sizeof(float));
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::FloatToBytes(soleValue_, buf, bKey_);
			}

			return sizeof(float);
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
				std::memcpy(buf, byArray_, sizeof(float));
				return sizeof(float) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				utils::FloatToBytes(soleValue_, buf, bKey_);
				return sizeof(float) + 1;
			}
		}
	}

	uint32_t DataValueFloat::ReadData(char* buf, uint32_t len)
	{
		if (bKey_)
		{
			if (len == -1)
			{
				valType_ = ValueType::NULL_VALUE;
				return sizeof(float);
			}

			valType_ = ValueType::BYTES_VALUE;
			byArray_ = buf;
			return sizeof(float);
		}
		else
		{
			valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			byArray_ = buf;
			return sizeof(float) + 1;
		}
	}

	uint32_t DataValueFloat::GetLength() const
	{
		return bKey_ ? sizeof(float) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(float));
	}

	uint32_t DataValueFloat::GetMaxLength() const
	{
		return sizeof(float) + (bKey_ ? 0 : 1);
	}
	
	uint32_t DataValueFloat::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(float) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(float));
	}

	void DataValueFloat::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = FLT_MIN;
	}
	void DataValueFloat::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = FLT_MAX;
	}
	void DataValueFloat::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}

	DataValueFloat::operator float() const
	{
		switch (valType_) 
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return  utils::FloatFromBytes(byArray_, bKey_);
		}

		return 0;
	}

	DataValueFloat& DataValueFloat::operator=(float val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueFloat& DataValueFloat::operator=(const DataValueFloat& src)
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

	bool DataValueFloat::operator > (const DataValueFloat& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		float v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::FloatFromBytes(byArray_, bKey_));
		float v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::FloatFromBytes(dv.byArray_, dv.bKey_));
		return v1 > v2;
	}

	bool DataValueFloat::operator < (const DataValueFloat& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueFloat::operator >= (const DataValueFloat& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		float v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::FloatFromBytes(byArray_, bKey_));
		float v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::FloatFromBytes(dv.byArray_, dv.bKey_));
		return v1 >= v2;
	}

	bool DataValueFloat::operator <= (const DataValueFloat& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueFloat::operator == (const DataValueFloat& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		float v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::FloatFromBytes(byArray_, bKey_));
		float v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::FloatFromBytes(dv.byArray_, dv.bKey_));
		return v1 == v2;
	}

	bool DataValueFloat::operator != (const DataValueFloat& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueFloat& dv)
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
			os << utils::FloatFromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}