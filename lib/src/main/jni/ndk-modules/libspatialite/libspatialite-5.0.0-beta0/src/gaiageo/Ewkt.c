/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
/************ Begin %include sections from the grammar ************************/

/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    ParseTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYNTOKEN           Number of terminal symbols
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
**    YY_MIN_REDUCE      Minimum value for reduce actions
**    YY_MAX_REDUCE      Maximum value for reduce actions
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 117
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  struct ewkt_data *p_data ;
#define ParseARG_PDECL , struct ewkt_data *p_data 
#define ParseARG_FETCH  struct ewkt_data *p_data  = yypParser->p_data 
#define ParseARG_STORE yypParser->p_data  = p_data 
#define YYNSTATE             335
#define YYNRULE              199
#define YYNTOKEN             20
#define YY_MAX_SHIFT         334
#define YY_MIN_SHIFTREDUCE   508
#define YY_MAX_SHIFTREDUCE   706
#define YY_ERROR_ACTION      707
#define YY_ACCEPT_ACTION     708
#define YY_NO_ACTION         709
#define YY_MIN_REDUCE        710
#define YY_MAX_REDUCE        908
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X.
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (694)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   334,  334,  334,  334,  710,  711,  712,  713,  714,  715,
 /*    10 */   716,  717,  718,  719,  720,  721,  722,  723,  724,  725,
 /*    20 */   726,  727,  728,  729,  730,  731,  732,  733,  734,  735,
 /*    30 */   736,  737,  148,  110,   87,  133,  109,   86,  132,  108,
 /*    40 */    85,  131,  107,   84,  130,  106,   83,  129,  105,   82,
 /*    50 */   903,  206,  333,  236,  212,  329,  194,  245,   28,  231,
 /*    60 */    27,  216,   25,  187,   24,  170,   22,  153,   20,   79,
 /*    70 */   333,  329,  708,    1,  299,  231,   28,  216,   27,  187,
 /*    80 */    25,  170,   24,  153,   22,   71,   19,  208,  207,  548,
 /*    90 */   206,  205,  248,  329,  200,  194,  262,  231,  330,  216,
 /*   100 */   236,  187,  101,  170,  100,  153,   99,   64,   98,  262,
 /*   110 */    96,  127,   81,  125,  102,  101,  207,  100,  270,   99,
 /*   120 */   240,   98,  200,   96,  124,   88,  123,  270,  122,  323,
 /*   130 */   121,  236,  119,  124,  104,  123,  278,  122,  218,  121,
 /*   140 */   236,  119,  147,  111,  146,  278,  145,  306,  144,  236,
 /*   150 */   142,  147,  128,  146,  249,  145,  331,  144,  208,  142,
 /*   160 */   240,  134,  205,  243,    2,   61,   77,   76,   75,   74,
 /*   170 */    73,   72,   78,   69,   68,   67,   66,   65,   87,   63,
 /*   180 */    94,   86,  110,   93,   85,  109,   92,   84,  108,   91,
 /*   190 */    83,  107,   90,   82,  106,   89,  117,  105,  148,  116,
 /*   200 */   140,  133,  115,  139,  132,  114,  138,  131,  113,  137,
 /*   210 */   130,  112,  136,  129,   26,  135,  210,  318,  313,  548,
 /*   220 */   308,   21,  248,  192,  327,  325,  548,  323,    4,  248,
 /*   230 */   332,  331,  325,  330,  183,  248,  240,  245,  182,  839,
 /*   240 */   222,  841,  842,  165,  240,  161,  157,  823,  311,  825,
 /*   250 */   826,  179,  240,  176,  173,  807,  902,  809,  810,  332,
 /*   260 */   795,  228,  797,  798,  243,  224,  775,  220,  777,  778,
 /*   270 */   767,  198,  769,  770,  548,  203,  186,   13,  548,  548,
 /*   280 */   327,  301,   23,  302,  245,  243,  245,  303,    3,  307,
 /*   290 */   245,  236,  308,  312,  236,  313,  226,  240,  316,  240,
 /*   300 */   317,  243,  318,  243,   12,  243,  229,  243,  230,  245,
 /*   310 */   321,  245,  232,  245,  236,  237,  233,  238,  236,  240,
 /*   320 */   241,  240,  242,  328,    5,  243,  245,  243,  542,  250,
 /*   330 */   548,  540,  251,  548,    6,  252,  253,    7,   70,  155,
 /*   340 */   254,  872,   95,   32,   97,  871,  870,   15,  869,  868,
 /*   350 */     9,  867,    8,  866,  159,  255,  157,  840,  173,  256,
 /*   360 */   887,  102,  770,  778,  118,   33,  120,  257,   16,   10,
 /*   370 */   258,  259,  260,   80,   11,  261,  810,  798,  901,  900,
 /*   380 */   161,  826,  176,  777,  899,  898,  897,  769,  163,  141,
 /*   390 */   896,  263,  125,   34,  264,  825,  797,  809,  179,  143,
 /*   400 */    17,  150,  265,  266,  267,  268,  269,  103,  886,  885,
 /*   410 */   884,  165,  883,  882,  881,  215,  795,  767,  271,  857,
 /*   420 */   127,  272,  273,  274,  807,  126,  275,  276,  277,  823,
 /*   430 */   856,  855,  854,  775,  853,  852,  149,  851,  152,  832,
 /*   440 */   156,  151,  280,  824,   42,  279,  220,  154,   46,  160,
 /*   450 */   158,  164,  829,  838,  281,  224,  835,   50,  282,  228,
 /*   460 */   162,   53,  283,  167,  166,  168,  816,  284,  169,  808,
 /*   470 */   172,  175,  822,  171,  285,  819,  174,  178,  286,  813,
 /*   480 */   177,  181,  754,  287,  186,  180,  288,   51,  289,  290,
 /*   490 */   184,  185,   18,  796,  193,  758,  189,  190,  191,  192,
 /*   500 */   199,  291,  188,  293,  198,  204,  756,  196,  197,  203,
 /*   510 */   201,  195,  292,  295,  752,  202,  294,   54,   56,   58,
 /*   520 */    62,  209,  213,   38,  219,  223,  227,  709,  296,  297,
 /*   530 */   298,  709,  709,  662,  661,  660,  300,  709,  659,  658,
 /*   540 */   657,  656,  709,  691,  305,  690,  689,   29,  310,  788,
 /*   550 */   211,  688,  304,  214,  776,  749,  315,  234,  768,  794,
 /*   560 */   762,  217,  764,  309,  791,  785,  221,  319,  314,  320,
 /*   570 */   225,  687,  760,  766,  322,  324,  235,  326,  686,  692,
 /*   580 */   676,  748,  239,  746,  747,  244,  246,  247,  675,  674,
 /*   590 */   673,  672,  671,   30,  709,  677,  647,  646,  645,  709,
 /*   600 */   644,  643,  642,  641,   31,  709,  628,  634,  631,  625,
 /*   610 */   709,  612,  618,  615,  609,  709,  602,  598,  541,  604,
 /*   620 */   709,  603,  542,  601,  540,  709,  600,  599,  597,  543,
 /*   630 */   709,  584,  709,  709,  709,   35,   36,   37,  709,   39,
 /*   640 */   578,  590,  709,  709,  709,   40,  580,   41,   43,  709,
 /*   650 */   587,  709,  703,  579,   44,   45,  709,  709,  581,  709,
 /*   660 */   709,  709,   47,   48,  709,  709,   49,  577,  570,  709,
 /*   670 */   709,  709,  572,  709,   52,  709,   55,  571,  709,  709,
 /*   680 */   709,  569,   57,  709,  709,   59,  538,  539,  709,   60,
 /*   690 */   709,  537,  536,   14,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*    10 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   42,
 /*    20 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*    30 */    53,   54,   27,   28,   29,   30,   31,   32,   33,   34,
 /*    40 */    35,   36,   37,   38,   39,   40,   41,   42,   43,   44,
 /*    50 */     0,   58,    2,   60,   57,    5,   63,   60,    8,    9,
 /*    60 */    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
 /*    70 */     2,    5,   21,   22,   60,    9,    8,   11,   10,   13,
 /*    80 */    12,   15,   14,   17,   16,   19,   18,   55,   56,    6,
 /*    90 */    58,   59,   60,    5,   62,   63,    2,    9,   58,   11,
 /*   100 */    60,   13,    8,   15,   10,   17,   12,   19,   14,    2,
 /*   110 */    16,  108,   18,  110,  111,    8,   56,   10,    2,   12,
 /*   120 */    60,   14,   62,   16,    8,   18,   10,    2,   12,   58,
 /*   130 */    14,   60,   16,    8,   18,   10,    2,   12,   58,   14,
 /*   140 */    60,   16,    8,   18,   10,    2,   12,   58,   14,   60,
 /*   150 */    16,    8,   18,   10,  113,   12,   56,   14,   55,   16,
 /*   160 */    60,   18,   59,   60,    3,   60,   48,   49,   50,   51,
 /*   170 */    52,   53,   48,   49,   50,   51,   52,   53,   29,  109,
 /*   180 */    29,   32,   28,   32,   35,   31,   35,   38,   34,   38,
 /*   190 */    41,   37,   41,   44,   40,   44,   28,   43,   27,   31,
 /*   200 */    27,   30,   34,   30,   33,   37,   33,   36,   40,   36,
 /*   210 */    39,   43,   39,   42,    3,   42,    3,   55,   56,    6,
 /*   220 */    58,    3,   60,    3,   55,   56,    6,   58,    7,   60,
 /*   230 */    55,   56,   56,   58,   57,   60,   60,   60,   61,  108,
 /*   240 */    56,  110,  111,   76,   60,   78,   79,  100,   56,  102,
 /*   250 */   103,   72,   60,   74,   75,   92,  115,   94,   95,   55,
 /*   260 */    88,   80,   90,   91,   60,   84,   76,   86,   78,   79,
 /*   270 */    72,    3,   74,   75,    6,    3,    3,    3,    6,    6,
 /*   280 */    55,   57,    3,   57,   60,   60,   60,   57,    3,   58,
 /*   290 */    60,   60,   58,   56,   60,   56,   55,   60,   55,   60,
 /*   300 */    55,   60,   55,   60,    3,   60,   57,   60,   57,   60,
 /*   310 */    57,   60,   58,   60,   60,   56,   58,   56,   60,   60,
 /*   320 */    55,   60,   55,   57,    3,   60,   60,   60,    4,  113,
 /*   330 */     6,    4,  113,    6,    3,  113,  113,    7,  109,    3,
 /*   340 */   113,  113,    3,    3,    3,  113,  113,    3,  113,  113,
 /*   350 */     7,  113,    3,  113,    3,  113,   79,  109,   75,  115,
 /*   360 */   114,  111,   75,   79,    3,    3,    3,  115,    3,    3,
 /*   370 */   115,  115,  115,  111,    7,  115,   95,   91,  115,  115,
 /*   380 */    78,  103,   74,   78,  115,  115,  115,   74,    3,    3,
 /*   390 */   115,  115,  110,    3,  114,  102,   90,   94,   72,    3,
 /*   400 */     3,    7,  114,  114,  114,  114,  114,  110,  114,  114,
 /*   410 */   114,   76,  114,  114,  114,    3,   88,   72,  114,  112,
 /*   420 */   108,  112,  112,  112,   92,  108,  112,  112,  112,  100,
 /*   430 */   112,  112,  112,   76,  112,  112,   77,  112,    3,  105,
 /*   440 */     7,   77,  105,  101,    3,  112,   86,   79,    3,    7,
 /*   450 */    78,    7,  104,  107,  107,   84,  106,    3,  106,   80,
 /*   460 */    76,    3,  104,    7,   73,   73,   97,   97,    3,   93,
 /*   470 */     7,    7,   99,   75,   99,   98,   74,    7,   98,   96,
 /*   480 */    72,    7,   65,   96,    3,   61,   65,    7,   69,   60,
 /*   490 */    60,   60,    3,   89,    7,   67,   60,   60,   60,    3,
 /*   500 */     7,   67,   63,   60,    3,    7,   66,   60,   60,    3,
 /*   510 */    59,   62,   66,   60,   64,   60,   64,    7,    7,    7,
 /*   520 */    60,   60,    7,    3,    7,    7,    7,  116,   71,   70,
 /*   530 */    68,  116,  116,    4,    4,    4,   69,  116,    4,    4,
 /*   540 */     4,    4,  116,    4,   71,    4,    4,    3,   70,   83,
 /*   550 */    82,    4,   83,   82,   77,   60,   68,   60,   73,   87,
 /*   560 */    69,   86,   70,   87,   85,   81,   84,   81,   85,   69,
 /*   570 */    80,    4,   68,   71,   71,   70,   60,   68,    4,    4,
 /*   580 */     4,   60,   60,   60,   60,   60,   60,   60,    4,    4,
 /*   590 */     4,    4,    4,    3,  116,    4,    4,    4,    4,  116,
 /*   600 */     4,    4,    4,    4,    3,  116,    4,    4,    4,    4,
 /*   610 */   116,    4,    4,    4,    4,  116,    4,    4,    4,    4,
 /*   620 */   116,    4,    4,    4,    4,  116,    4,    4,    4,    4,
 /*   630 */   116,    4,  116,  116,  116,    7,    7,    7,  116,    7,
 /*   640 */     4,    4,  116,  116,  116,    7,    4,    7,    7,  116,
 /*   650 */     4,  116,    1,    4,    7,    7,  116,  116,    4,  116,
 /*   660 */   116,  116,    7,    7,  116,  116,    7,    4,    4,  116,
 /*   670 */   116,  116,    4,  116,    7,  116,    7,    4,  116,  116,
 /*   680 */   116,    4,    7,  116,  116,    7,    4,    4,  116,    3,
 /*   690 */   116,    4,    4,    3,  116,  116,  116,  116,  116,  116,
 /*   700 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   710 */   116,  116,  116,  116,
};
#define YY_SHIFT_COUNT    (334)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (690)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */   694,   50,   68,  213,   66,   88,   94,  107,  116,  125,
 /*    10 */   134,  143,   83,   83,   83,  220,  268,  272,  273,  161,
 /*    20 */   161,  211,  218,  274,  279,  285,  301,  211,  274,   83,
 /*    30 */    83,   83,   83,   83,   83,   83,   83,   83,   83,   83,
 /*    40 */    83,   83,   83,   83,   83,   83,   83,   83,   83,   83,
 /*    50 */    83,   83,   83,   83,   83,   83,   83,   83,   83,   83,
 /*    60 */    83,  324,  327,  221,  321,  221,  221,  221,  221,  221,
 /*    70 */   221,  321,  221,  221,  221,  221,  221,  221,  221,  321,
 /*    80 */   330,  331,  330,  330,  330,  330,  330,  330,  331,  330,
 /*    90 */   330,  330,  330,  330,  330,  336,  339,  340,  341,  344,
 /*   100 */   336,  340,  330,  343,  349,  343,  343,  343,  343,  343,
 /*   110 */   343,  349,  343,  343,  343,  343,  343,  343,  351,  361,
 /*   120 */   362,  363,  365,  351,  362,  343,  367,  367,  366,  367,
 /*   130 */   367,  367,  367,  367,  366,  367,  367,  367,  367,  367,
 /*   140 */   367,  385,  386,  390,  396,  397,  385,  390,  367,  394,
 /*   150 */   412,  394,  412,  435,  433,  441,  336,  433,  442,  445,
 /*   160 */   351,  442,  444,  454,  385,  444,  456,  458,  456,  458,
 /*   170 */   465,  463,  340,  463,  464,  362,  464,  470,  390,  470,
 /*   180 */   474,  481,  474,  480,   83,   83,   83,  489,  487,   83,
 /*   190 */    83,   83,   83,  496,  487,  493,   83,   83,   83,  501,
 /*   200 */   493,  498,   83,   83,  506,  498,  510,  511,  512,   83,
 /*   210 */    83,  515,  480,  520,  515,  520,  412,  517,  510,  441,
 /*   220 */   517,  518,  511,  445,  518,  519,  512,  454,  519,  480,
 /*   230 */   480,  458,  510,  510,   83,   83,   83,  511,  511,   83,
 /*   240 */    83,  512,  512,   83,   83,   83,   83,   83,   83,  529,
 /*   250 */   530,  531,  534,  535,  536,  537,  539,  541,  542,  547,
 /*   260 */   567,  574,  544,  575,  576,  584,  585,  586,  587,  588,
 /*   270 */   590,  591,  592,  593,  594,  596,  597,  598,  601,  599,
 /*   280 */   602,  603,  604,  605,  607,  608,  609,  610,  612,  613,
 /*   290 */   614,  615,  617,  618,  619,  620,  622,  623,  624,  625,
 /*   300 */   627,  628,  629,  630,  636,  637,  632,  638,  640,  642,
 /*   310 */   646,  641,  647,  648,  649,  654,  655,  656,  659,  663,
 /*   320 */   664,  667,  668,  669,  673,  675,  677,  678,  682,  686,
 /*   330 */   683,  687,  688,  690,  651,
};
#define YY_REDUCE_COUNT (248)
#define YY_REDUCE_MIN   (-23)
#define YY_REDUCE_MAX   (527)
static const short yy_reduce_ofst[] = {
 /*     0 */    51,  -23,    5,   32,  118,  124,  149,  151,  154,  168,
 /*    10 */   171,  173,  162,  169,  175,   -7,   60,  103,  177,    3,
 /*    20 */   131,  167,  147,  179,  163,  172,  181,  190,  198,   40,
 /*    30 */   100,  204,   71,  176,  225,   -3,  224,  226,  230,   80,
 /*    40 */    89,  231,  234,  184,  192,  237,  239,  241,  243,  245,
 /*    50 */   247,  249,  251,  253,  254,  258,  259,  261,  265,  267,
 /*    60 */   266,   14,  105,   41,   70,  216,  219,  222,  223,  227,
 /*    70 */   228,  229,  232,  233,  235,  236,  238,  240,  242,  248,
 /*    80 */   141,  250,  244,  252,  255,  256,  257,  260,  262,  263,
 /*    90 */   264,  269,  270,  271,  275,  277,  278,  283,  281,  286,
 /*   100 */   284,  287,  276,  246,  282,  280,  288,  289,  290,  291,
 /*   110 */   292,  297,  294,  295,  296,  298,  299,  300,  302,  293,
 /*   120 */   308,  303,  306,  305,  313,  304,  307,  309,  312,  310,
 /*   130 */   311,  314,  315,  316,  317,  318,  319,  320,  322,  323,
 /*   140 */   325,  335,  329,  326,  332,  328,  357,  345,  333,  334,
 /*   150 */   359,  337,  364,  342,  346,  360,  368,  347,  350,  371,
 /*   160 */   372,  352,  348,  379,  384,  358,  369,  391,  370,  392,
 /*   170 */   376,  373,  398,  375,  377,  402,  380,  383,  408,  387,
 /*   180 */   417,  424,  421,  419,  429,  430,  431,  404,  428,   14,
 /*   190 */   436,  437,  438,  439,  434,  440,  443,  447,  448,  449,
 /*   200 */   446,  450,  453,  455,  451,  452,  457,  459,  462,  460,
 /*   210 */   461,  466,  467,  468,  469,  471,  477,  472,  473,  475,
 /*   220 */   476,  479,  478,  482,  483,  484,  488,  490,  486,  491,
 /*   230 */   500,  485,  502,  503,  495,  497,  516,  492,  505,  521,
 /*   240 */   522,  504,  509,  523,  524,  525,  495,  526,  527,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   904,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*    10 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*    20 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*    30 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*    40 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*    50 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*    60 */   707,  707,  707,  865,  707,  865,  865,  865,  865,  865,
 /*    70 */   865,  707,  865,  865,  865,  865,  865,  865,  865,  707,
 /*    80 */   895,  707,  895,  895,  895,  895,  895,  895,  707,  895,
 /*    90 */   895,  895,  895,  895,  895,  707,  707,  707,  707,  707,
 /*   100 */   707,  707,  895,  880,  707,  880,  880,  880,  880,  880,
 /*   110 */   880,  707,  880,  880,  880,  880,  880,  880,  707,  707,
 /*   120 */   707,  707,  707,  707,  707,  880,  850,  850,  707,  850,
 /*   130 */   850,  850,  850,  850,  707,  850,  850,  850,  850,  850,
 /*   140 */   850,  707,  707,  707,  707,  707,  707,  707,  850,  831,
 /*   150 */   707,  831,  707,  707,  837,  707,  707,  837,  834,  707,
 /*   160 */   707,  834,  828,  707,  707,  828,  815,  707,  815,  707,
 /*   170 */   707,  821,  707,  821,  818,  707,  818,  812,  707,  812,
 /*   180 */   753,  707,  753,  761,  707,  707,  707,  707,  757,  707,
 /*   190 */   707,  707,  707,  707,  757,  755,  707,  707,  707,  707,
 /*   200 */   755,  751,  707,  707,  707,  751,  765,  763,  759,  707,
 /*   210 */   707,  787,  761,  707,  787,  707,  707,  793,  765,  707,
 /*   220 */   793,  790,  763,  707,  790,  784,  759,  707,  784,  761,
 /*   230 */   761,  707,  765,  765,  707,  707,  707,  763,  763,  707,
 /*   240 */   707,  759,  759,  707,  707,  707,  748,  746,  707,  707,
 /*   250 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   260 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   270 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   280 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   290 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   300 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   310 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   320 */   707,  707,  707,  707,  707,  707,  707,  707,  707,  707,
 /*   330 */   707,  707,  707,  707,  707,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
  yyStackEntry *yystackEnd;            /* Last entry in the stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  /*    0 */ "$",
  /*    1 */ "EWKT_NEWLINE",
  /*    2 */ "EWKT_POINT",
  /*    3 */ "EWKT_OPEN_BRACKET",
  /*    4 */ "EWKT_CLOSE_BRACKET",
  /*    5 */ "EWKT_POINT_M",
  /*    6 */ "EWKT_NUM",
  /*    7 */ "EWKT_COMMA",
  /*    8 */ "EWKT_LINESTRING",
  /*    9 */ "EWKT_LINESTRING_M",
  /*   10 */ "EWKT_POLYGON",
  /*   11 */ "EWKT_POLYGON_M",
  /*   12 */ "EWKT_MULTIPOINT",
  /*   13 */ "EWKT_MULTIPOINT_M",
  /*   14 */ "EWKT_MULTILINESTRING",
  /*   15 */ "EWKT_MULTILINESTRING_M",
  /*   16 */ "EWKT_MULTIPOLYGON",
  /*   17 */ "EWKT_MULTIPOLYGON_M",
  /*   18 */ "EWKT_GEOMETRYCOLLECTION",
  /*   19 */ "EWKT_GEOMETRYCOLLECTION_M",
  /*   20 */ "error",
  /*   21 */ "main",
  /*   22 */ "in",
  /*   23 */ "state",
  /*   24 */ "program",
  /*   25 */ "geo_text",
  /*   26 */ "geo_textm",
  /*   27 */ "point",
  /*   28 */ "pointz",
  /*   29 */ "pointzm",
  /*   30 */ "linestring",
  /*   31 */ "linestringz",
  /*   32 */ "linestringzm",
  /*   33 */ "polygon",
  /*   34 */ "polygonz",
  /*   35 */ "polygonzm",
  /*   36 */ "multipoint",
  /*   37 */ "multipointz",
  /*   38 */ "multipointzm",
  /*   39 */ "multilinestring",
  /*   40 */ "multilinestringz",
  /*   41 */ "multilinestringzm",
  /*   42 */ "multipolygon",
  /*   43 */ "multipolygonz",
  /*   44 */ "multipolygonzm",
  /*   45 */ "geocoll",
  /*   46 */ "geocollz",
  /*   47 */ "geocollzm",
  /*   48 */ "pointm",
  /*   49 */ "linestringm",
  /*   50 */ "polygonm",
  /*   51 */ "multipointm",
  /*   52 */ "multilinestringm",
  /*   53 */ "multipolygonm",
  /*   54 */ "geocollm",
  /*   55 */ "point_coordxy",
  /*   56 */ "point_coordxyz",
  /*   57 */ "point_coordxym",
  /*   58 */ "point_coordxyzm",
  /*   59 */ "point_brkt_coordxy",
  /*   60 */ "coord",
  /*   61 */ "point_brkt_coordxym",
  /*   62 */ "point_brkt_coordxyz",
  /*   63 */ "point_brkt_coordxyzm",
  /*   64 */ "extra_brkt_pointsxy",
  /*   65 */ "extra_brkt_pointsxym",
  /*   66 */ "extra_brkt_pointsxyz",
  /*   67 */ "extra_brkt_pointsxyzm",
  /*   68 */ "extra_pointsxy",
  /*   69 */ "extra_pointsxym",
  /*   70 */ "extra_pointsxyz",
  /*   71 */ "extra_pointsxyzm",
  /*   72 */ "linestring_text",
  /*   73 */ "linestring_textm",
  /*   74 */ "linestring_textz",
  /*   75 */ "linestring_textzm",
  /*   76 */ "polygon_text",
  /*   77 */ "polygon_textm",
  /*   78 */ "polygon_textz",
  /*   79 */ "polygon_textzm",
  /*   80 */ "ring",
  /*   81 */ "extra_rings",
  /*   82 */ "ringm",
  /*   83 */ "extra_ringsm",
  /*   84 */ "ringz",
  /*   85 */ "extra_ringsz",
  /*   86 */ "ringzm",
  /*   87 */ "extra_ringszm",
  /*   88 */ "multipoint_text",
  /*   89 */ "multipoint_textm",
  /*   90 */ "multipoint_textz",
  /*   91 */ "multipoint_textzm",
  /*   92 */ "multilinestring_text",
  /*   93 */ "multilinestring_textm",
  /*   94 */ "multilinestring_textz",
  /*   95 */ "multilinestring_textzm",
  /*   96 */ "multilinestring_text2",
  /*   97 */ "multilinestring_textm2",
  /*   98 */ "multilinestring_textz2",
  /*   99 */ "multilinestring_textzm2",
  /*  100 */ "multipolygon_text",
  /*  101 */ "multipolygon_textm",
  /*  102 */ "multipolygon_textz",
  /*  103 */ "multipolygon_textzm",
  /*  104 */ "multipolygon_text2",
  /*  105 */ "multipolygon_textm2",
  /*  106 */ "multipolygon_textz2",
  /*  107 */ "multipolygon_textzm2",
  /*  108 */ "geocoll_text",
  /*  109 */ "geocoll_textm",
  /*  110 */ "geocoll_textz",
  /*  111 */ "geocoll_textzm",
  /*  112 */ "geocoll_text2",
  /*  113 */ "geocoll_textm2",
  /*  114 */ "geocoll_textz2",
  /*  115 */ "geocoll_textzm2",
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "geo_text ::= point",
 /*   1 */ "geo_text ::= pointz",
 /*   2 */ "geo_text ::= pointzm",
 /*   3 */ "geo_text ::= linestring",
 /*   4 */ "geo_text ::= linestringz",
 /*   5 */ "geo_text ::= linestringzm",
 /*   6 */ "geo_text ::= polygon",
 /*   7 */ "geo_text ::= polygonz",
 /*   8 */ "geo_text ::= polygonzm",
 /*   9 */ "geo_text ::= multipoint",
 /*  10 */ "geo_text ::= multipointz",
 /*  11 */ "geo_text ::= multipointzm",
 /*  12 */ "geo_text ::= multilinestring",
 /*  13 */ "geo_text ::= multilinestringz",
 /*  14 */ "geo_text ::= multilinestringzm",
 /*  15 */ "geo_text ::= multipolygon",
 /*  16 */ "geo_text ::= multipolygonz",
 /*  17 */ "geo_text ::= multipolygonzm",
 /*  18 */ "geo_text ::= geocoll",
 /*  19 */ "geo_text ::= geocollz",
 /*  20 */ "geo_text ::= geocollzm",
 /*  21 */ "geo_textm ::= pointm",
 /*  22 */ "geo_textm ::= linestringm",
 /*  23 */ "geo_textm ::= polygonm",
 /*  24 */ "geo_textm ::= multipointm",
 /*  25 */ "geo_textm ::= multilinestringm",
 /*  26 */ "geo_textm ::= multipolygonm",
 /*  27 */ "geo_textm ::= geocollm",
 /*  28 */ "point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET",
 /*  29 */ "pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET",
 /*  30 */ "pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET",
 /*  31 */ "pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET",
 /*  32 */ "point_brkt_coordxy ::= EWKT_OPEN_BRACKET coord coord EWKT_CLOSE_BRACKET",
 /*  33 */ "point_brkt_coordxym ::= EWKT_OPEN_BRACKET coord coord coord EWKT_CLOSE_BRACKET",
 /*  34 */ "point_brkt_coordxyz ::= EWKT_OPEN_BRACKET coord coord coord EWKT_CLOSE_BRACKET",
 /*  35 */ "point_brkt_coordxyzm ::= EWKT_OPEN_BRACKET coord coord coord coord EWKT_CLOSE_BRACKET",
 /*  36 */ "point_coordxy ::= coord coord",
 /*  37 */ "point_coordxym ::= coord coord coord",
 /*  38 */ "point_coordxyz ::= coord coord coord",
 /*  39 */ "point_coordxyzm ::= coord coord coord coord",
 /*  40 */ "coord ::= EWKT_NUM",
 /*  41 */ "extra_brkt_pointsxy ::=",
 /*  42 */ "extra_brkt_pointsxy ::= EWKT_COMMA point_brkt_coordxy extra_brkt_pointsxy",
 /*  43 */ "extra_brkt_pointsxym ::=",
 /*  44 */ "extra_brkt_pointsxym ::= EWKT_COMMA point_brkt_coordxym extra_brkt_pointsxym",
 /*  45 */ "extra_brkt_pointsxyz ::=",
 /*  46 */ "extra_brkt_pointsxyz ::= EWKT_COMMA point_brkt_coordxyz extra_brkt_pointsxyz",
 /*  47 */ "extra_brkt_pointsxyzm ::=",
 /*  48 */ "extra_brkt_pointsxyzm ::= EWKT_COMMA point_brkt_coordxyzm extra_brkt_pointsxyzm",
 /*  49 */ "extra_pointsxy ::=",
 /*  50 */ "extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy",
 /*  51 */ "extra_pointsxym ::=",
 /*  52 */ "extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym",
 /*  53 */ "extra_pointsxyz ::=",
 /*  54 */ "extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz",
 /*  55 */ "extra_pointsxyzm ::=",
 /*  56 */ "extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm",
 /*  57 */ "linestring ::= EWKT_LINESTRING linestring_text",
 /*  58 */ "linestringm ::= EWKT_LINESTRING_M linestring_textm",
 /*  59 */ "linestringz ::= EWKT_LINESTRING linestring_textz",
 /*  60 */ "linestringzm ::= EWKT_LINESTRING linestring_textzm",
 /*  61 */ "linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  62 */ "linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  63 */ "linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  64 */ "linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  65 */ "polygon ::= EWKT_POLYGON polygon_text",
 /*  66 */ "polygonm ::= EWKT_POLYGON_M polygon_textm",
 /*  67 */ "polygonz ::= EWKT_POLYGON polygon_textz",
 /*  68 */ "polygonzm ::= EWKT_POLYGON polygon_textzm",
 /*  69 */ "polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET",
 /*  70 */ "polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET",
 /*  71 */ "polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET",
 /*  72 */ "polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET",
 /*  73 */ "ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  74 */ "extra_rings ::=",
 /*  75 */ "extra_rings ::= EWKT_COMMA ring extra_rings",
 /*  76 */ "ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  77 */ "extra_ringsm ::=",
 /*  78 */ "extra_ringsm ::= EWKT_COMMA ringm extra_ringsm",
 /*  79 */ "ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  80 */ "extra_ringsz ::=",
 /*  81 */ "extra_ringsz ::= EWKT_COMMA ringz extra_ringsz",
 /*  82 */ "ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  83 */ "extra_ringszm ::=",
 /*  84 */ "extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm",
 /*  85 */ "multipoint ::= EWKT_MULTIPOINT multipoint_text",
 /*  86 */ "multipointm ::= EWKT_MULTIPOINT_M multipoint_textm",
 /*  87 */ "multipointz ::= EWKT_MULTIPOINT multipoint_textz",
 /*  88 */ "multipointzm ::= EWKT_MULTIPOINT multipoint_textzm",
 /*  89 */ "multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  90 */ "multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  91 */ "multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  92 */ "multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  93 */ "multipoint_text ::= EWKT_OPEN_BRACKET point_brkt_coordxy extra_brkt_pointsxy EWKT_CLOSE_BRACKET",
 /*  94 */ "multipoint_textm ::= EWKT_OPEN_BRACKET point_brkt_coordxym extra_brkt_pointsxym EWKT_CLOSE_BRACKET",
 /*  95 */ "multipoint_textz ::= EWKT_OPEN_BRACKET point_brkt_coordxyz extra_brkt_pointsxyz EWKT_CLOSE_BRACKET",
 /*  96 */ "multipoint_textzm ::= EWKT_OPEN_BRACKET point_brkt_coordxyzm extra_brkt_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  97 */ "multilinestring ::= EWKT_MULTILINESTRING multilinestring_text",
 /*  98 */ "multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm",
 /*  99 */ "multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz",
 /* 100 */ "multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm",
 /* 101 */ "multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET",
 /* 102 */ "multilinestring_text2 ::=",
 /* 103 */ "multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2",
 /* 104 */ "multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET",
 /* 105 */ "multilinestring_textm2 ::=",
 /* 106 */ "multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2",
 /* 107 */ "multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET",
 /* 108 */ "multilinestring_textz2 ::=",
 /* 109 */ "multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2",
 /* 110 */ "multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET",
 /* 111 */ "multilinestring_textzm2 ::=",
 /* 112 */ "multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2",
 /* 113 */ "multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text",
 /* 114 */ "multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm",
 /* 115 */ "multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz",
 /* 116 */ "multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm",
 /* 117 */ "multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET",
 /* 118 */ "multipolygon_text2 ::=",
 /* 119 */ "multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2",
 /* 120 */ "multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET",
 /* 121 */ "multipolygon_textm2 ::=",
 /* 122 */ "multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2",
 /* 123 */ "multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET",
 /* 124 */ "multipolygon_textz2 ::=",
 /* 125 */ "multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2",
 /* 126 */ "multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET",
 /* 127 */ "multipolygon_textzm2 ::=",
 /* 128 */ "multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2",
 /* 129 */ "geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text",
 /* 130 */ "geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm",
 /* 131 */ "geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz",
 /* 132 */ "geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm",
 /* 133 */ "geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 134 */ "geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 135 */ "geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 136 */ "geocoll_text ::= EWKT_OPEN_BRACKET multipoint geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 137 */ "geocoll_text ::= EWKT_OPEN_BRACKET multilinestring geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 138 */ "geocoll_text ::= EWKT_OPEN_BRACKET multipolygon geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 139 */ "geocoll_text ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_text geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 140 */ "geocoll_text2 ::=",
 /* 141 */ "geocoll_text2 ::= EWKT_COMMA point geocoll_text2",
 /* 142 */ "geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2",
 /* 143 */ "geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2",
 /* 144 */ "geocoll_text2 ::= EWKT_COMMA multipoint geocoll_text2",
 /* 145 */ "geocoll_text2 ::= EWKT_COMMA multilinestring geocoll_text2",
 /* 146 */ "geocoll_text2 ::= EWKT_COMMA multipolygon geocoll_text2",
 /* 147 */ "geocoll_text2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_text geocoll_text2",
 /* 148 */ "geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 149 */ "geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 150 */ "geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 151 */ "geocoll_textm ::= EWKT_OPEN_BRACKET multipointm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 152 */ "geocoll_textm ::= EWKT_OPEN_BRACKET multilinestringm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 153 */ "geocoll_textm ::= EWKT_OPEN_BRACKET multipolygonm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 154 */ "geocoll_textm ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 155 */ "geocoll_textm2 ::=",
 /* 156 */ "geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2",
 /* 157 */ "geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2",
 /* 158 */ "geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2",
 /* 159 */ "geocoll_textm2 ::= EWKT_COMMA multipointm geocoll_textm2",
 /* 160 */ "geocoll_textm2 ::= EWKT_COMMA multilinestringm geocoll_textm2",
 /* 161 */ "geocoll_textm2 ::= EWKT_COMMA multipolygonm geocoll_textm2",
 /* 162 */ "geocoll_textm2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2",
 /* 163 */ "geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 164 */ "geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 165 */ "geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 166 */ "geocoll_textz ::= EWKT_OPEN_BRACKET multipointz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 167 */ "geocoll_textz ::= EWKT_OPEN_BRACKET multilinestringz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 168 */ "geocoll_textz ::= EWKT_OPEN_BRACKET multipolygonz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 169 */ "geocoll_textz ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_textz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 170 */ "geocoll_textz2 ::=",
 /* 171 */ "geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2",
 /* 172 */ "geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2",
 /* 173 */ "geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2",
 /* 174 */ "geocoll_textz2 ::= EWKT_COMMA multipointz geocoll_textz2",
 /* 175 */ "geocoll_textz2 ::= EWKT_COMMA multilinestringz geocoll_textz2",
 /* 176 */ "geocoll_textz2 ::= EWKT_COMMA multipolygonz geocoll_textz2",
 /* 177 */ "geocoll_textz2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_textz geocoll_textz2",
 /* 178 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 179 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 180 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 181 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET multipointzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 182 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET multilinestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 183 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET multipolygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 184 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_textzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 185 */ "geocoll_textzm2 ::=",
 /* 186 */ "geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2",
 /* 187 */ "geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2",
 /* 188 */ "geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2",
 /* 189 */ "geocoll_textzm2 ::= EWKT_COMMA multipointzm geocoll_textzm2",
 /* 190 */ "geocoll_textzm2 ::= EWKT_COMMA multilinestringzm geocoll_textzm2",
 /* 191 */ "geocoll_textzm2 ::= EWKT_COMMA multipolygonzm geocoll_textzm2",
 /* 192 */ "geocoll_textzm2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_textzm geocoll_textzm2",
 /* 193 */ "main ::= in",
 /* 194 */ "in ::=",
 /* 195 */ "in ::= in state EWKT_NEWLINE",
 /* 196 */ "state ::= program",
 /* 197 */ "program ::= geo_text",
 /* 198 */ "program ::= geo_textm",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to ParseAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* Initialize a new parser that has already been allocated.
*/
static void ParseInit(void *yypParser){
  yyParser *pParser = (yyParser*)yypParser;
#ifdef YYTRACKMAXSTACKDEPTH
  pParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
  pParser->yytos = NULL;
  pParser->yystack = NULL;
  pParser->yystksz = 0;
  if( yyGrowStack(pParser) ){
    pParser->yystack = &pParser->yystk0;
    pParser->yystksz = 1;
  }
#endif
#ifndef YYNOERRORRECOVERY
  pParser->yyerrcnt = -1;
#endif
  pParser->yytos = pParser->yystack;
  pParser->yystack[0].stateno = 0;
  pParser->yystack[0].major = 0;
#if YYSTACKDEPTH>0
  pParser->yystackEnd = &pParser->yystack[YYSTACKDEPTH-1];
#endif
}

#ifndef Parse_ENGINEALWAYSONSTACK
/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ) ParseInit(pParser);
  return pParser;
}
#endif /* Parse_ENGINEALWAYSONSTACK */


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/*
** Clear all secondary memory allocations from the parser
*/
static void ParseFinalize(void *p){
  yyParser *pParser = (yyParser*)p;
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
}

#ifndef Parse_ENGINEALWAYSONSTACK
/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
#ifndef YYPARSEFREENEVERNULL
  if( p==0 ) return;
#endif
  ParseFinalize(p);
  (*freeProc)(p);
}
#endif /* Parse_ENGINEALWAYSONSTACK */

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int ParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/* This array of booleans keeps track of the parser statement
** coverage.  The element yycoverage[X][Y] is set when the parser
** is in state X and has a lookahead token Y.  In a well-tested
** systems, every element of this matrix should end up being set.
*/
#if defined(YYCOVERAGE)
static unsigned char yycoverage[YYNSTATE][YYNTOKEN];
#endif

/*
** Write into out a description of every state/lookahead combination that
**
**   (1)  has not been used by the parser, and
**   (2)  is not a syntax error.
**
** Return the number of missed state/lookahead combinations.
*/
#if defined(YYCOVERAGE)
int ParseCoverage(FILE *out){
  int stateno, iLookAhead, i;
  int nMissed = 0;
  for(stateno=0; stateno<YYNSTATE; stateno++){
    i = yy_shift_ofst[stateno];
    for(iLookAhead=0; iLookAhead<YYNTOKEN; iLookAhead++){
      if( yy_lookahead[i+iLookAhead]!=iLookAhead ) continue;
      if( yycoverage[stateno][iLookAhead]==0 ) nMissed++;
      if( out ){
        fprintf(out,"State %d lookahead %s %s\n", stateno,
                yyTokenName[iLookAhead],
                yycoverage[stateno][iLookAhead] ? "ok" : "missed");
      }
    }
  }
  return nMissed;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yytos->stateno;
 
  if( stateno>YY_MAX_SHIFT ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
#if defined(YYCOVERAGE)
  yycoverage[stateno][iLookAhead] = 1;
#endif
  do{
    i = yy_shift_ofst[stateno];
    assert( i>=0 );
    assert( i+YYNTOKEN<=(int)sizeof(yy_lookahead)/sizeof(yy_lookahead[0]) );
    assert( iLookAhead!=YYNOCODE );
    assert( iLookAhead < YYNTOKEN );
    i += iLookAhead;
    if( yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   ParseARG_FETCH;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/

     spatialite_e( "Giving up.  Parser stack overflow\n");
/******** End %stack_overflow code ********************************************/
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState, const char *zTag){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%s%s '%s', go to state %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%s%s '%s', pending reduce %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState - YY_MIN_REDUCE);
    }
  }
}
#else
# define yyTraceShift(X,Y,Z)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  ParseTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>yypParser->yystackEnd ){
    yypParser->yytos--;
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState, "Shift");
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;       /* Symbol on the left-hand side of the rule */
  signed char nrhs;     /* Negative of the number of RHS symbols in the rule */
} yyRuleInfo[] = {
  {   25,   -1 }, /* (0) geo_text ::= point */
  {   25,   -1 }, /* (1) geo_text ::= pointz */
  {   25,   -1 }, /* (2) geo_text ::= pointzm */
  {   25,   -1 }, /* (3) geo_text ::= linestring */
  {   25,   -1 }, /* (4) geo_text ::= linestringz */
  {   25,   -1 }, /* (5) geo_text ::= linestringzm */
  {   25,   -1 }, /* (6) geo_text ::= polygon */
  {   25,   -1 }, /* (7) geo_text ::= polygonz */
  {   25,   -1 }, /* (8) geo_text ::= polygonzm */
  {   25,   -1 }, /* (9) geo_text ::= multipoint */
  {   25,   -1 }, /* (10) geo_text ::= multipointz */
  {   25,   -1 }, /* (11) geo_text ::= multipointzm */
  {   25,   -1 }, /* (12) geo_text ::= multilinestring */
  {   25,   -1 }, /* (13) geo_text ::= multilinestringz */
  {   25,   -1 }, /* (14) geo_text ::= multilinestringzm */
  {   25,   -1 }, /* (15) geo_text ::= multipolygon */
  {   25,   -1 }, /* (16) geo_text ::= multipolygonz */
  {   25,   -1 }, /* (17) geo_text ::= multipolygonzm */
  {   25,   -1 }, /* (18) geo_text ::= geocoll */
  {   25,   -1 }, /* (19) geo_text ::= geocollz */
  {   25,   -1 }, /* (20) geo_text ::= geocollzm */
  {   26,   -1 }, /* (21) geo_textm ::= pointm */
  {   26,   -1 }, /* (22) geo_textm ::= linestringm */
  {   26,   -1 }, /* (23) geo_textm ::= polygonm */
  {   26,   -1 }, /* (24) geo_textm ::= multipointm */
  {   26,   -1 }, /* (25) geo_textm ::= multilinestringm */
  {   26,   -1 }, /* (26) geo_textm ::= multipolygonm */
  {   26,   -1 }, /* (27) geo_textm ::= geocollm */
  {   27,   -4 }, /* (28) point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET */
  {   28,   -4 }, /* (29) pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET */
  {   48,   -4 }, /* (30) pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET */
  {   29,   -4 }, /* (31) pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET */
  {   59,   -4 }, /* (32) point_brkt_coordxy ::= EWKT_OPEN_BRACKET coord coord EWKT_CLOSE_BRACKET */
  {   61,   -5 }, /* (33) point_brkt_coordxym ::= EWKT_OPEN_BRACKET coord coord coord EWKT_CLOSE_BRACKET */
  {   62,   -5 }, /* (34) point_brkt_coordxyz ::= EWKT_OPEN_BRACKET coord coord coord EWKT_CLOSE_BRACKET */
  {   63,   -6 }, /* (35) point_brkt_coordxyzm ::= EWKT_OPEN_BRACKET coord coord coord coord EWKT_CLOSE_BRACKET */
  {   55,   -2 }, /* (36) point_coordxy ::= coord coord */
  {   57,   -3 }, /* (37) point_coordxym ::= coord coord coord */
  {   56,   -3 }, /* (38) point_coordxyz ::= coord coord coord */
  {   58,   -4 }, /* (39) point_coordxyzm ::= coord coord coord coord */
  {   60,   -1 }, /* (40) coord ::= EWKT_NUM */
  {   64,    0 }, /* (41) extra_brkt_pointsxy ::= */
  {   64,   -3 }, /* (42) extra_brkt_pointsxy ::= EWKT_COMMA point_brkt_coordxy extra_brkt_pointsxy */
  {   65,    0 }, /* (43) extra_brkt_pointsxym ::= */
  {   65,   -3 }, /* (44) extra_brkt_pointsxym ::= EWKT_COMMA point_brkt_coordxym extra_brkt_pointsxym */
  {   66,    0 }, /* (45) extra_brkt_pointsxyz ::= */
  {   66,   -3 }, /* (46) extra_brkt_pointsxyz ::= EWKT_COMMA point_brkt_coordxyz extra_brkt_pointsxyz */
  {   67,    0 }, /* (47) extra_brkt_pointsxyzm ::= */
  {   67,   -3 }, /* (48) extra_brkt_pointsxyzm ::= EWKT_COMMA point_brkt_coordxyzm extra_brkt_pointsxyzm */
  {   68,    0 }, /* (49) extra_pointsxy ::= */
  {   68,   -3 }, /* (50) extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy */
  {   69,    0 }, /* (51) extra_pointsxym ::= */
  {   69,   -3 }, /* (52) extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym */
  {   70,    0 }, /* (53) extra_pointsxyz ::= */
  {   70,   -3 }, /* (54) extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz */
  {   71,    0 }, /* (55) extra_pointsxyzm ::= */
  {   71,   -3 }, /* (56) extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm */
  {   30,   -2 }, /* (57) linestring ::= EWKT_LINESTRING linestring_text */
  {   49,   -2 }, /* (58) linestringm ::= EWKT_LINESTRING_M linestring_textm */
  {   31,   -2 }, /* (59) linestringz ::= EWKT_LINESTRING linestring_textz */
  {   32,   -2 }, /* (60) linestringzm ::= EWKT_LINESTRING linestring_textzm */
  {   72,   -6 }, /* (61) linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
  {   73,   -6 }, /* (62) linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
  {   74,   -6 }, /* (63) linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
  {   75,   -6 }, /* (64) linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
  {   33,   -2 }, /* (65) polygon ::= EWKT_POLYGON polygon_text */
  {   50,   -2 }, /* (66) polygonm ::= EWKT_POLYGON_M polygon_textm */
  {   34,   -2 }, /* (67) polygonz ::= EWKT_POLYGON polygon_textz */
  {   35,   -2 }, /* (68) polygonzm ::= EWKT_POLYGON polygon_textzm */
  {   76,   -4 }, /* (69) polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET */
  {   77,   -4 }, /* (70) polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET */
  {   78,   -4 }, /* (71) polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET */
  {   79,   -4 }, /* (72) polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET */
  {   80,  -10 }, /* (73) ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
  {   81,    0 }, /* (74) extra_rings ::= */
  {   81,   -3 }, /* (75) extra_rings ::= EWKT_COMMA ring extra_rings */
  {   82,  -10 }, /* (76) ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
  {   83,    0 }, /* (77) extra_ringsm ::= */
  {   83,   -3 }, /* (78) extra_ringsm ::= EWKT_COMMA ringm extra_ringsm */
  {   84,  -10 }, /* (79) ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
  {   85,    0 }, /* (80) extra_ringsz ::= */
  {   85,   -3 }, /* (81) extra_ringsz ::= EWKT_COMMA ringz extra_ringsz */
  {   86,  -10 }, /* (82) ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
  {   87,    0 }, /* (83) extra_ringszm ::= */
  {   87,   -3 }, /* (84) extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm */
  {   36,   -2 }, /* (85) multipoint ::= EWKT_MULTIPOINT multipoint_text */
  {   51,   -2 }, /* (86) multipointm ::= EWKT_MULTIPOINT_M multipoint_textm */
  {   37,   -2 }, /* (87) multipointz ::= EWKT_MULTIPOINT multipoint_textz */
  {   38,   -2 }, /* (88) multipointzm ::= EWKT_MULTIPOINT multipoint_textzm */
  {   88,   -4 }, /* (89) multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
  {   89,   -4 }, /* (90) multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
  {   90,   -4 }, /* (91) multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
  {   91,   -4 }, /* (92) multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
  {   88,   -4 }, /* (93) multipoint_text ::= EWKT_OPEN_BRACKET point_brkt_coordxy extra_brkt_pointsxy EWKT_CLOSE_BRACKET */
  {   89,   -4 }, /* (94) multipoint_textm ::= EWKT_OPEN_BRACKET point_brkt_coordxym extra_brkt_pointsxym EWKT_CLOSE_BRACKET */
  {   90,   -4 }, /* (95) multipoint_textz ::= EWKT_OPEN_BRACKET point_brkt_coordxyz extra_brkt_pointsxyz EWKT_CLOSE_BRACKET */
  {   91,   -4 }, /* (96) multipoint_textzm ::= EWKT_OPEN_BRACKET point_brkt_coordxyzm extra_brkt_pointsxyzm EWKT_CLOSE_BRACKET */
  {   39,   -2 }, /* (97) multilinestring ::= EWKT_MULTILINESTRING multilinestring_text */
  {   52,   -2 }, /* (98) multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm */
  {   40,   -2 }, /* (99) multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz */
  {   41,   -2 }, /* (100) multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm */
  {   92,   -4 }, /* (101) multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET */
  {   96,    0 }, /* (102) multilinestring_text2 ::= */
  {   96,   -3 }, /* (103) multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2 */
  {   93,   -4 }, /* (104) multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET */
  {   97,    0 }, /* (105) multilinestring_textm2 ::= */
  {   97,   -3 }, /* (106) multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2 */
  {   94,   -4 }, /* (107) multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET */
  {   98,    0 }, /* (108) multilinestring_textz2 ::= */
  {   98,   -3 }, /* (109) multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2 */
  {   95,   -4 }, /* (110) multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET */
  {   99,    0 }, /* (111) multilinestring_textzm2 ::= */
  {   99,   -3 }, /* (112) multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2 */
  {   42,   -2 }, /* (113) multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text */
  {   53,   -2 }, /* (114) multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm */
  {   43,   -2 }, /* (115) multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz */
  {   44,   -2 }, /* (116) multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm */
  {  100,   -4 }, /* (117) multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET */
  {  104,    0 }, /* (118) multipolygon_text2 ::= */
  {  104,   -3 }, /* (119) multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2 */
  {  101,   -4 }, /* (120) multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET */
  {  105,    0 }, /* (121) multipolygon_textm2 ::= */
  {  105,   -3 }, /* (122) multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2 */
  {  102,   -4 }, /* (123) multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET */
  {  106,    0 }, /* (124) multipolygon_textz2 ::= */
  {  106,   -3 }, /* (125) multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2 */
  {  103,   -4 }, /* (126) multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET */
  {  107,    0 }, /* (127) multipolygon_textzm2 ::= */
  {  107,   -3 }, /* (128) multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2 */
  {   45,   -2 }, /* (129) geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text */
  {   54,   -2 }, /* (130) geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm */
  {   46,   -2 }, /* (131) geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz */
  {   47,   -2 }, /* (132) geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm */
  {  108,   -4 }, /* (133) geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET */
  {  108,   -4 }, /* (134) geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET */
  {  108,   -4 }, /* (135) geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET */
  {  108,   -4 }, /* (136) geocoll_text ::= EWKT_OPEN_BRACKET multipoint geocoll_text2 EWKT_CLOSE_BRACKET */
  {  108,   -4 }, /* (137) geocoll_text ::= EWKT_OPEN_BRACKET multilinestring geocoll_text2 EWKT_CLOSE_BRACKET */
  {  108,   -4 }, /* (138) geocoll_text ::= EWKT_OPEN_BRACKET multipolygon geocoll_text2 EWKT_CLOSE_BRACKET */
  {  108,   -5 }, /* (139) geocoll_text ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_text geocoll_text2 EWKT_CLOSE_BRACKET */
  {  112,    0 }, /* (140) geocoll_text2 ::= */
  {  112,   -3 }, /* (141) geocoll_text2 ::= EWKT_COMMA point geocoll_text2 */
  {  112,   -3 }, /* (142) geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2 */
  {  112,   -3 }, /* (143) geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2 */
  {  112,   -3 }, /* (144) geocoll_text2 ::= EWKT_COMMA multipoint geocoll_text2 */
  {  112,   -3 }, /* (145) geocoll_text2 ::= EWKT_COMMA multilinestring geocoll_text2 */
  {  112,   -3 }, /* (146) geocoll_text2 ::= EWKT_COMMA multipolygon geocoll_text2 */
  {  112,   -4 }, /* (147) geocoll_text2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_text geocoll_text2 */
  {  109,   -4 }, /* (148) geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  109,   -4 }, /* (149) geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  109,   -4 }, /* (150) geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  109,   -4 }, /* (151) geocoll_textm ::= EWKT_OPEN_BRACKET multipointm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  109,   -4 }, /* (152) geocoll_textm ::= EWKT_OPEN_BRACKET multilinestringm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  109,   -4 }, /* (153) geocoll_textm ::= EWKT_OPEN_BRACKET multipolygonm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  109,   -5 }, /* (154) geocoll_textm ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 EWKT_CLOSE_BRACKET */
  {  113,    0 }, /* (155) geocoll_textm2 ::= */
  {  113,   -3 }, /* (156) geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2 */
  {  113,   -3 }, /* (157) geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2 */
  {  113,   -3 }, /* (158) geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2 */
  {  113,   -3 }, /* (159) geocoll_textm2 ::= EWKT_COMMA multipointm geocoll_textm2 */
  {  113,   -3 }, /* (160) geocoll_textm2 ::= EWKT_COMMA multilinestringm geocoll_textm2 */
  {  113,   -3 }, /* (161) geocoll_textm2 ::= EWKT_COMMA multipolygonm geocoll_textm2 */
  {  113,   -4 }, /* (162) geocoll_textm2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 */
  {  110,   -4 }, /* (163) geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  110,   -4 }, /* (164) geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  110,   -4 }, /* (165) geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  110,   -4 }, /* (166) geocoll_textz ::= EWKT_OPEN_BRACKET multipointz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  110,   -4 }, /* (167) geocoll_textz ::= EWKT_OPEN_BRACKET multilinestringz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  110,   -4 }, /* (168) geocoll_textz ::= EWKT_OPEN_BRACKET multipolygonz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  110,   -5 }, /* (169) geocoll_textz ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_textz geocoll_textz2 EWKT_CLOSE_BRACKET */
  {  114,    0 }, /* (170) geocoll_textz2 ::= */
  {  114,   -3 }, /* (171) geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2 */
  {  114,   -3 }, /* (172) geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2 */
  {  114,   -3 }, /* (173) geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2 */
  {  114,   -3 }, /* (174) geocoll_textz2 ::= EWKT_COMMA multipointz geocoll_textz2 */
  {  114,   -3 }, /* (175) geocoll_textz2 ::= EWKT_COMMA multilinestringz geocoll_textz2 */
  {  114,   -3 }, /* (176) geocoll_textz2 ::= EWKT_COMMA multipolygonz geocoll_textz2 */
  {  114,   -4 }, /* (177) geocoll_textz2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_textz geocoll_textz2 */
  {  111,   -4 }, /* (178) geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  111,   -4 }, /* (179) geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  111,   -4 }, /* (180) geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  111,   -4 }, /* (181) geocoll_textzm ::= EWKT_OPEN_BRACKET multipointzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  111,   -4 }, /* (182) geocoll_textzm ::= EWKT_OPEN_BRACKET multilinestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  111,   -4 }, /* (183) geocoll_textzm ::= EWKT_OPEN_BRACKET multipolygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  111,   -5 }, /* (184) geocoll_textzm ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_textzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
  {  115,    0 }, /* (185) geocoll_textzm2 ::= */
  {  115,   -3 }, /* (186) geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2 */
  {  115,   -3 }, /* (187) geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2 */
  {  115,   -3 }, /* (188) geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2 */
  {  115,   -3 }, /* (189) geocoll_textzm2 ::= EWKT_COMMA multipointzm geocoll_textzm2 */
  {  115,   -3 }, /* (190) geocoll_textzm2 ::= EWKT_COMMA multilinestringzm geocoll_textzm2 */
  {  115,   -3 }, /* (191) geocoll_textzm2 ::= EWKT_COMMA multipolygonzm geocoll_textzm2 */
  {  115,   -4 }, /* (192) geocoll_textzm2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_textzm geocoll_textzm2 */
  {   21,   -1 }, /* (193) main ::= in */
  {   22,    0 }, /* (194) in ::= */
  {   22,   -3 }, /* (195) in ::= in state EWKT_NEWLINE */
  {   23,   -1 }, /* (196) state ::= program */
  {   24,   -1 }, /* (197) program ::= geo_text */
  {   24,   -1 }, /* (198) program ::= geo_textm */
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
**
** The yyLookahead and yyLookaheadToken parameters provide reduce actions
** access to the lookahead token (if any).  The yyLookahead will be YYNOCODE
** if the lookahead token has already been consumed.  As this procedure is
** only called from one place, optimizing compilers will in-line it, which
** means that the extra parameters have no performance impact.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno,       /* Number of the rule by which to reduce */
  int yyLookahead,             /* Lookahead token, or YYNOCODE if none */
  ParseTOKENTYPE yyLookaheadToken  /* Value of the lookahead token */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  (void)yyLookahead;
  (void)yyLookaheadToken;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    if( yysize ){
      fprintf(yyTraceFILE, "%sReduce %d [%s], go to state %d.\n",
        yyTracePrompt,
        yyruleno, yyRuleName[yyruleno], yymsp[yysize].stateno);
    }else{
      fprintf(yyTraceFILE, "%sReduce %d [%s].\n",
        yyTracePrompt, yyruleno, yyRuleName[yyruleno]);
    }
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=yypParser->yystackEnd ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        return;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* geo_text ::= point */
      case 1: /* geo_text ::= pointz */ yytestcase(yyruleno==1);
      case 2: /* geo_text ::= pointzm */ yytestcase(yyruleno==2);
      case 3: /* geo_text ::= linestring */ yytestcase(yyruleno==3);
      case 4: /* geo_text ::= linestringz */ yytestcase(yyruleno==4);
      case 5: /* geo_text ::= linestringzm */ yytestcase(yyruleno==5);
      case 6: /* geo_text ::= polygon */ yytestcase(yyruleno==6);
      case 7: /* geo_text ::= polygonz */ yytestcase(yyruleno==7);
      case 8: /* geo_text ::= polygonzm */ yytestcase(yyruleno==8);
      case 9: /* geo_text ::= multipoint */ yytestcase(yyruleno==9);
      case 10: /* geo_text ::= multipointz */ yytestcase(yyruleno==10);
      case 11: /* geo_text ::= multipointzm */ yytestcase(yyruleno==11);
      case 12: /* geo_text ::= multilinestring */ yytestcase(yyruleno==12);
      case 13: /* geo_text ::= multilinestringz */ yytestcase(yyruleno==13);
      case 14: /* geo_text ::= multilinestringzm */ yytestcase(yyruleno==14);
      case 15: /* geo_text ::= multipolygon */ yytestcase(yyruleno==15);
      case 16: /* geo_text ::= multipolygonz */ yytestcase(yyruleno==16);
      case 17: /* geo_text ::= multipolygonzm */ yytestcase(yyruleno==17);
      case 18: /* geo_text ::= geocoll */ yytestcase(yyruleno==18);
      case 19: /* geo_text ::= geocollz */ yytestcase(yyruleno==19);
      case 20: /* geo_text ::= geocollzm */ yytestcase(yyruleno==20);
      case 21: /* geo_textm ::= pointm */ yytestcase(yyruleno==21);
      case 22: /* geo_textm ::= linestringm */ yytestcase(yyruleno==22);
      case 23: /* geo_textm ::= polygonm */ yytestcase(yyruleno==23);
      case 24: /* geo_textm ::= multipointm */ yytestcase(yyruleno==24);
      case 25: /* geo_textm ::= multilinestringm */ yytestcase(yyruleno==25);
      case 26: /* geo_textm ::= multipolygonm */ yytestcase(yyruleno==26);
      case 27: /* geo_textm ::= geocollm */ yytestcase(yyruleno==27);
{ p_data->result = yymsp[0].minor.yy0; }
        break;
      case 28: /* point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET */
      case 29: /* pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==29);
      case 31: /* pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==31);
{ yymsp[-3].minor.yy0 = ewkt_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 30: /* pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET */
{ yymsp[-3].minor.yy0 = ewkt_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0);  }
        break;
      case 32: /* point_brkt_coordxy ::= EWKT_OPEN_BRACKET coord coord EWKT_CLOSE_BRACKET */
{ yymsp[-3].minor.yy0 = (void *) ewkt_point_xy( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 33: /* point_brkt_coordxym ::= EWKT_OPEN_BRACKET coord coord coord EWKT_CLOSE_BRACKET */
{ yymsp[-4].minor.yy0 = (void *) ewkt_point_xym( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 34: /* point_brkt_coordxyz ::= EWKT_OPEN_BRACKET coord coord coord EWKT_CLOSE_BRACKET */
{ yymsp[-4].minor.yy0 = (void *) ewkt_point_xyz( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 35: /* point_brkt_coordxyzm ::= EWKT_OPEN_BRACKET coord coord coord coord EWKT_CLOSE_BRACKET */
{ yymsp[-5].minor.yy0 = (void *) ewkt_point_xyzm( p_data, (double *)yymsp[-4].minor.yy0, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 36: /* point_coordxy ::= coord coord */
{ yylhsminor.yy0 = (void *) ewkt_point_xy( p_data, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-1].minor.yy0 = yylhsminor.yy0;
        break;
      case 37: /* point_coordxym ::= coord coord coord */
{ yylhsminor.yy0 = (void *) ewkt_point_xym( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 38: /* point_coordxyz ::= coord coord coord */
{ yylhsminor.yy0 = (void *) ewkt_point_xyz( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 39: /* point_coordxyzm ::= coord coord coord coord */
{ yylhsminor.yy0 = (void *) ewkt_point_xyzm( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-3].minor.yy0 = yylhsminor.yy0;
        break;
      case 40: /* coord ::= EWKT_NUM */
{ yylhsminor.yy0 = yymsp[0].minor.yy0; }
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 41: /* extra_brkt_pointsxy ::= */
      case 43: /* extra_brkt_pointsxym ::= */ yytestcase(yyruleno==43);
      case 45: /* extra_brkt_pointsxyz ::= */ yytestcase(yyruleno==45);
      case 47: /* extra_brkt_pointsxyzm ::= */ yytestcase(yyruleno==47);
      case 49: /* extra_pointsxy ::= */ yytestcase(yyruleno==49);
      case 51: /* extra_pointsxym ::= */ yytestcase(yyruleno==51);
      case 53: /* extra_pointsxyz ::= */ yytestcase(yyruleno==53);
      case 55: /* extra_pointsxyzm ::= */ yytestcase(yyruleno==55);
      case 74: /* extra_rings ::= */ yytestcase(yyruleno==74);
      case 77: /* extra_ringsm ::= */ yytestcase(yyruleno==77);
      case 80: /* extra_ringsz ::= */ yytestcase(yyruleno==80);
      case 83: /* extra_ringszm ::= */ yytestcase(yyruleno==83);
      case 102: /* multilinestring_text2 ::= */ yytestcase(yyruleno==102);
      case 105: /* multilinestring_textm2 ::= */ yytestcase(yyruleno==105);
      case 108: /* multilinestring_textz2 ::= */ yytestcase(yyruleno==108);
      case 111: /* multilinestring_textzm2 ::= */ yytestcase(yyruleno==111);
      case 118: /* multipolygon_text2 ::= */ yytestcase(yyruleno==118);
      case 121: /* multipolygon_textm2 ::= */ yytestcase(yyruleno==121);
      case 124: /* multipolygon_textz2 ::= */ yytestcase(yyruleno==124);
      case 127: /* multipolygon_textzm2 ::= */ yytestcase(yyruleno==127);
      case 140: /* geocoll_text2 ::= */ yytestcase(yyruleno==140);
      case 155: /* geocoll_textm2 ::= */ yytestcase(yyruleno==155);
      case 170: /* geocoll_textz2 ::= */ yytestcase(yyruleno==170);
      case 185: /* geocoll_textzm2 ::= */ yytestcase(yyruleno==185);
{ yymsp[1].minor.yy0 = NULL; }
        break;
      case 42: /* extra_brkt_pointsxy ::= EWKT_COMMA point_brkt_coordxy extra_brkt_pointsxy */
      case 44: /* extra_brkt_pointsxym ::= EWKT_COMMA point_brkt_coordxym extra_brkt_pointsxym */ yytestcase(yyruleno==44);
      case 46: /* extra_brkt_pointsxyz ::= EWKT_COMMA point_brkt_coordxyz extra_brkt_pointsxyz */ yytestcase(yyruleno==46);
      case 48: /* extra_brkt_pointsxyzm ::= EWKT_COMMA point_brkt_coordxyzm extra_brkt_pointsxyzm */ yytestcase(yyruleno==48);
      case 50: /* extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy */ yytestcase(yyruleno==50);
      case 52: /* extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym */ yytestcase(yyruleno==52);
      case 54: /* extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==54);
      case 56: /* extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm */ yytestcase(yyruleno==56);
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 57: /* linestring ::= EWKT_LINESTRING linestring_text */
      case 58: /* linestringm ::= EWKT_LINESTRING_M linestring_textm */ yytestcase(yyruleno==58);
      case 59: /* linestringz ::= EWKT_LINESTRING linestring_textz */ yytestcase(yyruleno==59);
      case 60: /* linestringzm ::= EWKT_LINESTRING linestring_textzm */ yytestcase(yyruleno==60);
{ yymsp[-1].minor.yy0 = ewkt_buildGeomFromLinestring( p_data, (gaiaLinestringPtr)yymsp[0].minor.yy0); }
        break;
      case 61: /* linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) ewkt_linestring_xy( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 62: /* linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) ewkt_linestring_xym( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 63: /* linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) ewkt_linestring_xyz( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 64: /* linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) ewkt_linestring_xyzm( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 65: /* polygon ::= EWKT_POLYGON polygon_text */
      case 66: /* polygonm ::= EWKT_POLYGON_M polygon_textm */ yytestcase(yyruleno==66);
      case 67: /* polygonz ::= EWKT_POLYGON polygon_textz */ yytestcase(yyruleno==67);
      case 68: /* polygonzm ::= EWKT_POLYGON polygon_textzm */ yytestcase(yyruleno==68);
{ yymsp[-1].minor.yy0 = ewkt_buildGeomFromPolygon( p_data, (gaiaPolygonPtr)yymsp[0].minor.yy0); }
        break;
      case 69: /* polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_polygon_xy( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 70: /* polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_polygon_xym( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 71: /* polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET */
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_polygon_xyz( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 72: /* polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_polygon_xyzm( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 73: /* ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) ewkt_ring_xy( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 75: /* extra_rings ::= EWKT_COMMA ring extra_rings */
      case 78: /* extra_ringsm ::= EWKT_COMMA ringm extra_ringsm */ yytestcase(yyruleno==78);
      case 81: /* extra_ringsz ::= EWKT_COMMA ringz extra_ringsz */ yytestcase(yyruleno==81);
      case 84: /* extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm */ yytestcase(yyruleno==84);
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 76: /* ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) ewkt_ring_xym( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 79: /* ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) ewkt_ring_xyz( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 82: /* ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) ewkt_ring_xyzm( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 85: /* multipoint ::= EWKT_MULTIPOINT multipoint_text */
      case 86: /* multipointm ::= EWKT_MULTIPOINT_M multipoint_textm */ yytestcase(yyruleno==86);
      case 87: /* multipointz ::= EWKT_MULTIPOINT multipoint_textz */ yytestcase(yyruleno==87);
      case 88: /* multipointzm ::= EWKT_MULTIPOINT multipoint_textzm */ yytestcase(yyruleno==88);
      case 97: /* multilinestring ::= EWKT_MULTILINESTRING multilinestring_text */ yytestcase(yyruleno==97);
      case 98: /* multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm */ yytestcase(yyruleno==98);
      case 99: /* multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz */ yytestcase(yyruleno==99);
      case 100: /* multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm */ yytestcase(yyruleno==100);
      case 113: /* multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text */ yytestcase(yyruleno==113);
      case 114: /* multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm */ yytestcase(yyruleno==114);
      case 115: /* multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz */ yytestcase(yyruleno==115);
      case 116: /* multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm */ yytestcase(yyruleno==116);
      case 129: /* geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text */ yytestcase(yyruleno==129);
      case 130: /* geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm */ yytestcase(yyruleno==130);
      case 131: /* geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz */ yytestcase(yyruleno==131);
      case 132: /* geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm */ yytestcase(yyruleno==132);
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 89: /* multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
      case 93: /* multipoint_text ::= EWKT_OPEN_BRACKET point_brkt_coordxy extra_brkt_pointsxy EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==93);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipoint_xy( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 90: /* multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
      case 94: /* multipoint_textm ::= EWKT_OPEN_BRACKET point_brkt_coordxym extra_brkt_pointsxym EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==94);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipoint_xym( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 91: /* multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
      case 95: /* multipoint_textz ::= EWKT_OPEN_BRACKET point_brkt_coordxyz extra_brkt_pointsxyz EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==95);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipoint_xyz( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 92: /* multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
      case 96: /* multipoint_textzm ::= EWKT_OPEN_BRACKET point_brkt_coordxyzm extra_brkt_pointsxyzm EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==96);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipoint_xyzm( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 101: /* multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multilinestring_xy( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 103: /* multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2 */
      case 106: /* multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2 */ yytestcase(yyruleno==106);
      case 109: /* multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==109);
      case 112: /* multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2 */ yytestcase(yyruleno==112);
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 104: /* multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multilinestring_xym( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 107: /* multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multilinestring_xyz( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 110: /* multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multilinestring_xyzm( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 117: /* multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipolygon_xy( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 119: /* multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2 */
      case 122: /* multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2 */ yytestcase(yyruleno==122);
      case 125: /* multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==125);
      case 128: /* multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2 */ yytestcase(yyruleno==128);
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 120: /* multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipolygon_xym( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 123: /* multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipolygon_xyz( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 126: /* multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) ewkt_multipolygon_xyzm( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 133: /* geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET */
      case 134: /* geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==134);
      case 135: /* geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==135);
      case 136: /* geocoll_text ::= EWKT_OPEN_BRACKET multipoint geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==136);
      case 137: /* geocoll_text ::= EWKT_OPEN_BRACKET multilinestring geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==137);
      case 138: /* geocoll_text ::= EWKT_OPEN_BRACKET multipolygon geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==138);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_geomColl_xy( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 139: /* geocoll_text ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_text geocoll_text2 EWKT_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) ewkt_geomColl_xy( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 141: /* geocoll_text2 ::= EWKT_COMMA point geocoll_text2 */
      case 142: /* geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2 */ yytestcase(yyruleno==142);
      case 143: /* geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2 */ yytestcase(yyruleno==143);
      case 144: /* geocoll_text2 ::= EWKT_COMMA multipoint geocoll_text2 */ yytestcase(yyruleno==144);
      case 145: /* geocoll_text2 ::= EWKT_COMMA multilinestring geocoll_text2 */ yytestcase(yyruleno==145);
      case 146: /* geocoll_text2 ::= EWKT_COMMA multipolygon geocoll_text2 */ yytestcase(yyruleno==146);
      case 156: /* geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2 */ yytestcase(yyruleno==156);
      case 157: /* geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2 */ yytestcase(yyruleno==157);
      case 158: /* geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2 */ yytestcase(yyruleno==158);
      case 159: /* geocoll_textm2 ::= EWKT_COMMA multipointm geocoll_textm2 */ yytestcase(yyruleno==159);
      case 160: /* geocoll_textm2 ::= EWKT_COMMA multilinestringm geocoll_textm2 */ yytestcase(yyruleno==160);
      case 161: /* geocoll_textm2 ::= EWKT_COMMA multipolygonm geocoll_textm2 */ yytestcase(yyruleno==161);
      case 171: /* geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2 */ yytestcase(yyruleno==171);
      case 172: /* geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2 */ yytestcase(yyruleno==172);
      case 173: /* geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2 */ yytestcase(yyruleno==173);
      case 174: /* geocoll_textz2 ::= EWKT_COMMA multipointz geocoll_textz2 */ yytestcase(yyruleno==174);
      case 175: /* geocoll_textz2 ::= EWKT_COMMA multilinestringz geocoll_textz2 */ yytestcase(yyruleno==175);
      case 176: /* geocoll_textz2 ::= EWKT_COMMA multipolygonz geocoll_textz2 */ yytestcase(yyruleno==176);
      case 186: /* geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2 */ yytestcase(yyruleno==186);
      case 187: /* geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2 */ yytestcase(yyruleno==187);
      case 188: /* geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2 */ yytestcase(yyruleno==188);
      case 189: /* geocoll_textzm2 ::= EWKT_COMMA multipointzm geocoll_textzm2 */ yytestcase(yyruleno==189);
      case 190: /* geocoll_textzm2 ::= EWKT_COMMA multilinestringzm geocoll_textzm2 */ yytestcase(yyruleno==190);
      case 191: /* geocoll_textzm2 ::= EWKT_COMMA multipolygonzm geocoll_textzm2 */ yytestcase(yyruleno==191);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 147: /* geocoll_text2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_text geocoll_text2 */
      case 162: /* geocoll_textm2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 */ yytestcase(yyruleno==162);
      case 177: /* geocoll_textz2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_textz geocoll_textz2 */ yytestcase(yyruleno==177);
      case 192: /* geocoll_textzm2 ::= EWKT_COMMA EWKT_GEOMETRYCOLLECTION geocoll_textzm geocoll_textzm2 */ yytestcase(yyruleno==192);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yymsp[-3].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 148: /* geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET */
      case 149: /* geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==149);
      case 150: /* geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==150);
      case 151: /* geocoll_textm ::= EWKT_OPEN_BRACKET multipointm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==151);
      case 152: /* geocoll_textm ::= EWKT_OPEN_BRACKET multilinestringm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==152);
      case 153: /* geocoll_textm ::= EWKT_OPEN_BRACKET multipolygonm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==153);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_geomColl_xym( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 154: /* geocoll_textm ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 EWKT_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) ewkt_geomColl_xym( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 163: /* geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET */
      case 164: /* geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==164);
      case 165: /* geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==165);
      case 166: /* geocoll_textz ::= EWKT_OPEN_BRACKET multipointz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==166);
      case 167: /* geocoll_textz ::= EWKT_OPEN_BRACKET multilinestringz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==167);
      case 168: /* geocoll_textz ::= EWKT_OPEN_BRACKET multipolygonz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==168);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_geomColl_xyz( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 169: /* geocoll_textz ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_textz geocoll_textz2 EWKT_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) ewkt_geomColl_xyz( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 178: /* geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
      case 179: /* geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==179);
      case 180: /* geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==180);
      case 181: /* geocoll_textzm ::= EWKT_OPEN_BRACKET multipointzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==181);
      case 182: /* geocoll_textzm ::= EWKT_OPEN_BRACKET multilinestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==182);
      case 183: /* geocoll_textzm ::= EWKT_OPEN_BRACKET multipolygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==183);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) ewkt_geomColl_xyzm( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 184: /* geocoll_textzm ::= EWKT_OPEN_BRACKET EWKT_GEOMETRYCOLLECTION geocoll_textzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) ewkt_geomColl_xyzm( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      default:
      /* (193) main ::= in */ yytestcase(yyruleno==193);
      /* (194) in ::= */ yytestcase(yyruleno==194);
      /* (195) in ::= in state EWKT_NEWLINE */ yytestcase(yyruleno==195);
      /* (196) state ::= program (OPTIMIZED OUT) */ assert(yyruleno!=196);
      /* (197) program ::= geo_text (OPTIMIZED OUT) */ assert(yyruleno!=197);
      /* (198) program ::= geo_textm (OPTIMIZED OUT) */ assert(yyruleno!=198);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[yysize].stateno,(YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert( !(yyact>YY_MAX_SHIFT && yyact<=YY_MAX_SHIFTREDUCE) );

  /* It is not possible for a REDUCE to be followed by an error */
  assert( yyact!=YY_ERROR_ACTION );

  yymsp += yysize+1;
  yypParser->yytos = yymsp;
  yymsp->stateno = (YYACTIONTYPE)yyact;
  yymsp->major = (YYCODETYPE)yygoto;
  yyTraceShift(yypParser, yyact, "... then shift");
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  ParseTOKENTYPE yyminor         /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/

/* 
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	p_data->ewkt_parse_error = 1;
	p_data->result = NULL;
/************ End %syntax_error code ******************************************/
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  yypParser = (yyParser*)yyp;
  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    int stateno = yypParser->yytos->stateno;
    if( stateno < YY_MIN_REDUCE ){
      fprintf(yyTraceFILE,"%sInput '%s' in state %d\n",
              yyTracePrompt,yyTokenName[yymajor],stateno);
    }else{
      fprintf(yyTraceFILE,"%sInput '%s' with pending reduce %d\n",
              yyTracePrompt,yyTokenName[yymajor],stateno-YY_MIN_REDUCE);
    }
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact >= YY_MIN_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE,yymajor,yyminor);
    }else if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact==YY_ACCEPT_ACTION ){
      yypParser->yytos--;
      yy_accept(yypParser);
      return;
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}
