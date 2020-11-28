#pragma once
#include <any>
#include "DataType.h"
#include "../table/Column.h"

namespace storage {
	class IDataValue {
	public:
		static bool IsIndexType(const DataType dt) { return ((int)dt & (int)DataType::INDEX_TYPE) == (int)DataType::INDEX_TYPE; }

		static bool IsFixLength(const DataType dt) { return ((int)dt & (int)DataType::FIX_LEN) == (int)DataType::FIX_LEN; }

		static bool IsCompressionable(const DataType dt) { return dt == DataType::VARCHAR || dt == DataType::BLOB; }

		static bool IsAutoPrimaryKey(const DataType dt) { return ((int)dt & (int)DataType::AUTO_INC_TYPE) == (int)DataType::AUTO_INC_TYPE; }

		static bool IsAggregate(const DataType dt) { return ((int)dt & (int)DataType::AGGR_TYPE) == (int)DataType::AGGR_TYPE; }

		static bool IsArrayType(const DataType dt) { return ((int)dt & (int)DataType::ARRAY_TYPE) == (int)DataType::ARRAY_TYPE; }

		static bool IsBigEndian() {
			short tmp = 0x0102;
			return *((char*)&tmp) == 0x01l;
		}

	public:
		IDataValue(const IDataValue& dv)
			: dataType_(dv.dataType_), valType_(dv.valType_), pColumn_(dv.pColumn_), bKey_(dv.bKey_){}
		IDataValue(DataType dataType, ValueType valType, ColumnBase* pColumn, const bool bKey)
			: dataType_(dataType), valType_(valType), bKey_(bKey), pColumn_(pColumn) {}
		virtual ~IDataValue() {	}
		/**
		* return the data type for this data value
		*/
		DataType GetDataType() const { return dataType_; }
		ValueType GetValueType() const { return valType_; }
		bool IsFixLength() const { return IsFixLength(dataType_); }
		bool IsNull() { return valType_ == ValueType::NULL_VALUE; }
		bool isKey() { return bKey_; }

		virtual std::any GetValue() const = 0;
		virtual uint32_t WriteData(char* buf) = 0;
		virtual uint32_t ReadData(char* buf, uint32_t len) = 0;
		virtual uint32_t GetLength(const bool bKey) const = 0;
		virtual uint32_t GetMaxLength() const = 0;
		virtual void SetMinValue() = 0;
		virtual void SetMaxValue() = 0;
		virtual void SetDefaultValue() = 0;


	protected:
		DataType dataType_;
		ValueType valType_;
		ColumnBase* pColumn_;
		bool bKey_;
	};
}