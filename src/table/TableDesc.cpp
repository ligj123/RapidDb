#include "TableDesc.h"
#include <regex>
#include "../utils/ErrorMsg.h"
#include "../dataType/DataValueLong.h"
#include "../dataType/DataValueInt.h"
#include "../dataType/DataValueShort.h"
#include "../dataType/DataValueByte.h"
#include "../dataType/DataValueChar.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueFloat.h"
#include "../dataType/DataValueUInt.h"
#include "../dataType/DataValueULong.h"
#include "../dataType/DataValueUShort.h"
#include "../dataType/DataValueVarChar.h"

namespace storage {
  using namespace utils;

  const char* TableDesc::COLUMN_CONNECTOR_CHAR = "|";
  const char* TableDesc::NAME_PATTERN = "^[_a-zA-Z\\u4E00-\\u9FA5][_a-zA-Z0-9\\u4E00-\\u9FA5]*?$";
	const char* TableDesc::PRIMARY_KEY = "PARMARYKEY";
	
	void TableDesc::IsValidName(string name)
	{
		static regex reg(NAME_PATTERN);
		cmatch mt;
		if (!regex_match(name.c_str(), mt, reg)) {
			throw ErrorMsg(1001, { name });
		}
	}

  TableDesc::TableDesc(string name, string desc) {
    IsValidName(name);

    _name = name;
    _desc = desc;
  }

	TableDesc::TableDesc() {}

	TableDesc::~TableDesc() {
		for (auto iter = _vctColumn.begin(); iter != _vctColumn.end(); iter++) {
			delete* iter;
		}
	}
 

  IndexType TableDesc::GetIndexType(string indexName) const
  {
    auto iter = _mapIndexType.find(indexName);
    if (iter == _mapIndexType.end())
      return IndexType::UNKNOWN;
    else
      return iter->second;
  }

  const TableColumn* TableDesc::GetColumn(string fieldName) const {
    auto iter = _mapColumnPos.find(fieldName);
    if (iter == _mapColumnPos.end()) {
      return nullptr;
    }
    else
    {
      return _vctColumn[iter->second];
    }
  }

  const TableColumn* TableDesc::GetColumn(int pos) {
    if (pos < 0 || pos > _vctColumn.size()) {
      return nullptr;
    }
    else {
      return _vctColumn[pos];
    }
  }

	TableDesc& TableDesc::AddFixLenColumn(string columnName, DataType dataType,
			bool nullable, string comment, any valDefault)
	{
		IsValidName(columnName);

		if (_mapColumnPos.find(columnName) != _mapColumnPos.end())
		{
			throw ErrorMsg(1003, { columnName });
		}

		if (!IDataValue::IsFixLength(dataType)) {
			throw ErrorMsg(1004, { columnName });
		}

		//IDataValue* dvDefault = nullptr;
		//if (valDefault.has_value())
		//{
		//	bool b = true;
		//	switch (dataType) {
		//	case DataType::LONG:
		//		dvDefault = new DataValueLong(valDefault
		//		if (!(valDefault instanceof Long) && !(valDefault instanceof Integer)) b = false;
		//		break;
		//	case BOOLEAN:
		//		if (!(valDefault instanceof Boolean)) b = false;
		//		break;
		//	case DATE:
		//		if (!(valDefault instanceof Date) && !(valDefault instanceof Long)) b = false;
		//		break;
		//	case DOUBLE:
		//		if (!(valDefault instanceof Double) && !(valDefault instanceof Float) && !(valDefault instanceof Integer)) b = false;
		//		break;
		//	case FLOAT:
		//		if (!(valDefault instanceof Float) && !(valDefault instanceof Double) && !(valDefault instanceof Integer)) b = false;
		//		break;
		//	case INT:
		//		if (!(valDefault instanceof Integer)) b = false;
		//		break;
		//	case SHORT:
		//		if (!(valDefault instanceof Short) && !(valDefault instanceof Integer)) b = false;
		//		break;
		//	default:
		//		b = false;
		//		break;
		//	}

		//	if (!b) {
		//		throw new InvalidKeyColumnStorageException(
		//			"Invalid default value data type, column name=" + columnName);
		//	}
		//}

		//mapColumnPos.put(columnName, lstColumn.size());
		//lstColumn.add(new Column(columnName, lstColumn.size(), nullable, dataType, -1, CompressionType.COMPRESSION_NONE,
		//	comment, 0, StandardCharsets.UTF_8, verifyDefaultValue(dataType, valDefault)));
		return *this;
	}

	//public TableDesc addBlobColumn(String columnName, int maxLength, boolean nullable,
	//	String comment, byte[] valDefault) throws InvalidKeyColumnStorageException {
	//	if (!Pattern.matches(NAME_PATTERN, columnName)) {
	//		throw new InvalidKeyColumnStorageException("Invalid column name, name=" + columnName);
	//	}

	//	if (mapColumnPos.containsKey(columnName)) {
	//		throw new InvalidKeyColumnStorageException("Repeated column name, name=" + columnName);
	//	}

	//	if (maxLength < 1 || maxLength > Configuration.getMaxColumnLength()) {
	//		throw new InvalidKeyColumnStorageException(
	//			"Invalid max length, it should >= 1 and <= MAX_COLUMN_LENGTH, name=" + columnName);
	//	}

	//	mapColumnPos.put(columnName, lstColumn.size());
	//	lstColumn.add(new Column(columnName, lstColumn.size(), nullable, DataType.BLOB, maxLength,
	//		CompressionType.COMPRESSION_NONE, comment, 0, StandardCharsets.UTF_8,
	//		verifyDefaultValue(DataType.BLOB, valDefault)));
	//	return this;
	//}

	//public TableDesc addStringColumn(String columnName, int maxLength, boolean nullable, Charset charset,
	//	String comment, String valDefault) throws InvalidKeyColumnStorageException {
	//	if (!Pattern.matches(NAME_PATTERN, columnName)) {
	//		throw new InvalidKeyColumnStorageException("Invalid column name, name=" + columnName);
	//	}

	//	if (mapColumnPos.containsKey(columnName)) {
	//		throw new InvalidKeyColumnStorageException("Repeated column name, name=" + columnName);
	//	}

	//	if (maxLength < 1 || maxLength > Configuration.getMaxColumnLength()) {
	//		throw new InvalidKeyColumnStorageException(
	//			"Invalid max length, it should >= 1 and <= MAX_COLUMN_LENGTH, name=" + columnName);
	//	}

	//	mapColumnPos.put(columnName, lstColumn.size());
	//	lstColumn.add(new Column(columnName, lstColumn.size(), nullable, DataType.CHAR, maxLength,
	//		CompressionType.COMPRESSION_NONE, comment, 0, charset, verifyDefaultValue(DataType.CHAR, valDefault)));
	//	return this;
	//}

	//public TableDesc addAutoPrimaryColumn(String columnName, DataType dataType, int step, String comment)
	//	throws InvalidKeyColumnStorageException {
	//	if (!Pattern.matches(NAME_PATTERN, columnName)) {
	//		throw new InvalidKeyColumnStorageException("Invalid column name, name=" + columnName);
	//	}

	//	if (mapColumnPos.size() > 0) {
	//		throw new InvalidKeyColumnStorageException("Primary key must be the first column, name=" + columnName);
	//	}

	//	if (!dataType.isAutoPrimaryKey()) {
	//		throw new InvalidKeyColumnStorageException(
	//			"Invalid data type, only integer data type use this method add it, name=" + columnName);
	//	}

	//	if (step < 1) {
	//		throw new InvalidKeyColumnStorageException("Invalid step, it should >= 1, name=" + columnName);
	//	}

	//	mapColumnPos.put(columnName, lstColumn.size());
	//	lstColumn.add(new Column(columnName, lstColumn.size(), false, dataType, -1, CompressionType.COMPRESSION_NONE,
	//		comment, step, StandardCharsets.UTF_8, null));
	//	return this;
	//}

	//public TableDesc addCompressColumn(String columnName, DataType dataType, int maxLength, CompressionType cType,
	//	boolean nullable, String comment) throws InvalidKeyColumnStorageException {
	//	if (!Pattern.matches(NAME_PATTERN, columnName)) {
	//		throw new InvalidKeyColumnStorageException("Invalid column name, name=" + columnName);
	//	}

	//	if (mapColumnPos.containsKey(columnName)) {
	//		throw new InvalidKeyColumnStorageException("Repeated column name, name=" + columnName);
	//	}

	//	if (!dataType.isCompressionable()) {
	//		throw new InvalidKeyColumnStorageException(
	//			"Invalid data type, only COMPRESS data type use this method add it, name=" + columnName);
	//	}

	//	if (maxLength < 1 || maxLength > Configuration.getMaxColumnLength()) {
	//		throw new InvalidKeyColumnStorageException(
	//			"Invalid max length, it should >= 1 and <= MAX_COLUMN_LENGTH, name=" + columnName);
	//	}

	//	mapColumnPos.put(columnName, lstColumn.size());
	//	lstColumn.add(new Column(columnName, lstColumn.size(), nullable, dataType, maxLength,
	//		cType, comment, 0, StandardCharsets.UTF_8, null));
	//	return this;
	//}

	//public TableDesc setPrimaryKey(String... priFields) throws InvalidKeyColumnStorageException {
	//	if (priFields == null) {
	//		throw new InvalidKeyColumnStorageException("Primary key can not be null");
	//	}

	//	String keys = "";
	//	List<Column> lst = new ArrayList<>();
	//	for (String str : priFields) {
	//		Integer pos = mapColumnPos.get(str);
	//		if (pos == null) {
	//			throw new InvalidKeyColumnStorageException("Failed to find the column, name=" + str);
	//		}

	//		Column column = lstColumn.get(pos);
	//		if (!column.getDataType().isIndexType()) {
	//			throw new InvalidKeyColumnStorageException(
	//				"Unsupport column data type for index, name=" + str + "\tDataType=" + column.getDataType());
	//		}

	//		if (keys.length() > 0) {
	//			keys += COLUMN_SPLITE_CHAR;
	//		}

	//		keys += str;
	//		lst.add(column);
	//	}

	//	if (lst.get(0).isIndex()) {
	//		throw new InvalidKeyColumnStorageException("Try to add repeated index, name=" + keys);
	//	}

	//	primaryKey = keys;
	//	if (mapIndexType.put(keys, IndexType.PRIMARY) != null) {
	//		throw new InvalidKeyColumnStorageException(
	//			"Try to insert repeated index as primart index, columns' names=" + keys);
	//	}

	//	mapIndexColumn.put(keys, lst);
	//	lst.get(0).setIndex(true);
	//	mapIndexFirstField.put(priFields[0], keys);

	//	return this;
	//}

	//public TableDesc addSecondaryKey(String name, IndexType indexType, String... fieldNames)
	//	throws InvalidKeyColumnStorageException {
	//	if (indexType != IndexType.UNIQUE && indexType != IndexType.NON_UNIQUE) {
	//		throw new InvalidKeyColumnStorageException("Only can add UNIQUE or NON_UNIQUE for secondary index");
	//	}

	//	if (fieldNames == null) {
	//		throw new InvalidKeyColumnStorageException("Secondary key should have at least 1 field.");
	//	}

	//	String keys = "";
	//	List<Column> lst = new ArrayList<>();

	//	for (String str : fieldNames) {
	//		Integer pos = mapColumnPos.get(str);
	//		if (pos == null) {
	//			throw new InvalidKeyColumnStorageException("Failed to find the column, name=" + str);
	//		}

	//		Column column = lstColumn.get(pos);
	//		if (!column.getDataType().isIndexType()) {
	//			throw new InvalidKeyColumnStorageException(
	//				"Unsupport column data type for index, name=" + str + "\tDataType=" + column.getDataType());
	//		}

	//		if (keys.length() > 0) {
	//			keys += COLUMN_SPLITE_CHAR;
	//		}

	//		keys += str;
	//		lst.add(column);
	//	}

	//	if (lst.get(0).isIndex()) {
	//		throw new InvalidKeyColumnStorageException("Try to add repeated index, name=" + keys);
	//	}

	//	if (name != null && name.length() > 0 && mapIndexName.put(name, keys) != null) {
	//		throw new InvalidKeyColumnStorageException("Repeated secondary index's name , name=" + name);
	//	}

	//	if (mapIndexType.put(keys, indexType) != null) {
	//		throw new InvalidKeyColumnStorageException("Repeated secondary index , columns' names=" + keys);
	//	}

	//	mapIndexColumn.put(keys, lst);
	//	lst.get(0).setIndex(true);
	//	mapIndexFirstField.put(fieldNames[0], keys);

	//	return this;
	//}

}