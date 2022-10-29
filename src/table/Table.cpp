#include "Table.h"
#include "../dataType/DataValueFactory.h"
#include <boost/crc.hpp>
#include <filesystem>

namespace storage {
// IndexProp buffer:
// 4 bytes to save buffer length, include this 4 bytes
// 2 bytes to save index name length + n bytes to save index name
// 2 bytes to save index position in all index, primarykey is 0
// 1 byte to save index type
// 2 bytes to save this index include how many columns
// Every column information will include 2 bytes column name length, n bytes
// column name and 2 bytes to save column position
uint32_t IndexProp::CalcSize() {
  uint32_t sz = UI32_LEN;
  sz += UI16_LEN + _name.size();
  sz += UI16_LEN + 1 + UI16_LEN + UI16_LEN * 2 * _vctCol.size();

  for (IndexColumn col : _vctCol) {
    sz += col.colName.size();
  }

  return sz;
}

uint32_t IndexProp::Write(Byte *bys, uint32_t bysLen) {
  Byte *tmp = bys + UI32_LEN;
  *(uint16_t *)tmp = (uint16_t)_name.size();
  tmp += UI16_LEN;
  BytesCopy(tmp, _name.c_str(), _name.size());
  tmp += _name.size();

  *(uint16_t *)tmp = _position;
  tmp += UI16_LEN;
  *tmp = (Byte)_type;
  tmp++;

  *(uint16_t *)tmp = (uint16_t)_vctCol.size();
  tmp += UI16_LEN;
  for (IndexColumn col : _vctCol) {
    *(uint16_t *)tmp = (uint16_t)col.colName.size();
    tmp += UI16_LEN;
    BytesCopy(tmp, col.colName.c_str(), col.colName.size());
    tmp += col.colName.size();
    *(uint16_t *)tmp = col.colPos;
    tmp += UI16_LEN;
  }

  assert((uint32_t)(tmp - bys) < bysLen);
  *(uint32_t *)bys = (uint32_t)(tmp - bys);

  return (uint32_t)(tmp - bys);
}

uint32_t IndexProp::Read(Byte *bys) {
  uint32_t len = *(uint32_t *)bys;

  bys += UI32_LEN;
  uint16_t sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  _name = string((char *)bys, sz);
  bys += sz;

  _position = *(uint16_t *)bys;
  bys += UI16_LEN;
  _type = (IndexType)(int8_t)bys;
  bys++;

  sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  for (uint16_t i = 0; i < sz; i++) {
    IndexColumn col;
    uint16_t len = *(uint16_t *)bys;
    bys += UI16_LEN;

    col.colName = string((char *)bys, len);
    bys += len;
    col.colPos = *(uint16_t *)bys;
    bys += UI16_LEN;
  }

  return len;
}

PhysTable::PhysTable(string &rootPath, string &dbName, string &tableName,
                     string &tableAlias, string &description)
    : BaseTable(tableName, tableAlias, description), _rootPath(rootPath),
      _dbName(dbName) {
  ReadData();
};

PhysTable::~PhysTable() {
  for (auto iter = _vctColumn.begin(); iter != _vctColumn.end(); iter++) {
    delete *iter;
  }
}

const PhysColumn *PhysTable::GetColumn(string &fieldName) const {
  auto iter = _mapColumnPos.find(fieldName);
  if (iter == _mapColumnPos.end()) {
    return nullptr;
  } else {
    return _vctColumn[iter->second];
  }
}

const PhysColumn *PhysTable::GetColumn(int pos) {
  if (pos < 0 || pos > _vctColumn.size()) {
    return nullptr;
  } else {
    return _vctColumn[pos];
  }
}

void PhysTable::AddColumn(string &columnName, DataType dataType, bool nullable,
                          uint32_t maxLen, string &comment, Charsets charset,
                          any &valDefault) {
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

  PhysColumn *cm =
      new PhysColumn(columnName, (uint32_t)_vctColumn.size(), dataType, comment,
                     nullable, maxLen, -1, -1, Charsets::UTF8, dvDefault);

  _vctColumn.push_back(cm);
  _mapColumnPos.insert(pair<string, int>(columnName, cm->GetPosition()));
}

void PhysTable::AddIndex(IndexType indexType, string &indexName,
                         MVector<string>::Type &colNames) {
  if (colNames.size() == 0) {
    throw ErrorMsg(TB_INDEX_EMPTY_COLUMN, {indexName});
  }
  if (_mapIndexNamePos.find(indexName) != _mapIndexNamePos.end()) {
    throw ErrorMsg(TB_REPEATED_INDEX, {indexName});
  }

  assert(indexType != IndexType::PRIMARY || _vctIndex.size() == 0);

  MVector<IndexColumn>::Type vctCol;
  for (string cname : colNames) {
    auto iter = _mapColumnPos.find(cname);
    if (iter == _mapColumnPos.end()) {
      throw ErrorMsg(TB_UNEXIST_COLUMN, {cname});
    }
  }

  MVector<IndexColumn>::Type vctIc;
  for (string col : colNames) {
    auto iter = _mapColumnPos.find(col);
    if (iter == _mapColumnPos.end()) {
      throw ErrorMsg(TB_UNEXIST_COLUMN, {col});
    }

    if (!IDataValue::IsIndexType(_vctColumn[iter->second]->GetDataType())) {
      throw ErrorMsg(
          TB_INDEX_UNSUPPORT_DATA_TYPE,
          {col, DateTypeToString(_vctColumn[iter->second]->GetDataType())});
    }

    vctIc.push_back(IndexColumn(iter->first, iter->second));
  }
  string iname = (indexType == IndexType::PRIMARY ? PRIMARY_KEY : indexName);
  IndexProp *prop = new IndexProp(iname, _vctIndex.size(), indexType, vctIc);
  _vctIndex.push_back(prop);
  _mapIndexNamePos.insert({prop->_name, prop->_position});
}

void PhysTable::WriteData() {
  Byte *buf = CachePool::ApplyBlock();
  Byte *bufs = buf;

  *(short *)buf = CURRENT_FILE_VERSION.GetMajorVersion();
  buf += UI16_LEN;
  *buf = CURRENT_FILE_VERSION.GetMinorVersion();
  buf++;
  *buf = CURRENT_FILE_VERSION.GetPatchVersion();
  buf++;
  buf += UI32_LEN * 2;

  *(uint32_t *)buf = (uint32_t)_name.size();
  buf += UI32_LEN;
  BytesCopy(buf, _name.c_str(), _name.size());
  buf += _name.size();
  *(uint32_t *)buf = (uint32_t)_desc.size();
  buf += UI32_LEN;
  BytesCopy(buf, _desc.c_str(), _desc.size());
  buf += _desc.size();

  *(uint32_t *)buf = (uint32_t)_vctColumn.size();
  buf + UI32_LEN;

  for (size_t i = 0; i < _vctColumn.size(); i++) {
    PhysColumn *col = _vctColumn[i];
    uint32_t len = col->WriteData(buf);
    buf += len;
  }

  *(uint32_t *)buf = (uint32_t)_vctIndex.size();
  buf += UI32_LEN;
  for (auto prop : _vctIndex) {
    uint32_t len = prop->Write(buf, 102400);
    buf += len;
  }

  assert(buf - bufs < Configure::GetResultBlockSize());

  *(uint32_t *)(bufs + UI32_LEN * 2) = (uint32_t)(buf - bufs - UI32_LEN * 2);
  boost::crc_32_type crc32;
  crc32.process_bytes(bufs + UI32_LEN * 2, buf - bufs - UI32_LEN * 2);
  *(uint32_t *)(bufs + UI32_LEN) = crc32.checksum();

  string path = _rootPath + "/" + _dbName + "/" + _name + "/metafile.dat";
  ofstream fs(path.c_str(), ios::out | ios::binary | ios::trunc);
  fs.write((char *)bufs, buf - bufs);
  fs.close();
  CachePool::ReleaseBlock(bufs);
}

void PhysTable::ReadData() {
  Byte *buf = CachePool::ApplyBlock();
  Byte *bufs = buf;

  string path = _rootPath + "/" + _dbName + "/" + _name + "/metafile.dat";
  ifstream fs(path.c_str(), ios::in | ios::binary);
  fs.read((char *)buf, Configure::GetResultBlockSize());
  uint32_t sz = fs.gcount();
  fs.close();
  assert(sz < Configure::GetResultBlockSize());

  FileVersion fv(*(int16_t *)buf, *(uint8_t *)(buf + 2), *(uint8_t *)(buf + 3));
  if (!(fv == CURRENT_FILE_VERSION)) {
    CachePool::ReleaseBlock(bufs);
    throw ErrorMsg(TB_ERROR_INDEX_VERSION, {path});
  }

  buf += UI32_LEN;

  uint32_t len = *(uint32_t *)(buf + UI32_LEN);
  if (len + UI32_LEN * 2 != sz) {
    CachePool::ReleaseBlock(bufs);
    throw ErrorMsg(FILE_OPEN_FAILED, {path});
  }

  boost::crc_32_type crc32;
  crc32.process_bytes(buf + UI32_LEN, len);
  if (*(uint32_t *)buf != crc32.checksum()) {
    CachePool::ReleaseBlock(buf);
    throw ErrorMsg(FILE_OPEN_FAILED, {path});
  }

  buf += UI32_LEN * 2;

  len = *(uint32_t *)buf;
  buf += UI32_LEN;
  _name = string((char *)buf, len);
  buf += len;

  len = *(uint32_t *)buf;
  buf += UI32_LEN;
  _desc = string((char *)buf, len);
  buf += len;

  len = *(uint32_t *)buf;
  buf += UI32_LEN;
  for (uint32_t i = 0; i < len; i++) {
    PhysColumn *col = new PhysColumn();
    uint32_t sz = col->ReadData(buf);
    buf += sz;

    _vctColumn.push_back(col);
    _mapColumnPos.insert({col->GetName(), i});
  }

  len = *(uint32_t *)buf;
  buf += UI32_LEN;

  for (int i = 0; i < len; i++) {
    IndexProp *prop = new IndexProp;
    uint32_t sz = prop->Read(buf);
    buf += sz;

    _vctIndex.push_back(prop);
    _mapIndexNamePos.insert({prop->_name, i});
    _mapIndexFirstField.insert({i, prop->_vctCol[0].colPos});
  }
}

bool PhysTable::OpenIndex(size_t idx, bool bCreate) {
  assert(idx > 0 && idx < _vctIndex.size());
  IndexProp *prop = _vctIndex[idx];
  string path = _rootPath + "/" + _dbName + "/" + _name + "/" +
                _vctIndex[idx]->_name + ".idx";

  VectorDataValue dvKey;
  dvKey.reserve(prop->_vctCol.size());
  for (IndexColumn ic : prop->_vctCol) {
    PhysColumn *pc = _vctColumn[ic.colPos];
    dvKey.push_back(
        DataValueFactory(pc->GetDataType(), true, pc->GetMaxLength()));
  }

  VectorDataValue dvVal;
  if (idx == 0) {
    dvVal.reserve(_vctColumn.size());
    for (PhysColumn *pc : _vctColumn) {
      dvVal.push_back(
          DataValueFactory(pc->GetDataType(), false, pc->GetMaxLength()));
    }
  } else {
    IndexProp *pPri = _vctIndex[0];
    dvVal.reserve(pPri->_vctCol.size());
    for (IndexColumn ic : pPri->_vctCol) {
      PhysColumn *pc = _vctColumn[ic.colPos];
      dvKey.push_back(
          DataValueFactory(pc->GetDataType(), true, pc->GetMaxLength()));
    }
  }

  if (bCreate == filesystem::exists(path)) {
    LOG_FATAL << "The index file"
              << (bCreate ? "has existed" : "should be existed")
              << ". path= " << path;
    abort();
  }

  prop->_tree = new IndexTree(prop->_name, path, dvKey, dvVal);
}

bool PhysTable::GenUpdateRecord(LeafRecord *srcRec, VectorDataValue *dstPr,
                                ActionType type, VectorLeafRecord &vctRec) {
                                  
                                }
} // namespace storage