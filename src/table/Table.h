#pragma once
#include "../core/IndexType.h"
#include "Column.h"
#include <any>
#include <string>
#include <unordered_map>
#include <vector>

namespace storage {
using namespace std;
static const char *COLUMN_CONNECTOR_CHAR = "|";
static const char *NAME_PATTERN =
    "^[_a-zA-Z\\u4E00-\\u9FA5][_a-zA-Z0-9\\u4E00-\\u9FA5]*?$";
static const char *PRIMARY_KEY = "PARMARYKEY";
static void IsValidName(string name) {
  static regex reg(NAME_PATTERN);
  cmatch mt;
  if (!regex_match(name.c_str(), mt, reg)) {
    throw ErrorMsg(TB_INVALID_FILE_VERSION, {name});
  }
}

class BaseTable {};

class PersistTable : public BaseTable {
public:
public:
  PersistTable(string tableName, string description);
  PersistTable();
  ~PersistTable();
  inline const string GetTableName() const { return _name; }
  inline void SetTableName(string name) {
    IsValidName(name);
    _name = name;
  }

  const string &GetDescription() const { return _desc; }
  const string GetPrimaryKey() const { return PRIMARY_KEY; }
  const vector<PersistColumn *> &GetPrimaryKeyColumns() const {
    return _mapIndexColumn.at(PRIMARY_KEY);
  }
  const unordered_map<string, vector<PersistColumn *>> &
  GetMapIndexColumns() const {
    return _mapIndexColumn;
  }
  IndexType GetIndexType(string indexName) const;
  const unordered_map<string, IndexType> &GetMapIndexType() const {
    return _mapIndexType;
  }
  const vector<PersistColumn *> &GetColumnArray() const { return _vctColumn; }
  const unordered_map<string, string> &GetMapIndexName() const {
    return _mapIndexName;
  }
  const PersistColumn *GetColumn(string fieldName) const;
  const PersistColumn *GetColumn(int pos);
  unordered_map<string, size_t> GetMapColumnPos() { return _mapColumnPos; }
  unordered_map<string, string> GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  void AddColumn(string &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, string &comment, utils::Charsets charset,
                 any &valDefault);
  void SetPrimaryKey(vector<string> priCols);
  void SetPrimaryKey(string priCol, uint32_t incStep);

  void AddSecondaryKey(string indexName, IndexType indexType,
                       vector<string> colNames);

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }

protected:
  /**Table name*/
  string _name;
  /**Table describer*/
  string _desc;
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  vector<PersistColumn *> _vctColumn;
  /** The map for column name and their position in column list */
  unordered_map<string, size_t> _mapColumnPos;
  /**The first parameter is the unique name for a index and the primary key's
  name is fixed, must be PRIMARY_KEY. The second parameter is the column(s)'
  name to constitute the index.*/
  unordered_map<string, string> _mapIndexName;
  /**The first parameter, the unique name for a index;
  The second parameter, the index type: PRIMARY, UNIQUE, NON_UNIQUE*/
  unordered_map<string, IndexType> _mapIndexType;
  /** The first parameter, the unique name for a index;
  The second parameter, the columns to constitute it.*/
  unordered_map<string, vector<PersistColumn *>> _mapIndexColumn;
  /**The first column name to constitute the index;
  The second parameter,  the unique name for a index;*/
  unordered_map<string, string> _mapIndexFirstField;
};
} // namespace storage
