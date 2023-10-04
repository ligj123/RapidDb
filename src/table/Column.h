#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include "../utils/BytesFuncs.h"
#include "../utils/CharsetConvert.h"
#include <any>
#include <memory>

namespace storage {
using namespace std;

class PhysColumn {
public:
  PhysColumn(int32_t index)
      : _name(), _index(index), _dataType(DataType::UNKNOWN), _bNullable(false),
        _comments(), _maxLength(-1), _initVal(-1), _incStep(-1),
        _charset(Charsets::UNKNOWN), _pDefaultVal(nullptr) {}
  PhysColumn(const MString &name, int32_t index, DataType dataType,
             const MString &comments, bool bNullable, int32_t maxLen,
             int64_t initVal, int64_t incStep, Charsets charset,
             IDataValue *defaultVal = nullptr)
      : _name(name), _index(index), _dataType(dataType), _bNullable(bNullable),
        _comments(comments), _maxLength(maxLen), _initVal(initVal),
        _incStep(incStep), _charset(charset), _pDefaultVal(defaultVal) {
    if (_pDefaultVal != nullptr)
      _pDefaultVal->SetConstRef();
  }
  ~PhysColumn() {
    if (_pDefaultVal != nullptr)
      _pDefaultVal->Free();
  }
  PhysColumn(PhysColumn &&rhs) noexcept {
    _name = rhs._name;
    _index = rhs._index;
    _dataType = rhs._dataType;
    _bNullable = rhs._bNullable;
    _comments = rhs._comments;
    _maxLength = rhs._maxLength;
    _initVal = rhs._initVal;
    _incStep = rhs._incStep;
    _charset = rhs._charset;
    _pDefaultVal = rhs._pDefaultVal;
    rhs._pDefaultVal = nullptr;
  }
  PhysColumn(const PhysColumn &rhs) = delete;
  PhysColumn &operator=(const PhysColumn &rhs) = delete;

  const MString &GetName() const { return _name; }
  int32_t GetIndex() const { return _index; }
  DataType GetDataType() const { return _dataType; }
  bool IsNullable() const { return _bNullable; }
  int32_t GetMaxLength() const { return _maxLength; }
  int64_t GetInitVal() const { return _initVal; }
  int64_t GetIncStep() const { return _incStep; }
  Charsets GetCharset() const { return _charset; }
  const IDataValue *GetDefaultVal() const { return _pDefaultVal; }
  const MString &GetComments() const { return _comments; }

  uint32_t ReadData(Byte *pBuf);
  uint32_t WriteData(Byte *pBuf);
  /**
   * @brief To calucate the length of byte arrray to save this column
   * 1) 2 + n bytes: the column name length and contents
   * 2) 4 bytes: data type
   * 3) 1 byte: true or false, this column could be null or not
   * 4) 2 bytes: Charsets
   * 5) 4 bytes: The max length of this column's value
   * 6) 8 bytes: _initVal
   * 7) 8 bytes: _incStep
   * 8) 2 + n bytes: the column comments length and contents
   * 9) 2 + n bytes: The default value length and contents
   */
  uint32_t CalcSize();

protected:
  MString _name;      // The column's name
  int32_t _index;     // The index in table start from 0
  DataType _dataType; // Which data type for this column
  bool _bNullable;    // Can be null or not for this column
  Charsets _charset;  // The charset for char data type
  int32_t _maxLength; // The max length for variable length data type
  // The initalize value for auto-increment column, the default is 0
  int64_t _initVal;
  int64_t _incStep;  // The step between two neighbor value, the default is 1
  MString _comments; // Comments for this column
  IDataValue *_pDefaultVal; // If the default value has or null
};

} // namespace storage
