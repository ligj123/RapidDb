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

  // statements
  ExprCreateDatabase *expr_create_db;
  ExprDropDatabase *expr_drop_db;
  ExprShowDatabases *expr_show_db;
  ExprUseDatabase *expr_use_db;
  ExprColumnInfo *expr_column_info;
  ExprConstraint *expr_constraint;
  ExprCreateTable *expr_create_table;
  ExprDropTable *expr_drop_table;
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
  
  ExprGroupItem *expr_group_item;
  ExprGroupBy *expr_group_by;
  ExprOrderTerm *expr_order_item;
  ExprOrderBy *expr_order_by;

  ExprStatement *expr_statement;
  ExprSelect *expr_select;
  ExprInsert *expr_insert;
  ExprUpdate *expr_update;
  ExprDelete *expr_delete;

  MVector<ExprColumnInfo*> *vct_col_info;
  MVector<ExprLogic*> *vct_logic;
  MVector<ExprGroupItem*> *vct_group_item
  MVector<ExprOrderTerm*> *vct_order_item
  MVector<ExprConstraint*> *vct_constraint;
}

    /*********************************
     ** Destructor symbols
     *********************************/
    // clang-format off
    %destructor { } <fval> <ival> <bval> <sval>
    %destructor {
      if ($$) {
        for (auto ptr : *($$)) {
          delete ptr;
        }
      }
      delete ($$);
    } <vct_col_info>
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
    %token DATABASE DATABASES

    %type <bval>  opt_not_exists opt_exists
    %type <expr_statement> expr_statement
    %type <expr_create_db> expr_create_db
    %type <expr_drop_db> expr_drop_db
    %type <expr_show_db> expr_show_db
    %type <expr_use_db> expr_use_db
    %type <expr_column_info> expr_column_info
    %type <ExprConstraint> expr_constraint
    %type <expr_create_table> expr_create_table
    %type <expr_drop_table> expr_drop_table
    %type <expr_show_table> expr_show_table
    %type <expr_trun_table> expr_trun_table
    %type <expr_select> expr_select
    %type <expr_insert> expr_insert
    %type <expr_update> expr_update
    %type <expr_delete> expr_delete
 
    %type <table_name> table_name

    %type <vct_col_info> vct_col_info
    /******************************
     ** Token Precedence and Associativity
     ** Precedence: lowest to highest
     ******************************/
    %left     OR
    %left     AND
    %right    NOT
    %nonassoc '=' EQUALS NOTEQUALS LIKE ILIKE
    %nonassoc '<' '>' LESS GREATER LESSEQ GREATEREQ

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
| expr_show_table { $$ = $1; }
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

expr_create_table : CREATE TABLE opt_not_exists table_name '(' vct_col_info ')'





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

vct_col_info : table_elem {
  $$ = new std::vector<TableElement*>();
  $$->push_back($1);
}
| table_elem_commalist ',' table_elem {
  $1->push_back($3);
  $$ = $1;
};

table_elem : column_def { $$ = $1; }
| table_constraint { $$ = $1; };

column_def : IDENTIFIER column_type opt_column_constraints {
  $$ = new ColumnDefinition($1, $2, $3);
  if (!$$->trySetNullableExplicit()) {
    yyerror(&yyloc, result, scanner, ("Conflicting nullability constraints for " + std::string{$1}).c_str());
  }
};

column_type : BIGINT { $$ = ColumnType{DataType::BIGINT}; }
| BOOLEAN { $$ = ColumnType{DataType::BOOLEAN}; }
| CHAR '(' INTVAL ')' { $$ = ColumnType{DataType::CHAR, $3}; }
| CHARACTER_VARYING '(' INTVAL ')' { $$ = ColumnType{DataType::VARCHAR, $3}; }
| DATE { $$ = ColumnType{DataType::DATE}; };
| DATETIME { $$ = ColumnType{DataType::DATETIME}; }
| DECIMAL opt_decimal_specification {
  $$ = ColumnType{DataType::DECIMAL, 0, $2->first, $2->second};
  delete $2;
}
| DOUBLE { $$ = ColumnType{DataType::DOUBLE}; }
| FLOAT { $$ = ColumnType{DataType::FLOAT}; }
| INT { $$ = ColumnType{DataType::INT}; }
| INTEGER { $$ = ColumnType{DataType::INT}; }
| LONG { $$ = ColumnType{DataType::LONG}; }
| REAL { $$ = ColumnType{DataType::REAL}; }
| SMALLINT { $$ = ColumnType{DataType::SMALLINT}; }
| TEXT { $$ = ColumnType{DataType::TEXT}; }
| TIME opt_time_precision { $$ = ColumnType{DataType::TIME, 0, $2}; }
| TIMESTAMP { $$ = ColumnType{DataType::DATETIME}; }
| VARCHAR '(' INTVAL ')' { $$ = ColumnType{DataType::VARCHAR, $3}; }



/******************************
 * Create Statement
 * CREATE TABLE students (name TEXT, student_number INTEGER, city TEXT, grade DOUBLE)
 * CREATE TABLE students FROM TBL FILE 'test/students.tbl'
 ******************************/
create_statement : CREATE TABLE opt_not_exists table_name FROM IDENTIFIER FILE file_path {
  $$ = new CreateStatement(kCreateTableFromTbl);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  if (strcasecmp($6, "tbl") != 0) {
    free($6);
    yyerror(&yyloc, result, scanner, "File type is unknown.");
    YYERROR;
  }
  free($6);
  $$->filePath = $8;
}
| CREATE TABLE opt_not_exists table_name '(' table_elem_commalist ')' {
  $$ = new CreateStatement(kCreateTable);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  $$->setColumnDefsAndConstraints($6);
  delete $6;
  if (result->errorMsg()) {
    delete $$;
    YYERROR;
  }
}
| CREATE TABLE opt_not_exists table_name AS select_statement {
  $$ = new CreateStatement(kCreateTable);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  $$->select = $6;
}
| CREATE INDEX opt_not_exists opt_index_name ON table_name '(' ident_commalist ')' {
  $$ = new CreateStatement(kCreateIndex);
  $$->indexName = $4;
  $$->ifNotExists = $3;
  $$->tableName = $6.name;
  $$->indexColumns = $8;
}
| CREATE VIEW opt_not_exists table_name opt_column_list AS select_statement {
  $$ = new CreateStatement(kCreateView);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  $$->viewColumns = $5;
  $$->select = $7;
};

opt_time_precision : '(' INTVAL ')' { $$ = $2; }
| /* empty */ { $$ = 0; };

opt_decimal_specification : '(' INTVAL ',' INTVAL ')' { $$ = new std::pair<int64_t, int64_t>{$2, $4}; }
| '(' INTVAL ')' { $$ = new std::pair<int64_t, int64_t>{$2, 0}; }
| /* empty */ { $$ = new std::pair<int64_t, int64_t>{0, 0}; };

opt_column_constraints : column_constraint_set { $$ = $1; }
| /* empty */ { $$ = new std::unordered_set<ConstraintType>(); };

column_constraint_set : column_constraint {
  $$ = new std::unordered_set<ConstraintType>();
  $$->insert($1);
}
| column_constraint_set column_constraint {
  $1->insert($2);
  $$ = $1;
}

column_constraint : PRIMARY KEY { $$ = ConstraintType::PrimaryKey; }
| UNIQUE { $$ = ConstraintType::Unique; }
| NULL { $$ = ConstraintType::Null; }
| NOT NULL { $$ = ConstraintType::NotNull; };

table_constraint : PRIMARY KEY '(' ident_commalist ')' { $$ = new TableConstraint(ConstraintType::PrimaryKey, $4); }
| UNIQUE '(' ident_commalist ')' { $$ = new TableConstraint(ConstraintType::Unique, $3); };

/******************************
 * Drop Statement
 * DROP TABLE students;
 * DEALLOCATE PREPARE stmt;
 ******************************/

drop_statement : DROP TABLE opt_exists table_name {
  $$ = new DropStatement(kDropTable);
  $$->ifExists = $3;
  $$->schema = $4.schema;
  $$->name = $4.name;
}
| DROP VIEW opt_exists table_name {
  $$ = new DropStatement(kDropView);
  $$->ifExists = $3;
  $$->schema = $4.schema;
  $$->name = $4.name;
}
| DEALLOCATE PREPARE IDENTIFIER {
  $$ = new DropStatement(kDropPreparedStatement);
  $$->ifExists = false;
  $$->name = $3;
}

| DROP INDEX opt_exists IDENTIFIER {
  $$ = new DropStatement(kDropIndex);
  $$->ifExists = $3;
  $$->indexName = $4;
};

/******************************
 * ALTER Statement
 * ALTER TABLE students DROP COLUMN name;
 ******************************/

alter_statement : ALTER TABLE opt_exists table_name alter_action {
  $$ = new AlterStatement($4.name, $5);
  $$->ifTableExists = $3;
  $$->schema = $4.schema;
};

alter_action : drop_action { $$ = $1; }

drop_action : DROP COLUMN opt_exists IDENTIFIER {
  $$ = new DropColumnAction($4);
  $$->ifExists = $3;
};

/******************************
 * Delete Statement / Truncate statement
 * DELETE FROM students WHERE grade > 3.0
 * DELETE FROM students <=> TRUNCATE students
 ******************************/
delete_statement : DELETE FROM table_name opt_where {
  $$ = new DeleteStatement();
  $$->schema = $3.schema;
  $$->tableName = $3.name;
  $$->expr = $4;
};

truncate_statement : TRUNCATE table_name {
  $$ = new DeleteStatement();
  $$->schema = $2.schema;
  $$->tableName = $2.name;
};

/******************************
 * Insert Statement
 * INSERT INTO students VALUES ('Max', 1112233, 'Musterhausen', 2.3)
 * INSERT INTO employees SELECT * FROM stundents
 ******************************/
insert_statement : INSERT INTO table_name opt_column_list VALUES '(' literal_list ')' {
  $$ = new InsertStatement(kInsertValues);
  $$->schema = $3.schema;
  $$->tableName = $3.name;
  $$->columns = $4;
  $$->values = $7;
}
| INSERT INTO table_name opt_column_list select_no_paren {
  $$ = new InsertStatement(kInsertSelect);
  $$->schema = $3.schema;
  $$->tableName = $3.name;
  $$->columns = $4;
  $$->select = $5;
};

opt_column_list : '(' ident_commalist ')' { $$ = $2; }
| /* empty */ { $$ = nullptr; };

/******************************
 * Update Statement
 * UPDATE students SET grade = 1.3, name='Felix Fürstenberg' WHERE name = 'Max Mustermann';
 ******************************/

update_statement : UPDATE table_ref_name_no_alias SET update_clause_commalist opt_where {
  $$ = new UpdateStatement();
  $$->table = $2;
  $$->updates = $4;
  $$->where = $5;
};

update_clause_commalist : update_clause {
  $$ = new std::vector<UpdateClause*>();
  $$->push_back($1);
}
| update_clause_commalist ',' update_clause {
  $1->push_back($3);
  $$ = $1;
};

update_clause : IDENTIFIER '=' expr {
  $$ = new UpdateClause();
  $$->column = $1;
  $$->value = $3;
};

/******************************
 * Select Statement
 ******************************/

select_statement : opt_with_clause select_with_paren {
  $$ = $2;
  $$->withDescriptions = $1;
}
| opt_with_clause select_no_paren {
  $$ = $2;
  $$->withDescriptions = $1;
}
| opt_with_clause select_with_paren set_operator select_within_set_operation opt_order opt_limit {
  $$ = $2;
  if ($$->setOperations == nullptr) {
    $$->setOperations = new std::vector<SetOperation*>();
  }
  $$->setOperations->push_back($3);
  $$->setOperations->back()->nestedSelectStatement = $4;
  $$->setOperations->back()->resultOrder = $5;
  $$->setOperations->back()->resultLimit = $6;
  $$->withDescriptions = $1;
};

select_within_set_operation : select_with_paren | select_within_set_operation_no_parentheses;

select_within_set_operation_no_parentheses : select_clause { $$ = $1; }
| select_clause set_operator select_within_set_operation {
  $$ = $1;
  if ($$->setOperations == nullptr) {
    $$->setOperations = new std::vector<SetOperation*>();
  }
  $$->setOperations->push_back($2);
  $$->setOperations->back()->nestedSelectStatement = $3;
};

select_with_paren : '(' select_no_paren ')' { $$ = $2; }
| '(' select_with_paren ')' { $$ = $2; };

select_no_paren : select_clause opt_order opt_limit opt_locking_clause {
  $$ = $1;
  $$->order = $2;

  // Limit could have been set by TOP.
  if ($3) {
    delete $$->limit;
    $$->limit = $3;
  }

  if ($4) {
    $$->lockings = $4;
  }
}
| select_clause set_operator select_within_set_operation opt_order opt_limit opt_locking_clause {
  $$ = $1;
  if ($$->setOperations == nullptr) {
    $$->setOperations = new std::vector<SetOperation*>();
  }
  $$->setOperations->push_back($2);
  $$->setOperations->back()->nestedSelectStatement = $3;
  $$->setOperations->back()->resultOrder = $4;
  $$->setOperations->back()->resultLimit = $5;
  $$->lockings = $6;
};

set_operator : set_type opt_all {
  $$ = $1;
  $$->isAll = $2;
};

set_type : UNION {
  $$ = new SetOperation();
  $$->setType = SetType::kSetUnion;
}
| INTERSECT {
  $$ = new SetOperation();
  $$->setType = SetType::kSetIntersect;
}
| EXCEPT {
  $$ = new SetOperation();
  $$->setType = SetType::kSetExcept;
};

opt_all : ALL { $$ = true; }
| /* empty */ { $$ = false; };

select_clause : SELECT opt_top opt_distinct select_list opt_from_clause opt_where opt_group {
  $$ = new SelectStatement();
  $$->limit = $2;
  $$->selectDistinct = $3;
  $$->selectList = $4;
  $$->fromTable = $5;
  $$->whereClause = $6;
  $$->groupBy = $7;
};

opt_distinct : DISTINCT { $$ = true; }
| /* empty */ { $$ = false; };

select_list : expr_list;

opt_from_clause : from_clause { $$ = $1; }
| /* empty */ { $$ = nullptr; };

from_clause : FROM table_ref { $$ = $2; };

opt_where : WHERE expr { $$ = $2; }
| /* empty */ { $$ = nullptr; };

opt_group : GROUP BY expr_list opt_having {
  $$ = new GroupByDescription();
  $$->columns = $3;
  $$->having = $4;
}
| /* empty */ { $$ = nullptr; };

opt_having : HAVING expr { $$ = $2; }
| /* empty */ { $$ = nullptr; };

opt_order : ORDER BY order_list { $$ = $3; }
| /* empty */ { $$ = nullptr; };

order_list : order_desc {
  $$ = new std::vector<OrderDescription*>();
  $$->push_back($1);
}
| order_list ',' order_desc {
  $1->push_back($3);
  $$ = $1;
};

order_desc : expr opt_order_type { $$ = new OrderDescription($2, $1); };

opt_order_type : ASC { $$ = kOrderAsc; }
| DESC { $$ = kOrderDesc; }
| /* empty */ { $$ = kOrderAsc; };

// TODO: TOP and LIMIT can take more than just int literals.

opt_top : TOP int_literal { $$ = new LimitDescription($2, nullptr); }
| /* empty */ { $$ = nullptr; };

opt_limit : LIMIT expr { $$ = new LimitDescription($2, nullptr); }
| OFFSET expr { $$ = new LimitDescription(nullptr, $2); }
| LIMIT expr OFFSET expr { $$ = new LimitDescription($2, $4); }
| LIMIT ALL { $$ = new LimitDescription(nullptr, nullptr); }
| LIMIT ALL OFFSET expr { $$ = new LimitDescription(nullptr, $4); }
| /* empty */ { $$ = nullptr; };

/******************************
 * Expressions
 ******************************/
expr_list : expr_alias {
  $$ = new std::vector<Expr*>();
  $$->push_back($1);
}
| expr_list ',' expr_alias {
  $1->push_back($3);
  $$ = $1;
};

opt_literal_list : literal_list { $$ = $1; }
| /* empty */ { $$ = nullptr; };

literal_list : literal {
  $$ = new std::vector<Expr*>();
  $$->push_back($1);
}
| literal_list ',' literal {
  $1->push_back($3);
  $$ = $1;
};

expr_alias : expr opt_alias {
  $$ = $1;
  if ($2) {
    $$->alias = strdup($2->name);
    delete $2;
  }
};

expr : operand | between_expr | logic_expr | exists_expr | in_expr;

operand : '(' expr ')' { $$ = $2; }
| array_index | scalar_expr | unary_expr | binary_expr | case_expr | function_expr | extract_expr | cast_expr |
    array_expr | '(' select_no_paren ')' {
  $$ = Expr::makeSelect($2);
};

scalar_expr : column_name | literal;

unary_expr : '-' operand { $$ = Expr::makeOpUnary(kOpUnaryMinus, $2); }
| NOT operand { $$ = Expr::makeOpUnary(kOpNot, $2); }
| operand ISNULL { $$ = Expr::makeOpUnary(kOpIsNull, $1); }
| operand IS NULL { $$ = Expr::makeOpUnary(kOpIsNull, $1); }
| operand IS NOT NULL { $$ = Expr::makeOpUnary(kOpNot, Expr::makeOpUnary(kOpIsNull, $1)); };

binary_expr : comp_expr | operand '-' operand { $$ = Expr::makeOpBinary($1, kOpMinus, $3); }
| operand '+' operand { $$ = Expr::makeOpBinary($1, kOpPlus, $3); }
| operand '/' operand { $$ = Expr::makeOpBinary($1, kOpSlash, $3); }
| operand '*' operand { $$ = Expr::makeOpBinary($1, kOpAsterisk, $3); }
| operand '%' operand { $$ = Expr::makeOpBinary($1, kOpPercentage, $3); }
| operand '^' operand { $$ = Expr::makeOpBinary($1, kOpCaret, $3); }
| operand LIKE operand { $$ = Expr::makeOpBinary($1, kOpLike, $3); }
| operand NOT LIKE operand { $$ = Expr::makeOpBinary($1, kOpNotLike, $4); }
| operand ILIKE operand { $$ = Expr::makeOpBinary($1, kOpILike, $3); }
| operand CONCAT operand { $$ = Expr::makeOpBinary($1, kOpConcat, $3); };

logic_expr : expr AND expr { $$ = Expr::makeOpBinary($1, kOpAnd, $3); }
| expr OR expr { $$ = Expr::makeOpBinary($1, kOpOr, $3); };

in_expr : operand IN '(' expr_list ')' { $$ = Expr::makeInOperator($1, $4); }
| operand NOT IN '(' expr_list ')' { $$ = Expr::makeOpUnary(kOpNot, Expr::makeInOperator($1, $5)); }
| operand IN '(' select_no_paren ')' { $$ = Expr::makeInOperator($1, $4); }
| operand NOT IN '(' select_no_paren ')' { $$ = Expr::makeOpUnary(kOpNot, Expr::makeInOperator($1, $5)); };

// CASE grammar based on: flex & bison by John Levine
// https://www.safaribooksonline.com/library/view/flex-bison/9780596805418/ch04.html#id352665
case_expr : CASE expr case_list END { $$ = Expr::makeCase($2, $3, nullptr); }
| CASE expr case_list ELSE expr END { $$ = Expr::makeCase($2, $3, $5); }
| CASE case_list END { $$ = Expr::makeCase(nullptr, $2, nullptr); }
| CASE case_list ELSE expr END { $$ = Expr::makeCase(nullptr, $2, $4); };

case_list : WHEN expr THEN expr { $$ = Expr::makeCaseList(Expr::makeCaseListElement($2, $4)); }
| case_list WHEN expr THEN expr { $$ = Expr::caseListAppend($1, Expr::makeCaseListElement($3, $5)); };

exists_expr : EXISTS '(' select_no_paren ')' { $$ = Expr::makeExists($3); }
| NOT EXISTS '(' select_no_paren ')' { $$ = Expr::makeOpUnary(kOpNot, Expr::makeExists($4)); };

comp_expr : operand '=' operand { $$ = Expr::makeOpBinary($1, kOpEquals, $3); }
| operand EQUALS operand { $$ = Expr::makeOpBinary($1, kOpEquals, $3); }
| operand NOTEQUALS operand { $$ = Expr::makeOpBinary($1, kOpNotEquals, $3); }
| operand '<' operand { $$ = Expr::makeOpBinary($1, kOpLess, $3); }
| operand '>' operand { $$ = Expr::makeOpBinary($1, kOpGreater, $3); }
| operand LESSEQ operand { $$ = Expr::makeOpBinary($1, kOpLessEq, $3); }
| operand GREATEREQ operand { $$ = Expr::makeOpBinary($1, kOpGreaterEq, $3); };

// `function_expr is used for window functions, aggregate expressions, and functions calls because we run into shift/
// reduce conflicts when splitting them.
function_expr : IDENTIFIER '(' ')' opt_window { $$ = Expr::makeFunctionRef($1, new std::vector<Expr*>(), false, $4); }
| IDENTIFIER '(' opt_distinct expr_list ')' opt_window { $$ = Expr::makeFunctionRef($1, $4, $3, $6); };

// Window function expressions, based on https://www.postgresql.org/docs/15/sql-expressions.html#SYNTAX-WINDOW-FUNCTIONS
// We do not support named windows, collations and exclusions (for simplicity) and filters (not part of the SQL standard).
opt_window : OVER '(' opt_partition opt_order opt_frame_clause ')' { $$ = new WindowDescription($3, $4, $5); }
| /* empty */ { $$ = nullptr; };

opt_partition : PARTITION BY expr_list { $$ = $3; }
| /* empty */ { $$ = nullptr; };

// We use the Postgres default if the frame end or the whole frame clause is omitted. "If `frame_end` is omitted, the
// end defaults to `CURRENT ROW`. [...] The default framing option is `RANGE UNBOUNDED PRECEDING`, which is the same as
// `RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW`."
opt_frame_clause : frame_type frame_bound { $$ = new FrameDescription{$1, $2, new FrameBound{0, kCurrentRow, false}}; }
| frame_type BETWEEN frame_bound AND frame_bound { $$ = new FrameDescription{$1, $3, $5}; }
| /* empty */ {
  $$ = new FrameDescription{kRange, new FrameBound{0, kPreceding, true}, new FrameBound{0, kCurrentRow, false}};
};

frame_type : RANGE { $$ = kRange; }
| ROWS { $$ = kRows; }
| GROUPS { $$ = kGroups; };

frame_bound : UNBOUNDED PRECEDING { $$ = new FrameBound{0, kPreceding, true}; }
| INTVAL PRECEDING { $$ = new FrameBound{$1, kPreceding, false}; }
| UNBOUNDED FOLLOWING { $$ = new FrameBound{0, kFollowing, true}; }
| INTVAL FOLLOWING { $$ = new FrameBound{$1, kFollowing, false}; }
| CURRENT_ROW { $$ = new FrameBound{0, kCurrentRow, false}; };

extract_expr : EXTRACT '(' datetime_field FROM expr ')' { $$ = Expr::makeExtract($3, $5); };

cast_expr : CAST '(' expr AS column_type ')' { $$ = Expr::makeCast($3, $5); };

datetime_field : SECOND { $$ = kDatetimeSecond; }
| MINUTE { $$ = kDatetimeMinute; }
| HOUR { $$ = kDatetimeHour; }
| DAY { $$ = kDatetimeDay; }
| MONTH { $$ = kDatetimeMonth; }
| YEAR { $$ = kDatetimeYear; };

datetime_field_plural : SECONDS { $$ = kDatetimeSecond; }
| MINUTES { $$ = kDatetimeMinute; }
| HOURS { $$ = kDatetimeHour; }
| DAYS { $$ = kDatetimeDay; }
| MONTHS { $$ = kDatetimeMonth; }
| YEARS { $$ = kDatetimeYear; };

duration_field : datetime_field | datetime_field_plural;

array_expr : ARRAY '[' expr_list ']' { $$ = Expr::makeArray($3); };

array_index : operand '[' int_literal ']' { $$ = Expr::makeArrayIndex($1, $3->ival); };

between_expr : operand BETWEEN operand AND operand { $$ = Expr::makeBetween($1, $3, $5); };

column_name : IDENTIFIER { $$ = Expr::makeColumnRef($1); }
| IDENTIFIER '.' IDENTIFIER { $$ = Expr::makeColumnRef($1, $3); }
| '*' { $$ = Expr::makeStar(); }
| IDENTIFIER '.' '*' { $$ = Expr::makeStar($1); };

literal : string_literal | bool_literal | num_literal | null_literal | date_literal | interval_literal | param_expr;

string_literal : STRING { $$ = Expr::makeLiteral($1); };

bool_literal : TRUE { $$ = Expr::makeLiteral(true); }
| FALSE { $$ = Expr::makeLiteral(false); };

num_literal : FLOATVAL { $$ = Expr::makeLiteral($1); }
| int_literal;

int_literal : INTVAL { $$ = Expr::makeLiteral($1); };

null_literal : NULL { $$ = Expr::makeNullLiteral(); };

date_literal : DATE STRING {
  int day{0}, month{0}, year{0}, chars_parsed{0};
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if (sscanf($2, "%4d-%2d-%2d%n", &day, &month, &year, &chars_parsed) != 3 || $2[chars_parsed] != 0) {
    free($2);
    yyerror(&yyloc, result, scanner, "Found incorrect date format. Expected format: YYYY-MM-DD");
    YYERROR;
  }
  $$ = Expr::makeDateLiteral($2);
};

interval_literal : int_literal duration_field {
  $$ = Expr::makeIntervalLiteral($1->ival, $2);
  delete $1;
}
| INTERVAL STRING datetime_field {
  int duration{0}, chars_parsed{0};
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if (sscanf($2, "%d%n", &duration, &chars_parsed) != 1 || $2[chars_parsed] != 0) {
    free($2);
    yyerror(&yyloc, result, scanner, "Found incorrect interval format. Expected format: INTEGER");
    YYERROR;
  }
  free($2);
  $$ = Expr::makeIntervalLiteral(duration, $3);
}
| INTERVAL STRING {
  int duration{0}, chars_parsed{0};
  // 'seconds' and 'minutes' are the longest accepted interval qualifiers (7 chars) + null byte
  char unit_string[8];
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if (sscanf($2, "%d %7s%n", &duration, unit_string, &chars_parsed) != 2 || $2[chars_parsed] != 0) {
    free($2);
    yyerror(&yyloc, result, scanner, "Found incorrect interval format. Expected format: INTEGER INTERVAL_QUALIIFIER");
    YYERROR;
  }
  free($2);

  DatetimeField unit;
  if (strcasecmp(unit_string, "second") == 0 || strcasecmp(unit_string, "seconds") == 0) {
    unit = kDatetimeSecond;
  } else if (strcasecmp(unit_string, "minute") == 0 || strcasecmp(unit_string, "minutes") == 0) {
    unit = kDatetimeMinute;
  } else if (strcasecmp(unit_string, "hour") == 0 || strcasecmp(unit_string, "hours") == 0) {
    unit = kDatetimeHour;
  } else if (strcasecmp(unit_string, "day") == 0 || strcasecmp(unit_string, "days") == 0) {
    unit = kDatetimeDay;
  } else if (strcasecmp(unit_string, "month") == 0 || strcasecmp(unit_string, "months") == 0) {
    unit = kDatetimeMonth;
  } else if (strcasecmp(unit_string, "year") == 0 || strcasecmp(unit_string, "years") == 0) {
    unit = kDatetimeYear;
  } else {
    yyerror(&yyloc, result, scanner, "Interval qualifier is unknown.");
    YYERROR;
  }
  $$ = Expr::makeIntervalLiteral(duration, unit);
};

param_expr : '?' {
  $$ = Expr::makeParameter(yylloc.total_column);
  $$->ival2 = yyloc.param_list.size();
  yyloc.param_list.push_back($$);
};

/******************************
 * Table
 ******************************/
table_ref : table_ref_atomic | table_ref_commalist ',' table_ref_atomic {
  $1->push_back($3);
  auto tbl = new TableRef(kTableCrossProduct);
  tbl->list = $1;
  $$ = tbl;
};

table_ref_atomic : nonjoin_table_ref_atomic | join_clause;

nonjoin_table_ref_atomic : table_ref_name | '(' select_statement ')' opt_table_alias {
  auto tbl = new TableRef(kTableSelect);
  tbl->select = $2;
  tbl->alias = $4;
  $$ = tbl;
};

table_ref_commalist : table_ref_atomic {
  $$ = new std::vector<TableRef*>();
  $$->push_back($1);
}
| table_ref_commalist ',' table_ref_atomic {
  $1->push_back($3);
  $$ = $1;
};

table_ref_name : table_name opt_table_alias {
  auto tbl = new TableRef(kTableName);
  tbl->schema = $1.schema;
  tbl->name = $1.name;
  tbl->alias = $2;
  $$ = tbl;
};

table_ref_name_no_alias : table_name {
  $$ = new TableRef(kTableName);
  $$->schema = $1.schema;
  $$->name = $1.name;
};

table_name : IDENTIFIER {
  $$.schema = nullptr;
  $$.name = $1;
}
| IDENTIFIER '.' IDENTIFIER {
  $$.schema = $1;
  $$.name = $3;
};

opt_index_name : IDENTIFIER { $$ = $1; }
| /* empty */ { $$ = nullptr; };

table_alias : alias | AS IDENTIFIER '(' ident_commalist ')' { $$ = new Alias($2, $4); };

opt_table_alias : table_alias | /* empty */ { $$ = nullptr; };

alias : AS IDENTIFIER { $$ = new Alias($2); }
| IDENTIFIER { $$ = new Alias($1); };

opt_alias : alias | /* empty */ { $$ = nullptr; };

/******************************
 * Row Locking Descriptions
 ******************************/

opt_locking_clause : opt_locking_clause_list { $$ = $1; }
| /* empty */ { $$ = nullptr; };

opt_locking_clause_list : locking_clause {
  $$ = new std::vector<LockingClause*>();
  $$->push_back($1);
}
| opt_locking_clause_list locking_clause {
  $1->push_back($2);
  $$ = $1;
};

locking_clause : FOR row_lock_mode opt_row_lock_policy {
  $$ = new LockingClause();
  $$->rowLockMode = $2;
  $$->rowLockWaitPolicy = $3;
  $$->tables = nullptr;
}
| FOR row_lock_mode OF ident_commalist opt_row_lock_policy {
  $$ = new LockingClause();
  $$->rowLockMode = $2;
  $$->tables = $4;
  $$->rowLockWaitPolicy = $5;
};

row_lock_mode : UPDATE { $$ = RowLockMode::ForUpdate; }
| NO KEY UPDATE { $$ = RowLockMode::ForNoKeyUpdate; }
| SHARE { $$ = RowLockMode::ForShare; }
| KEY SHARE { $$ = RowLockMode::ForKeyShare; };

opt_row_lock_policy : SKIP LOCKED { $$ = RowLockWaitPolicy::SkipLocked; }
| NOWAIT { $$ = RowLockWaitPolicy::NoWait; }
| /* empty */ { $$ = RowLockWaitPolicy::None; };

/******************************
 * With Descriptions
 ******************************/

opt_with_clause : with_clause | /* empty */ { $$ = nullptr; };

with_clause : WITH with_description_list { $$ = $2; };

with_description_list : with_description {
  $$ = new std::vector<WithDescription*>();
  $$->push_back($1);
}
| with_description_list ',' with_description {
  $1->push_back($3);
  $$ = $1;
};

with_description : IDENTIFIER AS select_with_paren {
  $$ = new WithDescription();
  $$->alias = $1;
  $$->select = $3;
};

/******************************
 * Join Statements
 ******************************/

join_clause : table_ref_atomic NATURAL JOIN nonjoin_table_ref_atomic {
  $$ = new TableRef(kTableJoin);
  $$->join = new JoinDefinition();
  $$->join->type = kJoinNatural;
  $$->join->left = $1;
  $$->join->right = $4;
}
| table_ref_atomic opt_join_type JOIN table_ref_atomic ON join_condition {
  $$ = new TableRef(kTableJoin);
  $$->join = new JoinDefinition();
  $$->join->type = (JoinType)$2;
  $$->join->left = $1;
  $$->join->right = $4;
  $$->join->condition = $6;
}
| table_ref_atomic opt_join_type JOIN table_ref_atomic USING '(' column_name ')' {
  $$ = new TableRef(kTableJoin);
  $$->join = new JoinDefinition();
  $$->join->type = (JoinType)$2;
  $$->join->left = $1;
  $$->join->right = $4;
  auto left_col = Expr::makeColumnRef(strdup($7->name));
  if ($7->alias) {
    left_col->alias = strdup($7->alias);
  }
  if ($1->getName()) {
    left_col->table = strdup($1->getName());
  }
  auto right_col = Expr::makeColumnRef(strdup($7->name));
  if ($7->alias) {
    right_col->alias = strdup($7->alias);
  }
  if ($4->getName()) {
    right_col->table = strdup($4->getName());
  }
  $$->join->condition = Expr::makeOpBinary(left_col, kOpEquals, right_col);
  delete $7;
};

opt_join_type : INNER { $$ = kJoinInner; }
| LEFT OUTER { $$ = kJoinLeft; }
| LEFT { $$ = kJoinLeft; }
| RIGHT OUTER { $$ = kJoinRight; }
| RIGHT { $$ = kJoinRight; }
| FULL OUTER { $$ = kJoinFull; }
| OUTER { $$ = kJoinFull; }
| FULL { $$ = kJoinFull; }
| CROSS { $$ = kJoinCross; }
| /* empty, default */ { $$ = kJoinInner; };

join_condition : expr;

/******************************
 * Misc
 ******************************/

opt_semicolon : ';' | /* empty */
    ;

ident_commalist : IDENTIFIER {
  $$ = new std::vector<char*>();
  $$->push_back($1);
}
| ident_commalist ',' IDENTIFIER {
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
