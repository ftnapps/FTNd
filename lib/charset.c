/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Characterset functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:	2:280/2802
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

#include "../config.h"
#include "mbselib.h"


#define BUF_APPEND(d,s)   str_append(d,sizeof(d),s)


/*
 * Alias linked list
 */
static CharsetAlias *charset_alias_list = NULL;
static CharsetAlias *charset_alias_last = NULL;

/*
 * Table linked list
 */
static CharsetTable *charset_table_list = NULL;
static CharsetTable *charset_table_last = NULL;

/*
 * Current charset mapping table
 */
static CharsetTable *charset_table_used = NULL;




/*
 * str_printf(): wrapper for sprintf()/snprintf()
 */
int str_printf(char *buf, size_t len, const char *fmt, ...)
{
    va_list args;
    int	    n;
	        
    va_start(args, fmt);
		    
    n = vsprintf(buf, fmt, args);
    if (n >= len) {
	WriteError("Internal error - str_printf() buf overflow");
	/**NOT REACHED**/
	return FALSE;
    }

    /* 
     * Make sure that buf[] is terminated with a \0. vsnprintf()
     * should do this automatically as required by the ANSI C99
     * proposal, but one never knows ... see also discussion on
     * BugTraq 
     */
    buf[len - 1] = 0;
    va_end(args);

    return n;
}


char *str_append(char *d, size_t n, char *s)
{
    int max = n - strlen(d) - 1;

    strncat(d, s, max);
    d[n-1] = 0;
    return d;
}



char *str_copy(char *d, size_t n, char *s)
{
    strncpy(d, s, n);
    d[n-1] = 0;
    return d;
}

#define BUF_COPY(d,s)   str_copy  (d,sizeof(d),s)


char *getchrs(int val)
{
    switch (val) {
        case FTNC_NONE:     return (char *)"Undefined";
        case FTNC_CP437:    return (char *)"CP437 2";
        case FTNC_CP850:    return (char *)"CP850 2";
        case FTNC_CP865:    return (char *)"CP865 2";
        case FTNC_CP866:    return (char *)"CP866 2";
	case FTNC_CP852:    return (char *)"CP852 2";
	case FTNC_CP895:    return (char *)"CP895 2";
        case FTNC_LATIN_1:  return (char *)"LATIN-1 2";
        case FTNC_LATIN_2:  return (char *)"LATIN-2 2";
        case FTNC_LATIN_5:  return (char *)"LATIN-5 2";
        case FTNC_MAC:      return (char *)"MAC 2";
        default:            return (char *)"LATIN-1 2";
    }
}


char *getchrsdesc(int val)
{
    switch (val) {
        case FTNC_NONE:     return (char *)"Undefined";
        case FTNC_CP437:    return (char *)"IBM codepage 437 (Western European) (ANSI terminal)";
        case FTNC_CP850:    return (char *)"IBM codepage 850 (Latin-1)";
        case FTNC_CP865:    return (char *)"IBM codepage 865 (Nordic)";
        case FTNC_CP866:    return (char *)"IBM codepage 866 (Russian)";
	case FTNC_CP852:    return (char *)"IBM codepage 852 (Czech, Latin-1)";
	case FTNC_CP895:    return (char *)"IBM codepage 895 (Czech, Kamenicky)";
        case FTNC_LATIN_1:  return (char *)"ISO 8859-1 (Western European)";
        case FTNC_LATIN_2:  return (char *)"ISO 8859-2 (Eastern European)";
        case FTNC_LATIN_5:  return (char *)"ISO 8859-5 (Turkish)";
        case FTNC_MAC:      return (char *)"MacIntosh character set";
        default:            return (char *)"ERROR";
    }
}



/*
 * Alloc new CharsetTable and put into linked list
 */
CharsetTable *charset_table_new(void)
{
    CharsetTable *p;

    /* Alloc and clear */
    p = (CharsetTable *)xmalloc(sizeof(CharsetTable));
    memset(p, 0, sizeof(CharsetTable));
    p->next = NULL;                     /* Just to be sure */
		        
    /* Put into linked list */
    if(charset_table_list)
	charset_table_last->next = p;
    else
	charset_table_list       = p;
    charset_table_last       = p;

    return p;
}


/*
 * Alloc new CharsetAlias and put into linked list
 */
CharsetAlias *charset_alias_new(void)
{
    CharsetAlias *p;

    /* Alloc and clear */
    p = (CharsetAlias *)xmalloc(sizeof(CharsetAlias));
    memset(p, 0, sizeof(CharsetAlias));
    p->next = NULL;                     /* Just to be sure */
		        
    /* Put into linked list */
    if(charset_alias_list)
	charset_alias_last->next = p;
    else
	charset_alias_list       = p;
    charset_alias_last       = p;

    return p;
}



/*
 * Write binary mapping file
 */
int charset_write_bin(char *name)
{
    FILE	    *fp;
    CharsetTable    *pt;
    CharsetAlias    *pa;
		    
    fp = fopen(name, "w+");
    if (!fp)
	return FALSE;

    /* 
     * Write aliases 
     */
    for (pa = charset_alias_list; pa; pa=pa->next) {
	fputc(CHARSET_FILE_ALIAS, fp);
	fwrite(pa, sizeof(CharsetAlias), 1, fp);
	if (ferror(fp)) {
	    fclose(fp);
	    return FALSE;
	}
    }

    /* 
     * Write tables 
     */
    for(pt = charset_table_list; pt; pt=pt->next) {
	fputc(CHARSET_FILE_TABLE, fp);
	fwrite(pt, sizeof(CharsetTable), 1, fp);
	if (ferror(fp)) {
	    fclose(fp);
	    return FALSE;
	}
    }

    fclose(fp);
    return TRUE;
}



/*
 * Read binary mapping file
 */
int charset_read_bin(void)
{
    FILE	    *fp;
    int		    c, n;
    CharsetTable    *pt;
    CharsetAlias    *pa;
    char	    *name;
    
    name = calloc(PATH_MAX, sizeof(char));
    sprintf(name, "%s/etc/charset.bin", getenv("MBSE_ROOT"));
    Syslog('s', "Reading %s", name);
    if ((fp = fopen(name, "r")) == NULL) {
	WriteError("$Can't open %s", name);
	free(name);
	return FALSE;
    }
    free(name);

    while ((c = fgetc(fp)) != EOF) {
        switch(c) {
	    case CHARSET_FILE_ALIAS:	pa = charset_alias_new();
					n = fread((void *)pa, sizeof(CharsetAlias), 1, fp);
					pa->next = NULL;                    /* overwritten by fread() */
					if (n != 1) 
					    return FALSE;
					Syslog('s', "read charset alias: %s -> %s", pa->alias, pa->name);
					break;
	    case CHARSET_FILE_TABLE:	pt = charset_table_new();
					n = fread((void *)pt, sizeof(CharsetTable), 1, fp);
					pt->next = NULL;                    /* overwritten by fread() */
					if (n != 1) 
					    return FALSE;
					Syslog('s', "read charset table: %s -> %s", pt->in, pt->out);
					break;
	    default:			return FALSE;
					break; 
        }
    }
    
    if(ferror(fp))
        return FALSE;
    fclose(fp);
    return TRUE;
}



/*
 * Convert to MIME quoted-printable =XX if qp==TRUE
 */
char *charset_qpen(int c, int qp)
{
    static char buf[4];

    c &= 0xff;
    
    if (qp && (c == '=' || c >= 0x80))
        str_printf(buf, sizeof(buf), "=%02.2X", c & 0xff);
    else {
        buf[0] = c;
        buf[1] = 0;
    }
    
    return buf;
}



/*
 * Map single character
 */
char *charset_map_c(int c, int qp)
{
    static char buf[MAX_CHARSET_OUT * 4];
    char *s;
    
    c &= 0xff;
    buf[0] = 0;
    
    if (charset_table_used && c>=0x80) {
        s = charset_table_used->map[c - 0x80];
        while(*s)
            BUF_APPEND(buf, charset_qpen(*s++, qp));
    } else {
        BUF_COPY(buf, charset_qpen(c, qp));
    }

    return buf;
}



/*
 * Search alias
 */
char *charset_alias_fsc(char *name)
{
    CharsetAlias *pa;

    /* 
     * Search for aliases
     */
    for (pa = charset_alias_list; pa; pa=pa->next) {
        if (strcasecmp(pa->name, name) == 0)
            return pa->alias;
    }

    return name;
}



char *charset_alias_rfc(char *name)
{
    CharsetAlias *pa;

    /* 
     * Search for aliases 
     */
    for (pa = charset_alias_list; pa; pa=pa->next) {
        if (strcasecmp(pa->alias, name) == 0)
            return pa->name;
    }

    return name;
}



/*
 * Set character mapping table
 */
int charset_set_in_out(char *in, char *out)
{
    CharsetTable    *pt;
    CharsetAlias    *pa;
    int		    i;
    
    if (!in || !out)
        return FALSE;

    /*
     * Check if charset.bin is loaded.
     */
    if ((charset_alias_list == NULL) || (charset_table_list == NULL))
	charset_read_bin();
    Syslog('s', "charset1: in=%s out=%s", in, out);


    /*
     * For charset names with a space (level number), shorten the name.
     */
    for (i = 0; i < strlen(in); i++)
	if (in[i] == ' ') {
	    in[i] = '\0';
	    break;
	}

    for (i = 0; i < strlen(out); i++)
	if (out[i] == ' ') {
	    out[i] = '\0';
	    break;
	}
    Syslog('s', "charset2: in=%s out=%s", in, out);

    /* Search for aliases */
    for (pa = charset_alias_list; pa; pa=pa->next) {
        if (strcasecmp(pa->alias, in) == 0)
            in = pa->name;
        if (strcasecmp(pa->alias, out) == 0)
            out = pa->name;
    }
    Syslog('s', "charset3: in=%s out=%s", in, out);

    /* Search for matching table */
    for (pt = charset_table_list; pt; pt=pt->next) {
        if ((strcasecmp(pt->in, in) == 0) && (strcasecmp(pt->out, out) == 0)) {
            Syslog('s', "charset: table found in=%s out=%s", pt->in, pt->out);
            charset_table_used = pt;
            return TRUE;
        }
    }

    Syslog('b', "charset: no table found in=%s out=%s", in, out);
    charset_table_used = NULL;
    return FALSE;
}


