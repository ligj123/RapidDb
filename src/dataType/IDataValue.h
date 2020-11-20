#pragma once
#include "DataType.h"
#include <any>

namespace storage {
	class IDataValue {
	public:
		static bool IsIndexType(const DataType dt) { return ((int)dt & (int)DataType::INDEX_TYPE) == (int)DataType::INDEX_TYPE; }

		static bool IsFixLength(const DataType dt) { return ((int)dt & (int)DataType::FIX_LEN) == (int)DataType::FIX_LEN; }

		static bool IsCompressionable(const DataType dt) { return dt == DataType::VARCHAR || dt == DataType::BLOB; }

		static bool IsAutoPrimaryKey(const DataType dt) { return ((int)dt & (int)DataType::AUTO_INC_TYPE) == (int)DataType::AUTO_INC_TYPE; }

		static bool IsAggregate(const DataType dt) { return ((int)dt & (int)DataType::AGGR_TYPE) == (int)DataType::AGGR_TYPE; }

		static bool IsArrayType(const DataType dt) { return ((int)dt & (int)DataType::ARRAY_TYPE) == (int)DataType::ARRAY_TYPE; }

	public:
		IDataValue(const IDataValue& dv) : dataType_(dv.dataType_), valType_(dv.valType_) {}
		IDataValue(const DataType dataType, const ValueType valType) : dataType_(dataType), valType_(valType) {}
		/**
		* return the data type for this data value
		*/
		DataType GetDataType() const { return dataType_; }
		ValueType GetValueType() const { return valType_; }
		virtual bool IsFixLength() const { return IsFixLength(dataType_); }

		virtual std::any GetValue() const = 0;
		virtual int WriteData(char* buf, bool bkey) = 0;
		virtual int ReadData(char* buf, bool bkey, int len) = 0;
		virtual int GetLength(const bool bKey) const = 0;
		virtual int GetMaxLength() const = 0;
		virtual void SetMinValue() = 0;
		virtual void SetMaxValue() = 0;
		virtual void SetDefaultValue() = 0;
		virtual bool IsNull() const = 0;


	protected:
		DataType dataType_;
		ValueType valType_;
	};
}