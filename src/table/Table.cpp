#include "Table.h"
#include "../dataType/DataValueFactory.h"
#include "../manager/DatabaseManager.h"
#include "./TableTaskMgr.h"

#include <boost/crc.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;

IndexProp::~IndexProp() {
  if (_tree != nullptr) {
    _tree->Close();
  }
}

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

uint32_t IndexProp::Read(const Byte *bys, uint32_t pos,
                         const MStrHashMap<uint32_t> &mapColumnPos) {
  _position = pos;
  const Byte *tmp = bys;
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

void PhysTable::Clear() {
  if (_tableTaskMgr != nullptr) {
    delete _tableTaskMgr;
  }

  _vctColumn.clear();
  _mapColumnPos.clear();
  _vctIndex.clear();
  _mapIndexNamePos.clear();
  _vctIndexPos.clear();
}

bool PhysTable::AddColumn(const MString &columnName, DataType dataType,
                          bool nullable, uint32_t maxLen,
                          const MString &comment, Charsets charset,
                          IDataValue *valDefault) {
  assert(_vctIndex.size() == 0);

  for (auto iter = _mapColumnPos.begin(); iter != _mapColumnPos.end(); iter++) {
    if (MStringEqualIgnoreCase(columnName, iter->first)) {
      _threadErrorMsg.reset(
          new ErrorMsg(TB_REPEATED_COLUMN_NAME, {columnName}));
      delete valDefault;
      return false;
    }
  }

  if (maxLen <= 0 && IDataValue::IsArrayType(dataType)) {
    _threadErrorMsg.reset(new ErrorMsg(TB_Array_MAX_LEN, {columnName}));
    delete valDefault;
    return false;
  }

  IDataValue *dvDefault = valDefault;
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
    _vctIndex.push_back(move(prop));
    _mapIndexNamePos.insert({prop._name, prop._position});
    return true;
  }

  MVector<IndexColumn> vctCol;
  MStrHashSet mset;
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

  _mapIndexFirstField.emplace(vctCol[0].colPos, (uint32_t)_vctIndex.size());
  IndexProp prop(iname, (uint32_t)_vctIndex.size(), indexType, vctCol);
  _mapIndexNamePos.insert({prop._name, prop._position});
  _vctIndex.push_back(move(prop));

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
  uint32_t len = UI32_LEN + UI32_LEN;
  len += UI16_LEN + (uint32_t)_fullName.size();
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
  // Table blob total length
  buf += UI32_LEN;
  // File version
  *(short *)buf = CURRENT_FILE_VERSION.GetMajorVersion();
  buf += UI16_LEN;
  *buf = CURRENT_FILE_VERSION.GetMinorVersion();
  buf++;
  *buf = CURRENT_FILE_VERSION.GetPatchVersion();
  buf++;

  // Table full name
  *(uint16_t *)buf = (uint16_t)_fullName.size();
  buf += UI16_LEN;
  BytesCopy(buf, _fullName.c_str(), _fullName.size());
  buf += _fullName.size();
  // Table create time
  *(uint64_t *)buf = _dtCreate;
  buf += UI64_LEN;
  // Table last update time
  *(uint64_t *)buf = _dtLastUpdate;
  buf += UI64_LEN;
  // Column count and information
  *(uint16_t *)buf = (uint16_t)_vctColumn.size();
  buf += UI16_LEN;

  for (size_t i = 0; i < _vctColumn.size(); i++) {
    uint32_t sz = _vctColumn[i].WriteData(buf);
    buf += sz;
  }
  // Index count and information
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

uint32_t PhysTable::LoadData(const Byte *bys) {
  const Byte *buf = bys;
  uint32_t sz = *(uint32_t *)buf;
  buf += UI32_LEN;

  // File Version verify
  FileVersion fv(*(int16_t *)buf, *(uint8_t *)(buf + 2), *(uint8_t *)(buf + 3));
  if (!(fv == CURRENT_FILE_VERSION)) {
    _threadErrorMsg.reset(new ErrorMsg(TB_ERROR_INDEX_VERSION));
    return UINT32_MAX;
  }
  buf += UI32_LEN;

  // Full table name
  uint32_t len = *(uint16_t *)buf;
  buf += UI16_LEN;
  _fullName = MString((char *)buf, len);
  buf += len;
  size_t pos = _fullName.find(".");
  _name = _fullName.substr(pos + 1);
  // Database name
  MString db_name = _fullName.substr(0, pos);
  _db = DatabaseManager::FindDb(db_name);
  assert(_db != nullptr);

  // Table create time
  _dtCreate = *(uint64_t *)buf;
  buf += UI64_LEN;
  // Table last update time
  _dtLastUpdate = *(uint64_t *)buf;
  buf += UI64_LEN;
  // Load column information
  len = *(uint16_t *)buf;
  buf += UI16_LEN;
  for (uint32_t i = 0; i < len; i++) {
    PhysColumn col(i);
    uint32_t csz = col.ReadData(buf);
    buf += csz;

    _mapColumnPos.insert({col.GetName(), i});
    _vctColumn.push_back(std::move(col));
  }
  // Load index information
  len = *(uint16_t *)buf;
  buf += UI16_LEN;
  for (uint32_t i = 0; i < len; i++) {
    IndexProp prop;
    uint32_t isz = prop.Read(buf, i, _mapColumnPos);
    buf += isz;

    _mapIndexFirstField.emplace(prop._vctCol[0].colPos, i);
    _mapIndexNamePos.insert({prop._name, i});
    _vctIndex.push_back(move(prop));

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

  _hash = MStrHash{}(_fullName);
  _tableStatus = ResStatus::Valid;
  return (uint32_t)(buf - bys);
}

bool PhysTable::OpenIndex(size_t idx, bool bCreate) {
  assert(idx >= 0 && idx < _vctIndex.size());
  IndexProp &prop = _vctIndex[idx];
  assert(prop._position == idx);

  VectorDataValue dvKey;
  dvKey.reserve(prop._vctCol.size());
  for (IndexColumn &ic : prop._vctCol) {
    PhysColumn &pc = _vctColumn[ic.colPos];
    dvKey.push_back(DataValueFactory(pc.GetDataType(), pc.GetMaxLength()));
  }

  VectorDataValue dvVal;
  if (idx == 0) {
    dvVal.reserve(_vctColumn.size());
    for (PhysColumn &pc : _vctColumn) {
      dvVal.push_back(DataValueFactory(pc.GetDataType(), pc.GetMaxLength()));
    }
  } else {
    IndexProp &pPri = _vctIndex[0];
    dvVal.reserve(pPri._vctCol.size());
    for (IndexColumn &ic : pPri._vctCol) {
      PhysColumn &pc = _vctColumn[ic.colPos];
      dvVal.push_back(DataValueFactory(pc.GetDataType(), pc.GetMaxLength()));
    }
  }

  MString tblPath = GetPath();
  MString idxPath = tblPath + "/" + _vctIndex[idx]._name + ".idx";
  prop._tree = new IndexTree();
  if (bCreate) {
    if (idx == 0) {
      if (!fs::exists(tblPath)) {
        fs::create_directories(tblPath);
      }
    }

    bool b =
        prop._tree->CreateIndexTree(_name, prop._name, idxPath, dvKey, dvVal,
                                    _tid + (uint32_t)idx, prop._type);

    if (!b) {
      return false;
    }

    if (idx == 0 && _vctColumn[0].GetInitVal() >= 0) {
      prop._tree->GetHeadPage()->SetAutoIncrementKey(
          _vctColumn[0].GetInitVal());
    }
  } else {
    assert(fs::exists(idxPath));
    bool b = prop._tree->LoadIndexTree(_name, prop._name, idxPath, dvKey, dvVal,
                                       _tid + (uint32_t)idx);
    if (!b) {
      return false;
    }
  }
  return true;
}

bool PhysTable::CheckColumnValues(VectorDataValue &vctDv) {
  assert(vctDv.size() == _vctColumn.size());

  for (size_t i = 0; i < vctDv.size(); i++) {
    PhysColumn &col = _vctColumn[i];
    if (vctDv[i]->IsNull()) {
      if (col.GetDefaultVal() != nullptr) {
        IDataValue *dv = const_cast<IDataValue *>(col.GetDefaultVal());
        vctDv[i]->Copy(*dv);
      } else if (!col.IsNullable()) {
        _threadErrorMsg.reset(
            new ErrorMsg(DT_NULL_VALUE, {col.GetName(), _name}));
        return false;
      }
    }
  }

  return true;
}

} // namespace storage