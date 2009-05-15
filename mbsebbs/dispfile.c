/*****************************************************************************
 *
 * $Id: dispfile.c,v 1.26 2007/09/02 15:04:36 mbse Exp $
 * Purpose ...............: Display ANSI/ASCII textfiles
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
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "funcs.h"
#include "language.h"
#include "oneline.h"
#include "misc.h"
#include "timeout.h"
#include "timecheck.h"
#include "exitinfo.h"
#include "mail.h"
#include "email.h"
#include "input.h"
#include "dispfile.h"
#include "filesub.h"
#include "term.h"
#include "ttyio.h"


extern int  rows;



/*
 * Function returns total number of bbs users
 */
int TotalUsers(void);
int TotalUsers(void)
{
    FILE    *pUsrConfig;
    int     ch = 0;
    char    *temp;
    struct  userhdr uhdr;
    struct  userrec u;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if(( pUsrConfig = fopen(temp,"rb")) == NULL)
        WriteError("ControlCodeK: Can't open users file %s for reading", temp);
    else {
        fread(&uhdr, sizeof(uhdr), 1, pUsrConfig);

        while (fread(&u, uhdr.recsize, 1, pUsrConfig) == 1)
            if ((!u.Deleted) && (strlen(u.sUserName) > 0))
		ch++;

        fclose(pUsrConfig);
    }
    free(temp);

    return ch;
}



/*
 * Function will display a aras rulefile to the user.
 * Searches in rules directory first for a file with
 * the full name of the area, then the fule name of the
 * area with a .rul suffix, and finally for the first
 * 8 characters of the areaname with a .rul suffix.
 * The search is case insensitive.
 *
 * Menu 221.
 */
void DisplayRules(void)
{
    DIR		    *dp;
    struct dirent   *de;
    int		    Found = FALSE;
    char	    temp[PATH_MAX];

    if ((dp = opendir(CFG.rulesdir)) == NULL) {
	WriteError("$Can't open directory %s", CFG.rulesdir);
	Enter(1);
	/* Can't open directory for listing: */
	pout(LIGHTRED, BLACK, (char *) Language(290));
	Enter(2);
	Pause();
	return;
    }

    while ((de = readdir(dp))) {
	if (de->d_name[0] != '.') {
	    strcpy(temp, msgs.Tag);
	    if (strcasecmp(de->d_name, temp) == 0) {
		Found = TRUE;
		snprintf(temp, PATH_MAX, "%s/%s", CFG.rulesdir, de->d_name);
		break;
	    }
	    snprintf(temp, PATH_MAX, "%s.rul", temp);
	    if (strcasecmp(de->d_name, temp) == 0) {
		Found = TRUE;
		snprintf(temp, PATH_MAX, "%s/%s", CFG.rulesdir, de->d_name);
		break;
	    }
	    memset(&temp, 0, sizeof(temp));
	    strncpy(temp, msgs.Tag, 8);
	    snprintf(temp, PATH_MAX, "%s.rul", temp);
	    if (strcasecmp(de->d_name, temp) == 0) {
		Found = TRUE;
		snprintf(temp, PATH_MAX, "%s/%s", CFG.rulesdir, de->d_name);
		break;
	    }
	}
    }
    closedir(dp);

    if (Found) {
	Syslog('+', "Display rules: %s", temp);
	DisplayTextFile(temp);
    } else {
	Syslog('+', "Display rules for %s failed, not found", msgs.Tag);
	Enter(1);
	/* No rules found for this area */
	pout(LIGHTRED, BLACK, (char *) Language(13));
	Enter(2);
	Pause();
    }
}



/*
 * Function will display a flat ascii textfile to the
 * user without control codes. This is used to display
 * area rules, but is also called from the menu function
 * that will display a textfile or the contents of a archive.
 */
int DisplayTextFile(char *filename)
{
    FILE	    *fp;
    char	    *buf;
    int		    i, x, z, lc = 0;
    unsigned char   c;

    if ((fp = fopen(filename, "r")) == NULL) {
	WriteError("$DisplayTextFile(%s) failed");
	return FALSE;
    }

    buf = calloc(81, sizeof(char));
    clear();
    colour(CFG.TextColourF, CFG.TextColourB);

    while (fgets(buf, 79, fp)) {
	i = strlen(buf);

	for (x = 0; x < i; x++) {
	    c = (*(buf + x) & 0xff);
	    if (isprint(c))
		PUTCHAR(c);
	}

	Enter(1);
	lc++;

	if ((lc >= rows) && (lc < 1000)) {
	    lc = 0;
	    /* More (Y/n/=) */
	    pout(CFG.MoreF, CFG.MoreB, (char *) Language(61));
	    alarm_on();
	    z = toupper(Readkey());

	    if (z == Keystroke(61, 1)) {
		Enter(1);
		fclose(fp);
		free(buf);
		return TRUE;
	    }

	    if (z == Keystroke(61, 2))
		lc = 50000;

	    Blanker(strlen(Language(61)));
	    colour(CFG.TextColourF, CFG.TextColourB);
	}
    }

    fclose(fp);
    free(buf);

    Enter(1);
    /* Press ENTER to continue */
    language(CFG.MoreF, CFG.MoreB, 436);
    alarm_on();
    Readkey();

    return TRUE;
}



/*
 * Function will display textfile in either ansi or ascii and 
 * display control codes if they exist.
 * Returns Success if it can display the requested file
 */
int DisplayFile(char *filename)
{
    FILE	    *fp = NULL;
    int		    iSec = 0;
    char	    tmp[256], tmp1[256], buf[256], out[1024], newfile[PATH_MAX];
    int		    x;
    unsigned char   c;
    
    /*
     * Open the file in the following search order:
     *  1 - users language .ans
     *  2 - default language .ans
     *  3 - Abort, there is no file to show.
     */
    snprintf(newfile, PATH_MAX, "%s/share/int/txtfiles/%s/%s.ans", getenv("MBSE_ROOT"), lang.lc, filename);
    if ((fp = fopen(newfile, "rb")) == NULL) {
	snprintf(newfile, PATH_MAX, "%s/share/int/txtfiles/%s/%s.ans", getenv("MBSE_ROOT"), CFG.deflang, filename);
	if ((fp = fopen(newfile, "rb")) == NULL) {
	    return FALSE;
	}
    }

    if (utf8) {
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');
    }
    Syslog('b', "Displayfile %s %s mode", newfile, utf8? "UTF-8":"CP437");
    memset(&out, 0, sizeof(out));

    while (fgets(buf, sizeof(buf)-1, fp)) {

	for (x = 0; x < strlen(buf); x++) {
	    c = buf[x] & 0xff;
	    switch (c) {
		case '':  strncat(out, ControlCodeU(buf[++x]), sizeof(out)-1);
			    break;

		case '':  strncat(out, ControlCodeF(buf[++x]), sizeof(out)-1);
			    break;

		case '':  strncat(out, ControlCodeK(buf[++x]), sizeof(out)-1);
			    break;

		case '':  PUTSTR(chartran(out));
			    memset(&out, 0, sizeof(out));
			    alarm_on();
			    Readkey();
			    break;

		case '':  /*
			     * This code will allow you to specify a security level
			     * in front of the text, ie ^B32000^Bthis is a test^B
			     * will print this is a test only if you have security
			     * above 32000. Only one set of control chars per line.
			     * You cannot have multiple securitys etc
			     */
			    x++;
			    strcpy(tmp1, "");
			    while (buf[x] != '') {
				snprintf(tmp, sizeof(tmp)-1, "%c", buf[x]);
				strncat(tmp1, tmp, sizeof(tmp1)-1);
				x++;
			    }
			    x++;
			    iSec = atoi(tmp1);
			    while ((x <= strlen(buf)) && buf[x] != '') {
				if (exitinfo.Security.level >= iSec) {
				    snprintf(tmp1, sizeof(tmp1) -1, "%c", buf[x]);
				    strncat(out, tmp1, sizeof(out)-1);
				}
				x++;
			    } 
			    break;

		case '\r':  break;

		case '\n':  strncat(out, (char *)"\r\n", sizeof(out));
			    break;

		case '':  PUTSTR(chartran(out));
			    memset(&out, 0, sizeof(out));
			    FLUSHOUT();
			    sleep(1);
			    break;

		default:    snprintf(tmp1, sizeof(tmp1)-1, "%c", buf[x]);
			    strncat(out, tmp1, sizeof(out));
	    } /* switch */
	} /* for */

	PUTSTR(chartran(out));
	memset(&out, 0, sizeof(out));

    } /* while fgets */

    fclose(fp);
    chartran_close();
    return TRUE;
}



int DisplayFileEnter(char *File)
{
    int	    rc;

    rc = DisplayFile(File);
    Enter(1);
    /* Press ENTER to continue */
    language(LIGHTMAGENTA, BLACK, 436);
    alarm_on();
    Readkey();
    return rc;
}



char *ControlCodeF(int ch)
{
    static char    temp[81];
    
    /* Update user info */
    ReadExitinfo();

    switch (toupper(ch)) {
	case '!':
		snprintf(temp, 81, "%s", exitinfo.sProtocol);
		break;
	case 'A':
		snprintf(temp, 81, "%d", exitinfo.Uploads);
		break;

	case 'B':
		snprintf(temp, 81, "%d", exitinfo.Downloads);
		break;

	case 'C':
		snprintf(temp, 81, "%u", exitinfo.DownloadK);
		break;

	case 'D':
		snprintf(temp, 81, "%u", exitinfo.UploadK);
		break;

	case 'E':
		snprintf(temp, 81, "%u", exitinfo.DownloadK + exitinfo.UploadK);
		break;

	case 'F':
		snprintf(temp, 81, "%u", LIMIT.DownK); 
		break;

	case 'H':
		snprintf(temp, 81, "%d", iAreaNumber);
		break;

	case 'I':
		snprintf(temp, 81, "%s", sAreaDesc);
		break;

	case 'J':
		snprintf(temp, 81, "%u", LIMIT.DownF);
		break;

	case 'K':
		snprintf(temp, 81, "%s", LIMIT.Description);
		break;

	default:
		snprintf(temp, 81, " ");
    }

    return temp;
}



char *ControlCodeU(int ch)
{
    static char	temp[81];
    
    /*
     * Update user info
     */
    TimeCheck();
    ReadExitinfo();

    switch (toupper(ch)) {
	case 'A':
		snprintf(temp, 81, "%s", exitinfo.sUserName);
		break;

	case 'B':
		snprintf(temp, 81, "%s", exitinfo.sLocation);
		break;

	case 'C':
		snprintf(temp, 81, "%s", exitinfo.sVoicePhone);
		break;

	case 'D':
		snprintf(temp, 81, "%s", exitinfo.sDataPhone);
		break;

	case 'E':
		snprintf(temp, 81, "%s", LastLoginDate);
		break;

	case 'F':
		snprintf(temp, 81, "%s %s", StrDateDMY(exitinfo.tFirstLoginDate), StrTimeHMS(exitinfo.tFirstLoginDate));
		break;
		
	case 'G':
		snprintf(temp, 81, "%s", LastLoginTime);
		break;

	case 'H':
		snprintf(temp, 81, "%d", exitinfo.Security.level);
		break;
		
	case 'I':
		snprintf(temp, 81, "%d",  exitinfo.iTotalCalls);
		break;

	case 'J':
		snprintf(temp, 81, "%d", exitinfo.iTimeUsed);
		break;

	case 'K':
		snprintf(temp, 81, "%d", exitinfo.iConnectTime);
		break;

	case 'L':
		snprintf(temp, 81, "%d", exitinfo.iTimeLeft);
		break;

	case 'M':
		snprintf(temp, 81, "%d", rows);
		break;

	case 'N':
		snprintf(temp, 81, "%s", FirstName);
		break;

	case 'O':
		snprintf(temp, 81, "%s", LastName);
		break;

	case 'Q':
	 	snprintf(temp, 81, "%s", exitinfo.ieNEWS ? (char *) Language(147) : (char *) Language(148));
		break;

	case 'P':
		snprintf(temp, 81, "%s", (char *) Language(147));
		break;

	case 'R':
		snprintf(temp, 81, "%s", exitinfo.HotKeys ? (char *) Language(147) : (char *) Language(148));
		break;

	case 'S':
		snprintf(temp, 81, "%d", exitinfo.iTimeUsed + exitinfo.iTimeLeft);
		break;

	case 'T':
		snprintf(temp, 81, "%s", exitinfo.sDateOfBirth);
		break;

	case 'U':
		snprintf(temp, 81, "%d", exitinfo.iPosted);
		break;

	case 'X':
		snprintf(temp, 81, "%s", lang.Name);
		break;

	case 'Y':
		snprintf(temp, 81, "%s", exitinfo.sHandle);
		break;

	case 'Z':
	 	snprintf(temp, 81, "%s", exitinfo.DoNotDisturb ? (char *) Language(147) : (char *) Language(148));
		break;

	case '1':
		snprintf(temp, 81, "%s", exitinfo.MailScan ? (char *) Language(147) : (char *) Language(148));
		break;

	case '2':
		snprintf(temp, 81, "%s", exitinfo.ieFILE ? (char *) Language(147) : (char *) Language(148));
		break;

	case '3':
		switch(exitinfo.MsgEditor) {
		    case X_LINEEDIT:	snprintf(temp, 81, "%s", Language(388));
					break;
		    case FSEDIT:	snprintf(temp, 81, "%s", Language(388));
					break;
		    case EXTEDIT:	snprintf(temp, 81, "%s", Language(389));
					break;
		    default:		snprintf(temp, 81, "?");
		}	    
		break;

	case '4':
		snprintf(temp, 81, "%s", exitinfo.FSemacs ? (char *) Language(147) : (char *) Language(148));
		break;

	case '5':
		snprintf(temp, 81, "%s", exitinfo.address[0]);
		break;

	case '6':
		snprintf(temp, 81, "%s", exitinfo.address[1]);
		break;

	case '7':
		snprintf(temp, 81, "%s", exitinfo.address[2]);
		break;

	case '8':
		snprintf(temp, 81, "%s", exitinfo.OL_ExtInfo ? (char *) Language(147) : (char *) Language(148));
		break;

	case '9':
		snprintf(temp, 81, "%s", getftnchrs(exitinfo.Charset));
		break;

	case '0':
		snprintf(temp, 81, "%s", exitinfo.Archiver);
		break;

	default:
		snprintf(temp, 81, " ");
    }

    return temp;
}



char *ControlCodeK(int ch)
{
    FILE	*pCallerLog;
    char	sDataFile[PATH_MAX], *p;
    static char	temp[81];
    lastread	LR;
    unsigned int	mycrc;

    switch (toupper(ch)) {
	case 'A':
		snprintf(temp, 81, "%s", (char *) GetDateDMY());
		break;

	case 'B':
		snprintf(temp, 81, "%s", (char *) GetLocalHMS());
		break;

	case 'C':
		snprintf(temp, 81, "%s", (char *) GLCdate());
		break;

	case 'D':
		snprintf(temp, 81, "%s", (char *) GLCdateyy());
		break;

	case 'E':
		snprintf(temp, 81, "%d", Speed());
		break;

	case 'F':
	  	snprintf(temp, 81, "%s", LastCaller);
		break;

	case 'G':
		snprintf(temp, 81, "%d", TotalUsers());
		break;

	case 'H':
		snprintf(sDataFile, PATH_MAX, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
		if((pCallerLog = fopen(sDataFile, "rb")) != NULL) {
		    fread(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);
		    snprintf(temp, 81, "%d", SYSINFO.SystemCalls);
		    fclose(pCallerLog);
		}
		break;

	case 'I':
		snprintf(temp, 81, "%d", iMsgAreaNumber + 1);
		break;

	case 'J':
		snprintf(temp, 81, "%s", sMsgAreaDesc);
		break;

	case 'K':
		snprintf(temp, 81, "%s", Oneliner_Get());
		break;

	case 'L':
		SetMsgArea(iMsgAreaNumber);
		snprintf(temp, 81, "%d", MsgBase.Total);
		break;

	case 'M':
		p = xstrcpy(exitinfo.sUserName);
		mycrc = StringCRC32(tl(p));
		free(p);
		LR.UserID = grecno;
		LR.UserCRC = mycrc;
		if (Msg_Open(sMsgAreaBase)) {
		    if (Msg_GetLastRead(&LR) == TRUE) {
			if (LR.HighReadMsg > MsgBase.Highest)
			    LR.HighReadMsg = MsgBase.Highest;
			snprintf(temp, 81, "%d", LR.HighReadMsg);
		    } else
			snprintf(temp, 81, "?");
		    Msg_Close();
		}
		break;

	case 'N':
		snprintf(temp, 81, "%s", sMailbox);
		break;

	case 'O':
		SetEmailArea(sMailbox);
		snprintf(temp, 81, "%d", EmailBase.Total);
		break;

	case 'P':
		SetEmailArea(sMailbox);
		p = xstrcpy(exitinfo.sUserName);
		mycrc = StringCRC32(tl(p));
		free(p);
		LR.UserID = grecno;
		LR.UserCRC = mycrc;
		if (Msg_Open(sMailpath)) {
		    if (Msg_GetLastRead(&LR) == TRUE) {
			if (LR.HighReadMsg > EmailBase.Highest)
			    LR.HighReadMsg = EmailBase.Highest;
			snprintf(temp, 81, "%d", LR.HighReadMsg);
		    } else
			snprintf(temp, 81, "?");
		    Msg_Close();
		}
		break;

	case 'Q':
		snprintf(temp, 81, "%s %s", StrDateDMY(LastCallerTime), StrTimeHMS(LastCallerTime));
		break;

	default:
		snprintf(temp, 81, " ");

    }

    return temp;
}


