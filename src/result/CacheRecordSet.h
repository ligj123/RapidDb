#pragma once
#include "../dataType/IDataValue.h"
#include "IResultSet.h"

namespace storage {
struct RowData {
  RowData(LeafRecord *lr, VectorDataValue &&vctDv)
      : _lr(lr), _vctDv(move(vctDv)) {}

  LeafRecord *_lr;
  VectorDataValue _vctDv;
};

class CacheRecordSet : public IResultSet {
public:
  CacheRecordSet(MVectorPtr<ExprColumn *> *vctCol) : IResultSet(vctCol) {}
  ~CacheRecordSet() {}
  void AddRow(VectorDataValue &&vctDv, LeafRecord *lr = nullptr) override {
    _vctRow.emplace_back(lr, move(vctDv));
  }
  bool First() override {
    if (_vctRow.size() == 0) {
      return false;
    }

    _currPos = 0;
    return true;
  }
  bool HasNext() override { return _currPos < _vctRow.size() - 1; }
  bool Next() override {
    if (_currPos < _vctRow.size()) {
      _currPos++;
    }

    if (_currPos < _vctRow.size()) {
      return true;
    } else {
      return true;
    }
  }
  bool Last() override {
    if (_vctRow.size() == 0) {
      return false;
    }

    _currPos = _vctRow.size() - 1;
    return true;
  }

  bool Absolute(int row) override {
    if (row < 0 || row >= _vctRow.size()) {
      return false;
    } else {
      _currPos = row;
      return true;
    }
  }
  bool IsValidRow() override {
    return _currPos >= 0 && _currPos < _vctRow.size();
  }
  int64_t GetRowCount() override { return _vctRow.size(); }
  int getFieldsCount() override { return static_cast<int>(_vctCol->size()); }
  DataType GetFieldDataType(int fieldIndex) override {
    if (fieldIndex < 0 || fieldIndex >= _vctCol->size()) {
      return DataType::UNKNOWN;
    }

    return _vctCol->at(fieldIndex)->_dataType;
  }
  MString &GetFieldName(int fieldIndex) override {
    static MString emptyStr;
    if (fieldIndex < 0 || fieldIndex >= _vctCol->size()) {
      return emptyStr;
    }

    return *(_vctCol->at(fieldIndex)->_name);
  }
  int GetFieldIndex(MString &fieldName) override {
    auto iter = _mapColPos.find(fieldName);
    if (iter != _mapColPos.end()) {
      return iter->second;
    } else {
      return -1;
    }
  }
  IDataValue *GetDataValue(int fieldIndex) override {
    if (_currPos < 0 || _currPos >= _vctRow.size()) {
      return nullptr;
    }
    if (fieldIndex < 0 || fieldIndex >= _vctCol->size()) {
      return nullptr;
    }

    return _vctRow[_currPos]._vctDv[fieldIndex]->AddRef();
  }

  IDataValue *GetDataValue(MString &fieldName) override {
    if (_currPos < 0 || _currPos >= _vctRow.size()) {
      return nullptr;
    }
    auto iter = _mapColPos.find(fieldName);
    if (iter == _mapColPos.end()) {
      return nullptr;
    } else {
      return _vctRow[_currPos]._vctDv[iter->second]->AddRef();
    }
  }

  bool GetCurrDataValueRow(VectorDataValue &vct) override {
    assert(vct.size() == 0);
    if (_currPos < 0 || _currPos >= _vctRow.size()) {
      return false;
    }

    VectorDataValue &src = _vctRow[_currPos]._vctDv;
    vct.reserve(src.size());
    for (IDataValue *dv : src) {
      vct.push_back(dv->AddRef());
    }

    return true;
  }

protected:
  MVector<RowData> _vctRow;
  int64_t _currPos;
};
} // namespace storage
