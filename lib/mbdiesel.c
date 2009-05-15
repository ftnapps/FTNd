/*****************************************************************************
 *
 * $Id: mbdiesel.c,v 1.29 2007/03/03 14:28:40 mbse Exp $
 * Purpose ...............: MBSE BBS functions for TURBODIESEL
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
#include "diesel.h"


static int firstrandom = TRUE;


void MacroVars( const char *codes, const char *fmt, ...)
{
    char    *tmp1, *tmp2, *vs, vc;
    va_list ap;
    int	    j, dieselrc, vd;
    double  vf;

    tmp1 = calloc(MAXSTR, sizeof(char));
    tmp2 = calloc(MAXSTR, sizeof(char));

    va_start(ap,fmt);
    for (j = 0; (codes[j] != '\0') && (fmt[j] != '\0') ; j++ ){
        tmp1[0] = '\0';
        switch (fmt[j]) {
	    case 's':   /* string */
                        vs = va_arg(ap, char *);
                        snprintf(tmp1, MAXSTR -1, "@(setvar,%c,\"%s\")",codes[j], clencode(vs));
                        break;
	    case 'd':   /* int */
                        vd = va_arg(ap, int);
                        snprintf(tmp1, MAXSTR -1, "@(setvar,%c,%d)",codes[j],vd);
                        break;
            case 'c':   /* char */
                        vc = va_arg(ap, int);
                        snprintf(tmp1, MAXSTR -1, "@(setvar,%c,%c)",codes[j],vc);
                        break;
            case 'f':   /* float */
                        vf = va_arg(ap, double);
                        snprintf(tmp1, MAXSTR -1, "@(setvar,%c,%f)",codes[j],vf);
                        break;
	}
        dieselrc = diesel(tmp1,tmp2);
	if (dieselrc) {
	    Syslog('!', "MacroVars error %d argument %d, macro %c type %c", dieselrc, j, codes[j], fmt[j]);
	    Syslogp('!', printable(tmp1, 0));
	}
    }
    va_end(ap);

    free(tmp1);
    free(tmp2);
}



void MacroClear(void)
{
    int	    dieselrc;
    char    tmp1[] = "@(CLEAR)", *tmp2;

    tmp2 = calloc(10,sizeof(char));
    dieselrc = diesel(tmp1, tmp2);
    if (dieselrc)
	Syslog('!', "MacroClear error %d", dieselrc);
    free(tmp2);
}



char *ParseMacro( const char *line, int *dieselrc)
{
    static char	res[MAXSTR];
    const char	*i;
    char	*tmp1, *tmp2, *tmp3;
    int		j, l;
    char	code;

    res[0]='\0';
    *dieselrc=0;

    if ( *line == '#' )
	return res;

    tmp1 = calloc(MAXSTR, sizeof(char));
    tmp2 = calloc(MAXSTR, sizeof(char));
    tmp3 = calloc(MAXSTR, sizeof(char));

    tmp1[0]='\0';

    for (i = line; i[0] != '\0'; i++) {
	if ( (i[0] == '@') && isalpha(i[1]) ){
	    l=2;
	    i++;
	    if (i[0] != '@') {
		if ((code = i[0]) != '\0' )
		    i++;
		while (( i[0] == '_') || ( i[0] == '>') || ( i[0] == '<') ){
		    l++;
		    i++;
		}
		i--;
		snprintf(tmp2, MAXSTR, "@(GETVAR,%c)",code);
		if (!diesel(tmp2,tmp3)==0){
		    snprintf(tmp3, MAXSTR, "%c%c",'@',code);
		}
		if (l>2){
		    if ( *i != '>')
			l=-l;
		    snprintf(&tmp1[strlen(tmp1)], MAXSTR, "%*.*s", l, l, tmp3);
		}else{
		    snprintf(&tmp1[strlen(tmp1)], MAXSTR, "%s", tmp3);
		}
	    }else{
		tmp1[(j=strlen(tmp1))]='@';
		tmp1[j+1]='\0';
	    }
	}else{
	    tmp1[(j=strlen(tmp1))]=i[0];
	    tmp1[j+1]='\0';
	}
    }

    i = tmp1;
    snprintf(tmp2, MAXSTR, "%s", tmp1);

    if ((tmp1[0]=='@') && (tmp1[1]=='{')){
	i++;
	i++;
	for (j=2; ((tmp1[j]!='}') && (tmp1[j]!='\0'));j++){
	    i++;
	}
	if ( (tmp1[j]=='}') ){
	    i++;
	    res[0]='\0';
	    if (j>2)
		snprintf(res, MAXSTR, "%.*s",j-2, &tmp1[2]);
	    if ((diesel(res,tmp3)!=0) || (atoi(tmp3)==0))
		snprintf(tmp2, MAXSTR, "@!%s",i);
	    else
		snprintf(tmp2, MAXSTR, "%s",i);
	}
    }
    *dieselrc=diesel(tmp2, res);

    free(tmp1);
    free(tmp2);
    free(tmp3);
    while (isspace(res[strlen(res) - 1])) {
	res[strlen(res) - 1] = EOS;
    }
    if ((res[0] == '@') && (res[1] =='!' ))
	res[0]='\0';

    cldecode(res);
    return res;
}



/*
 * Add random fortune cookie to the macrovars
 */
void Cookie(int);
void Cookie(int HtmlMode)
{
    FILE    *olf;
    char    *fname, outbuf[256];
    int	    recno, records;

    MacroVars("F", "s", "");
    fname = calloc(PATH_MAX, sizeof(char));
    snprintf(fname, PATH_MAX -1, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((olf = fopen(fname, "r")) == NULL) {
	WriteError("Can't open %s", fname);
	free(fname);
	return;
    }

    fread(&olhdr, sizeof(olhdr), 1, olf);
    fseek(olf, 0, SEEK_END);
    records = (ftell(olf) - olhdr.hdrsize) / olhdr.recsize;

    if (firstrandom) {
	srand(getpid());
	firstrandom = FALSE;
    }
    recno = (1+(int) (1.0 * records * rand() / (RAND_MAX + 1.0))) - 1;

    if (fseek(olf, olhdr.hdrsize + (recno * olhdr.recsize), SEEK_SET) == 0) {
	if (fread(&ol, olhdr.recsize, 1, olf) == 1) {
	    if (HtmlMode) {
		html_massage(ol.Oneline, outbuf, 255);
		MacroVars("F", "s", outbuf);
	    } else {
		MacroVars("F", "s", ol.Oneline);
	    }
	} else {
	    WriteError("Can't read record %d from %s", recno, fname);
	}
    } else {
	WriteError("Can't seek record %d in %s", recno, fname);
    }
    fclose(olf);
    free(fname);

    return;
}



/*
 * Translate ISO 8859-1 characters to named character entities
 */
void html_massage(char *inbuf, char *outbuf, size_t size)
{
        char    *inptr = inbuf;
        char    *outptr = outbuf;

        memset(outbuf, 0, sizeof(outbuf));

        while (*inptr) {

                switch ((unsigned char)*inptr) {

                        case '"':       snprintf(outptr, size, "&quot;");      break;
                        case '&':       snprintf(outptr, size, "&amp;");       break;
                        case '<':       snprintf(outptr, size, "&lt;");        break;
                        case '>':       snprintf(outptr, size, "&gt;");        break;
                        default:        *outptr++ = *inptr; *outptr = '\0';     break;
                }
                while (*outptr)
                        outptr++;

                inptr++;
        }
        *outptr = '\0';
}



FILE *OpenMacro(const char *filename, int Language, int htmlmode)
{
    FILE	*pLang, *fi = NULL;
    char	*temp, *aka, linebuf[1024], outbuf[1024];
		            
    temp = calloc(PATH_MAX, sizeof(char));
    aka  = calloc(81, sizeof(char));
    temp[0] = '\0';

    if (Language != '\0') {
	/*
	 * Maybe a valid language character, try to load the language
	 */
	snprintf(temp, PATH_MAX -1, "%s/etc/language.data", getenv("MBSE_ROOT"));
	if ((pLang = fopen(temp, "rb")) == NULL) {
	    WriteError("mbdiesel: Can't open language file: %s", temp);
	} else {
	    fread(&langhdr, sizeof(langhdr), 1, pLang);
    
	    while (fread(&lang, langhdr.recsize, 1, pLang) == 1) {
		if ((lang.LangKey[0] == Language) && (lang.Available)) {
		    snprintf(temp, PATH_MAX -1, "%s/share/int/macro/%s/%s", getenv("MBSE_ROOT"), lang.lc, filename);
		    break;
		}
	    }
	    fclose(pLang);
	}
    }
    
    /*
     * Try to open the selected language
     */
    if (temp[0] != '\0')
	fi = fopen(temp, "r");

    /*
     * If no selected language is loaded, try default language
     */
    if (fi == NULL) {
	Syslog('-', "Macro file \"%s\" for language %c not found, trying default", filename, Language);
	snprintf(temp, PATH_MAX -1, "%s/share/int/macro/%s/%s", getenv("MBSE_ROOT"), CFG.deflang, filename);
	fi = fopen(temp,"r");
    }

    if (fi == NULL)
	WriteError("OpenMacro(%s, %c): not found", filename, Language);
    else {
	/*
	 * Check macro file for update correct charset.
	 */
	while (fgets(linebuf, sizeof(linebuf) -1, fi)) {
	    if (strcasestr(linebuf, (char *)"text/html")) {
		if (! strcasestr(linebuf, (char *)"UTF-8")) {
		    WriteError("Macro file %s doesn't define 'Content-Type' content='text/html; charset=UTF-8'", temp);
		}
	    }
	}
	rewind(fi);

	snprintf(temp, PATH_MAX -1, "%s-%s", OsName(), OsCPU());
	if (CFG.aka[0].point)
	    snprintf(aka, 80, "%d:%d/%d.%d@%s", CFG.aka[0].zone, CFG.aka[0].net, CFG.aka[0].node, CFG.aka[0].point, CFG.aka[0].domain);
	else
	    snprintf(aka, 80, "%d:%d/%d@%s", CFG.aka[0].zone, CFG.aka[0].net, CFG.aka[0].node, CFG.aka[0].domain);

	if (htmlmode) {
	    MacroVars("O", "s", temp);
	    snprintf(linebuf, 1024, "%s", CFG.sysop);
	    html_massage(linebuf, outbuf, 1024);
	    MacroVars("U", "s", outbuf);
	    snprintf(linebuf, 1024, "%s", CFG.location);
	    html_massage(linebuf, outbuf, 1024);
	    MacroVars("L", "s", outbuf);
	    snprintf(linebuf, 1024, "%s", CFG.bbs_name);
	    html_massage(linebuf, outbuf, 1024);
	    MacroVars("N", "s", outbuf);
	    snprintf(linebuf, 1024, "%s", CFG.sysop_name);
	    html_massage(linebuf, outbuf, 1024);
	    MacroVars("S", "s", outbuf);
	    snprintf(linebuf, 1024, "%s", CFG.comment);
	    html_massage(linebuf, outbuf, 1024);
	    MacroVars("T", "s", outbuf);
	} else {
	    MacroVars("L", "s", CFG.location);
	    MacroVars("N", "s", CFG.bbs_name);
	    MacroVars("O", "s", temp);
	    MacroVars("S", "s", CFG.sysop_name);
	    MacroVars("T", "s", CFG.comment);
	    MacroVars("U", "s", CFG.sysop);
	}
	MacroVars("H", "s", CFG.www_url);
	MacroVars("M", "s", CFG.sysdomain);
	MacroVars("V", "s", VERSION);
	MacroVars("Y", "s", aka);
	MacroVars("Z", "d", 0);
	Cookie(htmlmode);
    }

    free(aka);
    free(temp);
    return fi;
}


