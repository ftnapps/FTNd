/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS functions for TURBODIESEL
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "users.h"
#include "records.h"
#include "common.h"
#include "clcomm.h"
#include "diesel.h"

static int firstrandom = TRUE;


void MacroVars( const char *codes, const char *fmt, ...)
{
        char *tmp1, *tmp2;
        va_list ap;
        int j;
        int dieselrc;
        char *vs;
        int vd;
        char vc;
        double vf;

        tmp1=calloc(MAXSTR, sizeof(char));
        tmp2=calloc(MAXSTR, sizeof(char));

        va_start(ap,fmt);
        for ( j=0; (codes[j] != '\0') && (fmt[j] != '\0') ; j++ ){
             tmp1[0]='\0';
             switch(fmt[j]) {
                   case 's':           /* string */
                        vs = va_arg(ap, char *);
                        sprintf(tmp1,"@(setvar,%c,\"%s\")",codes[j],vs);
                        break;
                   case 'd':           /* int */
                        vd = va_arg(ap, int);
                        sprintf(tmp1,"@(setvar,%c,%d)",codes[j],vd);
                        break;
                   case 'c':           /* char */
                        vc = va_arg(ap, int);
                        sprintf(tmp1,"@(setvar,%c,%c)",codes[j],vc);
                        break;
                   case 'f':           /* char */
                        vf = va_arg(ap, double);
                        sprintf(tmp1,"@(setvar,%c,%f)",codes[j],vf);
                        break;
                   }
                   dieselrc=diesel(tmp1,tmp2);
		   if (dieselrc) {
		       Syslog('!', "MacroVars error %d argument %d, macro %c type %c", dieselrc, j, codes[j], fmt[j]);
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
    char	*tmp1, *tmp2, *tmp3, *i;
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

    for ( i=line ; i[0] != '\0'; i++){
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
		sprintf(tmp2,"@(GETVAR,%c)",code);
		if (!diesel(tmp2,tmp3)==0){
		    sprintf(tmp3,"%c%c",'@',code);
		}
		if (l>2){
		    if ( *i != '>')
			l=-l;
		    sprintf(&tmp1[strlen(tmp1)],"%*.*s",l,l, tmp3);
		}else{
		    sprintf(&tmp1[strlen(tmp1)],"%s",tmp3);
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
    sprintf(tmp2,"%s",tmp1);

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
		sprintf(res,"%.*s",j-2,&tmp1[2]);
	    if ((diesel(res,tmp3)!=0) || (atoi(tmp3)==0))
		sprintf(tmp2,"@!%s",i);
	    else
		sprintf(tmp2,"%s",i);
	}
    }
    *dieselrc=diesel(tmp2, res);

    free(tmp1);
    free(tmp2);
    free(tmp3);
    while (isspace(res[strlen(res) - 1])) {
	res[strlen(res) - 1] = EOS;
    }
//  sprintf(&res[strlen(res)],"\r\n");
    if ((res[0] == '@') && (res[1] =='!' ))
	res[0]='\0';
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
    long    recno, records;

    MacroVars("F", "s", "");
    fname = calloc(PATH_MAX, sizeof(char));
    sprintf(fname, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

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
		html_massage(ol.Oneline, outbuf);
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
void html_massage(char *inbuf, char *outbuf)
{
        char    *inptr = inbuf;
        char    *outptr = outbuf;

        memset(outbuf, 0, sizeof(outbuf));

        while (*inptr) {

                switch ((unsigned char)*inptr) {

                        case '"':       sprintf(outptr, "&quot;");      break;
                        case '&':       sprintf(outptr, "&amp;");       break;
                        case '<':       sprintf(outptr, "&lt;");        break;
                        case '>':       sprintf(outptr, "&gt;");        break;
                        case 160:       sprintf(outptr, "&nbsp;");      break;
                        case 161:       sprintf(outptr, "&iexcl;");     break;
                        case 162:       sprintf(outptr, "&cent;");      break;
                        case 163:       sprintf(outptr, "&pound;");     break;
                        case 164:       sprintf(outptr, "&curren;");    break;
                        case 165:       sprintf(outptr, "&yen;");       break;
                        case 166:       sprintf(outptr, "&brvbar;");    break;
                        case 167:       sprintf(outptr, "&sect;");      break;
                        case 168:       sprintf(outptr, "&uml;");       break;
                        case 169:       sprintf(outptr, "&copy;");      break;
                        case 170:       sprintf(outptr, "&ordf;");      break;
                        case 171:       sprintf(outptr, "&laquo;");     break;
                        case 172:       sprintf(outptr, "&not;");       break;
                        case 173:       sprintf(outptr, "&shy;");       break;
                        case 174:       sprintf(outptr, "&reg;");       break;
                        case 175:       sprintf(outptr, "&macr;");      break;
                        case 176:       sprintf(outptr, "&deg;");       break;
                        case 177:       sprintf(outptr, "&plumin;");    break;
                        case 178:       sprintf(outptr, "&sup2;");      break;
                        case 179:       sprintf(outptr, "&sup3;");      break;
                        case 180:       sprintf(outptr, "&acute;");     break;
                        case 181:       sprintf(outptr, "&micro;");     break;
                        case 182:       sprintf(outptr, "&para;");      break;
                        case 183:       sprintf(outptr, "&middot;");    break;
                        case 184:       sprintf(outptr, "&cedil;");     break;
                        case 185:       sprintf(outptr, "&supl;");      break;
                        case 186:       sprintf(outptr, "&ordm;");      break;
                        case 187:       sprintf(outptr, "&raquo;");     break;
                        case 188:       sprintf(outptr, "&frac14;");    break;
                        case 189:       sprintf(outptr, "&frac12;");    break;
                        case 190:       sprintf(outptr, "&frac34;");    break;
                        case 191:       sprintf(outptr, "&iquest;");    break;
                        case 192:       sprintf(outptr, "&Agrave;");    break;
                        case 193:       sprintf(outptr, "&Aacute;");    break;
                        case 194:       sprintf(outptr, "&Acirc;");     break;
                        case 195:       sprintf(outptr, "&Atilde;");    break;
                        case 196:       sprintf(outptr, "&Auml;");      break;
                        case 197:       sprintf(outptr, "&Aring;");     break;
                        case 198:       sprintf(outptr, "&AElig;");     break;
                        case 199:       sprintf(outptr, "&Ccedil;");    break;
                        case 200:       sprintf(outptr, "&Egrave;");    break;
                        case 201:       sprintf(outptr, "&Eacute;");    break;
                        case 202:       sprintf(outptr, "&Ecirc;");     break;
                        case 203:       sprintf(outptr, "&Euml;");      break;
                        case 204:       sprintf(outptr, "&Igrave;");    break;
                        case 205:       sprintf(outptr, "&Iacute;");    break;
                        case 206:       sprintf(outptr, "&Icirc;");     break;
                        case 207:       sprintf(outptr, "&Iuml;");      break;
                        case 208:       sprintf(outptr, "&ETH;");       break;
                        case 209:       sprintf(outptr, "&Ntilde;");    break;
                        case 210:       sprintf(outptr, "&Ograve;");    break;
                        case 211:       sprintf(outptr, "&Oacute;");    break;
                        case 212:       sprintf(outptr, "&Ocirc;");     break;
                        case 213:       sprintf(outptr, "&Otilde;");    break;
                        case 214:       sprintf(outptr, "&Ouml;");      break;
                        case 215:       sprintf(outptr, "&times;");     break;
                        case 216:       sprintf(outptr, "&Oslash;");    break;
                        case 217:       sprintf(outptr, "&Ugrave;");    break;
                        case 218:       sprintf(outptr, "&Uacute;");    break;
                        case 219:       sprintf(outptr, "&Ucirc;");     break;
                        case 220:       sprintf(outptr, "&Uuml;");      break;
                        case 221:       sprintf(outptr, "&Yacute;");    break;
                        case 222:       sprintf(outptr, "&THORN;");     break;
                        case 223:       sprintf(outptr, "&szlig;");     break;
                        case 224:       sprintf(outptr, "&agrave;");    break;
                        case 225:       sprintf(outptr, "&aacute;");    break;
                        case 226:       sprintf(outptr, "&acirc;");     break;
                        case 227:       sprintf(outptr, "&atilde;");    break;
                        case 228:       sprintf(outptr, "&auml;");      break;
                        case 229:       sprintf(outptr, "&aring;");     break;
                        case 230:       sprintf(outptr, "&aelig;");     break;
                        case 231:       sprintf(outptr, "&ccedil;");    break;
                        case 232:       sprintf(outptr, "&egrave;");    break;
                        case 233:       sprintf(outptr, "&eacute;");    break;
                        case 234:       sprintf(outptr, "&ecirc;");     break;
                        case 235:       sprintf(outptr, "&euml;");      break;
                        case 236:       sprintf(outptr, "&igrave;");    break;
                        case 237:       sprintf(outptr, "&iacute;");    break;
                        case 238:       sprintf(outptr, "&icirc;");     break;
                        case 239:       sprintf(outptr, "&iuml;");      break;
                        case 240:       sprintf(outptr, "&eth;");       break;
                        case 241:       sprintf(outptr, "&ntilde;");    break;
                        case 242:       sprintf(outptr, "&ograve;");    break;
                        case 243:       sprintf(outptr, "&oacute;");    break;
                        case 244:       sprintf(outptr, "&ocirc;");     break;
                        case 245:       sprintf(outptr, "&otilde;");    break;
                        case 246:       sprintf(outptr, "&ouml;");      break;
                        case 247:       sprintf(outptr, "&divide;");    break;
                        case 248:       sprintf(outptr, "&oslash;");    break;
                        case 249:       sprintf(outptr, "&ugrave;");    break;
                        case 250:       sprintf(outptr, "&uacute;");    break;
                        case 251:       sprintf(outptr, "&ucirc;");     break;
                        case 252:       sprintf(outptr, "&uuml;");      break;
                        case 253:       sprintf(outptr, "&yacute;");    break;
                        case 254:       sprintf(outptr, "&thorn;");     break;
                        case 255:       sprintf(outptr, "&yuml;");      break;
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
	sprintf(temp, "%s/etc/language.data", getenv("MBSE_ROOT"));
	if ((pLang = fopen(temp, "rb")) == NULL) {
	    WriteError("mbdiesel: Can't open language file: %s", temp);
	} else {
	    fread(&langhdr, sizeof(langhdr), 1, pLang);
    
	    while (fread(&lang, langhdr.recsize, 1, pLang) == 1) {
		if ((lang.LangKey[0] == Language) && (lang.Available)) {
		    sprintf(temp,"%s/%s", lang.MacroPath, filename);
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
	sprintf(temp, "%s/%s", CFG.bbs_macros, filename);
	fi = fopen(temp,"r");
    }

    if (fi == NULL)
	WriteError("OpenMacro(%s, %c): not found", filename, Language);
    else {
	sprintf(temp, "%s-%s", OsName(), OsCPU());
	if (CFG.aka[0].point)
	    sprintf(aka, "%d:%d/%d.%d@%s", CFG.aka[0].zone, CFG.aka[0].net, CFG.aka[0].node, CFG.aka[0].point, CFG.aka[0].domain);
	else
	    sprintf(aka, "%d:%d/%d@%s", CFG.aka[0].zone, CFG.aka[0].net, CFG.aka[0].node, CFG.aka[0].domain);

	if (htmlmode) {
	    MacroVars("O", "s", temp);
	    sprintf(linebuf, "%s", CFG.sysop);
	    html_massage(linebuf, outbuf);
	    MacroVars("U", "s", outbuf);
	    sprintf(linebuf, "%s", CFG.location);
	    html_massage(linebuf, outbuf);
	    MacroVars("L", "s", outbuf);
	    sprintf(linebuf, "%s", CFG.bbs_name);
	    html_massage(linebuf, outbuf);
	    MacroVars("N", "s", outbuf);
	    sprintf(linebuf, "%s", CFG.sysop_name);
	    html_massage(linebuf, outbuf);
	    MacroVars("S", "s", outbuf);
	    sprintf(linebuf, "%s", CFG.comment);
	    html_massage(linebuf, outbuf);
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


