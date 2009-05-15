/*****************************************************************************
 *
 * $Id: diesel.c,v 1.13 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: TURBODIESEL Macro language
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

/*

                            T U R B O D I E S E L
         Template-based Uncomplicated Report-Building Oriented Dumb 
             Interpretively Evaluated String Expression Language

This is a  modified version of diesel  language.  Diesel is a  interpreted 
macro language, used in AutoCAD and released to public domain by AutoDesk.

Modified  version by Redy Rodriguez,  for use in mbsebbs.  Original diesel
language can be found at http://www.fournilab.ch/diesel.
 
This  "Dumb  Interpretively  Executed  String  Expression Language" is the
kernel of a macro  language  you  can  customise  by  adding  C  code  and
embedding it into your program.

It is short, written in portable C, and is  readily  integrated  into  any
program.  It is useful primarily to programs which need a very rudimentary
macro expansion facility without the complexity of a full language such as
Lisp or FORTH.

DIESEL  copies  its  input directly to the output until a macro character,
"@" or quoted string is  encountered.   Quoted  strings  may  be  used  to
suppress  evaluation  of  sequences of characters which would otherwise be
interpreted as macros.  Quote marks may be included in quoted  strings  by
two adjacent quote marks.  For example:

    "@(if,1,True,False)="""@(if,1,True,False)""""

Status  retrieval,  computation,  and  display  are  performed  by  DIESEL
functions.   The  available  functions  are  as   follows.    User-defined
functions are not implemented; what you see is all you've got.  Naturally,
if you embed DIESEL in your application, you'll add functions that provide
access  to  information  and  actions  within  your own program.  DIESEL's
arithmetic functions accept either floating point  or  integer  arguments,
and perform all calculations in floating point.

TURBODIESEL facilities
----------------------

If a line begin with # then will be not evaluated, and any output is done.

If a line begin with @! any output is done, but evaluation is performed.

If a line begin with @{<expresion>} produces  output only if expression is
TRUE (Any non-zero numeric value).

To easily format output, you can use one-char variable names as follow:

   @A will be replaced by result of evaluate @(GETVAR,A).

   @A_____ will be replaced by result of evaluate @(GETVAR,A) truncated or
padded with spaces to complete same lenght of '@A_____' (7 in that case).  

   You can use > or < to especify alignement rigth or left:
   
   @A_____>         @A_____< 
   

TURBODIESEL String Functions
----------------------------

    @(+,<val1>,<val2>,...<valn>)
	The  sum  of  the  numbers <val1>, <val2>, ...<valn> is returned.

    @(-,<val1>,<val2>,...<valn>)
	The  result  of subtracting the numbers <val2> through <valn> from
	<val1> is returned.

    @(*,<val1>,<val2>,...<valn>)
	The result of multiplying the numbers  <val1>,<val2>,...<valn>	is
	returned.

    @(/,<val1>,<val2>,...<valn>)
	The  result of dividing the number <val1> by <val2>,...  <valn> is
	returned.

    @(=,<val1>,<val2>)
	If the	numbers  <val1>  and  <val2>  are  equal  1  is  returned,
	otherwise 0 is returned.

    @(<,<val1>,<val2>)
	If  the number <val1> is less than <val2> 1 is returned, otherwise
	0 is returned.

    @(>,<val1>,<val2>)
	If the number  <val1>  is  greater  than  <val2>  1  is  returned,
	otherwise 0 is returned.

    @(!=,<val1>,<val2>)
	If  the  numbers  <val1>  and  <val2> are not equal 1 is returned,
	otherwise 0 is returned.

    @(<=,<val1>,<val2>)
	If the number <val1>  is  less	than  or  equal  to  <val2>  1	is
	returned, otherwise 0 is returned.

    @(>=,<val1>,<val2>)
	If  the  number  <val1>  is  greater  than or equal to <val2> 1 is
	returned, otherwise 0 is returned.

    @(AND,<val1>,<val2>,...<valn>)
	The bitwise logical AND of the integers <val1> through	<valn>	is
	returned.

    @(EQ,<val1>,<val2>)
	If the strings <val1> and <val2>  are  identical  1  is  returned,
	otherwise 0.

    @(EVAL,<str>)
	The  string <str> is passed to the DIESEL evaluator and the result
	of evaluating it is returned.

    @(FIX,<value>)
	The real number <value> is truncated to an integer  by	discarding
	any fractional part.

    @(IF,<expr>,<dotrue>,<dofalse>)
	If  <expr>  is	nonzero,  <dotrue>  is	evaluated  and	 returned.
	Otherwise,  <dofalse>  is  evaluated  and returned.  Note that the
	branch not chosen by <expr> is not evaluated.

    @(INDEX,<which>,<string>)
	<string> is assumed to contain one or more values delimited by the
	macro argument separator character, comma.  <which> selects one of
	these values to be extracted, with the first item  numbered  zero.

*   @(LOWER,<string>)
	The  <string> is returned converted to lower case according to the
	rules of the current locale.

    @(NTH,<which>,<arg0>,<arg1>,<argN>)
	Evaluates  and	returns  the  argument	selected  by  <which>.	If
	<which> is 0, <arg0> is returned, and so on.  Note the	difference
	between  @(NTH)  and  @(INDEX);  @(NTH) returns one of a series of
	arguments to the function while @(INDEX) extracts a value  from  a
	comma-delimited string passed as a single argument.  Arguments not
	selected by <which> are not evaluated.

    @(OR,<val1>,<val2>,...<valn>)
	The  bitwise  logical  OR of the integers <val1> through <valn> is
	returned.

*   @(STRCMP,<str1>,<str2>)
	Compare strings and returns -1 if <str1> is less than <Str2>, 0 if 
	both are equals, or 1 if <str1> is greater than <str2> .

    @(STRFILL,<string>,<ncopies>)
	Returns the result of concatenating <ncopies> of <string>.

    @(STRLEN,<string>)
	Returns the length of <string> in characters.

*   @(STRSTR,<str1>,<str2>) 
	Find first apparition of <str2> in <str1>, and return the position
	or 0 if not found.

    @(SUBSTR,<string>,<start>,<length>)
	Returns  the  substring  of <string> starting at character <start>
	and extending for <length> characters.	Characters in  the  string
	are numbered from 1.  If <length> is omitted, the entire remaining
	length of the string is returned.

    @(UPPER,<string>)
	The  <string> is returned converted to upper case according to the
	rules of the current locale.

    @(XOR,<val1>,<val2>,...<valn>)
	The bitwise logical XOR of the integers <val1> through	<valn>	is
	returned.

Variable Extensions
-------------------

The  base-line DIESEL includes no user-defined variables.  This allows
DIESEL to avoid allocating any local memory  and  renders  it  totally
reentrant.   If you compile DIESEL with the tag VARIABLES defined, the
following additional functions are  included  which  provide  variable
definition  and  access.   Note that these functions call malloc() and
strdup() and thus consume heap storage.

Variable names are case sensitive. 

If you want easily format output you  must use one-char variable names
then you  can format output as @V_____, @X_____< or @k___>. See above.

    @(GETVAR,varname)
	Returns  the value stored in <varname>.  If no  variable  with  
	the name <varname> exists, a bad argument error is reported.

    @(SETVAR,varname,value)
    	Stores  the  string  <value>  into  <varname>.  If no variable 
	called <varname> exists, a new variable is created.

*   @(CLEAR)
        Clear all variables.

Unix Extensions
---------------

If you compile DIESEL with the tag UNIXTENSIONS defined, the following
additional functions will be available:

@(GETENV,varname)
    Returns  the  variable <varname> from the environment.  If no such
    variable is defined, returns the null string.

@(TIME)
    Returns the current time in Unix fashion, as the number of seconds
    elapsed since 00:00:00 GMT January 1, 1970.

@(EDTIME,<time>,<picture>)
    Edit  the  Unix  time <time> to format <picture>.  If <time> is 0,
    the current date and time is edited (this is  just  shorthand  for
    the equivalent "@(EDTIME,@(TIME),<picture>)".).

    Assume the date is: Thursday, 2 September 1993 4:53:17

    Format phrases:
        D               2
        DD              02
        DDD             Thu
        DDDD            Thursday
        M               9
        MO              09
        MON             Sep
        MONTH           September
        YY              93
        YYYY            1993
        H               4
        HH              04
        MM              53
        SS              17
        AM/PM           AM
        am/pm           am
        A/P             A
        a/p             a

    If  any  of the "AM/PM" phrases appear in the picture, the "H" and
    "HH" phrases will edit the time according to  the  12  hour  civil
    clock  (12:00-12:59-1:00-11:59)  instead  of  the  24  hour  clock
    (00:00-23:59).

TURBODIESEL Mechanics
---------------------

Generally, if you mess something up in a DIESEL expression it's pretty
obvious  what  went  wrong.   DIESEL embeds an error indication in the
output stream depending on the nature of the error:

    @?              Syntax error (usually a missing right  parenthesis
                    or runaway string).

    @(<func>,??)    Incorrect arguments to <func>.

    @(<func>)??     Unknown function <func>.

    @++             Output string too long--evaluation truncated.


Using TURBODIESEL
-----------------

You invoke TURBODIESEL within your program by calling:

    int status;
    char instring[<whatever>], outstring[MAXSTR + 1];

    outstring = ParseMacro(instring, &status);

The  output  from  the  evaluation  will  be  stored in outstring when
control is returned to your  program.   If  no  errors  were  detected
during  evaluation,  status  will be zero.  Otherwise status gives the
character position within instring at which the  error  was  detected.
If an error  occurs, TURBODIESEL  will  include  an error  diagnostic,
documented above, in outstring.

    To set single-char variables you can use:
    
    MacroVars(<string-names>,<string-types>,<value1>,...,<valueN>);

    string-names -> Variable names
    string-types -> Variable types 
                    (s: string, c: char, d: integer, f: float).
    Both strings  must be same  lenght, and the number of values  must 
    match with lenght and types.
    
    Sample:
    
    MacroVars("ABCDE","sscdf","A String","Another String",'C',5,4.67);

    To clear all variables you can use:
    
    MacroClear();

    
*/

#include "../config.h"
#include "mbselib.h"
#include "diesel.h"

/* Get(<var>, <structure_type>) allocates a new  <structure_type>  and
   places  a  pointer  to  it  in  <var>.   The  definition  is subtly
   different depending on the setting of "lint" in order to get around
   the	infuriating "possible pointer alignment problem" natter on the
   Sun. */

#ifdef lint
#define Get(var, stype) (char *) var = malloc(sizeof(struct stype))
#else
#define Get(var, stype) var = (struct stype *) malloc(sizeof(struct stype))
#endif

struct mfent {
    char *fname;			    /* Function name */
    int (*ffunc)(int, char *[], char *);    /* Evaluation function */
};

#define Mfunc(x) static int x( int, char *[], char *);\
                  static int x( int nargs, char *argv[], char* output)
#ifdef VARIABLES

struct varitem {		      /* Variable chain item */
    struct varitem *vinext;	      /* Next variable item in chain */
    char *viname;		      /* Variable name */
    char *vivalue;		      /* Variable value */
};

static struct varitem *varlist = NULL; /* Variable chain */
#endif /* VARIABLES */

/*  UCASE  --  Force letters in string to upper case.  */

static void ucase(char *);
static void ucase(char *c)
{
    char ch;

    while ((ch = *c) != EOS) {
	if (islower(ch)) {
	    *c = toupper(ch);
	}
	c++;
    }
}

/*  LCASE  --  Force letters in string to upper case.  */

static void lcase(char *);
static void lcase(char *c)
{
    char ch;

    while ((ch = *c) != EOS) {
	if (isupper(ch)) {
	    *c = tolower(ch);
	}
	c++;
    }
}


/* The following functions are included just in case your benighted C
   library doesn't include them. */

#ifdef STRCASECMP

/*  STRCASECMP	--  Compare two strings, case insensitive.  */
static int strcasecmp(const char *, const char*);
static int strcasecmp(const char *s1, const char *s2)
{
    while ((*s1 != EOS) && (*s2 != EOS) && (toupper(*s1) == toupper(*s2))) {
	s1++;
	s2++;
    }
    if (*s1 == EOS) {
	return (*s2 == EOS) ? 0 : -1;
    }
    return (toupper(*s1) > toupper(*s2)) ? 1 : -1;
}
#endif /* STRCASECMP */

#ifdef STRNCASECMP

/*  STRNCASECMP  --  Compare two strings, length limited, case insensitive.  */

static int strncasecmp(const char *, const char *, const int);
static int strncasecmp(const char *s1, const char *s2, const int n)
{
    while ((*s1 != EOS) && (*s2 != EOS) &&
	   (n > 0) && (toupper(*s1) == toupper(*s2))) {
	s1++;
	s2++;
	n--;
    }
    if (n <= 0) {
	return 0;
    }
    if (*s1 == EOS) {
	return (*s2 == EOS) ? 0 : -1;
    }
    return (toupper(*s1) > toupper(*s2)) ? 1 : -1;
}
#endif /* STRNCASECMP */

/*  MLEDREAL  --  Edit a  double number  into  the  most  compact string
		  representation that doesn't lose significance. */
static void mledreal(double, char *);
static void mledreal(double r, char *edbuf)
{
    int sprec;

    V snprintf(edbuf, MAXSTR, "%.12f", r);
    if ((!strchr(edbuf, 'E')) && strchr(edbuf, '.')) {
	/* Trim redundant trailing zeroes off the number. */
	for (sprec = strlen(edbuf) - 1; sprec > 0; sprec--) {
	    if (edbuf[sprec] != '0' || edbuf[sprec - 1] == '.')
		break;
	    edbuf[sprec] = EOS;
	}
	/* Now, if all we're left with is a ".0", drop the decimal
	   portion entirely. */
	if ((strlen(edbuf) > 2) && (strcmp(edbuf + (strlen(edbuf) - 2),
	    ".0") == 0)) {
	    edbuf[strlen(edbuf) - 2] = EOS;
	}
    }
}

/*  IARG  --  Interpret  an  argument  as an integer.  The argument is
	      scanned according to the scanf() "%i" format.   TRUE  is
	      returned if a valid integer is scanned, FALSE otherwise. */
static int iarg(char *, int *);
static int iarg(char *argstr, int *intres)
{
    char earg[MAXSTR];

    if (diesel(argstr, earg) == 0) {
	return sscanf(earg, "%i", intres) == 1;
    }
    return FALSE;
}

#define Iarg(v,n)  if (!iarg(argv[(n)], &(v))) return FALSE

/*  RARG  --  Interpret an  argument  as  a  real.   The  argument  is
	      scanned according to the sscanf() "%lf" format.  TRUE is
	      returned if a valid double is scanned, FALSE otherwise. */
static int rarg(char *, double *);
static int rarg(char *argstr, double *realres)
{
    char earg[MAXSTR];

    if (diesel(argstr, earg) == 0) {
	return sscanf(earg, "%lf", realres) == 1;
    }
    return FALSE;
}

#define ArgCount(min,max) if (nargs < (min) || nargs > (max)) return FALSE

#define Rarg(v,n)  if (!rarg(argv[(n)], &(v))) return FALSE

#define Dsarg(s)   char s[MAXSTR]     /* Declare string argument */
#define Sarg(v,n)  if (diesel(argv[(n)], (v)) != 0) return FALSE

#define Rint(n)     V snprintf(output, MAXSTR, "%d", (n)); return TRUE/* Return int */
#define Rreal(n)    mledreal((n), output); return TRUE	     /* Return double */
#define Rstr(s)     V strcpy(output, (s)); return TRUE	     /* Return str */

/*
       M A C R O   I M P L E M E N T I N G   F U N C T I O N S

    The following functions, each with a header declared with Mfunc(),
    implement the macros available to the caller of Diesel.  To add  a
    macro,  simply  define  a new implementing function using the code
    below as a guideline, then add the macro and function name to  the
    macro function table, mftab[], which appears immediately after the
    last macro implementing function.

    A macro implementing function returns TRUE upon success (in  which
    case  it  must  supply  its output string in the "output" argument
    when it returns), FALSE in case of failure when  the  contents  of
    the  "output"  argument  are to be discarded, and DIAGNOSTIC if an
    error in the macro has caused a diagnostic message to be placed in
    the "output" string.

*/

/*  @(+,<int1>,<int2>,...)  --	Add numbers together  */

Mfunc(f_plus)
{
    int i;
    double result = 0;

    for (i = 0; i < nargs; i++) {
	double varg;

	Rarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result += varg;
	}
    }
    Rreal(result);
}

/*  @(-,<int1>,<int2>,...)  --	Subtract numbers from an initial number */

Mfunc(f_minus)
{
    int i;
    double result = 0;

    for (i = 0; i < nargs; i++) {
	double varg;

	Rarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result -= varg;
	}
    }
    Rreal(result);
}

/*  @(*,<int1>,<int2>,...)  --	Multiply numbers together  */

Mfunc(f_times)
{
    int i;
    double result = 1;

    for (i = 0; i < nargs; i++) {
	double varg;

	Rarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result *= varg;
	}
    }
    Rreal(result);
}

/*  @(/,<int1>,<int2>,...)  --	Divide a number by other numbers  */

Mfunc(f_divide)
{
    int i;
    double result = 1;

    for (i = 0; i < nargs; i++) {
	double varg;

	Rarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result /= varg;
	}
    }
    Rreal(result);
}

/*  @(=,<num1>,<num2>)	--  Test two numbers equal  */

Mfunc(f_numeq)
{
    double v1, v2;

    ArgCount(2, 2);

    Rarg(v1, 0);
    Rarg(v2, 1);

    Rint(FUZZEQ(v1, v2));
}

/*  @(!=,<num1>,<num2>)  --  Test two numbers unequal  */

Mfunc(f_numne)
{
    double v1, v2;

    ArgCount(2, 2);

    Rarg(v1, 0);
    Rarg(v2, 1);

    Rint(!FUZZEQ(v1, v2));
}

/*  @(<,<num1>,<num2>)	--  Test two numbers less than	*/

Mfunc(f_numlt)
{
    double v1, v2;

    ArgCount(2, 2);

    Rarg(v1, 0);
    Rarg(v2, 1);

    Rint(v1 < v2);
}

/*  @(>,<num1>,<num2>)	--  Test two numbers greater than  */

Mfunc(f_numgt)
{
    double v1, v2;

    ArgCount(2, 2);

    Rarg(v1, 0);
    Rarg(v2, 1);

    Rint(v1 > v2);
}

/*  @(>=,<num1>,<num2>)  --  Test two numbers greater than or equal  */

Mfunc(f_numge)
{
    double v1, v2;

    ArgCount(2, 2);

    Rarg(v1, 0);
    Rarg(v2, 1);

    Rint(v1 >= v2);
}

/*  @(<=,<num1>,<num2>)  --  Test two numbers less than or equal  */

Mfunc(f_numle)
{
    double v1, v2;

    ArgCount(2, 2);

    Rarg(v1, 0);
    Rarg(v2, 1);

    Rint(v1 <= v2);
}

/*  @(AND,<int1>,<int2>,...)  --  Bitwise AND integers together  */

Mfunc(f_and)
{
    int i, result = 1;

    for (i = 0; i < nargs; i++) {
	int varg;

	Iarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result &= varg;
	}
    }
    Rint(result);
}

#ifdef UNIXTENSIONS

/*  @(EDTIME,<time>,<picture>)	--  Edit time to format <picture>

	Assume the date is: Thursday, 2 September 1993 4:53:17

	Format phrases:
	    D		    2
	    DD		    02
	    DDD 	    Thu
	    DDDD	    Thursday
	    M		    9
	    MO		    09
	    MON 	    Sep
	    MONTH	    September
	    YY		    93
	    YYYY	    1993
	    H		    4
	    HH		    04
	    MM		    53
	    SS		    17
	    AM/PM	    AM
	    am/pm	    am
	    A/P 	    A
	    a/p 	    a

	If any of the "AM/PM" phrases appear in the picture,  the  "H"
	and  "HH"  phrases will edit the time according to the 12 hour
	civil clock (12:00-12:59-1:00-11:59) instead of  the  24  hour
	clock (00:00-23:59).

	If <time> is 0, the current time and date is edited.
*/

Mfunc(f_edtime)
{
    double val;
    Dsarg(pic);
    time_t ltime;
    struct tm *jd;
    char *pp = pic;

    static int mday, min, tmon, sec, heure, year, yearmod100;

    /* Why declare it this way?  Think about the poor sucker  who  has
       to  localise  a	strncasecmp(zilch,  "MONTH",  5)  when "MONTH"
       translates into different length words in other languages! */

    static char month[] = "MONTH",
		mon[]	= "MON",
		dddd[]	= "DDDD",
		ddd[]	= "DDD",
		ampm[]	= "AM/PM",
		ap[]	= "A/P";
    int lcompl;
#define lComp(x)    x, lcompl = strlen(x)

    static struct {
	char *pname;
	char *pfmt;
	int *pitem;
    } pictab[] = {
	/* Careful!  These must be sorted by descending order of
	   picture string length. */
	{(char *)"YYYY", (char *)"%02d", &year},
	{(char *)"DD",	 (char *)"%02d", &mday},
	{(char *)"HH",	 (char *)"%02d", &heure},
	{(char *)"MM",	 (char *)"%02d", &min},
	{(char *)"MO",	 (char *)"%02d", &tmon},
	{(char *)"SS",	 (char *)"%02d", &sec},
	{(char *)"YY",	 (char *)"%02d", &yearmod100},
	{(char *)"D",	 (char *)"%d",	 &mday},
	{(char *)"H",	 (char *)"%d",	 &heure},
	{(char *)"M",	 (char *)"%d",	 &tmon}
    };

    ArgCount(2, 2);

    Rarg(val, 0);
    Sarg(pic, 1);
    V strcpy(output, "");

    /* Special gimmick: if the time argument is zero,  use the current
       date  and  time	saved  at  the	start  of  the	entire	 macro
       evaluation.   This  not	only  saves  space and time, it avoids
       embarrassment due to the time incrementing  between  individual
       calls on @(edtime) within one overall macro line. */

    if (FUZZEQ(val, 0.0)) {
	ltime = time((time_t *) NULL);
    } else {
	ltime = val;
    }
    jd = localtime(&ltime);
    tmon = jd->tm_mon + 1;
    mday = jd->tm_mday;
    min = jd->tm_min;
    sec = jd->tm_sec;
    year = jd->tm_year + 1900;
    yearmod100 = year % 100;	      /* Calculate year mod 100 */
#ifdef lint
    /* The variables that appear in the following bogus statement
       are set above but only referenced via their pointers in the
       pictab[] table above.  Lint doesn't understand this, and
       complains that the variables are set but never referenced.
       Handing the following statement to lint shuts it up. */
    tmon = mday + min + tmon + sec + yearmod100;
#endif

    /* If  the time picture contains any  "A" or "P" characters, which
       indicate that time is expressed in AM or  PM  (or  any  of  its
       variants), convert the hour to 12 hour civil clock time. */

    heure = jd->tm_hour;
    if (strstr(pic, "AM/PM") || strstr(pic, "A/P") ||
	strstr(pic, "am/pm") || strstr(pic, "a/p")) {
	heure = jd->tm_hour % 12;
	if (heure == 0) {
	    heure = 12;
	}
    }

    while (*pp != EOS) {

	/* Detect incipient output string overflow and escape in time. */

	if (strlen(output) > STRLIMIT) {
	    V strcat(output, OverFlow);
	    return DIAGNOSTIC;
	}

	if (strncasecmp(pp, lComp(month)) == 0) {
	    static char *mois[] = {
			(char *)"January",
			(char *)"February",
			(char *)"March",
			(char *)"April",
			(char *)"May",
			(char *)"June",
			(char *)"July",
			(char *)"August",
			(char *)"September",
			(char *)"October",
			(char *)"November",
			(char *)"December" 
	    };
	    V strcat(output, mois[jd->tm_mon]);
	    pp += lcompl;
	} else if (strncasecmp(pp, lComp(mon)) == 0) {
	    static char *mois[] = {
			(char *)"Jan",
			(char *)"Feb",
			(char *)"Mar",
			(char *)"Apr",
			(char *)"May",
			(char *)"Jun",
			(char *)"Jul",
			(char *)"Aug",
			(char *)"Sep",
			(char *)"Oct",
			(char *)"Nov",
			(char *)"Dec" 
	    };
	    V strcat(output, mois[jd->tm_mon]);
	    pp += lcompl;
	} else if (strncasecmp(pp, lComp(dddd)) == 0) {
	    static char *jour[] = {
			(char *)"Sunday",
			(char *)"Monday",
			(char *)"Tuesday",
			(char *)"Wednesday",
			(char *)"Thursday",
			(char *)"Friday",
			(char *)"Saturday"
	    };
	    V strcat(output, jour[jd->tm_wday]);
	    pp += lcompl;
	} else if (strncasecmp(pp, lComp(ddd)) == 0) {
	    static char *jour[] = {
			(char *)"Sun",
			(char *)"Mon",
			(char *)"Tue",
			(char *)"Wed",
			(char *)"Thu",
			(char *)"Fri",
			(char *)"Sat"
	    };
	    V strcat(output, jour[jd->tm_wday]);
	    pp += lcompl;
	} else if (strncasecmp(pp, lComp(ampm)) == 0 ||
		   strncasecmp(pp, lComp(ap)) == 0) {
	    char AandP = (jd->tm_hour >= 12 ? 'P' : 'A');
	    int l = strlen(output);

	    if (islower(*pp)) {
		AandP = tolower(AandP);
	    }
	    output[l] = AandP;
	    if (pp[1] != '/') {
		output[++l] = pp[1];
	    }
	    output[l + 1] = EOS;
	    pp += lcompl;
	} else {
	    int i, foundit = FALSE;

	    for (i = 0; i < ELEMENTS(pictab); i++) {
		if (strncasecmp(pp, pictab[i].pname,
				strlen(pictab[i].pname)) == 0) {
		    V snprintf(output + strlen(output), MAXSTR, pictab[i].pfmt,
				*pictab[i].pitem);
		    pp += strlen(pictab[i].pname);
		    foundit = TRUE;
		    break;
		}
	    }
	    if (!foundit) {
		char *op = output + strlen(output);

		*op++ = *pp++;
		*op = EOS;
	    }
	}
    }
    return TRUE;
}
#endif /* UNIXTENSIONS */

/*  @(EQ,<str1>,<str2>)  --  Return 1 if strings equal, 0 otherwise  */

Mfunc(f_equal)
{
    Dsarg(arg1);
    Dsarg(arg2);

    ArgCount(2, 2);

    Sarg(arg1, 0);
    Sarg(arg2, 1);
    Rint(strcmp(arg1, arg2) == 0);
}

/*  @(EVAL,<arg>)  --  Evaluate <arg>, re-scanning as if in input stream  */

Mfunc(f_eval)
{
    Dsarg(arg);
    int retval;

    static int depth = 0;

    if (depth >= MAXDEPTH)
	return FALSE;

    ArgCount(1, 1);

    Sarg(arg, 0);
    depth++;
    retval = (diesel(arg,output) == 0);
    depth--;
    return retval;
}

/*  @(FIX,<real>)  --  The fractional part of <real> is truncated  */

Mfunc(f_fix)
{
    double r;
    int rfix;

    ArgCount(1, 1);

    Rarg(r, 0);
    rfix = r;

    Rint(rfix);
}

#ifdef UNIXTENSIONS

/*  @(GETENV,varname)  --  Get environment variable value  */

Mfunc(f_getenv)
{
    Dsarg(vname);
    char *ep;

    ArgCount(1, 1);

    Sarg(vname, 0);

    ep = getenv(vname);
    if (strlen(ep) >= STRLIMIT) {
	V strcpy(output, OverFlow);
	return DIAGNOSTIC;
    }
    Rstr(ep != NULL ? ep : "");
}
#endif /* UNIXTENSIONS */

#ifdef VARIABLES

/*  @(CLEAR)  --             Clear all variables */

Mfunc(f_clear)
{
    struct varitem *vp = varlist, *vi;

    while (vp != NULL) {
	vi = vp->vinext;
	free(vp->viname);
	free(vp->vivalue);
	free((char *) vp);
	vp = vi;
    }
    if (varlist != NULL)
    varlist=NULL;
    Rstr("");
}


/*  @(GETVAR,<varname>)  --  Returns the value for the named
			     variable.	Errors if the variable has
			     not been defined.	*/

Mfunc(f_getvar)
{
    Dsarg(vname);
    struct varitem *vp = varlist;

    ArgCount(1, 1);
    Sarg(vname, 0);

    while (vp != NULL) {
	if (strcmp(vp->viname, vname) == 0) {
	    Rstr(vp->vivalue);
	}
	vp = vp->vinext;
    }
    return FALSE;
}
#endif /* VARIABLES */

/*  @(IF,<int1>,<true>,<false>)  --  If <int1> is nonzero, evaluate and
				     return <true>, otherwise evaluate
				     and return <false>.  If <false>
				     is omitted and <int1> is zero, the
				     null string is returned. */

Mfunc(f_if)
{
    int bval;
    Dsarg(str);

    ArgCount(2, 3);

    Iarg(bval, 0);
    if (bval) {
	Sarg(str, 1);
    } else {
	if (nargs > 2) {
	    Sarg(str, 2);
	} else {
	    str[0] = EOS;
	}
    }
    Rstr(str);
}

/*  @(INDEX,<n>,<listarg>)  --	Extracts the nth item from a comma separated
				list <listarg>.  Returns the null string if
				no nth item exists. */

Mfunc(f_index)
{
    int bval;
    Dsarg(str);
    char *sp;

    ArgCount(2, 2);

    Iarg(bval, 0);
    if (bval < 0)
       return FALSE;

    Sarg(str, 1);
    sp = str;

    /* Advance the specified number of argument separators. */

    while (bval-- > 0) {
	sp = strchr(sp, ARGSEP);
	if (sp == NULL) {
	    Rstr("");
	}
	sp++;
    }

    /* If there's another argument separator, terminate the result
       string at that point. */

    if (strchr(sp, ARGSEP)) {
	*strchr(sp, ARGSEP) = EOS;
    }

    Rstr(sp);
}
/*  @(LOWER,<string>)	    --	Convert string to lower case  */

Mfunc(f_lower)
{
    ArgCount(1, 1);
    V strcpy(output, "");
    if (nargs > 0) {
	Dsarg(str);

	Sarg(str, 0);
	lcase(str);
	V strcpy(output, str);
    }
    return TRUE;
}

/*  @(NTH,<n>,<item1>,<item2>,...<itemj>)  --  Evauates and returns <itemn>  */

Mfunc(f_nth)
{
    int n;
    Dsarg(str);

    ArgCount(2, MAXARGS);

    Iarg(n, 0);
    if ((n < 0) || ((n + 1) >= nargs))
	return FALSE;

    Sarg(str, n + 1);
    Rstr(str);
}

/*  @(OR,<int1>,<int2>,...)  --  Bitwise OR integers together  */

Mfunc(f_or)
{
    int i, result = 0;

    for (i = 0; i < nargs; i++) {
	int varg;

	Iarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result |= varg;
	}
    }
    Rint(result);
}

#ifdef VARIABLES

/*  @(SETVAR,<varname>,<value>)  --  Sets the variable named <varname>
				     to the given <value>.  If the
				     variable is not currently defined,
				     a new variable is created.  Returns
				     the null string. */

Mfunc(f_setvar)
{
    Dsarg(vname);
    Dsarg(vvalue);
    struct varitem *vp = varlist, *vi;
    char *vnew;

    ArgCount(2, 2);
    Sarg(vname, 0);
    Sarg(vvalue, 1);

    vnew = strdup(vvalue);
    if (vnew == NULL) {
	/* Out of memory--cannot define new variable. */
	return FALSE;
    }

    while (vp != NULL) {
	if (strcmp(vp->viname, vname) == 0) {
	    free(vp->vivalue);
	    vp->vivalue = vnew;
	    Rstr("");
	}
	vp = vp->vinext;
    }
    Get(vi, varitem);
    if (vi == NULL) {
	return FALSE;
    }
    vi->viname = strdup(vname);
    if (vi->viname == NULL) {
	free((char *) vi);
	return FALSE;
    }
    vi->vinext = varlist;
    vi->vivalue = vnew;
    varlist = vi;
    Rstr("");
}
#endif /* VARIABLES */

/*  @(STRCMP,<str1>,<str2>)  --  Return 0, -1 or 1 if strings equal, less or greater  */

Mfunc(f_strcmp)
{
    Dsarg(arg1);
    Dsarg(arg2);

    ArgCount(2, 2);

    Sarg(arg1, 0);
    Sarg(arg2, 1);
    Rint(strcmp(arg1, arg2));
}

/*  @(STRFILL,<string>,<ncopies>)  --  Create a string by concatenating
				       <ncopies> of <string> together  */

Mfunc(f_strfill)
{
    Dsarg(str);
    int ncopies;

    ArgCount(2, 2);

    Sarg(str, 0);
    Iarg(ncopies, 1);
    if (ncopies < 1) {
	Rstr("");
    } else {
	output[0] = EOS;
	while (ncopies-- > 0) {
	    if ((strlen(output) + strlen(str)) >= STRLIMIT) {
		V strcpy(output, OverFlow);
		return DIAGNOSTIC;
	    }
	    V strcat(output, str);
	}
    }
    return TRUE;
}

/*  @(STRLEN,<string>)	--  Return length of string  */

Mfunc(f_strlen)
{
    Dsarg(str);

    ArgCount(1, 1);

    Sarg(str, 0);
    Rint((int)strlen(str));
}

/*  @(STRSTR,<str1>,<str2>)  --  Find a substring in a string  */

Mfunc(f_strstr)
{
    Dsarg(arg1);
    Dsarg(arg2);
    int j,l,r;
    
    ArgCount(2, 2);

    Sarg(arg1, 0);
    Sarg(arg2, 1);

    l=strlen(arg2);
    r=0;    
    for (j=0; arg1[j] != EOS; j++)
	if (strncmp(&arg1[j],arg2,l) == 0){
		r=(j+1);
		break;
	}
    Rint(r);
}

/*  @(SUBSTR,<string>,<start>,<length>)  --  Extract substring	*/

Mfunc(f_substr)
{
    ArgCount(2, 3);
    V strcpy(output, "");
    if (nargs > 0) {
	Dsarg(str);
	int start, len = MAXSTR + 1, l = strlen(argv[0]);

	Sarg(str, 0);
	Iarg(start, 1);
	if (nargs > 2) {
	    Iarg(len, 2);
	}
	if ((start >= 1) && ((start - 1) < l)) {
	    char *ip = str + (start - 1), *op = output;

	    while ((len-- > 0) && *ip) {
		*op++ = *ip++;
	    }
	    *op++ = EOS;
	}
    }
    return TRUE;
}

#ifdef UNIXTENSIONS

/*  @(TIME)  --  Return Unix integer time  */

/* ARGSUSED */
Mfunc(f_time)
{
    ArgCount(0, 0);

    V snprintf(output, MAXSTR, "%d", (int32_t) time((time_t *) NULL)); 
    return TRUE;
}
#endif /* UNIXTENSIONS */

/*  @(UPPER,<string>)	    --	Convert string to upper case  */

Mfunc(f_upper)
{
    ArgCount(1, 1);
    V strcpy(output, "");
    if (nargs > 0) {
	Dsarg(str);

	Sarg(str, 0);
	ucase(str);
	V strcpy(output, str);
    }
    return TRUE;
}

/*  @(XOR,<int1>,<int2>,...)  --  Bitwise XOR integers together  */

Mfunc(f_xor)
{
    int i, result = 0;

    for (i = 0; i < nargs; i++) {
	int varg;

	Iarg(varg, i);
	if (i == 0) {
	    result = varg;
	} else {
	    result ^= varg;
	}
    }
    Rint(result);
}

/* Macro name/function table. */

static struct mfent mftab[] = {
	{(char *)"+",	    f_plus},
	{(char *)"-",	    f_minus},
	{(char *)"*",	    f_times},
	{(char *)"/",	    f_divide},
	{(char *)"=",	    f_numeq},
	{(char *)"<",	    f_numlt},
	{(char *)">",	    f_numgt},
	{(char *)"!=",	    f_numne},
	{(char *)"<=",	    f_numle},
	{(char *)">=",	    f_numge},
	{(char *)"AND",     f_and},
	{(char *)"EQ",	    f_equal},
	{(char *)"EVAL",    f_eval},
	{(char *)"FIX",     f_fix},
	{(char *)"IF",	    f_if},
	{(char *)"INDEX",   f_index},
	{(char *)"LOWER",   f_lower},
	{(char *)"NTH",     f_nth},
	{(char *)"OR",	    f_or},
	{(char *)"STRCMP",  f_strcmp},
	{(char *)"STRFILL", f_strfill},
	{(char *)"STRLEN",  f_strlen},
	{(char *)"STRSTR",  f_strstr},
	{(char *)"SUBSTR",  f_substr},
	{(char *)"UPPER",   f_upper},
	{(char *)"XOR",     f_xor},

#ifdef UNIXTENSIONS
	{(char *)"EDTIME",  f_edtime},
	{(char *)"GETENV",  f_getenv},
	{(char *)"TIME",    f_time},
#endif /* UNIXTENSIONS */

#ifdef VARIABLES
	{(char *)"CLEAR",   f_clear},
	{(char *)"GETVAR",  f_getvar},
	{(char *)"SETVAR",  f_setvar},
#endif /* VARIABLES */
};

/*  COPYMODE  --  Copies  characters  from  the  input	to the output,
		  handling quoted literal strings as it  goes.	 If  a
		  nonquoted  macro  character  is encountered, returns
		  with the string  pointer  positioned	at  the  macro
		  character.   If  end	of  string is encountered, the
		  input pointer will be left  positioned  at  the  EOS
		  character.   Returns	0  if  the  end  of  string is
		  encountered, 1 if a macro is encountered, and -1  if
		  the  end  of	input  was encountered while copying a
		  quoted string. */

static int copymode(char **, char **);
static int copymode(char **in, char **out)
{
    char *ip = *in, *op = *out;
    char ch;
    int instring = FALSE;

    while ((ch = *ip++) != EOS) {
	switch (ch) {
	    case QUOTE:
		if (instring) {
		    /* If we're in a string  and  hit  a  quote,  peek
		       ahead.	If the next character is a quote also,
		       then this is a forced quote.  Copy it literally
		       to  the	output	stream and leave the in-string
		       mode in effect. */
		    if (*ip == QUOTE) {
			*op++ = QUOTE;
			ip++;
		    } else {
			instring = FALSE;
		    }
		} else {
		    instring = TRUE;
		}
		break;

	    case MACROCHAR:
		if (!instring && *ip == ARGOPEN) {
		    *in = ip;
		    *out = op;
		    return 1;
		}
		/* Wheeee!!!  Note fall-through. */

	    default:
		*op++ = ch;
		break;
	}

	/* If we're in danger of overflowing the output string, attach
	   the string overflow indication and bail  out.   We  advance
	   the	input  pointer	to the end of string and signal end of
	   input  being  encountered  to   cleanly   shut   down   the
	   interpreter. */

	if ((op - *out) > STRLIMIT) {
	    V strcpy(op, OverFlow);
	    *in = ip + strlen(ip);    /* Advance input pointer to EOS */
	    *out = op + strlen(op);   /* Calculate end of string pointer */
	    return 0;		      /* Say end of string was encountered */
	}
    }
    *in = ip - 1;
    *out = op;
    return instring ? -1 : 0;
}

/*  MACROMODE  --  Scan  a  macro, identifying its arguments.  Returns
		   the number  of  arguments  scanned  (including  the
		   macro  name)  if  the macro is valid, 0 if a syntax
		   error occurs, and -1 if the end of the input string
		   was	encountered  before the matching macro bracket
		   was found.  If a positive result is	returned,  the
		   output   string   will  contain  the  arguments  as
		   successive strings, separated by EOS markers. */

static int macromode(char **, char**);
static int macromode(char **in, char **out)
{
    char *ip = *in, *op = *out;
    char ch;
    int nargs = 0, instring = FALSE, depth = 0;

    if ((ch = *ip++) != ARGOPEN) {
	*op++ = MACROCHAR;
	*op++ = ch;
	*in = ip - 1;		      /* Unconsume character */
	*out = op;
	return 0;
    }

    /* Now scan the arguments of the macro, searching for the matching
       macro  bracket.	 We  recognise	quoted	strings  and  argument
       delimiter  characters  here,  but  don't  evaluate  any	of the
       arguments. */

    while ((ch = *ip++) != EOS) {
	switch (ch) {
	    case QUOTE:
		if (instring) {

		    /* If we're in a string  and  hit  a  quote,  peek
		       ahead.	If the next character is a quote also,
		       then this is a forced quote.  Copy it literally
		       to  the	output	stream and leave the in-string
		       mode in effect. */

		    if (*ip == QUOTE) {
			*op++ = QUOTE;
			ip++;
		    } else {
			instring = FALSE;
		    }
		} else {
		    instring = TRUE;
		}
		break;

	    case ARGOPEN:
		if (!instring) {
		    depth++;
		}
		*op++ = ch;
		break;

	    case ARGCLOSE:
		if (!instring) {
		    if (--depth < 0) {
			*op++ = EOS;
			nargs++;
			*out = op;
			*in = ip;
			return nargs;
		    }
		}
		*op++ = ch;
		break;

	    case ARGSEP:
		if (!instring && (depth == 0)) {
		    if (nargs >= MAXARGS - 1)
			goto errout;

		    nargs++;	      /* Increment number of arguments */
		    ch = EOS;	      /* Store argument break in output */
		}
		/* Wheeee!!!  Note fall-through. */

	    default:
		*op++ = ch;
		break;
	}

	/* If we're in danger of overflowing the output string, attach
	   the string overflow indication and bail  out.   We  advance
	   the	input  pointer	to the end of string and signal end of
	   input  being  encountered  to   cleanly   shut   down   the
	   interpreter. */

	if ((op - *out) > STRLIMIT) {
errout:
	    V strcpy(op, OverFlow);
	    *in = ip + strlen(ip);    /* Advance input pointer to EOS */
	    *out = op + strlen(op);   /* Calculate end of string pointer */
	    return -1;		      /* Call it an unmatched bracket */
	}
    }

    /* Hit end of input string without finding matching macro bracket. */

    *op++ = EOS;
    *out = op;
    *in = ip - 1;
    return -1;
}

/*  MACROVALUE	--  Determine the value of a macro.  Returns TRUE if
		    the macro was evaluated successfully, FALSE if
		    an error was detected, and DIAGNOSTIC if the macro
		    errored and supplied a diagnostic message as its
		    output. */

static int macrovalue(int, char *, char *);
static int macrovalue(int nargs, char *args, char *output)
{
    char *argv[MAXARGS];
    Dsarg(macname);
    int i;

    for (i = 0; i < MAXARGS; i++) {
	argv[i] = (char *)"";
    }
    for (i = 0; i < nargs; i++) {
	argv[i] = args;
	args += strlen(args) + 1;
    }

    /* Look up the argument function in the function table. */

    Sarg(macname, 0);
#ifdef CASEINS
    ucase(macname);
#endif

#ifdef DIESEL_TRACE
    if (tracing) {
	V printf("Eval: @(%s", macname);
	for (i = 1; i < nargs; i++) {
	    V printf(", %s", argv[i]);
	}
	V printf(")\n");
    }
#endif
    for (i = 0; i < ELEMENTS(mftab); i++) {
	if (strcmp(macname, mftab[i].fname) == 0) {
	    int mstat = (*mftab[i].ffunc)(nargs - 1, argv + 1, output);

	    /* If the macro bailed out without supplying a  diagnostic
	       message, make up a general-purpose message here. */

	    if (mstat == FALSE) {
		V snprintf(output, MAXSTR, " @(%s,%c%c) ", macname, '?', '?');
	    }
	    if (mstat != TRUE) {
#ifdef DIESEL_TRACE
		if (tracing) {
		    V printf("Err:  %s\n", output);
		}
#endif
		return DIAGNOSTIC;
	    }
#ifdef DIESEL_TRACE
	    if (tracing) {
		V printf("===>	%s\n", output);
	    }
#endif
	    return TRUE;
	}
    }
    V snprintf(output, MAXSTR, " @(%s)?? ", macname);
#ifdef DIESEL_TRACE
    if (tracing) {
	 V printf("Err:  %s\n", output);
    }
#endif
    return DIAGNOSTIC;
}

/*  MACROEVAL  --  Evaluate a macro and place its results in the output
		   string.  Returns 1 if the macro was valid, 0 in case
		   of error.  If the macro itself detected an error which
		   placed diagnostic output in out, 2 is returned. */

static int macroeval(char **, char**);
static int macroeval(char **in, char **out)
{
    char *ip = *in, *op = *out;
    char margs[MAXSTR], mvalue[MAXSTR];
    char *ma = margs;
    int mstat, nargs;

    nargs = mstat = macromode(&ip, &ma);

    if (mstat > 0) {
#ifdef ECHOMAC
	*op++ = ' ';
	*op++ = '<';
	V snprintf(op, MAXSTR, "(%d)", mstat);
	op += strlen(op);
	ma = margs;
	while (mstat-- > 0) {
	    int l = strlen(ma);

	    V strcpy(op, ma);
	    op += l;
	    ma += l + 1;
	    *op++ = ';';
	}
	*op++ = '>';
	*op++ = '=';
#endif

	/* Evaluate the macro. */

	mstat = macrovalue(nargs, margs, mvalue);
	V strcpy(op, mvalue);
	op += strlen(mvalue);
#ifdef ECHOMAC
	if (mstat == FALSE || mstat == DIAGNOSTIC) {
	    V strcpy(op, "*ERR*");
	    op += 5;
	}
	*op++ = ' ';
#endif
    } else {
	mstat = FALSE;
    }

    *op++ = EOS;
    *out = op;
    *in = ip;

    return mstat;
}

/*  DIESEL  --	Evaluate a string IN and  return  the  value  in  OUT.
		Returns  zero  if the evaluation was successful; if an
		error was detected, returns the column	at  which  the
		error was found. */

int diesel(char *in, char *out)
{
    int dstat;
    char *inp = in, *outp = out;

    while (TRUE) {
	dstat = copymode(&inp, &outp);
	if (dstat == 1) {
	    char margs[MAXSTR];
	    char *ma = margs;

	    dstat = macroeval(&inp, &ma);
	    if (dstat > 0) {

		/* If we're about to overflow the output string,  bail
		   out of  the	evaluation  and  append  the  overflow
		   marker. */

		if (((outp - out) + strlen(margs)) > STRLIMIT) {
		    V strcpy(outp, OverFlow);
		    return inp - in;
		}
		V strcpy(outp, margs);
		outp += strlen(margs);
	    } else {
		*outp++ = MACROCHAR;
		*outp++ = '?';
		*outp++ = EOS;
		return inp - in;
	    }

	    /* Error  detected	in  macro  evaluation  which  placed a
	       diagnostic string in the output.  */

	    if (dstat == DIAGNOSTIC) {
		return inp - in;
	    }
	} else {
	    *outp++ = EOS;
	    break;
	}
    }

    return dstat;
}


#ifdef TESTPROG

/*  Test program.  */

main()
{
    char in[MAXSTR + 1], out[MAXSTR + 1];
    int err;

    while (TRUE) {
	if (fgets(in, sizeof in, stdin) == NULL) {
	    break;
	}

	/* Cheap way to be insensitive to EOL conventions. */

	snprintf(out, MAXSTR, "%s",ParseMacro(in,&err));
	if (err) {
	    V printf("=> %s\n", in);
	    V printf("---");
	    while (--err > 0) {
		V printf("-");
	    }
	    V printf("^\n");
	}
	V printf("%s", out);
    }
    return 0;
}
#endif /* TESTPROG */
