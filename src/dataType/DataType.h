#pragma once
#include <ostream>

namespace storage {
	enum class DataType
	{
		INDEX_TYPE = 0x10000,
		FIX_LEN = 0x20000,
		ARRAY_TYPE = 0x40000,
		AGGR_TYPE = 0x80000,
		AUTO_INC_TYPE = 0x100000,

		UNKNOWN = 0,

		BYTE = AUTO_INC_TYPE + AGGR_TYPE + FIX_LEN + 1,
		SHORT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 2,
		INT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 3,
		LONG = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 4,

		UBYTE = AUTO_INC_TYPE + AGGR_TYPE + FIX_LEN + 5,
		USHORT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 6,
		UINT = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 7,
		ULONG = AUTO_INC_TYPE + AGGR_TYPE + INDEX_TYPE + FIX_LEN + 8,

		CHAR = INDEX_TYPE + FIX_LEN + ARRAY_TYPE + 9,
		VARCHAR = INDEX_TYPE + ARRAY_TYPE + 10,
		DATETIME = INDEX_TYPE + FIX_LEN + 11,
		FLOAT = AGGR_TYPE + INDEX_TYPE + FIX_LEN + 12,
		DOUBLE = AGGR_TYPE + INDEX_TYPE + FIX_LEN + 13,
		BLOB = ARRAY_TYPE + 14,
		BOOL = FIX_LEN + 15,
		COMPRESS = ARRAY_TYPE + 100,
	};

	enum class ValueType {
		NULL_VALUE = 0,
		SOLE_VALUE,
		BYTES_VALUE
	};

	std::ostream& operator<< (std::ostream& os, const DataType& dt);

	std::ostream& operator<< (std::ostream& os, const ValueType& vt);
}