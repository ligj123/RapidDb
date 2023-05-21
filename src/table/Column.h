#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include "../utils/BytesConvert.h"
#include "../utils/CharsetConvert.h"
#include <any>

namespace storage {
using namespace std;

class PhysColumn {
public:
  PhysColumn()
      : _name(), _index(), _dataType(), BaseColumn(), _bNullable(false),
        _comments(), _maxLength(0), _initVal(0), _incStep(1), _charset(),
        _pDefaultVal(nullptr) {}
  PhysColumn(const string &name, int32_t index, DataType dataType,
             const string &comments, bool bNullable, int32_t maxLen,
             int64_t initVal, int64_t incStep, Charsets charset,
             IDataValue *defaultVal = nullptr)
      : _name(name), _index(index), _dataType(dataType), _bNullable(bNullable),
        _comments(comments), _maxLength(maxLen), _initVal(initVal),
        _incStep(incStep), _charset(charset), _pDefaultVal(defaultVal) {}
  ~PhysColumn() {
    if (_pDefaultVal != nullptr)
      delete _pDefaultVal;
  }

public:
  const string &GetName() const { return _name; }
  int32_t GetIndex() { return _index; }
  DataType GetDataType() { return _dataType; }
  uint32_t ReadData(Byte *pBuf);
  uint32_t WriteData(Byte *pBuf);
  bool IsNullable() const { return _bNullable; }
  int32_t GetMaxLength() const { return _maxLength; }
  int64_t GetInitVal() const { return _initVal; }
  int64_t GetIncStep() const { return _incStep; }
  Charsets GetCharset() { return _charset; }
  const IDataValue *GetDefaultVal() { return _pDefaultVal; }
  const string &GetComments() { return _comments; }

protected:
  string _name;       // The column's name
  int32_t _index;     // The index in table start from 0
  DataType _dataType; // Which data type for this column
  bool _bNullable;    // Can be null or not for this column
  Charsets _charset;  // The charset for char data type
  int32_t _maxLength; // The max length for variable length data type
  // The initalize value for auto-increment column, the default is 0
  int64_t _initVal;
  int64_t _incStep; // The step between two neighbor value, the default is 1
  string _comments; // Comments for this column
  IDataValue *_pDefaultVal; // The default value if has or null
};

} // namespace storage
