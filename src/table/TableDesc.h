#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include "IndexType.h"
#include "Column.h"

namespace storage {
  using namespace std;

  class TableDesc
  {
  public:
    static const char* COLUMN_SPLITE_CHAR;
    static const char* NAME_PATTERN;
    inline static void IsValidName(string name);
  public:
    TableDesc(string tableName, string description);
    TableDesc() {}
    const string GetTableName() const{ return _name; }
    void SetTableName(string name) {
      IsValidName(name);
      _name = name;
    }

		const string& GetDescription() const {	return _desc;	}
		const string GetPrimaryKey() const { return _primaryKey; }
		const vector<ColumnInTable*>& GetPrimaryKeyColumns() const { return _mapIndexColumn.at(_primaryKey);	}
		const unordered_map<string, vector<ColumnInTable*>>& GetMapIndexColumns() const {	return _mapIndexColumn;	}
    IndexType GetIndexType(string indexName) const;
		const unordered_map<string, IndexType>& GetMapIndexType() const { return _mapIndexType; }
		const vector<ColumnInTable*>& GetColumnArray() const {	return _arrColumn; }
		const unordered_map<string, string>& GetMapIndexName() const {	return _mapIndexName;	}
		const ColumnInTable* GetColumn(string fieldName) const;
    const ColumnInTable* GetColumn(int pos);
    unordered_map<string, int> GetMapColumnPos() { return _mapColumnPos; }
    unordered_map<string, string> GetIndexFirstFieldMap() { return _mapIndexFirstField; }
		TableDesc AddFixLenColumn(string columnName, DataType dataType, bool nullable, string comment, any valDefault);
			

		/*TableDesc addBlobColumn(String columnName, int maxLength, boolean nullable,
			String comment, byte[] valDefault);
		TableDesc addAutoPrimaryColumn(String columnName, DataType dataType, int step, String comment);
			

		TableDesc addCompressColumn(String columnName, DataType dataType, int maxLength, CompressionType cType,
			boolean nullable, String comment);

		TableDesc setPrimaryKey(String... priFields);

    TableDesc addSecondaryKey(String name, IndexType indexType, String... fieldNames);*/

  protected:
    string _name;
    string _desc;
    vector<ColumnInTable*> _arrColumn;
    /** The map for column name and their position in column list */
    unordered_map<string, int> _mapColumnPos;
    string _primaryKey;
    unordered_map<string, string> _mapIndexName;
    unordered_map<string, IndexType> _mapIndexType;
    /** The map for column's names connected by semicolon for the index and their columns */
    unordered_map<string, vector<ColumnInTable*>> _mapIndexColumn;
    /**The first fields for composite index <=> the composite index that include this field */
    unordered_map<string, string> _mapIndexFirstField;
  };
}
