#include "PersistTable.h"
#include "../config/ErrorID.h"
#include "../config/FileVersion.h"
#include "../dataType/DataValueFactory.h"
#include "../utils/ErrorMsg.h"
#include <filesystem>
#include <fstream>
#include <regex>

namespace storage {
namespace fs = std::filesystem;

PersistTable::PersistTable(string &rootPath, string &dbName, string &tableName,
                           string &tableAlias, string &description)
    : BaseTable(tableName, tableAlias, description), _rootPath(rootPath),
      _dbName(dbName) {
  ReadData();
};

PersistTable::~PersistTable() {
  for (auto iter = _vctColumn.begin(); iter != _vctColumn.end(); iter++) {
    delete *iter;
  }
}

const PersistColumn *PersistTable::GetColumn(string &fieldName) const {
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
                             Charsets charset, any &valDefault) {
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
                                        -1, Charsets::UTF8, dvDefault);

  _vctColumn.push_back(cm);
  _mapColumnPos.insert(pair<string, int>(columnName, cm->GetPosition()));
}

void PersistTable::SetPrimaryKey(MVector<string>::Type &priCols) {
  if (priCols.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, {PRIMARY_KEY});
  }

  if (_mapIndex.find(PRIMARY_KEY) != _mapIndex.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, {PRIMARY_KEY});
  }

  MVector<IndexProp::Column>::Type vct;
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

void PersistTable::AddSecondaryKey(string &indexName, IndexType indexType,
                                   MVector<string>::Type &colNames) {
  if (colNames.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, {indexName});
  }
  if (_mapIndex.find(indexName) != _mapIndex.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, {indexName});
  }

  MVector<IndexProp::Column>::Type vct;
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

void PersistTable::WriteData() {
  string path = _rootPath + "/" + _dbName + "/" + _name + "/metafile.dat";
  ofstream fs(path, ios::out | ios::binary | ios::trunc);
  fs << CURRENT_FILE_VERSION.GetMajorVersion()
     << CURRENT_FILE_VERSION.GetMinorVersion()
     << CURRENT_FILE_VERSION.GetPatchVersion();

  fs << (int)_name.size();
  fs.write(_name.c_str(), _name.size());
  fs << (int)_desc.size();
  fs.write(_desc.c_str(), _desc.size());

  fs << (int)_vctColumn.size();
  Byte buf[10240];
  for (int i = 0; i < _vctColumn.size(); i++) {
    PersistColumn *col = _vctColumn[i];
    int len = col->WriteData(buf);
    fs << len;
    fs.write((char *)buf, len);
  }

  fs << (int)_mapIndex.size();
  for (auto iter = _mapIndex.begin(); iter != _mapIndex.end(); iter++) {
    fs << (int)iter->first.size();
    fs.write(iter->first.c_str(), iter->first.size());
    fs << (int)iter->second.type;

    MVector<IndexProp::Column>::Type &vctCol = iter->second.vctCol;
    fs << (int)vctCol.size();
    for (int i = 0; i < vctCol.size(); i++) {
      fs << vctCol[i].colPos;
    }
  }

  fs.close();
}

void PersistTable::ReadData() {
  string path = _rootPath + "/" + _dbName + "/" + _name + "/metafile.dat";
  ifstream fs(path, ios::in | ios::binary);
  char buf[10240];

  fs.read(buf, 4);
  FileVersion fv(*(int16_t *)buf, *(uint8_t *)(buf + 2), *(uint8_t *)(buf + 3));
  if (!(fv == CURRENT_FILE_VERSION)) {
    throw ErrorMsg(TB_ERROR_INDEX_VERSION, {path});
  }

  int len;
  fs >> len;
  fs.read(buf, len);
  buf[len] = 0;
  _name = buf;

  fs >> len;
  fs.read(buf, len);
  buf[len] = 0;
  _desc = buf;

  fs >> len;
  for (int i = 0; i < len; i++) {
    int len2;
    fs >> len2;
    fs.read(buf, len2);
    PersistColumn *col = new PersistColumn();
    col->ReadData((Byte *)buf);
    _vctColumn.push_back(col);
    _mapColumnPos.insert({col->GetName(), i});
  }

  fs >> len;
  for (int i = 0; i < len; i++) {
    int len2;
    fs >> len2;
    fs.read(buf, len2);
    buf[len2] = 0;
    string str = buf;
    IndexProp iprop;
    int type;
    fs >> type;
    iprop.type = (IndexType)type;

    fs >> len2;
    for (int j = 0; j < len2; j++) {
      IndexProp::Column clm;
      fs >> clm.colPos;
      clm.colName = _vctColumn[clm.colPos]->GetName();
      iprop.vctCol.push_back(clm);
    }

    _mapIndex.insert({str, iprop});
    _mapIndexFirstField.insert({iprop.vctCol[0].colPos, str});
  }
}

void PersistTable::OpenTable() {
  Clear();
  ReadData();
  string path = _rootPath + "/" + _dbName + "/" + _name + "/";
  for (auto iter = _mapIndex.begin(); iter != _mapIndex.end(); iter++) {
    path += iter->first;
    VectorDataValue &key = GenIndexDataValues(iter->second);
    IndexTree *tree;
    if (iter->first == PRIMARY_KEY) {
      VectorDataValue &val = GenColumsDataValues();
      tree = new IndexTree(_name, path, key, val);
      _primaryTree = tree;
    } else {
      VectorDataValue &val = GenIndexDataValues(_mapIndex[PRIMARY_KEY]);
      tree = new IndexTree(_name, path, key, val);
    }
    _mapTree.insert({iter->first, tree});
  }
}

void PersistTable::CloseTable() { Clear(); }

void PersistTable::Clear() {
  _primaryTree = nullptr;

  for (auto iter = _mapTree.begin(); iter != _mapTree.end(); iter++) {
    iter->second->Close(true);
  }

  _mapTree.clear();

  for (auto iter : _vctColumn) {
    delete iter;
  }
  _vctColumn.clear();
}

VectorDataValue &PersistTable::GenIndexDataValues(IndexProp prop) {
  VectorDataValue vct;
  vct.reserve(prop.vctCol.size());
  for (auto iter : prop.vctCol) {
    PersistColumn *col = _vctColumn[iter.colPos];
    IDataValue *dv =
        DataValueFactory(col->GetDataType(), true, col->GetMaxLength());
    vct.push_back(dv);
  }

  return vct;
}

VectorDataValue &PersistTable::GenColumsDataValues() {
  VectorDataValue vct;
  vct.reserve(_vctColumn.size());
  for (PersistColumn *col : _vctColumn) {
    IDataValue *dv =
        DataValueFactory(col->GetDataType(), false, col->GetMaxLength());
    vct.push_back(dv);
  }

  return vct;
}
} // namespace storage
