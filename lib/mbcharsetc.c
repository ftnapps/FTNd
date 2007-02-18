/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Characterset Compiler
 * Author ................: Martin Junius, for Fidogate
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek                FIDO:   2:280/2802
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

/*****************************************************************************
 * FIDOGATE --- Gateway UNIX Mail/News <-> FTN NetMail/EchoMail
 *
 * Charset mapping table compiler
 *
 *****************************************************************************
 * Copyright (C) 1990-2002
 *  _____ _____
 * |     |___  |   Martin Junius             <mj@fidogate.org>
 * | | | |   | |   Radiumstr. 18
 * |_|_|_|@home|   D-51069 Koeln, Germany
 *
 * This file is part of FIDOGATE.
 *
 * FIDOGATE is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FIDOGATE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FIDOGATE; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "mbselib.h"

#ifndef	USE_EXPERIMENT

#include "mbcharsetc.h"


char *str_copy(char *, size_t, char *);

#define strieq(a,b)  (strcasecmp ((a),(b)) == 0)
#define	PROGRAM	"mbcharsetc"
#define BUFFERSIZE  (32*1024) /* Global buffer */
#define BUF_COPY(d,s)   str_copy  (d,sizeof(d),s)

char buffer[BUFFERSIZE];



/*
 * Prototypes
 */
int	charset_parse_c		(char *);
int	charset_do_file		(char *);
int	compile_map		(char *, char *);
void	usage			(void);



/*
 * Check for hex digits, signed char-safe version of isxdigit()
 */
int is_xdigit(int c)
{
    /* 
     * Some <ctype.h> implementation only accept a parameter value range
     * of [-1,255]. This may lead to problems, because we're quite often
     * passing *p's to is_digit() with a char *p variable. The default
     * char type is signed in most C implementation. 
     */
    return isxdigit((c & 0xff));
}


/*
 * Check for octal digits
 */
int is_odigit(int c)
{
    return c>='0' && c<'8';
}



/*
 * isspace() replacement, checking for SPACE, TAB, CR, LF
 */
int is_space(int c)
{
    return c==' ' || c=='\t' || c=='\r' || c=='\n';
}


/*
 * Read line from config file. Strip `\n', leading spaces,
 * comments (starting with `#'), and empty lines. cf_getline() returns
 * a pointer to the first non-whitespace in buffer.
 */
static int cf_lineno = 0;

int cf_lineno_get(void)
{
    return cf_lineno;
}


int cf_lineno_set(int n)
{
    int	old;
	    
    old = cf_lineno;
    cf_lineno = n;

    return old;
}


char *cf_getline(char *cbuffer, int len, FILE *fp)
{
    char *p;

    while (fgets(cbuffer, len, fp)) {
	cf_lineno++;
	Striplf(cbuffer);
	for(p = cbuffer; *p && is_space(*p); p++) ; /* Skip white spaces */
	    if (*p != '#')
		return p;
    }
    return NULL;
}



/*
 * Parse character
 */
int charset_parse_c(char *s)
{
    int val, n;
    
    if(s[0] == '\\') {			/* Special: \NNN or \xNN */
	s++;
	val = 0;
	n = 0;
	if(s[0]=='x' || s[0]=='X') {	/* Hex */
	    s++;
	    while (is_xdigit(s[0]) && n<2) {
		s[0] = toupper(s[0]);
		val = val * 16 + s[0] - (s[0]>'9' ? 'A'-10 : '0');
		s++;
		n++;
	    }
	    if(*s)
		return ERROR;
	} else {			/* Octal */
	    while(is_odigit(s[0]) && n<3) {
		val = val * 8 +  s[0] - '0';
		s++;
		n++;
	    }
	    if(*s)
		return ERROR;
	}
    } else {
	if(s[1] == 0)			/* Single char */
	    val = s[0] & 0xff;
	else
	    return ERROR;
    }

    return val & 0xff;
}



/*
 * Process one line from charset.map file
 */
int charset_do_line(char *line)
{
    static CharsetTable	*pt = NULL;
    char		*key, *w1, *w2;
    CharsetAlias	*pa;
    int			i, c1, c2;
    
    key = strtok(line, " \t");
    if(!key)
	return TRUE;

    if (strieq(key, "include")) {
	w1 = strtok(NULL, " \t");
	if (charset_do_file(w1) == FALSE)
	    return FALSE;
    }

    /* Define alias */
    else if (strieq(key, "alias")) {
	w1 = strtok(NULL, " \t");
	w2 = strtok(NULL, " \t");
	if(!w1 || !w2) {
	    fprintf(stderr, "%s:%d: argument(s) for alias missing\n", PROGRAM, cf_lineno_get());
	    return FALSE;
	}
	
	pa = charset_alias_new();
	BUF_COPY(pa->alias, w1);
	BUF_COPY(pa->name, w2);
    }

    /* Define table */
    else if (strieq(key, "table")) {
	w1 = strtok(NULL, " \t");
	w2 = strtok(NULL, " \t");
	if(!w1 || !w2) {
	    fprintf(stderr, "%s:%d: argument(s) for table missing\n", PROGRAM, cf_lineno_get());
	    return FALSE;
	}

	pt = charset_table_new();
	BUF_COPY(pt->in, w1);
	BUF_COPY(pt->out, w2);
    }

    /* Define mapping for character(s) in table */
    else if (strieq(key, "map")) {
	w1 = strtok(NULL, " \t");
	if (!w1) {
	    fprintf(stderr, "%s:%d: argument for map missing\n", PROGRAM, cf_lineno_get());
	    return FALSE;
	}

	/* 1:1 mapping */
	if (strieq(w1, "1:1")) {
	    for(i=0; i<MAX_CHARSET_IN; i++) {
		if(pt->map[i][0] == 0) {
		    pt->map[i][0] = 0x80 + i;
		    pt->map[i][1] = 0;
		}
	    }
	}
	/* 1:1 mapping, but not for 0x80...0x9f */
	if(strieq(w1, "1:1-noctrl")) {
	    for(i=0x20; i<MAX_CHARSET_IN; i++) {
		if(pt->map[i][0] == 0) {
		    pt->map[i][0] = 0x80 + i;
		    pt->map[i][1] = 0;
		}
	    }
	}
	/* Mapping for undefined characters */
	else if(strieq(w1, "default")) {
	    /**FIXME: not yet implemented**/
	}
	/* Normal mapping */
	else {
	    if ((c1 = charset_parse_c(w1)) == ERROR) {
		fprintf(stderr, "%s:%d: illegal char %s\n", PROGRAM, cf_lineno_get(), w1);
		return FALSE;
	    }
	    if (c1 < 0x80) {
		fprintf(stderr, "%s:%d: illegal char %s, must be >= 0x80\n", PROGRAM, cf_lineno_get(), w1);
		return FALSE;
	    }

	    for (i=0; i<MAX_CHARSET_OUT-1 && (w2 = strtok(NULL, " \t")); i++ ) {
		if( (c2 = charset_parse_c(w2)) == ERROR) {
		    fprintf(stderr, "%s:%d: illegal char definition %s\n", PROGRAM, cf_lineno_get(), w2);
		    return FALSE;
		}
		pt->map[c1 & 0x7f][i] = c2;
	    }
	    for (; i<MAX_CHARSET_OUT; i++)
		pt->map[c1 & 0x7f][i] = 0;
	}
    }
    /* Error */
    else {
	fprintf(stderr, "%s:%d: illegal key word %s\n", PROGRAM, cf_lineno_get(), key);
	return FALSE;
    }
    
    return TRUE;
}



/*
 * Process charset.map file
 */
int charset_do_file(char *name)
{
    FILE *fp;
    char *p;
    int	oldn;

    if(!name)
	return FALSE;
    
    oldn = cf_lineno_set(0);
    fp = fopen(name, "r");
    if(!fp)
	return FALSE;
    
    while ((p = cf_getline(buffer, BUFFERSIZE, fp)))
	charset_do_line(p);

    fclose(fp);
    cf_lineno_set(oldn);
    
    return TRUE;
}



/*
 * Compile charset.map
 */
int compile_map(char *in, char *out)
{
    /* Read charset.map and compile */
    if(charset_do_file(in) == FALSE) {
	fprintf(stderr, "%s: compiling map file %s failed", PROGRAM, in);
	return MBERR_GENERAL;
    }

    /* Write binary output file */
    if (charset_write_bin(out) == ERROR) {
	fprintf(stderr, "%s: writing binary map file %s failed", PROGRAM, out);
	return MBERR_GENERAL;
    }

    return MBERR_OK;
}



void usage(void)
{
    fprintf(stderr, "\n%s: MBSE BBS %s Character Map Compiler\n", PROGRAM, VERSION);
    fprintf(stderr, "            %s\n\n", COPYRIGHT);
    fprintf(stderr, "usage: %s charset.map charset.bin\n\n", PROGRAM);
    exit(MBERR_OK);
}



int main(int argc, char **argv)
{
    int	    ret = MBERR_OK;
    char    *name_in, *name_out;
 
    if (argc != 3)
	usage();

    name_in = argv[1];
    name_out = argv[2];
    ret = compile_map(name_in, name_out);
    
    exit(ret);
}

#else


int main(int argc, char **argv)
{
    printf("program disabled by configure option\n");
    exit(0);
}

#endif

