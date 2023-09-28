/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_DB_SQL_PARSER_H_INCLUDED
# define YY_DB_SQL_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef DB_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define DB_DEBUG 1
#  else
#   define DB_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define DB_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined DB_DEBUG */
#if DB_DEBUG
extern int db_debug;
#endif
/* "%code requires" blocks.  */
#line 33 "sql_parser.y"

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

#line 89 "sql_parser.h"

/* Token kinds.  */
#ifndef DB_TOKENTYPE
# define DB_TOKENTYPE
  enum db_tokentype
  {
    SQL_DB_EMPTY = -2,
    SQL_YYEOF = 0,                 /* "end of file"  */
    SQL_DB_error = 256,            /* error  */
    SQL_DB_UNDEF = 257,            /* "invalid token"  */
    SQL_IDENTIFIER = 258,          /* IDENTIFIER  */
    SQL_STRING = 259,              /* STRING  */
    SQL_FLOATVAL = 260,            /* FLOATVAL  */
    SQL_INTVAL = 261,              /* INTVAL  */
    SQL_DEALLOCATE = 262,          /* DEALLOCATE  */
    SQL_PARAMETERS = 263,          /* PARAMETERS  */
    SQL_INTERSECT = 264,           /* INTERSECT  */
    SQL_TEMPORARY = 265,           /* TEMPORARY  */
    SQL_TIMESTAMP = 266,           /* TIMESTAMP  */
    SQL_DISTINCT = 267,            /* DISTINCT  */
    SQL_NVARCHAR = 268,            /* NVARCHAR  */
    SQL_RESTRICT = 269,            /* RESTRICT  */
    SQL_TRUNCATE = 270,            /* TRUNCATE  */
    SQL_ANALYZE = 271,             /* ANALYZE  */
    SQL_BETWEEN = 272,             /* BETWEEN  */
    SQL_CASCADE = 273,             /* CASCADE  */
    SQL_COLUMNS = 274,             /* COLUMNS  */
    SQL_CONTROL = 275,             /* CONTROL  */
    SQL_DEFAULT = 276,             /* DEFAULT  */
    SQL_EXECUTE = 277,             /* EXECUTE  */
    SQL_EXPLAIN = 278,             /* EXPLAIN  */
    SQL_INTEGER = 279,             /* INTEGER  */
    SQL_NATURAL = 280,             /* NATURAL  */
    SQL_PREPARE = 281,             /* PREPARE  */
    SQL_PRIMARY = 282,             /* PRIMARY  */
    SQL_SCHEMAS = 283,             /* SCHEMAS  */
    SQL_CHARACTER_VARYING = 284,   /* CHARACTER_VARYING  */
    SQL_REAL = 285,                /* REAL  */
    SQL_DECIMAL = 286,             /* DECIMAL  */
    SQL_SMALLINT = 287,            /* SMALLINT  */
    SQL_BIGINT = 288,              /* BIGINT  */
    SQL_SPATIAL = 289,             /* SPATIAL  */
    SQL_VARCHAR = 290,             /* VARCHAR  */
    SQL_VIRTUAL = 291,             /* VIRTUAL  */
    SQL_DESCRIBE = 292,            /* DESCRIBE  */
    SQL_BEFORE = 293,              /* BEFORE  */
    SQL_COLUMN = 294,              /* COLUMN  */
    SQL_CREATE = 295,              /* CREATE  */
    SQL_DELETE = 296,              /* DELETE  */
    SQL_DIRECT = 297,              /* DIRECT  */
    SQL_DOUBLE = 298,              /* DOUBLE  */
    SQL_ESCAPE = 299,              /* ESCAPE  */
    SQL_EXCEPT = 300,              /* EXCEPT  */
    SQL_EXISTS = 301,              /* EXISTS  */
    SQL_EXTRACT = 302,             /* EXTRACT  */
    SQL_CAST = 303,                /* CAST  */
    SQL_FORMAT = 304,              /* FORMAT  */
    SQL_GLOBAL = 305,              /* GLOBAL  */
    SQL_HAVING = 306,              /* HAVING  */
    SQL_IMPORT = 307,              /* IMPORT  */
    SQL_INSERT = 308,              /* INSERT  */
    SQL_ISNULL = 309,              /* ISNULL  */
    SQL_OFFSET = 310,              /* OFFSET  */
    SQL_RENAME = 311,              /* RENAME  */
    SQL_SCHEMA = 312,              /* SCHEMA  */
    SQL_SELECT = 313,              /* SELECT  */
    SQL_SORTED = 314,              /* SORTED  */
    SQL_TABLES = 315,              /* TABLES  */
    SQL_UNIQUE = 316,              /* UNIQUE  */
    SQL_UNLOAD = 317,              /* UNLOAD  */
    SQL_UPDATE = 318,              /* UPDATE  */
    SQL_VALUES = 319,              /* VALUES  */
    SQL_AFTER = 320,               /* AFTER  */
    SQL_ALTER = 321,               /* ALTER  */
    SQL_CROSS = 322,               /* CROSS  */
    SQL_DELTA = 323,               /* DELTA  */
    SQL_FLOAT = 324,               /* FLOAT  */
    SQL_GROUP = 325,               /* GROUP  */
    SQL_INDEX = 326,               /* INDEX  */
    SQL_INNER = 327,               /* INNER  */
    SQL_LIMIT = 328,               /* LIMIT  */
    SQL_LOCAL = 329,               /* LOCAL  */
    SQL_MERGE = 330,               /* MERGE  */
    SQL_MINUS = 331,               /* MINUS  */
    SQL_ORDER = 332,               /* ORDER  */
    SQL_OVER = 333,                /* OVER  */
    SQL_OUTER = 334,               /* OUTER  */
    SQL_RIGHT = 335,               /* RIGHT  */
    SQL_TABLE = 336,               /* TABLE  */
    SQL_UNION = 337,               /* UNION  */
    SQL_USING = 338,               /* USING  */
    SQL_WHERE = 339,               /* WHERE  */
    SQL_CALL = 340,                /* CALL  */
    SQL_CASE = 341,                /* CASE  */
    SQL_CHAR = 342,                /* CHAR  */
    SQL_COPY = 343,                /* COPY  */
    SQL_DATE = 344,                /* DATE  */
    SQL_DATETIME = 345,            /* DATETIME  */
    SQL_DESC = 346,                /* DESC  */
    SQL_DROP = 347,                /* DROP  */
    SQL_ELSE = 348,                /* ELSE  */
    SQL_FILE = 349,                /* FILE  */
    SQL_FROM = 350,                /* FROM  */
    SQL_FULL = 351,                /* FULL  */
    SQL_HASH = 352,                /* HASH  */
    SQL_HINT = 353,                /* HINT  */
    SQL_INTO = 354,                /* INTO  */
    SQL_JOIN = 355,                /* JOIN  */
    SQL_LEFT = 356,                /* LEFT  */
    SQL_LIKE = 357,                /* LIKE  */
    SQL_LOAD = 358,                /* LOAD  */
    SQL_LONG = 359,                /* LONG  */
    SQL_NULL = 360,                /* NULL  */
    SQL_PARTITION = 361,           /* PARTITION  */
    SQL_PLAN = 362,                /* PLAN  */
    SQL_SHOW = 363,                /* SHOW  */
    SQL_TEXT = 364,                /* TEXT  */
    SQL_THEN = 365,                /* THEN  */
    SQL_TIME = 366,                /* TIME  */
    SQL_VIEW = 367,                /* VIEW  */
    SQL_WHEN = 368,                /* WHEN  */
    SQL_WITH = 369,                /* WITH  */
    SQL_ADD = 370,                 /* ADD  */
    SQL_ALL = 371,                 /* ALL  */
    SQL_AND = 372,                 /* AND  */
    SQL_ASC = 373,                 /* ASC  */
    SQL_END = 374,                 /* END  */
    SQL_FOR = 375,                 /* FOR  */
    SQL_INT = 376,                 /* INT  */
    SQL_KEY = 377,                 /* KEY  */
    SQL_NOT = 378,                 /* NOT  */
    SQL_OFF = 379,                 /* OFF  */
    SQL_SET = 380,                 /* SET  */
    SQL_TOP = 381,                 /* TOP  */
    SQL_AS = 382,                  /* AS  */
    SQL_BY = 383,                  /* BY  */
    SQL_IF = 384,                  /* IF  */
    SQL_IN = 385,                  /* IN  */
    SQL_IS = 386,                  /* IS  */
    SQL_OF = 387,                  /* OF  */
    SQL_ON = 388,                  /* ON  */
    SQL_OR = 389,                  /* OR  */
    SQL_TO = 390,                  /* TO  */
    SQL_NO = 391,                  /* NO  */
    SQL_ARRAY = 392,               /* ARRAY  */
    SQL_CONCAT = 393,              /* CONCAT  */
    SQL_ILIKE = 394,               /* ILIKE  */
    SQL_SECOND = 395,              /* SECOND  */
    SQL_MINUTE = 396,              /* MINUTE  */
    SQL_HOUR = 397,                /* HOUR  */
    SQL_DAY = 398,                 /* DAY  */
    SQL_MONTH = 399,               /* MONTH  */
    SQL_YEAR = 400,                /* YEAR  */
    SQL_SECONDS = 401,             /* SECONDS  */
    SQL_MINUTES = 402,             /* MINUTES  */
    SQL_HOURS = 403,               /* HOURS  */
    SQL_DAYS = 404,                /* DAYS  */
    SQL_MONTHS = 405,              /* MONTHS  */
    SQL_YEARS = 406,               /* YEARS  */
    SQL_INTERVAL = 407,            /* INTERVAL  */
    SQL_TRUE = 408,                /* TRUE  */
    SQL_FALSE = 409,               /* FALSE  */
    SQL_BOOLEAN = 410,             /* BOOLEAN  */
    SQL_TRANSACTION = 411,         /* TRANSACTION  */
    SQL_BEGIN = 412,               /* BEGIN  */
    SQL_COMMIT = 413,              /* COMMIT  */
    SQL_ROLLBACK = 414,            /* ROLLBACK  */
    SQL_START = 415,               /* START  */
    SQL_NOWAIT = 416,              /* NOWAIT  */
    SQL_SKIP = 417,                /* SKIP  */
    SQL_LOCKED = 418,              /* LOCKED  */
    SQL_SHARE = 419,               /* SHARE  */
    SQL_RANGE = 420,               /* RANGE  */
    SQL_ROWS = 421,                /* ROWS  */
    SQL_GROUPS = 422,              /* GROUPS  */
    SQL_UNBOUNDED = 423,           /* UNBOUNDED  */
    SQL_FOLLOWING = 424,           /* FOLLOWING  */
    SQL_PRECEDING = 425,           /* PRECEDING  */
    SQL_CURRENT_ROW = 426,         /* CURRENT_ROW  */
    SQL_DATABASE = 427,            /* DATABASE  */
    SQL_DATABASES = 428,           /* DATABASES  */
    SQL_AUTO_INCREMENT = 429,      /* AUTO_INCREMENT  */
    SQL_COMMENT = 430,             /* COMMENT  */
    SQL_UPSERT = 431,              /* UPSERT  */
    SQL_AVERAGE = 432,             /* AVERAGE  */
    SQL_COUNT = 433,               /* COUNT  */
    SQL_MIN = 434,                 /* MIN  */
    SQL_MAX = 435,                 /* MAX  */
    SQL_SUM = 436,                 /* SUM  */
    SQL_USE = 437,                 /* USE  */
    SQL_EQ = 438,                  /* EQ  */
    SQL_NE = 439,                  /* NE  */
    SQL_LE = 440,                  /* LE  */
    SQL_GE = 441,                  /* GE  */
    SQL_UMINUS = 442               /* UMINUS  */
  };
  typedef enum db_tokentype db_token_kind_t;
#endif

/* Value type.  */
#if ! defined DB_STYPE && ! defined DB_STYPE_IS_DECLARED
union DB_STYPE
{
#line 100 "sql_parser.y"

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

#line 387 "sql_parser.h"

};
typedef union DB_STYPE DB_STYPE;
# define DB_STYPE_IS_TRIVIAL 1
# define DB_STYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined DB_LTYPE && ! defined DB_LTYPE_IS_DECLARED
typedef struct DB_LTYPE DB_LTYPE;
struct DB_LTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define DB_LTYPE_IS_DECLARED 1
# define DB_LTYPE_IS_TRIVIAL 1
#endif




int db_parse (ParserResult* result, yyscan_t scanner);


#endif /* !YY_DB_SQL_PARSER_H_INCLUDED  */
