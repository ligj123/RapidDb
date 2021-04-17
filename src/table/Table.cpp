#include "Table.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../utils/ErrorMsg.h"
#include <regex>

namespace storage {
using namespace utils;

PersistTable::~PersistTable() {
  for (auto iter = _vctColumn.begin(); iter != _vctColumn.end(); iter++) {
    delete *iter;
  }
}

const PersistColumn *PersistTable::GetColumn(string fieldName) const {
  auto iter = _mapColumnPos.find(fieldName);
  if (iter == _mapColumnPos.end()) {
    return nullptr;
  } else {
    return _vctColumn[iter->second];
  }
}

const PersistColumn *PersistTable::GetColumn(int pos) {
  if (pos < 0 || pos > _vctColumn.size()) {
    return nullptr;
  } else {
    return _vctColumn[pos];
  }
}

void PersistTable::AddColumn(string &columnName, DataType dataType,
                             bool nullable, uint32_t maxLen, string &comment,
                             utils::Charsets charset, any &valDefault) {
  transform(columnName.begin(), columnName.end(), columnName.begin(),
            ::toupper);
  IsValidName(columnName);

  if (_mapColumnPos.find(columnName) != _mapColumnPos.end()) {
    throw ErrorMsg(TB_REPEATED_COLUMN_NAME, {columnName});
  }

  if (maxLen <= 0 && !IDataValue::IsArrayType(dataType)) {
    throw new ErrorMsg(TB_Array_MAX_LEN, {columnName});
  }

  IDataValue *dvDefault = nullptr;
  if (valDefault.has_value()) {
    dvDefault = DataValueFactory(dataType, false, -1, valDefault);
  }

  PersistColumn *cm = new PersistColumn(columnName, (uint32_t)_vctColumn.size(),
                                        dataType, comment, nullable, maxLen, -1,
                                        utils::Charsets::UTF8, dvDefault);

  _vctColumn.push_back(cm);
  _mapColumnPos.insert(pair<string, size_t>(columnName, cm->GetPosition()));
}

void PersistTable::SetPrimaryKey(vector<string> priCols) {
  if (priCols.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, {PRIMARY_KEY});
  }

  if (_mapIndex.find(PRIMARY_KEY) != _mapIndex.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, {PRIMARY_KEY});
  }

  vector<IndexProp::Column> vct;
  for (string col : priCols) {
    auto iter = _mapColumnPos.find(col);
    if (iter == _mapColumnPos.end()) {
      throw ErrorMsg(TB_UNEXIST_COLUMN, {col});
    }

    IndexProp::Column clm;

    PersistColumn *tc = _vctColumn[iter->second];
    if (!IDataValue::IsIndexType(tc->GetDataType())) {
      throw ErrorMsg(TB_INDEX_UNSUPPORT_DATA_TYPE,
                     {col, DateTypeToString(tc->GetDataType())});
    }

    clm.colName = tc->GetName();
    clm.colPos = tc->GetPosition();

    vct.push_back(clm);
  }

  IndexProp prop{IndexType::PRIMARY, vct};
  _mapIndex.insert({PRIMARY_KEY, prop});
  _mapIndexFirstField.insert({vct[0].colPos, PRIMARY_KEY});
}

void PersistTable::AddSecondaryKey(string indexName, IndexType indexType,
                                   vector<string> colNames) {
  if (colNames.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, {indexName});
  }
  if (_mapIndex.find(indexName) != _mapIndex.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, {indexName});
  }

  vector<IndexProp::Column> vct;
  for (string col : colNames) {
    auto iter = _mapColumnPos.find(col);
    if (iter == _mapColumnPos.end()) {
      throw ErrorMsg(TB_UNEXIST_COLUMN, {col});
    }
    IndexProp::Column clm;

    PersistColumn *tc = _vctColumn[iter->second];
    if (!IDataValue::IsIndexType(tc->GetDataType())) {
      throw ErrorMsg(TB_INDEX_UNSUPPORT_DATA_TYPE,
                     {col, DateTypeToString(tc->GetDataType())});
    }

    clm.colName = tc->GetName();
    clm.colPos = tc->GetPosition();

    vct.push_back(clm);
  }

  IndexProp prop{indexType, vct};
  _mapIndex.insert({indexName, prop});
  _mapIndexFirstField.insert({vct[0].colPos, indexName});
}
} // namespace storage
