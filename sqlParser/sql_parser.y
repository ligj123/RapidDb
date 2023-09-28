// clang-format off
%{
  // clang-format on
  /**
 * sql_parser.y
 * defines sql_parser.h
 * outputs sql_parser.cpp
 */
  /*********************************
 ** Section 1: C Declarations
 *********************************/

#include "sql_parser.h"
#include "flex_lexer.h"

#include <stdio.h>
#include <string.h>

  int yyerror(YYLTYPE * llocp, ParserResult * result, yyscan_t scanner, const char* msg) {
    result->SetIsValid(false);
    result->SetErrorDetails(msg, llocp->first_line, llocp->first_column);
    return 0;
  }
  // clang-format off
%}
// clang-format on
/*********************************
 ** Section 2: SQL Parser Declarations
 *********************************/

// Specify code that is included in the generated .h and .c files
// clang-format off
%code requires {
// %code requires block

#include "../cache/Mallocator.h"
#include "../expr/BaseExpr.h"
#include "../expr/ExprAggr.h"
#include "../expr/ExprData.h"
#include "../expr/ExprDdl.h"
#include "../expr/ExprFunc.h"
#include "../expr/ExprLogic.h"
#include "../expr/ExprStatement.h"
#include "parser_typedef.h"
#include "ParserResult.h"

using namespace storage;

// Auto update column and line number
#define YY_USER_ACTION                        \
  yylloc->first_line = yylloc->last_line;     \
  yylloc->first_column = yylloc->last_column; \
  for (int i = 0; yytext[i] != '\0'; i++) {   \
    yylloc->total_column++;                   \
    yylloc->string_length++;                  \
    if (yytext[i] == '\n') {                  \
      yylloc->last_line++;                    \
      yylloc->last_column = 0;                \
    } else {                                  \
      yylloc->last_column++;                  \
    }                                         \
  }
}

// Define the names of the created files (defined in Makefile)
// %output  "bison_parser.cpp"
// %defines "bison_parser.h"

// Tell bison to create a reentrant parser
%define api.pure full

// Prefix the parser
%define api.prefix {db_}
%define api.token.prefix {SQL_}

%define parse.error verbose
%locations

%initial-action {
  // Initialize
  @$.first_column = 0;
  @$.last_column = 0;
  @$.first_line = 0;
  @$.last_line = 0;
  @$.total_column = 0;
  @$.string_length = 0;
};


// Define additional parameters for yylex (http://www.gnu.org/software/bison/manual/html_node/Pure-Calling.html)
%lex-param   { yyscan_t scanner }

// Define additional parameters for yyparse
%parse-param { ParserResult* result }
%parse-param { yyscan_t scanner }

/*********************************
 ** Define all data-types (http://www.gnu.org/software/bison/manual/html_node/Union-Decl.html)
 *********************************/
%union {
  // basic type
  bool bval;
  double fval;
  int64_t ival;
  uintmax_t uval;
  JoinType join_type;
  IndexType index_type;
  LockType lock_type;
  CompType comp_type;

  MString *sval;
  IDataValue *data_value;

  // parent for ExprData, ExprLogic and ExprAggr
  ExprElem *expr_elem;

  // ExprData
  ExprData *expr_data;
  ExprConst *expr_const;
  ExprField *expr_field;
  ExprParameter *expr_param;
  ExprAdd *expr_add;
  ExprSub *expr_sub;
  ExprMul *expr_mul;
  ExprDiv *expr_div;

  // ExprLogic
  ExprLogic *expr_logic;
  ExprComp *expr_cmp;
  ExprInNot *expr_in_not;
  ExprIsNullNot *expr_is_null_not;
  ExprBetween *expr_between;
  ExprLike *expr_like;
  ExprNot *expr_not;
  ExprAnd *expr_and;
  ExprOr * expr_or;

  // ExprAggr
  ExprAggr *expr_aggr;
  ExprCount *expr_count;
  ExprSum *expr_sum;
  ExprMax *expr_max;
  ExprMin *expr_min;
  ExprAvg *expr_avg;

  // Other elem
  ExprArray *expr_array;
  ExprColumn *expr_column;

  // Table elem
  ExprTable *expr_table;
  ExprCreateTableItem *expr_create_table_item;
  ExprDataType * expr_data_type;

  // Expr part
  ExprWhere *expr_where;
  ExprOn *expr_on;
  ExprHaving *expr_having;
  ExprLimit *expr_limit;
  ExprGroupBy *expr_group_by;
  ExprOrderItem *expr_order_item;
  ExprOrderBy *expr_order_by;

  // statements
  ExprStatement *expr_statement;
  //Expr DDL
  ExprCreateDatabase *expr_create_db;
  ExprDropDatabase *expr_drop_db;
  ExprShowDatabases *expr_show_db;
  ExprUseDatabase *expr_use_db;
  ExprCreateTable *expr_create_table;
  ExprDropTable *expr_drop_table;
  ExprShowTables *expr_show_tables;
  ExprTrunTable *expr_trun_table;
  ExprTransaction *expr_transaction;
  //Expr DML
  ExprSelect *expr_select;
  ExprInsert *expr_insert;
  ExprUpdate *expr_update;
  ExprDelete *expr_delete;

  //Expr vector
  MVectorPtr<MString*> *expr_vct_str;
  MVectorPtr<ExprColumn*> *expr_vct_column;
  MVectorPtr<ExprTable*> *expr_vct_table;
  MVectorPtr<ExprStatement*> * expr_vct_statement;
  MVectorPtr<ExprCreateTableItem*> *expr_vct_create_table_item;
  MVectorPtr<ExprOrderItem*> *expr_vct_order_item;
  MVectorPtr<ExprElem*> *expr_elem_row;
  MVectorPtr<MVectorPtr<ExprElem*>*> *expr_vct_elem_row;
  MVectorPtr<ExprData*> *expr_vct_data;
}

    /*********************************
     ** Destructor symbols
     *********************************/
    // clang-format off
    %destructor { } <fval> <ival> <bval> <join_type> <index_type> <lock_type> <comp_type>
    %destructor { if ($$ != nullptr) $$->DecRef(); } <data_value>
    %destructor { delete ($$); } <*>


    /*********************************
     ** Token Definition
     *********************************/
    %token <sval> IDENTIFIER STRING
    %token <fval> FLOATVAL
    %token <ival> INTVAL

    /* SQL Keywords */
    %token DEALLOCATE PARAMETERS INTERSECT TEMPORARY TIMESTAMP
    %token DISTINCT NVARCHAR RESTRICT TRUNCATE ANALYZE BETWEEN
    %token CASCADE COLUMNS CONTROL DEFAULT EXECUTE EXPLAIN
    %token INTEGER NATURAL PREPARE PRIMARY SCHEMAS CHARACTER_VARYING REAL DECIMAL SMALLINT BIGINT
    %token SPATIAL VARCHAR VIRTUAL DESCRIBE BEFORE COLUMN CREATE DELETE DIRECT
    %token DOUBLE ESCAPE EXCEPT EXISTS EXTRACT CAST FORMAT GLOBAL HAVING IMPORT
    %token INSERT ISNULL OFFSET RENAME SCHEMA SELECT SORTED
    %token TABLES UNIQUE UNLOAD UPDATE VALUES AFTER ALTER CROSS
    %token DELTA FLOAT GROUP INDEX INNER LIMIT LOCAL MERGE MINUS ORDER OVER
    %token OUTER RIGHT TABLE UNION USING WHERE CALL CASE CHAR COPY DATE DATETIME
    %token DESC DROP ELSE FILE FROM FULL HASH HINT INTO JOIN
    %token LEFT LIKE LOAD LONG NULL PARTITION PLAN SHOW TEXT THEN TIME
    %token VIEW WHEN WITH ADD ALL AND ASC END FOR INT KEY
    %token NOT OFF SET TOP AS BY IF IN IS OF ON OR TO NO
    %token ARRAY CONCAT ILIKE SECOND MINUTE HOUR DAY MONTH YEAR
    %token SECONDS MINUTES HOURS DAYS MONTHS YEARS INTERVAL
    %token TRUE FALSE BOOLEAN
    %token TRANSACTION BEGIN COMMIT ROLLBACK START
    %token NOWAIT SKIP LOCKED SHARE
    %token RANGE ROWS GROUPS UNBOUNDED FOLLOWING PRECEDING CURRENT_ROW
    %token DATABASE DATABASES AUTO_INCREMENT COMMENT UPSERT
    %token AVERAGE COUNT MIN MAX SUM USE

    // Basic data type
    %type <expr_data_type> expr_data_type
    %type <expr_vct_str> expr_vct_col_name
    %type <bval> opt_not_exists opt_exists col_nullable auto_increment opt_distinct opt_order_direction
    %type <sval> table_comment col_alias
    %type <join_type> join_type
    %type <comp_type> comp_type
    %type <index_type> index_type opt_index_type
    %type <lock_type> opt_lock_type
    
    // DDL
    %type <expr_statement> expr_statement
    %type <expr_create_db> expr_create_db
    %type <expr_drop_db> expr_drop_db
    %type <expr_show_db> expr_show_db
    %type <expr_use_db> expr_use_db
    %type <expr_create_table> expr_create_table
    %type <expr_drop_table> expr_drop_table
    %type <expr_show_tables> expr_show_tables
    %type <expr_trun_table> expr_trun_table
    %type <expr_transaction> expr_transaction

    // DML
    %type <expr_select> expr_select
    %type <expr_insert> expr_insert
    %type <expr_update> expr_update
    %type <expr_delete> expr_delete

    // DDL Item
    %type <expr_create_table_item> expr_create_table_item

    //DML Item
    %type <expr_where> opt_expr_where
    %type <expr_on> opt_expr_on
    %type <expr_group_by> opt_expr_group_by
    %type <expr_having> opt_expr_having
    %type <expr_order_item> expr_order_item
    %type <expr_order_by> opt_expr_order_by
    %type <expr_limit> opt_expr_limit
    %type <expr_column> expr_update_column expr_insert_column expr_select_column

    // Elem Type
    %type <data_value> const_dv const_int const_double const_string const_bool const_null default_col_dv
    %type <expr_elem> expr_elem
    %type <expr_data> expr_data expr_const expr_field expr_param expr_add expr_sub expr_mul expr_div expr_minus expr_func
    %type <expr_logic> expr_logic expr_cmp expr_in_not expr_is_null_not expr_between expr_like expr_not
    %type <expr_and> expr_and
    %type <expr_or> expr_or
    %type <expr_array> expr_array expr_vct_const
    %type <expr_aggr> expr_aggr expr_count expr_sum expr_max expr_min expr_avg
    %type <expr_table> expr_table  
    
    // expr vector
    %type <expr_elem_row> expr_elem_row
    %type <expr_vct_elem_row> expr_vct_elem_row
    %type <expr_vct_statement> statement_list
    %type <expr_vct_column> expr_vct_update_column opt_expr_vct_insert_column expr_vct_insert_column expr_vct_select_column opt_expr_vct_select_column
    %type <expr_vct_order_item> expr_vct_order_item
    %type <expr_vct_table> opt_expr_vct_table expr_vct_table
    %type <expr_vct_data> opt_expr_vct_data expr_vct_data
    %type <expr_vct_create_table_item> expr_vct_create_table_item

    /******************************
     ** Token Precedence and Associativity
     ** Precedence: lowest to highest
     ******************************/
    %left     OR
    %left     AND
    %right    NOT
    %nonassoc '=' EQ NE LIKE ILIKE
    %nonassoc '<' '>' LE GE

    %nonassoc NULL
    %nonassoc IS        /* sets precedence for IS NULL, etc */
    %left     '+' '-'
    %left     '*' '/' '%'
    %left     '^'
    %left     CONCAT

    /* Unary Operators */
    %right    UMINUS
    %left     '[' ']'
    %left     '(' ')'
    %left     '.'
    %left     JOIN
%%
/*********************************
  ** Section 3: Grammar Definition
*********************************/

// Defines our general input.

input : statement_list opt_semicolon {
  result->AddStatements($1);
  result->AddParameters(yyloc.param_list);
};

statement_list : expr_statement {
  yylloc.string_length = 0;
  $$ = new MVectorPtr<ExprStatement*>();
  $$->push_back($1);
}
| statement_list ';' expr_statement {
  yylloc.string_length = 0;
  $1->push_back($3);
  $$ = $1;
};

expr_statement : expr_create_db { $$ = $1; }
| expr_drop_db { $$ = $1; }
| expr_show_db { $$ = $1; }
| expr_use_db { $$ = $1; }
| expr_create_table { $$ = $1; }
| expr_drop_table { $$ = $1; }
| expr_show_tables { $$ = $1; }
| expr_trun_table { $$ = $1; }
| expr_select { $$ = $1; }
| expr_insert { $$ = $1; }
| expr_update { $$ = $1; }
| expr_delete { $$ = $1; }
| expr_transaction { $$ = $1; };

/******************************
 * Transaction Statement
 ******************************/

expr_create_db : CREATE DATABASE opt_not_exists IDENTIFIER {
  $$ = new ExprCreateDatabase($4, $3);
}
| CREATE SCHEMA opt_not_exists IDENTIFIER {
  $$ = new ExprCreateDatabase($4, $3);
};

expr_drop_db : DROP DATABASE opt_exists IDENTIFIER {
  $$ = new ExprDropDatabase($4, $3);
}
| DROP SCHEMA opt_exists IDENTIFIER {
  $$ = new ExprDropDatabase($4, $3);
};

expr_show_db : SHOW DATABASES {
  $$ = new ExprShowDatabases();
};

expr_use_db : USE IDENTIFIER {
  $$ = new ExprUseDatabase($2);
};

expr_create_table : CREATE TABLE opt_not_exists expr_table '(' expr_vct_create_table_item ')' {
  $$ = new ExprCreateTable($4, $3, $6);
};

expr_drop_table : DROP TABLE opt_exists expr_table {
  $$ = new ExprDropTable($4, $3);
};

expr_show_tables : SHOW TABLES FROM IDENTIFIER {
  $$ = new ExprShowTables($4);
}
| SHOW TABLES { $$ = new ExprShowTables(nullptr); }

expr_trun_table : TRUNCATE TABLE expr_table {
  $$ = new ExprTrunTable($3);
}

expr_transaction : BEGIN { $$ = new ExprTransaction(TranAction::TRAN_BEGIN); }
| START TRANSACTION { $$ = new ExprTransaction(TranAction::TRAN_BEGIN); }
| ROLLBACK { $$ = new ExprTransaction(TranAction::TRAN_ROLLBACK); }
| COMMIT { $$ = new ExprTransaction(TranAction::TRAN_COMMIT); };

expr_insert : INSERT INTO expr_table opt_expr_vct_insert_column VALUES expr_vct_elem_row {
  $$ = new ExprInsert();
  $$->_exprTable = $3;
  $$->_vctCol = $4;
  $$->_vctRowData = $6;
}
| INSERT INTO expr_table opt_expr_vct_insert_column expr_select {
  $$ = new ExprInsert();
  $$->_exprTable = $3;
  $$->_vctCol = $4;
  $$->_exprSelect = $5;
}
| UPSERT INTO expr_table opt_expr_vct_insert_column VALUES expr_vct_elem_row {
  $$ = new ExprInsert();
  $$->_exprTable = $3;
  $$->_vctCol = $4;
  $$->_vctRowData = $6;
  $$->_bUpsert = true;
};

expr_delete : DELETE FROM expr_table opt_expr_where opt_expr_order_by opt_expr_limit {
  $$ = new ExprDelete();
  $$->_exprTable = $3;
  $$->_exprWhere = $4;
  $$->_exprOrderBy = $5;
  $$->_exprLimit = $6;
};

expr_update : UPDATE expr_table SET expr_vct_update_column opt_expr_where opt_expr_order_by opt_expr_limit {
  $$ = new ExprUpdate();
  $$->_exprTable = $2;
  $$->_vctCol = $4;
  $$->_exprWhere = $5;
  $$->_exprOrderBy = $6;
  $$->_exprLimit = $7;
};

expr_select : SELECT opt_distinct opt_expr_vct_select_column opt_expr_vct_table opt_expr_where opt_expr_on opt_expr_group_by opt_expr_order_by opt_expr_limit opt_lock_type {
  $$ = new ExprSelect();
  $$->_bDistinct = $2;
  $$->_vctCol = $3;
  $$->_vctTable = $4;
  $$->_exprWhere = $5;
  $$->_exprOn = $6;
  $$->_exprGroupBy = $7;
  $$->_exprOrderBy = $8;
  $$->_exprLimit = $9;
  $$->_lockType = $10;
};

opt_expr_vct_select_column : expr_vct_select_column {
  $$ = $1;
}
| '*' {
   $$ = nullptr;
};

expr_vct_select_column : expr_select_column {
  $$ = new  MVectorPtr<ExprColumn*>();
  $$->push_back($1);
}
| expr_vct_select_column ',' expr_select_column {
  $1->push_back($3);
  $$ = $1;
};

expr_select_column : expr_elem col_alias {
  $$ = new ExprColumn(nullptr, $1, $2);
};

col_alias : AS IDENTIFIER { $$ = $2; }
| /* empty */ { $$ = nullptr; };

opt_expr_vct_insert_column : '(' expr_vct_insert_column ')' { $$ = $2; }
| /* empty */ { $$ = nullptr; };

expr_vct_insert_column : expr_insert_column {
  $$ = new  MVectorPtr<ExprColumn*>();
  $$->push_back($1);
}
| expr_vct_insert_column ',' expr_insert_column {
  $1->push_back($3);
  $$ = $1;
};

expr_insert_column : IDENTIFIER {
  $$ = new ExprColumn($1, nullptr, nullptr);
};

expr_vct_update_column : expr_update_column {
  $$ = new  MVectorPtr<ExprColumn*>();
  $$->push_back($1);
}
| expr_vct_update_column ',' expr_update_column {
  $1->push_back($3);
  $$ = $1;
};

expr_update_column : IDENTIFIER '=' expr_elem {
  $$ = new ExprColumn($1, $3, nullptr);
};

expr_table : IDENTIFIER {
  $$ = new ExprTable(nullptr, $1, nullptr);
}
| IDENTIFIER AS IDENTIFIER {
  $$ = new ExprTable(nullptr, $1, $3);
}
| IDENTIFIER '.' IDENTIFIER {
  $$ = new ExprTable($1, $3, nullptr);
}
| IDENTIFIER '.' IDENTIFIER AS IDENTIFIER {
  $$ = new ExprTable($1, $3, $5);
};

opt_not_exists : IF NOT EXISTS { $$ = true; }
| /* empty */ { $$ = false; };

opt_exists : IF EXISTS { $$ = true; }
| /* empty */ { $$ = false; };

opt_expr_vct_table : FROM expr_vct_table {
  $$ = $2;
}
| /* empty */ { $$ = nullptr; };

expr_vct_table : expr_table {
  $$ = new MVectorPtr<ExprTable*>();
  $$->push_back($1);
}
| opt_expr_vct_table join_type expr_table {
  $3->_joinType = $2;
  $1->push_back($3);
  $$ = $1;
};

join_type : INNER { $$ = JoinType::INNER_JOIN; }
| LEFT OUTER { $$ = JoinType::LEFT_JOIN; }
| LEFT { $$ = JoinType::LEFT_JOIN; }
| RIGHT OUTER { $$ = JoinType::RIGHT_JOIN; }
| RIGHT { $$ = JoinType::RIGHT_JOIN; }
| FULL OUTER { $$ = JoinType::OUTTER_JOIN; }
| OUTER { $$ = JoinType::OUTTER_JOIN; }
| FULL { $$ = JoinType::OUTTER_JOIN; }
| CROSS { $$ = JoinType::INNER_JOIN; }
| ',' { $$ = JoinType::INNER_JOIN; };

expr_vct_col_name : IDENTIFIER {
  $$ = new MVectorPtr<MString*>();
  $$->push_back($1);
}
| expr_vct_col_name ',' IDENTIFIER {
  $1->push_back($3);
  $$ = $1;
};

opt_semicolon : ';'
| /* empty */  ;

expr_vct_create_table_item : expr_create_table_item {
  $$ = new MVectorPtr<ExprCreateTableItem*>();
  $$->push_back($1);
}
| expr_vct_create_table_item ',' expr_create_table_item {
  $1->push_back($3);
  $$ = $1;
};

expr_create_table_item : IDENTIFIER expr_data_type col_nullable default_col_dv auto_increment opt_index_type table_comment {
  ExprColumnItem *item = new ExprColumnItem();
  item->_colName = $1;
  item->_dataType = $2->_dataType;
  item->_maxLength = $2->_maxLen;
  delete $2;
  item->_nullable = $3;
  item->_defaultVal = $4;
  item->_autoInc = $5;
  item->_indexType = $6;
  item->_comment = $7;
  $$ = item;
}
| index_type IDENTIFIER '(' expr_vct_col_name ')' {
  $$ = new ExprTableConstraint($2, $1, $4);
}
| index_type '(' expr_vct_col_name ')' {
  $$ = new ExprTableConstraint(nullptr, $1, $3);
};

expr_data_type : BIGINT { $$ = new ExprDataType(DataType::LONG); }
| BOOLEAN { $$ = new ExprDataType(DataType::BOOL); }
| CHAR '(' INTVAL ')' { $$ = new ExprDataType(DataType::FIXCHAR, $3); }
| DOUBLE { $$ = new ExprDataType(DataType::DOUBLE); }
| FLOAT { $$ = new ExprDataType(DataType::FLOAT); }
| INT { $$ = new ExprDataType(DataType::INT); }
| INTEGER { $$ = new ExprDataType(DataType::INT); }
| LONG { $$ = new ExprDataType(DataType::LONG); }
| REAL { $$ = new ExprDataType(DataType::DOUBLE); }
| SMALLINT { $$ = new ExprDataType(DataType::SHORT); }
| VARCHAR '(' INTVAL ')' { $$ = new ExprDataType(DataType::VARCHAR, $3); }

col_nullable : NULL { $$ = true; }
| NOT NULL { $$ = false; }
| /* empty */ { $$ = true; };

default_col_dv : DEFAULT const_dv { $$ = $2; }
| /* empty */ { $$ = nullptr; }

auto_increment : AUTO_INCREMENT { $$ = true; }
|  /* empty */ { $$ = false; };

opt_index_type : index_type { $$ = $1; }
| /* empty */ { $$ = IndexType::UNKNOWN; }

index_type : PRIMARY KEY { $$ = IndexType::PRIMARY; }
| PRIMARY { $$ = IndexType::PRIMARY; }
| UNIQUE KEY { $$ = IndexType::UNIQUE; }
| UNIQUE { $$ = IndexType::UNIQUE; }
| KEY { $$ = IndexType::NON_UNIQUE; }
| INDEX { $$ = IndexType::NON_UNIQUE; };

table_comment : COMMENT STRING { $$ = $2; }
|  /* empty */ { $$ = nullptr; };

expr_vct_elem_row : '(' expr_elem_row ')' {
  $$ = new MVectorPtr<MVectorPtr<ExprElem*>*>();
  $$->push_back($2);
}
| expr_vct_elem_row ',' '(' expr_elem_row ')' {
  $1->push_back($4);
  $$ = $1;
};

expr_elem_row : expr_elem {
  $$ = new MVectorPtr<ExprElem*>();
  $$->push_back($1);
}
| expr_elem_row ',' expr_elem {
  $1->push_back($3);
  $$ = $1;
};

opt_expr_where : WHERE expr_logic {
  $$ = new ExprWhere($2);
}
| /* empty */ { $$ = nullptr;};

opt_expr_on : ON expr_logic {
  $$ = new ExprOn($2);
}
| /* empty */ { $$ = nullptr; };

opt_expr_order_by : ORDER BY expr_vct_order_item {
  $$ = new ExprOrderBy($3);
}
| /* empty */ { $$ = nullptr; };

expr_vct_order_item : expr_order_item {
  $$ = new MVectorPtr<ExprOrderItem*>();
  $$->push_back($1);
}
| expr_vct_order_item ',' expr_order_item {
  $1->push_back($3);
  $$ = $1;
};

expr_order_item : IDENTIFIER opt_order_direction {
  $$ = new ExprOrderItem($1, $2);
};

opt_order_direction : ASC { $$ = true; }
| DESC { $$ = false; }
| /* empty */ { $$ = true; };

opt_expr_limit : LIMIT INTVAL {
  $$ = new ExprLimit(0, $2);
}
| LIMIT INTVAL ',' INTVAL {
  $$ = new ExprLimit($2, $4);
}
| /* empty */ { $$ = nullptr; };

opt_expr_group_by : GROUP BY expr_vct_col_name opt_expr_having {
  $$ = new ExprGroupBy($3, $4);
}
| /* empty */ { $$ = nullptr; };

opt_expr_having : HAVING expr_logic {
  $$ = new ExprHaving($2);
}
| /* empty */ { $$ = nullptr; };

opt_distinct : DISTINCT { $$ = true; }
| /* empty */ { $$ = false; };

opt_lock_type : FOR SHARE { $$ = LockType::SHARE_LOCK; }
| FOR UPDATE { $$ = LockType::WRITE_LOCK; }
| /* empty */ { $$ = LockType::NO_LOCK; };

expr_array : '(' expr_vct_const ')' { $$ = $2; };
expr_vct_const : const_dv {
  $$ = new ExprArray();
  $$->AddElem($1);
};
| expr_vct_const ',' const_dv {
   $1->AddElem($3);
   $$ = $1;
};

expr_elem : expr_logic { $$ = $1; }
| expr_data { $$ = $1; }
| expr_aggr { $$ = $1; };

expr_data : expr_const | expr_field | expr_param | expr_add | expr_sub | expr_mul | expr_div | expr_minus | expr_func
| '(' expr_data ')' { $$ = $2; };

expr_const : const_dv { $$ = new ExprConst($1); };

expr_field : IDENTIFIER { $$ = new ExprField(nullptr, $1); }
| IDENTIFIER '.' IDENTIFIER {$$ = new ExprField($1, $3);};

expr_param : '?' {
  ExprParameter *ep =  new ExprParameter();
  ep->_paraPos = yyloc.param_list.size();
  $$ = ep;
  yyloc.param_list.push_back(ep);
};

expr_add : expr_data '+' expr_data { $$ = new ExprAdd($1, $3); };

expr_sub : expr_data '-' expr_data { $$ = new ExprSub($1, $3); };

expr_mul : expr_data '*' expr_data { $$ = new ExprMul($1, $3); };

expr_div : expr_data '/' expr_data { $$ = new ExprDiv($1, $3); };

expr_minus : '-' expr_data { $$ = new ExprMinus($2); };

expr_func : IDENTIFIER '(' opt_expr_vct_data ')' {
  $$ = new ExprFunc($1, $3);
};

opt_expr_vct_data : expr_vct_data {
  $$ = $1;
}
| /* empty */ { $$ = nullptr; };

expr_vct_data : expr_data {
  $$ = new  MVectorPtr<ExprData*>();
  $$->push_back($1);
}
| expr_vct_data ',' expr_data {
  $1->push_back($3);
  $$ = $1;
};

const_dv : const_int | const_double | const_string | const_bool | const_null;
const_string : STRING {
  $$ = new DataValueVarChar($1->c_str(), $1->size());
  $$->SetConstRef();
  delete $1;
};

const_bool : TRUE {
  $$ = new DataValueBool(true);
  $$->SetConstRef();
}
| FALSE {
  $$ = new DataValueBool(false);
  $$->SetConstRef();
};

const_double : FLOATVAL {
  $$ = new DataValueDouble($1);
  $$->SetConstRef();
}

const_int : INTVAL {
  $$ = new DataValueLong($1);
  $$->SetConstRef();
};

const_null : NULL {
  $$ = new DataValueNull();
  $$->SetConstRef();
};

expr_logic : expr_cmp | expr_in_not | expr_is_null_not | expr_between | expr_like | expr_not
| expr_and { $$ = $1; }
| expr_or { $$ = $1; }
| '(' expr_logic ')' { $$ = $2; };

expr_cmp : expr_data comp_type expr_data {
  $$ = new ExprComp($2, $1, $3);
};

comp_type : '=' { $$ = CompType::EQ; }
| '>' { $$ = CompType::GT; }
| '<' { $$ = CompType::LT; }
| GE { $$ = CompType::GE; }
| LE { $$ = CompType::LE; }
| NE { $$ = CompType::NE; }
| EQ { $$ = CompType::EQ; }; 

expr_in_not : expr_data IN expr_array {
  $$ = new ExprInNot($1, $3, true);
}
| expr_data NOT IN expr_array {
  $$ = new ExprInNot($1, $4, false);
};

expr_is_null_not : expr_data IS NULL { $$ = new ExprIsNullNot($1, true); }
| expr_data IS NOT NULL { $$ = new ExprIsNullNot($1, false); };

expr_between : expr_data BETWEEN expr_data AND expr_data {
  $$ = new ExprBetween($1, $3, $5);
};

expr_like : expr_data LIKE const_string { $$ = new ExprLike($1, $3, true); }
| expr_data NOT LIKE const_string { $$ = new ExprLike($1, $4, false); };

expr_not : NOT expr_logic { $$ = new ExprNot($2); };

expr_and : expr_logic AND expr_logic {
  if ($1->GetType()== ExprType::EXPR_AND) {
    $$ = (ExprAnd*)$1;
    $$->_vctChild.push_back($3);
  } else {
    $$ = new ExprAnd();
    $$->_vctChild.push_back($1);
    $$->_vctChild.push_back($3);
  }
};

expr_or : expr_logic OR expr_logic {
  if ($1->GetType()== ExprType::EXPR_OR) {
    $$ = (ExprOr*)$1;
    $$->_vctChild.push_back($3);
  } else {
    $$ = new ExprOr();
    $$->_vctChild.push_back($1);
    $$->_vctChild.push_back($3);
  }
};

expr_aggr : expr_count | expr_sum | expr_max | expr_min | expr_avg 
| '(' expr_aggr ')' { $$ = $2; };

expr_count : COUNT '(' expr_data ')' {
  $$ = new ExprCount($3, false);
}
| COUNT '(' '*' ')' {
  $$ = new ExprCount(nullptr, true);
};

expr_sum : SUM '(' expr_data ')' {
  $$ = new ExprSum($3);
};

expr_max : MAX '(' expr_data ')' {
  $$ = new ExprMax($3);
};

expr_min : MIN '(' expr_data ')' {
  $$ = new ExprMin($3);
};

expr_avg : AVERAGE '(' expr_data ')' {
  $$ = new ExprAvg($3);
};

// clang-format off
%%
    // clang-format on
    /*********************************
 ** Section 4: Additional C code
 *********************************/

    /* empty */
