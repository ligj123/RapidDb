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

  using namespace storage;

  int yyerror(YYLTYPE * llocp, SQLParserResult * result, yyscan_t scanner, const char* msg) {
    result->setIsValid(false);
    result->setErrorDetails(strdup(msg), llocp->first_line, llocp->first_column);
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
#include "../expr/ExprDate.h"
#include "../expr/ExprDdl.h"
#include "../expr/ExprFunc.h"
#include "../expr/ExprLogic.h"
#include "../expr/ExprStatement.h"
#include "parser_typedef.h"

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
%parse-param { hsql::SQLParserResult* result }
%parse-param { yyscan_t scanner }

/*********************************
 ** Define all data-types (http://www.gnu.org/software/bison/manual/html_node/Union-Decl.html)
 *********************************/
%union {
  // clang-format on
  bool bval;
  storage::MString sval;
  double fval;
  int64_t ival;
  uintmax_t uval;
  JoinType join_type;

  // statements
  ExprCreateDatabase *expr_create_db;
  ExprDropDatabase *expr_drop_db;
  ExprShowDatabases *expr_show_db;
  ExprUseDatabase *expr_use_db;
  ExprTableElem *expr_table_elem;
  ExprColumnType expr_col_type;
  IDataValue *expr_data_val;
  ExprColumnInfo *expr_col_info;
  IndexType index_type;
  ExprConstraint *expr_constraint;
  ExprCreateTable *expr_create_table;
  ExprDropTables *expr_drop_tables;
  ExprShowTable *expr_show_table;
  ExprTrunTable *expr_trun_table;
  ExprTransaction *expr_transaction;

  ExprData *expr_data;
  ExprConst *expr_const;
  ExprField *expr_field;
  ExprParameter *expr_param;
  ExprAdd *expr_add;
  ExprSub *expr_sub;
  ExprMul *expr_mul;
  ExprDiv *expr_div;

  ExprAggr *expr_agr;
  ExprCount *expr_count;
  ExprSum *expr_sum;
  ExprMax *expr_max;
  ExprMin *expr_min;
  ExprAvg *expr_avg;

  ExprLogic *expr_logic;
  ExprComp *expr_cmp;
  ExprInNot *expr_in_not;
  ExprIsNullNot *expr_is_null_not;
  ExprBetween *expr_between;
  ExprLike *expr_like;
  ExprNot *expr_not;
  ExprAnd *expr_and;
  ExprOr * expr_or;

  ExprArray *expr_array;
  ExprTable *expr_table;
  ExprColumn *expr_column;
  ExprResColumn *expr_res_column;
  ExprJoinTable *expr_join_table;

  ExprWhere *expr_where;
  ExprOn *expr_on;
  ExprHaving *expr_having;
  ExprLimit *expr_limit;
  LockType lock_type;
  CompType comp_type;
  
  ExprGroupItem *expr_group_item;
  ExprGroupBy *expr_group_by;
  ExprOrderTerm *expr_order_item;
  ExprOrderBy *expr_order_by;

  ExprStatement *expr_statement;
  ExprSelect *expr_select;
  ExprInsert *expr_insert;
  ExprUpdate *expr_update;
  ExprDelete *expr_delete;

  MVectorPtr<ExprColumnInfo*> *expr_vct_col_info;
  MVectorPtr<ExprLogic*> *expr_vct_logic;
  MVectorPtr<ExprGroupItem*> *expr_vct_group_item
  MVectorPtr<ExprOrderTerm*> *expr_vct_order_item
  MVectorPtr<ExprTableElem*> *expr_vct_table_elem;
  MVectorPtr<MString> *vct_str;
  MVectorPtr<ExprData*> *expr_data_row;
  MVectorPtr<MVectorPtr<ExprData*>*> *expr_vct_data_row;
  MVectorPtr<ExprResColumn*> *expr_vct_res_col;
  MVectorPtr<ExprTable*> *opt_expr_vct_table;
}

    /*********************************
     ** Destructor symbols
     *********************************/
    // clang-format off
    %destructor { } <fval> <ival> <bval> <sval> <data_type> <expr_col_type>
    %destructor {
      if ($$) {
        for (auto ptr : *($$)) {
          delete ptr;
        }
      }
      delete ($$);
    }
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
    %token DATABASE DATABASES AUTO_INCREMENT COMMENT

    %type <data_type> data_type
    %type <vct_str> vct_col_name
    %type <bval> opt_not_exists opt_exists col_nullable auto_increment opt_distinct
    %type <sval> table_comment
    %type <join_type> join_type

    %type <expr_statement> expr_statement
    %type <expr_create_db> expr_create_db
    %type <expr_drop_db> expr_drop_db
    %type <expr_show_db> expr_show_db
    %type <expr_use_db> expr_use_db
    %type <expr_table_elem> expr_table_elem
    %type <expr_col_default> expr_col_default
    %type <expr_col_type> expr_col_type
    %type <expr_col_info> expr_col_info
    %type <expr_constraint> expr_constraint
    %type <expr_create_table> expr_create_table
    %type <expr_drop_table> expr_drop_table
    %type <expr_show_tables> expr_show_tables
    %type <expr_trun_table> expr_trun_table
    %type <expr_select> expr_select
    %type <expr_insert> expr_insert
    %type <expr_update> expr_update
    %type <expr_delete> expr_delete
    %type <index_type> index_type
    %type <expr_where> opt_expr_where
    %type <expr_on> opt_expr_on
    %type <expr_group_by> opt_expr_group_by
    %type <expr_Having> opt_expr_having
    %type <expr_limit> opt_expr_limit
    %type <lock_type> opt_lock_type
    %type <comp_type> comp_type

    %type <expr_logic> expr_logic
    %type <expr_cmp> expr_cmp
    %type <expr_in_not> expr_in_not
    %type <expr_is_null_not> expr_is_null_not
    %type <expr_between> expr_between
    %type <expr_like> expr_like
    %type <expr_not> expr_not
    %type <expr_and> expr_and
    %type <expr_or> expr_or
    %type <expr_array> expr_array
    %type <expr_vct_const> expr_vct_const

    %type <expr_data_val> const_dv const_int const_double const_string const_bool const_null
    %type <expr_data> expr_data expr_const expr_field expr_param expr_add expr_sub expr_mul expr_div expr_minus

    %type <expr_data_row> expr_data_row
    %type <expr_vct_data_row> expr_vct_data_row
    %type <table_name> table_name join_table_name

    %type <vct_table_elem> vct_table_elem
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
statement : expr_create_db { $$ = $1; }
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
| expr_transaction { $$ = $1; }

/******************************
 * Transaction Statement
 ******************************/

expr_transaction : BEGIN { $$ = new ExprTransaction(TranType::BEGIN); }
| START TRANSACTION { $$ = new ExprTransaction(TranType::BEGIN); }
| ROLLBACK { $$ = new ExprTransaction(TranType::ROLLBACK); }
| COMMIT { $$ = new ExprTransaction(TranType::COMMIT); };

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

expr_show_db : SHOW DATABASES { $$ = new ExprShowDatabases(); };
expr_use_db : USE IDENTIFIER { $$ = new ExprUseDatabase($2);}

expr_create_table : CREATE TABLE opt_not_exists table_name '(' vct_table_elem ')' {
  $$ = new ExprCreateTable($4, $3, $6);
};

expr_drop_table : DROP TABLE opt_exists table_name {
  $$ = new ExprDropTable($4, D3);
};

expr_show_tables : SHOW TABLES FROM IDENTIFIER {
  $$ = new ExprShowTables($4);
}
| SHOW TABLES { $$ = new ExprShowTables(str_null); }

expr_trun_table : TRUNCATE TABLE table_name {
  $$ = new ExprTrunTable($3);
}

expr_insert : INSERT INTO table_name '(' vct_col_name ')' VALUES expr_vct_data_row {
 $$ = new ExprInsert();
 $$->_exprTable = $3;
 $$->_vctCol = $5;
 $$->_vctRowData = $8;
};

expr_select : SELECT opt_distinct vct_res_col opt_expr_vct_table opt_expr_where opt_expr_group_by opt_expt_having opt_expr_limit opt_lock_type


opt_expr_where : WHERE expr_logic {
  $$ = new ExprWhere();
  $$->_exprLogic = $2;
}
| /* empty */ { $$ = nullptr;};

expr_logic : expr_cmp | expr_in_not | expr_is_null_not | expr_between | expr_like | expr_not | expr_and | expr_or
| '(' expr_logic ')' { $$ = $2; };

comp_type : '=' { $$ = CompType::EQ; }
| '>' { $$ = CompType::GT; }
| '<' { $$ = CompType::LT; }
| GE { $$ = CompType::GE; }
| LE { $$ = CompType::LE; }
| NE { $$ = CompType::NE; }
| EQ { $$ = CompType::EQ; }; 

expr_cmp : expr_data comp_type expr_data {
  $$ = new ExprComp($2, $1, $3);
};

expr_array : '(' expr_vct_const ')' { $$ = $2; }
expr_vct_const : const_dv {
  $$ = new ExprArray();
  $$->push_back($1);
};
| expr_vct_const ',' const_dv { $$->push_back($3); }

expr_in_not : expr_field IN expr_array {
  $$ = new ExprInNot($1, $3, true);
}
| expr_field NOT IN expr_array {
  $$ = new ExprInNot($1, $3, false);
};

expr_is_null_not : expr_data IS NULL { $$ = new ExprIsNullNot($1, true); }
| expr_data IS NOT NULL { $$ = new ExprIsNullNot($1, false); };


expr_between : expr_data BETWEEN expr_data AND expr_data {
  $$ = new ExprBetween($1, $3, $5);
};

expr_like : expr_data LIKE const_string { $$ = new ExprLike($1, $3, true); }
| expr_data NOT LIKE const_string { $$ = new ExprLike($1, $4, false); };

opt_expr_vct_table : table_name {
  $$ = new MVectorPtr<ExprTable>();
  $$->push_back($1);
}
| opt_expr_vct_table join_type table_name {
  $3->join_type = $2;
  $$->push_back($3);
};

join_type : INNER { $$ = INNER_JOIN; }
| LEFT OUTER { $$ = LEFT_JOIN; }
| LEFT { $$ = LEFT_JOIN; }
| RIGHT OUTER { $$ = RIGHT_JOIN; }
| RIGHT { $$ = RIGHT_JOIN; }
| FULL OUTER { $$ = OUTTER_JOIN; }
| OUTER { $$ = OUTTER_JOIN; }
| FULL { $$ = OUTTER_JOIN; }
| CROSS { $$ = INNER_JOIN; }
| ',' { $$ = INNER_JOIN; };

opt_distinct : DISTINCT { $$ = true; }
| /* empty */ { $$ = false; };

expr_vct_data_row : '(' expr_data_row ')' {
  $$ = new MVectorPtr<MVectorPtr<ExprData*>*>();
  $$->push_back($2);
}
| expr_vct_data_row ',' '(' expr_data_row ')' {
  $1->push_back($4);
  $$ = $1;
};

expr_data_row : expr_data {
  $$ = new MVectorPtr<ExprData*>();
  $$->push_back($1);
}
| expr_data_row ',' expr_data {
  $1->push_back($3);
  $$ = $1;
};

expr_data : expr_const | expr_field | expr_param | expr_add | expr_sub | expr_mul | expr_div | expr_minus
| '(' expr_data ')' { $$ = $2; };
expr_const : const_dv { $$ = new ExprConst($1); }
expr_field : IDENTIFIER { $$ = new ExprField(str_null, $1); }
| IDENTIFIER '.' IDENTIFIER {$$ = new ExprField($1, $3);};
expr_param : '?' { $$ =  new ExprParameter(); };
expr_add : expr_data '+' expr_data { $$ = new ExprAdd($1, $3); };
expr_sub : expr_data '-' expr_data { $$ = new ExprSub($1, $3); };
expr_mul : expr_data '*' expr_data { $$ = new ExprMul($1, $3); };
expr_div : expr_data '/' expr_data { $$ = new ExprDiv($1, $3); };
expr_minus : '-' expr_data { $$ = new ExprMinus($2); };


table_name : IDENTIFIER {
  $$ = new ExprTable(str_null, $1, str_null);
}
| IDENTIFIER AS IDENTIFIER {
  $$ = new ExprTable(str_null, $1, $3);
}
| IDENTIFIER '.' IDENTIFIER {
  $$ = new ExprTable($1, $3, str_null);
}
| IDENTIFIER '.' IDENTIFIER AS IDENTIFIER {
  $$ = new ExprTable($1, $3, $5);
};

opt_not_exists : IF NOT EXISTS { $$ = true; }
| /* empty */ { $$ = false; };

opt_exists : IF EXISTS { $$ = true; }
| /* empty */ { $$ = false; };

expr_vct_table_elem : expr_table_elem {
  $$ = new MVectorPtr<ExprTableElem*>();
  $$->push_back($1);
}
| expr_vct_table_elem ',' expr_table_elem {
  $1->push_back($3);
  $$ = $1;
};

expr_table_elem : expr_col_info { $$ = $1; }
| expr_constraint { $$ = $1; };

expr_col_info : IDENTIFIER expr_col_type col_nullable expr_col_default auto_increment index_type table_comment {
  $$ = new ColumnDefinition($1, $2, $3);
  if (!$$->trySetNullableExplicit()) {
    yyerror(&yyloc, result, scanner, ("Conflicting nullability constraints for " + std::string{$1}).c_str());
  }
};

expr_col_type : BIGINT { $$ = ExprColumnType(DataType::LONG); }
| BOOLEAN { $$ = ExprColumnType(DataType::BOOL); }
| CHAR '(' INTVAL ')' { $$ = ExprColumnType(DataType::FIXCHAR, $3); }
| DOUBLE { $$ = ExprColumnType(DataType::DOUBLE); }
| FLOAT { $$ = ExprColumnType(DataType::FLOAT); }
| INT { $$ = ExprColumnType(DataType::INT); }
| INTEGER { $$ = ExprColumnType(DataType::INT); }
| LONG { $$ = ExprColumnType(DataType::LONG); }
| REAL { $$ = ExprColumnType(DataType::DOUBLE); }
| SMALLINT { $$ = ExprColumnType(DataType::SHORT); }
| VARCHAR '(' INTVAL ')' { $$ = ExprColumnType(DataType::VARCHAR, $3); }

col_nullable : NULL { $$ = true; }
| NOT NULL { $$ = false; }
| /* empty */ { $$ = true; };

expr_col_default : DEFAULT const_dv { $$ = $2; }
| /* empty */ { $$ = DataValueNull(); }

const_dv : const_int | const_double | const_string | const_bool | const_null;
const_string : STRING { $$ = new DataValueVarChar($1, strlen($1)); };
const_bool : TRUE { $$ = new DataValueBool(true); }
| FALSE { $$ = new DataValueBool(false); };
const_double : FLOATVAL { $$ = new DataValueDouble($1); }
const_int : INTVAL { $$ = DataValueLong($1); };
const_null : NULL { $$ = DataValueNull(); };

auto_increment : AUTO_INCREMENT { $$ = true; }
|  /* empty */ { $$ = false; };

index_type : PRIMARY KEY { $$ = IndexType::PRIMARY; }
| PRIMARY { $$ = IndexType::PRIMARY; }
| UNIQUE KEY { $$ = IndexType::UNIQUE; }
| UNIQUE { $$ = IndexType::UNIQUE; }
| KEY { $$ = IndexType::NON_UNIQUE; }
| /* empty */ { $$ = IndexType::UNKNOWN; };

table_comment | COMMENT STRING { $$ = $2; }
|  /* empty */ { $$ = MString(); };


expr_constraint : INDEX IDENTIFIER IndexType '(' vct_col_name ')' {
  $$ = new ExprConstraint();
  $$->_idxName = $2;
  $$->_idxType = $3;
  $$->_vctCol = $5;
}
| PRIMARY KEY '(' vct_col_name ')' {
  $$ = new ExprConstraint();
  $$->_idxType = IndexType::PRIMARY;
  $$->_vctCol = $4;
};

vct_col_name : IDENTIFIER {
  $$ = new MVectorPtr<MString>();
  $$->push_back($1);
}
| vct_col_name ',' IDENTIFIER {
  $1->push_back($3);
  $$ = $1;
};



// clang-format off
%%
    // clang-format on
    /*********************************
 ** Section 4: Additional C code
 *********************************/

    /* empty */
