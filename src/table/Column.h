#pragma once
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include "../utils/CharsetConvert.h"
#include <any>
#include <string>

namespace storage {
using namespace std;
class BaseColumn {
public:
  BaseColumn() : _name(), _position(), _dataType() {}
  BaseColumn(std::string name, int32_t pos, DataType dataType)
      : _name(name), _position(pos), _dataType(dataType) {}
  virtual ~BaseColumn() {}
  const string &GetName() const { return _name; }
  int32_t GetPosition() { return _position; }
  DataType GetDataType() { return _dataType; }

  virtual uint32_t ReadData(Byte *pBuf) = 0;
  virtual uint32_t WriteData(Byte *pBuf) = 0;

protected:
  string _name;       // The column's name
  int32_t _position;  // The position that start from 0
  DataType _dataType; // Which data type for this column
};

class PersistColumn : public BaseColumn {
public:
  PersistColumn()
      : BaseColumn(), _bNullable(false), _comments(), _maxLength(0),
        _initVal(0), _incStep(1), _charset(), _pDefaultVal(nullptr) {}
  PersistColumn(const std::string &name, int32_t pos, DataType dataType,
                const string &comments, bool bNullable, int32_t maxLen,
                int64_t initVal, int64_t incStep, utils::Charsets charset,
                IDataValue *defaultVal)
      : BaseColumn(name, pos, dataType), _bNullable(bNullable),
        _comments(comments), _maxLength(maxLen), _initVal(initVal),
        _incStep(incStep), _charset(charset), _pDefaultVal(defaultVal) {}
  ~PersistColumn() { delete _pDefaultVal; }

public:
  uint32_t ReadData(Byte *pBuf) override;
  uint32_t WriteData(Byte *pBuf) override;
  bool IsNullable() const { return _bNullable; }
  int32_t GetMaxLength() const { return _maxLength; }
  int64_t GetInitVal() const { return _initVal; }
  int64_t GetIncStep() const { return _incStep; }
  utils::Charsets GetCharset() { return _charset; }
  const IDataValue *GetDefaultVal() { return _pDefaultVal; }
  const string &GetComments() { return _comments; }

protected:
  bool _bNullable;          // Can bu null or not for this column
  utils::Charsets _charset; // The charset for char data type
  int32_t _maxLength;       // The max length for variable length data type
  int64_t _initVal; // The initalize value for auto-increment column, the
                    // default is 0
  int64_t _incStep; // The step between two increment, the default is 1
  string _comments; // Comments for this column
  IDataValue *_pDefaultVal; // The default value if has or null
};

class TempColumn : public BaseColumn {
public:
  TempColumn(const std::string &name, uint32_t pos, DataType dataType,
             string alias, int dataBasicStart, int prevVarCols,
             int colNullPlace)
      : BaseColumn(name, pos, dataType), _alias(alias),
        _dataBasicStart(dataBasicStart), _prevVarCols(prevVarCols),
        _colNullPlace(colNullPlace) {}

  const string &GetAlias() const { return _alias; }
  const int GetDataBasicStart() const { return _dataBasicStart; }
  const int GetPrevVarCols() const { return _prevVarCols; }
  const int GetColNullPlace() const { return _colNullPlace; }

protected:
  string _alias; // The ailas of this column
  /**The data position in this record that start from row's values,
   * all variable columns's length set to 0 in this variable.
   * The variable columns' length saved to byte arrays*/
  int _dataBasicStart;
  /**To save how many variable columns(String or blob) before this column*/
  int _prevVarCols;
  /**How many bytes to save which columna are null, 8 columns per byte*/
  int _colNullPlace;
};
} // namespace storage
