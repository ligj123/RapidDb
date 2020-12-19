#pragma once
#include <ostream>
#include "../header.h"

namespace storage {
	enum class DataType : uint32_t
	{
		INDEX_TYPE = 0x10000,
		FIX_LEN = 0x20000,
		ARRAY_TYPE = 0x40000,
		AGGR_TYPE = 0x80000,
		AUTO_INC_TYPE = 0x100000,

		UNKNOWN = 0,

		CHAR = AUTO_INC_TYPE + AGGR_TYPE + FIX_LEN + 1,
		SHORT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 2,
		INT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 3,
		LONG = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 4,

		BYTE = AUTO_INC_TYPE + AGGR_TYPE + FIX_LEN + 5,
		USHORT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 6,
		UINT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 7,
		ULONG = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 8,

		FIXCHAR = INDEX_TYPE + FIX_LEN + ARRAY_TYPE + 9,
		VARCHAR = INDEX_TYPE + ARRAY_TYPE + 10,
		DATETIME = INDEX_TYPE + FIX_LEN + 11,
		FLOAT = AGGR_TYPE + INDEX_TYPE + FIX_LEN + 12,
		DOUBLE = AGGR_TYPE + INDEX_TYPE + FIX_LEN + 13,
		BLOB = ARRAY_TYPE + 14,
		BOOL = FIX_LEN + 15
	};

	enum class ValueType : uint8_t {
		NULL_VALUE = 0,
		SOLE_VALUE,
		BYTES_VALUE
	};

	static const int DEFAULT_MAX_LEN = 1000;
	
	inline std::ostream& operator<< (std::ostream& os, const DataType& dt)
	{
		switch (dt)
		{
		case DataType::BYTE:
			os << "BYTE(" << (int)DataType::BYTE << ")";
			break;
		case DataType::SHORT:
			os << "SHORT(" << (int)DataType::SHORT << ")";
			break;
		case DataType::INT:
			os << "INT(" << (int)DataType::INT << ")";
			break;
		case DataType::LONG:
			os << "LONG(" << (int)DataType::LONG << ")";
			break;

		case DataType::CHAR:
			os << "CHAR(" << (int)DataType::CHAR << ")";
			break;
		case DataType::USHORT:
			os << "USHORT(" << (int)DataType::USHORT << ")";
			break;
		case DataType::UINT:
			os << "UINT(" << (int)DataType::UINT << ")";
			break;
		case DataType::ULONG:
			os << "ULONG(" << (int)DataType::ULONG << ")";
			break;

		case DataType::FIXCHAR:
			os << "FIXCHAR(" << (int)DataType::FIXCHAR << ")";
			break;
		case DataType::VARCHAR:
			os << "VARCHAR(" << (int)DataType::VARCHAR << ")";
			break;
		case DataType::DATETIME:
			os << "DATETIME(" << (int)DataType::DATETIME << ")";
			break;
		case DataType::FLOAT:
			os << "FLOAT(" << (int)DataType::FLOAT << ")";
			break;
		case DataType::DOUBLE:
			os << "DOUBLE(" << (int)DataType::DOUBLE << ")";
			break;
		case DataType::BLOB:
			os << "BLOB(" << (int)DataType::BLOB << ")";
			break;
		case DataType::BOOL:
			os << "BOOL(" << (int)DataType::BOOL << ")";
			break;
		}
		return os;
	}
	
	inline std::ostream& operator<< (std::ostream& os, const ValueType& vt)
	{
		switch (vt)
		{
		case ValueType::BYTES_VALUE:
			os << "BYTES_VALUE(" << (int)ValueType::BYTES_VALUE << ")";
			break;
		case ValueType::NULL_VALUE:
			os << "NULL_VALUE(" << (int)ValueType::NULL_VALUE << ")";
			break;
		case ValueType::SOLE_VALUE:
			os << "SOLE_VALUE(" << (int)ValueType::SOLE_VALUE << ")";
			break;
		}

		return os;
	}
}