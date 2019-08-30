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
#define YYNOCODE 133
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  struct vanuatu_data *p_data ;
#define ParseARG_PDECL , struct vanuatu_data *p_data 
#define ParseARG_FETCH  struct vanuatu_data *p_data  = yypParser->p_data 
#define ParseARG_STORE yypParser->p_data  = p_data 
#define YYNSTATE             315
#define YYNRULE              201
#define YYNTOKEN             34
#define YY_MAX_SHIFT         314
#define YY_MIN_SHIFTREDUCE   490
#define YY_MAX_SHIFTREDUCE   690
#define YY_ERROR_ACTION      691
#define YY_ACCEPT_ACTION     692
#define YY_NO_ACTION         693
#define YY_MIN_REDUCE        694
#define YY_MAX_REDUCE        894
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
#define YY_ACTTAB_COUNT (603)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   314,  314,  314,  314,  314,  314,  694,  695,  696,  697,
 /*    10 */   698,  699,  700,  701,  702,  703,  704,  705,  706,  707,
 /*    20 */   708,  709,  710,  711,  712,  713,  714,  715,  716,  717,
 /*    30 */   718,  719,  720,  721,  887,  187,  313,  224,  174,  311,
 /*    40 */   309,  307,  228,  173,  221,  218,  215,  212,  209,  203,
 /*    50 */   197,  191,  185,  178,  170,  162,  153,  148,  143,  138,
 /*    60 */   133,  128,  123,  118,  113,   96,   79,   62,  307,  307,
 /*    70 */   309,  309,  692,    1,  212,  212,  215,  215,  191,  191,
 /*    80 */   197,  197,  162,  162,  170,  170,  138,  138,  143,  143,
 /*    90 */   118,  118,  123,  123,   54,   47,   71,   64,  311,  311,
 /*   100 */   530,  157,  313,  224,  218,  218,  156,  313,  203,  203,
 /*   110 */   221,    2,  178,  178,  209,  221,  148,  148,  185,  209,
 /*   120 */   128,  128,  153,  185,   88,   81,  133,  153,  161,  166,
 /*   130 */   105,  133,  226,  530,  165,   98,   60,   59,   58,   57,
 /*   140 */    56,   55,   61,   52,   51,   50,   49,   48,   77,   76,
 /*   150 */    75,   74,   73,   72,  230,   46,   78,   69,   68,   67,
 /*   160 */    66,   65,  279,    3,  224,   94,   93,   92,   91,   90,
 /*   170 */    89,   95,   86,   85,   84,   83,   82,  231,  232,  111,
 /*   180 */   110,  109,  108,  107,  106,  112,  103,  102,  101,  100,
 /*   190 */    99,  182,  169,  177,  184,  181,  229,  530,  530,  530,
 /*   200 */   280,  281,  224,  224,  193,  284,  285,  226,  226,  226,
 /*   210 */   286,  199,  289,  226,  205,  228,  228,  290,  291,  229,
 /*   220 */   294,  228,  228,  295,  296,  229,  233,  213,  229,  229,
 /*   230 */   226,  210,  211,  224,  224,  299,  214,  224,  301,  226,
 /*   240 */   216,  226,  217,  303,  228,  219,  228,  228,  220,  308,
 /*   250 */   229,  305,  226,  229,  310,  306,  229,  224,  228,  312,
 /*   260 */   234,  235,   53,    4,  229,  886,    5,    6,    7,  885,
 /*   270 */     8,    9,  884,  115,  190,  883,  117,  196,  882,  881,
 /*   280 */   122,  880,  236,  826,  120,  237,  202,  125,   63,  238,
 /*   290 */   127,  130,  208,  132,  239,  114,  240,  241,   32,  242,
 /*   300 */   135,   70,  871,  870,  137,  116,  140,   35,  869,  868,
 /*   310 */   142,  145,   80,  867,  866,  865,  243,   38,  825,  147,
 /*   320 */   139,  129,  244,  245,  246,  247,  248,  249,  856,   87,
 /*   330 */   855,  854,  853,   97,  852,  851,  850,  250,  824,  251,
 /*   340 */   252,  819,  253,  254,  255,  256,  841,  104,  840,  839,
 /*   350 */   838,  822,  837,  836,  835,  257,  823,  258,  810,  259,
 /*   360 */   119,  121,  809,  816,  260,  124,  808,  813,  261,  126,
 /*   370 */   806,  131,  807,  794,  262,  134,  136,  150,  141,   41,
 /*   380 */   152,  144,  146,  803,  263,  161,  149,  155,  151,  742,
 /*   390 */   793,  154,  266,   30,  800,  267,  268,  158,  159,  264,
 /*   400 */   160,   10,  164,  792,  169,  740,  163,  269,  797,   33,
 /*   410 */   782,  265,  270,  791,  271,  167,  168,   11,  172,  781,
 /*   420 */   177,  171,  738,  272,   36,  274,  175,  176,  273,   12,
 /*   430 */   180,  184,  780,  736,  179,  275,   39,  277,  276,  183,
 /*   440 */    13,  779,  188,  278,   17,  194,  283,  775,  778,  186,
 /*   450 */    21,  282,  762,  200,  189,  288,   25,  206,  192,  761,
 /*   460 */    29,  750,  753,  287,  195,  772,  292,  198,  760,  201,
 /*   470 */   769,  293,  298,  759,  748,  204,  300,  297,  674,  207,
 /*   480 */   754,  746,  302,  752,  733,  673,  744,  304,  222,  223,
 /*   490 */   672,  671,  751,  670,  732,  225,  731,  669,  227,  730,
 /*   500 */   668,  659,  658,  657,  656,  655,  654,  653,  644,  643,
 /*   510 */   642,  641,  640,  639,  638,  629,  628,  627,  626,  625,
 /*   520 */   624,  623,  616,  613,  610,  607,  600,  597,  594,  591,
 /*   530 */   586,  582,  525,  585,  581,  524,  584,  580,  523,  583,
 /*   540 */   579,  522,  572,  693,  693,  693,  562,  569,   14,   15,
 /*   550 */    16,  693,  693,  693,  561,  566,   18,   19,   20,  693,
 /*   560 */   693,  693,  560,  563,   22,   23,   24,  693,  693,  693,
 /*   570 */   559,  554,   26,   27,   28,  693,  553,  693,  552,  693,
 /*   580 */    31,  551,   34,  685,   37,  521,   42,  693,   40,  520,
 /*   590 */    43,  693,  693,  519,   44,  693,  693,  693,  518,  693,
 /*   600 */   693,  693,   45,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    37,   38,   39,   40,   41,   42,   43,   44,   45,   46,
 /*    10 */    47,   48,   49,   50,   51,   52,   53,   54,   55,   56,
 /*    20 */    57,   58,   59,   60,   61,   62,   63,   64,   65,   66,
 /*    30 */    67,   68,   69,   70,    0,   74,    2,   76,   72,    5,
 /*    40 */     6,    7,   76,   77,   10,   11,   12,   13,   14,   15,
 /*    50 */    16,   17,   18,   19,   20,   21,   22,   23,   24,   25,
 /*    60 */    26,   27,   28,   29,   30,   31,   32,   33,    7,    7,
 /*    70 */     6,    6,   35,   36,   13,   13,   12,   12,   17,   17,
 /*    80 */    16,   16,   21,   21,   20,   20,   25,   25,   24,   24,
 /*    90 */    29,   29,   28,   28,   33,   33,   32,   32,    5,    5,
 /*   100 */     8,   74,    2,   76,   11,   11,   79,    2,   15,   15,
 /*   110 */    10,    9,   19,   19,   14,   10,   23,   23,   18,   14,
 /*   120 */    27,   27,   22,   18,   31,   31,   26,   22,    3,   73,
 /*   130 */    30,   26,   76,    8,   78,   30,   64,   65,   66,   67,
 /*   140 */    68,   69,   64,   65,   66,   67,   68,   69,   50,   51,
 /*   150 */    52,   53,   54,   55,  131,  127,   50,   51,   52,   53,
 /*   160 */    54,   55,   74,    3,   76,   57,   58,   59,   60,   61,
 /*   170 */    62,   57,   58,   59,   60,   61,   62,  131,  131,   43,
 /*   180 */    44,   45,   46,   47,   48,   43,   44,   45,   46,   47,
 /*   190 */    48,   71,    3,    3,    3,   75,   76,    8,    8,    8,
 /*   200 */    74,   74,   76,   76,   73,   73,   73,   76,   76,   76,
 /*   210 */    73,   72,   72,   76,   71,   76,   76,   72,   72,   76,
 /*   220 */    71,   76,   76,   71,   71,   76,  131,   73,   76,   76,
 /*   230 */    76,   74,   74,   76,   76,   74,   73,   76,   73,   76,
 /*   240 */    72,   76,   72,   72,   76,   71,   76,   76,   71,   73,
 /*   250 */    76,   71,   76,   76,   72,   74,   76,   76,   76,   71,
 /*   260 */   131,  131,  127,    9,   76,  131,    3,    9,    3,  131,
 /*   270 */     9,    3,  131,    9,    3,  131,    3,    3,  131,  131,
 /*   280 */     3,  131,  131,  127,    9,  130,    3,    9,  126,  130,
 /*   290 */     3,    9,    3,    3,  130,   95,  130,  130,    3,  130,
 /*   300 */     9,  126,  130,  130,    3,   95,    9,    3,  130,  130,
 /*   310 */     3,    9,  125,  130,  130,  130,  130,    3,  126,    3,
 /*   320 */    90,   92,  129,  129,  129,  129,  129,  129,  129,  125,
 /*   330 */   129,  129,  129,  124,  129,  129,  129,  129,  125,  128,
 /*   340 */   128,  122,  128,  128,  128,  128,  128,  124,  128,  128,
 /*   350 */   128,  123,  128,  128,  128,  128,  124,  123,  119,  122,
 /*   360 */    94,   94,  118,  121,  121,   93,  117,  120,  120,   93,
 /*   370 */   115,   92,  116,  111,  115,   91,   91,    9,   90,    3,
 /*   380 */     3,   89,   89,  114,  114,    3,   88,    9,   88,   83,
 /*   390 */   110,   79,   83,    9,  113,   87,   76,   76,   76,  113,
 /*   400 */    76,    3,    9,  109,    3,   82,   78,   82,  112,    9,
 /*   410 */   107,  112,   86,  108,   76,   76,   76,    3,    9,  106,
 /*   420 */     3,   77,   81,   81,    9,   76,   76,   76,   85,    3,
 /*   430 */     9,    3,  105,   80,   75,   80,    9,   76,   84,   76,
 /*   440 */     3,  104,    9,   87,    3,    9,   86,  101,  103,  102,
 /*   450 */     3,  103,   95,    9,  102,   85,    3,    9,  100,   94,
 /*   460 */     3,   87,   90,  101,  100,   99,   99,   98,   93,   98,
 /*   470 */    97,   84,   87,   92,   86,   96,   86,   97,    4,   96,
 /*   480 */    91,   85,   85,   89,   76,    4,   84,   84,   76,   76,
 /*   490 */     4,    4,   88,    4,   76,   76,   76,    4,   76,   76,
 /*   500 */     4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   510 */     4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   520 */     4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   530 */     4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   540 */     4,    4,    4,  132,  132,  132,    4,    4,    9,    9,
 /*   550 */     9,  132,  132,  132,    4,    4,    9,    9,    9,  132,
 /*   560 */   132,  132,    4,    4,    9,    9,    9,  132,  132,  132,
 /*   570 */     4,    4,    9,    9,    9,  132,    4,  132,    4,  132,
 /*   580 */     9,    4,    9,    1,    9,    4,    3,  132,    9,    4,
 /*   590 */     3,  132,  132,    4,    3,  132,  132,  132,    4,  132,
 /*   600 */   132,  132,    3,  132,  132,  132,  132,  132,  132,  132,
 /*   610 */   132,  132,  132,  132,  132,  132,  132,  132,  132,  132,
 /*   620 */   132,  132,  132,  132,  132,  132,  132,  132,  132,  132,
 /*   630 */   132,  132,  132,  132,  132,  132,  132,
};
#define YY_SHIFT_COUNT    (314)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (599)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */   603,   34,   61,   62,   64,   65,   93,   94,  100,  105,
 /*    10 */   125,  189,  190,  191,   92,   92,   92,   92,   92,   92,
 /*    20 */    92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
 /*    30 */    92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
 /*    40 */    92,   92,   92,   92,   92,   92,  102,  160,  102,  102,
 /*    50 */   102,  102,  102,  102,  160,  102,  102,  102,  102,  102,
 /*    60 */   102,  102,  160,  254,  263,  254,  254,  254,  254,  254,
 /*    70 */   254,  263,  254,  254,  254,  254,  254,  254,  254,  263,
 /*    80 */   258,  265,  258,  258,  258,  258,  258,  258,  265,  258,
 /*    90 */   258,  258,  258,  258,  258,  258,  265,  261,  268,  261,
 /*   100 */   261,  261,  261,  261,  261,  268,  261,  261,  261,  261,
 /*   110 */   261,  261,  261,  268,  264,  271,  264,  271,  273,  275,
 /*   120 */   274,  275,  274,  277,  278,  283,  278,  283,  287,  282,
 /*   130 */   289,  282,  289,  290,  291,  295,  291,  295,  301,  297,
 /*   140 */   304,  297,  304,  307,  302,  314,  302,  314,  316,  368,
 /*   150 */   376,  368,  376,  377,  378,  382,  378,  384,   92,   92,
 /*   160 */    92,   92,  398,  393,  401,  393,  400,   92,   92,   92,
 /*   170 */   414,  409,  417,  409,  415,   92,   92,   92,  426,  421,
 /*   180 */   428,  421,  427,   92,   92,  437,  433,  384,  441,  433,
 /*   190 */   441,  271,  436,  400,  447,  436,  447,  274,  444,  415,
 /*   200 */   453,  444,  453,  283,  448,  427,  457,  448,  457,  289,
 /*   210 */   384,  384,  295,  400,  400,  304,  415,  415,  314,  427,
 /*   220 */   427,  376,   92,   92,   92,   92,   92,   92,   92,   92,
 /*   230 */   474,  481,  486,  487,  489,  493,  496,  497,  498,  499,
 /*   240 */   500,  501,  502,  503,  504,  505,  506,  507,  508,  509,
 /*   250 */   510,  511,  512,  513,  514,  515,  516,  517,  518,  519,
 /*   260 */   520,  521,  522,  523,  524,  525,  526,  527,  528,  529,
 /*   270 */   530,  531,  532,  533,  534,  535,  536,  537,  538,  539,
 /*   280 */   540,  541,  542,  543,  547,  548,  549,  550,  551,  555,
 /*   290 */   556,  557,  558,  559,  563,  564,  565,  566,  567,  571,
 /*   300 */   572,  573,  574,  575,  577,  579,  581,  583,  585,  587,
 /*   310 */   589,  591,  594,  599,  582,
};
#define YY_REDUCE_COUNT (229)
#define YY_REDUCE_MIN   (-39)
#define YY_REDUCE_MAX   (423)
static const short yy_reduce_ofst[] = {
 /*     0 */    37,  -37,   72,   78,   98,  106,  108,  114,  136,  142,
 /*    10 */    27,   56,  -34,  120,  -39,   88,  126,  127,  131,  132,
 /*    20 */   133,  137,  139,  140,  145,  146,  143,  149,  152,  153,
 /*    30 */   157,  158,  161,  154,  163,  165,  168,  170,  171,  174,
 /*    40 */   177,  180,  181,  176,  182,  188,   23,   28,   46,   47,
 /*    50 */    95,  129,  130,  134,  135,  138,  141,  144,  147,  148,
 /*    60 */   150,  151,  156,  155,  162,  159,  164,  166,  167,  169,
 /*    70 */   172,  175,  173,  178,  179,  183,  184,  185,  186,  192,
 /*    80 */   193,  187,  194,  195,  196,  197,  198,  199,  204,  201,
 /*    90 */   202,  203,  205,  206,  207,  208,  213,  211,  209,  212,
 /*   100 */   214,  215,  216,  217,  218,  223,  220,  221,  222,  224,
 /*   110 */   225,  226,  227,  232,  228,  200,  234,  210,  239,  219,
 /*   120 */   266,  237,  267,  244,  242,  272,  243,  276,  249,  247,
 /*   130 */   229,  248,  279,  256,  255,  284,  259,  285,  262,  269,
 /*   140 */   230,  270,  288,  280,  281,  292,  286,  293,  294,  296,
 /*   150 */   298,  299,  300,  305,  306,  312,  309,  308,  320,  321,
 /*   160 */   322,  324,  303,  323,  328,  325,  326,  338,  339,  340,
 /*   170 */   313,  341,  344,  342,  343,  349,  350,  351,  327,  353,
 /*   180 */   359,  355,  354,  361,  363,  337,  345,  356,  347,  348,
 /*   190 */   352,  357,  346,  360,  358,  362,  364,  365,  366,  370,
 /*   200 */   369,  367,  371,  375,  373,  387,  379,  380,  383,  381,
 /*   210 */   374,  385,  389,  388,  390,  372,  396,  397,  394,  402,
 /*   220 */   403,  404,  408,  412,  413,  418,  419,  420,  422,  423,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   888,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*    10 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*    20 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*    30 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*    40 */   691,  691,  691,  691,  691,  691,  879,  691,  879,  879,
 /*    50 */   879,  879,  879,  879,  691,  879,  879,  879,  879,  879,
 /*    60 */   879,  879,  691,  864,  691,  864,  864,  864,  864,  864,
 /*    70 */   864,  691,  864,  864,  864,  864,  864,  864,  864,  691,
 /*    80 */   849,  691,  849,  849,  849,  849,  849,  849,  691,  849,
 /*    90 */   849,  849,  849,  849,  849,  849,  691,  834,  691,  834,
 /*   100 */   834,  834,  834,  834,  834,  691,  834,  834,  834,  834,
 /*   110 */   834,  834,  834,  691,  821,  691,  821,  691,  691,  818,
 /*   120 */   691,  818,  691,  691,  815,  691,  815,  691,  691,  812,
 /*   130 */   691,  812,  691,  691,  805,  691,  805,  691,  691,  802,
 /*   140 */   691,  802,  691,  691,  799,  691,  799,  691,  691,  796,
 /*   150 */   691,  796,  691,  691,  741,  691,  741,  749,  691,  691,
 /*   160 */   691,  691,  691,  739,  691,  739,  747,  691,  691,  691,
 /*   170 */   691,  737,  691,  737,  745,  691,  691,  691,  691,  735,
 /*   180 */   691,  735,  743,  691,  691,  691,  777,  749,  691,  777,
 /*   190 */   691,  691,  774,  747,  691,  774,  691,  691,  771,  745,
 /*   200 */   691,  771,  691,  691,  768,  743,  691,  768,  691,  691,
 /*   210 */   749,  749,  691,  747,  747,  691,  745,  745,  691,  743,
 /*   220 */   743,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   230 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   240 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   250 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   260 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   270 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   280 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   290 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   300 */   691,  691,  691,  691,  691,  691,  691,  691,  691,  691,
 /*   310 */   691,  691,  691,  691,  691,
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
  /*    1 */ "VANUATU_NEWLINE",
  /*    2 */ "VANUATU_POINT",
  /*    3 */ "VANUATU_OPEN_BRACKET",
  /*    4 */ "VANUATU_CLOSE_BRACKET",
  /*    5 */ "VANUATU_POINT_M",
  /*    6 */ "VANUATU_POINT_Z",
  /*    7 */ "VANUATU_POINT_ZM",
  /*    8 */ "VANUATU_NUM",
  /*    9 */ "VANUATU_COMMA",
  /*   10 */ "VANUATU_LINESTRING",
  /*   11 */ "VANUATU_LINESTRING_M",
  /*   12 */ "VANUATU_LINESTRING_Z",
  /*   13 */ "VANUATU_LINESTRING_ZM",
  /*   14 */ "VANUATU_POLYGON",
  /*   15 */ "VANUATU_POLYGON_M",
  /*   16 */ "VANUATU_POLYGON_Z",
  /*   17 */ "VANUATU_POLYGON_ZM",
  /*   18 */ "VANUATU_MULTIPOINT",
  /*   19 */ "VANUATU_MULTIPOINT_M",
  /*   20 */ "VANUATU_MULTIPOINT_Z",
  /*   21 */ "VANUATU_MULTIPOINT_ZM",
  /*   22 */ "VANUATU_MULTILINESTRING",
  /*   23 */ "VANUATU_MULTILINESTRING_M",
  /*   24 */ "VANUATU_MULTILINESTRING_Z",
  /*   25 */ "VANUATU_MULTILINESTRING_ZM",
  /*   26 */ "VANUATU_MULTIPOLYGON",
  /*   27 */ "VANUATU_MULTIPOLYGON_M",
  /*   28 */ "VANUATU_MULTIPOLYGON_Z",
  /*   29 */ "VANUATU_MULTIPOLYGON_ZM",
  /*   30 */ "VANUATU_GEOMETRYCOLLECTION",
  /*   31 */ "VANUATU_GEOMETRYCOLLECTION_M",
  /*   32 */ "VANUATU_GEOMETRYCOLLECTION_Z",
  /*   33 */ "VANUATU_GEOMETRYCOLLECTION_ZM",
  /*   34 */ "error",
  /*   35 */ "main",
  /*   36 */ "in",
  /*   37 */ "state",
  /*   38 */ "program",
  /*   39 */ "geo_text",
  /*   40 */ "geo_textz",
  /*   41 */ "geo_textm",
  /*   42 */ "geo_textzm",
  /*   43 */ "point",
  /*   44 */ "linestring",
  /*   45 */ "polygon",
  /*   46 */ "multipoint",
  /*   47 */ "multilinestring",
  /*   48 */ "multipolygon",
  /*   49 */ "geocoll",
  /*   50 */ "pointz",
  /*   51 */ "linestringz",
  /*   52 */ "polygonz",
  /*   53 */ "multipointz",
  /*   54 */ "multilinestringz",
  /*   55 */ "multipolygonz",
  /*   56 */ "geocollz",
  /*   57 */ "pointm",
  /*   58 */ "linestringm",
  /*   59 */ "polygonm",
  /*   60 */ "multipointm",
  /*   61 */ "multilinestringm",
  /*   62 */ "multipolygonm",
  /*   63 */ "geocollm",
  /*   64 */ "pointzm",
  /*   65 */ "linestringzm",
  /*   66 */ "polygonzm",
  /*   67 */ "multipointzm",
  /*   68 */ "multilinestringzm",
  /*   69 */ "multipolygonzm",
  /*   70 */ "geocollzm",
  /*   71 */ "point_coordxy",
  /*   72 */ "point_coordxym",
  /*   73 */ "point_coordxyz",
  /*   74 */ "point_coordxyzm",
  /*   75 */ "point_brkt_coordxy",
  /*   76 */ "coord",
  /*   77 */ "point_brkt_coordxym",
  /*   78 */ "point_brkt_coordxyz",
  /*   79 */ "point_brkt_coordxyzm",
  /*   80 */ "extra_brkt_pointsxy",
  /*   81 */ "extra_brkt_pointsxym",
  /*   82 */ "extra_brkt_pointsxyz",
  /*   83 */ "extra_brkt_pointsxyzm",
  /*   84 */ "extra_pointsxy",
  /*   85 */ "extra_pointsxym",
  /*   86 */ "extra_pointsxyz",
  /*   87 */ "extra_pointsxyzm",
  /*   88 */ "linestring_text",
  /*   89 */ "linestring_textm",
  /*   90 */ "linestring_textz",
  /*   91 */ "linestring_textzm",
  /*   92 */ "polygon_text",
  /*   93 */ "polygon_textm",
  /*   94 */ "polygon_textz",
  /*   95 */ "polygon_textzm",
  /*   96 */ "ring",
  /*   97 */ "extra_rings",
  /*   98 */ "ringm",
  /*   99 */ "extra_ringsm",
  /*  100 */ "ringz",
  /*  101 */ "extra_ringsz",
  /*  102 */ "ringzm",
  /*  103 */ "extra_ringszm",
  /*  104 */ "multipoint_text",
  /*  105 */ "multipoint_textm",
  /*  106 */ "multipoint_textz",
  /*  107 */ "multipoint_textzm",
  /*  108 */ "multilinestring_text",
  /*  109 */ "multilinestring_textm",
  /*  110 */ "multilinestring_textz",
  /*  111 */ "multilinestring_textzm",
  /*  112 */ "multilinestring_text2",
  /*  113 */ "multilinestring_textm2",
  /*  114 */ "multilinestring_textz2",
  /*  115 */ "multilinestring_textzm2",
  /*  116 */ "multipolygon_text",
  /*  117 */ "multipolygon_textm",
  /*  118 */ "multipolygon_textz",
  /*  119 */ "multipolygon_textzm",
  /*  120 */ "multipolygon_text2",
  /*  121 */ "multipolygon_textm2",
  /*  122 */ "multipolygon_textz2",
  /*  123 */ "multipolygon_textzm2",
  /*  124 */ "geocoll_text",
  /*  125 */ "geocoll_textm",
  /*  126 */ "geocoll_textz",
  /*  127 */ "geocoll_textzm",
  /*  128 */ "geocoll_text2",
  /*  129 */ "geocoll_textm2",
  /*  130 */ "geocoll_textz2",
  /*  131 */ "geocoll_textzm2",
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "geo_text ::= point",
 /*   1 */ "geo_text ::= linestring",
 /*   2 */ "geo_text ::= polygon",
 /*   3 */ "geo_text ::= multipoint",
 /*   4 */ "geo_text ::= multilinestring",
 /*   5 */ "geo_text ::= multipolygon",
 /*   6 */ "geo_text ::= geocoll",
 /*   7 */ "geo_textz ::= pointz",
 /*   8 */ "geo_textz ::= linestringz",
 /*   9 */ "geo_textz ::= polygonz",
 /*  10 */ "geo_textz ::= multipointz",
 /*  11 */ "geo_textz ::= multilinestringz",
 /*  12 */ "geo_textz ::= multipolygonz",
 /*  13 */ "geo_textz ::= geocollz",
 /*  14 */ "geo_textm ::= pointm",
 /*  15 */ "geo_textm ::= linestringm",
 /*  16 */ "geo_textm ::= polygonm",
 /*  17 */ "geo_textm ::= multipointm",
 /*  18 */ "geo_textm ::= multilinestringm",
 /*  19 */ "geo_textm ::= multipolygonm",
 /*  20 */ "geo_textm ::= geocollm",
 /*  21 */ "geo_textzm ::= pointzm",
 /*  22 */ "geo_textzm ::= linestringzm",
 /*  23 */ "geo_textzm ::= polygonzm",
 /*  24 */ "geo_textzm ::= multipointzm",
 /*  25 */ "geo_textzm ::= multilinestringzm",
 /*  26 */ "geo_textzm ::= multipolygonzm",
 /*  27 */ "geo_textzm ::= geocollzm",
 /*  28 */ "point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET",
 /*  29 */ "pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET",
 /*  30 */ "pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET",
 /*  31 */ "pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET",
 /*  32 */ "point_brkt_coordxy ::= VANUATU_OPEN_BRACKET coord coord VANUATU_CLOSE_BRACKET",
 /*  33 */ "point_brkt_coordxym ::= VANUATU_OPEN_BRACKET coord coord coord VANUATU_CLOSE_BRACKET",
 /*  34 */ "point_brkt_coordxyz ::= VANUATU_OPEN_BRACKET coord coord coord VANUATU_CLOSE_BRACKET",
 /*  35 */ "point_brkt_coordxyzm ::= VANUATU_OPEN_BRACKET coord coord coord coord VANUATU_CLOSE_BRACKET",
 /*  36 */ "point_coordxy ::= coord coord",
 /*  37 */ "point_coordxym ::= coord coord coord",
 /*  38 */ "point_coordxyz ::= coord coord coord",
 /*  39 */ "point_coordxyzm ::= coord coord coord coord",
 /*  40 */ "coord ::= VANUATU_NUM",
 /*  41 */ "extra_brkt_pointsxy ::=",
 /*  42 */ "extra_brkt_pointsxy ::= VANUATU_COMMA point_brkt_coordxy extra_brkt_pointsxy",
 /*  43 */ "extra_brkt_pointsxym ::=",
 /*  44 */ "extra_brkt_pointsxym ::= VANUATU_COMMA point_brkt_coordxym extra_brkt_pointsxym",
 /*  45 */ "extra_brkt_pointsxyz ::=",
 /*  46 */ "extra_brkt_pointsxyz ::= VANUATU_COMMA point_brkt_coordxyz extra_brkt_pointsxyz",
 /*  47 */ "extra_brkt_pointsxyzm ::=",
 /*  48 */ "extra_brkt_pointsxyzm ::= VANUATU_COMMA point_brkt_coordxyzm extra_brkt_pointsxyzm",
 /*  49 */ "extra_pointsxy ::=",
 /*  50 */ "extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy",
 /*  51 */ "extra_pointsxym ::=",
 /*  52 */ "extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym",
 /*  53 */ "extra_pointsxyz ::=",
 /*  54 */ "extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz",
 /*  55 */ "extra_pointsxyzm ::=",
 /*  56 */ "extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm",
 /*  57 */ "linestring ::= VANUATU_LINESTRING linestring_text",
 /*  58 */ "linestringm ::= VANUATU_LINESTRING_M linestring_textm",
 /*  59 */ "linestringz ::= VANUATU_LINESTRING_Z linestring_textz",
 /*  60 */ "linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm",
 /*  61 */ "linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
 /*  62 */ "linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
 /*  63 */ "linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  64 */ "linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  65 */ "polygon ::= VANUATU_POLYGON polygon_text",
 /*  66 */ "polygonm ::= VANUATU_POLYGON_M polygon_textm",
 /*  67 */ "polygonz ::= VANUATU_POLYGON_Z polygon_textz",
 /*  68 */ "polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm",
 /*  69 */ "polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET",
 /*  70 */ "polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET",
 /*  71 */ "polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET",
 /*  72 */ "polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET",
 /*  73 */ "ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
 /*  74 */ "extra_rings ::=",
 /*  75 */ "extra_rings ::= VANUATU_COMMA ring extra_rings",
 /*  76 */ "ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
 /*  77 */ "extra_ringsm ::=",
 /*  78 */ "extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm",
 /*  79 */ "ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  80 */ "extra_ringsz ::=",
 /*  81 */ "extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz",
 /*  82 */ "ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  83 */ "extra_ringszm ::=",
 /*  84 */ "extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm",
 /*  85 */ "multipoint ::= VANUATU_MULTIPOINT multipoint_text",
 /*  86 */ "multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm",
 /*  87 */ "multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz",
 /*  88 */ "multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm",
 /*  89 */ "multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
 /*  90 */ "multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
 /*  91 */ "multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  92 */ "multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  93 */ "multipoint_text ::= VANUATU_OPEN_BRACKET point_brkt_coordxy extra_brkt_pointsxy VANUATU_CLOSE_BRACKET",
 /*  94 */ "multipoint_textm ::= VANUATU_OPEN_BRACKET point_brkt_coordxym extra_brkt_pointsxym VANUATU_CLOSE_BRACKET",
 /*  95 */ "multipoint_textz ::= VANUATU_OPEN_BRACKET point_brkt_coordxyz extra_brkt_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  96 */ "multipoint_textzm ::= VANUATU_OPEN_BRACKET point_brkt_coordxyzm extra_brkt_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  97 */ "multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text",
 /*  98 */ "multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm",
 /*  99 */ "multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz",
 /* 100 */ "multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm",
 /* 101 */ "multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET",
 /* 102 */ "multilinestring_text2 ::=",
 /* 103 */ "multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2",
 /* 104 */ "multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET",
 /* 105 */ "multilinestring_textm2 ::=",
 /* 106 */ "multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2",
 /* 107 */ "multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET",
 /* 108 */ "multilinestring_textz2 ::=",
 /* 109 */ "multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2",
 /* 110 */ "multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET",
 /* 111 */ "multilinestring_textzm2 ::=",
 /* 112 */ "multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2",
 /* 113 */ "multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text",
 /* 114 */ "multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm",
 /* 115 */ "multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz",
 /* 116 */ "multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm",
 /* 117 */ "multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET",
 /* 118 */ "multipolygon_text2 ::=",
 /* 119 */ "multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2",
 /* 120 */ "multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET",
 /* 121 */ "multipolygon_textm2 ::=",
 /* 122 */ "multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2",
 /* 123 */ "multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET",
 /* 124 */ "multipolygon_textz2 ::=",
 /* 125 */ "multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2",
 /* 126 */ "multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET",
 /* 127 */ "multipolygon_textzm2 ::=",
 /* 128 */ "multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2",
 /* 129 */ "geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text",
 /* 130 */ "geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm",
 /* 131 */ "geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz",
 /* 132 */ "geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm",
 /* 133 */ "geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 134 */ "geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 135 */ "geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 136 */ "geocoll_text ::= VANUATU_OPEN_BRACKET multipoint geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 137 */ "geocoll_text ::= VANUATU_OPEN_BRACKET multilinestring geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 138 */ "geocoll_text ::= VANUATU_OPEN_BRACKET multipolygon geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 139 */ "geocoll_text ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION geocoll_text geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 140 */ "geocoll_text2 ::=",
 /* 141 */ "geocoll_text2 ::= VANUATU_COMMA point geocoll_text2",
 /* 142 */ "geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2",
 /* 143 */ "geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2",
 /* 144 */ "geocoll_text2 ::= VANUATU_COMMA multipoint geocoll_text2",
 /* 145 */ "geocoll_text2 ::= VANUATU_COMMA multilinestring geocoll_text2",
 /* 146 */ "geocoll_text2 ::= VANUATU_COMMA multipolygon geocoll_text2",
 /* 147 */ "geocoll_text2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION geocoll_text geocoll_text2",
 /* 148 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 149 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 150 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 151 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET multipointm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 152 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET multilinestringm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 153 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET multipolygonm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 154 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 155 */ "geocoll_textm2 ::=",
 /* 156 */ "geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2",
 /* 157 */ "geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2",
 /* 158 */ "geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2",
 /* 159 */ "geocoll_textm2 ::= VANUATU_COMMA multipointm geocoll_textm2",
 /* 160 */ "geocoll_textm2 ::= VANUATU_COMMA multilinestringm geocoll_textm2",
 /* 161 */ "geocoll_textm2 ::= VANUATU_COMMA multipolygonm geocoll_textm2",
 /* 162 */ "geocoll_textm2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2",
 /* 163 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 164 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 165 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 166 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET multipointz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 167 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET multilinestringz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 168 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET multipolygonz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 169 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 170 */ "geocoll_textz2 ::=",
 /* 171 */ "geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2",
 /* 172 */ "geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2",
 /* 173 */ "geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2",
 /* 174 */ "geocoll_textz2 ::= VANUATU_COMMA multipointz geocoll_textz2",
 /* 175 */ "geocoll_textz2 ::= VANUATU_COMMA multilinestringz geocoll_textz2",
 /* 176 */ "geocoll_textz2 ::= VANUATU_COMMA multipolygonz geocoll_textz2",
 /* 177 */ "geocoll_textz2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz geocoll_textz2",
 /* 178 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 179 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 180 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 181 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET multipointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 182 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET multilinestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 183 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET multipolygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 184 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 185 */ "geocoll_textzm2 ::=",
 /* 186 */ "geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2",
 /* 187 */ "geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2",
 /* 188 */ "geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2",
 /* 189 */ "geocoll_textzm2 ::= VANUATU_COMMA multipointzm geocoll_textzm2",
 /* 190 */ "geocoll_textzm2 ::= VANUATU_COMMA multilinestringzm geocoll_textzm2",
 /* 191 */ "geocoll_textzm2 ::= VANUATU_COMMA multipolygonzm geocoll_textzm2",
 /* 192 */ "geocoll_textzm2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm geocoll_textzm2",
 /* 193 */ "main ::= in",
 /* 194 */ "in ::=",
 /* 195 */ "in ::= in state VANUATU_NEWLINE",
 /* 196 */ "state ::= program",
 /* 197 */ "program ::= geo_text",
 /* 198 */ "program ::= geo_textz",
 /* 199 */ "program ::= geo_textm",
 /* 200 */ "program ::= geo_textzm",
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
  {   39,   -1 }, /* (0) geo_text ::= point */
  {   39,   -1 }, /* (1) geo_text ::= linestring */
  {   39,   -1 }, /* (2) geo_text ::= polygon */
  {   39,   -1 }, /* (3) geo_text ::= multipoint */
  {   39,   -1 }, /* (4) geo_text ::= multilinestring */
  {   39,   -1 }, /* (5) geo_text ::= multipolygon */
  {   39,   -1 }, /* (6) geo_text ::= geocoll */
  {   40,   -1 }, /* (7) geo_textz ::= pointz */
  {   40,   -1 }, /* (8) geo_textz ::= linestringz */
  {   40,   -1 }, /* (9) geo_textz ::= polygonz */
  {   40,   -1 }, /* (10) geo_textz ::= multipointz */
  {   40,   -1 }, /* (11) geo_textz ::= multilinestringz */
  {   40,   -1 }, /* (12) geo_textz ::= multipolygonz */
  {   40,   -1 }, /* (13) geo_textz ::= geocollz */
  {   41,   -1 }, /* (14) geo_textm ::= pointm */
  {   41,   -1 }, /* (15) geo_textm ::= linestringm */
  {   41,   -1 }, /* (16) geo_textm ::= polygonm */
  {   41,   -1 }, /* (17) geo_textm ::= multipointm */
  {   41,   -1 }, /* (18) geo_textm ::= multilinestringm */
  {   41,   -1 }, /* (19) geo_textm ::= multipolygonm */
  {   41,   -1 }, /* (20) geo_textm ::= geocollm */
  {   42,   -1 }, /* (21) geo_textzm ::= pointzm */
  {   42,   -1 }, /* (22) geo_textzm ::= linestringzm */
  {   42,   -1 }, /* (23) geo_textzm ::= polygonzm */
  {   42,   -1 }, /* (24) geo_textzm ::= multipointzm */
  {   42,   -1 }, /* (25) geo_textzm ::= multilinestringzm */
  {   42,   -1 }, /* (26) geo_textzm ::= multipolygonzm */
  {   42,   -1 }, /* (27) geo_textzm ::= geocollzm */
  {   43,   -4 }, /* (28) point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET */
  {   57,   -4 }, /* (29) pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET */
  {   50,   -4 }, /* (30) pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET */
  {   64,   -4 }, /* (31) pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET */
  {   75,   -4 }, /* (32) point_brkt_coordxy ::= VANUATU_OPEN_BRACKET coord coord VANUATU_CLOSE_BRACKET */
  {   77,   -5 }, /* (33) point_brkt_coordxym ::= VANUATU_OPEN_BRACKET coord coord coord VANUATU_CLOSE_BRACKET */
  {   78,   -5 }, /* (34) point_brkt_coordxyz ::= VANUATU_OPEN_BRACKET coord coord coord VANUATU_CLOSE_BRACKET */
  {   79,   -6 }, /* (35) point_brkt_coordxyzm ::= VANUATU_OPEN_BRACKET coord coord coord coord VANUATU_CLOSE_BRACKET */
  {   71,   -2 }, /* (36) point_coordxy ::= coord coord */
  {   72,   -3 }, /* (37) point_coordxym ::= coord coord coord */
  {   73,   -3 }, /* (38) point_coordxyz ::= coord coord coord */
  {   74,   -4 }, /* (39) point_coordxyzm ::= coord coord coord coord */
  {   76,   -1 }, /* (40) coord ::= VANUATU_NUM */
  {   80,    0 }, /* (41) extra_brkt_pointsxy ::= */
  {   80,   -3 }, /* (42) extra_brkt_pointsxy ::= VANUATU_COMMA point_brkt_coordxy extra_brkt_pointsxy */
  {   81,    0 }, /* (43) extra_brkt_pointsxym ::= */
  {   81,   -3 }, /* (44) extra_brkt_pointsxym ::= VANUATU_COMMA point_brkt_coordxym extra_brkt_pointsxym */
  {   82,    0 }, /* (45) extra_brkt_pointsxyz ::= */
  {   82,   -3 }, /* (46) extra_brkt_pointsxyz ::= VANUATU_COMMA point_brkt_coordxyz extra_brkt_pointsxyz */
  {   83,    0 }, /* (47) extra_brkt_pointsxyzm ::= */
  {   83,   -3 }, /* (48) extra_brkt_pointsxyzm ::= VANUATU_COMMA point_brkt_coordxyzm extra_brkt_pointsxyzm */
  {   84,    0 }, /* (49) extra_pointsxy ::= */
  {   84,   -3 }, /* (50) extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy */
  {   85,    0 }, /* (51) extra_pointsxym ::= */
  {   85,   -3 }, /* (52) extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym */
  {   86,    0 }, /* (53) extra_pointsxyz ::= */
  {   86,   -3 }, /* (54) extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz */
  {   87,    0 }, /* (55) extra_pointsxyzm ::= */
  {   87,   -3 }, /* (56) extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm */
  {   44,   -2 }, /* (57) linestring ::= VANUATU_LINESTRING linestring_text */
  {   58,   -2 }, /* (58) linestringm ::= VANUATU_LINESTRING_M linestring_textm */
  {   51,   -2 }, /* (59) linestringz ::= VANUATU_LINESTRING_Z linestring_textz */
  {   65,   -2 }, /* (60) linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm */
  {   88,   -6 }, /* (61) linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
  {   89,   -6 }, /* (62) linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
  {   90,   -6 }, /* (63) linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
  {   91,   -6 }, /* (64) linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
  {   45,   -2 }, /* (65) polygon ::= VANUATU_POLYGON polygon_text */
  {   59,   -2 }, /* (66) polygonm ::= VANUATU_POLYGON_M polygon_textm */
  {   52,   -2 }, /* (67) polygonz ::= VANUATU_POLYGON_Z polygon_textz */
  {   66,   -2 }, /* (68) polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm */
  {   92,   -4 }, /* (69) polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET */
  {   93,   -4 }, /* (70) polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET */
  {   94,   -4 }, /* (71) polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET */
  {   95,   -4 }, /* (72) polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET */
  {   96,  -10 }, /* (73) ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
  {   97,    0 }, /* (74) extra_rings ::= */
  {   97,   -3 }, /* (75) extra_rings ::= VANUATU_COMMA ring extra_rings */
  {   98,  -10 }, /* (76) ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
  {   99,    0 }, /* (77) extra_ringsm ::= */
  {   99,   -3 }, /* (78) extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm */
  {  100,  -10 }, /* (79) ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
  {  101,    0 }, /* (80) extra_ringsz ::= */
  {  101,   -3 }, /* (81) extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz */
  {  102,  -10 }, /* (82) ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
  {  103,    0 }, /* (83) extra_ringszm ::= */
  {  103,   -3 }, /* (84) extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm */
  {   46,   -2 }, /* (85) multipoint ::= VANUATU_MULTIPOINT multipoint_text */
  {   60,   -2 }, /* (86) multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm */
  {   53,   -2 }, /* (87) multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz */
  {   67,   -2 }, /* (88) multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm */
  {  104,   -4 }, /* (89) multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
  {  105,   -4 }, /* (90) multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
  {  106,   -4 }, /* (91) multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
  {  107,   -4 }, /* (92) multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
  {  104,   -4 }, /* (93) multipoint_text ::= VANUATU_OPEN_BRACKET point_brkt_coordxy extra_brkt_pointsxy VANUATU_CLOSE_BRACKET */
  {  105,   -4 }, /* (94) multipoint_textm ::= VANUATU_OPEN_BRACKET point_brkt_coordxym extra_brkt_pointsxym VANUATU_CLOSE_BRACKET */
  {  106,   -4 }, /* (95) multipoint_textz ::= VANUATU_OPEN_BRACKET point_brkt_coordxyz extra_brkt_pointsxyz VANUATU_CLOSE_BRACKET */
  {  107,   -4 }, /* (96) multipoint_textzm ::= VANUATU_OPEN_BRACKET point_brkt_coordxyzm extra_brkt_pointsxyzm VANUATU_CLOSE_BRACKET */
  {   47,   -2 }, /* (97) multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text */
  {   61,   -2 }, /* (98) multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm */
  {   54,   -2 }, /* (99) multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz */
  {   68,   -2 }, /* (100) multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm */
  {  108,   -4 }, /* (101) multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET */
  {  112,    0 }, /* (102) multilinestring_text2 ::= */
  {  112,   -3 }, /* (103) multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2 */
  {  109,   -4 }, /* (104) multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET */
  {  113,    0 }, /* (105) multilinestring_textm2 ::= */
  {  113,   -3 }, /* (106) multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2 */
  {  110,   -4 }, /* (107) multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET */
  {  114,    0 }, /* (108) multilinestring_textz2 ::= */
  {  114,   -3 }, /* (109) multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2 */
  {  111,   -4 }, /* (110) multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET */
  {  115,    0 }, /* (111) multilinestring_textzm2 ::= */
  {  115,   -3 }, /* (112) multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2 */
  {   48,   -2 }, /* (113) multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text */
  {   62,   -2 }, /* (114) multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm */
  {   55,   -2 }, /* (115) multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz */
  {   69,   -2 }, /* (116) multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm */
  {  116,   -4 }, /* (117) multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET */
  {  120,    0 }, /* (118) multipolygon_text2 ::= */
  {  120,   -3 }, /* (119) multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2 */
  {  117,   -4 }, /* (120) multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET */
  {  121,    0 }, /* (121) multipolygon_textm2 ::= */
  {  121,   -3 }, /* (122) multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2 */
  {  118,   -4 }, /* (123) multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET */
  {  122,    0 }, /* (124) multipolygon_textz2 ::= */
  {  122,   -3 }, /* (125) multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2 */
  {  119,   -4 }, /* (126) multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET */
  {  123,    0 }, /* (127) multipolygon_textzm2 ::= */
  {  123,   -3 }, /* (128) multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2 */
  {   49,   -2 }, /* (129) geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text */
  {   63,   -2 }, /* (130) geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm */
  {   56,   -2 }, /* (131) geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz */
  {   70,   -2 }, /* (132) geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm */
  {  124,   -4 }, /* (133) geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  124,   -4 }, /* (134) geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  124,   -4 }, /* (135) geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  124,   -4 }, /* (136) geocoll_text ::= VANUATU_OPEN_BRACKET multipoint geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  124,   -4 }, /* (137) geocoll_text ::= VANUATU_OPEN_BRACKET multilinestring geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  124,   -4 }, /* (138) geocoll_text ::= VANUATU_OPEN_BRACKET multipolygon geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  124,   -5 }, /* (139) geocoll_text ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION geocoll_text geocoll_text2 VANUATU_CLOSE_BRACKET */
  {  128,    0 }, /* (140) geocoll_text2 ::= */
  {  128,   -3 }, /* (141) geocoll_text2 ::= VANUATU_COMMA point geocoll_text2 */
  {  128,   -3 }, /* (142) geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2 */
  {  128,   -3 }, /* (143) geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2 */
  {  128,   -3 }, /* (144) geocoll_text2 ::= VANUATU_COMMA multipoint geocoll_text2 */
  {  128,   -3 }, /* (145) geocoll_text2 ::= VANUATU_COMMA multilinestring geocoll_text2 */
  {  128,   -3 }, /* (146) geocoll_text2 ::= VANUATU_COMMA multipolygon geocoll_text2 */
  {  128,   -4 }, /* (147) geocoll_text2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION geocoll_text geocoll_text2 */
  {  125,   -4 }, /* (148) geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  125,   -4 }, /* (149) geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  125,   -4 }, /* (150) geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  125,   -4 }, /* (151) geocoll_textm ::= VANUATU_OPEN_BRACKET multipointm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  125,   -4 }, /* (152) geocoll_textm ::= VANUATU_OPEN_BRACKET multilinestringm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  125,   -4 }, /* (153) geocoll_textm ::= VANUATU_OPEN_BRACKET multipolygonm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  125,   -5 }, /* (154) geocoll_textm ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 VANUATU_CLOSE_BRACKET */
  {  129,    0 }, /* (155) geocoll_textm2 ::= */
  {  129,   -3 }, /* (156) geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2 */
  {  129,   -3 }, /* (157) geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2 */
  {  129,   -3 }, /* (158) geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2 */
  {  129,   -3 }, /* (159) geocoll_textm2 ::= VANUATU_COMMA multipointm geocoll_textm2 */
  {  129,   -3 }, /* (160) geocoll_textm2 ::= VANUATU_COMMA multilinestringm geocoll_textm2 */
  {  129,   -3 }, /* (161) geocoll_textm2 ::= VANUATU_COMMA multipolygonm geocoll_textm2 */
  {  129,   -4 }, /* (162) geocoll_textm2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 */
  {  126,   -4 }, /* (163) geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  126,   -4 }, /* (164) geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  126,   -4 }, /* (165) geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  126,   -4 }, /* (166) geocoll_textz ::= VANUATU_OPEN_BRACKET multipointz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  126,   -4 }, /* (167) geocoll_textz ::= VANUATU_OPEN_BRACKET multilinestringz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  126,   -4 }, /* (168) geocoll_textz ::= VANUATU_OPEN_BRACKET multipolygonz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  126,   -5 }, /* (169) geocoll_textz ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz geocoll_textz2 VANUATU_CLOSE_BRACKET */
  {  130,    0 }, /* (170) geocoll_textz2 ::= */
  {  130,   -3 }, /* (171) geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2 */
  {  130,   -3 }, /* (172) geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2 */
  {  130,   -3 }, /* (173) geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2 */
  {  130,   -3 }, /* (174) geocoll_textz2 ::= VANUATU_COMMA multipointz geocoll_textz2 */
  {  130,   -3 }, /* (175) geocoll_textz2 ::= VANUATU_COMMA multilinestringz geocoll_textz2 */
  {  130,   -3 }, /* (176) geocoll_textz2 ::= VANUATU_COMMA multipolygonz geocoll_textz2 */
  {  130,   -4 }, /* (177) geocoll_textz2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz geocoll_textz2 */
  {  127,   -4 }, /* (178) geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  127,   -4 }, /* (179) geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  127,   -4 }, /* (180) geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  127,   -4 }, /* (181) geocoll_textzm ::= VANUATU_OPEN_BRACKET multipointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  127,   -4 }, /* (182) geocoll_textzm ::= VANUATU_OPEN_BRACKET multilinestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  127,   -4 }, /* (183) geocoll_textzm ::= VANUATU_OPEN_BRACKET multipolygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  127,   -5 }, /* (184) geocoll_textzm ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
  {  131,    0 }, /* (185) geocoll_textzm2 ::= */
  {  131,   -3 }, /* (186) geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2 */
  {  131,   -3 }, /* (187) geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2 */
  {  131,   -3 }, /* (188) geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2 */
  {  131,   -3 }, /* (189) geocoll_textzm2 ::= VANUATU_COMMA multipointzm geocoll_textzm2 */
  {  131,   -3 }, /* (190) geocoll_textzm2 ::= VANUATU_COMMA multilinestringzm geocoll_textzm2 */
  {  131,   -3 }, /* (191) geocoll_textzm2 ::= VANUATU_COMMA multipolygonzm geocoll_textzm2 */
  {  131,   -4 }, /* (192) geocoll_textzm2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm geocoll_textzm2 */
  {   35,   -1 }, /* (193) main ::= in */
  {   36,    0 }, /* (194) in ::= */
  {   36,   -3 }, /* (195) in ::= in state VANUATU_NEWLINE */
  {   37,   -1 }, /* (196) state ::= program */
  {   38,   -1 }, /* (197) program ::= geo_text */
  {   38,   -1 }, /* (198) program ::= geo_textz */
  {   38,   -1 }, /* (199) program ::= geo_textm */
  {   38,   -1 }, /* (200) program ::= geo_textzm */
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
      case 1: /* geo_text ::= linestring */ yytestcase(yyruleno==1);
      case 2: /* geo_text ::= polygon */ yytestcase(yyruleno==2);
      case 3: /* geo_text ::= multipoint */ yytestcase(yyruleno==3);
      case 4: /* geo_text ::= multilinestring */ yytestcase(yyruleno==4);
      case 5: /* geo_text ::= multipolygon */ yytestcase(yyruleno==5);
      case 6: /* geo_text ::= geocoll */ yytestcase(yyruleno==6);
      case 7: /* geo_textz ::= pointz */ yytestcase(yyruleno==7);
      case 8: /* geo_textz ::= linestringz */ yytestcase(yyruleno==8);
      case 9: /* geo_textz ::= polygonz */ yytestcase(yyruleno==9);
      case 10: /* geo_textz ::= multipointz */ yytestcase(yyruleno==10);
      case 11: /* geo_textz ::= multilinestringz */ yytestcase(yyruleno==11);
      case 12: /* geo_textz ::= multipolygonz */ yytestcase(yyruleno==12);
      case 13: /* geo_textz ::= geocollz */ yytestcase(yyruleno==13);
      case 14: /* geo_textm ::= pointm */ yytestcase(yyruleno==14);
      case 15: /* geo_textm ::= linestringm */ yytestcase(yyruleno==15);
      case 16: /* geo_textm ::= polygonm */ yytestcase(yyruleno==16);
      case 17: /* geo_textm ::= multipointm */ yytestcase(yyruleno==17);
      case 18: /* geo_textm ::= multilinestringm */ yytestcase(yyruleno==18);
      case 19: /* geo_textm ::= multipolygonm */ yytestcase(yyruleno==19);
      case 20: /* geo_textm ::= geocollm */ yytestcase(yyruleno==20);
      case 21: /* geo_textzm ::= pointzm */ yytestcase(yyruleno==21);
      case 22: /* geo_textzm ::= linestringzm */ yytestcase(yyruleno==22);
      case 23: /* geo_textzm ::= polygonzm */ yytestcase(yyruleno==23);
      case 24: /* geo_textzm ::= multipointzm */ yytestcase(yyruleno==24);
      case 25: /* geo_textzm ::= multilinestringzm */ yytestcase(yyruleno==25);
      case 26: /* geo_textzm ::= multipolygonzm */ yytestcase(yyruleno==26);
      case 27: /* geo_textzm ::= geocollzm */ yytestcase(yyruleno==27);
{ p_data->result = yymsp[0].minor.yy0; }
        break;
      case 28: /* point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET */
{ yymsp[-3].minor.yy0 = vanuatu_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 29: /* pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET */
      case 30: /* pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==30);
      case 31: /* pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==31);
{ yymsp[-3].minor.yy0 = vanuatu_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0);  }
        break;
      case 32: /* point_brkt_coordxy ::= VANUATU_OPEN_BRACKET coord coord VANUATU_CLOSE_BRACKET */
{ yymsp[-3].minor.yy0 = (void *) vanuatu_point_xy( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 33: /* point_brkt_coordxym ::= VANUATU_OPEN_BRACKET coord coord coord VANUATU_CLOSE_BRACKET */
{ yymsp[-4].minor.yy0 = (void *) vanuatu_point_xym( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 34: /* point_brkt_coordxyz ::= VANUATU_OPEN_BRACKET coord coord coord VANUATU_CLOSE_BRACKET */
{ yymsp[-4].minor.yy0 = (void *) vanuatu_point_xyz( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 35: /* point_brkt_coordxyzm ::= VANUATU_OPEN_BRACKET coord coord coord coord VANUATU_CLOSE_BRACKET */
{ yymsp[-5].minor.yy0 = (void *) vanuatu_point_xyzm( p_data, (double *)yymsp[-4].minor.yy0, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 36: /* point_coordxy ::= coord coord */
{ yylhsminor.yy0 = (void *) vanuatu_point_xy( p_data, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-1].minor.yy0 = yylhsminor.yy0;
        break;
      case 37: /* point_coordxym ::= coord coord coord */
{ yylhsminor.yy0 = (void *) vanuatu_point_xym( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 38: /* point_coordxyz ::= coord coord coord */
{ yylhsminor.yy0 = (void *) vanuatu_point_xyz( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 39: /* point_coordxyzm ::= coord coord coord coord */
{ yylhsminor.yy0 = (void *) vanuatu_point_xyzm( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
  yymsp[-3].minor.yy0 = yylhsminor.yy0;
        break;
      case 40: /* coord ::= VANUATU_NUM */
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
      case 42: /* extra_brkt_pointsxy ::= VANUATU_COMMA point_brkt_coordxy extra_brkt_pointsxy */
      case 44: /* extra_brkt_pointsxym ::= VANUATU_COMMA point_brkt_coordxym extra_brkt_pointsxym */ yytestcase(yyruleno==44);
      case 46: /* extra_brkt_pointsxyz ::= VANUATU_COMMA point_brkt_coordxyz extra_brkt_pointsxyz */ yytestcase(yyruleno==46);
      case 48: /* extra_brkt_pointsxyzm ::= VANUATU_COMMA point_brkt_coordxyzm extra_brkt_pointsxyzm */ yytestcase(yyruleno==48);
      case 50: /* extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy */ yytestcase(yyruleno==50);
      case 52: /* extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym */ yytestcase(yyruleno==52);
      case 54: /* extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==54);
      case 56: /* extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm */ yytestcase(yyruleno==56);
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 57: /* linestring ::= VANUATU_LINESTRING linestring_text */
      case 58: /* linestringm ::= VANUATU_LINESTRING_M linestring_textm */ yytestcase(yyruleno==58);
      case 59: /* linestringz ::= VANUATU_LINESTRING_Z linestring_textz */ yytestcase(yyruleno==59);
      case 60: /* linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm */ yytestcase(yyruleno==60);
{ yymsp[-1].minor.yy0 = vanuatu_buildGeomFromLinestring( p_data, (gaiaLinestringPtr)yymsp[0].minor.yy0); }
        break;
      case 61: /* linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) vanuatu_linestring_xy( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 62: /* linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) vanuatu_linestring_xym( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 63: /* linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) vanuatu_linestring_xyz( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 64: /* linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) vanuatu_linestring_xyzm( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 65: /* polygon ::= VANUATU_POLYGON polygon_text */
      case 66: /* polygonm ::= VANUATU_POLYGON_M polygon_textm */ yytestcase(yyruleno==66);
      case 67: /* polygonz ::= VANUATU_POLYGON_Z polygon_textz */ yytestcase(yyruleno==67);
      case 68: /* polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm */ yytestcase(yyruleno==68);
{ yymsp[-1].minor.yy0 = vanuatu_buildGeomFromPolygon( p_data, (gaiaPolygonPtr)yymsp[0].minor.yy0); }
        break;
      case 69: /* polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_polygon_xy( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 70: /* polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_polygon_xym( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 71: /* polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET */
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_polygon_xyz( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 72: /* polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_polygon_xyzm( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 73: /* ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) vanuatu_ring_xy( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 75: /* extra_rings ::= VANUATU_COMMA ring extra_rings */
      case 78: /* extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm */ yytestcase(yyruleno==78);
      case 81: /* extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz */ yytestcase(yyruleno==81);
      case 84: /* extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm */ yytestcase(yyruleno==84);
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 76: /* ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) vanuatu_ring_xym( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 79: /* ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) vanuatu_ring_xyz( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 82: /* ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) vanuatu_ring_xyzm( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 85: /* multipoint ::= VANUATU_MULTIPOINT multipoint_text */
      case 86: /* multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm */ yytestcase(yyruleno==86);
      case 87: /* multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz */ yytestcase(yyruleno==87);
      case 88: /* multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm */ yytestcase(yyruleno==88);
      case 97: /* multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text */ yytestcase(yyruleno==97);
      case 98: /* multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm */ yytestcase(yyruleno==98);
      case 99: /* multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz */ yytestcase(yyruleno==99);
      case 100: /* multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm */ yytestcase(yyruleno==100);
      case 113: /* multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text */ yytestcase(yyruleno==113);
      case 114: /* multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm */ yytestcase(yyruleno==114);
      case 115: /* multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz */ yytestcase(yyruleno==115);
      case 116: /* multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm */ yytestcase(yyruleno==116);
      case 129: /* geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text */ yytestcase(yyruleno==129);
      case 130: /* geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm */ yytestcase(yyruleno==130);
      case 131: /* geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz */ yytestcase(yyruleno==131);
      case 132: /* geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm */ yytestcase(yyruleno==132);
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 89: /* multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
      case 93: /* multipoint_text ::= VANUATU_OPEN_BRACKET point_brkt_coordxy extra_brkt_pointsxy VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==93);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipoint_xy( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 90: /* multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
      case 94: /* multipoint_textm ::= VANUATU_OPEN_BRACKET point_brkt_coordxym extra_brkt_pointsxym VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==94);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipoint_xym( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 91: /* multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
      case 95: /* multipoint_textz ::= VANUATU_OPEN_BRACKET point_brkt_coordxyz extra_brkt_pointsxyz VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==95);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipoint_xyz( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 92: /* multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
      case 96: /* multipoint_textzm ::= VANUATU_OPEN_BRACKET point_brkt_coordxyzm extra_brkt_pointsxyzm VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==96);
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipoint_xyzm( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 101: /* multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multilinestring_xy( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 103: /* multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2 */
      case 106: /* multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2 */ yytestcase(yyruleno==106);
      case 109: /* multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==109);
      case 112: /* multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2 */ yytestcase(yyruleno==112);
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 104: /* multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multilinestring_xym( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 107: /* multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multilinestring_xyz( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 110: /* multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multilinestring_xyzm( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 117: /* multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipolygon_xy( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 119: /* multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2 */
      case 122: /* multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2 */ yytestcase(yyruleno==122);
      case 125: /* multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==125);
      case 128: /* multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2 */ yytestcase(yyruleno==128);
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 120: /* multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipolygon_xym( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 123: /* multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipolygon_xyz( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 126: /* multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) vanuatu_multipolygon_xyzm( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 133: /* geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET */
      case 134: /* geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==134);
      case 135: /* geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==135);
      case 136: /* geocoll_text ::= VANUATU_OPEN_BRACKET multipoint geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==136);
      case 137: /* geocoll_text ::= VANUATU_OPEN_BRACKET multilinestring geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==137);
      case 138: /* geocoll_text ::= VANUATU_OPEN_BRACKET multipolygon geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==138);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_geomColl_xy( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 139: /* geocoll_text ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION geocoll_text geocoll_text2 VANUATU_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) vanuatu_geomColl_xy( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 141: /* geocoll_text2 ::= VANUATU_COMMA point geocoll_text2 */
      case 142: /* geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2 */ yytestcase(yyruleno==142);
      case 143: /* geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2 */ yytestcase(yyruleno==143);
      case 144: /* geocoll_text2 ::= VANUATU_COMMA multipoint geocoll_text2 */ yytestcase(yyruleno==144);
      case 145: /* geocoll_text2 ::= VANUATU_COMMA multilinestring geocoll_text2 */ yytestcase(yyruleno==145);
      case 146: /* geocoll_text2 ::= VANUATU_COMMA multipolygon geocoll_text2 */ yytestcase(yyruleno==146);
      case 156: /* geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2 */ yytestcase(yyruleno==156);
      case 157: /* geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2 */ yytestcase(yyruleno==157);
      case 158: /* geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2 */ yytestcase(yyruleno==158);
      case 159: /* geocoll_textm2 ::= VANUATU_COMMA multipointm geocoll_textm2 */ yytestcase(yyruleno==159);
      case 160: /* geocoll_textm2 ::= VANUATU_COMMA multilinestringm geocoll_textm2 */ yytestcase(yyruleno==160);
      case 161: /* geocoll_textm2 ::= VANUATU_COMMA multipolygonm geocoll_textm2 */ yytestcase(yyruleno==161);
      case 171: /* geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2 */ yytestcase(yyruleno==171);
      case 172: /* geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2 */ yytestcase(yyruleno==172);
      case 173: /* geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2 */ yytestcase(yyruleno==173);
      case 174: /* geocoll_textz2 ::= VANUATU_COMMA multipointz geocoll_textz2 */ yytestcase(yyruleno==174);
      case 175: /* geocoll_textz2 ::= VANUATU_COMMA multilinestringz geocoll_textz2 */ yytestcase(yyruleno==175);
      case 176: /* geocoll_textz2 ::= VANUATU_COMMA multipolygonz geocoll_textz2 */ yytestcase(yyruleno==176);
      case 186: /* geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2 */ yytestcase(yyruleno==186);
      case 187: /* geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2 */ yytestcase(yyruleno==187);
      case 188: /* geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2 */ yytestcase(yyruleno==188);
      case 189: /* geocoll_textzm2 ::= VANUATU_COMMA multipointzm geocoll_textzm2 */ yytestcase(yyruleno==189);
      case 190: /* geocoll_textzm2 ::= VANUATU_COMMA multilinestringzm geocoll_textzm2 */ yytestcase(yyruleno==190);
      case 191: /* geocoll_textzm2 ::= VANUATU_COMMA multipolygonzm geocoll_textzm2 */ yytestcase(yyruleno==191);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 147: /* geocoll_text2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION geocoll_text geocoll_text2 */
      case 162: /* geocoll_textm2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 */ yytestcase(yyruleno==162);
      case 177: /* geocoll_textz2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz geocoll_textz2 */ yytestcase(yyruleno==177);
      case 192: /* geocoll_textzm2 ::= VANUATU_COMMA VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm geocoll_textzm2 */ yytestcase(yyruleno==192);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yymsp[-3].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 148: /* geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET */
      case 149: /* geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==149);
      case 150: /* geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==150);
      case 151: /* geocoll_textm ::= VANUATU_OPEN_BRACKET multipointm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==151);
      case 152: /* geocoll_textm ::= VANUATU_OPEN_BRACKET multilinestringm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==152);
      case 153: /* geocoll_textm ::= VANUATU_OPEN_BRACKET multipolygonm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==153);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_geomColl_xym( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 154: /* geocoll_textm ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_M geocoll_textm geocoll_textm2 VANUATU_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) vanuatu_geomColl_xym( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 163: /* geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET */
      case 164: /* geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==164);
      case 165: /* geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==165);
      case 166: /* geocoll_textz ::= VANUATU_OPEN_BRACKET multipointz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==166);
      case 167: /* geocoll_textz ::= VANUATU_OPEN_BRACKET multilinestringz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==167);
      case 168: /* geocoll_textz ::= VANUATU_OPEN_BRACKET multipolygonz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==168);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_geomColl_xyz( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 169: /* geocoll_textz ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz geocoll_textz2 VANUATU_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) vanuatu_geomColl_xyz( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 178: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
      case 179: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==179);
      case 180: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==180);
      case 181: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET multipointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==181);
      case 182: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET multilinestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==182);
      case 183: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET multipolygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==183);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) vanuatu_geomColl_xyzm( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 184: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-4].minor.yy0 = (void *) vanuatu_geomColl_xyzm( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      default:
      /* (193) main ::= in */ yytestcase(yyruleno==193);
      /* (194) in ::= */ yytestcase(yyruleno==194);
      /* (195) in ::= in state VANUATU_NEWLINE */ yytestcase(yyruleno==195);
      /* (196) state ::= program (OPTIMIZED OUT) */ assert(yyruleno!=196);
      /* (197) program ::= geo_text (OPTIMIZED OUT) */ assert(yyruleno!=197);
      /* (198) program ::= geo_textz (OPTIMIZED OUT) */ assert(yyruleno!=198);
      /* (199) program ::= geo_textm (OPTIMIZED OUT) */ assert(yyruleno!=199);
      /* (200) program ::= geo_textzm (OPTIMIZED OUT) */ assert(yyruleno!=200);
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
** Sandro Furieri 2010 Apr 4
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	p_data->vanuatu_parse_error = 1;
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
