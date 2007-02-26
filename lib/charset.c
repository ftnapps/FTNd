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


int		use_tran1 = FALSE;	/* Translate stage 1 active	*/
int		use_tran2 = FALSE;	/* Translate stage 2 active	*/
int		loglevel = '-';		/* Debug loglevel		*/
iconv_t		cd1;			/* Conversion descriptor 1	*/
iconv_t		cd2;			/* Conversion descriptor 2	*/



/*
 * Search table for CHRS: kludge to the current name.
 * This table may include obsolete names (and we need
 * them).
 */
struct _charalias charalias[] = {
    {(char *)"ASCII",           (char *)"US-ASCII"},
    {(char *)"VT100",           (char *)"US-ASCII"},
    {(char *)"LATIN",		(char *)"LATIN-1"},
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
    {FTNC_NONE,   (char *)"Undef",    (char *)"iso-8859-1", (char *)"Undef",    (char *)"ISO-8859-1", (char *)"C",           (char *)"Undefined"},
    {FTNC_LATIN_1,(char *)"LATIN-1 2",(char *)"iso-8859-1", (char *)"LATIN1",   (char *)"ISO-8859-1", (char *)"en_US",       (char *)"ISO 8859-1 (Western European)"},
    {FTNC_CP437,  (char *)"CP437 2",  (char *)"us-ascii",   (char *)"CP437",    (char *)"ISO-8859-1", (char *)"en_US",       (char *)"IBM codepage 437 (Western European) (ANSI terminal)"},
    {FTNC_CP865,  (char *)"CP865 2",  (char *)"iso-8859-1", (char *)"CP865",    (char *)"ISO-8859-1", (char *)"sv_SE",       (char *)"IBM codepage 865 (Nordic)"},
    {FTNC_MAC,    (char *)"MAC",      (char *)"Macintosh",  (char *)"MACINTOSH",(char *)"ISO-8859-1", (char *)"en_US",       (char *)"MacIntosh character set"},
    {FTNC_CP850,  (char *)"CP850 2",  (char *)"iso-8859-1", (char *)"CP850",    (char *)"ISO-8859-1", (char *)"en_US",       (char *)"IBM codepage 850 (Latin-1)"},
    {FTNC_LATIN_2,(char *)"LATIN-2 2",(char *)"iso-8859-2", (char *)"LATIN2",   (char *)"ISO-8859-2", (char *)"cs_CZ",       (char *)"ISO 8859-2 (Eastern European)"},
    {FTNC_CP852,  (char *)"CP852 2",  (char *)"iso-8859-2", (char *)"CP852",    (char *)"ISO-8859-2", (char *)"cs_CZ",       (char *)"IBM codepage 852 (Czech, Latin-1)"},
    {FTNC_CP895,  (char *)"CP895 2",  (char *)"iso-8859-2", (char *)"CP850",    (char *)"ISO-8859-2", (char *)"cs_CZ",       (char *)"IBM codepage 895 (Czech, Kamenicky)"},
    {FTNC_LATIN_5,(char *)"LATIN-5 2",(char *)"iso-8859-5", (char *)"LATIN5",   (char *)"ISO-8859-5", (char *)"turks",       (char *)"ISO 8859-5 (Turkish)"},
    {FTNC_CP866,  (char *)"CP866 2",  (char *)"iso-8859-5", (char *)"CP866",    (char *)"ISO-8859-5", (char *)"ru_RU",       (char *)"IBM codepage 866 (Russian)"},
    {FTNC_LATIN_9,(char *)"LATIN-9 2",(char *)"iso-8859-15",(char *)"LATIN-9",  (char *)"ISO-8859-15",(char *)"en_US",       (char *)"ISO 8859-1 (Western European EURO)"},
    {FTNC_KOI8_R, (char *)"KOI8-R 2", (char *)"koi8-r",     (char *)"KOI8-R",   (char *)"KOI8-R",     (char *)"ru_RUi.koi8r",(char *)"Unix codepage KOI8-R (Russian)"},
    {FTNC_CP936,  (char *)"CP936 2",  (char *)"hz-gb-2312", (char *)"GB2312",   (char *)"GB2312",     (char *)"zh_CN.gbk",   (char *)"IBM codepage 936 (Chinese, GBK)"},
    {FTNC_UTF8,   (char *)"UTF-8 4",  (char *)"utf-8",      (char *)"UTF-8",    (char *)"UTF-8",      (char *)"en_US.UTF-8", (char *)"Unicode UTF-8 (ISO/IEC 10646)"},
    {FTNC_ERROR,  NULL,               NULL,                 NULL,               NULL,                 NULL,                  (char *)"ERROR"}
};




/*
 * Returns index of charset or -1 if not found.
 */
int find_ftn_charset(char *ftnkludge)
{   
    static int  i;
    int         j;
    char        *ftn, *cmp;

    Syslog(loglevel, "find_ftn_charset(%s)", ftnkludge);

    ftn = calloc(80, sizeof(char));
    cmp = calloc(80, sizeof(char));

    snprintf(ftn, 80, "%s", ftnkludge);

    for (i = 0; i < strlen(ftn); i++) {
	if (ftn[i] == ' ') {
	    ftn[i] = '\0';
	    break;
	}
    }
    for (i = 0; charalias[i].alias; i++) {
	if (strcasecmp(ftn, charalias[i].alias) == 0)
	    break;
    }

    if (charalias[i].alias != NULL) {
	Syslog(loglevel, "found alias %s", charalias[i].ftnkludge);
	snprintf(ftn, 80, "%s", charalias[i].ftnkludge);
    }

    /*
     * Now search real entry. Throw away the charset level number,
     * we don't care about that useless byte.
     */
    for (i = 0; charmap[i].ftnkludge; i++) {
	snprintf(cmp, 80, "%s", charmap[i].ftnkludge);
	for (j = 0; j < strlen(cmp); j++) {
	    if (cmp[j] == ' ') {
		cmp[j] = '\0';
		break;
	    }
	}
	if (strcasecmp(ftn, cmp) == 0)
	    break;
    }

    free(ftn);
    free(cmp);

    if (charmap[i].ftnkludge == NULL) {
	WriteError("find_ftn_charset(%s) not found", ftnkludge);
	return FTNC_ERROR;
    }

    Syslog(loglevel, "find_ftn_charset(%s) result %d", ftnkludge, i);
    return i;
}



/*
 * Returns index of charset or -1 if not found.
 */
int find_rfc_charset(char *rfcname)
{
    static int  i;
    
    Syslog(loglevel, "find_rfc_charset(%s)", rfcname);
    
    for (i = 0; charmap[i].rfcname; i++) {
	if (strcasecmp(rfcname, charmap[i].rfcname) == 0)
	    break;
    }
    
    if (charmap[i].rfcname == NULL) {
	WriteError("find_rfc_charset(%s) not found", rfcname);
	return FTNC_ERROR;
    }
    
    Syslog(loglevel, "find_rfc_charset(%s) result %d", rfcname, i);
    return i;
}



char *getftnchrs(int val)
{
    int		i;
    static char	kludge[20];

    for (i = 0; (charmap[i].ftncidx != FTNC_ERROR); i++) {
	if (val == charmap[i].ftncidx) {
	    snprintf(kludge, 20, "%s", charmap[i].ftnkludge);
	    return kludge;
	}
    }

    /*
     * Not found, return a default.
     */
    return (char *)"LATIN-1 2";
}



char *getrfcchrs(int val)
{
    int		i;
    static char	rfcname[20];

    for (i = 0; (charmap[i].ftncidx != FTNC_ERROR); i++) {
	if (val == charmap[i].ftncidx) {
	    snprintf(rfcname, 20, "%s", charmap[i].rfcname);
	    return rfcname;
	}
    }

    /*
     * Not found, return a default
     */
    return (char *)"iso-8859-1";
}



char *get_ic_ftn(int val)
{
    int         i;
    static char ic_ftnname[20];
    
    for (i = 0; (charmap[i].ftncidx != FTNC_ERROR); i++) {
	if (val == charmap[i].ftncidx) {
	    snprintf(ic_ftnname, 20, "%s", charmap[i].ic_ftn);
	    return ic_ftnname;
	}
    }
    
    /*
     * Not found, return a default
     */
    return (char *)"LATIN1";
}



char *get_ic_rfc(int val)
{
    int         i;
    static char ic_rfcname[20];
    
    for (i = 0; (charmap[i].ftncidx != FTNC_ERROR); i++) {
	if (val == charmap[i].ftncidx) {
	    snprintf(ic_rfcname, 20, "%s", charmap[i].ic_rfc);
	    return ic_rfcname;
	}
    }
    
    /*
     * Not found, return a default
     */
    return (char *)"iso-8859-1";
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
    int		i;
    static char	langc[20];

    for (i = 0; (charmap[i].ftncidx != FTNC_ERROR); i++) {
	if (val == charmap[i].ftncidx) {
	    snprintf(langc, 20, "%s", charmap[i].lang);
	    return langc;
	}
    }

    return (char *)"C";
}



char *getchrsdesc(int val)
{
    int		i;
    static char	desc[60];

    for (i = 0; (charmap[i].ftncidx != FTNC_ERROR); i++) {
	if (val == charmap[i].ftncidx) {
	    snprintf(desc, 60, "%s", charmap[i].desc);
	    return desc;
	}
    }

    return (char *)"ERROR";
}



/*
 * Initialize charset translation. Translation can be done in 2 stages
 * with UTF-8 as the common centre because for example translate between
 * CP438 and ISO-8859-1 doesn't work directly. If translation is needed
 * with one side is UTF-8, only one stage will be used. If two the same
 * charactersets are given, the translation is off.
 * On success return 0, on error return -1 and write errorlog.
 */
int chartran_init(char *fromset, char *toset, int logl)
{
    loglevel = logl;

    if (use_tran1 || use_tran2) {
	WriteError("chartran_init() called while still open");
	chartran_close();
    }

    Syslog(loglevel, "chartran_init(%s, %s)", fromset, toset);

    if (strcmp(fromset, toset) == 0) {
	Syslog(loglevel, "nothing to translate");
	return 0;
    }

    if (strcmp(fromset, (char *)"UTF-8")) {
	cd1 = iconv_open("UTF-8", fromset);
	if (cd1 == (iconv_t)-1) {
	    WriteError("$chartran_init(%s, %s): iconv_open(UTF-8, %s) error", fromset, toset, fromset);
	    return -1;
	}
	use_tran1 = TRUE;
    }

    if (strcmp(toset, (char *)"UTF-8")) {
	cd2 = iconv_open(toset, "UTF-8");
	if (cd2 == (iconv_t)-1) {
	    WriteError("$chartran_init(%s, %s): iconv_open(%s, UTF-8) error", fromset, toset, toset);
	    if (use_tran1) {
		iconv_close(cd1);
		use_tran1 = FALSE;
	    }
	    return -1;
	}
	use_tran2 = TRUE;
    }

    return 0;
}



/*
 * Deinit active charset translation.
 */
void chartran_close(void)
{
    Syslog(loglevel, "chartran_close()");
    if (use_tran1) {
	iconv_close(cd1);
	use_tran1 = FALSE;
    }

    if (use_tran2) {
	iconv_close(cd2);
	use_tran2 = FALSE;
    }
}



/*
 * Translate a string, chartran_init must have been called to register
 * the charactersets to translate between.
 */
char *chartran(char *input)
{
    static char	outbuf[4096];
    static char	temp[4096];
    size_t	rc, inSize, outSize;
    char	*in, *out;

    memset(&outbuf, 0, sizeof(outbuf));
    memset(&temp, 0, sizeof(temp));

    /*
     * Transparant
     */
    if (!use_tran1 && !use_tran2) {
	strncpy(outbuf, input, sizeof(outbuf) -1);
	return outbuf;
    }

    /*
     * Translate to UTF-8
     */
    if (use_tran1 && !use_tran2) {
	inSize = strlen(input);
	outSize = sizeof(outbuf);
	in = input;
	out = outbuf;
	rc = iconv(cd1, &in, &inSize, &out, &outSize);
	if (rc == -1) {
	    WriteError("$iconv(%s) cd1", printable(input, 0));
	    strncpy(outbuf, input, sizeof(outbuf) -1);
	}
	if (strcmp(input, outbuf)) {
	    Syslog(loglevel, "i %s", printable(input, 0));
	    Syslog(loglevel, "u %s", printable(outbuf, 0));
	}
	return outbuf;
    }

    /*
     * Translate from UTF-8
     */
    if (!use_tran1 && use_tran2) {
	inSize = strlen(input);
	outSize = sizeof(outbuf);
	in = input;
	out = outbuf;
	rc = iconv(cd2, &in, &inSize, &out, &outSize);
	if (rc == -1) {
	    WriteError("$iconv(%s) cd2", printable(input, 0));
	    strncpy(outbuf, input, sizeof(outbuf) -1);
	}
	if (strcmp(input, outbuf)) {
	    Syslog(loglevel, "u %s", printable(input, 0));
	    Syslog(loglevel, "o %s", printable(outbuf, 0));
	}
	return outbuf;
    }

    /*
     * Double translation via UTF-8
     */
    inSize = strlen(input);
    outSize = sizeof(temp);
    in = input;
    out = temp;
    rc = iconv(cd1, &in, &inSize, &out, &outSize);
    if (rc == -1) {
	WriteError("$iconv(%s) cd1", printable(input, 0));
	strncpy(outbuf, input, sizeof(outbuf) -1);
	return outbuf;
    }
    if (strcmp(input, temp)) {
	Syslog(loglevel, "i %s", printable(input, 0));
    }

    inSize = strlen(temp);
    outSize = sizeof(outbuf);
    in = temp;
    out = outbuf;
    rc = iconv(cd2, &in, &inSize, &out, &outSize);
    if (rc == -1) {
	WriteError("$iconv(%s) cd2", printable(temp, 0));
	strncpy(outbuf, input, sizeof(outbuf) -1);
    }
    if (strcmp(input, temp) || strcmp(temp, outbuf)) {
	Syslog(loglevel, "u %s", printable(temp, 0));
    }
    if (strcmp(temp, outbuf)) {
	Syslog(loglevel, "o %s", printable(outbuf, 0));
    }

    return outbuf;
}


