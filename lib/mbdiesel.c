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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "users.h"
#include "records.h"
#include "common.h"
#include "clcomm.h"
#include "diesel.h"


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

        tmp1=calloc(256,sizeof(char));
        tmp2=calloc(256,sizeof(char));

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
                        vc = va_arg(ap, char);
                        sprintf(tmp1,"@(setvar,%c,%c)",codes[j],vc);
                        break;
                   case 'f':           /* char */
                        vf = va_arg(ap, double);
                        sprintf(tmp1,"@(setvar,%c,%f)",codes[j],vf);
                        break;
                   }
                   dieselrc=diesel(tmp1,tmp2);
		   if (dieselrc) {
		       Syslog('!', "MacroVars error %d argument %d", dieselrc, j);
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
    static char	res[256];
    char	*tmp1, *tmp2, *tmp3, *i;
    int		j, l;
    char	code;

    res[0]='\0';
    *dieselrc=0;

    if ( *line == '#' )
	return res;

    tmp1 = calloc(256,sizeof(char));
    tmp2 = calloc(256,sizeof(char));
    tmp3 = calloc(256,sizeof(char));

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



FILE *OpenMacro(const char *filename, int Language)
{
    FILE	*pLang, *fi = NULL;
    char	*temp;
		            
    temp = calloc(PATH_MAX, sizeof(char));
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
	sprintf(temp, "%s/%s", CFG.bbs_macros, filename);
	fi = fopen(temp,"r");
    }

    if (fi == NULL)
	WriteError("OpenMacro(%s, %c): not found", filename, Language);
    else {
	Syslog('d', "OpenMacro(%s, %c): using %s", filename, Language, temp);
	sprintf(temp, "%s-%s", OsName(), OsCPU());
	MacroVars("HLMNOSTUVYZ", "ssssssssssd", CFG.www_url, CFG.location, CFG.sysdomain, CFG.bbs_name, temp,
					    CFG.sysop_name, CFG.comment, CFG.sysop, VERSION, aka2str(CFG.aka[0]), 0);
    }

    free(temp);
    return fi;
}


