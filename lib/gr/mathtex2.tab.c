/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* First part of user prologue.  */
#line 11 "mathtex2.y"

#include "mathtex2.h"


#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

int yylex(void);
void yyerror(char const *);

#line 82 "mathtex2.tab.c"

#ifndef YY_NULLPTR
#if defined __cplusplus
#if 201103L <= __cplusplus
#define YY_NULLPTR nullptr
#else
#define YY_NULLPTR 0
#endif
#else
#define YY_NULLPTR ((void *)0)
#endif
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
#undef YYERROR_VERBOSE
#define YYERROR_VERBOSE 1
#else
#define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
enum yytokentype
{
  SINGLE_SYMBOL = 258,
  SUBSUPEROP = 259,
  APOSTROPHE = 260,
  FRAC = 261,
  DFRAC = 262,
  STACKREL = 263,
  BINOM = 264,
  GENFRAC = 265,
  SNOWFLAKE = 266,
  ACCENT = 267,
  UNKNOWN_SYMBOL = 268,
  FONT = 269,
  LATEXFONT = 270,
  LATEXTEXT = 271,
  FUNCTION = 272,
  C_OVER_C = 273,
  SPACE = 274,
  HSPACE = 275,
  LEFT = 276,
  RIGHT = 277,
  LEFT_DELIM = 278,
  AMBI_DELIM = 279,
  RIGHT_DELIM = 280,
  LBRACE = 281,
  RBRACE = 282,
  PIPE = 283,
  OPERATORNAME = 284,
  OVERLINE = 285,
  SQRT = 286,
  DIGIT = 287,
  PLUSMINUS = 288
};
#endif

/* Value type.  */
#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED
typedef ParserNode YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse(void);


#ifdef short
#undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
#ifdef __SIZE_TYPE__
#define YYSIZE_T __SIZE_TYPE__
#elif defined size_t
#define YYSIZE_T size_t
#elif !defined YYSIZE_T
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#else
#define YYSIZE_T unsigned
#endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T)-1)

#ifndef YY_
#if defined YYENABLE_NLS && YYENABLE_NLS
#if ENABLE_NLS
#include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#define YY_(Msgid) dgettext("bison-runtime", Msgid)
#endif
#endif
#ifndef YY_
#define YY_(Msgid) Msgid
#endif
#endif

#ifndef YY_ATTRIBUTE
#if (defined __GNUC__ && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__))) || \
    defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#define YY_ATTRIBUTE(Spec) __attribute__(Spec)
#else
#define YY_ATTRIBUTE(Spec) /* empty */
#endif
#endif

#ifndef YY_ATTRIBUTE_PURE
#define YY_ATTRIBUTE_PURE YY_ATTRIBUTE((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
#define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if !defined lint || defined __GNUC__
#define YYUSE(E) ((void)(E))
#else
#define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && !defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                            \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"") \
      _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#define YY_IGNORE_MAYBE_UNINITIALIZED_END _Pragma("GCC diagnostic pop")
#else
#define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
#define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#define YY_ASSERT(E) ((void)(0 && (E)))

#if !defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#ifdef YYSTACK_USE_ALLOCA
#if YYSTACK_USE_ALLOCA
#ifdef __GNUC__
#define YYSTACK_ALLOC __builtin_alloca
#elif defined __BUILTIN_VA_ARG_INCR
#include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#elif defined _AIX
#define YYSTACK_ALLOC __alloca
#elif defined _MSC_VER
#include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#define alloca _alloca
#else
#define YYSTACK_ALLOC alloca
#if !defined _ALLOCA_H && !defined EXIT_SUCCESS
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
/* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#endif
#endif
#endif

#ifdef YYSTACK_ALLOC
/* Pacify GCC's 'empty if-body' warning.  */
#define YYSTACK_FREE(Ptr) \
  do                      \
    { /* empty */         \
      ;                   \
    }                     \
  while (0)
#ifndef YYSTACK_ALLOC_MAXIMUM
/* The OS might guarantee only one guard page at the bottom of the stack,
   and a page size can be as small as 4096 bytes.  So we cannot safely
   invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
   to allow for a few compiler-allocated temporary stack slots.  */
#define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#endif
#else
#define YYSTACK_ALLOC YYMALLOC
#define YYSTACK_FREE YYFREE
#ifndef YYSTACK_ALLOC_MAXIMUM
#define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#endif
#if (defined __cplusplus && !defined EXIT_SUCCESS && \
     !((defined YYMALLOC || defined malloc) && (defined YYFREE || defined free)))
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#ifndef YYMALLOC
#define YYMALLOC malloc
#if !defined malloc && !defined EXIT_SUCCESS
void *malloc(YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#ifndef YYFREE
#define YYFREE free
#if !defined free && !defined EXIT_SUCCESS
void free(void *);      /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (!defined yyoverflow && (!defined __cplusplus || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAXIMUM (sizeof(union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
#define YYSTACK_BYTES(N) ((N) * (sizeof(yytype_int16) + sizeof(YYSTYPE)) + YYSTACK_GAP_MAXIMUM)

#define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
#define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
  do                                                                   \
    {                                                                  \
      YYSIZE_T yynewbytes;                                             \
      YYCOPY(&yyptr->Stack_alloc, Stack, yysize);                      \
      Stack = &yyptr->Stack_alloc;                                     \
      yynewbytes = yystacksize * sizeof(*Stack) + YYSTACK_GAP_MAXIMUM; \
      yyptr += yynewbytes / sizeof(*yyptr);                            \
    }                                                                  \
  while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if defined __GNUC__ && 1 < __GNUC__
#define YYCOPY(Dst, Src, Count) __builtin_memcpy(Dst, Src, (Count) * sizeof(*(Src)))
#else
#define YYCOPY(Dst, Src, Count)                                    \
  do                                                               \
    {                                                              \
      YYSIZE_T yyi;                                                \
      for (yyi = 0; yyi < (Count); yyi++) (Dst)[yyi] = (Src)[yyi]; \
    }                                                              \
  while (0)
#endif
#endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 88
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 288

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 45
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 35
/* YYNRULES -- Number of rules.  */
#define YYNRULES 102
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 154

#define YYUNDEFTOK 2
#define YYMAXUTOK 288

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX) ((unsigned)(YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] = {
    0, 2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  36, 37, 2,  2,  2,  2,  42, 41, 2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 39, 2, 40, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 34, 2,  35, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 2,  2,  2,  43, 38, 44, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,
    2, 2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  2,  3,  4, 5,
    6, 7, 8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] = {
    0,   59,  59,  64,  70,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
    96,  97,  102, 105, 108, 115, 119, 122, 126, 129, 135, 143, 150, 161, 175, 179, 183, 187, 191, 198, 202,
    206, 210, 214, 218, 225, 229, 233, 237, 241, 245, 252, 261, 270, 278, 286, 297, 306, 310, 313, 317, 321,
    325, 328, 332, 335, 338, 341, 344, 347, 350, 353, 359, 382, 392, 402, 410, 418, 429, 438, 450, 460, 470,
    480, 486, 497, 503, 510, 518, 529, 547, 565, 583, 602, 603, 609, 615, 621, 630, 634, 642, 646};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] = {"$end",
                                      "error",
                                      "$undefined",
                                      "SINGLE_SYMBOL",
                                      "SUBSUPEROP",
                                      "APOSTROPHE",
                                      "FRAC",
                                      "DFRAC",
                                      "STACKREL",
                                      "BINOM",
                                      "GENFRAC",
                                      "SNOWFLAKE",
                                      "ACCENT",
                                      "UNKNOWN_SYMBOL",
                                      "FONT",
                                      "LATEXFONT",
                                      "LATEXTEXT",
                                      "FUNCTION",
                                      "C_OVER_C",
                                      "SPACE",
                                      "HSPACE",
                                      "LEFT",
                                      "RIGHT",
                                      "LEFT_DELIM",
                                      "AMBI_DELIM",
                                      "RIGHT_DELIM",
                                      "LBRACE",
                                      "RBRACE",
                                      "PIPE",
                                      "OPERATORNAME",
                                      "OVERLINE",
                                      "SQRT",
                                      "DIGIT",
                                      "PLUSMINUS",
                                      "'['",
                                      "']'",
                                      "'('",
                                      "')'",
                                      "'|'",
                                      "'<'",
                                      "'>'",
                                      "'/'",
                                      "'.'",
                                      "'{'",
                                      "'}'",
                                      "$accept",
                                      "result",
                                      "math",
                                      "single_symbol",
                                      "token",
                                      "simple",
                                      "customspace",
                                      "subsuper",
                                      "auto_delim",
                                      "ambi_delim_symbol",
                                      "left_delim_symbol",
                                      "right_delim_symbol",
                                      "left_delim",
                                      "right_delim",
                                      "auto_delim_inner",
                                      "accent",
                                      "placeable",
                                      "genfrac",
                                      "overline",
                                      "operatorname",
                                      "operatorname_inner",
                                      "sqrt",
                                      "group",
                                      "simple_group",
                                      "required_group",
                                      "required_group_inner",
                                      "start_group",
                                      "group_inner",
                                      "frac",
                                      "dfrac",
                                      "stackrel",
                                      "binom",
                                      "NT_FLOAT",
                                      "int",
                                      "uint",
                                      YY_NULLPTR};
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] = {0,   256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269,
                                         270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284,
                                         285, 286, 287, 288, 91,  93,  40,  41,  124, 60,  62,  47,  46,  123, 125};
#endif

#define YYPACT_NINF -112

#define yypact_value_is_default(Yystate) (!!((Yystate) == (-112)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = {
    76,   -112, 226,  -112, -30,  -30,  -30,  -30,  -16,  -112, 226,  -112, -112, -11,  -112, -112, -112, -112,
    -9,   97,   -112, -112, -112, -112, -112, -112, -8,   -30,  -22,  -112, -112, -112, -112, -112, -112, -112,
    -112, -112, -112, -112, -112, 36,   76,   -112, -112, -112, -112, -112, -112, 144,  -112, -112, -112, -112,
    -112, -112, -112, 76,   -112, -112, -112, -112, -112, 76,   -30,  -30,  -30,  -30,  97,   -112, -112, -18,
    -112, -112, -112, -112, -112, -112, -112, -112, -112, -112, -112, -112, 185,  -112, -13,  -112, -112, -112,
    144,  144,  15,   76,   -6,   76,   -3,   -112, -112, -112, -112, -2,   -112, -26,  11,   0,    8,    19,
    185,  185,  9,    11,   4,    -112, -112, 246,  -112, -112, -112, -112, -112, 12,   11,   19,   19,   -112,
    11,   -112, -112, -112, -112, -30,  -112, -112, -112, -112, -112, -112, -112, 246,  19,   19,   -112, 10,
    13,   -18,  14,   24,   76,   -30,  25,   -30,  -112, -112};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] = {
    0,  5,  0,  32, 0,  0,  0,  0,  0,  58, 0,  25, 28, 0,  64, 62, 61, 26, 0,  0,   6,  7,  8,  18,  19,  20,
    0,  0,  0,  21, 22, 9,  10, 11, 12, 13, 14, 15, 16, 17, 87, 0,  2,  60, 3,  24,  27, 29, 23, 54,  59,  30,
    69, 71, 72, 70, 63, 88, 65, 66, 67, 68, 33, 0,  0,  0,  0,  0,  0,  57, 86, 0,   44, 38, 45, 39,  41,  40,
    35, 42, 36, 37, 43, 52, 76, 74, 0,  79, 1,  4,  54, 54, 0,  88, 0,  84, 0,  90,  91, 92, 93, 0,   101, 0,
    0,  0,  94, 99, 76, 76, 0,  0,  0,  55, 56, 0,  34, 89, 81, 85, 83, 0,  0,  100, 95, 31, 97, 102, 78,  77,
    75, 0,  50, 51, 47, 46, 48, 49, 53, 0,  96, 98, 80, 0,  0,  0,  0,  0,  88, 0,   0,  0,  82, 73};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-112, -112, -112, -112, 3,    -44,  -112, -112, -42, -111, 2,    -87,
                                      -112, -112, -65,  -112, 7,    -112, -112, -112, -78, -112, -112, -112,
                                      -5,   -38,  -112, -85,  -112, -112, -112, -112, -77, -15,  -93};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] = {-1, 41, 42,  43, 93, 45,  46, 47, 48, 82, 83, 138, 49, 116, 92,  50,  51, 52,
                                         53, 54, 110, 55, 56, 149, 64, 96, 57, 94, 58, 59,  60, 61,  105, 106, 107};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] = {
    65,  66,  67,  44,  137, 90,  102, 91,  117, 62,  123, 124, 86,  63, 102, 103, 122, 69,  123, 102, 111, 63,  85,
    87,  104, 113, 114, 68,  137, 140, 128, 129, 70,  141, 71,  84,  88, 115, 118, 131, 109, 120, 121, 102, 125, 89,
    90,  90,  91,  91,  126, 127, 143, 130, 144, 139, 145, 119, 147, 97, 98,  99,  100, 150, 109, 109, 95,  148, 146,
    152, 101, 112, 0,   0,   0,   0,   0,   0,   0,   1,   2,   3,   4,  5,   6,   7,   8,   9,   10,  11,  12,  13,
    14,  15,  16,  17,  18,  19,  95,  20,  21,  22,  23,  24,  25,  26, 27,  28,  29,  30,  31,  32,  33,  34,  35,
    36,  37,  38,  39,  40,  72,  73,  0,   74,  0,   75,  142, 0,   0,  0,   0,   76,  0,   77,  0,   78,  79,  0,
    80,  81,  0,   0,   0,   0,   151, 0,   153, 1,   2,   3,   4,   5,  6,   7,   8,   9,   10,  0,   12,  13,  14,
    15,  16,  17,  18,  19,  0,   20,  21,  22,  23,  24,  25,  26,  27, 28,  29,  30,  31,  32,  33,  34,  35,  36,
    37,  38,  39,  40,  1,   2,   3,   4,   5,   6,   7,   8,   9,   10, 108, 12,  13,  14,  15,  16,  17,  18,  0,
    0,   20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32, 33,  34,  35,  36,  37,  38,  39,  40,  1,
    0,   0,   4,   5,   6,   7,   8,   9,   10,  0,   0,   13,  14,  15, 16,  0,   0,   0,   0,   20,  21,  22,  23,
    24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37, 38,  39,  40,  73,  132, 0,   133, 75,  0,
    0,   0,   0,   0,   0,   134, 0,   135, 78,  0,   136, 80,  81};

static const yytype_int16 yycheck[] = {
    5,  6,   7,   0,   115, 49,  32,  49, 93,  2,  103, 104, 34, 43,  32,  33,  42, 10, 111, 32,  33, 43,  27, 28, 42,
    90, 91,  43,  139, 122, 108, 109, 43, 126, 43, 43,  0,   22, 44,  35,  84,  44, 44, 32,  44,  42, 90,  91, 90, 91,
    42, 32,  139, 44,  44,  43,  43,  95, 44,  64, 65,  66,  67, 148, 108, 109, 63, 43, 145, 44,  68, 86,  -1, -1, -1,
    -1, -1,  -1,  -1,  3,   4,   5,   6,  7,   8,  9,   10,  11, 12,  13,  14,  15, 16, 17,  18,  19, 20,  21, 95, 23,
    24, 25,  26,  27,  28,  29,  30,  31, 32,  33, 34,  35,  36, 37,  38,  39,  40, 41, 42,  43,  23, 24,  -1, 26, -1,
    28, 131, -1,  -1,  -1,  -1,  34,  -1, 36,  -1, 38,  39,  -1, 41,  42,  -1,  -1, -1, -1,  149, -1, 151, 3,  4,  5,
    6,  7,   8,   9,   10,  11,  12,  -1, 14,  15, 16,  17,  18, 19,  20,  21,  -1, 23, 24,  25,  26, 27,  28, 29, 30,
    31, 32,  33,  34,  35,  36,  37,  38, 39,  40, 41,  42,  43, 3,   4,   5,   6,  7,  8,   9,   10, 11,  12, 13, 14,
    15, 16,  17,  18,  19,  20,  -1,  -1, 23,  24, 25,  26,  27, 28,  29,  30,  31, 32, 33,  34,  35, 36,  37, 38, 39,
    40, 41,  42,  43,  3,   -1,  -1,  6,  7,   8,  9,   10,  11, 12,  -1,  -1,  15, 16, 17,  18,  -1, -1,  -1, -1, 23,
    24, 25,  26,  27,  28,  29,  30,  31, 32,  33, 34,  35,  36, 37,  38,  39,  40, 41, 42,  43,  24, 25,  -1, 27, 28,
    -1, -1,  -1,  -1,  -1,  -1,  35,  -1, 37,  38, -1,  40,  41, 42};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] = {
    0,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 46, 47, 48, 49, 50, 51, 52, 53, 57, 60, 61,
    62, 63, 64, 66, 67, 71, 73, 74, 75, 76, 61, 43, 69, 69, 69, 69, 43, 61, 43, 43, 23, 24, 26, 28, 34, 36,
    38, 39, 41, 42, 54, 55, 43, 69, 34, 69, 0,  49, 50, 53, 59, 49, 72, 49, 70, 69, 69, 69, 69, 55, 32, 33,
    42, 77, 78, 79, 13, 50, 65, 33, 78, 59, 59, 22, 58, 72, 44, 70, 44, 44, 42, 79, 79, 44, 42, 32, 65, 65,
    44, 35, 25, 27, 35, 37, 40, 54, 56, 43, 79, 79, 69, 56, 44, 43, 77, 44, 43, 68, 72, 69, 44, 69};

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] = {0,  45, 46, 47, 47, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
                                    48, 48, 49, 49, 49, 50, 50, 50, 50, 50, 51, 52, 52, 53, 54, 54, 54, 54, 54, 55, 55,
                                    55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 57, 58, 59, 59, 59, 60, 61, 61, 61, 61, 61,
                                    61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 62, 63, 64, 65, 65, 65, 66, 66, 67, 68, 69,
                                    70, 70, 71, 71, 72, 72, 73, 74, 75, 76, 77, 77, 77, 77, 77, 78, 78, 79, 79};

/* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] = {0, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 4, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,
                                    2, 2, 0, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 13, 2, 4, 0, 2,
                                    2, 2, 5, 3, 3, 3, 1, 2, 2, 1, 0, 2, 3, 3, 3, 3, 1, 2, 3, 2, 3, 1,  2, 1, 2};


#define yyerrok (yyerrstatus = 0)
#define yyclearin (yychar = YYEMPTY)
#define YYEMPTY (-2)
#define YYEOF 0

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab


#define YYRECOVERING() (!!yyerrstatus)

#define YYBACKUP(Token, Value)                        \
  do                                                  \
    if (yychar == YYEMPTY)                            \
      {                                               \
        yychar = (Token);                             \
        yylval = (Value);                             \
        YYPOPSTACK(yylen);                            \
        yystate = *yyssp;                             \
        goto yybackup;                                \
      }                                               \
    else                                              \
      {                                               \
        yyerror(YY_("syntax error: cannot back up")); \
        YYERROR;                                      \
      }                                               \
  while (0)

/* Error token number */
#define YYTERROR 1
#define YYERRCODE 256


/* Enable debugging if requested.  */
#if YYDEBUG

#ifndef YYFPRINTF
#include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#define YYFPRINTF fprintf
#endif

#define YYDPRINTF(Args)            \
  do                               \
    {                              \
      if (yydebug) YYFPRINTF Args; \
    }                              \
  while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
#define YY_LOCATION_PRINT(File, Loc) ((void)0)
#endif


#define YY_SYMBOL_PRINT(Title, Type, Value, Location) \
  do                                                  \
    {                                                 \
      if (yydebug)                                    \
        {                                             \
          YYFPRINTF(stderr, "%s ", Title);            \
          yy_symbol_print(stderr, Type, Value);       \
          YYFPRINTF(stderr, "\n");                    \
        }                                             \
    }                                                 \
  while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE *yyo, int yytype, YYSTYPE const *const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE(yyoutput);
  if (!yyvaluep) return;
#ifdef YYPRINT
  if (yytype < YYNTOKENS) YYPRINT(yyo, yytoknum[yytype], *yyvaluep);
#endif
  YYUSE(yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE *yyo, int yytype, YYSTYPE const *const yyvaluep)
{
  YYFPRINTF(yyo, "%s %s (", yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print(yyo, yytype, yyvaluep);
  YYFPRINTF(yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void yy_stack_print(yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF(stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF(stderr, " %d", yybot);
    }
  YYFPRINTF(stderr, "\n");
}

#define YY_STACK_PRINT(Bottom, Top)                 \
  do                                                \
    {                                               \
      if (yydebug) yy_stack_print((Bottom), (Top)); \
    }                                               \
  while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void yy_reduce_print(yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF(stderr, "Reducing stack by rule %d (line %lu):\n", yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF(stderr, "   $%d = ", yyi + 1);
      yy_symbol_print(stderr, yystos[yyssp[yyi + 1 - yynrhs]], &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF(stderr, "\n");
    }
}

#define YY_REDUCE_PRINT(Rule)                           \
  do                                                    \
    {                                                   \
      if (yydebug) yy_reduce_print(yyssp, yyvsp, Rule); \
    }                                                   \
  while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
#define YYDPRINTF(Args)
#define YY_SYMBOL_PRINT(Title, Type, Value, Location)
#define YY_STACK_PRINT(Bottom, Top)
#define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

#ifndef yystrlen
#if defined __GLIBC__ && defined _STRING_H
#define yystrlen strlen
#else
/* Return the length of YYSTR.  */
static YYSIZE_T yystrlen(const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++) continue;
  return yylen;
}
#endif
#endif

#ifndef yystpcpy
#if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#define yystpcpy stpcpy
#else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *yystpcpy(char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0') continue;

  return yyd - 1;
}
#endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T yytnamerr(char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;) switch (*++yyp)
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
            if (yyres) yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres) yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes:;
    }

  if (!yyres) return yystrlen(yystr);

  return (YYSIZE_T)(yystpcpy(yyres, yystr) - yyres);
}
#endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int yysyntax_error(YYSIZE_T *yymsg_alloc, char **yymsg, yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr(YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum
  {
    YYERROR_VERBOSE_ARGS_MAXIMUM = 5
  };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
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
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default(yyn))
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
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR && !yytable_value_is_error(yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr(YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
#define YYCASE_(N, S) \
  case N:             \
    yyformat = S;     \
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

  {
    YYSIZE_T yysize1 = yysize + yystrlen(yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (!(yysize <= *yymsg_alloc && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM)) *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
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
          yyp += yytnamerr(yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void yydestruct(const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE(yyvaluep);
  if (!yymsg) yymsg = "Deleting";
  YY_SYMBOL_PRINT(yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE(yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int yyparse(void)
{
  int yystate;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;

  /* The stacks and their tools:
     'yyss': related to states.
     'yyvs': related to semantic values.

     Refer to the stacks through separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs;
  YYSTYPE *yyvsp;

  YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N) (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF((stderr, "Entering state %d\n", yystate));
  YY_ASSERT(0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16)yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T)(yyssp - yyss + 1);

#if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow(YY_("memory exhausted"), &yyss1, yysize * sizeof(*yyssp), &yyvs1, yysize * sizeof(*yyvsp),
                   &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize) goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize) yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr = (union yyalloc *)YYSTACK_ALLOC(YYSTACK_BYTES(yystacksize));
        if (!yyptr) goto yyexhaustedlab;
        YYSTACK_RELOCATE(yyss_alloc, yyss);
        YYSTACK_RELOCATE(yyvs_alloc, yyvs);
#undef YYSTACK_RELOCATE
        if (yyss1 != yyssa) YYSTACK_FREE(yyss1);
      }
#endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF((stderr, "Stack size increased to %lu\n", (unsigned long)yystacksize));

      if (yyss + yystacksize - 1 <= yyssp) YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL) YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default(yyn)) goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF((stderr, "Reading a token: "));
      yychar = yylex();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE(yychar);
      YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken) goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error(yyn)) goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus) yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0) goto yyerrlab;
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
  yyval = yyvsp[1 - yylen];


  YY_REDUCE_PRINT(yyn);
  switch (yyn)
    {
    case 2:
#line 59 "mathtex2.y"
      {
        result_parser_node_index = copy_parser_node(yyvsp[0]);
      }
#line 1390 "mathtex2.tab.c"
      break;

    case 3:
#line 64 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.u.math.previous = 0;
        yyval.u.math.token = copy_parser_node(yyvsp[0]);
        yyval.type = NT_MATH;
      }
#line 1401 "mathtex2.tab.c"
      break;

    case 4:
#line 70 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.u.math.previous = copy_parser_node(yyvsp[-1]);
        yyval.u.math.token = copy_parser_node(yyvsp[0]);
        yyval.length += yyvsp[0].length;
      }
#line 1412 "mathtex2.tab.c"
      break;

    case 5:
#line 80 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1418 "mathtex2.tab.c"
      break;

    case 6:
#line 81 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1424 "mathtex2.tab.c"
      break;

    case 7:
#line 82 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1430 "mathtex2.tab.c"
      break;

    case 8:
#line 83 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1436 "mathtex2.tab.c"
      break;

    case 9:
#line 84 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1442 "mathtex2.tab.c"
      break;

    case 10:
#line 85 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1448 "mathtex2.tab.c"
      break;

    case 11:
#line 86 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1454 "mathtex2.tab.c"
      break;

    case 12:
#line 87 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1460 "mathtex2.tab.c"
      break;

    case 13:
#line 88 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1466 "mathtex2.tab.c"
      break;

    case 14:
#line 89 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1472 "mathtex2.tab.c"
      break;

    case 15:
#line 90 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1478 "mathtex2.tab.c"
      break;

    case 16:
#line 91 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1484 "mathtex2.tab.c"
      break;

    case 17:
#line 92 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1490 "mathtex2.tab.c"
      break;

    case 18:
#line 93 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1496 "mathtex2.tab.c"
      break;

    case 19:
#line 94 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1502 "mathtex2.tab.c"
      break;

    case 20:
#line 95 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1508 "mathtex2.tab.c"
      break;

    case 21:
#line 96 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1514 "mathtex2.tab.c"
      break;

    case 22:
#line 97 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1520 "mathtex2.tab.c"
      break;

    case 23:
#line 102 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1528 "mathtex2.tab.c"
      break;

    case 24:
#line 105 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1536 "mathtex2.tab.c"
      break;

    case 25:
#line 108 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_SYMBOL;
      }
#line 1545 "mathtex2.tab.c"
      break;

    case 26:
#line 115 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_SPACE;
      }
#line 1554 "mathtex2.tab.c"
      break;

    case 27:
#line 119 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1562 "mathtex2.tab.c"
      break;

    case 28:
#line 122 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_FONT;
      }
#line 1571 "mathtex2.tab.c"
      break;

    case 29:
#line 126 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1579 "mathtex2.tab.c"
      break;

    case 30:
#line 129 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1587 "mathtex2.tab.c"
      break;

    case 31:
#line 135 "mathtex2.y"
      {
        yyval = yyvsp[-3];
        yyval.length += yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_CUSTOMSPACE;
      }
#line 1597 "mathtex2.tab.c"
      break;

    case 32:
#line 143 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.index = 0;
        yyval.type = NT_SUBSUPER;
        yyval.u.subsuper.operator= '\'';
        yyval.u.subsuper.token = 0;
      }
#line 1609 "mathtex2.tab.c"
      break;

    case 33:
#line 150 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.length += yyvsp[0].length;
        yyval.type = NT_SUBSUPER;
        yyval.u.subsuper.operator= yyvsp[-1].source[0];
        yyval.u.subsuper.token = copy_parser_node(yyvsp[0]);
      }
#line 1622 "mathtex2.tab.c"
      break;

    case 34:
#line 161 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_AUTO_DELIM;
        yyval.u.autodelim.left_delim_start = yyvsp[-2].source + 5;
        yyval.u.autodelim.left_delim_length = yyvsp[-2].length - 5;
        yyval.u.autodelim.right_delim_start = yyvsp[0].source + 6;
        yyval.u.autodelim.right_delim_length = yyvsp[0].length - 6;
        yyval.u.autodelim.inner_node_index = copy_parser_node(yyvsp[-1]);
      }
#line 1638 "mathtex2.tab.c"
      break;

    case 35:
#line 175 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1647 "mathtex2.tab.c"
      break;

    case 36:
#line 179 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1656 "mathtex2.tab.c"
      break;

    case 37:
#line 183 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1665 "mathtex2.tab.c"
      break;

    case 38:
#line 187 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1674 "mathtex2.tab.c"
      break;

    case 39:
#line 191 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1683 "mathtex2.tab.c"
      break;

    case 40:
#line 198 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1692 "mathtex2.tab.c"
      break;

    case 41:
#line 202 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1701 "mathtex2.tab.c"
      break;

    case 42:
#line 206 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1710 "mathtex2.tab.c"
      break;

    case 43:
#line 210 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1719 "mathtex2.tab.c"
      break;

    case 44:
#line 214 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1728 "mathtex2.tab.c"
      break;

    case 45:
#line 218 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1737 "mathtex2.tab.c"
      break;

    case 46:
#line 225 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1746 "mathtex2.tab.c"
      break;

    case 47:
#line 229 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1755 "mathtex2.tab.c"
      break;

    case 48:
#line 233 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1764 "mathtex2.tab.c"
      break;

    case 49:
#line 237 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1773 "mathtex2.tab.c"
      break;

    case 50:
#line 241 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1782 "mathtex2.tab.c"
      break;

    case 51:
#line 245 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 1791 "mathtex2.tab.c"
      break;

    case 52:
#line 252 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
      }
#line 1802 "mathtex2.tab.c"
      break;

    case 53:
#line 261 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
      }
#line 1813 "mathtex2.tab.c"
      break;

    case 54:
#line 270 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = NULL;
        yyval.length = 0;
        yyval.u.autodeliminner.previous = 0;
        yyval.u.autodeliminner.token = 0;
        yyval.type = NT_AUTO_DELIM_INNER;
      }
#line 1826 "mathtex2.tab.c"
      break;

    case 55:
#line 278 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.type = NT_AUTO_DELIM_INNER;
        yyval.u.autodeliminner.previous = copy_parser_node(yyvsp[0]);
        yyval.u.autodeliminner.token = copy_parser_node(yyvsp[-1]);
        yyval.length += yyvsp[0].length;
      }
#line 1839 "mathtex2.tab.c"
      break;

    case 56:
#line 286 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.type = NT_AUTO_DELIM_INNER;
        yyval.u.autodeliminner.previous = copy_parser_node(yyvsp[0]);
        yyval.u.autodeliminner.token = copy_parser_node(yyvsp[-1]);
        yyval.length += yyvsp[0].length;
      }
#line 1852 "mathtex2.tab.c"
      break;

    case 57:
#line 297 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.type = NT_ACCENT;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.u.accent.token = copy_parser_node(yyvsp[0]);
      }
#line 1864 "mathtex2.tab.c"
      break;

    case 58:
#line 306 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_SYMBOL;
      }
#line 1873 "mathtex2.tab.c"
      break;

    case 59:
#line 310 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1881 "mathtex2.tab.c"
      break;

    case 60:
#line 313 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_SYMBOL;
      }
#line 1890 "mathtex2.tab.c"
      break;

    case 61:
#line 317 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_C_OVER_C;
      }
#line 1899 "mathtex2.tab.c"
      break;

    case 62:
#line 321 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_FUNCTION;
      }
#line 1908 "mathtex2.tab.c"
      break;

    case 63:
#line 325 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1916 "mathtex2.tab.c"
      break;

    case 64:
#line 328 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_LATEXTEXT;
      }
#line 1925 "mathtex2.tab.c"
      break;

    case 65:
#line 332 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1933 "mathtex2.tab.c"
      break;

    case 66:
#line 335 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1941 "mathtex2.tab.c"
      break;

    case 67:
#line 338 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1949 "mathtex2.tab.c"
      break;

    case 68:
#line 341 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1957 "mathtex2.tab.c"
      break;

    case 69:
#line 344 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1965 "mathtex2.tab.c"
      break;

    case 70:
#line 347 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1973 "mathtex2.tab.c"
      break;

    case 71:
#line 350 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1981 "mathtex2.tab.c"
      break;

    case 72:
#line 353 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 1989 "mathtex2.tab.c"
      break;

    case 73:
#line 359 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-12].source;
        yyval.length = yyvsp[-12].length + yyvsp[-11].length + yyvsp[-10].length + yyvsp[-9].length + yyvsp[-8].length +
                       yyvsp[-7].length + yyvsp[-6].length + yyvsp[-5].length + yyvsp[-4].length + yyvsp[-3].length +
                       yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GENFRAC;
        yyval.u.genfrac.left_delim_start = yyvsp[-10].source;
        yyval.u.genfrac.left_delim_length = yyvsp[-10].length;
        yyval.u.genfrac.right_delim_start = yyvsp[-7].source;
        yyval.u.genfrac.right_delim_length = yyvsp[-7].length;
        double thickness = 0;
        int n = sscanf(yyvsp[-4].source, "\\hspace{%lf}", &thickness);
        if (n != 1)
          {
            thickness = NAN;
          }
        yyval.u.genfrac.thickness = thickness;
        yyval.u.genfrac.style_text_start = yyvsp[-2].source;
        yyval.u.genfrac.style_text_length = yyvsp[-2].length;
        yyval.u.genfrac.numerator_group = copy_parser_node(yyvsp[-1]);
        yyval.u.genfrac.denominator_group = copy_parser_node(yyvsp[0]);
      }
#line 2014 "mathtex2.tab.c"
      break;

    case 74:
#line 382 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OVERLINE;
        yyval.u.overline.body = copy_parser_node(yyvsp[0]);
      }
#line 2026 "mathtex2.tab.c"
      break;

    case 75:
#line 392 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.source = yyvsp[-3].source;
        yyval.length = yyvsp[-3].length + yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OPERATORNAME;
      }
#line 2038 "mathtex2.tab.c"
      break;

    case 76:
#line 402 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = NULL;
        yyval.length = 0;
        yyval.type = NT_OTHER;
        yyval.u.operatorname.previous = 0;
        yyval.u.operatorname.token = 0;
      }
#line 2051 "mathtex2.tab.c"
      break;

    case 77:
#line 410 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
        yyval.u.operatorname.previous = copy_parser_node(yyvsp[0]);
        yyval.u.operatorname.token = copy_parser_node(yyvsp[-1]);
      }
#line 2064 "mathtex2.tab.c"
      break;

    case 78:
#line 418 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
        yyval.u.operatorname.previous = copy_parser_node(yyvsp[0]);
        yyval.u.operatorname.token = copy_parser_node(yyvsp[-1]);
      }
#line 2077 "mathtex2.tab.c"
      break;

    case 79:
#line 429 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_SQRT;
        yyval.u.sqrt.index_start = "";
        yyval.u.sqrt.index_length = 0;
        yyval.u.sqrt.token = copy_parser_node(yyvsp[0]);
      }
#line 2091 "mathtex2.tab.c"
      break;

    case 80:
#line 438 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-4].source;
        yyval.length = yyvsp[-4].length + yyvsp[-3].length + yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_SQRT;
        yyval.u.sqrt.index_start = yyvsp[-2].source;
        yyval.u.sqrt.index_length = yyvsp[-2].length;
        yyval.u.sqrt.token = copy_parser_node(yyvsp[0]);
      }
#line 2105 "mathtex2.tab.c"
      break;

    case 81:
#line 450 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GROUP;
      }
#line 2117 "mathtex2.tab.c"
      break;

    case 82:
#line 460 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GROUP;
      }
#line 2129 "mathtex2.tab.c"
      break;

    case 83:
#line 470 "mathtex2.y"
      {
        yyval = yyvsp[-1];
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GROUP;
      }
#line 2141 "mathtex2.tab.c"
      break;

    case 84:
#line 480 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
        yyval.u.group.previous = 0;
        yyval.u.group.token = copy_parser_node(yyvsp[0]);
      }
#line 2152 "mathtex2.tab.c"
      break;

    case 85:
#line 486 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
        yyval.u.group.previous = copy_parser_node(yyvsp[0]);
        yyval.u.group.token = copy_parser_node(yyvsp[-1]);
      }
#line 2165 "mathtex2.tab.c"
      break;

    case 86:
#line 497 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
      }
#line 2176 "mathtex2.tab.c"
      break;

    case 87:
#line 503 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 2185 "mathtex2.tab.c"
      break;

    case 88:
#line 510 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = NULL;
        yyval.length = 0;
        yyval.type = NT_OTHER;
        yyval.u.group.previous = 0;
        yyval.u.group.token = 0;
      }
#line 2198 "mathtex2.tab.c"
      break;

    case 89:
#line 518 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
        yyval.u.group.previous = copy_parser_node(yyvsp[0]);
        yyval.u.group.token = copy_parser_node(yyvsp[-1]);
      }
#line 2211 "mathtex2.tab.c"
      break;

    case 90:
#line 529 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GENFRAC;
        yyval.u.genfrac.left_delim_start = "";
        yyval.u.genfrac.left_delim_length = 0;
        yyval.u.genfrac.right_delim_start = "";
        yyval.u.genfrac.right_delim_length = 0;
        yyval.u.genfrac.thickness = NAN;
        yyval.u.genfrac.style_text_start = "{0}";
        yyval.u.genfrac.style_text_length = 3;
        yyval.u.genfrac.numerator_group = copy_parser_node(yyvsp[-1]);
        yyval.u.genfrac.denominator_group = copy_parser_node(yyvsp[0]);
      }
#line 2231 "mathtex2.tab.c"
      break;

    case 91:
#line 547 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GENFRAC;
        yyval.u.genfrac.left_delim_start = "";
        yyval.u.genfrac.left_delim_length = 0;
        yyval.u.genfrac.right_delim_start = "";
        yyval.u.genfrac.right_delim_length = 0;
        yyval.u.genfrac.thickness = NAN;
        yyval.u.genfrac.style_text_start = "{1}";
        yyval.u.genfrac.style_text_length = 3;
        yyval.u.genfrac.numerator_group = copy_parser_node(yyvsp[-1]);
        yyval.u.genfrac.denominator_group = copy_parser_node(yyvsp[0]);
      }
#line 2251 "mathtex2.tab.c"
      break;

    case 92:
#line 565 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GENFRAC;
        yyval.u.genfrac.left_delim_start = "";
        yyval.u.genfrac.left_delim_length = 0;
        yyval.u.genfrac.right_delim_start = "";
        yyval.u.genfrac.right_delim_length = 0;
        yyval.u.genfrac.thickness = 0.0;
        yyval.u.genfrac.style_text_start = "{1}";
        yyval.u.genfrac.style_text_length = 3;
        yyval.u.genfrac.numerator_group = copy_parser_node(yyvsp[-1]);
        yyval.u.genfrac.denominator_group = copy_parser_node(yyvsp[0]);
      }
#line 2271 "mathtex2.tab.c"
      break;

    case 93:
#line 583 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_GENFRAC;
        yyval.u.genfrac.left_delim_start = "(";
        yyval.u.genfrac.left_delim_length = 1;
        yyval.u.genfrac.right_delim_start = ")";
        yyval.u.genfrac.right_delim_length = 1;
        yyval.u.genfrac.thickness = 0.0;
        yyval.u.genfrac.style_text_start = "{1}";
        yyval.u.genfrac.style_text_length = 3;
        yyval.u.genfrac.numerator_group = copy_parser_node(yyvsp[-1]);
        yyval.u.genfrac.denominator_group = copy_parser_node(yyvsp[0]);
      }
#line 2291 "mathtex2.tab.c"
      break;

    case 94:
#line 602 "mathtex2.y"
      {
        yyval = yyvsp[0];
      }
#line 2297 "mathtex2.tab.c"
      break;

    case 95:
#line 603 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_FLOAT;
      }
#line 2308 "mathtex2.tab.c"
      break;

    case 96:
#line 609 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_FLOAT;
      }
#line 2319 "mathtex2.tab.c"
      break;

    case 97:
#line 615 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_FLOAT;
      }
#line 2330 "mathtex2.tab.c"
      break;

    case 98:
#line 621 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-2].source;
        yyval.length = yyvsp[-2].length + yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_FLOAT;
      }
#line 2341 "mathtex2.tab.c"
      break;

    case 99:
#line 630 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_INTEGER;
      }
#line 2350 "mathtex2.tab.c"
      break;

    case 100:
#line 634 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_INTEGER;
      }
#line 2361 "mathtex2.tab.c"
      break;

    case 101:
#line 642 "mathtex2.y"
      {
        yyval = yyvsp[0];
        yyval.type = NT_OTHER;
      }
#line 2370 "mathtex2.tab.c"
      break;

    case 102:
#line 646 "mathtex2.y"
      {
        yyval.index = 0;
        yyval.source = yyvsp[-1].source;
        yyval.length = yyvsp[-1].length + yyvsp[0].length;
        yyval.type = NT_OTHER;
      }
#line 2381 "mathtex2.tab.c"
      break;


#line 2385 "mathtex2.tab.c"

    default:
      break;
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
  YY_SYMBOL_PRINT("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK(yylen);
  yylen = 0;
  YY_STACK_PRINT(yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp ? yytable[yyi] : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE(yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if !YYERROR_VERBOSE
      yyerror(YY_("syntax error"));
#else
#define YYSYNTAX_ERROR yysyntax_error(&yymsg_alloc, &yymsg, yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf) YYSTACK_FREE(yymsg);
            yymsg = (char *)YYSTACK_ALLOC(yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror(yymsgp);
        if (yysyntax_error_status == 2) goto yyexhaustedlab;
      }
#undef YYSYNTAX_ERROR
#endif
    }


  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF) YYABORT;
        }
      else
        {
          yydestruct("Error: discarding", yytoken, &yylval);
          yychar = YYEMPTY;
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
  if (0) YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK(yylen);
  yylen = 0;
  YY_STACK_PRINT(yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3; /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default(yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn) break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss) YYABORT;


      yydestruct("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK(1);
      yystate = *yyssp;
      YY_STACK_PRINT(yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror(YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE(yychar);
      yydestruct("Cleanup: discarding lookahead", yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK(yylen);
  YY_STACK_PRINT(yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct("Cleanup: popping", yystos[*yyssp], yyvsp);
      YYPOPSTACK(1);
    }
#ifndef yyoverflow
  if (yyss != yyssa) YYSTACK_FREE(yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf) YYSTACK_FREE(yymsg);
#endif
  return yyresult;
}
#line 654 "mathtex2.y"


const char *snowflake_symbols[] = {"\\doteqdot", "\\doteq",   "\\dotminus", "\\barleftarrow",
                                   "\\ddots",    "\\dotplus", "\\dots",     "\\barwedge"};
const char *accent_symbols[] = {"\\hat",           "\\breve",    "\\bar",     "\\grave",   "\\acute",
                                "\\tilde",         "\\dot",      "\\ddot",    "\\vec",     "\\overrightarrow",
                                "\\overleftarrow", "\\mathring", "\\widebar", "\\widehat", "\\widetilde"};
const char *font_symbols[] = {"\\rm",      "\\cal", "\\it",   "\\tt",      "\\sf",  "\\bf",
                              "\\default", "\\bb",  "\\frak", "\\circled", "\\scr", "\\regular"};
const char *latexfont_symbols[] = {
    "\\mathrm",   "\\mathcal",     "\\mathit",  "\\mathtt",      "\\mathsf", "\\mathbf", "\\mathdefault", "\\mathbb",
    "\\mathfrak", "\\mathcircled", "\\mathscr", "\\mathregular", "\\textrm", "\\textit", "\\textbf",      "\\texttt"};
const char *c_over_c_symbols[] = {"\\AA"};
const char *space_symbols[] = {"\\thinspace", "\\enspace", "\\quad", "\\qquad"};
const char *left_delim_symbols[] = {"\\int", "\\lfloor", "\\langle", "\\lceil", "\\sum"};
const char *ambi_delim_symbols[] = {"\\backslash", "\\uparrow",     "\\downarrow", "\\updownarrow", "\\Uparrow",
                                    "\\Downarrow", "\\Updownarrow", "\\vert",      "\\Vert"};
const char *right_delim_symbols[] = {"\\rfloor", "\\rangle", "\\rceil"};
const char *function_symbols[] = {"\\arccos", "\\csc", "\\ker",    "\\min",  "\\arcsin", "\\deg", "\\lg",     "\\Pr",
                                  "\\arctan", "\\det", "\\lim",    "\\sec",  "\\arg",    "\\dim", "\\liminf", "\\sin",
                                  "\\cos",    "\\exp", "\\limsup", "\\sinh", "\\cosh",   "\\gcd", "\\ln",     "\\sup",
                                  "\\cot",    "\\hom", "\\log",    "\\tan",  "\\coth",   "\\inf", "\\max",    "\\tanh"};

int symbol_in_symbol_list(const char *symbol, size_t length, const char **symbol_list, size_t num_symbols)
{
  int i;
  for (i = 0; i < num_symbols; i++)
    {
      if (strncmp(symbol, symbol_list[i], length) == 0 && symbol_list[i][length] == 0)
        {
          return 1;
        }
    }
  return 0;
}

int symbol_is_snowflake(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, snowflake_symbols, sizeof(snowflake_symbols) / sizeof(const char *));
}

int symbol_is_accent(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, accent_symbols, sizeof(accent_symbols) / sizeof(const char *));
}

int symbol_is_font(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, font_symbols, sizeof(font_symbols) / sizeof(const char *));
}

int symbol_is_latexfont(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, latexfont_symbols, sizeof(latexfont_symbols) / sizeof(const char *));
}

int symbol_is_c_over_c(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, c_over_c_symbols, sizeof(c_over_c_symbols) / sizeof(const char *));
}

int symbol_is_space(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, space_symbols, sizeof(space_symbols) / sizeof(const char *));
}

int symbol_is_left_delim(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, left_delim_symbols, sizeof(left_delim_symbols) / sizeof(const char *));
}

int symbol_is_ambi_delim(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, ambi_delim_symbols, sizeof(ambi_delim_symbols) / sizeof(const char *));
}

int symbol_is_right_delim(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, right_delim_symbols, sizeof(right_delim_symbols) / sizeof(const char *));
}
int symbol_is_function(const char *symbol, size_t length)
{
  return symbol_in_symbol_list(symbol, length, function_symbols, sizeof(function_symbols) / sizeof(const char *));
}


const char *cursor;
enum State state;
const char *symbol_start;
int ignore_whitespace;

int yylex(void)
{
  yylval.index = 0;
  for (; *cursor != 0 || state == INSIDE_SYMBOL; cursor++)
    {
      int c = *cursor;
      if (c == ' ' && ignore_whitespace)
        {
          ignore_whitespace = 0;
          continue;
        }
      ignore_whitespace = 0;
      switch (state)
        {
        case OUTSIDE_SYMBOL:
          {
            if ('0' <= c && c <= '9')
              {
                yylval.source = cursor;
                yylval.length = 1;
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return DIGIT;
              }
            else if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (0x80 <= c && c <= 0x1ffff) ||
                     strchr(" *,=:;!?&'@", c) != NULL)
              {
                if (c == ' ')
                  {
                    break;
                  }
                yylval.source = cursor;
                yylval.length = 1;
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return SINGLE_SYMBOL;
              }
            else if (strchr("()[]<>./{}|", c) != NULL)
              {
                yylval.source = cursor;
                yylval.length = 1;
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return c;
              }
            else
              {
                switch (c)
                  {
                  case '\\':
                    state = INSIDE_SYMBOL;
                    symbol_start = cursor;
                    break;
                  case '_':
                  case '^':
                    yylval.source = cursor;
                    yylval.length = 1;
                    yylval.type = NT_TERMINAL_SYMBOL;
                    cursor += 1;
                    return SUBSUPEROP;
                  case '\'':
                    yylval.source = cursor;
                    yylval.length = 1;
                    yylval.type = NT_TERMINAL_SYMBOL;
                    cursor += 1;
                    while (*cursor == '\'')
                      {
                        yylval.length = 1;
                        cursor += 1;
                      }
                    return APOSTROPHE;
                  case '~':
                    yylval.source = cursor;
                    yylval.length = 1;
                    yylval.type = NT_TERMINAL_SYMBOL;
                    cursor += 1;
                    return SPACE;
                  case '-':
                  case '+':
                    yylval.source = cursor;
                    yylval.length = 1;
                    yylval.type = NT_TERMINAL_SYMBOL;
                    cursor += 1;
                    return PLUSMINUS;
                  default:
                    yylval.source = cursor;
                    yylval.length = 1;
                    yylval.type = NT_TERMINAL_SYMBOL;
                    cursor += 1;
                    return c;
                  }
              }
          }
          break;
        case INSIDE_SYMBOL:
          {
            if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
              {
                /* valid part of symbol */
              }
            else if (c == '{' && cursor == symbol_start + 1)
              {
                state = OUTSIDE_SYMBOL;
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start + 1);
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return LBRACE;
              }
            else if (c == '}' && cursor == symbol_start + 1)
              {
                state = OUTSIDE_SYMBOL;
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start + 1);
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return RBRACE;
              }
            else if (c == '|' && cursor == symbol_start + 1)
              {
                state = OUTSIDE_SYMBOL;
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start + 1);
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return PIPE;
              }
            else if (strchr(",/>:; !", c) != NULL && cursor == symbol_start + 1)
              {
                state = OUTSIDE_SYMBOL;
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start + 1);
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return SPACE;
              }
            else if (strchr("%$[]_#", c) != NULL && cursor == symbol_start + 1)
              {
                state = OUTSIDE_SYMBOL;
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start + 1);
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return SINGLE_SYMBOL;
              }
            else if (strchr("\"`'~.^", c) != NULL && cursor == symbol_start + 1)
              {
                state = OUTSIDE_SYMBOL;
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start + 1);
                yylval.type = NT_TERMINAL_SYMBOL;
                cursor += 1;
                return ACCENT;
              }
            else
              {
                state = OUTSIDE_SYMBOL;
                int result;
                yylval.type = NT_TERMINAL_SYMBOL;
                if (strncmp("\\frac", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = FRAC;
                  }
                else if (strncmp("\\dfrac", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = DFRAC;
                  }
                else if (strncmp("\\stackrel", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = STACKREL;
                  }
                else if (strncmp("\\binom", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = BINOM;
                  }
                else if (strncmp("\\genfrac", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = GENFRAC;
                  }
                else if (strncmp("\\operatorname", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = OPERATORNAME;
                  }
                else if (strncmp("\\overline", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = OVERLINE;
                  }
                else if (strncmp("\\sqrt", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = SQRT;
                  }
                else if (strncmp("\\hspace", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = HSPACE;
                  }
                else if (strncmp("\\left", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = LEFT;
                  }
                else if (strncmp("\\right", symbol_start, (int)(cursor - symbol_start)) == 0)
                  {
                    result = RIGHT;
                  }
                else if (symbol_is_snowflake(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = SNOWFLAKE;
                  }
                else if (symbol_is_accent(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = ACCENT;
                  }
                else if (symbol_is_font(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = FONT;
                  }
                else if (symbol_is_latexfont(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = LATEXFONT;
                  }
                else if (symbol_is_function(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = FUNCTION;
                  }
                else if (symbol_is_c_over_c(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = C_OVER_C;
                  }
                else if (symbol_is_space(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = SPACE;
                  }
                else if (symbol_is_left_delim(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = LEFT_DELIM;
                  }
                else if (symbol_is_ambi_delim(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = AMBI_DELIM;
                  }
                else if (symbol_is_right_delim(symbol_start, (int)(cursor - symbol_start)))
                  {
                    result = RIGHT_DELIM;
                  }
                else
                  {
                    yylval.type = NT_SYMBOL;
                    result = SINGLE_SYMBOL;
                    ignore_whitespace = 1;
                  }
                yylval.source = symbol_start;
                yylval.length = (int)(cursor - symbol_start);
                if (result == LATEXFONT && *cursor == '{' && strncmp(symbol_start, "\\text", 5) == 0)
                  {
                    const char *text_end = strchr(symbol_start, '}');
                    if (text_end && (strrchr(text_end, '{') == cursor))
                      {
                        yylval.length = (int)(text_end - symbol_start);
                        cursor = text_end + 1;
                        result = LATEXTEXT;
                      }
                  }
                return result;
              }
          }
          break;
        default:
          break;
        }
    }
  return 0;
}
