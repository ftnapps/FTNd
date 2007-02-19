/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Characterset functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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


/*
 * Search table for CHRS: kludge to the current name.
 * This table may include obsolete names (and we need
 * them).
 */
struct _charalias charalias[] = {
    {(char *)"ASCII",           (char *)"US-ASCII"},
    {(char *)"VT100",           (char *)"US-ASCII"},
    {(char *)"AMIGA",           (char *)"CP437"},
    {(char *)"IBMPC",           (char *)"CP437"},
    {(char *)"PC-8",            (char *)"CP437"},
    {(char *)"CP850",           (char *)"CP437"},
    {(char *)"MACINTOSH",       (char *)"MAC"},
    {(char *)"ALT",             (char *)"CP866"},
    {(char *)"X-ALT",           (char *)"CP866"},
    {(char *)"X-CP866",         (char *)"CP866"},
    {(char *)"+7_FIDO",         (char *)"CP866"},
    {(char *)"KOI8-U",          (char *)"KOI8-R"},
    {(char *)"IBM-878",         (char *)"KOI8-R"},
    {(char *)"CP878",           (char *)"KOI8-R"},
    {(char *)"IBM-915",         (char *)"ISO-8859-1"},
    {(char *)"X-CP1251",        (char *)"CP1251"},
    {(char *)"GBK",             (char *)"CP936"},
    {(char *)"HZ-GB-2312",      (char *)"CP936"},
    {NULL,                      NULL}
};



/*
 * Array of charset identifiers. Order is important for reverse
 * search from rfc -> ftn, best ftn kludge should be on top.
 */
struct _charmap charmap[] = {
    {FTNC_LATIN_1, (char *)"LATIN-1 2", (char *)"iso-8859-1",  (char *)"LATIN1",    (char *)"ISO-8859-1",  (char *)"en_US"},
    {FTNC_CP437,   (char *)"CP437 2",   (char *)"iso-8859-1",  (char *)"CP437",     (char *)"ISO-8859-1",  (char *)"en_US"},
    {FTNC_CP865,   (char *)"CP865 2",   (char *)"iso-8859-1",  (char *)"CP865",     (char *)"ISO-8859-1",  (char *)"sv_SE"},
    {FTNC_MAC,     (char *)"MAC",       (char *)"Macintosh",   (char *)"MACINTOSH", (char *)"ISO-8859-1",  (char *)"en_US"},
    {FTNC_CP850,   (char *)"CP850 2",   (char *)"iso-8859-1",  (char *)"CP850",     (char *)"ISO-8859-1",  (char *)"en_US"},
    {FTNC_LATIN_2, (char *)"LATIN-2 2", (char *)"iso-8859-2",  (char *)"LATIN2",    (char *)"ISO-8859-2",  (char *)"cs_CZ"},
    {FTNC_CP852,   (char *)"CP852 2",   (char *)"iso-8859-2",  (char *)"CP852",     (char *)"ISO-8859-2",  (char *)"cs_CZ"},
    {FTNC_CP895,   (char *)"CP895 2",   (char *)"iso-8859-2",  (char *)"CP895",     (char *)"ISO-8859-2",  (char *)"cs_CZ"},
    {FTNC_LATIN_5, (char *)"LATIN-5 2", (char *)"iso-8859-5",  (char *)"LATIN5",    (char *)"ISO-8859-5",  (char *)"turks"},
    {FTNC_CP866,   (char *)"CP866 2",   (char *)"iso-8859-5",  (char *)"CP866",     (char *)"ISO-8859-5",  (char *)"ru_RU"},
    {FTNC_LATIN_9, (char *)"LATIN-9 2", (char *)"iso-8859-15", (char *)"LATIN9",    (char *)"ISO-8859-15", (char *)"en_US"},
    {FTNC_KOI8_R,  (char *)"KOI8-R 2",  (char *)"koi8-r",      (char *)"KOI8-R",    (char *)"KOI8-R",      (char *)"ru_RUi.koi8r"},
    {FTNC_CP936,   (char *)"CP936 2",   (char *)"hz-gb-2312",  (char *)"GB2312",    (char *)"GB2312",      (char *)"zh_CN.gbk"},
    {FTNC_NONE,    NULL,                NULL,                  NULL,                NULL,                  NULL}
};



#ifndef USE_EXPERIMENT

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
		    
    n = vsnprintf(buf, len, fmt, args);
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

#endif

char *getftnchrs(int val)
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
	case FTNC_KOI8_R:   return (char *)"KOI8-R 2";
	case FTNC_CP936:    return (char *)"CP936 2";
        default:            return (char *)"LATIN-1 2";
    }
}



char *getrfcchrs(int val)
{
    switch (val) {
	case FTNC_NONE:     return (char *)"iso-8859-1";
	case FTNC_CP437:    return (char *)"cp437";
	case FTNC_CP850:    return (char *)"cp850";
	case FTNC_CP865:    return (char *)"cp865";
	case FTNC_CP866:    return (char *)"cp866";
	case FTNC_CP852:    return (char *)"cp852";
	case FTNC_CP895:    return (char *)"cp895";
	case FTNC_LATIN_1:  return (char *)"iso-8859-1";
	case FTNC_LATIN_2:  return (char *)"iso-8859-2";
	case FTNC_LATIN_5:  return (char *)"iso-8859-5";
	case FTNC_MAC:      return (char *)"Macintosh";
	case FTNC_KOI8_R:   return (char *)"koi8-r";
	case FTNC_CP936:    return (char *)"hz-gb-2312";
	default:            return (char *)"iso-8859-1";
    }
}



/*
 * Experimental table that should translate from the user selected
 * charset to a locale. This is not the right way to do, the best
 * thing is to store each bbs users locale instead and then lookup
 * his characterset using standard library calls.
 *
 * This is one of the things the bbs world never saw coming, in the
 * "good" old days bbses were almost allways called local. Thanks
 * to the internet bbs users are now all over the world.
 */
char *getlocale(int val)
{
    switch (val) {
	case FTNC_NONE:	    return (char *)"C";
	case FTNC_CP437:    return (char *)"en_US";
	case FTNC_CP850:    return (char *)"en_US";
	case FTNC_CP865:    return (char *)"sv_SE";
	case FTNC_CP866:    return (char *)"ru_RU";
	case FTNC_CP852:    return (char *)"cs_CZ";
	case FTNC_CP895:    return (char *)"cs_CZ";
	case FTNC_LATIN_1:  return (char *)"en_US";
	case FTNC_LATIN_2:  return (char *)"cs_CZ";
	case FTNC_MAC:	    return (char *)"en_US";
	case FTNC_KOI8_R:   return (char *)"ru_RU.koi8r";
	case FTNC_CP936:    return (char *)"zh_CN.gbk";
	default:	    return (char *)"C";
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
	case FTNC_KOI8_R:   return (char *)"Unix codepage KOI8-R (Russian)";
	case FTNC_CP936:    return (char *)"IBM codepage 936 (Chinese, GBK)";
        default:            return (char *)"ERROR";
    }
}


#ifndef USE_EXPERIMENT

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
    snprintf(name, PATH_MAX -1, "%s/etc/charset.bin", getenv("MBSE_ROOT"));
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
//					Syslog('s', "read charset alias: %s -> %s", pa->alias, pa->name);
					break;
	    case CHARSET_FILE_TABLE:	pt = charset_table_new();
					n = fread((void *)pt, sizeof(CharsetTable), 1, fp);
					pt->next = NULL;                    /* overwritten by fread() */
					if (n != 1) 
					    return FALSE;
//					Syslog('s', "read charset table: %s -> %s", pt->in, pt->out);
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

    /* Search for aliases */
    for (pa = charset_alias_list; pa; pa=pa->next) {
        if (strcasecmp(pa->alias, in) == 0)
            in = pa->name;
        if (strcasecmp(pa->alias, out) == 0)
            out = pa->name;
    }
    Syslog('m', "charset: aliases in=%s out=%s", in, out);

    /* Search for matching table */
    for (pt = charset_table_list; pt; pt=pt->next) {
        if ((strcasecmp(pt->in, in) == 0) && (strcasecmp(pt->out, out) == 0)) {
            Syslog('s', "charset: table found in=%s out=%s", pt->in, pt->out);
            charset_table_used = pt;
            return TRUE;
        }
    }

    Syslog('s', "charset: no table found in=%s out=%s", in, out);
    charset_table_used = NULL;
    return FALSE;
}

#endif

