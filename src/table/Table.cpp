#include "Table.h"
#include "../dataType/DataValueFactory.h"
#include <boost/crc.hpp>
#include <filesystem>

namespace storage {

int32_t IndexProp::CalcSize() {
  uint32_t sz = UI16_LEN + (uint32_t)_name.size();
  sz += 1 + UI16_LEN;

  for (IndexColumn col : _vctCol) {
    sz += UI16_LEN + col.colName.size();
  }

  return sz;
}

uint32_t IndexProp::Write(Byte *bys) {
  Byte *tmp = bys;
  *(uint16_t *)tmp = (uint16_t)_name.size();
  tmp += UI16_LEN;
  BytesCopy(tmp, _name.c_str(), _name.size());
  tmp += _name.size();
  *tmp = (Byte)_type;
  tmp++;

  *(uint16_t *)tmp = (uint16_t)_vctCol.size();
  tmp += UI16_LEN;
  for (IndexColumn col : _vctCol) {
    *(uint16_t *)tmp = (uint16_t)col.colName.size();
    tmp += UI16_LEN;
    BytesCopy(tmp, col.colName.c_str(), col.colName.size());
    tmp += col.colName.size();
  }

  return (uint32_t)(tmp - bys);
}

uint32_t IndexProp::Read(Byte *bys, uint32_t pos,
                         const MHashMap<string, uint32_t> &mapColumnPos) {
  _position = pos;
  Byte *tmp = bys;
  uint16_t sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  _name = string((char *)bys, sz);
  bys += sz;
  _type = (IndexType) * (int8_t *)bys;
  bys++;

  sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  for (uint16_t i = 0; i < sz; i++) {
    IndexColumn col;
    uint16_t len = *(uint16_t *)bys;
    bys += UI16_LEN;

    col.colName = string((char *)bys, len);
    bys += len;

    col.colPos = mapColumnPos.find(col.colName)->second;
    _vctCol.push_back(col);
  }

  return uint32_t(bys - tmp);
}

bool PhysTable::AddColumn(string &columnName, DataType dataType, bool nullable,
                          uint32_t maxLen, string &comment, Charsets charset,
                          any &valDefault) {
  assert(_vctIndex.size() == 0);
  transform(columnName.begin(), columnName.end(), columnName.begin(),
            ::toupper);
  IsValidName(columnName);

  if (_mapColumnPos.find(columnName) != _mapColumnPos.end()) {
    _threadErrorMsg.reset(new ErrorMsg(TB_REPEATED_COLUMN_NAME, {columnName}));
    return false;
  }

  if (maxLen <= 0 && !IDataValue::IsArrayType(dataType)) {
    _threadErrorMsg.reset(new ErrorMsg(TB_Array_MAX_LEN, {columnName}));
    return false;
  }

  IDataValue *dvDefault = nullptr;
  if (valDefault.has_value()) {
    dvDefault = DataValueFactory(dataType, maxLen, valDefault);
  }

  PhysColumn *cm =
      new PhysColumn(columnName, (uint32_t)_vctColumn.size(), dataType, comment,
                     nullable, maxLen, -1, -1, Charsets::UTF8, dvDefault);

  _vctColumn.push_back(cm);
  _mapColumnPos.insert(pair<string, int>(columnName, cm->GetPosition()));
  return true;
}

bool PhysTable::AddColumn(string &columnName, DataType dataType,
                          string &comment, int64_t initVal, int64_t incStep) {
  assert(_vctIndex.size() == 0);
  assert(_vctColumn.size() == 0);

  transform(columnName.begin(), columnName.end(), columnName.begin(),
            ::toupper);
  IsValidName(columnName);

  if (_vctColumn.size() > 0) {
    _threadErrorMsg.reset(new ErrorMsg(TB_REPEATED_COLUMN_NAME, {columnName}));
    return false;
  }

  PhysColumn *cm = new PhysColumn(columnName, 0, dataType, comment, false, -1,
                                  initVal, incStep, Charsets::UTF8, nullptr);

  _vctColumn.push_back(cm);
  _mapColumnPos.insert(pair<string, int>(columnName, cm->GetPosition()));
  return true;
}

bool PhysTable::AddIndex(IndexType indexType, string &indexName,
                         MVector<string> &colNames) {
  if (colNames.size() == 0) {
    _threadErrorMsg.reset(new ErrorMsg(TB_INDEX_EMPTY_COLUMN, {indexName}));
    return false;
  }
  if (_mapIndexNamePos.find(indexName) != _mapIndexNamePos.end()) {
    _threadErrorMsg.reset(new ErrorMsg(TB_REPEATED_INDEX, {indexName}));
    return false;
  }

  assert(indexType != IndexType::PRIMARY || _vctIndex.size() == 0);

  MVector<IndexColumn> vctCol;
  MHashSet<MString> mset;
  for (string cname : colNames) {
    if (mset.contains(cname)) {
      _threadErrorMsg.reset(new ErrorMsg(TB_REPEATED_COLUMN_NAME, {cname}));
      return false;
    }
    mset.insert(cname);

    auto iter = _mapColumnPos.find(cname);
    if (iter == _mapColumnPos.end()) {
      _threadErrorMsg.reset(new ErrorMsg(TB_UNEXIST_COLUMN, {cname}));
      return false;
    }

    if (!IDataValue::IsIndexType(_vctColumn[iter->second].GetDataType())) {
      _threadErrorMsg.reset(new ErrorMsg(
          TB_INDEX_UNSUPPORT_DATA_TYPE,
          {col, DateTypeToString(_vctColumn[iter->second]->GetDataType())}));
      return false;
    }

    vctCol.push_back(IndexColumn(iter->first, iter->second));
  }

  string iname = (indexType == IndexType::PRIMARY ? PRIMARY_KEY : indexName);
  IndexProp prop(iname, (uint32_t)_vctIndex.size(), indexType, vctCol);
  _vctIndex.push_back(prop);
  _mapIndexNamePos.insert({prop->_name, prop->_position});
}

uint32_t PhysTable::CalcLength() {}

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

  string path = _db->GetPath() + "/" + _name + "/metafile.dat";
  ofstream fs(path.c_str(), ios::out | ios::binary | ios::trunc);
  fs.write((char *)bufs, buf - bufs);
  fs.close();
  CachePool::ReleaseBlock(bufs);
}

void PhysTable::ReadData() {
  Byte *buf = CachePool::ApplyBlock();
  Byte *bufs = buf;

  string path = _db->GetPath() + "/" + _name + "/metafile.dat";
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
  string path =
      _db->GetPath() + "/" + _name + "/" + _vctIndex[idx]->_name + ".idx";

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

  prop->_tree = new IndexTree();
  prop->_tree->InitIndex(prop->_name, path, dvKey, dvVal, 0);
  return true;
}

void PhysTable::GenSecondaryRecords(const LeafRecord *lrSrc,
                                    const LeafRecord *lrDst,
                                    const VectorDataValue &dstPr,
                                    ActionType type, Statement *stmt,
                                    VectorLeafRecord &vctRec) {
  if (_vctIndex.size() == 1) {
    return;
  }
  assert(lrSrc != nullptr || type == ActionType::INSERT);
  VectorDataValue srcPr;

  if (lrSrc != nullptr) {
    int rt = lrSrc->GetListValue(_vctIndexPos, srcPr);
    assert(rt >= 0);
  }

  for (size_t i = 1; i < _vctIndex.size(); i++) {
    IndexProp *prop = _vctIndex[i];
    VectorDataValue dstSk;

    dstSk.SetRef(true);
    dstSk.reserve(prop->_vctCol.size());

    for (IndexColumn &ic : prop->_vctCol) {
      dstSk.push_back(dstPr.at(ic.colPos));
    }
    if (lrSrc == nullptr) {
      LeafRecord *lr =
          new LeafRecord(prop->_tree, dstSk, lrDst->GetBysValue() + UI16_2_LEN,
                         lrDst->GetKeyLength(), ActionType::INSERT, stmt);
      vctRec.push_back(lr);
      continue;
    }

    VectorDataValue srcSk;
    srcSk.SetRef(true);
    srcSk.reserve(prop->_vctCol.size());
    for (IndexColumn &ic : prop->_vctCol) {
      srcSk.push_back(srcPr.at(ic.colPos));
    }

    assert(srcSk.size() == dstSk.size());
    bool equal = true;
    for (size_t i = 0; i < srcSk.size(); i++) {
      if (*srcSk[i] == *dstSk[i]) {
        equal = false;
        break;
      }
    }

    if (!equal) {
      LeafRecord *lrSrc2 =
          new LeafRecord(prop->_tree, srcSk, lrDst->GetBysValue() + UI16_2_LEN,
                         lrDst->GetKeyLength(), ActionType::DELETE, stmt);
      LeafRecord *lrDst2 =
          new LeafRecord(prop->_tree, dstSk, lrDst->GetBysValue() + UI16_2_LEN,
                         lrDst->GetKeyLength(), ActionType::INSERT, stmt);
      vctRec.push_back(lrSrc2);
      vctRec.push_back(lrDst2);
    }
  }
}
} // namespace storage