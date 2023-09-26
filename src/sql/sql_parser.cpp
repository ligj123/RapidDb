/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         DB_STYPE
#define YYLTYPE         DB_LTYPE
/* Substitute the variable and function names.  */
#define yyparse         db_parse
#define yylex           db_lex
#define yyerror         db_error
#define yydebug         db_debug
#define yynerrs         db_nerrs

/* First part of user prologue.  */
#line 2 "sql_parser.y"

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

#line 103 "sql_parser.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "sql_parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENTIFIER = 3,                 /* IDENTIFIER  */
  YYSYMBOL_STRING = 4,                     /* STRING  */
  YYSYMBOL_FLOATVAL = 5,                   /* FLOATVAL  */
  YYSYMBOL_INTVAL = 6,                     /* INTVAL  */
  YYSYMBOL_DEALLOCATE = 7,                 /* DEALLOCATE  */
  YYSYMBOL_PARAMETERS = 8,                 /* PARAMETERS  */
  YYSYMBOL_INTERSECT = 9,                  /* INTERSECT  */
  YYSYMBOL_TEMPORARY = 10,                 /* TEMPORARY  */
  YYSYMBOL_TIMESTAMP = 11,                 /* TIMESTAMP  */
  YYSYMBOL_DISTINCT = 12,                  /* DISTINCT  */
  YYSYMBOL_NVARCHAR = 13,                  /* NVARCHAR  */
  YYSYMBOL_RESTRICT = 14,                  /* RESTRICT  */
  YYSYMBOL_TRUNCATE = 15,                  /* TRUNCATE  */
  YYSYMBOL_ANALYZE = 16,                   /* ANALYZE  */
  YYSYMBOL_BETWEEN = 17,                   /* BETWEEN  */
  YYSYMBOL_CASCADE = 18,                   /* CASCADE  */
  YYSYMBOL_COLUMNS = 19,                   /* COLUMNS  */
  YYSYMBOL_CONTROL = 20,                   /* CONTROL  */
  YYSYMBOL_DEFAULT = 21,                   /* DEFAULT  */
  YYSYMBOL_EXECUTE = 22,                   /* EXECUTE  */
  YYSYMBOL_EXPLAIN = 23,                   /* EXPLAIN  */
  YYSYMBOL_INTEGER = 24,                   /* INTEGER  */
  YYSYMBOL_NATURAL = 25,                   /* NATURAL  */
  YYSYMBOL_PREPARE = 26,                   /* PREPARE  */
  YYSYMBOL_PRIMARY = 27,                   /* PRIMARY  */
  YYSYMBOL_SCHEMAS = 28,                   /* SCHEMAS  */
  YYSYMBOL_CHARACTER_VARYING = 29,         /* CHARACTER_VARYING  */
  YYSYMBOL_REAL = 30,                      /* REAL  */
  YYSYMBOL_DECIMAL = 31,                   /* DECIMAL  */
  YYSYMBOL_SMALLINT = 32,                  /* SMALLINT  */
  YYSYMBOL_BIGINT = 33,                    /* BIGINT  */
  YYSYMBOL_SPATIAL = 34,                   /* SPATIAL  */
  YYSYMBOL_VARCHAR = 35,                   /* VARCHAR  */
  YYSYMBOL_VIRTUAL = 36,                   /* VIRTUAL  */
  YYSYMBOL_DESCRIBE = 37,                  /* DESCRIBE  */
  YYSYMBOL_BEFORE = 38,                    /* BEFORE  */
  YYSYMBOL_COLUMN = 39,                    /* COLUMN  */
  YYSYMBOL_CREATE = 40,                    /* CREATE  */
  YYSYMBOL_DELETE = 41,                    /* DELETE  */
  YYSYMBOL_DIRECT = 42,                    /* DIRECT  */
  YYSYMBOL_DOUBLE = 43,                    /* DOUBLE  */
  YYSYMBOL_ESCAPE = 44,                    /* ESCAPE  */
  YYSYMBOL_EXCEPT = 45,                    /* EXCEPT  */
  YYSYMBOL_EXISTS = 46,                    /* EXISTS  */
  YYSYMBOL_EXTRACT = 47,                   /* EXTRACT  */
  YYSYMBOL_CAST = 48,                      /* CAST  */
  YYSYMBOL_FORMAT = 49,                    /* FORMAT  */
  YYSYMBOL_GLOBAL = 50,                    /* GLOBAL  */
  YYSYMBOL_HAVING = 51,                    /* HAVING  */
  YYSYMBOL_IMPORT = 52,                    /* IMPORT  */
  YYSYMBOL_INSERT = 53,                    /* INSERT  */
  YYSYMBOL_ISNULL = 54,                    /* ISNULL  */
  YYSYMBOL_OFFSET = 55,                    /* OFFSET  */
  YYSYMBOL_RENAME = 56,                    /* RENAME  */
  YYSYMBOL_SCHEMA = 57,                    /* SCHEMA  */
  YYSYMBOL_SELECT = 58,                    /* SELECT  */
  YYSYMBOL_SORTED = 59,                    /* SORTED  */
  YYSYMBOL_TABLES = 60,                    /* TABLES  */
  YYSYMBOL_UNIQUE = 61,                    /* UNIQUE  */
  YYSYMBOL_UNLOAD = 62,                    /* UNLOAD  */
  YYSYMBOL_UPDATE = 63,                    /* UPDATE  */
  YYSYMBOL_VALUES = 64,                    /* VALUES  */
  YYSYMBOL_AFTER = 65,                     /* AFTER  */
  YYSYMBOL_ALTER = 66,                     /* ALTER  */
  YYSYMBOL_CROSS = 67,                     /* CROSS  */
  YYSYMBOL_DELTA = 68,                     /* DELTA  */
  YYSYMBOL_FLOAT = 69,                     /* FLOAT  */
  YYSYMBOL_GROUP = 70,                     /* GROUP  */
  YYSYMBOL_INDEX = 71,                     /* INDEX  */
  YYSYMBOL_INNER = 72,                     /* INNER  */
  YYSYMBOL_LIMIT = 73,                     /* LIMIT  */
  YYSYMBOL_LOCAL = 74,                     /* LOCAL  */
  YYSYMBOL_MERGE = 75,                     /* MERGE  */
  YYSYMBOL_MINUS = 76,                     /* MINUS  */
  YYSYMBOL_ORDER = 77,                     /* ORDER  */
  YYSYMBOL_OVER = 78,                      /* OVER  */
  YYSYMBOL_OUTER = 79,                     /* OUTER  */
  YYSYMBOL_RIGHT = 80,                     /* RIGHT  */
  YYSYMBOL_TABLE = 81,                     /* TABLE  */
  YYSYMBOL_UNION = 82,                     /* UNION  */
  YYSYMBOL_USING = 83,                     /* USING  */
  YYSYMBOL_WHERE = 84,                     /* WHERE  */
  YYSYMBOL_CALL = 85,                      /* CALL  */
  YYSYMBOL_CASE = 86,                      /* CASE  */
  YYSYMBOL_CHAR = 87,                      /* CHAR  */
  YYSYMBOL_COPY = 88,                      /* COPY  */
  YYSYMBOL_DATE = 89,                      /* DATE  */
  YYSYMBOL_DATETIME = 90,                  /* DATETIME  */
  YYSYMBOL_DESC = 91,                      /* DESC  */
  YYSYMBOL_DROP = 92,                      /* DROP  */
  YYSYMBOL_ELSE = 93,                      /* ELSE  */
  YYSYMBOL_FILE = 94,                      /* FILE  */
  YYSYMBOL_FROM = 95,                      /* FROM  */
  YYSYMBOL_FULL = 96,                      /* FULL  */
  YYSYMBOL_HASH = 97,                      /* HASH  */
  YYSYMBOL_HINT = 98,                      /* HINT  */
  YYSYMBOL_INTO = 99,                      /* INTO  */
  YYSYMBOL_JOIN = 100,                     /* JOIN  */
  YYSYMBOL_LEFT = 101,                     /* LEFT  */
  YYSYMBOL_LIKE = 102,                     /* LIKE  */
  YYSYMBOL_LOAD = 103,                     /* LOAD  */
  YYSYMBOL_LONG = 104,                     /* LONG  */
  YYSYMBOL_NULL = 105,                     /* NULL  */
  YYSYMBOL_PARTITION = 106,                /* PARTITION  */
  YYSYMBOL_PLAN = 107,                     /* PLAN  */
  YYSYMBOL_SHOW = 108,                     /* SHOW  */
  YYSYMBOL_TEXT = 109,                     /* TEXT  */
  YYSYMBOL_THEN = 110,                     /* THEN  */
  YYSYMBOL_TIME = 111,                     /* TIME  */
  YYSYMBOL_VIEW = 112,                     /* VIEW  */
  YYSYMBOL_WHEN = 113,                     /* WHEN  */
  YYSYMBOL_WITH = 114,                     /* WITH  */
  YYSYMBOL_ADD = 115,                      /* ADD  */
  YYSYMBOL_ALL = 116,                      /* ALL  */
  YYSYMBOL_AND = 117,                      /* AND  */
  YYSYMBOL_ASC = 118,                      /* ASC  */
  YYSYMBOL_END = 119,                      /* END  */
  YYSYMBOL_FOR = 120,                      /* FOR  */
  YYSYMBOL_INT = 121,                      /* INT  */
  YYSYMBOL_KEY = 122,                      /* KEY  */
  YYSYMBOL_NOT = 123,                      /* NOT  */
  YYSYMBOL_OFF = 124,                      /* OFF  */
  YYSYMBOL_SET = 125,                      /* SET  */
  YYSYMBOL_TOP = 126,                      /* TOP  */
  YYSYMBOL_AS = 127,                       /* AS  */
  YYSYMBOL_BY = 128,                       /* BY  */
  YYSYMBOL_IF = 129,                       /* IF  */
  YYSYMBOL_IN = 130,                       /* IN  */
  YYSYMBOL_IS = 131,                       /* IS  */
  YYSYMBOL_OF = 132,                       /* OF  */
  YYSYMBOL_ON = 133,                       /* ON  */
  YYSYMBOL_OR = 134,                       /* OR  */
  YYSYMBOL_TO = 135,                       /* TO  */
  YYSYMBOL_NO = 136,                       /* NO  */
  YYSYMBOL_ARRAY = 137,                    /* ARRAY  */
  YYSYMBOL_CONCAT = 138,                   /* CONCAT  */
  YYSYMBOL_ILIKE = 139,                    /* ILIKE  */
  YYSYMBOL_SECOND = 140,                   /* SECOND  */
  YYSYMBOL_MINUTE = 141,                   /* MINUTE  */
  YYSYMBOL_HOUR = 142,                     /* HOUR  */
  YYSYMBOL_DAY = 143,                      /* DAY  */
  YYSYMBOL_MONTH = 144,                    /* MONTH  */
  YYSYMBOL_YEAR = 145,                     /* YEAR  */
  YYSYMBOL_SECONDS = 146,                  /* SECONDS  */
  YYSYMBOL_MINUTES = 147,                  /* MINUTES  */
  YYSYMBOL_HOURS = 148,                    /* HOURS  */
  YYSYMBOL_DAYS = 149,                     /* DAYS  */
  YYSYMBOL_MONTHS = 150,                   /* MONTHS  */
  YYSYMBOL_YEARS = 151,                    /* YEARS  */
  YYSYMBOL_INTERVAL = 152,                 /* INTERVAL  */
  YYSYMBOL_TRUE = 153,                     /* TRUE  */
  YYSYMBOL_FALSE = 154,                    /* FALSE  */
  YYSYMBOL_BOOLEAN = 155,                  /* BOOLEAN  */
  YYSYMBOL_TRANSACTION = 156,              /* TRANSACTION  */
  YYSYMBOL_BEGIN = 157,                    /* BEGIN  */
  YYSYMBOL_COMMIT = 158,                   /* COMMIT  */
  YYSYMBOL_ROLLBACK = 159,                 /* ROLLBACK  */
  YYSYMBOL_START = 160,                    /* START  */
  YYSYMBOL_NOWAIT = 161,                   /* NOWAIT  */
  YYSYMBOL_SKIP = 162,                     /* SKIP  */
  YYSYMBOL_LOCKED = 163,                   /* LOCKED  */
  YYSYMBOL_SHARE = 164,                    /* SHARE  */
  YYSYMBOL_RANGE = 165,                    /* RANGE  */
  YYSYMBOL_ROWS = 166,                     /* ROWS  */
  YYSYMBOL_GROUPS = 167,                   /* GROUPS  */
  YYSYMBOL_UNBOUNDED = 168,                /* UNBOUNDED  */
  YYSYMBOL_FOLLOWING = 169,                /* FOLLOWING  */
  YYSYMBOL_PRECEDING = 170,                /* PRECEDING  */
  YYSYMBOL_CURRENT_ROW = 171,              /* CURRENT_ROW  */
  YYSYMBOL_DATABASE = 172,                 /* DATABASE  */
  YYSYMBOL_DATABASES = 173,                /* DATABASES  */
  YYSYMBOL_AUTO_INCREMENT = 174,           /* AUTO_INCREMENT  */
  YYSYMBOL_COMMENT = 175,                  /* COMMENT  */
  YYSYMBOL_AVERAGE = 176,                  /* AVERAGE  */
  YYSYMBOL_COUNT = 177,                    /* COUNT  */
  YYSYMBOL_MIN = 178,                      /* MIN  */
  YYSYMBOL_MAX = 179,                      /* MAX  */
  YYSYMBOL_SUM = 180,                      /* SUM  */
  YYSYMBOL_USE = 181,                      /* USE  */
  YYSYMBOL_182_ = 182,                     /* '='  */
  YYSYMBOL_EQ = 183,                       /* EQ  */
  YYSYMBOL_NE = 184,                       /* NE  */
  YYSYMBOL_185_ = 185,                     /* '<'  */
  YYSYMBOL_186_ = 186,                     /* '>'  */
  YYSYMBOL_LE = 187,                       /* LE  */
  YYSYMBOL_GE = 188,                       /* GE  */
  YYSYMBOL_189_ = 189,                     /* '+'  */
  YYSYMBOL_190_ = 190,                     /* '-'  */
  YYSYMBOL_191_ = 191,                     /* '*'  */
  YYSYMBOL_192_ = 192,                     /* '/'  */
  YYSYMBOL_193_ = 193,                     /* '%'  */
  YYSYMBOL_194_ = 194,                     /* '^'  */
  YYSYMBOL_UMINUS = 195,                   /* UMINUS  */
  YYSYMBOL_196_ = 196,                     /* '['  */
  YYSYMBOL_197_ = 197,                     /* ']'  */
  YYSYMBOL_198_ = 198,                     /* '('  */
  YYSYMBOL_199_ = 199,                     /* ')'  */
  YYSYMBOL_200_ = 200,                     /* '.'  */
  YYSYMBOL_201_ = 201,                     /* ';'  */
  YYSYMBOL_202_ = 202,                     /* ','  */
  YYSYMBOL_203_ = 203,                     /* '?'  */
  YYSYMBOL_YYACCEPT = 204,                 /* $accept  */
  YYSYMBOL_input = 205,                    /* input  */
  YYSYMBOL_statement_list = 206,           /* statement_list  */
  YYSYMBOL_expr_statement = 207,           /* expr_statement  */
  YYSYMBOL_expr_create_db = 208,           /* expr_create_db  */
  YYSYMBOL_expr_drop_db = 209,             /* expr_drop_db  */
  YYSYMBOL_expr_show_db = 210,             /* expr_show_db  */
  YYSYMBOL_expr_use_db = 211,              /* expr_use_db  */
  YYSYMBOL_expr_create_table = 212,        /* expr_create_table  */
  YYSYMBOL_expr_drop_table = 213,          /* expr_drop_table  */
  YYSYMBOL_expr_show_tables = 214,         /* expr_show_tables  */
  YYSYMBOL_expr_trun_table = 215,          /* expr_trun_table  */
  YYSYMBOL_expr_transaction = 216,         /* expr_transaction  */
  YYSYMBOL_expr_insert = 217,              /* expr_insert  */
  YYSYMBOL_expr_delete = 218,              /* expr_delete  */
  YYSYMBOL_expr_update = 219,              /* expr_update  */
  YYSYMBOL_expr_select = 220,              /* expr_select  */
  YYSYMBOL_opt_expr_vct_select_column = 221, /* opt_expr_vct_select_column  */
  YYSYMBOL_expr_vct_select_column = 222,   /* expr_vct_select_column  */
  YYSYMBOL_expr_select_column = 223,       /* expr_select_column  */
  YYSYMBOL_col_alias = 224,                /* col_alias  */
  YYSYMBOL_expr_vct_insert_column = 225,   /* expr_vct_insert_column  */
  YYSYMBOL_expr_insert_column = 226,       /* expr_insert_column  */
  YYSYMBOL_expr_vct_update_column = 227,   /* expr_vct_update_column  */
  YYSYMBOL_expr_update_column = 228,       /* expr_update_column  */
  YYSYMBOL_expr_table = 229,               /* expr_table  */
  YYSYMBOL_opt_not_exists = 230,           /* opt_not_exists  */
  YYSYMBOL_opt_exists = 231,               /* opt_exists  */
  YYSYMBOL_opt_expr_vct_table = 232,       /* opt_expr_vct_table  */
  YYSYMBOL_expr_vct_table = 233,           /* expr_vct_table  */
  YYSYMBOL_join_type = 234,                /* join_type  */
  YYSYMBOL_expr_vct_col_name = 235,        /* expr_vct_col_name  */
  YYSYMBOL_opt_semicolon = 236,            /* opt_semicolon  */
  YYSYMBOL_expr_vct_create_table_item = 237, /* expr_vct_create_table_item  */
  YYSYMBOL_expr_create_table_item = 238,   /* expr_create_table_item  */
  YYSYMBOL_expr_data_type = 239,           /* expr_data_type  */
  YYSYMBOL_col_nullable = 240,             /* col_nullable  */
  YYSYMBOL_default_col_dv = 241,           /* default_col_dv  */
  YYSYMBOL_auto_increment = 242,           /* auto_increment  */
  YYSYMBOL_opt_index_type = 243,           /* opt_index_type  */
  YYSYMBOL_index_type = 244,               /* index_type  */
  YYSYMBOL_table_comment = 245,            /* table_comment  */
  YYSYMBOL_expr_vct_elem_row = 246,        /* expr_vct_elem_row  */
  YYSYMBOL_expr_elem_row = 247,            /* expr_elem_row  */
  YYSYMBOL_opt_expr_where = 248,           /* opt_expr_where  */
  YYSYMBOL_opt_expr_on = 249,              /* opt_expr_on  */
  YYSYMBOL_opt_expr_order_by = 250,        /* opt_expr_order_by  */
  YYSYMBOL_expr_vct_order_item = 251,      /* expr_vct_order_item  */
  YYSYMBOL_expr_order_item = 252,          /* expr_order_item  */
  YYSYMBOL_opt_order_direction = 253,      /* opt_order_direction  */
  YYSYMBOL_opt_expr_limit = 254,           /* opt_expr_limit  */
  YYSYMBOL_opt_expr_group_by = 255,        /* opt_expr_group_by  */
  YYSYMBOL_opt_expr_having = 256,          /* opt_expr_having  */
  YYSYMBOL_opt_distinct = 257,             /* opt_distinct  */
  YYSYMBOL_opt_lock_type = 258,            /* opt_lock_type  */
  YYSYMBOL_expr_array = 259,               /* expr_array  */
  YYSYMBOL_expr_vct_const = 260,           /* expr_vct_const  */
  YYSYMBOL_expr_elem = 261,                /* expr_elem  */
  YYSYMBOL_expr_data = 262,                /* expr_data  */
  YYSYMBOL_expr_const = 263,               /* expr_const  */
  YYSYMBOL_expr_field = 264,               /* expr_field  */
  YYSYMBOL_expr_param = 265,               /* expr_param  */
  YYSYMBOL_expr_add = 266,                 /* expr_add  */
  YYSYMBOL_expr_sub = 267,                 /* expr_sub  */
  YYSYMBOL_expr_mul = 268,                 /* expr_mul  */
  YYSYMBOL_expr_div = 269,                 /* expr_div  */
  YYSYMBOL_expr_minus = 270,               /* expr_minus  */
  YYSYMBOL_expr_func = 271,                /* expr_func  */
  YYSYMBOL_opt_expr_vct_data = 272,        /* opt_expr_vct_data  */
  YYSYMBOL_expr_vct_data = 273,            /* expr_vct_data  */
  YYSYMBOL_const_dv = 274,                 /* const_dv  */
  YYSYMBOL_const_string = 275,             /* const_string  */
  YYSYMBOL_const_bool = 276,               /* const_bool  */
  YYSYMBOL_const_double = 277,             /* const_double  */
  YYSYMBOL_const_int = 278,                /* const_int  */
  YYSYMBOL_const_null = 279,               /* const_null  */
  YYSYMBOL_expr_logic = 280,               /* expr_logic  */
  YYSYMBOL_expr_cmp = 281,                 /* expr_cmp  */
  YYSYMBOL_comp_type = 282,                /* comp_type  */
  YYSYMBOL_expr_in_not = 283,              /* expr_in_not  */
  YYSYMBOL_expr_is_null_not = 284,         /* expr_is_null_not  */
  YYSYMBOL_expr_between = 285,             /* expr_between  */
  YYSYMBOL_expr_like = 286,                /* expr_like  */
  YYSYMBOL_expr_not = 287,                 /* expr_not  */
  YYSYMBOL_expr_and = 288,                 /* expr_and  */
  YYSYMBOL_expr_or = 289,                  /* expr_or  */
  YYSYMBOL_expr_aggr = 290,                /* expr_aggr  */
  YYSYMBOL_expr_count = 291,               /* expr_count  */
  YYSYMBOL_expr_sum = 292,                 /* expr_sum  */
  YYSYMBOL_expr_max = 293,                 /* expr_max  */
  YYSYMBOL_expr_min = 294,                 /* expr_min  */
  YYSYMBOL_expr_avg = 295                  /* expr_avg  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined DB_LTYPE_IS_TRIVIAL && DB_LTYPE_IS_TRIVIAL \
             && defined DB_STYPE_IS_TRIVIAL && DB_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  47
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   390

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  204
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  92
/* YYNRULES -- Number of rules.  */
#define YYNRULES  217
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  358

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   441


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,   193,     2,     2,
     198,   199,   191,   189,   202,   190,   200,   192,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   201,
     185,   182,   186,   203,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   196,     2,   197,   194,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   183,   184,   187,
     188,   195
};

#if DB_DEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   326,   326,   331,   336,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   360,   363,
     367,   370,   374,   378,   382,   386,   390,   393,   395,   399,
     400,   401,   402,   404,   410,   417,   425,   434,   447,   450,
     454,   458,   463,   467,   470,   472,   476,   481,   485,   489,
     494,   498,   501,   504,   507,   511,   512,   514,   515,   517,
     520,   522,   526,   532,   533,   534,   535,   536,   537,   538,
     539,   540,   541,   543,   547,   552,   553,   555,   559,   564,
     577,   580,   584,   585,   586,   587,   588,   589,   590,   591,
     592,   593,   594,   596,   597,   598,   600,   601,   603,   604,
     606,   607,   609,   610,   611,   612,   613,   614,   616,   617,
     619,   623,   628,   632,   637,   640,   642,   645,   647,   650,
     652,   656,   661,   665,   666,   667,   669,   672,   675,   677,
     680,   682,   685,   687,   688,   690,   691,   692,   694,   695,
     699,   704,   705,   706,   708,   708,   708,   708,   708,   708,
     708,   708,   708,   709,   711,   713,   714,   716,   723,   725,
     727,   729,   731,   733,   737,   740,   742,   746,   751,   751,
     751,   751,   751,   752,   757,   758,   760,   762,   764,   766,
     766,   766,   766,   766,   766,   767,   768,   769,   771,   775,
     776,   777,   778,   779,   780,   781,   783,   786,   790,   791,
     793,   797,   798,   800,   802,   813,   824,   824,   824,   824,
     824,   825,   827,   830,   834,   838,   842,   846
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "IDENTIFIER", "STRING",
  "FLOATVAL", "INTVAL", "DEALLOCATE", "PARAMETERS", "INTERSECT",
  "TEMPORARY", "TIMESTAMP", "DISTINCT", "NVARCHAR", "RESTRICT", "TRUNCATE",
  "ANALYZE", "BETWEEN", "CASCADE", "COLUMNS", "CONTROL", "DEFAULT",
  "EXECUTE", "EXPLAIN", "INTEGER", "NATURAL", "PREPARE", "PRIMARY",
  "SCHEMAS", "CHARACTER_VARYING", "REAL", "DECIMAL", "SMALLINT", "BIGINT",
  "SPATIAL", "VARCHAR", "VIRTUAL", "DESCRIBE", "BEFORE", "COLUMN",
  "CREATE", "DELETE", "DIRECT", "DOUBLE", "ESCAPE", "EXCEPT", "EXISTS",
  "EXTRACT", "CAST", "FORMAT", "GLOBAL", "HAVING", "IMPORT", "INSERT",
  "ISNULL", "OFFSET", "RENAME", "SCHEMA", "SELECT", "SORTED", "TABLES",
  "UNIQUE", "UNLOAD", "UPDATE", "VALUES", "AFTER", "ALTER", "CROSS",
  "DELTA", "FLOAT", "GROUP", "INDEX", "INNER", "LIMIT", "LOCAL", "MERGE",
  "MINUS", "ORDER", "OVER", "OUTER", "RIGHT", "TABLE", "UNION", "USING",
  "WHERE", "CALL", "CASE", "CHAR", "COPY", "DATE", "DATETIME", "DESC",
  "DROP", "ELSE", "FILE", "FROM", "FULL", "HASH", "HINT", "INTO", "JOIN",
  "LEFT", "LIKE", "LOAD", "LONG", "NULL", "PARTITION", "PLAN", "SHOW",
  "TEXT", "THEN", "TIME", "VIEW", "WHEN", "WITH", "ADD", "ALL", "AND",
  "ASC", "END", "FOR", "INT", "KEY", "NOT", "OFF", "SET", "TOP", "AS",
  "BY", "IF", "IN", "IS", "OF", "ON", "OR", "TO", "NO", "ARRAY", "CONCAT",
  "ILIKE", "SECOND", "MINUTE", "HOUR", "DAY", "MONTH", "YEAR", "SECONDS",
  "MINUTES", "HOURS", "DAYS", "MONTHS", "YEARS", "INTERVAL", "TRUE",
  "FALSE", "BOOLEAN", "TRANSACTION", "BEGIN", "COMMIT", "ROLLBACK",
  "START", "NOWAIT", "SKIP", "LOCKED", "SHARE", "RANGE", "ROWS", "GROUPS",
  "UNBOUNDED", "FOLLOWING", "PRECEDING", "CURRENT_ROW", "DATABASE",
  "DATABASES", "AUTO_INCREMENT", "COMMENT", "AVERAGE", "COUNT", "MIN",
  "MAX", "SUM", "USE", "'='", "EQ", "NE", "'<'", "'>'", "LE", "GE", "'+'",
  "'-'", "'*'", "'/'", "'%'", "'^'", "UMINUS", "'['", "']'", "'('", "')'",
  "'.'", "';'", "','", "'?'", "$accept", "input", "statement_list",
  "expr_statement", "expr_create_db", "expr_drop_db", "expr_show_db",
  "expr_use_db", "expr_create_table", "expr_drop_table",
  "expr_show_tables", "expr_trun_table", "expr_transaction", "expr_insert",
  "expr_delete", "expr_update", "expr_select",
  "opt_expr_vct_select_column", "expr_vct_select_column",
  "expr_select_column", "col_alias", "expr_vct_insert_column",
  "expr_insert_column", "expr_vct_update_column", "expr_update_column",
  "expr_table", "opt_not_exists", "opt_exists", "opt_expr_vct_table",
  "expr_vct_table", "join_type", "expr_vct_col_name", "opt_semicolon",
  "expr_vct_create_table_item", "expr_create_table_item", "expr_data_type",
  "col_nullable", "default_col_dv", "auto_increment", "opt_index_type",
  "index_type", "table_comment", "expr_vct_elem_row", "expr_elem_row",
  "opt_expr_where", "opt_expr_on", "opt_expr_order_by",
  "expr_vct_order_item", "expr_order_item", "opt_order_direction",
  "opt_expr_limit", "opt_expr_group_by", "opt_expr_having", "opt_distinct",
  "opt_lock_type", "expr_array", "expr_vct_const", "expr_elem",
  "expr_data", "expr_const", "expr_field", "expr_param", "expr_add",
  "expr_sub", "expr_mul", "expr_div", "expr_minus", "expr_func",
  "opt_expr_vct_data", "expr_vct_data", "const_dv", "const_string",
  "const_bool", "const_double", "const_int", "const_null", "expr_logic",
  "expr_cmp", "comp_type", "expr_in_not", "expr_is_null_not",
  "expr_between", "expr_like", "expr_not", "expr_and", "expr_or",
  "expr_aggr", "expr_count", "expr_sum", "expr_max", "expr_min",
  "expr_avg", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-293)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       1,   -14,   -23,    16,    20,   114,   128,     0,   -56,  -293,
    -293,  -293,    -4,   168,   176,   -12,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
     128,    58,    58,    58,   128,   128,  -293,    15,  -115,    73,
      68,    68,    68,   106,  -293,  -293,  -293,  -293,     1,  -293,
    -293,    84,   211,   128,   226,   150,    35,  -111,  -293,  -293,
    -293,  -293,    25,  -293,  -293,    40,    41,    47,    48,    49,
      72,  -293,    32,  -293,   156,    52,  -293,   131,   125,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,    -1,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,   256,
     257,   258,   218,   264,   128,   266,   268,  -293,   227,  -293,
      74,  -293,    25,   197,   274,    72,   275,    25,   125,  -293,
      72,    46,    72,    72,    72,    72,  -148,   101,   -72,    80,
      12,   150,    32,   277,  -293,    72,   278,   -36,    96,    31,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,    72,    72,    72,
      72,    72,    25,    25,  -293,   174,   121,   -81,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,    19,    -1,   177,   231,  -293,
     -55,  -293,   -67,   119,   117,  -293,    51,   130,   107,   132,
     136,   149,   153,  -293,  -293,  -293,  -293,   -40,  -293,   199,
    -293,  -293,   -26,  -293,   278,    96,    99,  -293,  -293,   225,
    -148,  -148,  -293,  -293,   -67,  -293,   216,   331,    32,   258,
     197,   233,   214,   224,  -293,  -293,   -49,  -293,    -2,   344,
     343,  -293,   -11,   274,  -293,    72,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,   271,   272,   276,  -293,   128,
      25,   283,    72,  -293,  -293,   -32,  -293,  -293,  -293,  -293,
    -293,   231,  -293,  -293,  -293,  -293,   158,  -293,  -293,   159,
    -293,  -293,  -293,    34,  -293,  -293,  -293,    19,   160,   356,
      22,   161,  -293,   162,   163,  -293,  -293,   -67,  -293,  -293,
    -293,  -293,    -1,   232,   197,   -67,  -293,    99,  -293,   359,
     360,  -293,   262,   341,  -293,   356,  -293,   -19,  -293,  -293,
    -293,   344,   362,    32,   167,   356,   231,  -293,   171,   172,
    -293,    99,   198,   -18,  -293,   370,  -293,  -293,    17,  -293,
     178,   -46,   254,  -293,  -293,  -293,  -293,    21,  -293,  -293,
    -293,    32,    32,    25,  -293,   -54,  -293,   200,  -293,  -293,
      18,    -1,  -293,  -293,   373,  -293,  -293,  -293
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,   134,     0,     0,     0,    29,
      32,    31,     0,     0,     0,    76,     3,     5,     6,     7,
       8,     9,    10,    11,    12,    17,    14,    16,    15,    13,
       0,    56,    56,    56,     0,     0,   133,     0,    51,     0,
      58,    58,    58,    27,    22,    30,    23,     1,    75,     2,
      28,     0,     0,     0,     0,   115,     0,   155,   173,   176,
     177,   178,     0,   174,   175,     0,     0,     0,     0,     0,
       0,    39,     0,   157,    60,    38,    40,    44,   142,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   154,   170,
     171,   169,   168,   172,   141,   179,   180,   181,   182,   183,
     184,   185,   186,   143,   206,   207,   208,   209,   210,     0,
       0,     0,     0,     0,     0,     0,     0,     4,     0,    19,
       0,    18,     0,   119,     0,   165,     0,     0,     0,   203,
       0,     0,     0,     0,     0,     0,   162,     0,     0,     0,
      60,   115,     0,     0,    42,     0,     0,     0,     0,     0,
     189,   195,   194,   191,   190,   193,   192,     0,     0,     0,
       0,     0,     0,     0,    52,    53,     0,   115,    48,    57,
      21,    25,    20,    26,    55,     0,   114,     0,   128,    47,
       0,    45,   166,     0,   164,   156,     0,     0,     0,     0,
       0,     0,     0,   153,   187,   211,    61,     0,    59,   117,
      41,    43,     0,   201,     0,     0,     0,   196,   198,     0,
     158,   159,   160,   161,   188,   204,   205,     0,     0,     0,
     119,     0,   103,   105,   107,   106,     0,    77,     0,     0,
       0,    35,     0,     0,   163,     0,   217,   213,   212,   216,
     215,   214,    71,    63,    69,    67,    70,    65,    72,     0,
       0,   130,     0,   202,   197,     0,   139,   199,    54,    50,
      49,   128,    88,    90,    91,    82,     0,    85,    86,     0,
      89,    87,    83,    95,   102,   104,    24,     0,     0,     0,
     125,   118,   120,   126,     0,    34,    46,   167,    66,    68,
      64,    62,   116,     0,   119,   200,   138,     0,    36,     0,
       0,    93,     0,    97,    78,     0,    73,     0,   124,   123,
     122,     0,     0,     0,    33,     0,   128,   140,     0,     0,
      94,     0,    99,     0,    81,     0,   121,   127,     0,   112,
       0,   132,   137,    92,    84,    96,    98,   101,    80,    74,
     110,     0,     0,     0,   129,     0,    37,   109,   100,   113,
       0,   131,   136,   135,     0,    79,   111,   108
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -293,  -293,  -293,   330,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,   147,  -293,  -293,   238,
    -293,  -293,   148,  -293,   164,   -28,   113,   133,   242,  -293,
    -293,  -292,  -293,  -293,   108,  -293,  -293,  -293,  -293,  -293,
      50,  -293,  -293,    42,   -39,  -293,  -165,  -293,    75,  -293,
    -228,  -293,  -293,  -293,  -293,   184,  -293,  -207,   -62,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -189,  -120,  -293,  -293,  -293,  -293,   -48,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,   318,  -293,  -293,  -293,
    -293,  -293
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    74,    75,    76,
     144,   180,   181,   167,   168,    39,    52,   113,   141,   198,
     249,   307,    49,   226,   227,   273,   303,   322,   337,   347,
     228,   355,   314,   328,   123,   251,   178,   281,   282,   310,
     231,   294,   344,    37,   346,   207,   255,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,   183,   184,
      88,    89,    90,    91,    92,    93,    94,    95,   161,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     128,   278,    50,   122,    43,   343,    55,    56,   136,   352,
     137,   259,   109,   323,   129,    38,     1,   256,    57,    58,
      59,    60,   221,   331,   138,   120,   203,   242,    57,    58,
      59,    60,   243,   298,    31,    57,    58,    59,    60,   244,
     245,     2,     3,   159,   160,   162,   222,     5,   222,    57,
      58,    59,    60,   284,     4,   261,   246,    40,    32,     5,
     128,   247,   163,   182,     6,   137,   204,    30,   186,   188,
     189,   190,   191,   192,   176,    57,    58,    59,    60,   138,
     223,    41,   223,   202,   253,   110,   171,   125,   332,   126,
     224,   252,   224,     7,   205,   210,   211,   212,   213,   214,
     128,   128,   199,    58,    59,    60,   329,   140,   317,     8,
     353,    34,   196,   308,   215,   216,   162,    44,   145,    35,
      61,   219,   157,   158,   159,   160,    36,   194,   220,   316,
      61,    38,   335,   163,   349,   329,   208,    61,    62,   301,
     309,   225,   145,   225,   232,    53,    54,   233,    62,    33,
     276,    61,    45,   277,   209,    62,   325,   302,     9,    10,
      11,    12,   248,   157,   158,   159,   160,   296,    63,    64,
     297,    46,    42,   287,   114,   115,    47,    61,    63,    64,
     324,   338,    13,   325,   325,    63,    64,    51,   128,    48,
     295,    65,    66,    67,    68,    69,   279,   112,   111,    63,
      64,   116,   292,   146,    61,    70,    71,   118,    65,    66,
      67,    68,    69,    72,   119,    70,   340,   356,    73,   341,
     341,   291,    70,   127,   147,    63,    64,   146,    73,   121,
      72,   148,   149,   124,   122,    73,    70,   187,   130,   131,
     157,   158,   159,   160,   135,   132,   133,   134,   147,    73,
     236,   140,    63,    64,   142,   148,   149,   262,   143,   164,
     165,   166,    70,   263,   169,   264,   265,   170,   266,   172,
     135,   173,   175,   174,   177,    73,   267,   179,   185,   195,
     201,   128,    58,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   206,   351,   157,   158,   159,   160,
     193,   217,   268,   218,   230,   229,   238,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   234,   235,
     269,   157,   158,   159,   160,   157,   158,   159,   160,   237,
     257,   239,   250,   162,   258,   240,   274,   270,   157,   158,
     159,   160,   157,   158,   159,   160,   275,   280,   241,   283,
     288,   289,   193,   293,   271,   290,   299,   300,   305,   306,
     315,   313,   321,   311,   312,   318,   319,   320,   327,   330,
     333,   334,   336,   339,   345,   354,   342,   357,   117,   285,
     200,   286,   197,   260,   350,   304,   326,   348,   272,   254,
     139
};

static const yytype_int16 yycheck[] =
{
      62,     3,    30,    84,    60,    51,    34,    35,    70,    63,
      72,   218,   127,   305,    62,     3,    15,   206,     3,     4,
       5,     6,     3,   315,    72,    53,   146,    67,     3,     4,
       5,     6,    72,   261,    57,     3,     4,     5,     6,    79,
      80,    40,    41,   191,   192,   117,    27,    58,    27,     3,
       4,     5,     6,    64,    53,   220,    96,    57,    81,    58,
     122,   101,   134,   125,    63,   127,   102,    81,   130,   131,
     132,   133,   134,   135,   122,     3,     4,     5,     6,   127,
      61,    81,    61,   145,   204,   200,   114,   198,   316,   200,
      71,   117,    71,    92,   130,   157,   158,   159,   160,   161,
     162,   163,   141,     4,     5,     6,   313,    95,   297,   108,
     164,    95,   140,    91,   162,   163,   117,   173,    17,    99,
     105,   202,   189,   190,   191,   192,    12,   199,   167,   294,
     105,     3,   321,   134,   341,   342,   105,   105,   123,   105,
     118,   122,    17,   122,   199,    32,    33,   202,   123,   172,
     199,   105,   156,   202,   123,   123,   202,   123,   157,   158,
     159,   160,   202,   189,   190,   191,   192,   199,   153,   154,
     202,     3,   172,   235,    41,    42,     0,   105,   153,   154,
     199,   199,   181,   202,   202,   153,   154,   129,   250,   201,
     252,   176,   177,   178,   179,   180,   198,   129,   125,   153,
     154,    95,   250,   102,   105,   190,   191,   123,   176,   177,
     178,   179,   180,   198,     3,   190,   199,   199,   203,   202,
     202,   249,   190,   198,   123,   153,   154,   102,   203,     3,
     198,   130,   131,   198,    84,   203,   190,   191,   198,   198,
     189,   190,   191,   192,   198,   198,   198,   198,   123,   203,
     199,    95,   153,   154,   202,   130,   131,    24,   127,     3,
       3,     3,   190,    30,    46,    32,    33,     3,    35,     3,
     198,     3,   198,    46,    77,   203,    43,     3,     3,   199,
       3,   343,     4,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   198,   343,   189,   190,   191,   192,
     199,   127,    69,   182,    73,   128,   199,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   199,   202,
      87,   189,   190,   191,   192,   189,   190,   191,   192,   199,
     105,   199,   133,   117,     3,   199,   122,   104,   189,   190,
     191,   192,   189,   190,   191,   192,   122,     3,   199,     6,
      79,    79,   199,    70,   121,    79,   198,   198,   198,     3,
     128,   198,    21,   202,   202,     6,     6,   105,     6,   202,
     199,   199,   174,     3,   120,   175,   198,     4,    48,   232,
     142,   233,   140,   219,   342,   277,   311,   337,   155,   205,
      72
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,    15,    40,    41,    53,    58,    63,    92,   108,   157,
     158,   159,   160,   181,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
      81,    57,    81,   172,    95,    99,    12,   257,     3,   229,
      57,    81,   172,    60,   173,   156,     3,     0,   201,   236,
     229,   129,   230,   230,   230,   229,   229,     3,     4,     5,
       6,   105,   123,   153,   154,   176,   177,   178,   179,   180,
     190,   191,   198,   203,   221,   222,   223,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   274,   275,
     276,   277,   278,   279,   280,   281,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   127,
     200,   125,   129,   231,   231,   231,    95,   207,   123,     3,
     229,     3,    84,   248,   198,   198,   200,   198,   262,   280,
     198,   198,   198,   198,   198,   198,   262,   262,   280,   290,
      95,   232,   202,   127,   224,    17,   102,   123,   130,   131,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   282,   117,   134,     3,     3,     3,   227,   228,    46,
       3,   229,     3,     3,    46,   198,   280,    77,   250,     3,
     225,   226,   262,   272,   273,     3,   262,   191,   262,   262,
     262,   262,   262,   199,   199,   199,   229,   232,   233,   248,
     223,     3,   262,   275,   102,   130,   198,   259,   105,   123,
     262,   262,   262,   262,   262,   280,   280,   127,   182,   202,
     248,     3,    27,    61,    71,   122,   237,   238,   244,   128,
      73,   254,   199,   202,   199,   202,   199,   199,   199,   199,
     199,   199,    67,    72,    79,    80,    96,   101,   202,   234,
     133,   249,   117,   275,   259,   260,   274,   105,     3,   261,
     228,   250,    24,    30,    32,    33,    35,    43,    69,    87,
     104,   121,   155,   239,   122,   122,   199,   202,     3,   198,
       3,   251,   252,     6,    64,   220,   226,   262,    79,    79,
      79,   229,   280,    70,   255,   262,   199,   202,   254,   198,
     198,   105,   123,   240,   238,   198,     3,   235,    91,   118,
     253,   202,   202,   198,   246,   128,   250,   274,     6,     6,
     105,    21,   241,   235,   199,   202,   252,     6,   247,   261,
     202,   235,   254,   199,   199,   274,   174,   242,   199,     3,
     199,   202,   198,    51,   256,   120,   258,   243,   244,   261,
     247,   280,    63,   164,   175,   245,   199,     4
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   204,   205,   206,   206,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   208,   208,
     209,   209,   210,   211,   212,   213,   214,   214,   215,   216,
     216,   216,   216,   217,   217,   218,   219,   220,   221,   221,
     222,   222,   223,   224,   224,   225,   225,   226,   227,   227,
     228,   229,   229,   229,   229,   230,   230,   231,   231,   232,
     232,   233,   233,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   235,   235,   236,   236,   237,   237,   238,
     238,   238,   239,   239,   239,   239,   239,   239,   239,   239,
     239,   239,   239,   240,   240,   240,   241,   241,   242,   242,
     243,   243,   244,   244,   244,   244,   244,   244,   245,   245,
     246,   246,   247,   247,   248,   248,   249,   249,   250,   250,
     251,   251,   252,   253,   253,   253,   254,   254,   254,   255,
     255,   256,   256,   257,   257,   258,   258,   258,   259,   260,
     260,   261,   261,   261,   262,   262,   262,   262,   262,   262,
     262,   262,   262,   262,   263,   264,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   272,   273,   273,   274,   274,
     274,   274,   274,   275,   276,   276,   277,   278,   279,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   281,   282,
     282,   282,   282,   282,   282,   282,   283,   283,   284,   284,
     285,   286,   286,   287,   288,   289,   290,   290,   290,   290,
     290,   290,   291,   291,   292,   293,   294,   295
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     4,
       4,     4,     2,     2,     7,     4,     4,     2,     3,     1,
       2,     1,     1,     8,     7,     6,     7,    10,     1,     1,
       1,     3,     2,     2,     0,     1,     3,     1,     1,     3,
       3,     1,     3,     3,     5,     3,     0,     2,     0,     2,
       0,     1,     3,     1,     2,     1,     2,     1,     2,     1,
       1,     1,     1,     1,     3,     1,     0,     1,     3,     7,
       5,     4,     1,     1,     4,     1,     1,     1,     1,     1,
       1,     1,     4,     1,     2,     0,     2,     0,     1,     0,
       1,     0,     2,     1,     2,     1,     1,     1,     2,     0,
       3,     5,     1,     3,     2,     0,     2,     0,     3,     0,
       1,     3,     2,     1,     1,     0,     2,     4,     0,     4,
       0,     2,     0,     1,     0,     2,     2,     0,     3,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     1,     3,     1,     3,     3,
       3,     3,     2,     4,     1,     0,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     3,     4,     3,     4,
       5,     3,     4,     2,     3,     3,     1,     1,     1,     1,
       1,     3,     4,     4,     4,     4,     4,     4
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = SQL_DB_EMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == SQL_DB_EMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, result, scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use SQL_DB_error or SQL_DB_UNDEF. */
#define YYERRCODE SQL_DB_UNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if DB_DEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined DB_LTYPE_IS_TRIVIAL && DB_LTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, result, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, ParserResult* result, yyscan_t scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (result);
  YY_USE (scanner);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, ParserResult* result, yyscan_t scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, result, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, ParserResult* result, yyscan_t scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), result, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, result, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !DB_DEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !DB_DEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, ParserResult* result, yyscan_t scanner)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (result);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_IDENTIFIER: /* IDENTIFIER  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).sval)); }
#line 1807 "sql_parser.cpp"
        break;

    case YYSYMBOL_STRING: /* STRING  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).sval)); }
#line 1813 "sql_parser.cpp"
        break;

    case YYSYMBOL_FLOATVAL: /* FLOATVAL  */
#line 198 "sql_parser.y"
                { }
#line 1819 "sql_parser.cpp"
        break;

    case YYSYMBOL_INTVAL: /* INTVAL  */
#line 198 "sql_parser.y"
                { }
#line 1825 "sql_parser.cpp"
        break;

    case YYSYMBOL_statement_list: /* statement_list  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_statement)); }
#line 1831 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_statement: /* expr_statement  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_statement)); }
#line 1837 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_create_db: /* expr_create_db  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_create_db)); }
#line 1843 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_drop_db: /* expr_drop_db  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_drop_db)); }
#line 1849 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_show_db: /* expr_show_db  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_show_db)); }
#line 1855 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_use_db: /* expr_use_db  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_use_db)); }
#line 1861 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_create_table: /* expr_create_table  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_create_table)); }
#line 1867 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_drop_table: /* expr_drop_table  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_drop_table)); }
#line 1873 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_show_tables: /* expr_show_tables  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_show_tables)); }
#line 1879 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_trun_table: /* expr_trun_table  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_trun_table)); }
#line 1885 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_transaction: /* expr_transaction  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_transaction)); }
#line 1891 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_insert: /* expr_insert  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_insert)); }
#line 1897 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_delete: /* expr_delete  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_delete)); }
#line 1903 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_update: /* expr_update  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_update)); }
#line 1909 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_select: /* expr_select  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_select)); }
#line 1915 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_vct_select_column: /* opt_expr_vct_select_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_column)); }
#line 1921 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_select_column: /* expr_vct_select_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_column)); }
#line 1927 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_select_column: /* expr_select_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_column)); }
#line 1933 "sql_parser.cpp"
        break;

    case YYSYMBOL_col_alias: /* col_alias  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).sval)); }
#line 1939 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_insert_column: /* expr_vct_insert_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_column)); }
#line 1945 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_insert_column: /* expr_insert_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_column)); }
#line 1951 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_update_column: /* expr_vct_update_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_column)); }
#line 1957 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_update_column: /* expr_update_column  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_column)); }
#line 1963 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_table: /* expr_table  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_table)); }
#line 1969 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_not_exists: /* opt_not_exists  */
#line 198 "sql_parser.y"
                { }
#line 1975 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_exists: /* opt_exists  */
#line 198 "sql_parser.y"
                { }
#line 1981 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_vct_table: /* opt_expr_vct_table  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_table)); }
#line 1987 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_table: /* expr_vct_table  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_table)); }
#line 1993 "sql_parser.cpp"
        break;

    case YYSYMBOL_join_type: /* join_type  */
#line 198 "sql_parser.y"
                { }
#line 1999 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_col_name: /* expr_vct_col_name  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_str)); }
#line 2005 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_create_table_item: /* expr_vct_create_table_item  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_create_table_item)); }
#line 2011 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_create_table_item: /* expr_create_table_item  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_create_table_item)); }
#line 2017 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_data_type: /* expr_data_type  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data_type)); }
#line 2023 "sql_parser.cpp"
        break;

    case YYSYMBOL_col_nullable: /* col_nullable  */
#line 198 "sql_parser.y"
                { }
#line 2029 "sql_parser.cpp"
        break;

    case YYSYMBOL_default_col_dv: /* default_col_dv  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2035 "sql_parser.cpp"
        break;

    case YYSYMBOL_auto_increment: /* auto_increment  */
#line 198 "sql_parser.y"
                { }
#line 2041 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_index_type: /* opt_index_type  */
#line 198 "sql_parser.y"
                { }
#line 2047 "sql_parser.cpp"
        break;

    case YYSYMBOL_index_type: /* index_type  */
#line 198 "sql_parser.y"
                { }
#line 2053 "sql_parser.cpp"
        break;

    case YYSYMBOL_table_comment: /* table_comment  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).sval)); }
#line 2059 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_elem_row: /* expr_vct_elem_row  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_elem_row)); }
#line 2065 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_elem_row: /* expr_elem_row  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_elem_row)); }
#line 2071 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_where: /* opt_expr_where  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_where)); }
#line 2077 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_on: /* opt_expr_on  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_on)); }
#line 2083 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_order_by: /* opt_expr_order_by  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_order_by)); }
#line 2089 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_order_item: /* expr_vct_order_item  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_order_item)); }
#line 2095 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_order_item: /* expr_order_item  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_order_item)); }
#line 2101 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_order_direction: /* opt_order_direction  */
#line 198 "sql_parser.y"
                { }
#line 2107 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_limit: /* opt_expr_limit  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_limit)); }
#line 2113 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_group_by: /* opt_expr_group_by  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_group_by)); }
#line 2119 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_having: /* opt_expr_having  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_having)); }
#line 2125 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_distinct: /* opt_distinct  */
#line 198 "sql_parser.y"
                { }
#line 2131 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_lock_type: /* opt_lock_type  */
#line 198 "sql_parser.y"
                { }
#line 2137 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_array: /* expr_array  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_array)); }
#line 2143 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_const: /* expr_vct_const  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_array)); }
#line 2149 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_elem: /* expr_elem  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_elem)); }
#line 2155 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_data: /* expr_data  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2161 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_const: /* expr_const  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2167 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_field: /* expr_field  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2173 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_param: /* expr_param  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2179 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_add: /* expr_add  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2185 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_sub: /* expr_sub  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2191 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_mul: /* expr_mul  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2197 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_div: /* expr_div  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2203 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_minus: /* expr_minus  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2209 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_func: /* expr_func  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_data)); }
#line 2215 "sql_parser.cpp"
        break;

    case YYSYMBOL_opt_expr_vct_data: /* opt_expr_vct_data  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_data)); }
#line 2221 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_vct_data: /* expr_vct_data  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_vct_data)); }
#line 2227 "sql_parser.cpp"
        break;

    case YYSYMBOL_const_dv: /* const_dv  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2233 "sql_parser.cpp"
        break;

    case YYSYMBOL_const_string: /* const_string  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2239 "sql_parser.cpp"
        break;

    case YYSYMBOL_const_bool: /* const_bool  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2245 "sql_parser.cpp"
        break;

    case YYSYMBOL_const_double: /* const_double  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2251 "sql_parser.cpp"
        break;

    case YYSYMBOL_const_int: /* const_int  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2257 "sql_parser.cpp"
        break;

    case YYSYMBOL_const_null: /* const_null  */
#line 199 "sql_parser.y"
                { if (((*yyvaluep).data_value) != nullptr) ((*yyvaluep).data_value)->DecRef(); }
#line 2263 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_logic: /* expr_logic  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2269 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_cmp: /* expr_cmp  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2275 "sql_parser.cpp"
        break;

    case YYSYMBOL_comp_type: /* comp_type  */
#line 198 "sql_parser.y"
                { }
#line 2281 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_in_not: /* expr_in_not  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2287 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_is_null_not: /* expr_is_null_not  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2293 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_between: /* expr_between  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2299 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_like: /* expr_like  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2305 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_not: /* expr_not  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_logic)); }
#line 2311 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_and: /* expr_and  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_and)); }
#line 2317 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_or: /* expr_or  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_or)); }
#line 2323 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_aggr: /* expr_aggr  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_aggr)); }
#line 2329 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_count: /* expr_count  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_aggr)); }
#line 2335 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_sum: /* expr_sum  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_aggr)); }
#line 2341 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_max: /* expr_max  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_aggr)); }
#line 2347 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_min: /* expr_min  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_aggr)); }
#line 2353 "sql_parser.cpp"
        break;

    case YYSYMBOL_expr_avg: /* expr_avg  */
#line 200 "sql_parser.y"
                { delete (((*yyvaluep).expr_aggr)); }
#line 2359 "sql_parser.cpp"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (ParserResult* result, yyscan_t scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined DB_LTYPE_IS_TRIVIAL && DB_LTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = SQL_DB_EMPTY; /* Cause a token to be read.  */


/* User initialization code.  */
#line 79 "sql_parser.y"
{
  // Initialize
  yylloc.first_column = 0;
  yylloc.last_column = 0;
  yylloc.first_line = 0;
  yylloc.last_line = 0;
  yylloc.total_column = 0;
  yylloc.string_length = 0;
}

#line 2467 "sql_parser.cpp"

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == SQL_DB_EMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= SQL_YYEOF)
    {
      yychar = SQL_YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == SQL_DB_error)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = SQL_DB_UNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = SQL_DB_EMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* input: statement_list opt_semicolon  */
#line 326 "sql_parser.y"
                                     {
  result->AddStatements((yyvsp[-1].expr_vct_statement));
  result->AddParameters(yyloc.param_list);
}
#line 2683 "sql_parser.cpp"
    break;

  case 3: /* statement_list: expr_statement  */
#line 331 "sql_parser.y"
                                {
  yylloc.string_length = 0;
  (yyval.expr_vct_statement) = new MVectorPtr<ExprStatement*>();
  (yyval.expr_vct_statement)->push_back((yyvsp[0].expr_statement));
}
#line 2693 "sql_parser.cpp"
    break;

  case 4: /* statement_list: statement_list ';' expr_statement  */
#line 336 "sql_parser.y"
                                    {
  yylloc.string_length = 0;
  (yyvsp[-2].expr_vct_statement)->push_back((yyvsp[0].expr_statement));
  (yyval.expr_vct_statement) = (yyvsp[-2].expr_vct_statement);
}
#line 2703 "sql_parser.cpp"
    break;

  case 5: /* expr_statement: expr_create_db  */
#line 342 "sql_parser.y"
                                { (yyval.expr_statement) = (yyvsp[0].expr_create_db); }
#line 2709 "sql_parser.cpp"
    break;

  case 6: /* expr_statement: expr_drop_db  */
#line 343 "sql_parser.y"
               { (yyval.expr_statement) = (yyvsp[0].expr_drop_db); }
#line 2715 "sql_parser.cpp"
    break;

  case 7: /* expr_statement: expr_show_db  */
#line 344 "sql_parser.y"
               { (yyval.expr_statement) = (yyvsp[0].expr_show_db); }
#line 2721 "sql_parser.cpp"
    break;

  case 8: /* expr_statement: expr_use_db  */
#line 345 "sql_parser.y"
              { (yyval.expr_statement) = (yyvsp[0].expr_use_db); }
#line 2727 "sql_parser.cpp"
    break;

  case 9: /* expr_statement: expr_create_table  */
#line 346 "sql_parser.y"
                    { (yyval.expr_statement) = (yyvsp[0].expr_create_table); }
#line 2733 "sql_parser.cpp"
    break;

  case 10: /* expr_statement: expr_drop_table  */
#line 347 "sql_parser.y"
                  { (yyval.expr_statement) = (yyvsp[0].expr_drop_table); }
#line 2739 "sql_parser.cpp"
    break;

  case 11: /* expr_statement: expr_show_tables  */
#line 348 "sql_parser.y"
                   { (yyval.expr_statement) = (yyvsp[0].expr_show_tables); }
#line 2745 "sql_parser.cpp"
    break;

  case 12: /* expr_statement: expr_trun_table  */
#line 349 "sql_parser.y"
                  { (yyval.expr_statement) = (yyvsp[0].expr_trun_table); }
#line 2751 "sql_parser.cpp"
    break;

  case 13: /* expr_statement: expr_select  */
#line 350 "sql_parser.y"
              { (yyval.expr_statement) = (yyvsp[0].expr_select); }
#line 2757 "sql_parser.cpp"
    break;

  case 14: /* expr_statement: expr_insert  */
#line 351 "sql_parser.y"
              { (yyval.expr_statement) = (yyvsp[0].expr_insert); }
#line 2763 "sql_parser.cpp"
    break;

  case 15: /* expr_statement: expr_update  */
#line 352 "sql_parser.y"
              { (yyval.expr_statement) = (yyvsp[0].expr_update); }
#line 2769 "sql_parser.cpp"
    break;

  case 16: /* expr_statement: expr_delete  */
#line 353 "sql_parser.y"
              { (yyval.expr_statement) = (yyvsp[0].expr_delete); }
#line 2775 "sql_parser.cpp"
    break;

  case 17: /* expr_statement: expr_transaction  */
#line 354 "sql_parser.y"
                   { (yyval.expr_statement) = (yyvsp[0].expr_transaction); }
#line 2781 "sql_parser.cpp"
    break;

  case 18: /* expr_create_db: CREATE DATABASE opt_not_exists IDENTIFIER  */
#line 360 "sql_parser.y"
                                                           {
  (yyval.expr_create_db) = new ExprCreateDatabase((yyvsp[0].sval), (yyvsp[-1].bval));
}
#line 2789 "sql_parser.cpp"
    break;

  case 19: /* expr_create_db: CREATE SCHEMA opt_not_exists IDENTIFIER  */
#line 363 "sql_parser.y"
                                          {
  (yyval.expr_create_db) = new ExprCreateDatabase((yyvsp[0].sval), (yyvsp[-1].bval));
}
#line 2797 "sql_parser.cpp"
    break;

  case 20: /* expr_drop_db: DROP DATABASE opt_exists IDENTIFIER  */
#line 367 "sql_parser.y"
                                                   {
  (yyval.expr_drop_db) = new ExprDropDatabase((yyvsp[0].sval), (yyvsp[-1].bval));
}
#line 2805 "sql_parser.cpp"
    break;

  case 21: /* expr_drop_db: DROP SCHEMA opt_exists IDENTIFIER  */
#line 370 "sql_parser.y"
                                    {
  (yyval.expr_drop_db) = new ExprDropDatabase((yyvsp[0].sval), (yyvsp[-1].bval));
}
#line 2813 "sql_parser.cpp"
    break;

  case 22: /* expr_show_db: SHOW DATABASES  */
#line 374 "sql_parser.y"
                              {
  (yyval.expr_show_db) = new ExprShowDatabases();
}
#line 2821 "sql_parser.cpp"
    break;

  case 23: /* expr_use_db: USE IDENTIFIER  */
#line 378 "sql_parser.y"
                             {
  (yyval.expr_use_db) = new ExprUseDatabase((yyvsp[0].sval));
}
#line 2829 "sql_parser.cpp"
    break;

  case 24: /* expr_create_table: CREATE TABLE opt_not_exists expr_table '(' expr_vct_create_table_item ')'  */
#line 382 "sql_parser.y"
                                                                                              {
  (yyval.expr_create_table) = new ExprCreateTable((yyvsp[-3].expr_table), (yyvsp[-4].bval), (yyvsp[-1].expr_vct_create_table_item));
}
#line 2837 "sql_parser.cpp"
    break;

  case 25: /* expr_drop_table: DROP TABLE opt_exists expr_table  */
#line 386 "sql_parser.y"
                                                   {
  (yyval.expr_drop_table) = new ExprDropTable((yyvsp[0].expr_table), (yyvsp[-1].bval));
}
#line 2845 "sql_parser.cpp"
    break;

  case 26: /* expr_show_tables: SHOW TABLES FROM IDENTIFIER  */
#line 390 "sql_parser.y"
                                               {
  (yyval.expr_show_tables) = new ExprShowTables((yyvsp[0].sval));
}
#line 2853 "sql_parser.cpp"
    break;

  case 27: /* expr_show_tables: SHOW TABLES  */
#line 393 "sql_parser.y"
              { (yyval.expr_show_tables) = new ExprShowTables(nullptr); }
#line 2859 "sql_parser.cpp"
    break;

  case 28: /* expr_trun_table: TRUNCATE TABLE expr_table  */
#line 395 "sql_parser.y"
                                            {
  (yyval.expr_trun_table) = new ExprTrunTable((yyvsp[0].expr_table));
}
#line 2867 "sql_parser.cpp"
    break;

  case 29: /* expr_transaction: BEGIN  */
#line 399 "sql_parser.y"
                         { (yyval.expr_transaction) = new ExprTransaction(TranAction::TRAN_BEGIN); }
#line 2873 "sql_parser.cpp"
    break;

  case 30: /* expr_transaction: START TRANSACTION  */
#line 400 "sql_parser.y"
                    { (yyval.expr_transaction) = new ExprTransaction(TranAction::TRAN_BEGIN); }
#line 2879 "sql_parser.cpp"
    break;

  case 31: /* expr_transaction: ROLLBACK  */
#line 401 "sql_parser.y"
           { (yyval.expr_transaction) = new ExprTransaction(TranAction::TRAN_ROLLBACK); }
#line 2885 "sql_parser.cpp"
    break;

  case 32: /* expr_transaction: COMMIT  */
#line 402 "sql_parser.y"
         { (yyval.expr_transaction) = new ExprTransaction(TranAction::TRAN_COMMIT); }
#line 2891 "sql_parser.cpp"
    break;

  case 33: /* expr_insert: INSERT INTO expr_table '(' expr_vct_insert_column ')' VALUES expr_vct_elem_row  */
#line 404 "sql_parser.y"
                                                                                             {
  (yyval.expr_insert) = new ExprInsert();
  (yyval.expr_insert)->_exprTable = (yyvsp[-5].expr_table);
  (yyval.expr_insert)->_vctCol = (yyvsp[-3].expr_vct_column);
  (yyval.expr_insert)->_vctRowData = (yyvsp[0].expr_vct_elem_row);
}
#line 2902 "sql_parser.cpp"
    break;

  case 34: /* expr_insert: INSERT INTO expr_table '(' expr_vct_insert_column ')' expr_select  */
#line 410 "sql_parser.y"
                                                                    {
  (yyval.expr_insert) = new ExprInsert();
  (yyval.expr_insert)->_exprTable = (yyvsp[-4].expr_table);
  (yyval.expr_insert)->_vctCol = (yyvsp[-2].expr_vct_column);
  (yyval.expr_insert)->_exprSelect = (yyvsp[0].expr_select);
}
#line 2913 "sql_parser.cpp"
    break;

  case 35: /* expr_delete: DELETE FROM expr_table opt_expr_where opt_expr_order_by opt_expr_limit  */
#line 417 "sql_parser.y"
                                                                                     {
  (yyval.expr_delete) = new ExprDelete();
  (yyval.expr_delete)->_exprTable = (yyvsp[-3].expr_table);
  (yyval.expr_delete)->_exprWhere = (yyvsp[-2].expr_where);
  (yyval.expr_delete)->_exprOrderBy = (yyvsp[-1].expr_order_by);
  (yyval.expr_delete)->_exprLimit = (yyvsp[0].expr_limit);
}
#line 2925 "sql_parser.cpp"
    break;

  case 36: /* expr_update: UPDATE expr_table SET expr_vct_update_column opt_expr_where opt_expr_order_by opt_expr_limit  */
#line 425 "sql_parser.y"
                                                                                                           {
  (yyval.expr_update) = new ExprUpdate();
  (yyval.expr_update)->_exprTable = (yyvsp[-5].expr_table);
  (yyval.expr_update)->_vctCol = (yyvsp[-3].expr_vct_column);
  (yyval.expr_update)->_exprWhere = (yyvsp[-2].expr_where);
  (yyval.expr_update)->_exprOrderBy = (yyvsp[-1].expr_order_by);
  (yyval.expr_update)->_exprLimit = (yyvsp[0].expr_limit);
}
#line 2938 "sql_parser.cpp"
    break;

  case 37: /* expr_select: SELECT opt_distinct opt_expr_vct_select_column opt_expr_vct_table opt_expr_where opt_expr_on opt_expr_group_by opt_expr_order_by opt_expr_limit opt_lock_type  */
#line 434 "sql_parser.y"
                                                                                                                                                                            {
  (yyval.expr_select) = new ExprSelect();
  (yyval.expr_select)->_bDistinct = (yyvsp[-8].bval);
  (yyval.expr_select)->_vctCol = (yyvsp[-7].expr_vct_column);
  (yyval.expr_select)->_vctTable = (yyvsp[-6].expr_vct_table);
  (yyval.expr_select)->_exprWhere = (yyvsp[-5].expr_where);
  (yyval.expr_select)->_exprOn = (yyvsp[-4].expr_on);
  (yyval.expr_select)->_exprGroupBy = (yyvsp[-3].expr_group_by);
  (yyval.expr_select)->_exprOrderBy = (yyvsp[-2].expr_order_by);
  (yyval.expr_select)->_exprLimit = (yyvsp[-1].expr_limit);
  (yyval.expr_select)->_lockType = (yyvsp[0].lock_type);
}
#line 2955 "sql_parser.cpp"
    break;

  case 38: /* opt_expr_vct_select_column: expr_vct_select_column  */
#line 447 "sql_parser.y"
                                                    {
  (yyval.expr_vct_column) = (yyvsp[0].expr_vct_column);
}
#line 2963 "sql_parser.cpp"
    break;

  case 39: /* opt_expr_vct_select_column: '*'  */
#line 450 "sql_parser.y"
      {
   (yyval.expr_vct_column) = new  MVectorPtr<ExprColumn*>();
}
#line 2971 "sql_parser.cpp"
    break;

  case 40: /* expr_vct_select_column: expr_select_column  */
#line 454 "sql_parser.y"
                                            {
  (yyval.expr_vct_column) = new  MVectorPtr<ExprColumn*>();
  (yyval.expr_vct_column)->push_back((yyvsp[0].expr_column));
}
#line 2980 "sql_parser.cpp"
    break;

  case 41: /* expr_vct_select_column: expr_vct_select_column ',' expr_select_column  */
#line 458 "sql_parser.y"
                                                {
  (yyvsp[-2].expr_vct_column)->push_back((yyvsp[0].expr_column));
  (yyval.expr_vct_column) = (yyvsp[-2].expr_vct_column);
}
#line 2989 "sql_parser.cpp"
    break;

  case 42: /* expr_select_column: expr_elem col_alias  */
#line 463 "sql_parser.y"
                                         {
  (yyval.expr_column) = new ExprColumn(nullptr, (yyvsp[-1].expr_elem), (yyvsp[0].sval));
}
#line 2997 "sql_parser.cpp"
    break;

  case 43: /* col_alias: AS IDENTIFIER  */
#line 467 "sql_parser.y"
                          {
  (yyval.sval) = (yyvsp[0].sval);
}
#line 3005 "sql_parser.cpp"
    break;

  case 44: /* col_alias: %empty  */
#line 470 "sql_parser.y"
              { (yyval.sval) = nullptr; }
#line 3011 "sql_parser.cpp"
    break;

  case 45: /* expr_vct_insert_column: expr_insert_column  */
#line 472 "sql_parser.y"
                                            {
  (yyval.expr_vct_column) = new  MVectorPtr<ExprColumn*>();
  (yyval.expr_vct_column)->push_back((yyvsp[0].expr_column));
}
#line 3020 "sql_parser.cpp"
    break;

  case 46: /* expr_vct_insert_column: expr_vct_insert_column ',' expr_insert_column  */
#line 476 "sql_parser.y"
                                                {
  (yyvsp[-2].expr_vct_column)->push_back((yyvsp[0].expr_column));
  (yyval.expr_vct_column) = (yyvsp[-2].expr_vct_column);
}
#line 3029 "sql_parser.cpp"
    break;

  case 47: /* expr_insert_column: IDENTIFIER  */
#line 481 "sql_parser.y"
                                {
  (yyval.expr_column) = new ExprColumn((yyvsp[0].sval), nullptr, nullptr);
}
#line 3037 "sql_parser.cpp"
    break;

  case 48: /* expr_vct_update_column: expr_update_column  */
#line 485 "sql_parser.y"
                                            {
  (yyval.expr_vct_column) = new  MVectorPtr<ExprColumn*>();
  (yyval.expr_vct_column)->push_back((yyvsp[0].expr_column));
}
#line 3046 "sql_parser.cpp"
    break;

  case 49: /* expr_vct_update_column: expr_vct_update_column ',' expr_update_column  */
#line 489 "sql_parser.y"
                                                {
  (yyvsp[-2].expr_vct_column)->push_back((yyvsp[0].expr_column));
  (yyval.expr_vct_column) = (yyvsp[-2].expr_vct_column);
}
#line 3055 "sql_parser.cpp"
    break;

  case 50: /* expr_update_column: IDENTIFIER '=' expr_elem  */
#line 494 "sql_parser.y"
                                              {
  (yyval.expr_column) = new ExprColumn((yyvsp[-2].sval), (yyvsp[0].expr_elem), nullptr);
}
#line 3063 "sql_parser.cpp"
    break;

  case 51: /* expr_table: IDENTIFIER  */
#line 498 "sql_parser.y"
                        {
  (yyval.expr_table) = new ExprTable(nullptr, (yyvsp[0].sval), nullptr);
}
#line 3071 "sql_parser.cpp"
    break;

  case 52: /* expr_table: IDENTIFIER AS IDENTIFIER  */
#line 501 "sql_parser.y"
                           {
  (yyval.expr_table) = new ExprTable(nullptr, (yyvsp[-2].sval), (yyvsp[0].sval));
}
#line 3079 "sql_parser.cpp"
    break;

  case 53: /* expr_table: IDENTIFIER '.' IDENTIFIER  */
#line 504 "sql_parser.y"
                            {
  (yyval.expr_table) = new ExprTable((yyvsp[-2].sval), (yyvsp[0].sval), nullptr);
}
#line 3087 "sql_parser.cpp"
    break;

  case 54: /* expr_table: IDENTIFIER '.' IDENTIFIER AS IDENTIFIER  */
#line 507 "sql_parser.y"
                                          {
  (yyval.expr_table) = new ExprTable((yyvsp[-4].sval), (yyvsp[-2].sval), (yyvsp[0].sval));
}
#line 3095 "sql_parser.cpp"
    break;

  case 55: /* opt_not_exists: IF NOT EXISTS  */
#line 511 "sql_parser.y"
                               { (yyval.bval) = true; }
#line 3101 "sql_parser.cpp"
    break;

  case 56: /* opt_not_exists: %empty  */
#line 512 "sql_parser.y"
              { (yyval.bval) = false; }
#line 3107 "sql_parser.cpp"
    break;

  case 57: /* opt_exists: IF EXISTS  */
#line 514 "sql_parser.y"
                       { (yyval.bval) = true; }
#line 3113 "sql_parser.cpp"
    break;

  case 58: /* opt_exists: %empty  */
#line 515 "sql_parser.y"
              { (yyval.bval) = false; }
#line 3119 "sql_parser.cpp"
    break;

  case 59: /* opt_expr_vct_table: FROM expr_vct_table  */
#line 517 "sql_parser.y"
                                         {
  (yyval.expr_vct_table) = (yyvsp[0].expr_vct_table);
}
#line 3127 "sql_parser.cpp"
    break;

  case 60: /* opt_expr_vct_table: %empty  */
#line 520 "sql_parser.y"
              { (yyval.expr_vct_table) = nullptr; }
#line 3133 "sql_parser.cpp"
    break;

  case 61: /* expr_vct_table: expr_table  */
#line 522 "sql_parser.y"
                            {
  (yyval.expr_vct_table) = new MVectorPtr<ExprTable*>();
  (yyval.expr_vct_table)->push_back((yyvsp[0].expr_table));
}
#line 3142 "sql_parser.cpp"
    break;

  case 62: /* expr_vct_table: opt_expr_vct_table join_type expr_table  */
#line 526 "sql_parser.y"
                                          {
  (yyvsp[0].expr_table)->_joinType = (yyvsp[-1].join_type);
  (yyvsp[-2].expr_vct_table)->push_back((yyvsp[0].expr_table));
  (yyval.expr_vct_table) = (yyvsp[-2].expr_vct_table);
}
#line 3152 "sql_parser.cpp"
    break;

  case 63: /* join_type: INNER  */
#line 532 "sql_parser.y"
                  { (yyval.join_type) = JoinType::INNER_JOIN; }
#line 3158 "sql_parser.cpp"
    break;

  case 64: /* join_type: LEFT OUTER  */
#line 533 "sql_parser.y"
             { (yyval.join_type) = JoinType::LEFT_JOIN; }
#line 3164 "sql_parser.cpp"
    break;

  case 65: /* join_type: LEFT  */
#line 534 "sql_parser.y"
       { (yyval.join_type) = JoinType::LEFT_JOIN; }
#line 3170 "sql_parser.cpp"
    break;

  case 66: /* join_type: RIGHT OUTER  */
#line 535 "sql_parser.y"
              { (yyval.join_type) = JoinType::RIGHT_JOIN; }
#line 3176 "sql_parser.cpp"
    break;

  case 67: /* join_type: RIGHT  */
#line 536 "sql_parser.y"
        { (yyval.join_type) = JoinType::RIGHT_JOIN; }
#line 3182 "sql_parser.cpp"
    break;

  case 68: /* join_type: FULL OUTER  */
#line 537 "sql_parser.y"
             { (yyval.join_type) = JoinType::OUTTER_JOIN; }
#line 3188 "sql_parser.cpp"
    break;

  case 69: /* join_type: OUTER  */
#line 538 "sql_parser.y"
        { (yyval.join_type) = JoinType::OUTTER_JOIN; }
#line 3194 "sql_parser.cpp"
    break;

  case 70: /* join_type: FULL  */
#line 539 "sql_parser.y"
       { (yyval.join_type) = JoinType::OUTTER_JOIN; }
#line 3200 "sql_parser.cpp"
    break;

  case 71: /* join_type: CROSS  */
#line 540 "sql_parser.y"
        { (yyval.join_type) = JoinType::INNER_JOIN; }
#line 3206 "sql_parser.cpp"
    break;

  case 72: /* join_type: ','  */
#line 541 "sql_parser.y"
      { (yyval.join_type) = JoinType::INNER_JOIN; }
#line 3212 "sql_parser.cpp"
    break;

  case 73: /* expr_vct_col_name: IDENTIFIER  */
#line 543 "sql_parser.y"
                               {
  (yyval.expr_vct_str) = new MVectorPtr<MString*>();
  (yyval.expr_vct_str)->push_back((yyvsp[0].sval));
}
#line 3221 "sql_parser.cpp"
    break;

  case 74: /* expr_vct_col_name: expr_vct_col_name ',' IDENTIFIER  */
#line 547 "sql_parser.y"
                                   {
  (yyvsp[-2].expr_vct_str)->push_back((yyvsp[0].sval));
  (yyval.expr_vct_str) = (yyvsp[-2].expr_vct_str);
}
#line 3230 "sql_parser.cpp"
    break;

  case 77: /* expr_vct_create_table_item: expr_create_table_item  */
#line 555 "sql_parser.y"
                                                    {
  (yyval.expr_vct_create_table_item) = new MVectorPtr<ExprCreateTableItem*>();
  (yyval.expr_vct_create_table_item)->push_back((yyvsp[0].expr_create_table_item));
}
#line 3239 "sql_parser.cpp"
    break;

  case 78: /* expr_vct_create_table_item: expr_vct_create_table_item ',' expr_create_table_item  */
#line 559 "sql_parser.y"
                                                        {
  (yyvsp[-2].expr_vct_create_table_item)->push_back((yyvsp[0].expr_create_table_item));
  (yyval.expr_vct_create_table_item) = (yyvsp[-2].expr_vct_create_table_item);
}
#line 3248 "sql_parser.cpp"
    break;

  case 79: /* expr_create_table_item: IDENTIFIER expr_data_type col_nullable default_col_dv auto_increment opt_index_type table_comment  */
#line 564 "sql_parser.y"
                                                                                                                           {
  ExprColumnItem *item = new ExprColumnItem();
  item->_colName = (yyvsp[-6].sval);
  item->_dataType = (yyvsp[-5].expr_data_type)->_dataType;
  item->_maxLength = (yyvsp[-5].expr_data_type)->_maxLen;
  delete (yyvsp[-5].expr_data_type);
  item->_nullable = (yyvsp[-4].bval);
  item->_defaultVal = (yyvsp[-3].data_value);
  item->_autoInc = (yyvsp[-2].bval);
  item->_indexType = (yyvsp[-1].index_type);
  item->_comment = (yyvsp[0].sval);
  (yyval.expr_create_table_item) = item;
}
#line 3266 "sql_parser.cpp"
    break;

  case 80: /* expr_create_table_item: index_type IDENTIFIER '(' expr_vct_col_name ')'  */
#line 577 "sql_parser.y"
                                                  {
  (yyval.expr_create_table_item) = new ExprTableConstraint((yyvsp[-3].sval), (yyvsp[-4].index_type), (yyvsp[-1].expr_vct_str));
}
#line 3274 "sql_parser.cpp"
    break;

  case 81: /* expr_create_table_item: index_type '(' expr_vct_col_name ')'  */
#line 580 "sql_parser.y"
                                       {
  (yyval.expr_create_table_item) = new ExprTableConstraint(nullptr, (yyvsp[-3].index_type), (yyvsp[-1].expr_vct_str));
}
#line 3282 "sql_parser.cpp"
    break;

  case 82: /* expr_data_type: BIGINT  */
#line 584 "sql_parser.y"
                        { (yyval.expr_data_type) = new ExprDataType(DataType::LONG); }
#line 3288 "sql_parser.cpp"
    break;

  case 83: /* expr_data_type: BOOLEAN  */
#line 585 "sql_parser.y"
          { (yyval.expr_data_type) = new ExprDataType(DataType::BOOL); }
#line 3294 "sql_parser.cpp"
    break;

  case 84: /* expr_data_type: CHAR '(' INTVAL ')'  */
#line 586 "sql_parser.y"
                      { (yyval.expr_data_type) = new ExprDataType(DataType::FIXCHAR, (yyvsp[-1].ival)); }
#line 3300 "sql_parser.cpp"
    break;

  case 85: /* expr_data_type: DOUBLE  */
#line 587 "sql_parser.y"
         { (yyval.expr_data_type) = new ExprDataType(DataType::DOUBLE); }
#line 3306 "sql_parser.cpp"
    break;

  case 86: /* expr_data_type: FLOAT  */
#line 588 "sql_parser.y"
        { (yyval.expr_data_type) = new ExprDataType(DataType::FLOAT); }
#line 3312 "sql_parser.cpp"
    break;

  case 87: /* expr_data_type: INT  */
#line 589 "sql_parser.y"
      { (yyval.expr_data_type) = new ExprDataType(DataType::INT); }
#line 3318 "sql_parser.cpp"
    break;

  case 88: /* expr_data_type: INTEGER  */
#line 590 "sql_parser.y"
          { (yyval.expr_data_type) = new ExprDataType(DataType::INT); }
#line 3324 "sql_parser.cpp"
    break;

  case 89: /* expr_data_type: LONG  */
#line 591 "sql_parser.y"
       { (yyval.expr_data_type) = new ExprDataType(DataType::LONG); }
#line 3330 "sql_parser.cpp"
    break;

  case 90: /* expr_data_type: REAL  */
#line 592 "sql_parser.y"
       { (yyval.expr_data_type) = new ExprDataType(DataType::DOUBLE); }
#line 3336 "sql_parser.cpp"
    break;

  case 91: /* expr_data_type: SMALLINT  */
#line 593 "sql_parser.y"
           { (yyval.expr_data_type) = new ExprDataType(DataType::SHORT); }
#line 3342 "sql_parser.cpp"
    break;

  case 92: /* expr_data_type: VARCHAR '(' INTVAL ')'  */
#line 594 "sql_parser.y"
                         { (yyval.expr_data_type) = new ExprDataType(DataType::VARCHAR, (yyvsp[-1].ival)); }
#line 3348 "sql_parser.cpp"
    break;

  case 93: /* col_nullable: NULL  */
#line 596 "sql_parser.y"
                    { (yyval.bval) = true; }
#line 3354 "sql_parser.cpp"
    break;

  case 94: /* col_nullable: NOT NULL  */
#line 597 "sql_parser.y"
           { (yyval.bval) = false; }
#line 3360 "sql_parser.cpp"
    break;

  case 95: /* col_nullable: %empty  */
#line 598 "sql_parser.y"
              { (yyval.bval) = true; }
#line 3366 "sql_parser.cpp"
    break;

  case 96: /* default_col_dv: DEFAULT const_dv  */
#line 600 "sql_parser.y"
                                  { (yyval.data_value) = (yyvsp[0].data_value); }
#line 3372 "sql_parser.cpp"
    break;

  case 97: /* default_col_dv: %empty  */
#line 601 "sql_parser.y"
              { (yyval.data_value) = nullptr; }
#line 3378 "sql_parser.cpp"
    break;

  case 98: /* auto_increment: AUTO_INCREMENT  */
#line 603 "sql_parser.y"
                                { (yyval.bval) = true; }
#line 3384 "sql_parser.cpp"
    break;

  case 99: /* auto_increment: %empty  */
#line 604 "sql_parser.y"
               { (yyval.bval) = false; }
#line 3390 "sql_parser.cpp"
    break;

  case 100: /* opt_index_type: index_type  */
#line 606 "sql_parser.y"
                            { (yyval.index_type) = (yyvsp[0].index_type); }
#line 3396 "sql_parser.cpp"
    break;

  case 101: /* opt_index_type: %empty  */
#line 607 "sql_parser.y"
              { (yyval.index_type) = IndexType::UNKNOWN; }
#line 3402 "sql_parser.cpp"
    break;

  case 102: /* index_type: PRIMARY KEY  */
#line 609 "sql_parser.y"
                         { (yyval.index_type) = IndexType::PRIMARY; }
#line 3408 "sql_parser.cpp"
    break;

  case 103: /* index_type: PRIMARY  */
#line 610 "sql_parser.y"
          { (yyval.index_type) = IndexType::PRIMARY; }
#line 3414 "sql_parser.cpp"
    break;

  case 104: /* index_type: UNIQUE KEY  */
#line 611 "sql_parser.y"
             { (yyval.index_type) = IndexType::UNIQUE; }
#line 3420 "sql_parser.cpp"
    break;

  case 105: /* index_type: UNIQUE  */
#line 612 "sql_parser.y"
         { (yyval.index_type) = IndexType::UNIQUE; }
#line 3426 "sql_parser.cpp"
    break;

  case 106: /* index_type: KEY  */
#line 613 "sql_parser.y"
      { (yyval.index_type) = IndexType::NON_UNIQUE; }
#line 3432 "sql_parser.cpp"
    break;

  case 107: /* index_type: INDEX  */
#line 614 "sql_parser.y"
        { (yyval.index_type) = IndexType::NON_UNIQUE; }
#line 3438 "sql_parser.cpp"
    break;

  case 108: /* table_comment: COMMENT STRING  */
#line 616 "sql_parser.y"
                               { (yyval.sval) = (yyvsp[0].sval); }
#line 3444 "sql_parser.cpp"
    break;

  case 109: /* table_comment: %empty  */
#line 617 "sql_parser.y"
               { (yyval.sval) = nullptr; }
#line 3450 "sql_parser.cpp"
    break;

  case 110: /* expr_vct_elem_row: '(' expr_elem_row ')'  */
#line 619 "sql_parser.y"
                                          {
  (yyval.expr_vct_elem_row) = new MVectorPtr<MVectorPtr<ExprElem*>*>();
  (yyval.expr_vct_elem_row)->push_back((yyvsp[-1].expr_elem_row));
}
#line 3459 "sql_parser.cpp"
    break;

  case 111: /* expr_vct_elem_row: expr_vct_elem_row ',' '(' expr_elem_row ')'  */
#line 623 "sql_parser.y"
                                              {
  (yyvsp[-4].expr_vct_elem_row)->push_back((yyvsp[-1].expr_elem_row));
  (yyval.expr_vct_elem_row) = (yyvsp[-4].expr_vct_elem_row);
}
#line 3468 "sql_parser.cpp"
    break;

  case 112: /* expr_elem_row: expr_elem  */
#line 628 "sql_parser.y"
                          {
  (yyval.expr_elem_row) = new MVectorPtr<ExprElem*>();
  (yyval.expr_elem_row)->push_back((yyvsp[0].expr_elem));
}
#line 3477 "sql_parser.cpp"
    break;

  case 113: /* expr_elem_row: expr_elem_row ',' expr_elem  */
#line 632 "sql_parser.y"
                              {
  (yyvsp[-2].expr_elem_row)->push_back((yyvsp[0].expr_elem));
  (yyval.expr_elem_row) = (yyvsp[-2].expr_elem_row);
}
#line 3486 "sql_parser.cpp"
    break;

  case 114: /* opt_expr_where: WHERE expr_logic  */
#line 637 "sql_parser.y"
                                  {
  (yyval.expr_where) = new ExprWhere((yyvsp[0].expr_logic));
}
#line 3494 "sql_parser.cpp"
    break;

  case 115: /* opt_expr_where: %empty  */
#line 640 "sql_parser.y"
              { (yyval.expr_where) = nullptr;}
#line 3500 "sql_parser.cpp"
    break;

  case 116: /* opt_expr_on: ON expr_logic  */
#line 642 "sql_parser.y"
                            {
  (yyval.expr_on) = new ExprOn((yyvsp[0].expr_logic));
}
#line 3508 "sql_parser.cpp"
    break;

  case 117: /* opt_expr_on: %empty  */
#line 645 "sql_parser.y"
              { (yyval.expr_on) = nullptr; }
#line 3514 "sql_parser.cpp"
    break;

  case 118: /* opt_expr_order_by: ORDER BY expr_vct_order_item  */
#line 647 "sql_parser.y"
                                                 {
  (yyval.expr_order_by) = new ExprOrderBy((yyvsp[0].expr_vct_order_item));
}
#line 3522 "sql_parser.cpp"
    break;

  case 119: /* opt_expr_order_by: %empty  */
#line 650 "sql_parser.y"
              { (yyval.expr_order_by) = nullptr; }
#line 3528 "sql_parser.cpp"
    break;

  case 120: /* expr_vct_order_item: expr_order_item  */
#line 652 "sql_parser.y"
                                      {
  (yyval.expr_vct_order_item) = new MVectorPtr<ExprOrderItem*>();
  (yyval.expr_vct_order_item)->push_back((yyvsp[0].expr_order_item));
}
#line 3537 "sql_parser.cpp"
    break;

  case 121: /* expr_vct_order_item: expr_vct_order_item ',' expr_order_item  */
#line 656 "sql_parser.y"
                                          {
  (yyvsp[-2].expr_vct_order_item)->push_back((yyvsp[0].expr_order_item));
  (yyval.expr_vct_order_item) = (yyvsp[-2].expr_vct_order_item);
}
#line 3546 "sql_parser.cpp"
    break;

  case 122: /* expr_order_item: IDENTIFIER opt_order_direction  */
#line 661 "sql_parser.y"
                                                 {
  (yyval.expr_order_item) = new ExprOrderItem((yyvsp[-1].sval), (yyvsp[0].bval));
}
#line 3554 "sql_parser.cpp"
    break;

  case 123: /* opt_order_direction: ASC  */
#line 665 "sql_parser.y"
                          { (yyval.bval) = true; }
#line 3560 "sql_parser.cpp"
    break;

  case 124: /* opt_order_direction: DESC  */
#line 666 "sql_parser.y"
       { (yyval.bval) = false; }
#line 3566 "sql_parser.cpp"
    break;

  case 125: /* opt_order_direction: %empty  */
#line 667 "sql_parser.y"
              { (yyval.bval) = true; }
#line 3572 "sql_parser.cpp"
    break;

  case 126: /* opt_expr_limit: LIMIT INTVAL  */
#line 669 "sql_parser.y"
                              {
  (yyval.expr_limit) = new ExprLimit(0, (yyvsp[0].ival));
}
#line 3580 "sql_parser.cpp"
    break;

  case 127: /* opt_expr_limit: LIMIT INTVAL ',' INTVAL  */
#line 672 "sql_parser.y"
                          {
  (yyval.expr_limit) = new ExprLimit((yyvsp[-2].ival), (yyvsp[0].ival));
}
#line 3588 "sql_parser.cpp"
    break;

  case 128: /* opt_expr_limit: %empty  */
#line 675 "sql_parser.y"
              { (yyval.expr_limit) = nullptr; }
#line 3594 "sql_parser.cpp"
    break;

  case 129: /* opt_expr_group_by: GROUP BY expr_vct_col_name opt_expr_having  */
#line 677 "sql_parser.y"
                                                               {
  (yyval.expr_group_by) = new ExprGroupBy((yyvsp[-1].expr_vct_str), (yyvsp[0].expr_having));
}
#line 3602 "sql_parser.cpp"
    break;

  case 130: /* opt_expr_group_by: %empty  */
#line 680 "sql_parser.y"
              { (yyval.expr_group_by) = nullptr; }
#line 3608 "sql_parser.cpp"
    break;

  case 131: /* opt_expr_having: HAVING expr_logic  */
#line 682 "sql_parser.y"
                                    {
  (yyval.expr_having) = new ExprHaving((yyvsp[0].expr_logic));
}
#line 3616 "sql_parser.cpp"
    break;

  case 132: /* opt_expr_having: %empty  */
#line 685 "sql_parser.y"
              { (yyval.expr_having) = nullptr; }
#line 3622 "sql_parser.cpp"
    break;

  case 133: /* opt_distinct: DISTINCT  */
#line 687 "sql_parser.y"
                        { (yyval.bval) = true; }
#line 3628 "sql_parser.cpp"
    break;

  case 134: /* opt_distinct: %empty  */
#line 688 "sql_parser.y"
              { (yyval.bval) = false; }
#line 3634 "sql_parser.cpp"
    break;

  case 135: /* opt_lock_type: FOR SHARE  */
#line 690 "sql_parser.y"
                          { (yyval.lock_type) = LockType::SHARE_LOCK; }
#line 3640 "sql_parser.cpp"
    break;

  case 136: /* opt_lock_type: FOR UPDATE  */
#line 691 "sql_parser.y"
             { (yyval.lock_type) = LockType::WRITE_LOCK; }
#line 3646 "sql_parser.cpp"
    break;

  case 137: /* opt_lock_type: %empty  */
#line 692 "sql_parser.y"
              { (yyval.lock_type) = LockType::NO_LOCK; }
#line 3652 "sql_parser.cpp"
    break;

  case 138: /* expr_array: '(' expr_vct_const ')'  */
#line 694 "sql_parser.y"
                                    { (yyval.expr_array) = (yyvsp[-1].expr_array); }
#line 3658 "sql_parser.cpp"
    break;

  case 139: /* expr_vct_const: const_dv  */
#line 695 "sql_parser.y"
                          {
  (yyval.expr_array) = new ExprArray();
  (yyval.expr_array)->AddElem((yyvsp[0].data_value));
}
#line 3667 "sql_parser.cpp"
    break;

  case 140: /* expr_vct_const: expr_vct_const ',' const_dv  */
#line 699 "sql_parser.y"
                              {
   (yyvsp[-2].expr_array)->AddElem((yyvsp[0].data_value));
   (yyval.expr_array) = (yyvsp[-2].expr_array);
}
#line 3676 "sql_parser.cpp"
    break;

  case 141: /* expr_elem: expr_logic  */
#line 704 "sql_parser.y"
                       { (yyval.expr_elem) = (yyvsp[0].expr_logic); }
#line 3682 "sql_parser.cpp"
    break;

  case 142: /* expr_elem: expr_data  */
#line 705 "sql_parser.y"
            { (yyval.expr_elem) = (yyvsp[0].expr_data); }
#line 3688 "sql_parser.cpp"
    break;

  case 143: /* expr_elem: expr_aggr  */
#line 706 "sql_parser.y"
            { (yyval.expr_elem) = (yyvsp[0].expr_aggr); }
#line 3694 "sql_parser.cpp"
    break;

  case 153: /* expr_data: '(' expr_data ')'  */
#line 709 "sql_parser.y"
                    { (yyval.expr_data) = (yyvsp[-1].expr_data); }
#line 3700 "sql_parser.cpp"
    break;

  case 154: /* expr_const: const_dv  */
#line 711 "sql_parser.y"
                      { (yyval.expr_data) = new ExprConst((yyvsp[0].data_value)); }
#line 3706 "sql_parser.cpp"
    break;

  case 155: /* expr_field: IDENTIFIER  */
#line 713 "sql_parser.y"
                        { (yyval.expr_data) = new ExprField(nullptr, (yyvsp[0].sval)); }
#line 3712 "sql_parser.cpp"
    break;

  case 156: /* expr_field: IDENTIFIER '.' IDENTIFIER  */
#line 714 "sql_parser.y"
                            {(yyval.expr_data) = new ExprField((yyvsp[-2].sval), (yyvsp[0].sval));}
#line 3718 "sql_parser.cpp"
    break;

  case 157: /* expr_param: '?'  */
#line 716 "sql_parser.y"
                 {
  ExprParameter *ep =  new ExprParameter();
  ep->_paraPos = yyloc.param_list.size();
  (yyval.expr_data) = ep;
  yyloc.param_list.push_back(ep);
}
#line 3729 "sql_parser.cpp"
    break;

  case 158: /* expr_add: expr_data '+' expr_data  */
#line 723 "sql_parser.y"
                                   { (yyval.expr_data) = new ExprAdd((yyvsp[-2].expr_data), (yyvsp[0].expr_data)); }
#line 3735 "sql_parser.cpp"
    break;

  case 159: /* expr_sub: expr_data '-' expr_data  */
#line 725 "sql_parser.y"
                                   { (yyval.expr_data) = new ExprSub((yyvsp[-2].expr_data), (yyvsp[0].expr_data)); }
#line 3741 "sql_parser.cpp"
    break;

  case 160: /* expr_mul: expr_data '*' expr_data  */
#line 727 "sql_parser.y"
                                   { (yyval.expr_data) = new ExprMul((yyvsp[-2].expr_data), (yyvsp[0].expr_data)); }
#line 3747 "sql_parser.cpp"
    break;

  case 161: /* expr_div: expr_data '/' expr_data  */
#line 729 "sql_parser.y"
                                   { (yyval.expr_data) = new ExprDiv((yyvsp[-2].expr_data), (yyvsp[0].expr_data)); }
#line 3753 "sql_parser.cpp"
    break;

  case 162: /* expr_minus: '-' expr_data  */
#line 731 "sql_parser.y"
                           { (yyval.expr_data) = new ExprMinus((yyvsp[0].expr_data)); }
#line 3759 "sql_parser.cpp"
    break;

  case 163: /* expr_func: IDENTIFIER '(' opt_expr_vct_data ')'  */
#line 733 "sql_parser.y"
                                                 {
  (yyval.expr_data) = new ExprFunc((yyvsp[-3].sval), (yyvsp[-1].expr_vct_data));
}
#line 3767 "sql_parser.cpp"
    break;

  case 164: /* opt_expr_vct_data: expr_vct_data  */
#line 737 "sql_parser.y"
                                  {
  (yyval.expr_vct_data) = (yyvsp[0].expr_vct_data);
}
#line 3775 "sql_parser.cpp"
    break;

  case 165: /* opt_expr_vct_data: %empty  */
#line 740 "sql_parser.y"
              { (yyval.expr_vct_data) = nullptr; }
#line 3781 "sql_parser.cpp"
    break;

  case 166: /* expr_vct_data: expr_data  */
#line 742 "sql_parser.y"
                          {
  (yyval.expr_vct_data) = new  MVectorPtr<ExprData*>();
  (yyval.expr_vct_data)->push_back((yyvsp[0].expr_data));
}
#line 3790 "sql_parser.cpp"
    break;

  case 167: /* expr_vct_data: expr_vct_data ',' expr_data  */
#line 746 "sql_parser.y"
                              {
  (yyvsp[-2].expr_vct_data)->push_back((yyvsp[0].expr_data));
  (yyval.expr_vct_data) = (yyvsp[-2].expr_vct_data);
}
#line 3799 "sql_parser.cpp"
    break;

  case 173: /* const_string: STRING  */
#line 752 "sql_parser.y"
                      {
  (yyval.data_value) = new DataValueVarChar((yyvsp[0].sval)->c_str(), (yyvsp[0].sval)->size());
  delete (yyvsp[0].sval);
}
#line 3808 "sql_parser.cpp"
    break;

  case 174: /* const_bool: TRUE  */
#line 757 "sql_parser.y"
                  { (yyval.data_value) = new DataValueBool(true); }
#line 3814 "sql_parser.cpp"
    break;

  case 175: /* const_bool: FALSE  */
#line 758 "sql_parser.y"
        { (yyval.data_value) = new DataValueBool(false); }
#line 3820 "sql_parser.cpp"
    break;

  case 176: /* const_double: FLOATVAL  */
#line 760 "sql_parser.y"
                        { (yyval.data_value) = new DataValueDouble((yyvsp[0].fval)); }
#line 3826 "sql_parser.cpp"
    break;

  case 177: /* const_int: INTVAL  */
#line 762 "sql_parser.y"
                   { (yyval.data_value) = new DataValueLong((yyvsp[0].ival)); }
#line 3832 "sql_parser.cpp"
    break;

  case 178: /* const_null: NULL  */
#line 764 "sql_parser.y"
                  { (yyval.data_value) = new DataValueNull(); }
#line 3838 "sql_parser.cpp"
    break;

  case 185: /* expr_logic: expr_and  */
#line 767 "sql_parser.y"
           { (yyval.expr_logic) = (yyvsp[0].expr_and); }
#line 3844 "sql_parser.cpp"
    break;

  case 186: /* expr_logic: expr_or  */
#line 768 "sql_parser.y"
          { (yyval.expr_logic) = (yyvsp[0].expr_or); }
#line 3850 "sql_parser.cpp"
    break;

  case 187: /* expr_logic: '(' expr_logic ')'  */
#line 769 "sql_parser.y"
                     { (yyval.expr_logic) = (yyvsp[-1].expr_logic); }
#line 3856 "sql_parser.cpp"
    break;

  case 188: /* expr_cmp: expr_data comp_type expr_data  */
#line 771 "sql_parser.y"
                                         {
  (yyval.expr_logic) = new ExprComp((yyvsp[-1].comp_type), (yyvsp[-2].expr_data), (yyvsp[0].expr_data));
}
#line 3864 "sql_parser.cpp"
    break;

  case 189: /* comp_type: '='  */
#line 775 "sql_parser.y"
                { (yyval.comp_type) = CompType::EQ; }
#line 3870 "sql_parser.cpp"
    break;

  case 190: /* comp_type: '>'  */
#line 776 "sql_parser.y"
      { (yyval.comp_type) = CompType::GT; }
#line 3876 "sql_parser.cpp"
    break;

  case 191: /* comp_type: '<'  */
#line 777 "sql_parser.y"
      { (yyval.comp_type) = CompType::LT; }
#line 3882 "sql_parser.cpp"
    break;

  case 192: /* comp_type: GE  */
#line 778 "sql_parser.y"
     { (yyval.comp_type) = CompType::GE; }
#line 3888 "sql_parser.cpp"
    break;

  case 193: /* comp_type: LE  */
#line 779 "sql_parser.y"
     { (yyval.comp_type) = CompType::LE; }
#line 3894 "sql_parser.cpp"
    break;

  case 194: /* comp_type: NE  */
#line 780 "sql_parser.y"
     { (yyval.comp_type) = CompType::NE; }
#line 3900 "sql_parser.cpp"
    break;

  case 195: /* comp_type: EQ  */
#line 781 "sql_parser.y"
     { (yyval.comp_type) = CompType::EQ; }
#line 3906 "sql_parser.cpp"
    break;

  case 196: /* expr_in_not: expr_data IN expr_array  */
#line 783 "sql_parser.y"
                                      {
  (yyval.expr_logic) = new ExprInNot((yyvsp[-2].expr_data), (yyvsp[0].expr_array), true);
}
#line 3914 "sql_parser.cpp"
    break;

  case 197: /* expr_in_not: expr_data NOT IN expr_array  */
#line 786 "sql_parser.y"
                              {
  (yyval.expr_logic) = new ExprInNot((yyvsp[-3].expr_data), (yyvsp[0].expr_array), false);
}
#line 3922 "sql_parser.cpp"
    break;

  case 198: /* expr_is_null_not: expr_data IS NULL  */
#line 790 "sql_parser.y"
                                     { (yyval.expr_logic) = new ExprIsNullNot((yyvsp[-2].expr_data), true); }
#line 3928 "sql_parser.cpp"
    break;

  case 199: /* expr_is_null_not: expr_data IS NOT NULL  */
#line 791 "sql_parser.y"
                        { (yyval.expr_logic) = new ExprIsNullNot((yyvsp[-3].expr_data), false); }
#line 3934 "sql_parser.cpp"
    break;

  case 200: /* expr_between: expr_data BETWEEN expr_data AND expr_data  */
#line 793 "sql_parser.y"
                                                         {
  (yyval.expr_logic) = new ExprBetween((yyvsp[-4].expr_data), (yyvsp[-2].expr_data), (yyvsp[0].expr_data));
}
#line 3942 "sql_parser.cpp"
    break;

  case 201: /* expr_like: expr_data LIKE const_string  */
#line 797 "sql_parser.y"
                                        { (yyval.expr_logic) = new ExprLike((yyvsp[-2].expr_data), (yyvsp[0].data_value), true); }
#line 3948 "sql_parser.cpp"
    break;

  case 202: /* expr_like: expr_data NOT LIKE const_string  */
#line 798 "sql_parser.y"
                                  { (yyval.expr_logic) = new ExprLike((yyvsp[-3].expr_data), (yyvsp[0].data_value), false); }
#line 3954 "sql_parser.cpp"
    break;

  case 203: /* expr_not: NOT expr_logic  */
#line 800 "sql_parser.y"
                          { (yyval.expr_logic) = new ExprNot((yyvsp[0].expr_logic)); }
#line 3960 "sql_parser.cpp"
    break;

  case 204: /* expr_and: expr_logic AND expr_logic  */
#line 802 "sql_parser.y"
                                     {
  if ((yyvsp[-2].expr_logic)->GetType()== ExprType::EXPR_AND) {
    (yyval.expr_and) = (ExprAnd*)(yyvsp[-2].expr_logic);
    (yyval.expr_and)->_vctChild.push_back((yyvsp[0].expr_logic));
  } else {
    (yyval.expr_and) = new ExprAnd();
    (yyval.expr_and)->_vctChild.push_back((yyvsp[-2].expr_logic));
    (yyval.expr_and)->_vctChild.push_back((yyvsp[0].expr_logic));
  }
}
#line 3975 "sql_parser.cpp"
    break;

  case 205: /* expr_or: expr_logic OR expr_logic  */
#line 813 "sql_parser.y"
                                   {
  if ((yyvsp[-2].expr_logic)->GetType()== ExprType::EXPR_OR) {
    (yyval.expr_or) = (ExprOr*)(yyvsp[-2].expr_logic);
    (yyval.expr_or)->_vctChild.push_back((yyvsp[0].expr_logic));
  } else {
    (yyval.expr_or) = new ExprOr();
    (yyval.expr_or)->_vctChild.push_back((yyvsp[-2].expr_logic));
    (yyval.expr_or)->_vctChild.push_back((yyvsp[0].expr_logic));
  }
}
#line 3990 "sql_parser.cpp"
    break;

  case 211: /* expr_aggr: '(' expr_aggr ')'  */
#line 825 "sql_parser.y"
                    { (yyval.expr_aggr) = (yyvsp[-1].expr_aggr); }
#line 3996 "sql_parser.cpp"
    break;

  case 212: /* expr_count: COUNT '(' expr_data ')'  */
#line 827 "sql_parser.y"
                                     {
  (yyval.expr_aggr) = new ExprCount((yyvsp[-1].expr_data), false);
}
#line 4004 "sql_parser.cpp"
    break;

  case 213: /* expr_count: COUNT '(' '*' ')'  */
#line 830 "sql_parser.y"
                    {
  (yyval.expr_aggr) = new ExprCount(nullptr, true);
}
#line 4012 "sql_parser.cpp"
    break;

  case 214: /* expr_sum: SUM '(' expr_data ')'  */
#line 834 "sql_parser.y"
                                 {
  (yyval.expr_aggr) = new ExprSum((yyvsp[-1].expr_data));
}
#line 4020 "sql_parser.cpp"
    break;

  case 215: /* expr_max: MAX '(' expr_data ')'  */
#line 838 "sql_parser.y"
                                 {
  (yyval.expr_aggr) = new ExprMax((yyvsp[-1].expr_data));
}
#line 4028 "sql_parser.cpp"
    break;

  case 216: /* expr_min: MIN '(' expr_data ')'  */
#line 842 "sql_parser.y"
                                 {
  (yyval.expr_aggr) = new ExprMin((yyvsp[-1].expr_data));
}
#line 4036 "sql_parser.cpp"
    break;

  case 217: /* expr_avg: AVERAGE '(' expr_data ')'  */
#line 846 "sql_parser.y"
                                     {
  (yyval.expr_aggr) = new ExprAvg((yyvsp[-1].expr_data));
}
#line 4044 "sql_parser.cpp"
    break;


#line 4048 "sql_parser.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == SQL_DB_EMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, result, scanner, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= SQL_YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == SQL_YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, result, scanner);
          yychar = SQL_DB_EMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, result, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, result, scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != SQL_DB_EMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, result, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, result, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 851 "sql_parser.y"

    // clang-format on
    /*********************************
 ** Section 4: Additional C code
 *********************************/

    /* empty */
