/* parsedate.c
**
** $ Revision: 1.2 $
**
**  Originally written by Steven M. Bellovin <smb@research.att.com> while
**  at the University of North Carolina at Chapel Hill.  Later tweaked by
**  a couple of people on Usenet.  Completely overhauled by Rich $alz
**  <rsalz@osf.org> and Jim Berets <jberets@bbn.com> in August, 1990.
**  Further revised (removed obsolete constructs and cleaned up timezone
**  names) in August, 1991, by Rich.  Paul Eggert <eggert@twinsun.com>
**  helped in September, 1992.
**
**  This grammar has six shift/reduce conflicts.
**
**  This code is in the public domain and has no copyright.
*/
/* SUPPRESS 530 *//* Empty body for statement */
/* SUPPRESS 593 on yyerrlab *//* Label was not used */
/* SUPPRESS 593 on yynewstate *//* Label was not used */
/* SUPPRESS 595 on yypvt *//* Automatic variable may be used before set */

#include "../config.h"
#include "ftndlib.h"



#if !defined(HAVE_STRUCT_TM_TM_ZONE) && !defined(_TIMEZONE) && !defined(HAVE_DECLARED_TIMEZONE)
extern time_t timezone;
#endif

#define yylhs		date_yylhs
#define yylen		date_yylen
#define yydefred	date_yydefred
#define yydgoto		date_yydgoto
#define yysindex	date_yysindex
#define yyrindex	date_yyrindex
#define yygindex	date_yygindex
#define yytable		date_yytable
#define yycheck		date_yycheck
#define yyparse		date_parse
#define yylex		date_lex
#define yyerror		date_error

static int date_lex(void);

    /* See the LeapYears table in Convert. */
#define EPOCH		1970
#define END_OF_TIME	2038
    /* Constants for general time calculations. */
#define DST_OFFSET	1
#define SECSPERDAY	(24L * 60L * 60L)
    /* Readability for TABLE stuff. */
#define HOUR(x)		(x * 60)

#define LPAREN		'('
#define RPAREN		')'
#define IS7BIT(x)	((unsigned int)(x) < 0200)

/*
**  Get the number of elements in a fixed-size array, or a pointer just
**  past the end of it.
*/
#define SIZEOF(array)   ((int)(sizeof array / sizeof array[0]))
#define ENDOF(array) (&array[SIZEOF(array)])
#define CTYPE(isXXXXX, c)  ((isascii((c)) && isXXXXX((c))))


typedef char const      *STRING;
typedef char * const    CSTRING;

/*
**  An entry in the lexical lookup table.
*/
typedef struct _TABLE {
    STRING	name;
    int		type;
    time_t	value;
} TABLE;

/*
**  Daylight-savings mode:  on, off, or not yet known.
*/
typedef enum _DSTMODE {
    DSTon, DSToff, DSTmaybe
} DSTMODE;


/*
**  Global variables.  We could get rid of most of them by using a yacc
**  union, but this is more efficient.  (This routine predates the
**  yacc %union construct.)
*/
static char	*yyInput;
static DSTMODE	yyDSTmode;
static int	yyHaveDate;
static int	yyHaveRel;
static int	yyHaveTime;
static time_t	yyTimezone;
static time_t	yyDay;
static time_t	yyHour;
static time_t	yyMinutes;
static time_t	yyMonth;
static time_t	yySeconds;
static time_t	yyYear;
static MERIDIAN	yyMeridian;
static time_t	yyRelMonth;
static time_t	yyRelSeconds;



static void		date_error(char *);


#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		44
#define	YYFLAG		-32768
#define	YYNTBASE	15

#define YYTRANSLATE(x) ((unsigned)(x) <= 265 ? yytranslate[x] : 23)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    14,     2,     2,    13,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    12,     2,     2,
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
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,     9,    11,    13,    16,    21,    26,
    33,    40,    42,    44,    47,    49,    51,    55,    61,    64,
    69,    72,    76,    82,    85,    88,    91,    94,    95
};

static const short yyrhs[] = {    -1,
    15,    16,     0,    17,     0,    17,    18,     0,    20,     0,
    21,     0,    10,    22,     0,    10,    12,    10,    22,     0,
    10,    12,    10,    19,     0,    10,    12,    10,    12,    10,
    22,     0,    10,    12,    10,    12,    10,    19,     0,    11,
     0,     4,     0,    11,    19,     0,    19,     0,     9,     0,
    10,    13,    10,     0,    10,    13,    10,    13,    10,     0,
     6,    10,     0,     6,    10,    14,    10,     0,    10,     6,
     0,    10,     6,    10,     0,     3,    14,    10,     6,    10,
     0,     9,     8,     0,    10,     8,     0,     9,     7,     0,
    10,     7,     0,     0,     5,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   160,   161,   164,   173,   177,   180,   185,   197,   203,   210,
   216,   226,   230,   234,   242,   248,   269,   273,   293,   297,
   308,   312,   323,   336,   339,   342,   345,   350,   353
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","tDAY","tDAYZONE",
"tMERIDIAN","tMONTH","tMONTH_UNIT","tSEC_UNIT","tSNUMBER","tUNUMBER","tZONE",
"':'","'/'","','","spec","item","time","zone","numzone","date","rel","o_merid", NULL
};
#endif

static const short yyr1[] = {     0,
    15,    15,    16,    16,    16,    16,    17,    17,    17,    17,
    17,    18,    18,    18,    18,    19,    20,    20,    20,    20,
    20,    20,    20,    21,    21,    21,    21,    22,    22
};

static const short yyr2[] = {     0,
     0,     2,     1,     2,     1,     1,     2,     4,     4,     6,
     6,     1,     1,     2,     1,     1,     3,     5,     2,     4,
     2,     3,     5,     2,     2,     2,     2,     0,     1
};

static const short yydefact[] = {     1,
     0,     0,     0,     0,    28,     2,     3,     5,     6,     0,
    19,    26,    24,    29,    21,    27,    25,     0,     0,     7,
    13,    16,    12,     4,    15,     0,     0,    22,    28,    17,
    14,     0,    20,     0,     9,     8,     0,    23,    28,    18,
    11,    10,     0,     0
};

static const short yydefgoto[] = {     1,
     6,     7,    24,    25,     8,     9,    20
};

static const short yypact[] = {-32768,
     1,   -11,    11,    20,    12,-32768,     4,-32768,-32768,    13,
    16,-32768,-32768,-32768,    21,-32768,-32768,    22,    23,-32768,
-32768,-32768,     5,-32768,-32768,    28,    25,-32768,    17,    24,
-32768,    26,-32768,    29,-32768,-32768,    30,-32768,     0,-32768,
-32768,-32768,    38,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,   -23,-32768,-32768,   -27
};


#define	YYLAST		40


static const short yytable[] = {    31,
    43,    36,    10,     2,    14,    35,     3,    21,    22,     4,
     5,    42,    22,    22,    23,    41,    14,    15,    16,    17,
    11,    14,    26,    18,    19,    22,    12,    13,    34,    27,
    28,    29,    30,    32,    33,    38,    37,    44,    39,    40
};

static const short yycheck[] = {    23,
     0,    29,    14,     3,     5,    29,     6,     4,     9,     9,
    10,    39,     9,     9,    11,    39,     5,     6,     7,     8,
    10,     5,    10,    12,    13,     9,     7,     8,    12,    14,
    10,    10,    10,     6,    10,    10,    13,     0,    10,    10
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
//  #line 3 "/usr/local/share/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), cyylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&cyylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&cyylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&cyylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&cyylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int		yychar;		/*  the lookahead symbol		*/
CYYSTYPE	cyylval;		/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register CYYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  CYYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  CYYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  CYYSTYPE cyylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  CYYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      CYYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror((char *)"parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (CYYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, cyylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = cyylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 3:
{
	    yyHaveTime++;
#if	defined(lint)
	    /* I am compulsive about lint natterings... */
	    if (yyHaveTime == -1) {
		YYERROR;
	    }
#endif	/* defined(lint) */
	;
    break;}
case 4:
{
	    yyHaveTime++;
	    yyTimezone = yyvsp[0].Number;
	;
    break;}
case 5:
{
	    yyHaveDate++;
	;
    break;}
case 6:
{
	    yyHaveRel = 1;
	;
    break;}
case 7:
{
	    if (yyvsp[-1].Number < 100) {
		yyHour = yyvsp[-1].Number;
		yyMinutes = 0;
	    }
	    else {
		yyHour = yyvsp[-1].Number / 100;
		yyMinutes = yyvsp[-1].Number % 100;
	    }
	    yySeconds = 0;
	    yyMeridian = yyvsp[0].Meridian;
	;
    break;}
case 8:
{
	    yyHour = yyvsp[-3].Number;
	    yyMinutes = yyvsp[-1].Number;
	    yySeconds = 0;
	    yyMeridian = yyvsp[0].Meridian;
	;
    break;}
case 9:
{
	    yyHour = yyvsp[-3].Number;
	    yyMinutes = yyvsp[-1].Number;
	    yyTimezone = yyvsp[0].Number;
	    yyMeridian = MER24;
	    yyDSTmode = DSToff;
	;
    break;}
case 10:
{
	    yyHour = yyvsp[-5].Number;
	    yyMinutes = yyvsp[-3].Number;
	    yySeconds = yyvsp[-1].Number;
	    yyMeridian = yyvsp[0].Meridian;
	;
    break;}
case 11:
{
	    yyHour = yyvsp[-5].Number;
	    yyMinutes = yyvsp[-3].Number;
	    yySeconds = yyvsp[-1].Number;
	    yyTimezone = yyvsp[0].Number;
	    yyMeridian = MER24;
	    yyDSTmode = DSToff;
	;
    break;}
case 12:
{
	    yyval.Number = yyvsp[0].Number;
	    yyDSTmode = DSToff;
	;
    break;}
case 13:
{
	    yyval.Number = yyvsp[0].Number;
	    yyDSTmode = DSTon;
	;
    break;}
case 14:
{
	    /* Only allow "GMT+300" and "GMT-0800" */
	    if (yyvsp[-1].Number != 0) {
		YYABORT;
	    }
	    yyval.Number = yyvsp[0].Number;
	    yyDSTmode = DSToff;
	;
    break;}
case 15:
{
	    yyval.Number = yyvsp[0].Number;
	    yyDSTmode = DSToff;
	;
    break;}
case 16:
{
	    int		i;

	    /* Unix and GMT and numeric timezones -- a little confusing. */
	    if (yyvsp[0].Number < 0) {
		/* Don't work with negative modulus. */
		yyvsp[0].Number = -yyvsp[0].Number;
		if (yyvsp[0].Number > 9999 || (i = yyvsp[0].Number % 100) >= 60) {
		    YYABORT;
		}
		yyval.Number = (yyvsp[0].Number / 100) * 60 + i;
	    }
	    else {
		if (yyvsp[0].Number > 9999 || (i = yyvsp[0].Number % 100) >= 60) {
		    YYABORT;
		}
		yyval.Number = -((yyvsp[0].Number / 100) * 60 + i);
	    }
	;
    break;}
case 17:
{
	    yyMonth = yyvsp[-2].Number;
	    yyDay = yyvsp[0].Number;
	;
    break;}
case 18:
{
	    if (yyvsp[-4].Number > 100) {
			/* assume YYYY/MM/DD format, so need not to add 1900 */
			yyYear = yyvsp[-4].Number;
			yyMonth = yyvsp[-2].Number;
			yyDay = yyvsp[0].Number;
	    }
	    else {
			/* assume MM/DD/YY* format */
			yyMonth = yyvsp[-4].Number;
			yyDay = yyvsp[-2].Number;
			if (yyvsp[0].Number > 100) {
		    	/* assume year is YYYY format, so need not to add 1900 */
		    	yyYear = yyvsp[0].Number;
			} else {
		    	/* assume year is YY format, so need to add 1900 */
		    	yyYear = yyvsp[0].Number + 1900;
			}
	    }
	;
    break;}
case 19:
{
	    yyMonth = yyvsp[-1].Number;
	    yyDay = yyvsp[0].Number;
	;
    break;}
case 20:
{
	    yyMonth = yyvsp[-3].Number;
	    yyDay = yyvsp[-2].Number;
	    if (yyvsp[0].Number > 100) {
		/* assume year is YYYY format, so need not to add 1900 */
		yyYear = yyvsp[0].Number;
	    } else {
		/* assume year is YY format, so need to add 1900 */
		yyYear = yyvsp[0].Number + 1900;
	    }
	;
    break;}
case 21:
{
	    yyDay = yyvsp[-1].Number;
	    yyMonth = yyvsp[0].Number;
	;
    break;}
case 22:
{
	    yyDay = yyvsp[-2].Number;
	    yyMonth = yyvsp[-1].Number;
	    if (yyvsp[0].Number > 100) {
		/* assume year is YYYY format, so need not to add 1900 */
		yyYear = yyvsp[0].Number;
	    } else {
		/* assume year is YY format, so need to add 1900 */
		yyYear = yyvsp[0].Number + 1900;
	    }
	;
    break;}
case 23:
{
	    yyDay = yyvsp[-2].Number;
	    yyMonth = yyvsp[-1].Number;
	    if (yyvsp[0].Number > 100) {
		/* assume year is YYYY format, so need not to add 1900 */
		yyYear = yyvsp[0].Number;
	    } else {
		/* assume year is YY format, so need to add 1900 */
		yyYear = yyvsp[0].Number + 1900;
	    }
	;
    break;}
case 24:
{
	    yyRelSeconds += yyvsp[-1].Number * yyvsp[0].Number;
	;
    break;}
case 25:
{
	    yyRelSeconds += yyvsp[-1].Number * yyvsp[0].Number;
	;
    break;}
case 26:
{
	    yyRelMonth += yyvsp[-1].Number * yyvsp[0].Number;
	;
    break;}
case 27:
{
	    yyRelMonth += yyvsp[-1].Number * yyvsp[0].Number;
	;
    break;}
case 28:
{
	    yyval.Meridian = MER24;
	;
    break;}
case 29:
{
	    yyval.Meridian = yyvsp[0].Meridian;
	;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror((char *)"parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = cyylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}


/* Month and day table. */
static TABLE	MonthDayTable[] = {
    { "january",	tMONTH,  1 },
    { "february",	tMONTH,  2 },
    { "march",		tMONTH,  3 },
    { "april",		tMONTH,  4 },
    { "may",		tMONTH,  5 },
    { "june",		tMONTH,  6 },
    { "july",		tMONTH,  7 },
    { "august",		tMONTH,  8 },
    { "september",	tMONTH,  9 },
    { "october",	tMONTH, 10 },
    { "november",	tMONTH, 11 },
    { "december",	tMONTH, 12 },
	/* The value of the day isn't used... */
    { "sunday",		tDAY, 0 },
    { "monday",		tDAY, 0 },
    { "tuesday",	tDAY, 0 },
    { "wednesday",	tDAY, 0 },
    { "thursday",	tDAY, 0 },
    { "friday",		tDAY, 0 },
    { "saturday",	tDAY, 0 },
};

/* Time units table. */
static TABLE	UnitsTable[] = {
    { "year",		tMONTH_UNIT,	12 },
    { "month",		tMONTH_UNIT,	1 },
    { "week",		tSEC_UNIT,	7 * 24 * 60 * 60 },
    { "day",		tSEC_UNIT,	1 * 24 * 60 * 60 },
    { "hour",		tSEC_UNIT,	60 * 60 },
    { "minute",		tSEC_UNIT,	60 },
    { "min",		tSEC_UNIT,	60 },
    { "second",		tSEC_UNIT,	1 },
    { "sec",		tSEC_UNIT,	1 },
};

/* Timezone table. */
static TABLE	TimezoneTable[] = {
    { "gmt",	tZONE,     HOUR( 0) },	/* Greenwich Mean */
    { "ut",	tZONE,     HOUR( 0) },	/* Universal */
    { "utc",	tZONE,     HOUR( 0) },	/* Universal Coordinated */
    { "cut",	tZONE,     HOUR( 0) },	/* Coordinated Universal */
    { "z",	tZONE,     HOUR( 0) },	/* Greenwich Mean */
    { "wet",	tZONE,     HOUR( 0) },	/* Western European */
    { "bst",	tDAYZONE,  HOUR( 0) },	/* British Summer */
    { "nst",	tZONE,     HOUR(3)+30 }, /* Newfoundland Standard */
    { "ndt",	tDAYZONE,  HOUR(3)+30 }, /* Newfoundland Daylight */
    { "ast",	tZONE,     HOUR( 4) },	/* Atlantic Standard */
    { "adt",	tDAYZONE,  HOUR( 4) },	/* Atlantic Daylight */
    { "est",	tZONE,     HOUR( 5) },	/* Eastern Standard */
    { "edt",	tDAYZONE,  HOUR( 5) },	/* Eastern Daylight */
    { "cst",	tZONE,     HOUR( 6) },	/* Central Standard */
    { "cdt",	tDAYZONE,  HOUR( 6) },	/* Central Daylight */
    { "mst",	tZONE,     HOUR( 7) },	/* Mountain Standard */
    { "mdt",	tDAYZONE,  HOUR( 7) },	/* Mountain Daylight */
    { "pst",	tZONE,     HOUR( 8) },	/* Pacific Standard */
    { "pdt",	tDAYZONE,  HOUR( 8) },	/* Pacific Daylight */
    { "yst",	tZONE,     HOUR( 9) },	/* Yukon Standard */
    { "ydt",	tDAYZONE,  HOUR( 9) },	/* Yukon Daylight */
    { "akst",	tZONE,     HOUR( 9) },	/* Alaska Standard */
    { "akdt",	tDAYZONE,  HOUR( 9) },	/* Alaska Daylight */
    { "hst",	tZONE,     HOUR(10) },	/* Hawaii Standard */
    { "hast",	tZONE,     HOUR(10) },	/* Hawaii-Aleutian Standard */
    { "hadt",	tDAYZONE,  HOUR(10) },	/* Hawaii-Aleutian Daylight */
    { "ces",	tDAYZONE,  -HOUR(1) },	/* Central European Summer */
    { "cest",	tDAYZONE,  -HOUR(1) },	/* Central European Summer */
    { "mez",	tZONE,     -HOUR(1) },	/* Middle European */
    { "mezt",	tDAYZONE,  -HOUR(1) },	/* Middle European Summer */
    { "cet",	tZONE,     -HOUR(1) },	/* Central European */
    { "met",	tZONE,     -HOUR(1) },	/* Middle European */
    { "eet",	tZONE,     -HOUR(2) },	/* Eastern Europe */
    { "msk",	tZONE,     -HOUR(3) },	/* Moscow Winter */
    { "msd",	tDAYZONE,  -HOUR(3) },	/* Moscow Summer */
    { "wast",	tZONE,     -HOUR(8) },	/* West Australian Standard */
    { "wadt",	tDAYZONE,  -HOUR(8) },	/* West Australian Daylight */
    { "hkt",	tZONE,     -HOUR(8) },	/* Hong Kong */
    { "cct",	tZONE,     -HOUR(8) },	/* China Coast */
    { "jst",	tZONE,     -HOUR(9) },	/* Japan Standard */
    { "kst",	tZONE,     -HOUR(9) },	/* Korean Standard */
    { "kdt",	tZONE,     -HOUR(9) },	/* Korean Daylight */
    { "cast",	tZONE,     -(HOUR(9)+30) }, /* Central Australian Standard */
    { "cadt",	tDAYZONE,  -(HOUR(9)+30) }, /* Central Australian Daylight */
    { "east",	tZONE,     -HOUR(10) },	/* Eastern Australian Standard */
    { "eadt",	tDAYZONE,  -HOUR(10) },	/* Eastern Australian Daylight */
    { "nzst",	tZONE,     -HOUR(12) },	/* New Zealand Standard */
    { "nzdt",	tDAYZONE,  -HOUR(12) },	/* New Zealand Daylight */

    /* For completeness we include the following entries. */
#if	0

    /* Duplicate names.  Either they conflict with a zone listed above
     * (which is either more likely to be seen or just been in circulation
     * longer), or they conflict with another zone in this section and
     * we could not reasonably choose one over the other. */
    { "fst",	tZONE,     HOUR( 2) },	/* Fernando De Noronha Standard */
    { "fdt",	tDAYZONE,  HOUR( 2) },	/* Fernando De Noronha Daylight */
    { "bst",	tZONE,     HOUR( 3) },	/* Brazil Standard */
    { "est",	tZONE,     HOUR( 3) },	/* Eastern Standard (Brazil) */
    { "edt",	tDAYZONE,  HOUR( 3) },	/* Eastern Daylight (Brazil) */
    { "wst",	tZONE,     HOUR( 4) },	/* Western Standard (Brazil) */
    { "wdt",	tDAYZONE,  HOUR( 4) },	/* Western Daylight (Brazil) */
    { "cst",	tZONE,     HOUR( 5) },	/* Chile Standard */
    { "cdt",	tDAYZONE,  HOUR( 5) },	/* Chile Daylight */
    { "ast",	tZONE,     HOUR( 5) },	/* Acre Standard */
    { "adt",	tDAYZONE,  HOUR( 5) },	/* Acre Daylight */
    { "cst",	tZONE,     HOUR( 5) },	/* Cuba Standard */
    { "cdt",	tDAYZONE,  HOUR( 5) },	/* Cuba Daylight */
    { "est",	tZONE,     HOUR( 6) },	/* Easter Island Standard */
    { "edt",	tDAYZONE,  HOUR( 6) },	/* Easter Island Daylight */
    { "sst",	tZONE,     HOUR(11) },	/* Samoa Standard */
    { "ist",	tZONE,     -HOUR(2) },	/* Israel Standard */
    { "idt",	tDAYZONE,  -HOUR(2) },	/* Israel Daylight */
    { "idt",	tDAYZONE,  -(HOUR(3)+30) }, /* Iran Daylight */
    { "ist",	tZONE,     -(HOUR(3)+30) }, /* Iran Standard */
    { "cst",	 tZONE,     -HOUR(8) },	/* China Standard */
    { "cdt",	 tDAYZONE,  -HOUR(8) },	/* China Daylight */
    { "sst",	 tZONE,     -HOUR(8) },	/* Singapore Standard */

    /* Dubious (e.g., not in Olson's TIMEZONE package) or obsolete. */
    { "gst",	tZONE,     HOUR( 3) },	/* Greenland Standard */
    { "wat",	tZONE,     -HOUR(1) },	/* West Africa */
    { "at",	tZONE,     HOUR( 2) },	/* Azores */
    { "gst",	tZONE,     -HOUR(10) },	/* Guam Standard */
    { "nft",	tZONE,     HOUR(3)+30 }, /* Newfoundland */
    { "idlw",	tZONE,     HOUR(12) },	/* International Date Line West */
    { "mewt",	tZONE,     -HOUR(1) },	/* Middle European Winter */
    { "mest",	tDAYZONE,  -HOUR(1) },	/* Middle European Summer */
    { "swt",	tZONE,     -HOUR(1) },	/* Swedish Winter */
    { "sst",	tDAYZONE,  -HOUR(1) },	/* Swedish Summer */
    { "fwt",	tZONE,     -HOUR(1) },	/* French Winter */
    { "fst",	tDAYZONE,  -HOUR(1) },	/* French Summer */
    { "bt",	tZONE,     -HOUR(3) },	/* Baghdad */
    { "it",	tZONE,     -(HOUR(3)+30) }, /* Iran */
    { "zp4",	tZONE,     -HOUR(4) },	/* USSR Zone 3 */
    { "zp5",	tZONE,     -HOUR(5) },	/* USSR Zone 4 */
    { "ist",	tZONE,     -(HOUR(5)+30) }, /* Indian Standard */
    { "zp6",	tZONE,     -HOUR(6) },	/* USSR Zone 5 */
    { "nst",	tZONE,     -HOUR(7) },	/* North Sumatra */
    { "sst",	tZONE,     -HOUR(7) },	/* South Sumatra */
    { "jt",	tZONE,     -(HOUR(7)+30) }, /* Java (3pm in Cronusland!) */
    { "nzt",	tZONE,     -HOUR(12) },	/* New Zealand */
    { "idle",	tZONE,     -HOUR(12) },	/* International Date Line East */
    { "cat",	tZONE,     HOUR(10) },	/* -- expired 1967 */
    { "nt",	tZONE,     HOUR(11) },	/* -- expired 1967 */
    { "ahst",	tZONE,     HOUR(10) },	/* -- expired 1983 */
    { "hdt",	tDAYZONE,  HOUR(10) },	/* -- expired 1986 */
#endif	/* 0 */
};



/* ARGSUSED */
static void
date_error(s)
    char	*s;
{
    /* NOTREACHED */
}

int GetTimeInfo(TIMEINFO *Now)
{
    static int32_t	NextHour;
    static int		LastTzone;
    struct tm		*tm;
    int			secondsUntilNextHour;
#if	defined(HAVE_GETTIMEOFDAY)
    struct timeval	tv;
#endif	/* defined(HAVE_GETTIMEOFDAY) */
#if	!defined(HAVE_STRUCT_TM_TM_ZONE)
    struct tm		local;
    struct tm		gmt;
#endif	/* !defined(HAVE_STRUCT_TM_TM_ZONE) */

    /* Get the basic time. */
#if	defined(HAVE_GETTIMEOFDAY)
    if (gettimeofday(&tv, (struct timezone *)NULL) == -1)
	return -1;
    Now->time = tv.tv_sec;
    Now->usec = tv.tv_usec;
#else
    /* Can't check for -1 since that might be a time, I guess. */
    Now->time = time(NULL);
    Now->usec = 0;
#endif	/* defined(HAVE_GETTIMEOFDAY) */

    /* Now get the timezone if the last time < HH:00:00 <= now for some HH.  */
    if (NextHour <= Now->time) {
	if ((tm = localtime(&Now->time)) == NULL)
	    return -1;
	secondsUntilNextHour = 60 * (60 - tm->tm_min) - tm->tm_sec;
#if	!defined(HAVE_STRUCT_TM_TM_ZONE)
	/* To get the timezone, compare localtime with GMT. */
	local = *tm;
	if ((tm = gmtime(&Now->time)) == NULL)
	    return -1;
	gmt = *tm;

	/* Assume we are never more than 24 hours away. */
	LastTzone = gmt.tm_yday - local.tm_yday;
	if (LastTzone > 1)
	    LastTzone = -24;
	else if (LastTzone < -1)
	    LastTzone = 24;
	else
	    LastTzone *= 24;

	/* Scale in the hours and minutes; ignore seconds. */
	LastTzone += gmt.tm_hour - local.tm_hour;
	LastTzone *= 60;
	LastTzone += gmt.tm_min - local.tm_min;
#else
	LastTzone =  (0 - tm->tm_gmtoff) / 60;
#endif	/* defined(HAVE_STRUCT_TM_TM_ZONE) */
	NextHour = Now->time + secondsUntilNextHour;
    }
    Now->tzone = LastTzone;
    return 0;
}



static time_t ToSeconds(time_t Hours, time_t Minutes, time_t Seconds, MERIDIAN Meridian)
{
    if (Minutes < 0 || Minutes > 59 || Seconds < 0 || Seconds > 61)
	return -1;
    if (Meridian == MER24) {
	if (Hours < 0 || Hours > 23)
	    return -1;
    }
    else {
	if (Hours < 1 || Hours > 12)
	    return -1;
	if (Hours == 12)
	    Hours = 0;
	if (Meridian == MERpm)
	    Hours += 12;
    }
    return (Hours * 60L + Minutes) * 60L + Seconds;
}


static time_t
Convert(time_t Month, time_t Day, time_t Year, time_t Hours, time_t Minutes, time_t Seconds, MERIDIAN Meridian, DSTMODE dst)
{
    static int	DaysNormal[13] = {
	0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    static int	DaysLeap[13] = {
	0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    static int	LeapYears[] = {
	1972, 1976, 1980, 1984, 1988, 1992, 1996,
	2000, 2004, 2008, 2012, 2016, 2020, 2024, 2028, 2032, 2036
    };
    int	                *yp;
    int	                *mp;
    time_t	        Julian;
    int	                i;
    time_t		tod;

    /* Year should not be passed as a relative value, but absolute one.
       so this should not happen, but just ensure it */
    if (Year < 0)
	Year = -Year;
    if (Year < 100)
	Year += 1900;
    if (Year < EPOCH)
	Year += 100;
    for (mp = DaysNormal, yp = LeapYears; yp < ENDOF(LeapYears); yp++)
	if (Year == *yp) {
	    mp = DaysLeap;
	    break;
	}
    if (Year < EPOCH || Year > END_OF_TIME
     || Month < 1 || Month > 12
     /* NOSTRICT *//* conversion from long may lose accuracy */
     || Day < 1 || Day > mp[(int)Month])
	return -1;

    Julian = Day - 1 + (Year - EPOCH) * 365;
    for (yp = LeapYears; yp < ENDOF(LeapYears); yp++, Julian++)
	if (Year <= *yp)
	    break;
    for (i = 1; i < Month; i++)
	Julian += *++mp;
    Julian *= SECSPERDAY;
    Julian += yyTimezone * 60L;
    if ((tod = ToSeconds(Hours, Minutes, Seconds, Meridian)) < 0)
	return -1;
    Julian += tod;
    tod = Julian;
    if (dst == DSTon || (dst == DSTmaybe && localtime(&tod)->tm_isdst))
	Julian -= DST_OFFSET * 60 * 60;
    return Julian;
}


static time_t DSTcorrect(time_t Start, time_t Future)
{
    time_t	StartDay;
    time_t	FutureDay;

    StartDay = (localtime(&Start)->tm_hour + 1) % 24;
    FutureDay = (localtime(&Future)->tm_hour + 1) % 24;
    return (Future - Start) + (StartDay - FutureDay) * DST_OFFSET * 60 * 60;
}


static time_t RelativeMonth(time_t Start, time_t RelMonth)
{
    struct tm	*tm;
    time_t	Month;
    time_t	Year;

    tm = localtime(&Start);
    Month = 12 * tm->tm_year + tm->tm_mon + RelMonth;
    Year = Month / 12;
    Year += 1900;
    Month = Month % 12 + 1;
    return DSTcorrect(Start,
	    Convert(Month, (time_t)tm->tm_mday, Year,
		(time_t)tm->tm_hour, (time_t)tm->tm_min, (time_t)tm->tm_sec,
		MER24, DSTmaybe));
}


static int LookupWord(char *buff, int length)
{
    char	        *p;
    STRING	        q;
    TABLE	        *tp;
    int	                c;

    p = buff;
    c = p[0];

    /* See if we have an abbreviation for a month. */
    if (length == 3 || (length == 4 && p[3] == '.'))
	for (tp = MonthDayTable; tp < ENDOF(MonthDayTable); tp++) {
	    q = tp->name;
	    if (c == q[0] && p[1] == q[1] && p[2] == q[2]) {
		cyylval.Number = tp->value;
		return tp->type;
	    }
	}
    else
	for (tp = MonthDayTable; tp < ENDOF(MonthDayTable); tp++)
	    if (c == tp->name[0] && strcmp(p, tp->name) == 0) {
		cyylval.Number = tp->value;
		return tp->type;
	    }

    /* Try for a timezone. */
    for (tp = TimezoneTable; tp < ENDOF(TimezoneTable); tp++)
	if (c == tp->name[0] && p[1] == tp->name[1]
	 && strcmp(p, tp->name) == 0) {
	    cyylval.Number = tp->value;
	    return tp->type;
	}

    /* Try the units table. */
    for (tp = UnitsTable; tp < ENDOF(UnitsTable); tp++)
	if (c == tp->name[0] && strcmp(p, tp->name) == 0) {
	    cyylval.Number = tp->value;
	    return tp->type;
	}

    /* Strip off any plural and try the units table again. */
    if (--length > 0 && p[length] == 's') {
	p[length] = '\0';
	for (tp = UnitsTable; tp < ENDOF(UnitsTable); tp++)
	    if (c == tp->name[0] && strcmp(p, tp->name) == 0) {
		p[length] = 's';
		cyylval.Number = tp->value;
		return tp->type;
	    }
	p[length] = 's';
    }
    length++;

    /* Drop out any periods. */
    for (p = buff, q = (STRING)buff; *q; q++)
	if (*q != '.')
	    *p++ = *q;
    *p = '\0';

    /* Try the meridians. */
    if (buff[1] == 'm' && buff[2] == '\0') {
	if (buff[0] == 'a') {
	    cyylval.Meridian = MERam;
	    return tMERIDIAN;
	}
	if (buff[0] == 'p') {
	    cyylval.Meridian = MERpm;
	    return tMERIDIAN;
	}
    }

    /* If we saw any periods, try the timezones again. */
    if (p - buff != length) {
	c = buff[0];
	for (p = buff, tp = TimezoneTable; tp < ENDOF(TimezoneTable); tp++)
	    if (c == tp->name[0] && p[1] == tp->name[1]
	    && strcmp(p, tp->name) == 0) {
		cyylval.Number = tp->value;
		return tp->type;
	    }
    }

    /* Unknown word -- assume GMT timezone. */
    cyylval.Number = 0;
    return tZONE;
}


static int date_lex(void)
{
    char	        c;
    char	        *p;
    char		buff[20];
    int	                sign;
    int	                i;
    int	                nesting;

    for ( ; ; ) {
	/* Get first character after the whitespace. */
	for ( ; ; ) {
	    while (CTYPE(isspace, (int)*yyInput))
		yyInput++;
	    c = *yyInput;

	    /* Ignore RFC 822 comments, typically time zone names. */
	    if (c != LPAREN)
		break;
	    for (nesting = 1; (c = *++yyInput) != RPAREN || --nesting; )
		if (c == LPAREN)
		    nesting++;
		else if (!IS7BIT(c) || c == '\0' || c == '\r'
		     || (c == '\\' && ((c = *++yyInput) == '\0' || !IS7BIT(c))))
		    /* Lexical error: bad comment. */
		    return '?';
	    yyInput++;
	}

	/* A number? */
	if (CTYPE(isdigit, (int)c) || c == '-' || c == '+') {
	    if (c == '-' || c == '+') {
		sign = c == '-' ? -1 : 1;
		yyInput++;
		if (!CTYPE(isdigit, (int)*yyInput))
		    /* Skip the plus or minus sign. */
		    continue;
	    }
	    else
		sign = 0;
	    for (i = 0; (c = *yyInput++) != '\0' && CTYPE(isdigit, (int)c); )
		i = 10 * i + c - '0';
	    yyInput--;
	    cyylval.Number = sign < 0 ? -i : i;
	    return sign ? tSNUMBER : tUNUMBER;
	}

	/* A word? */
	if (CTYPE(isalpha, (int)c)) {
	    for (p = buff; (c = *yyInput++) == '.' || CTYPE(isalpha, (int)c); )
		if (p < &buff[sizeof buff - 1])
		    *p++ = CTYPE(isupper, (int)c) ? tolower(c) : c;
	    *p = '\0';
	    yyInput--;
	    return LookupWord(buff, p - buff);
	}

	return *yyInput++;
    }
}


time_t parsedate(char *p, TIMEINFO *now)
{
    struct tm		*tm;
    TIMEINFO		ti;
    time_t		Start;

    yyInput = p;
    if (now == NULL) {
		now = &ti;
		(void)GetTimeInfo(&ti);
    }

    tm = localtime(&now->time);
    yyYear = tm->tm_year + 1900;
    yyMonth = tm->tm_mon + 1;
    yyDay = tm->tm_mday;
#ifdef HAVE_STRUCT_TM_TM_ZONE
    yyTimezone = tm->tm_gmtoff/60;
#else
	yyTimezone = timezone/60;
#endif
    yyDSTmode = DSTmaybe;
    yyHour = 0;
    yyMinutes = 0;
    yySeconds = 0;
    yyMeridian = MER24;
    yyRelSeconds = 0;
    yyRelMonth = 0;
    yyHaveDate = 0;
    yyHaveRel = 0;
    yyHaveTime = 0;

    if (date_parse() || yyHaveTime > 1 || yyHaveDate > 1)
	return -1;

    if (yyHaveDate || yyHaveTime) {
	Start = Convert(yyMonth, yyDay, yyYear, yyHour, yyMinutes, yySeconds,
		    yyMeridian, yyDSTmode);
	if (Start < 0)
	    return -1;
    }
    else {
	Start = now->time;
	if (!yyHaveRel)
	    Start -= (tm->tm_hour * 60L + tm->tm_min) * 60L + tm->tm_sec;
    }

    Start += yyRelSeconds;
    if (yyRelMonth)
	Start += RelativeMonth(Start, yyRelMonth);

    /* Have to do *something* with a legitimate -1 so it's distinguishable
     * from the error return value.  (Alternately could set errno on error.) */
    return Start == -1 ? 0 : Start;
}
