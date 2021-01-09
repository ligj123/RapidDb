#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include "../core/IndexType.h"
#include "Column.h"

namespace storage {
  using namespace std;

  class TableDesc
  {
  public:
    static const char* COLUMN_CONNECTOR_CHAR;
    static const char* NAME_PATTERN;
    static const char* PRIMARY_KEY;
    static void IsValidName(string name);

  public:
    TableDesc(string tableName, string description);
    TableDesc();
    ~TableDesc();
    const string GetTableName() const{ return _name; }
    void SetTableName(string name) {
      IsValidName(name);
      _name = name;
    }

		const string& GetDescription() const {	return _desc;	}
		const string GetPrimaryKey() const { return PRIMARY_KEY; }
		const vector<TableColumn*>& GetPrimaryKeyColumns() const { return _mapIndexColumn.at(PRIMARY_KEY);	}
		const unordered_map<string, vector<TableColumn*>>& GetMapIndexColumns() const {	return _mapIndexColumn;	}
    IndexType GetIndexType(string indexName) const;
		const unordered_map<string, IndexType>& GetMapIndexType() const { return _mapIndexType; }
		const vector<TableColumn*>& GetColumnArray() const {	return _vctColumn; }
		const unordered_map<string, string>& GetMapIndexName() const {	return _mapIndexName;	}
		const TableColumn* GetColumn(string fieldName) const;
    const TableColumn* GetColumn(int pos);
    unordered_map<string, size_t> GetMapColumnPos() { return _mapColumnPos; }
    unordered_map<string, string> GetIndexFirstFieldMap() { return _mapIndexFirstField; }
			
    void AddColumn(string& columnName, DataType dataType, bool nullable, uint32_t maxLen,
      string& comment, utils::Charsets charset, any& valDefault);
		void SetPrimaryKey(vector<string> priCols);
    void SetPrimaryKey(string priCol, uint32_t incStep);

    void AddSecondaryKey(string indexName, IndexType indexType, vector<string> colNames);

  protected:
    inline bool IsExistedColumn(string name) {
      return _mapColumnPos.find(name) != _mapColumnPos.end();
    }
  protected:
    /**Table name*/
    string _name;
    /**Table describer*/
    string _desc;
    /**Include all columns in this table, they will order by actual position in the table.*/
    vector<TableColumn*> _vctColumn;
    /** The map for column name and their position in column list */
    unordered_map<string, size_t> _mapColumnPos;
    /**The first parameter is the unique name for a index and the primary key's name is fixed, must be PRIMARY_KEY.
    The second parameter is the column(s)' name to constitute the index.*/
    unordered_map<string, string> _mapIndexName;
    /**The first parameter, the unique name for a index;
    The second parameter, the index type: PRIMARY, UNIQUE, NON_UNIQUE*/
    unordered_map<string, IndexType> _mapIndexType;
    /** The first parameter, the unique name for a index;
    The second parameter, the columns to constitute it.*/
    unordered_map<string, vector<TableColumn*>> _mapIndexColumn;
    /**The first column name to constitute the index;
    The second parameter,  the unique name for a index;*/
    unordered_map<string, string> _mapIndexFirstField;
  };
}
