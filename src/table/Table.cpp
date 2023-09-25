#include "Table.h"
#include "../dataType/DataValueFactory.h"
#include <boost/crc.hpp>
#include <filesystem>

namespace storage {

uint32_t IndexProp::CalcSize() {
  uint32_t sz = UI16_LEN + (uint32_t)_name.size();
  sz += 1 + UI16_LEN;

  for (IndexColumn col : _vctCol) {
    sz += UI16_LEN + (uint32_t)col.colName.size();
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
                         const MHashMap<MString, uint32_t> &mapColumnPos) {
  _position = pos;
  Byte *tmp = bys;
  uint16_t sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  _name = MString((char *)bys, sz);
  bys += sz;
  _type = (IndexType) * (int8_t *)bys;
  bys++;

  sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  for (uint16_t i = 0; i < sz; i++) {
    IndexColumn col;
    uint16_t len = *(uint16_t *)bys;
    bys += UI16_LEN;

    col.colName = MString((char *)bys, len);
    bys += len;

    col.colPos = mapColumnPos.find(col.colName)->second;
    _vctCol.push_back(col);
  }

  return uint32_t(bys - tmp);
}

bool PhysTable::AddColumn(const MString &columnName, DataType dataType,
                          bool nullable, uint32_t maxLen,
                          const MString &comment, Charsets charset,
                          const any &valDefault) {
  assert(_vctIndex.size() == 0);

  for (auto iter = _mapColumnPos.begin(); iter != _mapColumnPos.end(); iter++) {
    if (MStringEqualIgnoreCase(columnName, iter->first)) {
      _threadErrorMsg.reset(
          new ErrorMsg(TB_REPEATED_COLUMN_NAME, {columnName}));
      return false;
    }
  }

  if (maxLen <= 0 && IDataValue::IsArrayType(dataType)) {
    _threadErrorMsg.reset(new ErrorMsg(TB_Array_MAX_LEN, {columnName}));
    return false;
  }

  IDataValue *dvDefault = nullptr;
  if (valDefault.has_value()) {
    dvDefault = DataValueFactory(dataType, maxLen, valDefault);
  }

  _mapColumnPos.insert(pair<MString, int>(columnName, (int)_vctColumn.size()));
  _vctColumn.emplace_back(columnName, (uint32_t)_vctColumn.size(), dataType,
                          comment, nullable, maxLen, -1, -1, Charsets::UTF8,
                          dvDefault);
  return true;
}

bool PhysTable::AddColumn(const MString &columnName, DataType dataType,
                          const MString &comment, int64_t initVal,
                          int64_t incStep) {
  assert(_vctIndex.size() == 0);
  assert(_vctColumn.size() == 0);

  if (_vctColumn.size() > 0) {
    _threadErrorMsg.reset(new ErrorMsg(TB_REPEATED_COLUMN_NAME, {columnName}));
    return false;
  }

  _mapColumnPos.insert(pair<MString, int>(columnName, 0));
  _vctColumn.emplace_back(columnName, 0, dataType, comment, false, -1, initVal,
                          incStep, Charsets::UTF8, nullptr);
  return true;
}

bool PhysTable::AddIndex(IndexType indexType, const MString &indexName,
                         const MVector<MString> &colNames) {
  if (colNames.size() == 0 && indexType != IndexType::HIDE_PRIMARY) {
    _threadErrorMsg.reset(new ErrorMsg(TB_INDEX_EMPTY_COLUMN, {indexName}));
    return false;
  }

  MString iname = (indexType == IndexType::PRIMARY ? PRIMARY_KEY : indexName);

  for (auto iter = _mapIndexNamePos.begin(); iter != _mapIndexNamePos.end();
       iter++) {
    if (MStringEqualIgnoreCase(iter->first, iname)) {
      _threadErrorMsg.reset(new ErrorMsg(TB_REPEATED_INDEX, {iname}));
      return false;
    }
  }

  assert((indexType != IndexType::PRIMARY &&
          indexType != IndexType::HIDE_PRIMARY) ||
         _vctIndex.size() == 0);

  if (indexType == IndexType::HIDE_PRIMARY) {
    MVector<IndexColumn> vctCol;
    IndexProp prop(PRIMARY_KEY, 0, indexType, vctCol);
    _vctIndex.push_back(prop);
    _mapIndexNamePos.insert({prop._name, prop._position});
    return true;
  }

  MVector<IndexColumn> vctCol;
  MHashSet<MString> mset;
  for (const MString &cname : colNames) {
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
          {cname, DateTypeToMString(_vctColumn[iter->second].GetDataType())}));
      return false;
    }

    vctCol.push_back(IndexColumn(iter->first, iter->second));
  }

  IndexProp prop(iname, (uint32_t)_vctIndex.size(), indexType, vctCol);
  _vctIndex.push_back(prop);
  _mapIndexNamePos.insert({prop._name, prop._position});

  if (indexType == IndexType::PRIMARY)
    return true;

  for (IndexColumn &ic : prop._vctCol) {
    size_t i = 0;
    for (; i < _vctIndexPos.size(); i++) {
      if (_vctIndexPos[i] == ic.colPos)
        break;
      else if (_vctIndexPos[i] > (int)ic.colPos) {
        _vctIndexPos.insert(_vctIndexPos.begin() + i, ic.colPos);
        break;
      }
    }
    if (i == _vctIndexPos.size())
      _vctIndexPos.push_back(ic.colPos);
  }

  return true;
}

uint32_t PhysTable::CalcSize() {
  uint32_t len = UI32_LEN + UI32_LEN + UI32_LEN;
  len += UI16_LEN + (uint32_t)_fullName.size();
  len += UI16_LEN + (uint32_t)_desc.size();
  len += UI64_LEN + UI64_LEN;
  len += UI16_LEN;

  for (size_t i = 0; i < _vctColumn.size(); i++) {
    len += _vctColumn[i].CalcSize();
  }

  len += UI16_LEN;
  for (size_t i = 0; i < _vctIndex.size(); i++) {
    len += _vctIndex[i].CalcSize();
  }

  return len;
}

uint32_t PhysTable::SaveData(Byte *bys) {
  Byte *buf = bys;
  buf += UI32_LEN;

  *(short *)buf = CURRENT_FILE_VERSION.GetMajorVersion();
  buf += UI16_LEN;
  *buf = CURRENT_FILE_VERSION.GetMinorVersion();
  buf++;
  *buf = CURRENT_FILE_VERSION.GetPatchVersion();
  buf++;

  *(uint32_t *)buf = _tid;
  buf += UI32_LEN;

  *(uint16_t *)buf = (uint16_t)_fullName.size();
  buf += UI16_LEN;
  BytesCopy(buf, _fullName.c_str(), _fullName.size());
  buf += _fullName.size();

  *(uint16_t *)buf = (uint16_t)_desc.size();
  buf += UI16_LEN;
  BytesCopy(buf, _desc.c_str(), _desc.size());
  buf += _desc.size();

  *(uint64_t *)buf = _dtCreate;
  buf += UI64_LEN;
  *(uint64_t *)buf = _dtLastUpdate;
  buf += UI64_LEN;

  *(uint16_t *)buf = (uint16_t)_vctColumn.size();
  buf += UI16_LEN;

  for (size_t i = 0; i < _vctColumn.size(); i++) {
    uint32_t sz = _vctColumn[i].WriteData(buf);
    buf += sz;
  }

  *(uint16_t *)buf = (uint16_t)_vctIndex.size();
  buf += UI16_LEN;
  for (size_t i = 0; i < _vctIndex.size(); i++) {
    uint32_t len = _vctIndex[i].Write(buf);
    buf += len;
  }

  uint32_t sz = (int32_t)(buf - bys);
  *(uint32_t *)buf = sz;
  return sz;
}

uint32_t PhysTable::LoadData(Byte *bys) {
  Byte *buf = bys;
  uint32_t sz = *(uint32_t *)buf;
  buf += UI32_LEN;

  FileVersion fv(*(int16_t *)buf, *(uint8_t *)(buf + 2), *(uint8_t *)(buf + 3));
  if (!(fv == CURRENT_FILE_VERSION)) {
    _threadErrorMsg.reset(new ErrorMsg(TB_ERROR_INDEX_VERSION));
    return UINT32_MAX;
  }
  buf += UI32_LEN;

  _tid = *(uint32_t *)buf;
  buf += UI32_LEN;

  uint32_t len = *(uint16_t *)buf;
  buf += UI16_LEN;
  _fullName = MString((char *)buf, len);
  buf += len;
  size_t pos = _fullName.find(".");
  _name = _fullName.substr(pos + 1);
  _dbName = _fullName.substr(0, pos);

  len = *(uint16_t *)buf;
  buf += UI16_LEN;
  _desc = MString((char *)buf, len);
  buf += len;

  _dtCreate = *(uint64_t *)buf;
  buf += UI64_LEN;
  _dtLastUpdate = *(uint64_t *)buf;
  buf += UI64_LEN;

  len = *(uint16_t *)buf;
  buf += UI16_LEN;
  for (uint32_t i = 0; i < len; i++) {
    PhysColumn col(i);
    uint32_t csz = col.ReadData(buf);
    buf += csz;

    _mapColumnPos.insert({col.GetName(), i});
    _vctColumn.push_back(std::move(col));
  }

  len = *(uint16_t *)buf;
  buf += UI16_LEN;
  for (uint32_t i = 0; i < len; i++) {
    IndexProp prop;
    uint32_t isz = prop.Read(buf, i, _mapColumnPos);
    buf += isz;

    _vctIndex.push_back(prop);
    _mapIndexNamePos.insert({prop._name, i});

    for (IndexColumn &ic : prop._vctCol) {
      size_t i = 0;
      for (; i < _vctIndexPos.size(); i++) {
        if (_vctIndexPos[i] == ic.colPos)
          break;
        else if (_vctIndexPos[i] > (int)ic.colPos) {
          _vctIndexPos.insert(_vctIndexPos.begin() + i, ic.colPos);
          break;
        }
      }
      if (i == _vctIndexPos.size())
        _vctIndexPos.push_back(ic.colPos);
    }
  }

  return (uint32_t)(bys - buf);
}

bool PhysTable::OpenIndex(size_t idx, bool bCreate) {
  assert(idx >= 0 && idx < _vctIndex.size());
  IndexProp &prop = _vctIndex[idx];
  MString path = Configure::GetDbRootPath().c_str();
  path += "/" + _dbName + "/" + _name + "/" + _vctIndex[idx]._name + ".idx";

  VectorDataValue dvKey;
  dvKey.reserve(prop._vctCol.size());
  for (IndexColumn &ic : prop._vctCol) {
    PhysColumn &pc = _vctColumn[ic.colPos];
    dvKey.push_back(
        DataValueFactory(pc.GetDataType(), true, pc.GetMaxLength()));
  }

  VectorDataValue dvVal;
  if (idx == 0) {
    dvVal.reserve(_vctColumn.size());
    for (PhysColumn &pc : _vctColumn) {
      dvVal.push_back(
          DataValueFactory(pc.GetDataType(), false, pc.GetMaxLength()));
    }
  } else {
    IndexProp &pPri = _vctIndex[0];
    dvVal.reserve(pPri._vctCol.size());
    for (IndexColumn &ic : pPri._vctCol) {
      PhysColumn &pc = _vctColumn[ic.colPos];
      dvKey.push_back(
          DataValueFactory(pc.GetDataType(), true, pc.GetMaxLength()));
    }
  }

  assert(bCreate == filesystem::exists(path));

  prop._tree = new IndexTree();
  if (bCreate)
    prop._tree->CreateIndex(prop._name, path, dvKey, dvVal,
                            _tid + (uint32_t)idx, prop._type);
  else
    prop._tree->InitIndex(prop._name, path, dvKey, dvVal, _tid + (uint32_t)idx);
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
    IndexProp &prop = _vctIndex[i];
    VectorDataValue dstSk;

    dstSk.reserve(prop._vctCol.size());

    for (IndexColumn &ic : prop._vctCol) {
      dstSk.push_back(dstPr.at(ic.colPos)->AddRef());
    }
    if (lrSrc == nullptr) {
      LeafRecord *lr =
          new LeafRecord(prop._tree, dstSk, lrDst->GetBysValue() + UI16_2_LEN,
                         lrDst->GetKeyLength(), ActionType::INSERT, stmt);
      vctRec.push_back(lr);
      continue;
    }

    VectorDataValue srcSk;
    srcSk.reserve(prop._vctCol.size());
    for (IndexColumn &ic : prop._vctCol) {
      srcSk.push_back(srcPr.at(ic.colPos)->AddRef());
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
          new LeafRecord(prop._tree, srcSk, lrDst->GetBysValue() + UI16_2_LEN,
                         lrDst->GetKeyLength(), ActionType::DELETE, stmt);
      LeafRecord *lrDst2 =
          new LeafRecord(prop._tree, dstSk, lrDst->GetBysValue() + UI16_2_LEN,
                         lrDst->GetKeyLength(), ActionType::INSERT, stmt);
      vctRec.push_back(lrSrc2);
      vctRec.push_back(lrDst2);
    }
  }
}
} // namespace storage