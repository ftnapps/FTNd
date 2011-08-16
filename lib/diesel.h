/* $Id: diesel.h,v 1.7 2007/02/26 21:02:30 mbse Exp $ */

#ifndef	_DIESEL_H
#define	_DIESEL_H

#define UNIXTENSIONS
#define VARIABLES


#define FALSE	    0
#define TRUE	    1
#define DIAGNOSTIC  2

#define EOS	'\0'

#define V	(void)

/*  Globals exported  */

#ifdef DIESEL_TRACE
int tracing = TRUE;		      /* Trace macro evalution */
#endif

/*  Local variables.  */

#define MAXARGS     10		      /* Maximum arguments to a macro */
#define MAXSTR	    19200 	      /* Maximum string length */ /* Was 2560 */ /* Size full UTF-8 code filesdesc */
#define MAXDEPTH    32		      /* Maximum recursion depth for eval */

#define MACROCHAR   '@' 	      /* Macro trigger character */
#define ARGOPEN     '(' 	      /* Argument open bracket */
#define ARGCLOSE    ')' 	      /* Argument close bracket */
#define ARGSEP	    ',' 	      /* Argument separator character */
#define QUOTE	    '"' 	      /* Literal string quote character */
#define CASEINS 		      /* Case-insensitive function names */

#define STRLIMIT    (MAXSTR - 20)     /* String output length limit */
#define OverFlow    " @(++)"	      /* Glyph indicating string overflow */

#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))
#define FUZZEQ(a, b) ((((a) < (b)) ? ((b) - (a)) : ((a) - (b))) < 1E-10)


int diesel(char *, char *);


/*
 * MBSE BBS specific functions
 */
char *ParseMacro( const char *, int * );
void MacroVars( const char *, const char *, ... );
void MacroClear(void);
void html_massage(char *, char *, size_t);
FILE *OpenMacro(const char *, int, int);

#endif

