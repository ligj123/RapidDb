#include "TableDesc.h"
#include <regex>
#include "../utils/ErrorMsg.h"
#include "../dataType/DataValueFactory.h"
#include "../config/ErrorID.h"

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
    throw ErrorMsg(TB_INVALID_FILE_VERSION, { name });
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

const PersistColumn* TableDesc::GetColumn(string fieldName) const {
  auto iter = _mapColumnPos.find(fieldName);
  if (iter == _mapColumnPos.end()) {
    return nullptr;
  }     else
  {
    return _vctColumn[iter->second];
  }
}

const PersistColumn* TableDesc::GetColumn(int pos) {
  if (pos < 0 || pos > _vctColumn.size()) {
    return nullptr;
  }     else {
    return _vctColumn[pos];
  }
}

void TableDesc::AddColumn(string& columnName, DataType dataType, bool nullable, uint32_t maxLen,
  string& comment, utils::Charsets charset, any& valDefault)
{
  transform(columnName.begin(), columnName.end(), columnName.begin(), ::toupper);
  IsValidName(columnName);

  if (_mapColumnPos.find(columnName) != _mapColumnPos.end()) {
    throw ErrorMsg(TB_REPEATED_COLUMN_NAME, { columnName });
  }

  if (maxLen <= 0 && !IDataValue::IsArrayType(dataType)) {
    throw new ErrorMsg(TB_Array_MAX_LEN, { columnName });
  }

  IDataValue* dvDefault = nullptr;
  if (valDefault.has_value()) {
    dvDefault = DataValueFactory(dataType, false, -1, valDefault);
  }

  PersistColumn* cm = new PersistColumn(columnName, (uint32_t)_vctColumn.size(), dataType, comment, nullable,
    maxLen, -1, utils::Charsets::UTF8, dvDefault);

  _vctColumn.push_back(cm);
  _mapColumnPos.insert(pair<string, size_t>(columnName, cm->GetPosition()));
}

void TableDesc::SetPrimaryKey(vector<string> priCols)
{
  if (priCols.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, { PRIMARY_KEY });
  }
  if (_mapIndexName.find(PRIMARY_KEY) != _mapIndexName.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, { PRIMARY_KEY });
  }

  string names;
  vector<PersistColumn*> vct;
  for (string col : priCols) {
    auto iter = _mapColumnPos.find(col);
    if (iter == _mapColumnPos.end()) {
      throw ErrorMsg(TB_UNEXIST_COLUMN, { col });
    }

    PersistColumn* tc = _vctColumn[iter->second];
    vct.push_back(tc);
    if (!IDataValue::IsIndexType(tc->GetDataType())) {
      throw ErrorMsg(TB_INDEX_UNSUPPORT_DATA_TYPE, { col, DateTypeToString(tc->GetDataType()) });
    }
    if (names.size() > 0) names += COLUMN_CONNECTOR_CHAR;
    names += col;
  }

  _mapIndexName.insert(pair<string, string>(PRIMARY_KEY, names));
  _mapIndexType.insert(pair<string, IndexType>(PRIMARY_KEY, IndexType::PRIMARY));
  _mapIndexColumn.insert(pair<string, vector<PersistColumn*>>(PRIMARY_KEY, vct));
  _mapIndexFirstField.insert(pair<string, string>(priCols[0], PRIMARY_KEY));
}

void TableDesc::SetPrimaryKey(string priCol, uint32_t incStep)
{
  if (_mapIndexName.find(PRIMARY_KEY) != _mapIndexName.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, { PRIMARY_KEY });
  }

  string names;
  vector<PersistColumn*> vct;
  auto iter = _mapColumnPos.find(priCol);
  if (iter == _mapColumnPos.end()) {
    throw ErrorMsg(TB_UNEXIST_COLUMN, { priCol });
  }

  PersistColumn* tc = _vctColumn[iter->second];
  vct.push_back(tc);
  if (!IDataValue::IsIndexType(tc->GetDataType())) {
    throw ErrorMsg(TB_INDEX_UNSUPPORT_DATA_TYPE, { priCol, DateTypeToString(tc->GetDataType()) });
  }
  if (names.size() > 0) names += COLUMN_CONNECTOR_CHAR;
  names += priCol;

  _mapIndexName.insert(pair<string, string>(PRIMARY_KEY, names));
  _mapIndexType.insert(pair<string, IndexType>(PRIMARY_KEY, IndexType::PRIMARY));
  _mapIndexColumn.insert(pair<string, vector<PersistColumn*>>(PRIMARY_KEY, vct));
  _mapIndexFirstField.insert(pair<string, string>(priCol, PRIMARY_KEY));
}

void TableDesc::AddSecondaryKey(string indexName, IndexType indexType, vector<string> colNames)
{
  if (colNames.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, { indexName });
  }
  if (_mapIndexName.find(indexName) != _mapIndexName.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, { indexName });
  }

  string names;
  vector<PersistColumn*> vct;
  for (string col : colNames) {
    auto iter = _mapColumnPos.find(col);
    if (iter == _mapColumnPos.end()) {
      throw ErrorMsg(TB_UNEXIST_COLUMN, { col });
    }

    PersistColumn* tc = _vctColumn[iter->second];
    vct.push_back(tc);
    if (!IDataValue::IsIndexType(tc->GetDataType())) {
      throw ErrorMsg(TB_INDEX_UNSUPPORT_DATA_TYPE, { col, DateTypeToString(tc->GetDataType()) });
    }
    if (names.size() > 0) names += COLUMN_CONNECTOR_CHAR;
    names += col;
  }

  _mapIndexName.insert(pair<string, string>(indexName, names));
  _mapIndexType.insert(pair<string, IndexType>(indexName, IndexType::PRIMARY));
  _mapIndexColumn.insert(pair<string, vector<PersistColumn*>>(indexName, vct));
  _mapIndexFirstField.insert(pair<string, string>(colNames[0], indexName));
  _mapIndexType.insert(pair<string, IndexType>(indexName, indexType));
}
}
