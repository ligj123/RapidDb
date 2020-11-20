#include "DataType.h"

namespace storage {
	//std::ostream& operator<< (std::ostream& os, const DataType& dt)
	//{
	//	switch (dt)
	//	{
	//	case DataType::BYTE:
	//		os << "BYTE(" << (int)DataType::BYTE << ")";
	//		break;
	//	case DataType::SHORT:
	//		os << "SHORT(" << (int)DataType::SHORT << ")";
	//		break;
	//	case DataType::INT:
	//		os << "INT(" << (int)DataType::INT << ")";
	//		break;
	//	case DataType::LONG:
	//		os << "LONG(" << (int)DataType::LONG << ")";
	//		break;

	//	case DataType::UBYTE:
	//		os << "UBYTE(" << (int)DataType::UBYTE << ")";
	//		break;
	//	case DataType::USHORT:
	//		os << "USHORT(" << (int)DataType::USHORT << ")";
	//		break;
	//	case DataType::UINT:
	//		os << "UINT(" << (int)DataType::UINT << ")";
	//		break;
	//	case DataType::ULONG:
	//		os << "ULONG(" << (int)DataType::ULONG << ")";
	//		break;

	//	case DataType::CHAR:
	//		os << "CHAR(" << (int)DataType::CHAR << ")";
	//		break;
	//	case DataType::VARCHAR:
	//		os << "VARCHAR(" << (int)DataType::VARCHAR << ")";
	//		break;
	//	case DataType::DATETIME:
	//		os << "DATETIME(" << (int)DataType::DATETIME << ")";
	//		break;
	//	case DataType::FLOAT:
	//		os << "FLOAT(" << (int)DataType::FLOAT << ")";
	//		break;
	//	case DataType::DOUBLE:
	//		os << "DOUBLE(" << (int)DataType::DOUBLE << ")";
	//		break;
	//	case DataType::BLOB:
	//		os << "BLOB(" << (int)DataType::BLOB << ")";
	//		break;
	//	case DataType::BOOL:
	//		os << "BOOL(" << (int)DataType::BOOL << ")";
	//		break;
	//	case DataType::COMPRESS:
	//		os << "COMPRESS(" << (int)DataType::COMPRESS << ")";
	//		break;
	//	}
	//	return os;
	//}

	//std::ostream& operator<< (std::ostream& os, const ValueType& vt)
	//{
	//	switch (vt)
	//	{
	//	case ValueType::BYTES_VALUE:
	//		os << "BYTES_VALUE(" << (int)ValueType::BYTES_VALUE << ")";
	//		break;
	//	case ValueType::NULL_VALUE:
	//		os << "NULL_VALUE(" << (int)ValueType::NULL_VALUE << ")";
	//		break;
	//	case ValueType::SOLE_VALUE:
	//		os << "SOLE_VALUE(" << (int)ValueType::SOLE_VALUE << ")";
	//		break;
	//	}

	//	return os;
	//}
}