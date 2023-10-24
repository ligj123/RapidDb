#include "ErrorMsg.h"
#include "BytesFuncs.h"
#include "ErrorID.h"
#include <fstream>
#include <iostream>
#include <regex>

namespace storage {
thread_local unique_ptr<ErrorMsg> _threadErrorMsg = nullptr;
unordered_map<int, MString> ErrorMsg::_mapErrorMsg = {
    {TB_INVALID_FILE_VERSION,
     "Failed to parse file version text {1}, it is not a valid version."},
    {TB_INVALID_TABLE_NAME, "Invalid table name, name = {1}."},
    {TB_REPEATED_COLUMN_NAME, "Repeated column name, name = {1}."},
    {TB_Array_MAX_LEN, "MaxLength must be large than 0 for no array data type. "
                       "columnName = {1}, dataType = {2}."},
    {TB_COLUMN_OVER_LENGTH, "Column's length exceed the limit. The limit size "
                            "= {1}, the actual size = {2}."},
    {TB_REPEATED_INDEX, "Try to add repeated index {1}."},
    {TB_UNEXIST_COLUMN, "Failed to find the column {1}."},
    {TB_INDEX_UNSUPPORT_DATA_TYPE, "Try to add index with unsupport data type, "
                                   "column name = {1}, data type = {2}."},
    {TB_INDEX_EMPTY_COLUMN,
     "Index should have one column at least. Index name = {1}."},
    {TB_ERROR_INDEX_VERSION, "Error index file version."},
    {TB_ERROR_RECORD_VERSION_STAMP,
     "Error record version, its value should be between {1} and {2}."},
    {TB_COLUMN_INDEX_OUT_RANGE, "column index is out of range, column "
                                "index={1}, column index range from 0 to {2}."},
    {TB_INVALID_COLUMN_NAME, "Invalid column name, name={1}."},
    {TB_INVALID_RESULT_SET,
     "Invalid result set, please call it after initalization."},
    {TB_COLUMN_UNNULLABLE, "The column {1} is be nullable."},

    // data type error
    {DT_UNSUPPORT_CONVERT, "Unsupport data type conversion from {1} to {2}."},
    {DT_INPUT_OVER_LENGTH, "Try to input over length value for column, max "
                           "length={1}, actual length={2}."},
    {DT_UNKNOWN_TYPE, "Unknown data type, data type = {1}."},
    {DT_UNSUPPORT_OPER, "Unsupport operation {1} for data type {2}. "},

    // Cache manage
    {CM_EXCEED_LIMIT, "Exceed the cache size limit."},
    {CM_FAILED_FIND_SIZE, "Failed to find the buffer cache with size = {1}."},

    // File operator
    {FILE_OPEN_FAILED, "Filed to open file, name = {1}."},

    // Core error
    {CORE_EXCEED_KEY_LENGTH,
     "The index key's length exceed the limit of configure. Length={1}."},
    {CORE_REPEATED_RECORD, "Try to insert repeated records into unique index."},

    // Expression
    {EXPR_INDEX_OUT_RANGE, "The index {1} is out of range of expression "
                           "parameter. Here is only {2} parameters."},
    {EXPR_ERROR_DATATYPE, "Input error datatype {1} for expression parameter, "
                          "expected datatype {2}."},
    {EXPR_EXCEED_MAX_LENGTH,
     "The length of parameter is {1}, exceed the max length {2}."},

    // Transaction
    {TRAN_ADD_TASK_FAILED,
     "Try to add task to a transaction with invalid status. ID={1}"},
    {TRAN_PARAM_INVALID,
     "The byte array for statement's parameters is invalid."},

    // Statement
    {STAT_PARAM_NUM_INVALID,
     "Invalid statement's parameters number, expect {1}, actual {2}."},
    {STAT_PARAM_LEN_INVALID,
     "Invalid statement's parameters values length, expect {1}, actual {2}."},

    // SQLParser
    {SQL_PARSER_INIT_FAILED, "SQLParser: Error when initializing lexer!"},
};

} // namespace storage
