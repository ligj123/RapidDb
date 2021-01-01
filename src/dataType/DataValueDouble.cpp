#include "DataValueDouble.h"
#include <stdexcept>
#include <string>
#include "../utils/ErrorMsg.h"
#include "../utils/BytesConvert.h"

namespace storage {
	DataValueDouble::DataValueDouble(bool bKey)
		:IDataValue(DataType::DOUBLE, ValueType::NULL_VALUE, bKey)
	{
	}

	DataValueDouble::DataValueDouble(double val, bool bKey)
		: IDataValue(DataType::DOUBLE, ValueType::SOLE_VALUE, bKey), soleValue_(val)
	{
	}

	DataValueDouble::DataValueDouble(Byte* byArray, bool bKey)
		: IDataValue(DataType::DOUBLE, ValueType::BYTES_VALUE, bKey), byArray_(byArray)
	{
	}

	DataValueDouble::DataValueDouble(std::any val, bool bKey)
		: IDataValue(DataType::DOUBLE, ValueType::SOLE_VALUE, bKey)
	{
		if (val.type() == typeid(double)) soleValue_ = std::any_cast<double>(val);
		else if (val.type() == typeid(float)) soleValue_ = std::any_cast<float>(val);
		else if (val.type() == typeid(int64_t)) soleValue_ = (double)std::any_cast<int64_t>(val);
		else if (val.type() == typeid(int32_t)) soleValue_ = std::any_cast<int32_t>(val);
		else if (val.type() == typeid(int16_t)) soleValue_ = std::any_cast<int16_t>(val);
		else if (val.type() == typeid(uint64_t)) soleValue_ = (double)std::any_cast<uint64_t>(val);
		else if (val.type() == typeid(uint32_t)) soleValue_ = std::any_cast<uint32_t>(val);
		else if (val.type() == typeid(uint16_t)) soleValue_ = std::any_cast<uint16_t>(val);
		else if (val.type() == typeid(int8_t)) soleValue_ = std::any_cast<int8_t>(val);
		else if (val.type() == typeid(uint8_t)) soleValue_ = std::any_cast<uint8_t>(val);
		else if (val.type() == typeid(std::string)) soleValue_ = std::stod(std::any_cast<std::string>(val));
		else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueDouble" });
	}

	DataValueDouble::DataValueDouble(const DataValueDouble& src)
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

	std::any DataValueDouble::GetValue() const
	{
		switch (valType_)
		{
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::DoubleFromBytes(byArray_, bKey_);
		case ValueType::NULL_VALUE:
		default:
			return std::any();
		}
	}

	uint32_t DataValueDouble::WriteData(Byte* buf)
	{
		if (bKey_)
		{
			if (valType_ == ValueType::NULL_VALUE)
			{
				std::memset(buf, 0, sizeof(double));
			}
			else if (valType_ == ValueType::BYTES_VALUE)
			{
				std::memcpy(buf, byArray_, sizeof(double));
			}
			else if (valType_ == ValueType::SOLE_VALUE)
			{
				utils::DoubleToBytes(soleValue_, buf, bKey_);
			}

			return sizeof(double);
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
				std::memcpy(buf, byArray_, sizeof(double));
				return sizeof(double) + 1;
			}
			else
			{
				*buf = 1;
				buf++;
				utils::DoubleToBytes(soleValue_, buf, bKey_);
				return sizeof(double) + 1;
			}
		}
	}

	uint32_t DataValueDouble::ReadData(Byte* buf, uint32_t len)
	{
		if (bKey_)
		{
			if (len == -1)
			{
				valType_ = ValueType::NULL_VALUE;
				return sizeof(double);
			}

			valType_ = ValueType::BYTES_VALUE;
			byArray_ = buf;
			return sizeof(double);
		}
		else
		{
			valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
			buf++;

			if (valType_ == ValueType::NULL_VALUE)
				return 1;

			byArray_ = buf;
			return sizeof(double) + 1;
		}
	}

	uint32_t DataValueDouble::GetDataLength() const
	{
		return bKey_ ? sizeof(double) : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(double));
	}

	uint32_t DataValueDouble::GetMaxLength() const
	{
		return sizeof(double);
	}
	
	uint32_t DataValueDouble::GetPersistenceLength() const
	{
		return bKey_ ? sizeof(double) : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(double));
	}

	void DataValueDouble::SetMinValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = -DBL_MAX;
	}
	void DataValueDouble::SetMaxValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = DBL_MAX;
	}
	void DataValueDouble::SetDefaultValue()
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = 0;
	}

	DataValueDouble::operator double() const
	{
		switch (valType_) 
		{
		case ValueType::NULL_VALUE:
			return 0;
		case ValueType::SOLE_VALUE:
			return soleValue_;
		case ValueType::BYTES_VALUE:
			return utils::DoubleFromBytes(byArray_, bKey_);
		}

		return 0;
	}

	DataValueDouble& DataValueDouble::operator=(double val)
	{
		valType_ = ValueType::SOLE_VALUE;
		soleValue_ = val;
		return *this;
	}

	DataValueDouble& DataValueDouble::operator=(const DataValueDouble& src)
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

	bool DataValueDouble::operator > (const DataValueDouble& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return false; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		double v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::DoubleFromBytes(byArray_, bKey_));
		double v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::DoubleFromBytes(dv.byArray_, dv.bKey_));
		return v1 > v2;
	}

	bool DataValueDouble::operator < (const DataValueDouble& dv) const
	{
		return !(*this >= dv);
	}

	bool DataValueDouble::operator >= (const DataValueDouble& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

		double v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::DoubleFromBytes(byArray_, bKey_));
		double v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::DoubleFromBytes(dv.byArray_, dv.bKey_));
		return v1 >= v2;
	}

	bool DataValueDouble::operator <= (const DataValueDouble& dv) const
	{
		return !(*this > dv);
	}

	bool DataValueDouble::operator == (const DataValueDouble& dv) const
	{
		if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
		if (dv.valType_ == ValueType::NULL_VALUE) { return false; }

		double v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : utils::DoubleFromBytes(byArray_, bKey_));
		double v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : utils::DoubleFromBytes(dv.byArray_, dv.bKey_));
		return v1 == v2;
	}

	bool DataValueDouble::operator != (const DataValueDouble& dv) const
	{
		return !(*this == dv);
	}

	std::ostream& operator<< (std::ostream& os, const DataValueDouble& dv)
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
			os << utils::DoubleFromBytes(dv.byArray_, dv.bKey_);
			break;
		}

		return os;
	}
}