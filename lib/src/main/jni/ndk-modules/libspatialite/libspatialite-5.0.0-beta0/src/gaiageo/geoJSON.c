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
#define YYNOCODE 84
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  struct geoJson_data *p_data ;
#define ParseARG_PDECL , struct geoJson_data *p_data 
#define ParseARG_FETCH  struct geoJson_data *p_data  = yypParser->p_data 
#define ParseARG_STORE yypParser->p_data  = p_data 
#define YYNSTATE             532
#define YYNRULE              159
#define YYNTOKEN             25
#define YY_MAX_SHIFT         531
#define YY_MIN_SHIFTREDUCE   679
#define YY_MAX_SHIFTREDUCE   837
#define YY_ERROR_ACTION      838
#define YY_ACCEPT_ACTION     839
#define YY_NO_ACTION         840
#define YY_MIN_REDUCE        841
#define YY_MAX_REDUCE        999
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
#define YY_ACTTAB_COUNT (820)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   531,  531,  531,  841,  842,  843,  844,  845,  846,  847,
 /*    10 */   848,  849,  850,  851,  852,  853,  854,  528,  104,  219,
 /*    20 */    97,   96,   95,  189,   88,   87,  994,  514,  530,    2,
 /*    30 */   473,  428,  381,  342,  303,  264,   95,  709,   88,   87,
 /*    40 */    94,  217,   93,   92,  103,  233,  102,  101,  104,   86,
 /*    50 */    97,   96,  258,  244,  214,  211,  208,  968,  229,  225,
 /*    60 */   839,    1,  302,  235,  297,  254,  250,  289,  263,  139,
 /*    70 */   341,  131,  336,  201,  380,  328,  375,  514,  427,  367,
 /*    80 */   414,  242,  472,  406,  461,  514,  527,  453,  522,  710,
 /*    90 */   711,  511,  477,  475,  185,  184,  193,  192,  197,  196,
 /*   100 */   260,  205,  204,  207,  195,  247,  222,  251,  226,  275,
 /*   110 */   255,  272,  238,  237,  230,  266,  265,  469,  274,  273,
 /*   120 */   270,  465,  278,  277,  514,  287,  282,  284,  286,  285,
 /*   130 */   514,  288,  276,  295,  486,  291,  290,  514,  113,  109,
 /*   140 */   309,   63,  305,  304,  514,  301,  300,  313,  312,  321,
 /*   150 */   317,  316,   74,  514,  325,  324,  327,  315,  330,  329,
 /*   160 */   334,  121,  117,  157,  514,  340,  339,  344,  343,  348,
 /*   170 */   352,  351,  220,  514,  356,  355,  360,  221,  364,  363,
 /*   180 */   514,  366,  354,  369,  368,  373,  123,  379,  378,  514,
 /*   190 */   122,  387,  399,  383,  382,  514,  514,  391,  390,   29,
 /*   200 */   395,  394,  405,  393,  403,  402,  408,  407,  412,    4,
 /*   210 */   423,   40,  514,  434,  418,  426,  425,  514,  430,  429,
 /*   220 */   107,  438,  437,  442,  441,  446,  450,  449,   51,  514,
 /*   230 */   452,  440,  455,  454,  459,  471,  470,  488,  514,  492,
 /*   240 */   496,  487,  500,  514,  495,  504,  499,  508,   62,  514,
 /*   250 */   191,  507,  510,  498,  516,  203,  222,  520,  515,  526,
 /*   260 */   314,  514,  311,  525,    3,  194,  326,  353,  323,  350,
 /*   270 */   206,  365,  115,  362,  392,  404,  389,  401,  439,  451,
 /*   280 */   436,  448,  497,  509,  494,  506,  155,  226,  144,  230,
 /*   290 */     5,  707,  987,  986,  247,  985,  236,  245,  246,  111,
 /*   300 */   119,  150,  251,  255,  980,  979,  978,    6,  261,  262,
 /*   310 */   108,  961,  129,  131,  106,  298,  112,  137,  116,  958,
 /*   320 */   139,  299,  110,  943,  120,  114,  465,  469,  118,  141,
 /*   330 */   147,  337,  377,  940,  130,  376,  911,  338,  138,  415,
 /*   340 */   420,  125,  416,  417,  418,  419,  133,  124,  908,  463,
 /*   350 */   421,  422,  140,  423,  877,  132,  811,  805,  424,    8,
 /*   360 */   146,  464,  142,  462,  148,  186,  187,  875,  188,  466,
 /*   370 */   467,  468,  999,    9,  190,  512,  513,  523,  169,  183,
 /*   380 */   524,  809,  803,  810,   10,  804,   11,  199,  198,  200,
 /*   390 */   202,  170,  808,   12,   13,  802,   14,   15,  209,   16,
 /*   400 */   210,   17,  478,  212,  213,  218,  215,  216,  159,  482,
 /*   410 */   840,  821,   89,  224,  831,  840,  820,  829,   90,  223,
 /*   420 */   227,  228,   91,  483,  160,  827,  231,  232,  234,  840,
 /*   430 */   490,  807,   18,  819,  801,  240,  840,  243,  840,  840,
 /*   440 */   840,   77,   98,  241,   19,  840,  814,  813,  181,   99,
 /*   450 */   248,  239,  830,  249,  840,  502,  828,  252,  253,  826,
 /*   460 */   256,  100,  800,  161,  257,  259,   80,  105,  840,  793,
 /*   470 */   162,  812,   20,  787,  840,  701,   21,  267,  268,  171,
 /*   480 */   280,  271,  269,  840,  695,  283,  791,  785,  792,   81,
 /*   490 */   786,   22,   23,  292,  281,  840,  279,   25,  296,  840,
 /*   500 */   790,   24,  840,  784,  172,   26,  840,  788,  294,   27,
 /*   510 */   789,  840,  783,  840,  163,  840,  293,  797,  840,   28,
 /*   520 */   840,  782,  794,  775,   30,  769,   31,  840,  840,  308,
 /*   530 */   306,  307,  173,  319,   32,  310,  840,  840,  773,   33,
 /*   540 */   767,  774,  840,   34,  768,  320,  322,  318,  840,  840,
 /*   550 */    36,   35,  772,  766,  174,   37,  840,   39,   38,  840,
 /*   560 */   771,  765,  331,  840,  840,  332,  335,  333,  840,  779,
 /*   570 */   840,  770,  776,  164,  764,  345,   41,  840,  761,   42,
 /*   580 */   346,  755,  840,  840,  349,  759,  840,  347,  357,   43,
 /*   590 */   753,  175,   44,  840,  760,   45,  840,  840,  754,  358,
 /*   600 */   359,  176,   46,  361,  758,   47,   48,  840,  752,   49,
 /*   610 */   840,  757,  751,  370,  840,  371,  374,  840,  372,  840,
 /*   620 */    50,   52,  165,  840,  763,  762,  840,  756,  840,  750,
 /*   630 */   741,  735,   53,  384,  840,   54,  385,  388,   55,  840,
 /*   640 */   739,  840,  386,  177,  733,   56,  740,  734,  840,  840,
 /*   650 */   396,  397,  400,  178,  840,  398,   57,   58,   59,  738,
 /*   660 */   732,  840,   60,  840,  737,  731,  409,  410,  840,  413,
 /*   670 */   840,  411,  840,   61,  126,  736,  127,  747,  128,  840,
 /*   680 */    64,  840,  840,  134,  135,  743,  744,  730,  840,  727,
 /*   690 */   136,  840,  166,  840,   65,  721,  742,  840,  431,  432,
 /*   700 */   435,  840,  481,   67,  840,  840,  433,   66,  725,  719,
 /*   710 */   179,  704,  840,   68,  840,  726,  720,  443,  840,  489,
 /*   720 */   444,  447,  445,  840,  180,  840,   69,   70,  702,  724,
 /*   730 */   718,   71,  840,  723,   72,  840,  717,  696,  456,  840,
 /*   740 */   457,  460,   78,  458,  840,   73,  840,  155,  143,  840,
 /*   750 */   729,  703,  145,   75,  840,  728,   79,  149,  840,  707,
 /*   760 */   167,  722,  151,  840,  501,  840,  716,  840,  706,  474,
 /*   770 */   705,  476,  479,    7,  840,  480,  484,  485,  491,  840,
 /*   780 */    76,  698,  493,  840,  840,  697,  505,  182,  840,  152,
 /*   790 */   503,   82,  153,  840,  154,  840,   83,  840,  700,  694,
 /*   800 */   517,  518,  840,  840,  521,  840,  840,  840,  519,   84,
 /*   810 */   699,  156,  708,  840,   85,  158,  529,  693,  168,  834,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
 /*    10 */    38,   39,   40,   41,   42,   43,   44,    5,   75,    2,
 /*    20 */    77,   78,   79,   46,   81,   82,    0,   50,    2,   10,
 /*    30 */    18,   19,   20,   21,   22,   23,   79,   15,   81,   82,
 /*    40 */    79,    5,   81,   82,   75,    5,   77,   78,   75,   23,
 /*    50 */    77,   78,    5,    9,   18,   19,   12,   74,   18,   19,
 /*    60 */    26,   27,    7,    2,    9,   18,   19,   12,   24,   59,
 /*    70 */     7,   61,    9,   46,    7,   12,    9,   50,    7,   12,
 /*    80 */     9,   46,    7,   12,    9,   50,    7,   12,    9,   16,
 /*    90 */    17,   12,   51,   52,   73,   74,   73,   74,   73,   74,
 /*   100 */     2,   73,   74,   47,   48,   57,   58,   55,   56,    7,
 /*   110 */    45,    9,   73,   74,   49,   69,   70,   45,   69,   70,
 /*   120 */    46,   49,   69,   70,   50,    7,   46,    9,   69,   70,
 /*   130 */    50,   47,   48,   46,    2,   69,   70,   50,   57,   58,
 /*   140 */    46,   10,   65,   66,   50,   69,   70,   65,   66,   46,
 /*   150 */    65,   66,   10,   50,   65,   66,   47,   48,   65,   66,
 /*   160 */    46,   55,   56,   10,   50,   65,   66,   63,   64,   46,
 /*   170 */    63,   64,   80,   50,   63,   64,   46,   80,   63,   64,
 /*   180 */    50,   47,   48,   63,   64,   46,   45,   63,   64,   50,
 /*   190 */    49,   46,   46,   57,   58,   50,   50,   57,   58,   10,
 /*   200 */    57,   58,   47,   48,   57,   58,   57,   58,   46,    6,
 /*   210 */    45,   10,   50,   46,   49,   57,   58,   50,   55,   56,
 /*   220 */    10,   55,   56,   55,   56,   46,   55,   56,   10,   50,
 /*   230 */    47,   48,   55,   56,   46,   55,   56,   45,   50,   46,
 /*   240 */    45,   49,   45,   50,   49,   46,   49,   45,   10,   50,
 /*   250 */     9,   49,   47,   48,   45,    9,   58,   46,   49,   45,
 /*   260 */     7,   50,    9,   49,   10,   24,    7,    7,    9,    9,
 /*   270 */    24,    7,   10,    9,    7,    7,    9,    9,    7,    7,
 /*   280 */     9,    9,    7,    7,    9,    9,    6,   56,   10,   49,
 /*   290 */     6,   11,   80,   80,   57,   80,   80,   76,   76,   10,
 /*   300 */    10,   10,   55,   45,   76,   76,   76,   10,   76,   73,
 /*   310 */     6,   72,   10,   61,   58,   72,    6,   10,    6,   71,
 /*   320 */    59,   71,   57,   68,    6,   56,   49,   45,   55,    6,
 /*   330 */     6,   68,   53,   67,    6,   54,   62,   67,    6,   54,
 /*   340 */    53,   49,   49,   49,   49,   62,   45,   61,   60,   50,
 /*   350 */    45,   45,   49,   45,   54,   59,    8,    8,   60,    4,
 /*   360 */    45,   50,   49,   54,   45,   24,    6,   53,   11,   53,
 /*   370 */    50,   50,   50,   10,    4,   50,   50,   50,    6,   50,
 /*   380 */    50,    8,    8,    8,    4,    8,    4,    6,   24,   11,
 /*   390 */     4,    6,    8,   10,    4,    8,    4,    4,    7,    4,
 /*   400 */     6,    4,   13,    7,    6,    3,    7,    6,    4,    6,
 /*   410 */    83,   11,    4,    6,    8,   83,   11,    8,    4,    7,
 /*   420 */     7,    6,    4,   13,    4,    8,    7,    6,    3,   83,
 /*   430 */     6,    8,    4,   11,    8,    6,   83,    4,   83,   83,
 /*   440 */    83,   10,    4,   11,   10,   83,   11,   11,    6,    4,
 /*   450 */     7,   24,    8,    6,   83,    6,    8,    7,    6,    8,
 /*   460 */     7,    4,    8,    4,    6,    3,   10,    4,   83,    8,
 /*   470 */     6,   11,    4,    8,   83,    8,   10,    7,    6,    6,
 /*   480 */     6,    4,   11,   83,    8,    4,    8,    8,    8,    4,
 /*   490 */     8,    4,    4,    7,   11,   83,    7,    4,    4,   83,
 /*   500 */     8,   10,   83,    8,    6,    4,   83,    8,   11,    4,
 /*   510 */     8,   83,    8,   83,    6,   83,    6,   11,   83,   10,
 /*   520 */    83,    8,   11,    8,    4,    8,    4,   83,   83,   11,
 /*   530 */     7,    6,    6,    6,   10,    4,   83,   83,    8,    4,
 /*   540 */     8,    8,   83,    4,    8,   11,    4,    7,   83,   83,
 /*   550 */     4,   10,    8,    8,    6,    4,   83,   10,    4,   83,
 /*   560 */     8,    8,    7,   83,   83,    6,    4,   11,   83,   11,
 /*   570 */    83,    8,   11,    6,    8,    7,    4,   83,    8,    4,
 /*   580 */     6,    8,   83,   83,    4,    8,   83,   11,    7,   10,
 /*   590 */     8,    6,    4,   83,    8,    4,   83,   83,    8,    6,
 /*   600 */    11,    6,   10,    4,    8,    4,    4,   83,    8,    4,
 /*   610 */    83,    8,    8,    7,   83,    6,    4,   83,   11,   83,
 /*   620 */    10,    4,    6,   83,   11,   11,   83,    8,   83,    8,
 /*   630 */     8,    8,    4,    7,   83,   10,    6,    4,    4,   83,
 /*   640 */     8,   83,   11,    6,    8,    4,    8,    8,   83,   83,
 /*   650 */     7,    6,    4,    6,   83,   11,   10,    4,    4,    8,
 /*   660 */     8,   83,    4,   83,    8,    8,    7,    6,   83,    4,
 /*   670 */    83,   11,   83,   10,    6,    8,    6,   11,    6,   83,
 /*   680 */     4,   83,   83,    6,    6,   11,   11,    8,   83,    8,
 /*   690 */     6,   83,    6,   83,    4,    8,   11,   83,    7,    6,
 /*   700 */     4,   83,   14,    4,   83,   83,   11,   10,    8,    8,
 /*   710 */     6,    8,   83,    4,   83,    8,    8,    7,   83,    7,
 /*   720 */     6,    4,   11,   83,    6,   83,   10,    4,    8,    8,
 /*   730 */     8,    4,   83,    8,    4,   83,    8,    8,    7,   83,
 /*   740 */     6,    4,    4,   11,   83,   10,   83,    6,    6,   83,
 /*   750 */    11,    8,    6,    4,   83,   11,    4,    6,   83,   11,
 /*   760 */     6,    8,    6,   83,    7,   83,    8,   83,    8,    8,
 /*   770 */     8,    8,    2,    4,   83,    4,    4,    3,   11,   83,
 /*   780 */     4,    8,    4,   83,   83,    8,    4,    6,   83,    6,
 /*   790 */    11,    4,    6,   83,    6,   83,    4,   83,    8,    8,
 /*   800 */     7,    6,   83,   83,    4,   83,   83,   83,   11,   10,
 /*   810 */     8,    6,   11,   83,    4,    4,    3,    8,    6,    1,
 /*   820 */    83,   83,   83,   83,   83,   83,   83,   83,   83,   83,
 /*   830 */    83,   83,   83,   83,   83,   83,   83,   83,   83,   83,
 /*   840 */    83,   83,   83,   83,   83,
};
#define YY_SHIFT_COUNT    (531)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (818)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */   820,   26,   17,   61,   61,   98,   98,   73,   19,   22,
 /*    10 */    19,   19,   22,   19,  132,  131,  142,  153,   19,   22,
 /*    20 */   189,   22,  189,  189,   22,  189,  132,  189,   22,  131,
 /*    30 */   189,  201,   22,  201,  201,   22,  201,  132,  201,   22,
 /*    40 */   142,  201,  218,   22,  218,  218,   22,  218,  132,  218,
 /*    50 */    22,  153,  218,  131,   22,  131,  131,   22,  131,  132,
 /*    60 */   131,   22,  153,  238,  131,  142,   22,  142,  142,   22,
 /*    70 */   142,  132,  142,   22,  153,  142,  153,   22,  153,  153,
 /*    80 */    22,  153,  132,  153,   22,  153,  254,  203,  203,  210,
 /*    90 */   262,  278,  203,  203,  203,  203,  284,  284,  289,  290,
 /*   100 */   291,  284,  284,  284,  284,  297,  304,  302,  210,  304,
 /*   110 */   310,  307,  289,  310,  312,  278,  262,  312,  318,  291,
 /*   120 */   290,  318,  323,  324,  328,  323,  278,  278,  278,  278,
 /*   130 */   302,  328,  332,  324,  291,  291,  291,  291,  307,  332,
 /*   140 */   323,  278,  323,   22,   22,  278,  324,  291,  324,   22,
 /*   150 */    22,  291,   22,   22,   22,   22,   22,   22,   12,   36,
 /*   160 */    40,   47,   44,   55,   63,   67,   71,   75,   79,  241,
 /*   170 */   246,  102,  118,  253,  259,  260,  264,  267,  268,  271,
 /*   180 */   272,  275,  276,  280,  348,  349,  355,  341,  360,  357,
 /*   190 */   363,  370,  373,  374,  380,  372,  375,  377,  382,  364,
 /*   200 */   381,  378,  383,  386,  384,  387,  390,  385,  392,  393,
 /*   210 */   391,  394,  395,  396,  398,  397,  399,  401,  404,  402,
 /*   220 */   400,  405,  406,  408,  412,  407,  409,  414,  413,  415,
 /*   230 */   417,  418,  419,  421,  420,  425,  422,  423,  426,  428,
 /*   240 */   427,  429,  432,  434,  433,  435,  436,  444,  438,  443,
 /*   250 */   447,  448,  445,  450,  452,  451,  457,  453,  458,  459,
 /*   260 */   462,  460,  454,  463,  464,  461,  465,  468,  470,  472,
 /*   270 */   471,  466,  477,  478,  479,  487,  473,  480,  482,  488,
 /*   280 */   489,  474,  483,  491,  481,  492,  495,  493,  498,  501,
 /*   290 */   502,  504,  505,  486,  510,  497,  509,  494,  506,  511,
 /*   300 */   499,  513,  520,  508,  515,  517,  522,  523,  525,  518,
 /*   310 */   524,  531,  530,  532,  535,  526,  533,  536,  539,  540,
 /*   320 */   527,  534,  541,  542,  544,  545,  546,  548,  551,  552,
 /*   330 */   553,  554,  555,  559,  556,  547,  562,  558,  561,  563,
 /*   340 */   566,  572,  567,  570,  573,  575,  568,  574,  576,  579,
 /*   350 */   580,  577,  582,  588,  585,  586,  590,  591,  581,  593,
 /*   360 */   589,  592,  599,  596,  600,  601,  595,  602,  603,  604,
 /*   370 */   605,  606,  609,  607,  610,  612,  613,  614,  619,  621,
 /*   380 */   617,  616,  622,  623,  628,  626,  630,  631,  625,  633,
 /*   390 */   632,  636,  634,  637,  638,  639,  641,  643,  645,  644,
 /*   400 */   646,  648,  651,  652,  653,  647,  654,  656,  657,  658,
 /*   410 */   659,  661,  660,  663,  665,  666,  668,  670,  672,  674,
 /*   420 */   675,  677,  678,  684,  685,  667,  679,  676,  686,  681,
 /*   430 */   687,  690,  691,  693,  695,  697,  696,  700,  701,  699,
 /*   440 */   704,  707,  708,  709,  710,  714,  711,  716,  717,  721,
 /*   450 */   722,  723,  718,  727,  725,  728,  730,  731,  734,  732,
 /*   460 */   735,  737,  739,  741,  742,  746,  744,  748,  751,  756,
 /*   470 */   753,  758,  749,  754,  760,  761,  762,  763,  769,  389,
 /*   480 */   770,  771,  688,  403,  410,  772,  774,  703,  773,  776,
 /*   490 */   712,  424,  767,  431,  778,  720,  729,  738,  442,  743,
 /*   500 */   777,  752,  757,  449,  779,  456,  782,  467,  476,  485,
 /*   510 */   781,  787,  783,  786,  788,  790,  791,  792,  793,  795,
 /*   520 */   797,  799,  800,  801,  805,  802,  809,  810,  812,  811,
 /*   530 */   813,  818,
};
#define YY_REDUCE_COUNT (157)
#define YY_REDUCE_MIN   (-57)
#define YY_REDUCE_MAX   (330)
static const short yy_reduce_ofst[] = {
 /*     0 */    34,  -28,  -57,  -43,  -39,  -31,  -27,   41,   21,  -23,
 /*    10 */    23,   25,   27,   28,   56,   48,   52,   65,   39,   35,
 /*    20 */    46,   74,   49,   53,   80,   59,   84,   66,   87,   81,
 /*    30 */    76,   77,   94,   82,   85,  103,   89,  109,   93,  114,
 /*    40 */   106,  100,  104,  123,  107,  111,  130,  115,  134,  120,
 /*    50 */   139,  141,  124,  136,  145,  140,  143,  146,  147,  155,
 /*    60 */   149,  162,  165,   10,  158,  163,  167,  166,  168,  179,
 /*    70 */   171,  183,  177,  188,   72,  180,  192,  193,  195,  197,
 /*    80 */   199,  202,  205,  209,  211,  214,  -17,   92,   97,  198,
 /*    90 */   231,  240,  212,  213,  215,  216,  221,  222,  237,  247,
 /*   100 */   258,  228,  229,  230,  232,  236,  239,  252,  256,  243,
 /*   110 */   248,  261,  265,  250,  255,  277,  269,  263,  266,  282,
 /*   120 */   273,  270,  281,  279,  274,  285,  292,  293,  294,  295,
 /*   130 */   286,  283,  288,  287,  301,  305,  306,  308,  296,  298,
 /*   140 */   300,  303,  309,  299,  311,  313,  314,  315,  316,  320,
 /*   150 */   321,  319,  322,  325,  326,  327,  329,  330,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   995,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    10 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    20 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    30 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    40 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    50 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    60 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    70 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*    80 */   838,  838,  838,  838,  838,  838,  838,  984,  984,  838,
 /*    90 */   838,  838,  984,  984,  984,  984,  977,  977,  838,  838,
 /*   100 */   838,  977,  977,  977,  977,  838,  960,  838,  838,  960,
 /*   110 */   957,  838,  838,  957,  942,  838,  838,  942,  939,  838,
 /*   120 */   838,  939,  876,  874,  910,  876,  838,  838,  838,  838,
 /*   130 */   838,  910,  907,  874,  838,  838,  838,  838,  838,  907,
 /*   140 */   876,  838,  876,  838,  838,  838,  874,  838,  874,  838,
 /*   150 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   160 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   170 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   180 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   190 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   200 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   210 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   220 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   230 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   240 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   250 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   260 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   270 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   280 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   290 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   300 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   310 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   320 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   330 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   340 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   350 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   360 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   370 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   380 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   390 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   400 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   410 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   420 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   430 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   440 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   450 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   460 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   470 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   480 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   490 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   500 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   510 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   520 */   838,  838,  838,  838,  838,  838,  838,  838,  838,  838,
 /*   530 */   838,  838,
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
  /*    1 */ "GEOJSON_NEWLINE",
  /*    2 */ "GEOJSON_OPEN_BRACE",
  /*    3 */ "GEOJSON_TYPE",
  /*    4 */ "GEOJSON_COLON",
  /*    5 */ "GEOJSON_POINT",
  /*    6 */ "GEOJSON_COMMA",
  /*    7 */ "GEOJSON_COORDS",
  /*    8 */ "GEOJSON_CLOSE_BRACE",
  /*    9 */ "GEOJSON_BBOX",
  /*   10 */ "GEOJSON_OPEN_BRACKET",
  /*   11 */ "GEOJSON_CLOSE_BRACKET",
  /*   12 */ "GEOJSON_CRS",
  /*   13 */ "GEOJSON_NAME",
  /*   14 */ "GEOJSON_PROPS",
  /*   15 */ "GEOJSON_NUM",
  /*   16 */ "GEOJSON_SHORT_SRID",
  /*   17 */ "GEOJSON_LONG_SRID",
  /*   18 */ "GEOJSON_LINESTRING",
  /*   19 */ "GEOJSON_POLYGON",
  /*   20 */ "GEOJSON_MULTIPOINT",
  /*   21 */ "GEOJSON_MULTILINESTRING",
  /*   22 */ "GEOJSON_MULTIPOLYGON",
  /*   23 */ "GEOJSON_GEOMETRYCOLLECTION",
  /*   24 */ "GEOJSON_GEOMS",
  /*   25 */ "error",
  /*   26 */ "main",
  /*   27 */ "in",
  /*   28 */ "state",
  /*   29 */ "program",
  /*   30 */ "geo_text",
  /*   31 */ "point",
  /*   32 */ "pointz",
  /*   33 */ "linestring",
  /*   34 */ "linestringz",
  /*   35 */ "polygon",
  /*   36 */ "polygonz",
  /*   37 */ "multipoint",
  /*   38 */ "multipointz",
  /*   39 */ "multilinestring",
  /*   40 */ "multilinestringz",
  /*   41 */ "multipolygon",
  /*   42 */ "multipolygonz",
  /*   43 */ "geocoll",
  /*   44 */ "geocollz",
  /*   45 */ "point_coordxy",
  /*   46 */ "bbox",
  /*   47 */ "short_crs",
  /*   48 */ "long_crs",
  /*   49 */ "point_coordxyz",
  /*   50 */ "coord",
  /*   51 */ "short_srid",
  /*   52 */ "long_srid",
  /*   53 */ "extra_pointsxy",
  /*   54 */ "extra_pointsxyz",
  /*   55 */ "linestring_text",
  /*   56 */ "linestring_textz",
  /*   57 */ "polygon_text",
  /*   58 */ "polygon_textz",
  /*   59 */ "ring",
  /*   60 */ "extra_rings",
  /*   61 */ "ringz",
  /*   62 */ "extra_ringsz",
  /*   63 */ "multipoint_text",
  /*   64 */ "multipoint_textz",
  /*   65 */ "multilinestring_text",
  /*   66 */ "multilinestring_textz",
  /*   67 */ "multilinestring_text2",
  /*   68 */ "multilinestring_textz2",
  /*   69 */ "multipolygon_text",
  /*   70 */ "multipolygon_textz",
  /*   71 */ "multipolygon_text2",
  /*   72 */ "multipolygon_textz2",
  /*   73 */ "geocoll_text",
  /*   74 */ "geocoll_textz",
  /*   75 */ "coll_point",
  /*   76 */ "geocoll_text2",
  /*   77 */ "coll_linestring",
  /*   78 */ "coll_polygon",
  /*   79 */ "coll_pointz",
  /*   80 */ "geocoll_textz2",
  /*   81 */ "coll_linestringz",
  /*   82 */ "coll_polygonz",
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "geo_text ::= point",
 /*   1 */ "geo_text ::= pointz",
 /*   2 */ "geo_text ::= linestring",
 /*   3 */ "geo_text ::= linestringz",
 /*   4 */ "geo_text ::= polygon",
 /*   5 */ "geo_text ::= polygonz",
 /*   6 */ "geo_text ::= multipoint",
 /*   7 */ "geo_text ::= multipointz",
 /*   8 */ "geo_text ::= multilinestring",
 /*   9 */ "geo_text ::= multilinestringz",
 /*  10 */ "geo_text ::= multipolygon",
 /*  11 */ "geo_text ::= multipolygonz",
 /*  12 */ "geo_text ::= geocoll",
 /*  13 */ "geo_text ::= geocollz",
 /*  14 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  15 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  16 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  17 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  18 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  19 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /*  20 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  21 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  22 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  23 */ "pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  24 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  25 */ "point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /*  26 */ "short_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON short_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE",
 /*  27 */ "long_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON long_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE",
 /*  28 */ "point_coordxy ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET",
 /*  29 */ "point_coordxyz ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET",
 /*  30 */ "coord ::= GEOJSON_NUM",
 /*  31 */ "short_srid ::= GEOJSON_SHORT_SRID",
 /*  32 */ "long_srid ::= GEOJSON_LONG_SRID",
 /*  33 */ "extra_pointsxy ::=",
 /*  34 */ "extra_pointsxy ::= GEOJSON_COMMA point_coordxy extra_pointsxy",
 /*  35 */ "extra_pointsxyz ::=",
 /*  36 */ "extra_pointsxyz ::= GEOJSON_COMMA point_coordxyz extra_pointsxyz",
 /*  37 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  38 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  39 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  40 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  41 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  42 */ "linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /*  43 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  44 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  45 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  46 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  47 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  48 */ "linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /*  49 */ "linestring_text ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET",
 /*  50 */ "linestring_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET",
 /*  51 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  52 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  53 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  54 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  55 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  56 */ "polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /*  57 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  58 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  59 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  60 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  61 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  62 */ "polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /*  63 */ "polygon_text ::= GEOJSON_OPEN_BRACKET ring extra_rings GEOJSON_CLOSE_BRACKET",
 /*  64 */ "polygon_textz ::= GEOJSON_OPEN_BRACKET ringz extra_ringsz GEOJSON_CLOSE_BRACKET",
 /*  65 */ "ring ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET",
 /*  66 */ "extra_rings ::=",
 /*  67 */ "extra_rings ::= GEOJSON_COMMA ring extra_rings",
 /*  68 */ "ringz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET",
 /*  69 */ "extra_ringsz ::=",
 /*  70 */ "extra_ringsz ::= GEOJSON_COMMA ringz extra_ringsz",
 /*  71 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  72 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  73 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  74 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  75 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  76 */ "multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE",
 /*  77 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  78 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  79 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  80 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  81 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  82 */ "multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE",
 /*  83 */ "multipoint_text ::= GEOJSON_OPEN_BRACKET point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET",
 /*  84 */ "multipoint_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET",
 /*  85 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  86 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  87 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  88 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  89 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  90 */ "multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE",
 /*  91 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  92 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  93 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  94 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  95 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  96 */ "multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE",
 /*  97 */ "multilinestring_text ::= GEOJSON_OPEN_BRACKET linestring_text multilinestring_text2 GEOJSON_CLOSE_BRACKET",
 /*  98 */ "multilinestring_text2 ::=",
 /*  99 */ "multilinestring_text2 ::= GEOJSON_COMMA linestring_text multilinestring_text2",
 /* 100 */ "multilinestring_textz ::= GEOJSON_OPEN_BRACKET linestring_textz multilinestring_textz2 GEOJSON_CLOSE_BRACKET",
 /* 101 */ "multilinestring_textz2 ::=",
 /* 102 */ "multilinestring_textz2 ::= GEOJSON_COMMA linestring_textz multilinestring_textz2",
 /* 103 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 104 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 105 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 106 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 107 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 108 */ "multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE",
 /* 109 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 110 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 111 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 112 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 113 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 114 */ "multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE",
 /* 115 */ "multipolygon_text ::= GEOJSON_OPEN_BRACKET polygon_text multipolygon_text2 GEOJSON_CLOSE_BRACKET",
 /* 116 */ "multipolygon_text2 ::=",
 /* 117 */ "multipolygon_text2 ::= GEOJSON_COMMA polygon_text multipolygon_text2",
 /* 118 */ "multipolygon_textz ::= GEOJSON_OPEN_BRACKET polygon_textz multipolygon_textz2 GEOJSON_CLOSE_BRACKET",
 /* 119 */ "multipolygon_textz2 ::=",
 /* 120 */ "multipolygon_textz2 ::= GEOJSON_COMMA polygon_textz multipolygon_textz2",
 /* 121 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 122 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 123 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 124 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 125 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 126 */ "geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE",
 /* 127 */ "geocollz ::= GEOJSON_GEOMETRYCOLLECTION geocoll_textz",
 /* 128 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 129 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 130 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 131 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 132 */ "geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE",
 /* 133 */ "geocoll_text ::= GEOJSON_OPEN_BRACKET coll_point geocoll_text2 GEOJSON_CLOSE_BRACKET",
 /* 134 */ "geocoll_text ::= GEOJSON_OPEN_BRACKET coll_linestring geocoll_text2 GEOJSON_CLOSE_BRACKET",
 /* 135 */ "geocoll_text ::= GEOJSON_OPEN_BRACKET coll_polygon geocoll_text2 GEOJSON_CLOSE_BRACKET",
 /* 136 */ "geocoll_text2 ::=",
 /* 137 */ "geocoll_text2 ::= GEOJSON_COMMA coll_point geocoll_text2",
 /* 138 */ "geocoll_text2 ::= GEOJSON_COMMA coll_linestring geocoll_text2",
 /* 139 */ "geocoll_text2 ::= GEOJSON_COMMA coll_polygon geocoll_text2",
 /* 140 */ "geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_pointz geocoll_textz2 GEOJSON_CLOSE_BRACKET",
 /* 141 */ "geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_linestringz geocoll_textz2 GEOJSON_CLOSE_BRACKET",
 /* 142 */ "geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_polygonz geocoll_textz2 GEOJSON_CLOSE_BRACKET",
 /* 143 */ "geocoll_textz2 ::=",
 /* 144 */ "geocoll_textz2 ::= GEOJSON_COMMA coll_pointz geocoll_textz2",
 /* 145 */ "geocoll_textz2 ::= GEOJSON_COMMA coll_linestringz geocoll_textz2",
 /* 146 */ "geocoll_textz2 ::= GEOJSON_COMMA coll_polygonz geocoll_textz2",
 /* 147 */ "coll_point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE",
 /* 148 */ "coll_pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE",
 /* 149 */ "coll_linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE",
 /* 150 */ "coll_linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE",
 /* 151 */ "coll_polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE",
 /* 152 */ "coll_polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE",
 /* 153 */ "main ::= in",
 /* 154 */ "in ::=",
 /* 155 */ "in ::= in state GEOJSON_NEWLINE",
 /* 156 */ "state ::= program",
 /* 157 */ "program ::= geo_text",
 /* 158 */ "bbox ::= coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_COMMA coord",
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
  {   30,   -1 }, /* (0) geo_text ::= point */
  {   30,   -1 }, /* (1) geo_text ::= pointz */
  {   30,   -1 }, /* (2) geo_text ::= linestring */
  {   30,   -1 }, /* (3) geo_text ::= linestringz */
  {   30,   -1 }, /* (4) geo_text ::= polygon */
  {   30,   -1 }, /* (5) geo_text ::= polygonz */
  {   30,   -1 }, /* (6) geo_text ::= multipoint */
  {   30,   -1 }, /* (7) geo_text ::= multipointz */
  {   30,   -1 }, /* (8) geo_text ::= multilinestring */
  {   30,   -1 }, /* (9) geo_text ::= multilinestringz */
  {   30,   -1 }, /* (10) geo_text ::= multipolygon */
  {   30,   -1 }, /* (11) geo_text ::= multipolygonz */
  {   30,   -1 }, /* (12) geo_text ::= geocoll */
  {   30,   -1 }, /* (13) geo_text ::= geocollz */
  {   31,   -9 }, /* (14) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   31,  -15 }, /* (15) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   31,  -13 }, /* (16) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   31,  -13 }, /* (17) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   31,  -19 }, /* (18) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   31,  -19 }, /* (19) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   32,   -9 }, /* (20) pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   32,  -15 }, /* (21) pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   32,  -13 }, /* (22) pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   32,  -13 }, /* (23) pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   31,  -19 }, /* (24) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   31,  -19 }, /* (25) point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   47,  -13 }, /* (26) short_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON short_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE */
  {   48,  -13 }, /* (27) long_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON long_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE */
  {   45,   -5 }, /* (28) point_coordxy ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET */
  {   49,   -7 }, /* (29) point_coordxyz ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET */
  {   50,   -1 }, /* (30) coord ::= GEOJSON_NUM */
  {   51,   -1 }, /* (31) short_srid ::= GEOJSON_SHORT_SRID */
  {   52,   -1 }, /* (32) long_srid ::= GEOJSON_LONG_SRID */
  {   53,    0 }, /* (33) extra_pointsxy ::= */
  {   53,   -3 }, /* (34) extra_pointsxy ::= GEOJSON_COMMA point_coordxy extra_pointsxy */
  {   54,    0 }, /* (35) extra_pointsxyz ::= */
  {   54,   -3 }, /* (36) extra_pointsxyz ::= GEOJSON_COMMA point_coordxyz extra_pointsxyz */
  {   33,   -9 }, /* (37) linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   33,  -15 }, /* (38) linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   33,  -13 }, /* (39) linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   33,  -13 }, /* (40) linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   33,  -19 }, /* (41) linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   33,  -19 }, /* (42) linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   34,   -9 }, /* (43) linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   34,  -15 }, /* (44) linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   34,  -13 }, /* (45) linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   34,  -13 }, /* (46) linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   34,  -19 }, /* (47) linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   34,  -19 }, /* (48) linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   55,   -6 }, /* (49) linestring_text ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
  {   56,   -6 }, /* (50) linestring_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
  {   35,   -9 }, /* (51) polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   35,  -15 }, /* (52) polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   35,  -13 }, /* (53) polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   35,  -13 }, /* (54) polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   35,  -19 }, /* (55) polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   35,  -19 }, /* (56) polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   36,   -9 }, /* (57) polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   36,  -15 }, /* (58) polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   36,  -13 }, /* (59) polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   36,  -13 }, /* (60) polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   36,  -19 }, /* (61) polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   36,  -19 }, /* (62) polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   57,   -4 }, /* (63) polygon_text ::= GEOJSON_OPEN_BRACKET ring extra_rings GEOJSON_CLOSE_BRACKET */
  {   58,   -4 }, /* (64) polygon_textz ::= GEOJSON_OPEN_BRACKET ringz extra_ringsz GEOJSON_CLOSE_BRACKET */
  {   59,  -10 }, /* (65) ring ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
  {   60,    0 }, /* (66) extra_rings ::= */
  {   60,   -3 }, /* (67) extra_rings ::= GEOJSON_COMMA ring extra_rings */
  {   61,  -10 }, /* (68) ringz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
  {   62,    0 }, /* (69) extra_ringsz ::= */
  {   62,   -3 }, /* (70) extra_ringsz ::= GEOJSON_COMMA ringz extra_ringsz */
  {   37,   -9 }, /* (71) multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
  {   37,  -15 }, /* (72) multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
  {   37,  -13 }, /* (73) multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
  {   37,  -13 }, /* (74) multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
  {   37,  -19 }, /* (75) multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
  {   37,  -19 }, /* (76) multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
  {   38,   -9 }, /* (77) multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */
  {   38,  -15 }, /* (78) multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */
  {   38,  -13 }, /* (79) multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */
  {   38,  -13 }, /* (80) multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */
  {   38,  -19 }, /* (81) multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */
  {   38,  -19 }, /* (82) multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */
  {   63,   -4 }, /* (83) multipoint_text ::= GEOJSON_OPEN_BRACKET point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
  {   64,   -4 }, /* (84) multipoint_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
  {   39,   -9 }, /* (85) multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */
  {   39,  -15 }, /* (86) multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */
  {   39,  -13 }, /* (87) multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */
  {   39,  -13 }, /* (88) multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */
  {   39,  -19 }, /* (89) multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */
  {   39,  -19 }, /* (90) multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */
  {   40,   -9 }, /* (91) multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */
  {   40,  -15 }, /* (92) multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */
  {   40,  -13 }, /* (93) multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */
  {   40,  -13 }, /* (94) multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */
  {   40,  -19 }, /* (95) multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */
  {   40,  -19 }, /* (96) multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */
  {   65,   -4 }, /* (97) multilinestring_text ::= GEOJSON_OPEN_BRACKET linestring_text multilinestring_text2 GEOJSON_CLOSE_BRACKET */
  {   67,    0 }, /* (98) multilinestring_text2 ::= */
  {   67,   -3 }, /* (99) multilinestring_text2 ::= GEOJSON_COMMA linestring_text multilinestring_text2 */
  {   66,   -4 }, /* (100) multilinestring_textz ::= GEOJSON_OPEN_BRACKET linestring_textz multilinestring_textz2 GEOJSON_CLOSE_BRACKET */
  {   68,    0 }, /* (101) multilinestring_textz2 ::= */
  {   68,   -3 }, /* (102) multilinestring_textz2 ::= GEOJSON_COMMA linestring_textz multilinestring_textz2 */
  {   41,   -9 }, /* (103) multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */
  {   41,  -15 }, /* (104) multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */
  {   41,  -13 }, /* (105) multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */
  {   41,  -13 }, /* (106) multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */
  {   41,  -19 }, /* (107) multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */
  {   41,  -19 }, /* (108) multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */
  {   42,   -9 }, /* (109) multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */
  {   42,  -15 }, /* (110) multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */
  {   42,  -13 }, /* (111) multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */
  {   42,  -13 }, /* (112) multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */
  {   42,  -19 }, /* (113) multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */
  {   42,  -19 }, /* (114) multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */
  {   69,   -4 }, /* (115) multipolygon_text ::= GEOJSON_OPEN_BRACKET polygon_text multipolygon_text2 GEOJSON_CLOSE_BRACKET */
  {   71,    0 }, /* (116) multipolygon_text2 ::= */
  {   71,   -3 }, /* (117) multipolygon_text2 ::= GEOJSON_COMMA polygon_text multipolygon_text2 */
  {   70,   -4 }, /* (118) multipolygon_textz ::= GEOJSON_OPEN_BRACKET polygon_textz multipolygon_textz2 GEOJSON_CLOSE_BRACKET */
  {   72,    0 }, /* (119) multipolygon_textz2 ::= */
  {   72,   -3 }, /* (120) multipolygon_textz2 ::= GEOJSON_COMMA polygon_textz multipolygon_textz2 */
  {   43,   -9 }, /* (121) geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */
  {   43,  -15 }, /* (122) geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */
  {   43,  -13 }, /* (123) geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */
  {   43,  -13 }, /* (124) geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */
  {   43,  -19 }, /* (125) geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */
  {   43,  -19 }, /* (126) geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */
  {   44,   -2 }, /* (127) geocollz ::= GEOJSON_GEOMETRYCOLLECTION geocoll_textz */
  {   44,  -15 }, /* (128) geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */
  {   44,  -13 }, /* (129) geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */
  {   44,  -13 }, /* (130) geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */
  {   44,  -19 }, /* (131) geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */
  {   44,  -19 }, /* (132) geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */
  {   73,   -4 }, /* (133) geocoll_text ::= GEOJSON_OPEN_BRACKET coll_point geocoll_text2 GEOJSON_CLOSE_BRACKET */
  {   73,   -4 }, /* (134) geocoll_text ::= GEOJSON_OPEN_BRACKET coll_linestring geocoll_text2 GEOJSON_CLOSE_BRACKET */
  {   73,   -4 }, /* (135) geocoll_text ::= GEOJSON_OPEN_BRACKET coll_polygon geocoll_text2 GEOJSON_CLOSE_BRACKET */
  {   76,    0 }, /* (136) geocoll_text2 ::= */
  {   76,   -3 }, /* (137) geocoll_text2 ::= GEOJSON_COMMA coll_point geocoll_text2 */
  {   76,   -3 }, /* (138) geocoll_text2 ::= GEOJSON_COMMA coll_linestring geocoll_text2 */
  {   76,   -3 }, /* (139) geocoll_text2 ::= GEOJSON_COMMA coll_polygon geocoll_text2 */
  {   74,   -4 }, /* (140) geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_pointz geocoll_textz2 GEOJSON_CLOSE_BRACKET */
  {   74,   -4 }, /* (141) geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_linestringz geocoll_textz2 GEOJSON_CLOSE_BRACKET */
  {   74,   -4 }, /* (142) geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_polygonz geocoll_textz2 GEOJSON_CLOSE_BRACKET */
  {   80,    0 }, /* (143) geocoll_textz2 ::= */
  {   80,   -3 }, /* (144) geocoll_textz2 ::= GEOJSON_COMMA coll_pointz geocoll_textz2 */
  {   80,   -3 }, /* (145) geocoll_textz2 ::= GEOJSON_COMMA coll_linestringz geocoll_textz2 */
  {   80,   -3 }, /* (146) geocoll_textz2 ::= GEOJSON_COMMA coll_polygonz geocoll_textz2 */
  {   75,   -9 }, /* (147) coll_point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
  {   79,   -9 }, /* (148) coll_pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */
  {   77,   -9 }, /* (149) coll_linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
  {   81,   -9 }, /* (150) coll_linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */
  {   78,   -9 }, /* (151) coll_polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
  {   82,   -9 }, /* (152) coll_polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */
  {   26,   -1 }, /* (153) main ::= in */
  {   27,    0 }, /* (154) in ::= */
  {   27,   -3 }, /* (155) in ::= in state GEOJSON_NEWLINE */
  {   28,   -1 }, /* (156) state ::= program */
  {   29,   -1 }, /* (157) program ::= geo_text */
  {   46,   -7 }, /* (158) bbox ::= coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_COMMA coord */
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
      case 2: /* geo_text ::= linestring */ yytestcase(yyruleno==2);
      case 3: /* geo_text ::= linestringz */ yytestcase(yyruleno==3);
      case 4: /* geo_text ::= polygon */ yytestcase(yyruleno==4);
      case 5: /* geo_text ::= polygonz */ yytestcase(yyruleno==5);
      case 6: /* geo_text ::= multipoint */ yytestcase(yyruleno==6);
      case 7: /* geo_text ::= multipointz */ yytestcase(yyruleno==7);
      case 8: /* geo_text ::= multilinestring */ yytestcase(yyruleno==8);
      case 9: /* geo_text ::= multilinestringz */ yytestcase(yyruleno==9);
      case 10: /* geo_text ::= multipolygon */ yytestcase(yyruleno==10);
      case 11: /* geo_text ::= multipolygonz */ yytestcase(yyruleno==11);
      case 12: /* geo_text ::= geocoll */ yytestcase(yyruleno==12);
      case 13: /* geo_text ::= geocollz */ yytestcase(yyruleno==13);
{ p_data->result = yymsp[0].minor.yy0; }
        break;
      case 14: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 20: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==20);
{ yymsp[-8].minor.yy0 = geoJSON_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 15: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 21: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==21);
{ yymsp[-14].minor.yy0 = geoJSON_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 16: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 17: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==17);
      case 22: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==22);
      case 23: /* pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==23);
{ yymsp[-12].minor.yy0 = geoJSON_buildGeomFromPointSrid( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
        break;
      case 18: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 19: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==19);
      case 24: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==24);
      case 25: /* point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==25);
{ yymsp[-18].minor.yy0 = geoJSON_buildGeomFromPointSrid( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
        break;
      case 26: /* short_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON short_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE */
      case 27: /* long_crs ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_NAME GEOJSON_COMMA GEOJSON_PROPS GEOJSON_COLON GEOJSON_OPEN_BRACE GEOJSON_NAME GEOJSON_COLON long_srid GEOJSON_CLOSE_BRACE GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==27);
{ yymsp[-12].minor.yy0 = yymsp[-2].minor.yy0; }
        break;
      case 28: /* point_coordxy ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET */
{ yymsp[-4].minor.yy0 = (void *) geoJSON_point_xy( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 29: /* point_coordxyz ::= GEOJSON_OPEN_BRACKET coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_CLOSE_BRACKET */
{ yymsp[-6].minor.yy0 = (void *) geoJSON_point_xyz( p_data, (double *)yymsp[-5].minor.yy0, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-1].minor.yy0); }
        break;
      case 30: /* coord ::= GEOJSON_NUM */
      case 31: /* short_srid ::= GEOJSON_SHORT_SRID */ yytestcase(yyruleno==31);
      case 32: /* long_srid ::= GEOJSON_LONG_SRID */ yytestcase(yyruleno==32);
{ yylhsminor.yy0 = yymsp[0].minor.yy0; }
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 33: /* extra_pointsxy ::= */
      case 35: /* extra_pointsxyz ::= */ yytestcase(yyruleno==35);
      case 66: /* extra_rings ::= */ yytestcase(yyruleno==66);
      case 69: /* extra_ringsz ::= */ yytestcase(yyruleno==69);
      case 98: /* multilinestring_text2 ::= */ yytestcase(yyruleno==98);
      case 101: /* multilinestring_textz2 ::= */ yytestcase(yyruleno==101);
      case 116: /* multipolygon_text2 ::= */ yytestcase(yyruleno==116);
      case 119: /* multipolygon_textz2 ::= */ yytestcase(yyruleno==119);
      case 136: /* geocoll_text2 ::= */ yytestcase(yyruleno==136);
      case 143: /* geocoll_textz2 ::= */ yytestcase(yyruleno==143);
{ yymsp[1].minor.yy0 = NULL; }
        break;
      case 34: /* extra_pointsxy ::= GEOJSON_COMMA point_coordxy extra_pointsxy */
      case 36: /* extra_pointsxyz ::= GEOJSON_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==36);
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 37: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 43: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==43);
{ yymsp[-8].minor.yy0 = geoJSON_buildGeomFromLinestring( p_data, (gaiaLinestringPtr)yymsp[-1].minor.yy0); }
        break;
      case 38: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 44: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==44);
{ yymsp[-14].minor.yy0 = geoJSON_buildGeomFromLinestring( p_data, (gaiaLinestringPtr)yymsp[-1].minor.yy0); }
        break;
      case 39: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 40: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==40);
      case 45: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==45);
      case 46: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==46);
{ yymsp[-12].minor.yy0 = geoJSON_buildGeomFromLinestringSrid( p_data, (gaiaLinestringPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
        break;
      case 41: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 42: /* linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==42);
      case 47: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==47);
      case 48: /* linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==48);
{ yymsp[-18].minor.yy0 = geoJSON_buildGeomFromLinestringSrid( p_data, (gaiaLinestringPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
        break;
      case 49: /* linestring_text ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) geoJSON_linestring_xy( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 50: /* linestring_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yymsp[-5].minor.yy0 = (void *) geoJSON_linestring_xyz( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 51: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 57: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==57);
{ yymsp[-8].minor.yy0 = geoJSON_buildGeomFromPolygon( p_data, (gaiaPolygonPtr)yymsp[-1].minor.yy0); }
        break;
      case 52: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 58: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==58);
{ yymsp[-14].minor.yy0 = geoJSON_buildGeomFromPolygon( p_data, (gaiaPolygonPtr)yymsp[-1].minor.yy0); }
        break;
      case 53: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 54: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==54);
      case 59: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==59);
      case 60: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==60);
{ yymsp[-12].minor.yy0 = geoJSON_buildGeomFromPolygonSrid( p_data, (gaiaPolygonPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
        break;
      case 55: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 56: /* polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==56);
      case 61: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==61);
      case 62: /* polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==62);
{ yymsp[-18].minor.yy0 = geoJSON_buildGeomFromPolygonSrid( p_data, (gaiaPolygonPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
        break;
      case 63: /* polygon_text ::= GEOJSON_OPEN_BRACKET ring extra_rings GEOJSON_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) geoJSON_polygon_xy(p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 64: /* polygon_textz ::= GEOJSON_OPEN_BRACKET ringz extra_ringsz GEOJSON_CLOSE_BRACKET */
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) geoJSON_polygon_xyz(p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 65: /* ring ::= GEOJSON_OPEN_BRACKET point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy GEOJSON_COMMA point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) geoJSON_ring_xy(p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 67: /* extra_rings ::= GEOJSON_COMMA ring extra_rings */
      case 70: /* extra_ringsz ::= GEOJSON_COMMA ringz extra_ringsz */ yytestcase(yyruleno==70);
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 68: /* ringz ::= GEOJSON_OPEN_BRACKET point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz GEOJSON_COMMA point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yymsp[-9].minor.yy0 = (void *) geoJSON_ring_xyz(p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 71: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 77: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==77);
      case 85: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==85);
      case 91: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==91);
      case 103: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==103);
      case 109: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==109);
      case 121: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==121);
{ yymsp[-8].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 72: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 78: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==78);
      case 86: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==86);
      case 92: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==92);
      case 104: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==104);
      case 110: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==110);
      case 122: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==122);
      case 128: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==128);
{ yymsp[-14].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 73: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 74: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==74);
      case 79: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==79);
      case 80: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==80);
      case 87: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==87);
      case 88: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==88);
      case 93: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==93);
      case 94: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==94);
      case 105: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==105);
      case 106: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==106);
      case 111: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==111);
      case 112: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==112);
      case 123: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==123);
      case 124: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==124);
      case 129: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==129);
      case 130: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==130);
{ yymsp[-12].minor.yy0 = (void *) geoJSON_setSrid((gaiaGeomCollPtr)yymsp[-1].minor.yy0, (int *)yymsp[-5].minor.yy0); }
        break;
      case 75: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */
      case 76: /* multipoint ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==76);
      case 81: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==81);
      case 82: /* multipointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOINT GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipoint_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==82);
      case 89: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==89);
      case 90: /* multilinestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==90);
      case 95: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==95);
      case 96: /* multilinestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTILINESTRING GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multilinestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==96);
      case 107: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==107);
      case 108: /* multipolygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==108);
      case 113: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==113);
      case 114: /* multipolygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_MULTIPOLYGON GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON multipolygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==114);
      case 125: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==125);
      case 126: /* geocoll ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_text GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==126);
      case 131: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON short_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==131);
      case 132: /* geocollz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_GEOMETRYCOLLECTION GEOJSON_COMMA GEOJSON_CRS GEOJSON_COLON long_crs GEOJSON_COMMA GEOJSON_BBOX GEOJSON_COLON GEOJSON_OPEN_BRACKET bbox GEOJSON_CLOSE_BRACKET GEOJSON_COMMA GEOJSON_GEOMS GEOJSON_COLON geocoll_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==132);
{ yymsp[-18].minor.yy0 = (void *) geoJSON_setSrid((gaiaGeomCollPtr)yymsp[-1].minor.yy0, (int *)yymsp[-11].minor.yy0); }
        break;
      case 83: /* multipoint_text ::= GEOJSON_OPEN_BRACKET point_coordxy extra_pointsxy GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) geoJSON_multipoint_xy(p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 84: /* multipoint_textz ::= GEOJSON_OPEN_BRACKET point_coordxyz extra_pointsxyz GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) geoJSON_multipoint_xyz(p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 97: /* multilinestring_text ::= GEOJSON_OPEN_BRACKET linestring_text multilinestring_text2 GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) geoJSON_multilinestring_xy( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 99: /* multilinestring_text2 ::= GEOJSON_COMMA linestring_text multilinestring_text2 */
      case 102: /* multilinestring_textz2 ::= GEOJSON_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==102);
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 100: /* multilinestring_textz ::= GEOJSON_OPEN_BRACKET linestring_textz multilinestring_textz2 GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) geoJSON_multilinestring_xyz(p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 115: /* multipolygon_text ::= GEOJSON_OPEN_BRACKET polygon_text multipolygon_text2 GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) geoJSON_multipolygon_xy(p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 117: /* multipolygon_text2 ::= GEOJSON_COMMA polygon_text multipolygon_text2 */
      case 120: /* multipolygon_textz2 ::= GEOJSON_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==120);
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 118: /* multipolygon_textz ::= GEOJSON_OPEN_BRACKET polygon_textz multipolygon_textz2 GEOJSON_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yymsp[-3].minor.yy0 = (void *) geoJSON_multipolygon_xyz(p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 127: /* geocollz ::= GEOJSON_GEOMETRYCOLLECTION geocoll_textz */
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 133: /* geocoll_text ::= GEOJSON_OPEN_BRACKET coll_point geocoll_text2 GEOJSON_CLOSE_BRACKET */
      case 134: /* geocoll_text ::= GEOJSON_OPEN_BRACKET coll_linestring geocoll_text2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==134);
      case 135: /* geocoll_text ::= GEOJSON_OPEN_BRACKET coll_polygon geocoll_text2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==135);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) geoJSON_geomColl_xy(p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 137: /* geocoll_text2 ::= GEOJSON_COMMA coll_point geocoll_text2 */
      case 138: /* geocoll_text2 ::= GEOJSON_COMMA coll_linestring geocoll_text2 */ yytestcase(yyruleno==138);
      case 139: /* geocoll_text2 ::= GEOJSON_COMMA coll_polygon geocoll_text2 */ yytestcase(yyruleno==139);
      case 144: /* geocoll_textz2 ::= GEOJSON_COMMA coll_pointz geocoll_textz2 */ yytestcase(yyruleno==144);
      case 145: /* geocoll_textz2 ::= GEOJSON_COMMA coll_linestringz geocoll_textz2 */ yytestcase(yyruleno==145);
      case 146: /* geocoll_textz2 ::= GEOJSON_COMMA coll_polygonz geocoll_textz2 */ yytestcase(yyruleno==146);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 140: /* geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_pointz geocoll_textz2 GEOJSON_CLOSE_BRACKET */
      case 141: /* geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_linestringz geocoll_textz2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==141);
      case 142: /* geocoll_textz ::= GEOJSON_OPEN_BRACKET coll_polygonz geocoll_textz2 GEOJSON_CLOSE_BRACKET */ yytestcase(yyruleno==142);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0 = (void *) geoJSON_geomColl_xyz(p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 147: /* coll_point ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxy GEOJSON_CLOSE_BRACE */
      case 148: /* coll_pointz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POINT GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON point_coordxyz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==148);
{ yymsp[-8].minor.yy0 = geoJSON_buildGeomFromPoint(p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 149: /* coll_linestring ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_text GEOJSON_CLOSE_BRACE */
      case 150: /* coll_linestringz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_LINESTRING GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON linestring_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==150);
{ yymsp[-8].minor.yy0 = geoJSON_buildGeomFromLinestring(p_data, (gaiaLinestringPtr)yymsp[-1].minor.yy0); }
        break;
      case 151: /* coll_polygon ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_text GEOJSON_CLOSE_BRACE */
      case 152: /* coll_polygonz ::= GEOJSON_OPEN_BRACE GEOJSON_TYPE GEOJSON_COLON GEOJSON_POLYGON GEOJSON_COMMA GEOJSON_COORDS GEOJSON_COLON polygon_textz GEOJSON_CLOSE_BRACE */ yytestcase(yyruleno==152);
{ yymsp[-8].minor.yy0 = geoJSON_buildGeomFromPolygon(p_data, (gaiaPolygonPtr)yymsp[-1].minor.yy0); }
        break;
      default:
      /* (153) main ::= in */ yytestcase(yyruleno==153);
      /* (154) in ::= */ yytestcase(yyruleno==154);
      /* (155) in ::= in state GEOJSON_NEWLINE */ yytestcase(yyruleno==155);
      /* (156) state ::= program (OPTIMIZED OUT) */ assert(yyruleno!=156);
      /* (157) program ::= geo_text (OPTIMIZED OUT) */ assert(yyruleno!=157);
      /* (158) bbox ::= coord GEOJSON_COMMA coord GEOJSON_COMMA coord GEOJSON_COMMA coord */ yytestcase(yyruleno==158);
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
	p_data->geoJson_parse_error = 1;
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
