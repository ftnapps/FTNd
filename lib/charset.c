/*****************************************************************************
 *
 * File ..................: charset.c
 * Purpose ...............: Common utilities
 * Last modification date : 09-Sep-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "common.h"
#include "clcomm.h"


/* ### Created by P.Saratxaga on 7 Nov 1995 ###
 * Functions for charset reading
 * - bugfix for bad Content-Type lines lacking ";". By Marc Schaeffer.
 */

int	defaultrfcchar = CHRS_DEFAULT_RFC;
int	defaultftnchar = CHRS_DEFAULT_FTN;
int	toftnchar = CHRS_NOTSET;


//#ifndef HAVE_STRCASESTR
//char *strcasestr(char *, char *);
//#endif


/* tailor getoutcode() and getincode() to show your transcodage preferences */

int getoutcode(int code)	/* rfc -> FTN */
{
	if ((code==CHRS_MACINTOSH) && (toftnchar!=CHRS_NOTSET))
		return toftnchar;
	else if (code==CHRS_MACINTOSH) return CHRS_ISO_8859_1;
	else if ((toftnchar!=CHRS_NOTSET) && (code==defaultrfcchar)) 
		return toftnchar;
	else if (code==CHRS_UTF_7||code==CHRS_UTF_8) return CHRS_AUTODETECT;
	else if (code==CHRS_ZW) return CHRS_GB;
	else return code;
}



int getincode(int code)	/* FTN -> rfc */
{
	if (code==CHRS_CP437) return CHRS_ISO_8859_1;
	else if (code==CHRS_CP850) return CHRS_ISO_8859_1;
	else if (code==CHRS_CP852) return CHRS_ISO_8859_2;
	else if (code==CHRS_CP862) return CHRS_ISO_8859_8;
	else if (code==CHRS_CP866) return CHRS_KOI8_R;
	else if (code==CHRS_CP895) return CHRS_ISO_8859_2;
	else if (code==CHRS_EUC_JP) return CHRS_ISO_2022_JP;
/*	else if (code==CHRS_EUC_KR) return CHRS_ISO_2022_KR; */	
	else if (code==CHRS_FIDOMAZOVIA) return CHRS_ISO_8859_2;
	else if (code==CHRS_ISO_11) return CHRS_ISO_8859_1;
	else if (code==CHRS_ISO_4) return CHRS_ISO_8859_1;
	else if (code==CHRS_ISO_60) return CHRS_ISO_8859_1;
	else if (code==CHRS_ISO_8859_1_QP) return CHRS_ISO_8859_1_QP;
	else if (code==CHRS_MACINTOSH) return CHRS_ISO_8859_1;
	else if (code==CHRS_MIK_CYR) return CHRS_ISO_8859_5;
	else if (code==CHRS_SJIS) return CHRS_ISO_2022_JP;
	else if (code==defaultftnchar) return CHRS_AUTODETECT;
	else return code;
}



char *getcharset(int code)
{
	char *charset;

	if (code==CHRS_ASCII) charset=(char *)"us-ascii";
	else if (code==CHRS_BIG5) charset=(char *)"x-CN-Big5";
	else if (code==CHRS_CP424) charset=(char *)"x-cp424";
	else if (code==CHRS_CP437) charset=(char *)"x-cp437";
	else if (code==CHRS_CP850) charset=(char *)"x-cp850";
	else if (code==CHRS_CP852) charset=(char *)"x-cp852";
	else if (code==CHRS_CP862) charset=(char *)"x-cp862";
	else if (code==CHRS_CP866) charset=(char *)"x-cp866";
	else if (code==CHRS_CP895) charset=(char *)"x-cp895";
	else if (code==CHRS_EUC_JP) charset=(char *)"EUC-jp";
	else if (code==CHRS_EUC_KR) charset=(char *)"EUC-kr";
	else if (code==CHRS_FIDOMAZOVIA) charset=(char *)"x-FIDOMAZOVIA";
	else if (code==CHRS_GB) charset=(char *)"x-CN-GB";
	else if (code==CHRS_HZ) charset=(char *)"x-HZ";
	else if (code==CHRS_ISO_2022_CN) charset=(char *)"iso-2022-cn";
	else if (code==CHRS_ISO_2022_JP) charset=(char *)"iso-2022-jp";
	else if (code==CHRS_ISO_2022_KR) charset=(char *)"iso-2022-kr";
	else if (code==CHRS_ISO_2022_TW) charset=(char *)"iso-2022-tw";
	else if (code==CHRS_ISO_8859_1) charset=(char *)"iso-8859-1";
	else if (code==CHRS_ISO_8859_1_QP) charset=(char *)"iso-8859-1";
	else if (code==CHRS_ISO_8859_2) charset=(char *)"iso-8859-2";
	else if (code==CHRS_ISO_8859_3) charset=(char *)"iso-8859-3";
	else if (code==CHRS_ISO_8859_4) charset=(char *)"iso-8859-4";
	else if (code==CHRS_ISO_8859_5) charset=(char *)"iso-8859-5";
	else if (code==CHRS_ISO_8859_6) charset=(char *)"iso-8859-6";
	else if (code==CHRS_ISO_8859_7) charset=(char *)"iso-8859-7";
	else if (code==CHRS_ISO_8859_8) charset=(char *)"iso-8859-8";
	else if (code==CHRS_ISO_8859_9) charset=(char *)"iso-8859-5";
	else if (code==CHRS_ISO_8859_10) charset=(char *)"iso-8859-10";
	else if (code==CHRS_ISO_8859_11) charset=(char *)"iso-8859-11";
	else if (code==CHRS_ISO_8859_15) charset=(char *)"iso-8859-15";
	else if (code==CHRS_KOI8_R) charset=(char *)"koi8-r";
	else if (code==CHRS_KOI8_U) charset=(char *)"koi8-u";
	else if (code==CHRS_MACINTOSH) charset=(char *)"x-mac-roman";
	else if (code==CHRS_MIK_CYR) charset=(char *)"x-mik-cyr";
	else if (code==CHRS_NEC) charset=(char *)"x-NEC-JIS";
	else if (code==CHRS_SJIS) charset=(char *)"x-sjis";
	else if (code==CHRS_UTF_7) charset=(char *)"utf-7";
	else if (code==CHRS_UTF_8) charset=(char *)"utf-8";
	else if (code==CHRS_VISCII_11) charset=(char *)"viscii";
	else if (code==CHRS_ZW) charset=(char *)"x-zW";
	else charset=(char *)"us-ascii"; /* mime default */

	return charset;
}



int getcode(char *p)
{
	int code;

	while (*p && isspace(*p)) p++;
	if (strncmp(p,"\"",1) == 0) p++;
/*	if (strncasecmp(p,"us-ascii",8) == 0) code=CHRS_ASCII; */
/* most newsreaders/mail user agents are misconfigured and put "us-ascii" when
   in fact they use the local charset. */
	if (strncasecmp(p,"us-ascii",8) == 0) code=defaultrfcchar;
	else if (strncasecmp(p,"CN-GB",5) == 0) code=CHRS_GB;
	else if (strncasecmp(p,"CN-Big5",7) == 0) code=CHRS_BIG5;
	else if (strncasecmp(p,"EUC-jp",6) == 0) code=CHRS_EUC_JP;
	else if (strncasecmp(p,"EUC-kr",6) == 0) code=CHRS_EUC_KR;
	else if (strncasecmp(p,"iso-2022-cn",11) == 0) code=CHRS_ISO_2022_CN;
	else if (strncasecmp(p,"iso-2022-jp",11) == 0) code=CHRS_ISO_2022_JP;
	else if (strncasecmp(p,"iso-2022-kr",11) == 0) code=CHRS_ISO_2022_KR;
	else if (strncasecmp(p,"iso-2022-tw",11) == 0) code=CHRS_ISO_2022_TW;
	else if (strncasecmp(p,"iso8859-1",9) == 0) code = CHRS_ISO_8859_1; /* erroneous iso8859-1 */
	else if (strncasecmp(p,"iso-8859-10",11) == 0) code=CHRS_ISO_8859_10;
	else if (strncasecmp(p,"iso-8859-11",11) == 0) code=CHRS_ISO_8859_11;
	else if (strncasecmp(p,"iso-8859-15",11) == 0) code=CHRS_ISO_8859_15;
	else if (strncasecmp(p,"iso-8859-1",10) == 0) code = CHRS_ISO_8859_1;
	else if (strncasecmp(p,"iso-8859-2",10) == 0) code=CHRS_ISO_8859_2;
	else if (strncasecmp(p,"iso-8859-3",10) == 0) code=CHRS_ISO_8859_3;
	else if (strncasecmp(p,"iso-8859-4",10) == 0) code=CHRS_ISO_8859_4;
	else if (strncasecmp(p,"iso-8859-5",10) == 0) code=CHRS_ISO_8859_5;
	else if (strncasecmp(p,"iso-8859-6",10) == 0) code=CHRS_ISO_8859_6;
	else if (strncasecmp(p,"iso-8859-7",10) == 0) code=CHRS_ISO_8859_7;
	else if (strncasecmp(p,"iso-8859-8",10) == 0) code=CHRS_ISO_8859_8;
	else if (strncasecmp(p,"iso-8859-9",10) == 0) code=CHRS_ISO_8859_9;
	else if (strncasecmp(p,"koi8-r",6) == 0) code=CHRS_KOI8_R;
	else if (strncasecmp(p,"koi8-u",6) == 0) code=CHRS_KOI8_U;
	else if (strncasecmp(p,"macintosh",9) == 0) code=CHRS_MACINTOSH;
	else if (strncasecmp(p,"Shift_JIS",9) == 0) code=CHRS_SJIS;
	else if (strncasecmp(p,"utf-7",5) == 0) code=CHRS_UTF_7;
	else if (strncasecmp(p,"utf-8",5) == 0) code=CHRS_UTF_8;
	else if (strncasecmp(p,"viscii",6) == 0) code=CHRS_VISCII_11;
	else if (strncasecmp(p,"x-cp424",7) == 0) code=CHRS_CP424;
	else if (strncasecmp(p,"x-cp437",7) == 0) code=CHRS_CP437;
	else if (strncasecmp(p,"x-cp850",7) == 0) code=CHRS_CP850;
	else if (strncasecmp(p,"x-cp852",7) == 0) code=CHRS_CP852;
	else if (strncasecmp(p,"x-cp862",7) == 0) code=CHRS_CP862;
	else if (strncasecmp(p,"x-cp866",7) == 0) code=CHRS_CP866;
	else if (strncasecmp(p,"x-cp895",7) == 0) code=CHRS_CP895;
	else if (strncasecmp(p,"x-CN-GB",7) == 0) code=CHRS_GB;
	else if (strncasecmp(p,"x-CN-Big5",9) == 0) code=CHRS_BIG5;
	else if (strncasecmp(p,"x-EUC-jp",8) == 0) code=CHRS_EUC_JP;
	else if (strncasecmp(p,"x-FIDOMAZ",9) == 0) code=CHRS_FIDOMAZOVIA;
	else if (strncasecmp(p,"x-gb2312",8) == 0) code=CHRS_GB;
	else if (strncasecmp(p,"x-HZ",4) == 0) code=CHRS_HZ;
	else if (strncasecmp(p,"x-mac-roman",11) == 0) code=CHRS_MACINTOSH;
	else if (strncasecmp(p,"x-MAZOVIA",9) == 0) code=CHRS_FIDOMAZOVIA;
	else if (strncasecmp(p,"x-mik",5) == 0) code=CHRS_MIK_CYR;
	else if (strncasecmp(p,"x-NEC-JIS",9) == 0) code=CHRS_NEC;
	else if (strncasecmp(p,"x-Shift-JIS",11) == 0) code=CHRS_SJIS;
	else if (strncasecmp(p,"x-sjis",6) == 0) code=CHRS_SJIS;
	else if (strncasecmp(p,"x-tis620",8) == 0) code=CHRS_ISO_8859_11;
	else if (strncasecmp(p,"x-x-big5",8) == 0) code=CHRS_BIG5;
	else if (strncasecmp(p,"x-zW",4) == 0) code=CHRS_ZW;
	/* only intended to be in Areas file. So we can try to found the 
	 * from the values of chars in message itself */
	else if (strncasecmp(p,"AUTODETECT",10) == 0) code=CHRS_AUTODETECT;
	else if (strncasecmp(p,"default",7) == 0) code=CHRS_NOTSET;
	else { code=CHRS_NOTSET; Syslog('+', "Unknown charset: %s",p); }
	return code;
}



int readcharset(char *p)
{
	int code;

	if (!strchr(p, ';')) /* foolproof MSC96 */
		return CHRS_NOTSET;
	else if ((strcasestr(p,(char *)"text/plain")) && (strcasestr(p,(char *)"charset=")))
		code=getcode(strcasestr(strchr(p,';'),(char *)"charset=")+8);
	else if ((strcasestr(p,(char *)"text/html")) && (strcasestr(p,(char *)"charset=")))
		code=getcode(strcasestr(strchr(p,';'),(char *)"charset=")+8);
	else code=CHRS_NOTSET;
	return code;
}



/* readchrs() is also used to read outcode in Areas file (if JE defined) */

int readchrs(char *p)
{
	int code;

	while (*p && isspace(*p)) p++;
	if (strncasecmp(p,"8859",4) == 0) code=CHRS_ISO_8859_1;
	/* for X-FTN-CODEPAGE: */
	else if (strncasecmp(p,"437",3) == 0) code=CHRS_CP437;
	else if (strncasecmp(p,"850",3) == 0) code=CHRS_CP850;
	else if (strncasecmp(p,"Arabic",6) == 0) code=CHRS_ISO_8859_6;
	else if (strncasecmp(p,"ASCII",5) == 0) code=CHRS_ASCII;
	else if (strncasecmp(p,"BIG",3) == 0) code=CHRS_BIG5;
	else if (strncasecmp(p,"CP 852",6) == 0) code=CHRS_CP852;
	else if (strncasecmp(p,"CP424",5) == 0) code=CHRS_CP424;
	else if (strncasecmp(p,"CP437",5) == 0) code=CHRS_CP437;
	else if (strncasecmp(p,"CP850",5) == 0) code=CHRS_CP850;
	else if (strncasecmp(p,"CP852",5) == 0) code=CHRS_CP852;
	else if (strncasecmp(p,"CP862",5) == 0) code=CHRS_CP862;
	else if (strncasecmp(p,"CP866",5) == 0) code=CHRS_CP866; /* ??? */
	else if (strncasecmp(p,"CP895",5) == 0) code=CHRS_CP895;
	else if (strncasecmp(p,"CP932",5) == 0) code=CHRS_SJIS;
	else if (strncasecmp(p,"CP942",5) == 0) code=CHRS_SJIS;
	else if (strncasecmp(p,"Cyrillic",8) == 0) code=CHRS_ISO_8859_5;
	else if (strncasecmp(p,"EUC-JP",6) == 0) code=CHRS_EUC_JP;
	else if (strncasecmp(p,"EUC-KR",6) == 0) code=CHRS_EUC_KR;
	else if (strncasecmp(p,"EUC",3) == 0) code=CHRS_EUC_JP;
	else if (strncasecmp(p,"FIDOMAZ",7) == 0) code=CHRS_FIDOMAZOVIA;
	else if (strncasecmp(p,"GB",2) == 0) code=CHRS_GB;
	else if (strncasecmp(p,"Greek",5) == 0) code=CHRS_ISO_8859_7;
	else if (strncasecmp(p,"Hebrew",6) == 0) code=CHRS_ISO_8859_8;
	else if (strncasecmp(p,"HZ",2) == 0) code=CHRS_HZ;
	/* Some FTN programs are misconfigured and use "IBMPC 2" kludge
	 * for the local DOS charset, even if it is DOS cyrillic or other
	 * so we will assume defaultftnchar here
	 */
	else if (strncasecmp(p,"IBMPC",5) == 0) code=defaultftnchar;
	else if (strncasecmp(p,"IBM",3) == 0) code=CHRS_CP437; /* "IBMPC 1" "IBMPC 2" "IBM CMP" */
	else if (strncasecmp(p,"ISO-11",6) == 0) code=CHRS_ISO_11;
	else if (strncasecmp(p,"ISO-2022-CN",11) == 0) code=CHRS_ISO_2022_CN;
	else if (strncasecmp(p,"ISO-2022-KR",11) == 0) code=CHRS_ISO_2022_KR;
	else if (strncasecmp(p,"ISO-2022-TW",11) == 0) code=CHRS_ISO_2022_TW;
	else if (strncasecmp(p,"ISO-4",5) == 0) code=CHRS_ISO_4;
	else if (strncasecmp(p,"ISO-60",6) == 0) code=CHRS_ISO_60;
	else if (strncasecmp(p,"ISO-8859",8) == 0) code=CHRS_ISO_8859_1;
	else if (strncasecmp(p,"JIS",3) == 0) code=CHRS_ISO_2022_JP; /* ??? - JE */
	else if (strncasecmp(p,"Kanji",5) == 0) code=CHRS_ISO_2022_JP;
	else if (strncasecmp(p,"KOI8-R",6) == 0) code=CHRS_KOI8_R;
	else if (strncasecmp(p,"KOI8-U",6) == 0) code=CHRS_KOI8_U;
	else if (strncasecmp(p,"KOI8",4) == 0) code=CHRS_KOI8_R;
	else if (strncasecmp(p,"LATIN-0",7) == 0) code=CHRS_ISO_8859_15;
	else if (strncasecmp(p,"LATIN1QP",8) == 0) code=CHRS_ISO_8859_1_QP;
	else if (strncasecmp(p,"LATIN-1",7) == 0) code=CHRS_ISO_8859_1;
	else if (strncasecmp(p,"Latin-2",7) == 0) code=CHRS_ISO_8859_2;
	else if (strncasecmp(p,"Latin-3",7) == 0) code=CHRS_ISO_8859_3;
	else if (strncasecmp(p,"Latin-4",7) == 0) code=CHRS_ISO_8859_4;
	else if (strncasecmp(p,"Latin-5",7) == 0) code=CHRS_ISO_8859_9;
	else if (strncasecmp(p,"Latin-6",7) == 0) code=CHRS_ISO_8859_10;
	else if (strncasecmp(p,"MAC",3) == 0) code=CHRS_MACINTOSH;
	else if (strncasecmp(p,"MIK",3) == 0) code=CHRS_MIK_CYR;
	else if (strncasecmp(p,"MAZOVIA",7) == 0) code=CHRS_FIDOMAZOVIA;
	else if (strncasecmp(p,"NEC",3) == 0) code=CHRS_NEC; /* ??? - JE */
	else if (strncasecmp(p,"PC-8",4) == 0) code=CHRS_CP437;
	else if (strncasecmp(p,"SJIS",4) == 0) code=CHRS_SJIS; /* ??? - JE */
	else if (strncasecmp(p,"Thai",4) == 0) code=CHRS_ISO_8859_11;
	else if (strncasecmp(p,"UJIS",4) == 0) code=CHRS_EUC_JP;
	else if (strncasecmp(p,"UTF-7",5) == 0) code=CHRS_UTF_7;
	else if (strncasecmp(p,"UTF-8",5) == 0) code=CHRS_UTF_8;
	else if (strncasecmp(p,"VISCII",6) == 0) code=CHRS_VISCII_11;
	else if (strncasecmp(p,"ZW",2) == 0) code=CHRS_ZW;
	/* only intended to be in Areas file. So we can try to found the
	 * from the values of chars in message itself */
	else if (strncasecmp(p,"AUTODETECT",10) == 0) code=CHRS_AUTODETECT;
	else if (strncasecmp(p,"default",7) == 0) code=CHRS_NOTSET;
	else { code=CHRS_NOTSET; Syslog('+', "Unknown CHRS: %s",p); }
	return code;
/* if you know of other CHRS: values which are in use, let me know so I can
   include them. Mail me to srtxg@chanae.alphanet.ch or srtxg (2:293/2219) */
}




char *getchrs(int code)
{
	char *chrs=NULL;

	if (code == CHRS_ASCII) chrs=(char *)"ASCII 2";
	else if (code == CHRS_BIG5) chrs=(char *)"BIG5"; /* ??? */
	else if (code == CHRS_CP424) chrs=(char *)"CP424"; /* ??? */
	else if (code == CHRS_CP437) chrs=(char *)"IBMPC 2";
	else if (code == CHRS_CP850) chrs=(char *)"CP850 2";
	else if (code == CHRS_CP852) chrs=(char *)"CP852"; /* ??? */
	else if (code == CHRS_CP862) chrs=(char *)"CP862"; /* ??? */
	else if (code == CHRS_CP866) chrs=(char *)"CP866";
	else if (code == CHRS_CP895) chrs=(char *)"CP895 2";
	else if (code == CHRS_EUC_JP) chrs=(char *)"UJIS"; /* ??? */
	else if (code == CHRS_EUC_KR) chrs=(char *)"EUC-KR"; /* ??? */
	else if (code == CHRS_FIDOMAZOVIA) chrs=(char *)"FIDOMAZ 2";
	else if (code == CHRS_GB) chrs=(char *)"GB"; /* ??? */
	else if (code == CHRS_HZ) chrs=(char *)"HZ 2"; /* ??? */
	else if (code == CHRS_ISO_2022_CN) chrs=(char *)"ISO-2022-CN"; /* ??? */
	else if (code == CHRS_ISO_2022_JP) chrs=(char *)"JIS";
	else if (code == CHRS_ISO_2022_KR) chrs=(char *)"ISO-2022-KR"; /* ??? */
	else if (code == CHRS_ISO_2022_TW) chrs=(char *)"ISO-2022-TW"; /* ??? */
	else if (code == CHRS_ISO_8859_1) chrs=(char *)"LATIN-1 2";
	else if (code == CHRS_ISO_8859_1_QP) chrs=(char *)"LATIN-1 2";
	else if (code == CHRS_ISO_8859_2) chrs=(char *)"Latin-2 3";
	else if (code == CHRS_ISO_8859_3) chrs=(char *)"Latin-3 3";
	else if (code == CHRS_ISO_8859_4) chrs=(char *)"Latin-4 3";
	else if (code == CHRS_ISO_8859_5) chrs=(char *)"Cyrillic 3"; /* ??? */
	else if (code == CHRS_ISO_8859_6) chrs=(char *)"Arabic 3"; /* ??? */
	else if (code == CHRS_ISO_8859_7) chrs=(char *)"Greek 3"; /* ??? */
	else if (code == CHRS_ISO_8859_8) chrs=(char *)"Hebrew 3"; /* ??? */
	else if (code == CHRS_ISO_8859_9) chrs=(char *)"Latin-5 3";
	else if (code == CHRS_ISO_8859_10) chrs=(char *)"Latin-6 3";
	else if (code == CHRS_ISO_8859_11) chrs=(char *)"Thai 3";
	else if (code == CHRS_ISO_8859_15) chrs=(char *)"LATIN-0 2";
	else if (code == CHRS_KOI8_R) chrs=(char *)"KOI8-R"; /* ??? */
	else if (code == CHRS_KOI8_U) chrs=(char *)"KOI8-U"; /* ??? */
	else if (code == CHRS_MACINTOSH) chrs=(char *)"MAC 2";
	else if (code == CHRS_MIK_CYR) chrs=(char *)"MIK-CYR";
	else if (code == CHRS_NEC) chrs=(char *)"NEC-JIS"; /* ??? */
	else if (code == CHRS_SJIS) chrs=(char *)"SJIS"; /* ??? */
	else if (code == CHRS_UTF_7) chrs=(char *)"UTF-7";
	else if (code == CHRS_UTF_8) chrs=(char *)"UTF-8";
	else if (code == CHRS_VISCII_11) chrs=(char *)"VISCII 3";
	else if (code == CHRS_ZW) chrs=(char *)"ZW"; /* ??? */
	else chrs=NULL;

	return chrs;
}



void writechrs(int code, FILE *pkt, int ispkt)
{
	char *akludge,*endline,*chrs=NULL;

	akludge = endline = NULL;

	if (ispkt==0) { 
		akludge=(char *)"X-FTN-"; endline=(char *)"\n"; 
	} else if (ispkt==1) { 
		akludge=(char *)"\1"; endline=(char *)"\r"; 
	} else if (ispkt==2) { 
		akludge=(char *)"X-FTN-ORIG"; endline=(char *)"\n"; 
	} else if (ispkt==3) {
		akludge=(char *)"\1"; endline=(char *)"\n";
	}
	chrs=getchrs(code);
	if (chrs) fprintf(pkt,"%sCHRS: %s%s",akludge,chrs,endline);
}


// WORDT NIET GEBRUIKT ??
void writecharset(int code, FILE *pip, rfcmsg *msg, rfcmsg *kmsg)
{
	char *p, *charset=NULL;

	charset=getcharset(code);

	if ((p=hdr((char *)"Mime-Version",msg))) fprintf(pip,(char *)"Mime-Version:%s",p);
	else if ((p=hdr((char *)"RFC-Mime-Version",kmsg))) fprintf(pip,(char *)"Mime-Version: %s",p);
	else if ((p=hdr((char *)"Mime-Version",kmsg))) fprintf(pip,(char *)"Mime-Version: %s",p);
	else if ((charset) && (code != CHRS_NOTSET)) fprintf(pip,"Mime-Version: 1.0\n");

	if ((p=hdr((char *)"Content-Type",msg))) fprintf(pip,"Content-Type:%s",p);
	else if ((p=hdr((char *)"RFC-Content-Type",kmsg))) fprintf(pip,"Content-Type: %s",p);
	else if ((p=hdr((char *)"Content-Type",kmsg))) fprintf(pip,"Content-Type: %s",p);
	else if ((charset) && (code != CHRS_NOTSET))
	{
		if ((p=hdr((char *)"FSCHTML",kmsg)) || (p=hdr((char *)"HTML",kmsg)))
			fprintf(pip,"Content-Type: text/html; charset=%s\n",charset);
		else
 			fprintf(pip,"Content-Type: text/plain; charset=%s\n",charset);
	}

	if ((p=hdr((char *)"Content-Length",msg))) fprintf(pip,"Content-Length%s",p);
	else if ((p=hdr((char *)"RFC-Content-Length",kmsg))) fprintf(pip,"Content-Length: %s",p);
	else if ((p=hdr((char *)"Content-Length",kmsg))) fprintf(pip,"Content-Length: %s",p);

	if ((p=hdr((char *)"Content-Transfer-Encoding",msg))) fprintf(pip,"Content-Transfer-Encoding:%s",p);
	else if ((p=hdr((char *)"RFC-Content-Transfer-Encoding",kmsg))) fprintf(pip,"Content-Transfer-Encoding: %s",p);
	else if ((p=hdr((char *)"Content-Transfer-Encoding",kmsg))) fprintf(pip,"Content-Transfer-Encoding: %s",p);
	else if ((charset) && (code == CHRS_ISO_8859_1_QP)) fprintf(pip,"Content-Transfer-Encoding: quoted-printable\n");
	else if ((charset) && (code != CHRS_NOTSET)) { fprintf(pip,"Content-Transfer-Encoding: ");
		if ((code == CHRS_ASCII || code == CHRS_UTF_7)) fprintf(pip,"7bit\n");
		else if (strncasecmp(charset,"iso-2022-",9) == 0) fprintf(pip,"7bit\n");
		else fprintf(pip,"8bit\n"); /* all others are 8 bit */
	}
}

