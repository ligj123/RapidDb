#pragma once
#include "../config/ErrorID.h"
#include "../core/IndexType.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include "Column.h"
#include <any>
#include <string>
#include <unordered_map>
#include <vector>

namespace storage {
using namespace std;
static const char *COLUMN_CONNECTOR_CHAR = "|";
static const char *PRIMARY_KEY = "PARMARYKEY";

struct IndexProp {
  IndexType type;
  struct Column {
    string colName;
    int colPos = 0;
  };
  vector<Column> vctCol;
};

class BaseTable {
public:
  BaseTable() {}
  BaseTable(const string &name, const string &desc)
      : _name(name), _desc(desc) {}
  virtual ~BaseTable() {}
  inline const string GetTableName() const { return _name; }
  inline void SetTableName(string name) {
    IsValidName(name);
    _name = name;
  }
  inline const string &GetDescription() const { return _desc; }
  inline void SetTableDesc(const string &desc) { _desc = desc; }

protected:
  /**Table name*/
  string _name;
  /**Table describer*/
  string _desc;
};

class PersistTable : public BaseTable {
public:
public:
  PersistTable(string tableName, string description)
      : BaseTable(tableName, description){};
  PersistTable(){};
  ~PersistTable();

  const string GetPrimaryKey() const { return PRIMARY_KEY; }
  const IndexProp &GetPrimaryKeyColumns() const {
    return _mapIndex.at(PRIMARY_KEY);
  }
  const unordered_map<string, IndexProp> &GetMapIndex() const {
    return _mapIndex;
  }
  IndexType GetIndexType(string indexName) const {
    auto iter = _mapIndex.find(indexName);
    if (iter == _mapIndex.end())
      return IndexType::UNKNOWN;
    return iter->second.type;
  }

  const vector<PersistColumn *> &GetColumnArray() const { return _vctColumn; }

  const PersistColumn *GetColumn(string fieldName) const;
  const PersistColumn *GetColumn(int pos);
  unordered_map<string, int> GetMapColumnPos() { return _mapColumnPos; }
  unordered_map<int, string> GetIndexFirstFieldMap() {
    return _mapIndexFirstField;
  }

  void AddColumn(string &columnName, DataType dataType, bool nullable,
                 uint32_t maxLen, string &comment, utils::Charsets charset,
                 any &valDefault);
  void SetPrimaryKey(vector<string> priCols);

  void AddSecondaryKey(string indexName, IndexType indexType,
                       vector<string> colNames);

protected:
  inline bool IsExistedColumn(string name) {
    return _mapColumnPos.find(name) != _mapColumnPos.end();
  }

protected:
  /**Include all columns in this table, they will order by actual position in
   * the table.*/
  vector<PersistColumn *> _vctColumn;
  /** The map for column name and their position in column list */
  unordered_map<string, int> _mapColumnPos;
  /**The first parameter is the unique name for a index and the primary key's
  name is fixed, must be PRIMARY_KEY. The second parameter is IndexProp.*/
  unordered_map<string, IndexProp> _mapIndex;
  /**The first column position to constitute the index;
  The second parameter,  the unique name for a index;*/
  unordered_map<int, string> _mapIndexFirstField;
};

class TempTable : public BaseTable {};
} // namespace storage
