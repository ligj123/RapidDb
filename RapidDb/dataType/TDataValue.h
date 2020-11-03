#pragma once
#include <stdexcept>
#include "IDataValue.h"

template<class T>
class TDataValue : public IDataValue
{
public:
	TDataValue(const DataType dt, const int maxLength);
	TDataValue(const DataType dt, const T& soleVal, const int maxLength);
	TDataValue(const DataType dt, char* byArray, const int spos, const int len, const int maxLength);

public:
	virtual std::any getValue() const;
	virtual int WriteData(char* buf, int pos, bool bkey);
	virtual int ReadData(char* buf, int pos, bool bkey, int len);
	virtual int GetLength(bool bKey) const;
	virtual int GetMaxLength() const;
	virtual bool IsNull() const;
	virtual void SetMinValue() {}
	virtual void SetMaxValue() {}
	virtual void SetDefaultValue() {}

	virtual bool operator > (const IDataValue& dv) const;
	virtual bool operator < (const IDataValue& dv) const;
	virtual bool operator >= (const IDataValue& dv) const;
	virtual bool operator <= (const IDataValue& dv) const;
	virtual bool operator == (const IDataValue& dv) const;
	virtual bool operator != (const IDataValue& dv) const;
protected:
	int maxLength_;
	union {
		T soleValue_;
		struct {
			char* byArray_;
			uint32_t spos_;
			uint32_t len_;
		};
	};
};

template<class T> TDataValue<T>::TDataValue(const DataType dt, const int maxLength)
	: IDataValue(dt, ValueType::NULL_VALUE), maxLength_(maxLength)
{
}

template<class T> TDataValue<T>::TDataValue(const DataType dt, const T& soleVal, const int maxLength)
	: IDataValue(dt, ValueType::SOLE_VALUE), soleValue_(soleVal), maxLength_(maxLength)
{
}
template<class T> TDataValue<T>::TDataValue(const DataType dt, char* byArray, const int spos, const int len, const int maxLength)
	: IDataValue(dt, ValueType::BYTES_VALUE), byArray_(byArray), spos_(spos),
	len_(len), maxLength_(maxLength)
{
}

template<class T> bool TDataValue<T>::operator > (const IDataValue& idv) const
{
	const TDataValue<T>& dv =  reinterpret_cast<const TDataValue<T>&>(idv);
	if (dataType_ != dv.dataType_)
	{
		throw std::logic_error("The data value is not consistent with the right.");
	}

	if (valType_ == ValueType::NULL_VALUE) { return false; }
	if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

	if (!IsArrayType(dataType_))
	{
		const T& v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : *(reinterpret_cast<const T*>(byArray_ + spos_)));
		const T& v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : *(reinterpret_cast<const T*>(dv.byArray_  + dv.spos_)));
		return v1 > v2;
	}

	if (valType_ == ValueType::BYTES_VALUE && dv.valType_ == ValueType::BYTES_VALUE)
	{
		int len = std::min(len_, dv.len_);
		int hr = std::memcmp(byArray_, dv.byArray_, len);
		if (hr != 0) return hr;
		else return len_ > dv.len_;
	}

	// For other data type, for example, it will only to know how to compare in its child class;
	throw std::logic_error("Unsupport data type, should implement in subclass.");
}

template<class T> bool TDataValue<T>::operator < (const IDataValue& dv) const
{
	return (*this) >= dv;
}

template<class T> bool TDataValue<T>::operator >= (const IDataValue& idv) const
{
	const TDataValue<T>& dv = reinterpret_cast<const TDataValue<T>&>(idv);
	if (dataType_ != dv.dataType_)
	{
		throw std::logic_error("The data value is not consistent with the right.");
	}

	if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
	if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

	if (!IsArrayType(dataType_))
	{
		const T& v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : *(reinterpret_cast<const T*>(byArray_ + spos_)));
		const T& v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : *(reinterpret_cast<const T*>(dv.byArray_ + dv.spos_)));
		return v1 >= v2;
	}

	if (valType_ == ValueType::BYTES_VALUE && dv.valType_ == ValueType::BYTES_VALUE)
	{
		int len = std::min(len_, dv.len_);
		int hr = std::memcmp(byArray_, dv.byArray_, len);
		if (hr != 0) return hr;
		else return len_ >= dv.len_;
	}

	// For other data type, for example, it will only to know how to compare in its child class;
	throw std::logic_error("Unsupport data type, should implement in subclass.");
}

template<class T> bool TDataValue<T>::operator <= (const IDataValue& dv) const
{
	return (*this) > dv;
}

template<class T> bool TDataValue<T>::operator == (const IDataValue& idv) const
{
	const TDataValue<T>& dv = reinterpret_cast<const TDataValue<T>&>(idv);
	if (dataType_ != dv.dataType_)
	{
		throw std::logic_error("The data value is not consistent with the right.");
	}

	if (valType_ != dv.valType_) { return false; }
	if (valType_ == ValueType::NULL_VALUE) { return true; }

	if (!IsArrayType(dataType_))
	{
		const T& v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : *(reinterpret_cast<const T*>(byArray_ + spos_)));
		const T& v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : *(reinterpret_cast<const T*>(dv.byArray_ + dv.spos_)));
		return v1 == v2;
	}

	if (valType_ == ValueType::BYTES_VALUE && (dv.valType_ == ValueType::BYTES_VALUE))
	{
		int len = std::min(len_, dv.len_);
		int hr = std::memcmp(byArray_, dv.byArray_, len);
		if (hr != 0) return false;
		else return len_ == dv.len_;
	}

	// For other data type, for example, it will only to know how to compare in its child class;
	throw std::logic_error("Unsupport operator for this data type, should implement in subclass.");
}

template<class T> bool TDataValue<T>::operator != (const IDataValue& dv) const
{
	return (*this) == dv;
}

template<class T> std::any TDataValue<T>::getValue() const
{
	if (valType_ == ValueType::NULL_VALUE) return nullptr;
	if (valType_ == ValueType::SOLE_VALUE) return soleValue_;
	if (!IsArrayType(dataType_)) return *(reinterpret_cast<T*>(byArray_ + spos_));

	throw std::logic_error("Unsupport operator for this data type, should implement in subclass.");
}

template<class T> int TDataValue<T>::WriteData(char* buf, int pos, bool bkey)
{
	if (bkey)
	{
		if (valType_ == ValueType::NULL_VALUE)
		{
			if (IsFixLength(dataType_)) { return maxLength_; }
			return 0;
		}

		if (valType_ == ValueType::BYTES_VALUE)
		{
			int len = (IsFixLength(dataType_) ? maxLength_ : len_);
			std::memcpy(buf + pos, byArray_ + spos_, len);
			return len;
		}

		if (!IsArrayType(dataType_))
		{
			std::memcpy(buf + pos, reinterpret_cast<char*>(&soleValue_), sizeof(maxLength_));
			return maxLength_;
		}
	}
	else
	{
		if (valType_ == ValueType::NULL_VALUE)
		{
			buf[pos] = 0;
			return 1;
		}

		if (valType_ == ValueType::BYTES_VALUE)
		{
			buf[pos++] = 1;
			int len = (IsFixLength(dataType_) ? maxLength_ : len_ + sizeof(uint32_t));
			std::memcpy(buf + pos, byArray_ + spos_, len);
			return len + 1;
		}

		if (!IsArrayType(dataType_))
		{
			buf[pos++] = 1;
			std::memcpy(buf + pos, reinterpret_cast<char*>(&soleValue_), maxLength_);
			return maxLength_ + 1;
		}
	}

	throw std::logic_error("Unsupport operator for this data type, should implement in subclass.");
}

template<class T> int TDataValue<T>::ReadData(char* buf, int pos, bool bkey, int len)
{
	if (len > maxLength_) {
		throw std::out_of_range("Try to read too much bytes than max length of data value.");
	}

	if (bkey)
	{
		if (len == -1)
		{
			valType_ = ValueType::NULL_VALUE;
			if (IsFixLength(dataType_)) { return sizeof(T); }
			return 0;
		}

		valType_ = ValueType::BYTES_VALUE;
		len_ = IsFixLength(dataType_) ? maxLength_ : len;
		byArray_ = buf;
		spos_ = pos;
		return len_;
	}
	else
	{
		valType_ = (buf[pos++] ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
		if (valType_ == ValueType::BYTES_VALUE)
		{
			if (IsFixLength(dataType_))
			{
				len_ = maxLength_;
				byArray_ = buf + pos;
				return len_;
			}
			else
			{
				len_ = *(reinterpret_cast <uint32_t*>(buf + pos));
				byArray_ = buf + pos;
				return len_ + sizeof(uint32_t);
			}
		}
	}
	throw std::logic_error("Unsupport operator for this data type, should implement in subclass.");
}

template<class T> int TDataValue<T>::GetLength(bool bKey) const
{
	if (bKey)
	{
		if (IsFixLength(dataType_)) return maxLength_;
		if (valType_ == ValueType::NULL_VALUE) return 0;
		if (valType_ == ValueType::BYTES_VALUE) return len_;
	}
	else
	{
		if (valType_ == ValueType::NULL_VALUE) return 1;
		if (IsFixLength(dataType_)) return maxLength_ + 1;
		if (valType_ == ValueType::BYTES_VALUE) return len_ + 1;
	}

	throw std::logic_error("Unsupport operator for this data type, should implement in subclass.");
}

template<class T> int TDataValue<T>::GetMaxLength() const
{
	return maxLength_;
}

template<class T> bool TDataValue<T>::IsNull() const
{
	return valType_ == ValueType::NULL_VALUE;
}
