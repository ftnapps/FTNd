/*****************************************************************************
 *
 * $Id: msgutil.c,v 1.31 2007/03/05 12:25:20 mbse Exp $
 * Purpose ...............: Announce new files and FileFind
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/diesel.h"
#include "msgutil.h"


extern int		do_quiet;		/* Suppress screen output    */


/*
 * Translation table from Hi-USA-ANSI to Lo-ASCII,
 * currently only ANSI graphics are translated.
 */
char lotab[] = {
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217"
"\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237"
"\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257"
"\260\261\262\174\053\053\053\053\053\043\174\043\043\053\053\053"
"\053\053\053\053\053\053\053\053\043\043\043\043\043\075\043\053"
"\053\053\053\053\053\053\053\053\053\053\053\333\334\335\336\337"
"\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357"
"\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"
};



void Msg_Id(fidoaddr aka)
{
	char		*temp;
	unsigned int	crc = -1;

	temp = calloc(81, sizeof(char));
	snprintf(temp, 81, "\001MSGID: %s %08x", aka2str(aka), sequencer());
	MsgText_Add2(temp);
	Msg.MsgIdCRC = upd_crc32(temp, crc, strlen(temp));
	Msg.ReplyCRC = 0xffffffff;
	free(temp);
}



/*
 * Passed charset overrides defaults
 */
void Msg_Pid(int charset)
{
	char	*temp;
	time_t	tt;

	temp = calloc(81, sizeof(char));
	snprintf(temp, 81, "\001PID: MBSE-FIDO %s (%s-%s)", VERSION, OsName(), OsCPU());
	MsgText_Add2(temp);
	if (charset != FTNC_NONE) {
	    snprintf(temp, 81, "\001CHRS: %s", getftnchrs(charset));
	} else if (msgs.Charset != FTNC_NONE) {
	    snprintf(temp, 81, "\001CHRS: %s", getftnchrs(msgs.Charset));
	} else {
	    snprintf(temp, 81, "\001CHRS: %s", getftnchrs(FTNC_LATIN_1));
	}
	MsgText_Add2(temp);
	tt = time(NULL);
	snprintf(temp, 81, "\001TZUTC: %s", gmtoffset(tt));
	MsgText_Add2(temp);
	free(temp);
}



void Msg_Macro(FILE *fi)
{
    char    *temp, *line;
    int	    res;

    temp = calloc(MAXSTR, sizeof(char));
    line = calloc(MAXSTR, sizeof(char));

    while ((fgets(line, MAXSTR-2, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {

	/*
	 * Skip comment lines
	 */
	if (line[0] != '#') {
	    Striplf(line);
	    if (strlen(line) == 0) {
		/*
		 * Empty lines are just written
		 */
		MsgText_Add2((char *)"");
	    } else {
		strncpy(temp, ParseMacro(line,&res), MAXSTR-2);
		if (res)
		    Syslog('!', "Macro error line: \"%s\"", line);
		/*
		 * Only output if something was evaluated
		 */
		if (strlen(temp)) {
		    MsgText_Add2(temp);
		}
	    }
	}
    }

    free(line);
    free(temp);
}



int Msg_Top(char *template, int language, fidoaddr aka)
{
    char    *temp, *line;
    FILE    *fp, *fi;
    int	    fileptr, fileptr1 = -1L;
    int	    hasmodems = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));

    if ((fi = OpenMacro(template, language, FALSE))) {
	/*
	 * First override default aka with current aka, then display header.
	 */
	MacroVars("Y", "s", aka2str(aka));
	Msg_Macro(fi);
	fileptr = ftell(fi);

	MacroVars("pqrf", "dsss", 0, "", "", "");
	if (strlen(CFG.IP_Flags) && strlen(CFG.IP_Phone)) {
	    MacroVars("pqrf", "dsds", 2, CFG.IP_Phone, CFG.IP_Speed, CFG.IP_Flags);
	    fseek(fi, fileptr, SEEK_SET);
	    Msg_Macro(fi);
	    hasmodems = TRUE;
	}

	snprintf(temp, PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) != NULL) {
	    fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fp);
	    while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fp) == 1) {
		if (((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) && ttyinfo.available && strlen(ttyinfo.phone)) {
		    MacroVars("pqrf", "dsss", ttyinfo.type, ttyinfo.phone, ttyinfo.speed, ttyinfo.flags);
		    fseek(fi, fileptr, SEEK_SET);
		    Msg_Macro(fi);
		    hasmodems = TRUE;
		}
	    }
	    fclose(fp);
	}

	/*
	 * If no modems were defined, the fileptr is at a wrong place and
	 * must be moved.
	 */
	if (!hasmodems) {
	    line = calloc(MAXSTR, sizeof(char));
	    while ((fgets(line, MAXSTR-2, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {}
	    free(line);
	}

	/*
	 * TTY info footer
	 */
	Msg_Macro(fi);
	fileptr1 = ftell(fi);
	fclose(fi);
    }
    
    free(temp);
    return fileptr1;
}



void Msg_Bot(fidoaddr UseAka, char *Org, char *template)
{
	char	*temp, *aka;

	temp = calloc(81, sizeof(char));
	aka  = calloc(40, sizeof(char));

	/*
	 * Add tearline, this is hardcoded.
	 */
	MsgText_Add2((char *)"");
	MsgText_Add2(TearLine());

	if (UseAka.point)
		snprintf(aka, 40, "(%d:%d/%d.%d)", UseAka.zone, UseAka.net, UseAka.node, UseAka.point);
	else
		snprintf(aka, 40, "(%d:%d/%d)", UseAka.zone, UseAka.net, UseAka.node);

	snprintf(temp, 81, " * Origin: %s %s", Org, aka);
	MsgText_Add2(temp);
	free(aka);
	free(temp);
}



void CountPosted(char *Base)
{
	char	*temp;
	FILE	*fp;

	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r+")) != NULL) {
		fread(&msgshdr, sizeof(msgshdr), 1, fp);

		while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
			if (msgs.Active && (strlen(Base) == strlen(msgs.Base)) &&
			    (!strcmp(Base, msgs.Base))) {
				msgs.Posted.total++;
				msgs.Posted.tweek++;
				msgs.Posted.tdow[Diw]++;
				msgs.Posted.month[Miy]++;
				fseek(fp, - msgshdr.recsize, SEEK_CUR);
				fwrite(&msgs, msgshdr.recsize, 1, fp);
				break;
			}
			fseek(fp, msgshdr.syssize, SEEK_CUR);
		}

		fclose(fp);
	} else {
		WriteError("$Can't open %s", temp);
	}

	free(temp);
}


