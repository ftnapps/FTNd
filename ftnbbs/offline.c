/*****************************************************************************
 *
 * Purpose ...............: Offline Reader
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "../lib/bluewave.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "mail.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "file.h"
#include "filesub.h"
#include "exitinfo.h"
#include "timeout.h"
#include "msgutil.h"
#include "pop3.h"
#include "offline.h"
#include "whoson.h"
#include "term.h"
#include "ttyio.h"
#include "openport.h"
#include "transfer.h"



int		Total, TotalPersonal, Current, Personal;
unsigned int	TotalPack;
unsigned short	BarWidth;
lastread	LR;
static char	TempStr[128];
extern int	do_mailout;
extern int	cols;
extern int	rows;
char		*newtear = NULL;
extern unsigned int	mib_posted;


typedef struct	_msg_high {
	struct	_msg_high	*next;
	unsigned int		Area;
	char			Tag[60];
	unsigned int		LastMsg;
	unsigned int		Personal;
} msg_high;



/*
 *  Internal prototypes.
 */
void		AddArc(char *, char *);
void		tidy_high(msg_high **);
void		fill_high(msg_high **, unsigned int, char *, unsigned int, unsigned int);
void		UpdateLR(msg_high *, FILE *);
void		Add_Kludges(fidoaddr, int, char *);
int		OLR_Prescan(void);
void		DrawBar(char *);
unsigned int	BlueWave_PackArea(unsigned int, int);
void		BlueWave_Fetch(void);
unsigned int	QWK_PackArea(unsigned int, int);
void		QWK_Fetch(void);
float		IEEToMSBIN(float);
float		MSBINToIEE(float);
char		*StripSpaces(char *, int);
unsigned int	ASCII_PackArea(unsigned int, int, char *);



/****************************************************************************
 *
 *                           Global Functions
 */


void AddArc(char *Temp, char *Pktname)
{
    execute_str((char *)archiver.marc, Pktname, Temp, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    unlink(Temp);
    printf(".");
}



void tidy_high(msg_high **hdp)
{
    msg_high *tmp, *old;

    for (tmp = *hdp; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *hdp = NULL;
}



/*
 *  Add an area to the array
 */
void fill_high(msg_high **hdp, unsigned int Area, char *tag, unsigned int Last, unsigned int Pers)
{
    msg_high *tmp;

    tmp = (msg_high *)malloc(sizeof(msg_high));
    tmp->next = *hdp;
    tmp->Area = Area;
    snprintf(tmp->Tag, 51, "%s", tag);
    tmp->LastMsg = Last;
    tmp->Personal = Pers;
    *hdp = tmp;
}



void UpdateLR(msg_high *mhl, FILE *mf)
{
    char	*p;
    msg_high	*tmp;
    unsigned int mycrc;

    /*      Updating lastread pointer */
    poutCR(YELLOW, BLACK, (char *)Language(449));
    colour(LIGHTMAGENTA, BLACK);
        
    for (tmp = mhl; tmp; tmp = tmp->next) {
	PUTCHAR('.');
	fseek(mf, ((tmp->Area -1) * (msgshdr.recsize + msgshdr.syssize)) + msgshdr.hdrsize, SEEK_SET);
	fread(&msgs, msgshdr.recsize, 1, mf);
	if (Msg_Open(msgs.Base)) {
	    if (Msg_Lock(30L)) {
		if (tmp->Personal) 
		    Syslog('?', "Personal messages to update");

		p = xstrcpy(exitinfo.sUserName);
		mycrc = StringCRC32(tl(p));
		free(p);
		LR.UserID = grecno;
		LR.UserCRC = mycrc;
		if (Msg_GetLastRead(&LR) == TRUE) {
		    LR.LastReadMsg = tmp->LastMsg;
		    if (tmp->LastMsg > LR.HighReadMsg)
			LR.HighReadMsg = tmp->LastMsg;
		    if (LR.HighReadMsg > Msg_Highest()) {
			Syslog('?', "Highread was too high");
			LR.HighReadMsg = Msg_Highest();
		    }
		    LR.UserCRC = mycrc;
		    if (!Msg_SetLastRead(LR))
			WriteError("Error update lastread");
		} else {
		    /*
		     * Append new lastread pointer
		     */
		    LR.UserCRC = mycrc;
		    LR.UserID = grecno;
		    LR.LastReadMsg = tmp->LastMsg;
		    LR.HighReadMsg = tmp->LastMsg;
		    if (!Msg_NewLastRead(LR))
			WriteError("Can't append new lastread");
		}
		Msg_UnLock();
	    }
	    Msg_Close();
	}
    }
}



/*
 *  Add message kludges at the start, append the message text and add 
 *  trailing kludges, tearline and originline.
 */
void Add_Kludges(fidoaddr dest, int IsReply, char *fn)
{
    FILE    *tp;

    Add_Headkludges(fido2faddr(dest), IsReply);

    if ((tp = fopen(fn, "r")) != NULL) {
	Msg_Write(tp);
	fclose(tp);
    }

    Add_Footkludges(FALSE, newtear, FALSE);
    Msg_AddMsg();
    Msg_UnLock();
}



/****************************************************************************
 *
 *                          Offline Configuration
 */


/*
 *  Tag areas, called from menu.
 */
void OLR_TagArea()
{
    char    *Msgname, *Tagname;
    FILE    *ma, *tf;
    char    *buf, msg[81];
    int	    total, Offset, Area;
    int	    lines, input, ignore = FALSE, maxlines;

    WhosDoingWhat(OLR, NULL);

    Msgname = calloc(PATH_MAX, sizeof(char));
    Tagname = calloc(PATH_MAX, sizeof(char));
    buf     = calloc(81,  sizeof(char));

    snprintf(Msgname, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    snprintf(Tagname, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);

    clear();
    /*      Tag Offline Reader message areas */
    poutCR(YELLOW, BLACK, (char *)Language(66));

    do {
	Enter(1);
	/*        Enter the name of the conference, or ? for a list: */
	pout(WHITE, BLACK, (char *)Language(228));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(buf, 20);

	if (buf[0] == '?') {
	    maxlines = lines = rows - 1;
	    /*      Conference           Area  Msgs   Description */
	    poutCR(LIGHTCYAN, BLACK, (char *)Language(229));
	    if ((ma = fopen(Msgname, "r")) != NULL) {
		fread(&msgshdr, sizeof(msgshdr), 1, ma);
		Area = 0;
		if ((tf = fopen(Tagname, "r")) != NULL) {
		    while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
			fseek(ma, msgshdr.syssize, SEEK_CUR);
			fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
			Area++;
			if (Msg_Open(msgs.Base)) {
			    total = Msg_Number();
			    Msg_Close();
			} else
			    total = 0;
			if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && (!olrtagrec.Tagged) && strlen(msgs.QWKname)) {
			    if ( (lines != 0) || (ignore) ) {
				lines--;
				snprintf(msg, 81, "%-20.20s %-5d %-5d  %s", msgs.QWKname, Area, total, msgs.Name);
				poutCR(CYAN, BLACK, msg);
			    }
			    if (lines == 0) {
				/* More (Y/n/=) */
				snprintf(msg, 81, "%s%c\x08", (char *) Language(61),Keystroke(61,0));
				pout(WHITE, BLACK, msg);
				alarm_on();
				input = toupper(Readkey());
				PUTCHAR(input);
				PUTCHAR('\r');
				if ((input == Keystroke(61, 0)) || (input == '\r'))
				    lines = maxlines;

				if (input == Keystroke(61, 1)) {
				    break;
				}

				if (input == Keystroke(61, 2))
				    ignore = TRUE;
				else
				    lines  = maxlines;
				colour(CYAN, BLACK);
			    }
			}
		    }
		    fclose(tf);
		} 
		fclose(ma);
	    }
	} else if (buf[0] != '\0') {
	    if (atoi(buf) != 0) {
		if ((ma = fopen(Msgname, "r")) != NULL) {
		    fread(&msgshdr, sizeof(msgshdr), 1, ma);
		    Offset = msgshdr.hdrsize + ((atoi(buf)-1) * (msgshdr.recsize + msgshdr.syssize));
		    fseek(ma, Offset, SEEK_SET);
		    if (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
			if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
			    if ((tf = fopen(Tagname, "r+")) != NULL) {
				fseek(tf, (atoi(buf)-1) * sizeof(olrtagrec), SEEK_SET);
				fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
				if (!olrtagrec.Tagged) {
				    olrtagrec.Tagged = TRUE;
				    fseek(tf, - sizeof(olrtagrec), SEEK_CUR);
				    fwrite(&olrtagrec, sizeof(olrtagrec), 1, tf);
				    Syslog('+', "OLR Tagged %d %s", atoi(buf), msgs.QWKname);
				}
				fclose(tf);
			    }
			}
		    }
		    fclose(ma);
		}
	    } else {
		if ((ma = fopen(Msgname, "r")) != NULL) {
		    fread(&msgshdr, sizeof(msgshdr), 1, ma);
		    Area = 0;
		    while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
			fseek(ma, msgshdr.syssize, SEEK_CUR);
			Area++;
			if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
			    if (strcasecmp(msgs.QWKname, buf) == 0) {
				if ((tf = fopen(Tagname, "r+")) != NULL) {
				    fseek(tf, (Area-1) * sizeof(olrtagrec), SEEK_SET);
				    fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
				    if (!olrtagrec.Tagged) {
					olrtagrec.Tagged = TRUE;
					fseek(tf, - sizeof(olrtagrec), SEEK_CUR);
					fwrite(&olrtagrec, sizeof(olrtagrec), 1, tf);
					Syslog('+', "OLR Tagged %d %s", Area, msgs.QWKname);
				    }
				    fclose(tf);
				}
			    }
			}
		    }
		    fclose(ma);
		}
	    }
	}

    } while (buf[0] != '\0');

    free(buf);
    free(Tagname);
    free(Msgname);
}



/*
 *  Untag areas, called from menu.
 */
void OLR_UntagArea()
{
    char    *Msgname, *Tagname, *buf, msg[81];
    FILE    *ma, *tf;
    int	    total, Offset, Area;
    int     lines, input, ignore = FALSE, maxlines;

    WhosDoingWhat(OLR, NULL);

    Msgname = calloc(PATH_MAX, sizeof(char));
    Tagname = calloc(PATH_MAX, sizeof(char));
    buf	= calloc(81,  sizeof(char));

    snprintf(Msgname, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    snprintf(Tagname, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);

    clear();
    /*      Untag Offline Reader message areas */
    poutCR(YELLOW, BLACK, (char *)Language(256));

    do {
	Enter(1);
	/*        Enter the name of the conference, or ? for a list:  */
	pout(WHITE, BLACK, (char *)Language(228));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(buf, 20);

	if (buf[0] == '?') {
	    maxlines = lines = rows - 1;
	    /*      Conference           Area  Msgs   Description */
	    poutCR(LIGHTCYAN, BLACK, (char *)Language(229));
	    if ((ma = fopen(Msgname, "r")) != NULL) {
		fread(&msgshdr, sizeof(msgshdr), 1, ma);
		Area = 0;
		if ((tf = fopen(Tagname, "r")) != NULL) {
		    while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
			fseek(ma, msgshdr.syssize, SEEK_CUR);
			fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
			Area++;
			if (Msg_Open(msgs.Base)) {
			    total = Msg_Number();
			    Msg_Close();
			} else
			    total = 0;
			if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && olrtagrec.Tagged && strlen(msgs.QWKname)) {
			    if ( (lines != 0) || (ignore) ) {
				lines--;
				snprintf(msg, 81, "%-20.20s %-5d %-5d  %s", msgs.QWKname, Area, total, msgs.Name);
				poutCR(CYAN, BLACK, msg);
			    }
			    if (lines == 0) {
				/* More (Y/n/=) */
				snprintf(msg, 81, "%s%c\x08", (char *) Language(61),Keystroke(61,0));
				pout(WHITE, BLACK, msg);
				alarm_on();
				input = toupper(Readkey());
				PUTCHAR(input);
				PUTCHAR('\r');
				if ((input == Keystroke(61, 0)) || (input == '\r'))
				    lines = maxlines;

				if (input == Keystroke(61, 1)) {
				    break;
				}

				if (input == Keystroke(61, 2))
				    ignore = TRUE;
				else
				    lines  = maxlines;
				colour(CYAN, BLACK);
			    }
			}
		    }
		    fclose(tf);
		} 
		fclose(ma);
	    }
	} else if (buf[0] != '\0') {
	    if (atoi(buf) != 0) {
		if ((ma = fopen(Msgname, "r")) != NULL) {
		    fread(&msgshdr, sizeof(msgshdr), 1, ma);
		    Offset = msgshdr.hdrsize + ((atoi(buf)-1) * (msgshdr.recsize + msgshdr.syssize));
		    fseek(ma, Offset, SEEK_SET);
		    if (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
			if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
			    if ((tf = fopen(Tagname, "r+")) != NULL) {
				fseek(tf, (atoi(buf)-1) * sizeof(olrtagrec), SEEK_SET);
				fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
				if (olrtagrec.Tagged) {
				    if (msgs.OLR_Forced) {
					printf("Area cannot be switched off\n");
				    } else {
					olrtagrec.Tagged = FALSE;
					fseek(tf, - sizeof(olrtagrec), SEEK_CUR);
					fwrite(&olrtagrec, sizeof(olrtagrec), 1, tf);
					Syslog('+', "OLR Untagged %d %s", atoi(buf), msgs.QWKname);
				    }
				}
				fclose(tf);
			    }
			}
		    }
		    fclose(ma);
		}
	    } else {
		if ((ma = fopen(Msgname, "r")) != NULL) {
		    fread(&msgshdr, sizeof(msgshdr), 1, ma);
		    Area = 0;
		    while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
			fseek(ma, msgshdr.syssize, SEEK_CUR);
			Area++;
			if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
			    if (strcasecmp(msgs.QWKname, buf) == 0) {
				if ((tf = fopen(Tagname, "r+")) != NULL) {
				    fseek(tf, (Area-1) * sizeof(olrtagrec), SEEK_SET);
				    fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
				    if (olrtagrec.Tagged) {
					if (msgs.OLR_Forced) {
					    poutCR(LIGHTRED, BLACK, (char *)"Area cannot be switched off");
					} else {
					    olrtagrec.Tagged = FALSE;
					    fseek(tf, - sizeof(olrtagrec), SEEK_CUR);
					    fwrite(&olrtagrec, sizeof(olrtagrec), 1, tf);
					    Syslog('+', "OLR Untagged %d %s", Area, msgs.QWKname);
					}
				    }
				    fclose(tf);
				}
			    }
			}
		    }
		    fclose(ma);
		}
	    }
	}

    } while (buf[0] != '\0');

    free(buf);
    free(Tagname);
    free(Msgname);
}



void New_Hdr(void);
void New_Hdr()
{
    char	*temp;

    temp = calloc(128, sizeof(char));
    clear();
    colour(YELLOW, BLACK);
    /* New or deleted mail areas at */
    snprintf(temp, 128, "%s%s", (char *) Language(364), CFG.bbs_name);
    Center(temp);
    Enter(1);
    colour(WHITE, BLUE);
    /* Area  State  Type     Description */
    snprintf(temp, 128, "%-79s", (char *) Language(365));
    poutCR(WHITE, BLUE, temp);
    free(temp);
}



void New_Area(int);
void New_Area(int Area)
{
    char    msg[81];

    	/*    New    */
    snprintf(msg, 81, "%4d  %s", Area, (char *)Language(391));
    pout(LIGHTCYAN, BLACK, msg);

    switch (msgs.Type) {
	case LOCALMAIL:	PUTSTR((char *)Language(392)); /* Local    */
			break;
	case NETMAIL:	PUTSTR((char *)Language(393)); /* Netmail  */
			break;
	case LIST:
	case ECHOMAIL:	PUTSTR((char *)Language(394)); /* Echomail */
			break;
	case NEWS:	PUTSTR((char *)Language(395)); /* News     */
			break;
    }
    PUTSTR(msgs.Name);
    Enter(1);
}



void Old_Area(int);
void Old_Area(int Area)
{
    char    msg[81];

    /*            Del */
    snprintf(msg, 81, "%4d  %s", Area, (char *)Language(397));
    poutCR(LIGHTRED, BLACK, msg);
}



/*
 *  Sync tagged areas file. If CFG.NewAreas is on then we show the
 *  changed areas to the user. This one is called by user.c during login.
 */
void OLR_SyncTags()
{
    char    *Tagname, *Msgname;
    FILE    *fp, *ma;
    int	    Area;
    int	    Changed = FALSE;

    Tagname = calloc(PATH_MAX, sizeof(char));
    Msgname = calloc(PATH_MAX, sizeof(char));
    snprintf(Tagname, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
    snprintf(Msgname, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));

    if ((fp = fopen(Tagname, "r+")) == NULL) {

	/*
	 *  If the user has no .olrtagsfile yet, we silently create
	 *  a new one. The user will not be notified of new areas
	 *  of coarse.
	 */
	Syslog('m', "Creating %s", Tagname);
	if ((fp = fopen(Tagname, "w")) != NULL) {

	    if ((ma = fopen(Msgname, "r")) != NULL) {
		fread(&msgshdr, sizeof(msgshdr), 1, ma);

		while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
		    memset(&olrtagrec, 0, sizeof(olrtagrec));
		    if ((msgs.Active) && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
			olrtagrec.Available = TRUE;
			olrtagrec.ScanNew = TRUE;
			if (msgs.OLR_Forced || msgs.OLR_Default)
			    olrtagrec.Tagged = TRUE;
		    }
		    fwrite(&olrtagrec, sizeof(olrtagrec), 1, fp);
		    fseek(ma, msgshdr.syssize, SEEK_CUR);
		}

		fclose(ma);
	    }
	}
    } else {
	/*
	 *  The user has been here before...
	 */
	if ((ma = fopen(Msgname, "r")) != NULL) {
	    fread(&msgshdr, sizeof(msgshdr), 1, ma);
	    Area = 0;

	    while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
		Area++;

		if (fread(&olrtagrec, sizeof(olrtagrec), 1, fp) == 1) {

		    /*
		     *  Check if this is a new area for the user.
		     */
		    if ((msgs.Active) && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname) && (!olrtagrec.Available)) {
			Syslog('m', "New msg area %ld %s", Area, msgs.Name);
			fseek(fp, - sizeof(olrtagrec), SEEK_CUR);
			olrtagrec.Available = TRUE;
			olrtagrec.ScanNew = TRUE;
			if (msgs.OLR_Forced || msgs.OLR_Default)
			    olrtagrec.Tagged = TRUE;
			fwrite(&olrtagrec, sizeof(olrtagrec), 1, fp);
			if (CFG.NewAreas) {
			    if (!Changed) {
				New_Hdr();
				Changed = TRUE;
			    }
			    New_Area(Area);
			}
		    } else {
			/*
			 *  Check if this area is no longer
			 *  available for the user.
			 */
			if (((!msgs.Active) || (!Access(exitinfo.Security, msgs.RDSec))) && olrtagrec.Available) {
			    Syslog('m', "Deleted msg area %ld", Area);
			    fseek(fp, - sizeof(olrtagrec), SEEK_CUR);
			    olrtagrec.Available = FALSE;
			    olrtagrec.ScanNew = FALSE;
			    olrtagrec.Tagged = FALSE;
			    fwrite(&olrtagrec, sizeof(olrtagrec), 1, fp);
			    if (CFG.NewAreas) {
				if (!Changed) {
				    New_Hdr();
				    Changed = TRUE;
				}
				Old_Area(Area);
			    }
			}
		    }

		} else {
		    /*
		     * If the number if msg areas was increased,
		     * append a new tagrecord.
		     */
		    memset(&olrtagrec, 0, sizeof(olrtagrec));
		    if ((msgs.Active) && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
			Syslog('m', "Append new area %ld %s", Area, msgs.Name);
			olrtagrec.Available = TRUE;
			olrtagrec.ScanNew = TRUE;
		        if (msgs.OLR_Forced || msgs.OLR_Default)
			    olrtagrec.Tagged = TRUE;
			if (CFG.NewAreas) {
			    if (!Changed) {
				New_Hdr();
			        Changed = TRUE;
			    }
			    New_Area(Area);
			}
		    }
		    fwrite(&olrtagrec, sizeof(olrtagrec), 1, fp);
		}
		fseek(ma, msgshdr.syssize, SEEK_CUR);
	    }

	    fclose(ma);
	}
    }

    fclose(fp);

    if (Changed) {
	colour(LIGHTGREEN, BLACK);
	if (utf8)
	    chartran_init((char *)"CP437", (char *)"UTF-8", 'B');
	PUTSTR(chartran(fLine_str(79)));
	chartran_close();
	Pause();
    }

    SetMsgArea(exitinfo.iLastMsgArea);
    free(Tagname);
    free(Msgname);
}



/*
 *  View tagged areas, called from menu.
 */
void OLR_ViewTags()
{
    char    *Tagname, *Msgname, msg[81];
    FILE    *tf, *ma;
    int	    total, Area = 0;
    int     lines, input, ignore = FALSE, maxlines;

    WhosDoingWhat(OLR, NULL);

    Tagname = calloc(PATH_MAX, sizeof(char));
    Msgname = calloc(PATH_MAX, sizeof(char));
    snprintf(Tagname, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
    snprintf(Msgname, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));

    if ((tf = fopen(Tagname, "r")) == NULL) {
	WriteError("$Can't open %s", Tagname);
	free(Tagname);
	free(Msgname);
	return;
    }

    if ((ma = fopen(Msgname, "r")) == NULL) {
	WriteError("$Can't open %s", Msgname);
	fclose(tf);
	free(Tagname);
	free(Msgname);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, ma);

    clear();
    /*       You have selected the following Conference(s): */
    poutCR(YELLOW, BLACK, (char *)Language(260));
    Enter(1);
    /*         Conference           Area  Msgs   Description */
    poutCR(LIGHTCYAN, BLACK, (char *)Language(229));
    colour(CYAN, BLACK);
    maxlines = lines = rows - 1;

    while (fread(&msgs, msgshdr.recsize, 1, ma) == 1) {
	fseek(ma, msgshdr.syssize, SEEK_CUR);
	fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
	Area++;

	if (olrtagrec.Tagged) {
	    if (Msg_Open(msgs.Base)) {
		total = Msg_Number();
		Msg_Close();
	    } else
		total = 0;
	    if ( (lines != 0) || (ignore) ) {
		lines--;
		snprintf(msg, 81, "%-20.20s %-5d %-5d  %s", msgs.QWKname, Area, total, msgs.Name);
		PUTSTR(msg);
		Enter(1);
	    }
	    if (lines == 0) {
		/* More (Y/n/=) */
		snprintf(msg, 81, "%s%c\x08", (char *) Language(61),Keystroke(61,0));
		pout(WHITE, BLACK, msg);
		alarm_on();
		input = toupper(Readkey());
		PUTCHAR(input);
		PUTCHAR('\r');
		if ((input == Keystroke(61, 0)) || (input == '\r'))
		    lines = maxlines;

		if (input == Keystroke(61, 1)) {
		    break;
		}
		if (input == Keystroke(61, 2))
		    ignore = TRUE;
		else
		    lines  = maxlines;
		colour(CYAN, BLACK);
	    }
	}
    }

    fclose(tf);
    fclose(ma);
    Pause();
    free(Tagname);
    free(Msgname);
}



/*
 *  Prescan all selected areas. Show the user the areas and messages in it.
 */
int OLR_Prescan()
{
    unsigned short  RetVal = FALSE, Areas;
    unsigned int    Number, mycrc;
    char	    *Temp, msg[81], *p;
    FILE	    *mf, *tf;
    int		    x;

    WhosDoingWhat(OLR, NULL);
    clear();
    /*      Offline Reader Download */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(277));
    Enter(2);

    if (exitinfo.Email)
	check_popmail(exitinfo.Name, exitinfo.Password);

    Temp = calloc(PATH_MAX, sizeof(char));
    snprintf(Temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    mf = fopen(Temp, "r");
    fread(&msgshdr, sizeof(msgshdr), 1, mf);

    snprintf(Temp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
    tf = fopen(Temp, "r");
    Total = TotalPersonal = Areas = 0;

    Enter(1);
    colour(WHITE, BLUE);
    /*        Forum                Description                               Msgs. Pers.   */
    poutCR(WHITE, BLUE, (char *)Language(297));

    while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	fseek(mf, msgshdr.syssize, SEEK_CUR);
	fread(&olrtagrec, sizeof(olrtagrec), 1, tf);

	if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname) && olrtagrec.Tagged) {
	    if (Msg_Open(msgs.Base)) {
		Areas++;
		Current = Personal = 0;
		snprintf(msg, 81, "%-20.20s %-41.41s ", msgs.QWKname, msgs.Name);
		pout(LIGHTCYAN, BLACK, msg);

		memset(&LR, 0, sizeof(LR));
		p = xstrcpy(exitinfo.sUserName);
		mycrc = StringCRC32(tl(p));
		free(p);
		LR.UserID = grecno;
		LR.UserCRC = mycrc;
		if (Msg_GetLastRead(&LR))
		    Number = LR.HighReadMsg;
		else
		    Number = Msg_Lowest() -1;
		if (Number > Msg_Highest())
		    Number = Msg_Highest();
		if (Msg_Next(&Number)) {
		    do {
			Msg_ReadHeader(Number);
			Current++;
			Total++;
			if ((strcasecmp(Msg.To, exitinfo.sUserName) == 0) || (strcasecmp(Msg.To, exitinfo.sHandle) == 0)) {
			    Personal++;
			    TotalPersonal++;
			} else if (msgs.Type == NETMAIL) {
			    Current--;
			    Total--;
			}
		    } while (Msg_Next(&Number));
		}

		snprintf(msg, 81, "%5u %5u", Current, Personal);
		poutCR(LIGHTGREEN, BLACK, msg);
		Msg_Close();
	    }
	}
    }

    Syslog('+', "OLR Prescan: %u Areas, %u Messages", Areas, Total);

    Enter(1);
    /*        Total messages found: */
    snprintf(msg, 81, "%s %u", (char *)Language(338), Total);
    pout(LIGHTBLUE, BLACK, msg);
    Enter(2);

    if (Total == 0L) {
	/*      No messages found to download! */
	poutCR(YELLOW, BLACK, (char *)Language(374));
	PUTCHAR('\007');
	Pause();
    } else {
	if (CFG.OLR_MaxMsgs != 0 && Total > CFG.OLR_MaxMsgs) {
	    /*      Too much messages. Only the first    will be packed! */
	    snprintf(msg, 81, "%s %d %s", (char *)Language(377), CFG.OLR_MaxMsgs, (char *)Language(411));
	    PUTCHAR('\007');
	    Enter(2);
	    Total = CFG.OLR_MaxMsgs;
	}

	/*      Do you want to download these messages [Y/n]? */
	pout(CFG.HiliteF, CFG.HiliteB, (char *)Language(425));
	alarm_on();
	x = toupper(Readkey());

	if (x != Keystroke(425, 1)) {
	    RetVal = TRUE;
	    TotalPack = Total;
	    BarWidth = 0;
	}
    }

    if (mf != NULL)	
	fclose(mf);
    if (tf != NULL)
	fclose(tf);

    free(Temp);
    return(RetVal);
}



/*
 * Draw progess bar
 */
void DrawBar(char *Pktname)
{
    char    msg[81];

    Enter(1);
    /*        Preparing packet */
    snprintf(msg, 81, "%s %s...", (char *)Language(445), Pktname);
    pout(YELLOW, BLACK, msg);
    Enter(2);
    poutCR(LIGHTGREEN, BLACK, (char *)"0%%    10%%  20%%   30%%   40%%   50%%   60%%   70%%   80%%   90%%   100%%");
    PUTSTR((char *)"|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|");
    PUTCHAR('\r');
}



void OLR_RestrictDate()
{
    WhosDoingWhat(OLR, NULL);

    PUTSTR((char *)"Not Yet Implemented");
    Enter(1);
    Pause();
}



/*
 *  Upload offline mail. Filenames: BBSID.NEW or BBSID.REP.
 *  Should also do hhhhhhhh.SU0 for point uploads.
 */
void OLR_Upload(void)
{
    char	*File, *temp, *Arc, Dirpath[PATH_MAX], Filename[81];
    int		rc = 0, RetVal = FALSE;
    FILE	*fp;
    up_list	*up = NULL, *tmpf;

    if (strlen(CFG.bbsid) == 0) {
	PUTSTR((char *)"System configuration error, inform sysop");
	Enter(1);
	WriteError("Config OLR bbsid not configured");
	Pause();
	return;
    }

    WhosDoingWhat(OLR, NULL);
    clear();
    /*      Offline Reader Upload */
    poutCR(LIGHTMAGENTA, BLACK, (char *)Language(439));

    File  = calloc(PATH_MAX, sizeof(char));
    temp  = calloc(PATH_MAX, sizeof(char));

    Enter(1);

    if ((rc = upload(&up))) {
	Syslog('+', "Upload failed, rc=%d", rc);
	return;
    }
    Home();
    Enter(1);

    snprintf(Dirpath, PATH_MAX, "%s/%s/upl", CFG.bbs_usersdir, exitinfo.Name);
    snprintf(Filename, 81, "%s.NEW", CFG.bbsid);

    if (!RetVal) {
	RetVal = getfilecase(Dirpath, Filename);
	Syslog('m', "%s RetVal=%s", Filename, RetVal?"True":"False");
    }

    if (!RetVal) {
	snprintf(Filename, 81, "%s.REP", CFG.bbsid);
	RetVal = getfilecase(Dirpath, Filename);
	Syslog('m', "%s RetVal=%s", Filename, RetVal?"True":"False");
    }

    if (RetVal == FALSE) {
	WriteError("Invalid OLR packed received");
	for (tmpf = up; tmpf; tmpf = tmpf->next) {
	    Syslog('+', "Delete %s", tmpf->filename);
	    unlink(tmpf->filename);
	}
	tidy_upload(&up);
	/*      Invalid packet received */
	pout(LIGHTRED, BLACK, (char *)Language(440));
	Enter(2);
	sleep(3);
	return;
    }
    tidy_upload(&up);

    snprintf(File, PATH_MAX, "%s/%s", Dirpath, Filename);
    Syslog('+', "Received OLR packet %s", File);

    if ((Arc = GetFileType(File)) == NULL) {
	/*      Unknown compression type */
	poutCR(LIGHTRED, BLACK, (char *)Language(441));
	Syslog('+', "Unknown compression type");
	Pause();
	unlink(File);
	return;
    }

    Syslog('m', "File type is %s", Arc);

    snprintf(temp, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL)
	return;

    fread(&archiverhdr, sizeof(archiverhdr), 1, fp);

    while (fread(&archiver, archiverhdr.recsize, 1, fp) == 1) {
	if ((strcmp(Arc, archiver.name) == 0) && archiver.available)
	    break;
    }
    fclose(fp);

    if (strcmp(Arc, archiver.name) || (!archiver.available)) {
	Syslog('+', "Archiver %s not available", Arc);
	/*      Archiver not available */
	poutCR(LIGHTRED, BLACK, (char *)Language(442));
	Pause();
	unlink(File);
	return;
    }

    Syslog('m', "Archiver %s", archiver.comment);

    colour(CFG.TextColourF, CFG.TextColourB);
    /* Unpacking archive */
    pout(CFG.TextColourF, CFG.TextColourB, (char *) Language(201));
    PUTCHAR(' ');
    snprintf(temp, PATH_MAX, "%s %s", archiver.funarc, File);
    Syslog('m', "Unarc %s", temp);
    colour(CFG.HiliteF, CFG.HiliteB);

    rc = execute_str(archiver.funarc, File, NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    if (rawport() != 0) {
	WriteError("Unable to set raw mode");
    }
    if (rc) {
	WriteError("$Failed %s", temp);
	/* ERROR */
	poutCR(LIGHTRED, BLACK, (char *) Language(217));
	Pause();
	return;
    }

    /* Ok */
    PUTSTR((char *) Language(200));
    Enter(1);
    unlink(File);

    /*
     * Check for BlueWave files, case insensitive.
     */
    RetVal = FALSE;
    snprintf(Dirpath, PATH_MAX, "%s/%s", CFG.bbs_usersdir, exitinfo.Name);
    snprintf(Filename, 81, "%s.UPL", CFG.bbsid);
    RetVal = getfilecase(Dirpath, Filename);
    Syslog('m', "%s RetVal=%s", Filename, RetVal?"True":"False");

    if (!RetVal) {
	snprintf(Filename, 81, "%s.REQ", CFG.bbsid);
	RetVal = getfilecase(Dirpath, Filename);
	Syslog('m', "%s RetVal=%s", Filename, RetVal?"True":"False");
    }

    if (!RetVal) {
	snprintf(Filename, 81, "%s.OLC", CFG.bbsid);
	RetVal = getfilecase(Dirpath, Filename);
	Syslog('m', "%s RetVal=%s", Filename, RetVal?"True":"False");
    }

    if (RetVal) {
	Syslog('+', "OLR packet is BlueWave v3");
	free(File);
	free(temp);
	BlueWave_Fetch();
	return;
    }

    /*
     * Check for QWK packet
     */
    snprintf(Filename, 81, "%s.MSG", CFG.bbsid);
    RetVal = getfilecase(Dirpath, Filename);
    Syslog('m', "%s RetVal=%s", Filename, RetVal?"True":"False");

    if (RetVal) {
	Syslog('+', "OLR packet is QWK");
	free(File);
	free(temp);
	QWK_Fetch();
	return;
    }

    WriteError("OLR_Upload: Garbage in mailpacket, clean directory!");
    /*      Unknown type mailpacket */
    poutCR(LIGHTRED, BLACK, (char *)Language(443));
    Pause();
    free(File);
    free(temp);
}



/***************************************************************************
 *
 *                    BlueWave specific functions.
 */


char *Extensions[] = {
   (char *)".SU0", (char *)".MO0", (char *)".TU0", (char *)".WE0", (char *)".TH0", (char *)".FR0", (char *)".SA0"
};



/*
 *  Download a BlueWave mailpacket, called from menu.
 */
void OLR_DownBW()
{
    struct tm	    *tp;
    time_t	    Now;
    char	    Pktname[32], *Work, *Temp, *cwd = NULL, *p;
    int		    Area = 0;
    int		    RetVal = FALSE, rc = 0;
    FILE	    *fp, *tf, *mf, *af;
    INF_HEADER	    Inf;
    INF_AREA_INFO   AreaInf;
    unsigned int    Start, High, mycrc;
    msg_high	    *mhl = NULL;

    if (strlen(CFG.bbsid) == 0) {
	PUTSTR((char *)"System configuration error, inform sysop");
	Enter(1);
	WriteError("Config OLR bbsid not configured");
	Pause();
	return;
    }

    if (!OLR_Prescan())
	return;

    Total = TotalPersonal = 0;
    clear();
    /*      BlueWave Offline download */
    poutCR(LIGHTBLUE, BLACK, (char *)Language(444));

    Work = calloc(PATH_MAX, sizeof(char));
    Temp = calloc(PATH_MAX, sizeof(char));

    Now = time(NULL);
    tp = localtime(&Now);
    Syslog('+', "Preparing BlueWave packet");

    snprintf(Pktname, 32, "%s%s", CFG.bbsid , Extensions[tp->tm_wday]);
    Syslog('m', "Packet name %s", Pktname);
    snprintf(Work, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);
    Syslog('m', "Work path %s", Work);

    snprintf(Temp, PATH_MAX, "%s/%s.INF", Work, CFG.bbsid);
    if ((fp = fopen(Temp, "w+")) == NULL) {
	WriteError("$Can't create %s", Temp);
	return;
    }

    /*
     *  Write the info header.
     */
    memset(&Inf, 0, sizeof(Inf));
    Inf.ver = PACKET_LEVEL;
    strncpy((char *)Inf.loginname, exitinfo.sUserName, 43);
    strncpy((char *)Inf.aliasname, exitinfo.sHandle, 43);
    Inf.zone  = le_us(CFG.aka[0].zone);
    Inf.net   = le_us(CFG.aka[0].net);
    Inf.node  = le_us(CFG.aka[0].node);
    Inf.point = le_us(CFG.aka[0].point);
    strncpy((char *)Inf.sysop, CFG.sysop_name, 41);
    strncpy((char *)Inf.systemname, CFG.bbs_name, 65);
    Inf.maxfreqs = CFG.OLR_MaxReq;
    if (exitinfo.HotKeys)
	Inf.uflags |= le_us(INF_HOTKEYS);
    Inf.uflags |= le_us(INF_GRAPHICS);
    if (exitinfo.OL_ExtInfo)
	Inf.uflags |= le_us(INF_EXT_INFO);
    Inf.credits = le_us(exitinfo.Credit);
    Inf.inf_header_len = le_us((unsigned short)sizeof(INF_HEADER));
    Inf.inf_areainfo_len = le_us((unsigned short)sizeof(INF_AREA_INFO));
    Inf.mix_structlen = le_us((unsigned short)sizeof(MIX_REC));
    Inf.fti_structlen = le_us((unsigned short)sizeof(FTI_REC));
    Inf.uses_upl_file = TRUE;
    Inf.can_forward = TRUE;
    strncpy((char *)Inf.packet_id, CFG.bbsid, 9);
    fwrite(&Inf, sizeof(INF_HEADER), 1, fp);

    /*
     *  Check to see if this stuff is compiled packed. If not the
     *  download packet is useless.
     */
    if ((sizeof(Inf) != ORIGINAL_INF_HEADER_LEN) || (sizeof(AreaInf) != ORIGINAL_INF_AREA_LEN)) {
	WriteError("PANIC: Probably not \"packed\" compiled!");
	fclose(fp);
	return;
    }

    snprintf(Temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((mf = fopen(Temp, "r")) == NULL) {
	WriteError("$Can't open %s", Temp);
	fclose(fp);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, mf);

    snprintf(Temp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
    if ((tf = fopen(Temp, "r")) == NULL) {
	WriteError("$Can't open %s", Temp);
	fclose(fp);
	fclose(mf);
	return;
    }

    /*
     *  Write the areas information
     */
    while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	fseek(mf, msgshdr.syssize, SEEK_CUR);
	fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
	Area++;

	if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
	    memset(&AreaInf, 0, sizeof(AreaInf));
	    snprintf((char *)AreaInf.areanum, 6, "%u", Area);
	    strncpy((char *)AreaInf.echotag, msgs.QWKname, 21);
	    strncpy((char *)AreaInf.title, msgs.Name, 50);
	    if (olrtagrec.Tagged) {
		AreaInf.area_flags |= le_us((tWORD)INF_SCANNING);
		RetVal = TRUE;
	    }

	    switch(msgs.Type) {
		case LOCALMAIL:
				break;

		case NETMAIL:	AreaInf.area_flags |= le_us((tWORD)(INF_ECHO+INF_NETMAIL+INF_HASFILE));
				break;

		case LIST:
		case ECHOMAIL:	AreaInf.area_flags |= le_us((tWORD)INF_ECHO);
				break;

//		case EMAIL:	AreaInf.area_flags |= le_us((tWORD)(INF_ECHO+INF_NETMAIL));
//				AreaInf.network_type |= le_us((tWORD)INF_NET_INTERNET);
//				break;

		case NEWS:	AreaInf.area_flags |= le_us((tWORD)INF_ECHO);
				AreaInf.network_type |= le_us((tWORD)INF_NET_INTERNET);
				break;
	    }
	    
	    switch(msgs.MsgKinds) {
		case BOTH:	if (Access(exitinfo.Security, msgs.WRSec))
				    AreaInf.area_flags |= le_us((tWORD)INF_POST);
				break;

		case PRIVATE:	if (Access(exitinfo.Security, msgs.WRSec))
				    AreaInf.area_flags |= le_us((tWORD)INF_POST);
				AreaInf.area_flags |= le_us((tWORD)INF_NO_PUBLIC);
				break;

		case PUBLIC:	if (Access(exitinfo.Security, msgs.WRSec))
				    AreaInf.area_flags |= le_us((tWORD)INF_POST);
				AreaInf.area_flags |= le_us((tWORD)INF_NO_PRIVATE);
				break;

		case RONLY:	break;
	    }

	    if (msgs.Aliases)
		AreaInf.area_flags |= le_us((tWORD)INF_ALIAS_NAME);

	    fwrite(&AreaInf, sizeof(AreaInf), 1, fp);
	}
    }
    fclose(fp);

    if (RetVal) {
	Area = 0;
	DrawBar(Pktname);
	fseek(mf, sizeof(msgshdr), SEEK_SET);
	fseek(tf, 0, SEEK_SET);

	while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	    fseek(mf, msgshdr.syssize, SEEK_CUR);
	    fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
	    Area++;
	    if (olrtagrec.Tagged) {
		if (Msg_Open(msgs.Base)) {
		    Current = Personal = 0;
		    if (Msg_Highest() != 0) {
			memset(&LR, 0, sizeof(LR));
			p = xstrcpy(exitinfo.sUserName);
			mycrc = StringCRC32(tl(p));
			free(p);
			LR.UserID = grecno;
			LR.UserCRC = mycrc;
			if (Msg_GetLastRead(&LR))
			    Start = LR.HighReadMsg;
			else
			    Start = Msg_Lowest() -1;
			if (Start > Msg_Highest())
			    Start = Msg_Highest();
			if (Start < Msg_Highest()) {
			    Syslog('m', "First %lu, Last %lu, Start %lu", Msg_Lowest(), Msg_Highest(), Start);
			    High = BlueWave_PackArea(Start, Area);
			    fill_high(&mhl, Area, msgs.Tag, High, Personal);
			}
		    }
		    Syslog('+', "Area %-20s %5ld (%ld personal)", msgs.QWKname, Current, Personal);
		    Msg_Close();
		}
	    }
	}
	Syslog('+', "Packed %ld messages (%ld personal)", Total, TotalPersonal);
    }
    fclose(tf);

    alarm_on();

    if (Total) {
	/*        Packing with */
	Enter(1);
	PUTSTR((char *)Language(446));
	PUTCHAR(' ');
	snprintf(Temp, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	if ((af = fopen(Temp, "r")) != NULL) {
	    fread(&archiverhdr, sizeof(archiverhdr), 1, af);
	    while (fread(&archiver, archiverhdr.recsize, 1, af) == 1) {
		if (archiver.available && (!strcmp(archiver.name, exitinfo.Archiver))) {
		    Syslog('+', "Archiver %s", archiver.comment);
		    PUTSTR(archiver.comment);
		    PUTCHAR(' ');
		    cwd = getcwd(cwd, PATH_MAX);
		    chdir(Work);
		    snprintf(Temp, PATH_MAX, "%s.DAT", CFG.bbsid);
		    AddArc(Temp, Pktname);
		    alarm_on();
		    snprintf(Temp, PATH_MAX, "%s.FTI", CFG.bbsid);
		    AddArc(Temp, Pktname);
		    snprintf(Temp, PATH_MAX, "%s.INF", CFG.bbsid);
		    AddArc(Temp, Pktname);
		    snprintf(Temp, PATH_MAX, "%s.MIX", CFG.bbsid);
		    AddArc(Temp, Pktname);
		    chdir(cwd);
		    free(cwd);
		    cwd = NULL;
		    snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s", CFG.bbs_usersdir, exitinfo.Name, Pktname);
		    rc = DownloadDirect(Temp, FALSE);
		    Syslog('m', "Download result %d", rc);
		    unlink(Temp);
		}
	    }
	    fclose(af);
	}
    }

    if (rc) {
	Syslog('+', "BlueWave download failed");
	/*      Download failed */
	poutCR(CFG.HiliteF, CFG.HiliteB, (char *)Language(447));
    } else {
	Syslog('+', "BlueWave download successfull");
	PUTCHAR('\r');
	/*        Download successfull */
	poutCR(CFG.HiliteF, CFG.HiliteB, (char *)Language(448));

	if (mhl != NULL)
	    UpdateLR(mhl, mf);
    }

    fclose(mf);
    tidy_high(&mhl);

    free(Temp);
    free(Work);
    Enter(2);
    Pause();
}



/*
 *  BlueWave Fetch reply packet.
 */
void BlueWave_Fetch()
{
    char	*temp, *buffer, *b, Dirpath[PATH_MAX], Filename[81], Echotag[20];
    FILE	*fp, *up = NULL, *mf, *tp = NULL, *iol = NULL;
    UPL_HEADER	Uph;
    UPL_REC	Upr;
    REQ_REC	Req;
    int		i = 0, j, Found, OLC_head, AreaChanges = FALSE;
    fidoaddr	dest;
    time_t	now;
    struct tm	*tm;

    /*      Processing BlueWave reply packet */
    poutCR(LIGHTBLUE, BLACK, (char *)Language(450));
    temp = calloc(PATH_MAX, sizeof(char));
    b = calloc(256, sizeof(char));
    buffer = b;

    /*
     *  Process uploaded mail
     */
    snprintf(Dirpath, PATH_MAX, "%s/%s", CFG.bbs_usersdir, exitinfo.Name);
    snprintf(Filename, 81, "%s.UPL", CFG.bbsid);
    if (getfilecase(Dirpath, Filename)) {
	snprintf(temp, PATH_MAX, "%s/%s", Dirpath, Filename);
	up = fopen(temp, "r");
    }
    if (up != NULL) {
	fread(&Uph, sizeof(UPL_HEADER), 1, up);
	Syslog('+', "Processing BlueWave v3 \"%s\" file", Filename);
	Syslog('+', "Client: %s %d.%d", Uph.reader_name, Uph.reader_major, Uph.reader_minor);
	if (le_us(Uph.upl_header_len) != sizeof(UPL_HEADER)) {
	    WriteError("Recordsize mismatch");
	    fclose(up);
	    free(temp);
	    /*      ERROR in packet */
	    poutCR(LIGHTRED, BLACK, (char *)Language(451));
	    Pause();
	    return;
	}
	Syslog('+', "Login %s, Alias %s", Uph.loginname, Uph.aliasname);
	Syslog('m', "Tear: %s", Uph.reader_tear);
	if (strlen((char *)Uph.reader_tear))
	    newtear = xstrcpy((char *)Uph.reader_tear);

	/* MORE CHECKS HERE */

	colour(CFG.TextColourF, CFG.TextColourB);
	/*      Import messages  */
	pout(CFG.TextColourF, CFG.TextColourB, (char *)Language(452));
	PUTCHAR(' ');
	colour(CFG.HiliteF, CFG.HiliteB);
	i = 0;

	memset(&Upr, 0, sizeof(UPL_REC));
	Syslog('m', "Start UPL reply records");
	while (fread(&Upr, le_us(Uph.upl_rec_len), 1, up) == 1) {
	    PUTCHAR('.');
	    Syslog('m', "  From  : %s", Upr.from);
	    Syslog('m', "  To    : %s", Upr.to);
	    Syslog('m', "  Subj  : %s", Upr.subj);
	    now = (time_t)le_int((int)Upr.unix_date);
	    tm = gmtime(&now);
	    Syslog('m', "  Date  : %02d-%02d-%d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon+1, 
		    tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	    Syslog('m', "  Dest  : %d:%d/%d.%d", le_us(Upr.destzone), le_us(Upr.destnet), 
		    le_us(Upr.destnode), le_us(Upr.destpoint));
	    if (Upr.msg_attr & le_us(UPL_INACTIVE))
		Syslog('m', "  Message is Inactive");
	    if (Upr.msg_attr & le_us(UPL_PRIVATE))
		Syslog('m', "  Message is Private");
	    if (Upr.msg_attr & le_us(UPL_HAS_FILE))
		Syslog('m', "  File Attach");
	    if (Upr.msg_attr & le_us(UPL_NETMAIL))
		Syslog('m', "  Is Netmail");
	    if (Upr.msg_attr & le_us(UPL_IS_REPLY))
		Syslog('m', "  Is Reply");
	    if (Upr.network_type)
		Syslog('m', "  Type  : Internet");
	    else
		Syslog('m', "  Type  : Fidonet");
	    getfilecase(Dirpath, (char *)Upr.filename);	
	    Syslog('m', "  File  : %s", Upr.filename);
	    Syslog('m', "  Tag   : %s", Upr.echotag);

	    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	    if ((mf = fopen(temp, "r+")) != NULL) {
		fread(&msgshdr, sizeof(msgshdr), 1, mf);
		Found = FALSE;

		if (strlen((char *)Upr.echotag)) {
		    while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
			fseek(mf, msgshdr.syssize, SEEK_CUR);
			if (msgs.Active && (strcasecmp(msgs.QWKname, (char *)Upr.echotag) == 0)) {
			    Found = TRUE;
			    break;
			}
		    }
		} else {
		    /*
		     *  If there is no echotag, the filename is used
		     *  this is "areanum.msgnum" so we pick the part
		     *  before the dot and pray that it's ok.
		     */
		    temp = strtok(strdup((char *)Upr.filename), ".");
		    if (fseek(mf, ((atoi(temp) -1) * (msgshdr.recsize + msgshdr.syssize)) + msgshdr.hdrsize, SEEK_SET) == 0)
			if (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
			    Found = TRUE;
			    fseek(mf, msgshdr.syssize, SEEK_CUR);
			}
		}

		/* SHOULD ALSO CHECK FROM FIELD */
		if (!Found) {
		    WriteError("No msg area, File \"%s\"", Upr.filename);
		} else {
		    if ((Access(exitinfo.Security, msgs.WRSec)) && (msgs.MsgKinds != RONLY)) {

			if (Open_Msgbase(msgs.Base, 'w')) {
			    Msg_New();
			    strcpy(Msg.From, (char *)Upr.from);
			    strcpy(Msg.To, (char *)Upr.to);
			    strcpy(Msg.Subject, (char *)Upr.subj);
			    mbse_CleanSubject(Msg.Subject);
			    if (Upr.msg_attr & le_us(UPL_PRIVATE))
				Msg.Private = TRUE;
			    if (msgs.MsgKinds == PRIVATE)
				Msg.Private = TRUE;
			    Msg.Written = le_int((int)Upr.unix_date) - (gmt_offset((time_t)0) * 60);
			    Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
			    Msg.Local  = TRUE;
			    dest.zone  = le_us(Upr.destzone);
			    dest.net   = le_us(Upr.destnet);
			    dest.node  = le_us(Upr.destnode);
			    dest.point = le_us(Upr.destpoint);
			    Add_Kludges(dest, FALSE, (char *)Upr.filename);
			    Syslog('+', "Msg (%ld) to \"%s\", \"%s\", in %s", Msg.Id, Msg.To, Msg.Subject, msgs.QWKname);
			    snprintf(temp, PATH_MAX, "%s/%s/%s", CFG.bbs_usersdir, exitinfo.Name, Upr.filename);
			    unlink(temp);
			    i++;
			    fseek(mf, - (msgshdr.recsize + msgshdr.syssize), SEEK_CUR);
			    msgs.Posted.total++;
			    msgs.Posted.tweek++;
			    msgs.Posted.tdow[Diw]++;
			    msgs.Posted.month[Miy]++;
			    msgs.LastPosted = time(NULL);
			    fwrite(&msgs, msgshdr.recsize, 1, mf);

			    /*
			     * Add quick mailscan info
			     */
			    if (msgs.Type != LOCALMAIL) {
				snprintf(temp, PATH_MAX, "%s/tmp/%smail.jam", getenv("MBSE_ROOT"),
					((msgs.Type == ECHOMAIL) || (msgs.Type == LIST))? "echo" : "net");
				if ((fp = fopen(temp, "a")) != NULL) {
				    fprintf(fp, "%s %u\n", msgs.Base, Msg.Id);
				    fclose(fp);
				}
			    }
			    Close_Msgbase(msgs.Base);
			}
		    } else {
			Enter(1);
			/*        No Write access to area */
			snprintf(temp, PATH_MAX, "%s %s", (char *)Language(453), msgs.Name);
			poutCR(LIGHTRED, BLACK, temp);
			WriteError("No Write Access to area %s", msgs.Name);
		    }
		}
		fclose(mf);
	    }
	    memset(&Upr, 0, sizeof(UPL_REC));
	}
	Syslog('m', "Stop UPL reply records");
	Enter(1);
	if (i) {
	    /*         Messages imported */
	    snprintf(temp, PATH_MAX, "%d %s", i, (char *)Language(454));
	    poutCR(CFG.TextColourF, CFG.TextColourB, temp);
	    ReadExitinfo();
	    exitinfo.iPosted += i;
	    mib_posted += i;
	    WriteExitinfo();
	    do_mailout = TRUE;
	}
	fclose(up);
	snprintf(temp, PATH_MAX, "%s/%s", Dirpath, Filename);
	unlink(temp);
    }

    if (newtear) {
	free(newtear);
	newtear = NULL;
    }

    /*
     *  Process offline configuration
     */
    snprintf(Filename, 81, "%s.OLC", CFG.bbsid);
    if (getfilecase(Dirpath, Filename)) {
	snprintf(temp, PATH_MAX, "%s/%s", Dirpath, Filename);
	iol = fopen(temp, "r");
    }
    if (iol != NULL) {
	/*      Processing Offline Configuration */
	poutCR(LIGHTBLUE, BLACK, (char *)Language(455));
	Syslog('+', "Processing BlueWave v3 configuration file \"%s\"", Filename);
	OLC_head = FALSE;
	
	while (fgets(b, 255, iol) != NULL ) {
	    buffer = b;
	    for (j = 0; j < strlen(b); j++) {
		if (*(b + j) == '\0')
		    break;
		if (*(b + j) == '\r')
		    *(b + j) = '\0';
		if (*(b + j) == '\n')
		    *(b + j) = '\0';
	    }
	    while (isspace(buffer[0]))
		buffer++;
	    Syslog('m', "Reading: \"%s\"", printable(buffer, 0));

	    /*
	     * The .OLC file starts with [Global Mail Host Configuration]
	     * Then some global options.
	     * Then each mail area like [AREA_TAG]
	     * Then the area options.
	     */
	    if ((strncasecmp(buffer,"[Global",7) == 0) && (strlen(buffer) > 22) ) {
		OLC_head = TRUE;
		continue;
	    } else {   
		if (buffer[0]=='[') {
		    strtok(buffer,"]");
		    buffer++;
		    strncpy(Echotag, buffer, 20);
		    if (OLC_head) {
			OLC_head = FALSE;
			if (AreaChanges) {
			    /*
			     * There are areachanges, first reset all areas.
			     */
			    Syslog('m', "Resetting all areas");
			    snprintf(temp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
			    if ((up = fopen(temp, "r+")) != NULL) {
				snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
				if ((mf = fopen(temp, "r")) != NULL) {
				    fread(&msgshdr, sizeof(msgshdr), 1, mf);
				    while (fread(&olrtagrec, sizeof(olrtagrec), 1, up) == 0) {
					fread(&msgs, msgshdr.recsize, 1, mf);
					fseek(mf, msgshdr.syssize, SEEK_CUR);
					if (!msgs.OLR_Forced)
					    olrtagrec.Tagged = FALSE;
					fseek(up, - sizeof(olrtagrec), SEEK_CUR);
					fwrite(&olrtagrec, sizeof(olrtagrec), 1, up);
				    }
				    fclose(mf);
				}
				fclose(up);
			    }
			    i = 0;
			}
		    continue;
		    }
		}
	    }

	    /*
	     * Process header commands
	     */
	    if (OLC_head == TRUE){
		if (strncasecmp(buffer,"AreaChanges", 11) == 0) { 
		    buffer += 11; 
		    while (isspace(buffer[0]) || buffer[0] == '=')
			buffer++;
		    if ((strncasecmp(buffer,"TRUE",4)==0) || (strncasecmp(buffer,"YES",3)==0) || (strncasecmp(buffer,"ON",2)==0))
			AreaChanges = TRUE;
		}
		if (strncasecmp(buffer,"MenuHotKeys", 11) == 0) { 
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"ExpertMenus", 11 ) == 0) {
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"SkipUserMsgs", 12) == 0) { 
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"DoorGraphics", 12) == 0) { 
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"Password", 8) == 0) {
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"Filter", 6) == 0) {
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"Keyword", 7) == 0) {
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		if (strncasecmp(buffer,"Macro",5) == 0) {
		    Syslog('m', "%s - ignored", printable(buffer, 0));
		}
		continue;
	    } else {
		if (strncasecmp(buffer,"Scan", 4) == 0) {
		    buffer+=4;
		    while (isspace(buffer[0]) || buffer[0] == '=') 
			buffer++;
		    if ((strncasecmp(buffer,"All",3)==0) || (strncasecmp(buffer,"Pers",4)==0)) {
			if (strlen(Echotag) > 0) {
			    snprintf(temp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
			    if ((up = fopen(temp, "r+")) != NULL) {
				snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
				if ((mf = fopen(temp, "r")) != NULL) {
				    fread(&msgshdr, sizeof(msgshdr), 1, mf);
				    while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
					fseek(mf, msgshdr.syssize, SEEK_CUR);
					fread(&olrtagrec, sizeof(olrtagrec), 1, up);
					if ((strcmp(msgs.QWKname, Echotag) == 0) && (msgs.Active) &&
						    (Access(exitinfo.Security, msgs.RDSec)) && strlen(msgs.QWKname)) {
					    Syslog('m', "  Area %s", Echotag);
					    olrtagrec.Tagged = TRUE;
					    fseek(up, - sizeof(olrtagrec), SEEK_CUR);
					    fwrite(&olrtagrec, sizeof(olrtagrec), 1, up);
					    i++;
					    break;
					}
				    }
				    fclose(mf);
				}
				fclose(up);
			    }
			}
		    }
		}
	    }
	}
	fclose(iol);
	snprintf(temp, PATH_MAX, "%s/%s", Dirpath, Filename);
	unlink(temp);
	/*         Message areas selected */
	snprintf(temp, PATH_MAX, "%d %s", i, (char *)Language(456));
	poutCR(CYAN, BLACK, temp);
	Syslog('+', "  %d active message areas.", i);
    }

    /*
     *  Check for .REQ file. This file should be saved in the users home
     *  directory so that the next time a OLR download is done the
     *  requests are added.
     *  FIXME: nothing implemented yet!
     */
    snprintf(Filename, 81, "%s.REQ", CFG.bbsid);
    if (getfilecase(Dirpath, Filename)) {
	tp = fopen(temp, "r");
    }
    if (tp != NULL) {
	i = 0;
//	colour(LIGHTBLUE, BLACK);
	/*      Processing file requests */
//	printf("%s\n", (char *)Language(457));
	Syslog('+', "Reading file requests %s (not supported)", Filename);

	while (fread(&Req, sizeof(REQ_REC), 1, tp) == 1) {
	    Syslog('m', "  File %s", Req.filename);
	    snprintf(temp, PATH_MAX, "%-12s ", Req.filename);
	    pout(CFG.TextColourF, CFG.TextColourB, temp);
	    colour(CFG.HiliteF, CFG.HiliteB);

	    Enter(1);
	}

	fclose(tp);
    }

    free(temp);
    free(b);
    Pause();
}



/*
 *  Add one area to the BlueWave download packet
 */
unsigned int BlueWave_PackArea(unsigned int ulLast, int Area)
{
    FILE	    *fdm, *fdfti, *fdmix;
    char	    *Temp, *Text, msg[81];
    unsigned int    Number;
    MIX_REC	    Mix;
    FTI_REC	    Fti;
    struct tm	    *tp;
    int		    Pack, length;

    Number = ulLast;
    Temp = calloc(PATH_MAX, sizeof(char));

    snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s.FTI", CFG.bbs_usersdir, exitinfo.Name, CFG.bbsid);
    fdfti = fopen(Temp, "a+");
    fseek(fdfti, 0, SEEK_END);	/* We need to do this, else ftell doesn't work right */

    snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s.MIX", CFG.bbs_usersdir, exitinfo.Name, CFG.bbsid);
    fdmix = fopen(Temp, "a+");
    fseek(fdmix, 0, SEEK_END);

    snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s.DAT", CFG.bbs_usersdir, exitinfo.Name, CFG.bbsid);
    fdm = fopen(Temp, "a+");
    fseek(fdm, 0, SEEK_END);

    memset(&Mix, 0, sizeof(MIX_REC));
    snprintf((char *)Mix.areanum, 6, "%u", Area);
//    Syslog('m', "fti position: %d", ftell(fdfti));
    Mix.msghptr = le_int((int)ftell(fdfti));

    if ((fdfti != NULL) && (fdmix != NULL) && (fdm != NULL)) {
	if (Msg_Next(&Number)) {
	    do {
		Msg_ReadHeader(Number);
		Msg_Read(Number, 79);
		Pack = TRUE;

		if ((strcasecmp(Msg.To, exitinfo.sUserName) == 0) || (strcasecmp(Msg.To, exitinfo.sHandle) == 0)) {
		    Personal++;
		    TotalPersonal++;
		} else if (msgs.Type == NETMAIL) {
		    Pack = FALSE;
		} else if (msgs.MsgKinds == PRIVATE ) {
		    Pack = FALSE;
		} else if (msgs.MsgKinds == BOTH ) {
		    if (Msg.Private == TRUE ) 
			Pack = FALSE;
                }

		if (Pack) {
		    Current++;
		    Total++;
		    memset (&Fti, 0, sizeof (FTI_REC));

		    strncpy((char *)Fti.from, Msg.From, 36);
		    strncpy((char *)Fti.to, Msg.To, 36);
		    strncpy((char *)Fti.subject, Msg.Subject, 72);
		    tp = localtime(&Msg.Written);
		    snprintf((char *)Fti.date, 20, "%2d %.3s %2d %2d:%02d:%02d", tp->tm_mday, 
			    (char *) Language(398 + tp->tm_mon), tp->tm_year, tp->tm_hour, tp->tm_min, tp->tm_sec);
		    Fti.msgnum = le_us((tWORD)Number);
		    Fti.msgptr = le_int((tLONG)ftell(fdm));
		    Fti.replyto = le_us((tWORD)Msg.Original);
		    Fti.replyat = le_us((tWORD)Msg.Reply);
		    if (msgs.Type == NETMAIL) {
			Fti.orig_zone = le_us(msgs.Aka.zone);
			Fti.orig_net  = le_us(msgs.Aka.net);
			Fti.orig_node = le_us(msgs.Aka.node);
		    }

		    length = (int)fwrite(" ", 1, 1, fdm);

		    if ((Text = (char *)MsgText_First()) != NULL) {
			do {
			    if ((Text[0] != 0x01) || (strncmp(Text, "\001MSGID", 6) == 0) ||
				((strncmp(Text, "\001FMPT ", 6) == 0) && (msgs.Type == NETMAIL)) || (exitinfo.OL_ExtInfo)) {
				length += fwrite(Text, 1, strlen(Text), fdm);
				length += fwrite("\r\n", 1, 2, fdm);
			    }
			} while ((Text = (char *)MsgText_Next()) != NULL);

			Fti.msglength = le_int(length);
			fwrite(&Fti, sizeof (Fti), 1, fdfti);
		    }
		}

		if (BarWidth != (unsigned short)((Total * 61L) / TotalPack)) {
		    BarWidth = (unsigned short)((Total * 61L) / TotalPack);
		    snprintf(msg, 81, "\r%.*s", BarWidth, "ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл");
		    pout(CYAN, BLACK, msg);
		}
	    } while (Msg_Next(&Number));
	}
    }

    Mix.totmsgs = le_us((tWORD)Current);
    Mix.numpers = le_us((tWORD)Personal);
//    Syslog('m', "mix: %6s %6d %6d %6d", Mix.areanum, Mix.totmsgs, Mix.numpers, Mix.msghptr);
    fwrite(&Mix, sizeof (Mix), 1, fdmix);

    if (fdfti != NULL)
	fclose(fdfti);
    if (fdmix != NULL)
	fclose(fdmix);
    if (fdm != NULL)
	fclose(fdm);

    free(Temp);
    return Number;
}



/***********************************************************************************
 *
 *  QWK specific functions
 *
 */



void OLR_DownQWK(void)
{
    struct tm	    *tp;
    time_t	    Now;
    char	    Pktname[32], *cwd = NULL, *Work, *Temp, *p;
    int		    Area = 0, i, rc = 0;
    FILE	    *fp = NULL, *tf, *mf, *af;
    unsigned int    Start, High, mycrc;
    msg_high	    *tmp, *mhl = NULL;

    if (strlen(CFG.bbsid) == 0) {
	poutCR(LIGHTRED, BLACK, (char *)"System configuration error, inform sysop");
	WriteError("Config OLR bbsid not configured");
	Pause();
	return;
    }

    if (!OLR_Prescan())
	return;

    Total = TotalPersonal = 0L;
    clear();
    /*      QWK Offline Download */
    poutCR(LIGHTBLUE, BLACK, (char *)Language(458));

    Work = calloc(PATH_MAX, sizeof(char));
    Temp = calloc(PATH_MAX, sizeof(char));

    Now = time(NULL);
    tp = localtime(&Now);
    Syslog('+', "Preparing QWK packet");

    snprintf(Temp, PATH_MAX, "%s.QWK", CFG.bbsid);
    snprintf(Pktname, 32, "%s", tl(Temp));
    Syslog('m', "Packet name %s", Pktname);
    snprintf(Work, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);
    Syslog('m', "Work path %s", Work);

    snprintf(Temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((mf = fopen(Temp, "r")) == NULL) {
	WriteError("$Can't open %s", Temp);
	fclose(fp);
	free(Temp);
	free(Work);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, mf);

    snprintf(Temp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
    if ((tf = fopen(Temp, "r")) == NULL) {
	WriteError("$Can't open %s", Temp);
	fclose(fp);
	fclose(mf);
	free(Temp);
	free(Work);
	return;
    }

    Area = 0;
    DrawBar(Pktname);
    fseek(mf, sizeof(msgshdr), SEEK_SET);
    fseek(tf, 0, SEEK_SET);

    while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	fseek(mf, msgshdr.syssize, SEEK_CUR);
	fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
	Area++;
	if (olrtagrec.Tagged) {
	    if (Msg_Open(msgs.Base)) {
		Current = Personal = 0;
		if (Msg_Highest() != 0) {
		    memset(&LR, 0, sizeof(LR));
		    p = xstrcpy(exitinfo.sUserName);
		    mycrc = StringCRC32(tl(p));
		    free(p);
		    LR.UserID = grecno;
		    LR.UserCRC = mycrc;
		    if (Msg_GetLastRead(&LR))
			Start = LR.HighReadMsg;
		    else
			Start = Msg_Lowest() -1;
		    if (Start > Msg_Highest())
			Start = Msg_Highest();
		    if (Start < Msg_Highest()) {
			Syslog('m', "First %lu, Last %lu, Start %lu", Msg_Lowest(), Msg_Highest(), Start);
			High = QWK_PackArea(Start, Area);
			fill_high(&mhl, Area, msgs.Tag, High, Personal);
		    }
		}
		Syslog('+', "Area %-20s %5ld (%ld personal)", msgs.QWKname, Current, Personal);
		Msg_Close();
	    }
	}
    }

    snprintf(Temp, PATH_MAX, "%s/CONTROL.DAT", Work);
    if ((fp = fopen(Temp, "w+")) != NULL) {
	fprintf(fp, "%s\n", CFG.bbs_name);
	fprintf(fp, "%s\n", CFG.location);
	fprintf(fp, "-Unpublished-\n");
	fprintf(fp, "%s\n", CFG.sysop_name);
	fprintf(fp, "00000,%s\n", CFG.bbsid);
		
	fprintf(fp, "%02d-%02d-%04d,%02d:%02d:%02d\n", tp->tm_mday, tp->tm_mon+1, tp->tm_year+1900, 
				tp->tm_hour, tp->tm_min, tp->tm_sec);
	snprintf(Temp, PATH_MAX, "%s", exitinfo.sUserName);
	fprintf(fp, "%s\n", tu(Temp));
	fprintf(fp, " \n");
	fprintf(fp, "0\n");
	fprintf(fp, "%u\n", Total);

	/*
	 * Count available areas.
	 */
	i = 0;
	fseek(mf, msgshdr.hdrsize, SEEK_SET);
	fseek(tf, 0, SEEK_SET);
	while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	    fseek(mf, msgshdr.syssize, SEEK_CUR);
	    if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname))
		i++;
	}
	fprintf(fp, "%d\n", i - 1);

	/*
	 * Write available areas
	 */
	i = 0;
	fseek(mf, msgshdr.hdrsize, SEEK_SET);
	fseek(tf, 0, SEEK_SET);
	while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	    fseek(mf, msgshdr.syssize, SEEK_CUR);
	    i++;
	    if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) && strlen(msgs.QWKname)) {
		fprintf(fp, "%d\n%s\n", i, msgs.QWKname);
	    }
	}

	fprintf(fp, "WELCOME\n");
	fprintf(fp, "NEWS\n");
	fprintf(fp, "GOODBYE\n");
	fclose(fp);	
    }

    snprintf(Temp, PATH_MAX, "%s/DOOR.ID", Work);
    if ((fp = fopen(Temp, "w+")) != 0) {
	fprintf(fp, "DOOR = MBSE BBS QWK\n");
	fprintf(fp, "VERSION = %s\n", VERSION);
	fprintf(fp, "SYSTEM = %s\n", CFG.bbs_name);
	fprintf(fp, "CONTROLNAME = MBSEQWK\n");
	fprintf(fp, "CONTROLTYPE = ADD\n");
	fprintf(fp, "CONTROLTYPE = DROP\n");
	/*
	 * QWKE extensions
	 */
//	fprintf(fp, "CONTROLTYPE = MAXKEYWORDS 0\n");
//	fprintf(fp, "CONTROLTYPE = MAXFILTERS 0\n");
//	fprintf(fp, "CONTROLTYPE = MAXTWITS 0\n");
//	fprintf(fp, "CONTROLTYPE = ALLOWATTACH\n");
//	fprintf(fp, "CONTROLTYPE = ALLOWFILES\n");
//	fprintf(fp, "CONTROLTYPE = ALLOWREQUESTS\n");
//	fprintf(fp, "CONTROLTYPE = MAXREQUESTS 0\n");
	fclose(fp);
    }

    Syslog('+', "Packed %ld messages (%ld personal)", Total, TotalPersonal);
    fclose(tf);

    if (Total) {
	Enter(1);
	/*        Packing with */
	PUTSTR((char *)Language(446));
	PUTCHAR(' ');
	snprintf(Temp, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	if ((af = fopen(Temp, "r")) != NULL) {
	    fread(&archiverhdr, sizeof(archiverhdr), 1, af);
	    while (fread(&archiver, archiverhdr.recsize, 1, af) == 1) {
		if (archiver.available && (!strcmp(archiver.name, exitinfo.Archiver))) {
		    Syslog('+', "Archiver %s", archiver.comment);
		    PUTSTR(archiver.comment);
		    PUTCHAR(' ');
		    cwd = getcwd(cwd, PATH_MAX);
		    Syslog('m', "chdir(%s) rc=%d", Work, chdir(Work));
		    snprintf(Temp, PATH_MAX, "CONTROL.DAT");
		    AddArc(Temp, Pktname);
		    alarm_on();
		    snprintf(Temp, PATH_MAX, "MESSAGES.DAT");
		    AddArc(Temp, Pktname);

		    for (tmp = mhl; tmp; tmp = tmp->next) {
			snprintf(Temp, PATH_MAX, "%03d.NDX", tmp->Area);
			AddArc(Temp, Pktname);
		    }

		    snprintf(Temp, PATH_MAX, "PERSONAL.NDX");
		    if (TotalPersonal) {
			AddArc(Temp, Pktname);
		    } else
			unlink(Temp);

		    snprintf(Temp, PATH_MAX, "DOOR.ID");
		    AddArc(Temp, Pktname);
		    chdir(cwd);
		    free(cwd);
		    cwd = NULL;
		    snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s", CFG.bbs_usersdir, exitinfo.Name, Pktname);
		    rc = DownloadDirect(Temp, FALSE);
		    Syslog('m', "Download result %d", rc);
		    unlink(Temp);
		}
	    }
	    fclose(af);
	}
    }

    if (rc) {
	Syslog('+', "QWK download failed");
	/*      Download failed */
	pout(CFG.HiliteF, CFG.HiliteB, (char *)Language(447));
    } else {
	Syslog('+', "QWK download successfull");
	PUTCHAR('\r');
	/*        Download successfull */
	poutCR(CFG.HiliteF, CFG.HiliteB, (char *)Language(448));

	if (mhl != NULL)
	    UpdateLR(mhl, mf);
    }
    fclose(mf);
    tidy_high(&mhl);

    free(Temp);
    free(Work);
    Enter(2);
    Pause();
}



/*
 *  QWK Fetch Reply packet
 */
void QWK_Fetch()
{
    char	    *temp, *otemp, Temp[128], szLine[132], *pLine = NULL, *pBuff, Dirpath[PATH_MAX], Filename[81];
    FILE	    *fp, *up = NULL, *op, *mf;
    unsigned short  nRec, i, r, x, nCol = 0, nWidth, nReaded, nPosted = 0;
    unsigned int    Area;
    struct tm	    *ltm = NULL;
    fidoaddr	    dest;
    int		    HasTear;

    /*      Processing BlueWave reply packet */
    poutCR(LIGHTBLUE, BLACK, (char *)Language(459));
    temp = calloc(PATH_MAX, sizeof(char));
    otemp = calloc(PATH_MAX, sizeof(char));
    nWidth = 78;

    snprintf(Dirpath, PATH_MAX, "%s/%s", CFG.bbs_usersdir, exitinfo.Name);
    snprintf(Filename, 81, "%s.MSG", CFG.bbsid);
    if (getfilecase(Dirpath, Filename)) {
	snprintf(temp, PATH_MAX, "%s/%s", Dirpath, Filename);
        up = fopen(temp, "r");
    }

    if (up != NULL) {
	Syslog('+', "Processing QWK file %s", Filename);

	fread(&Temp, 128, 1, up);
	Temp[8] = '\0';
	if (strcmp(CFG.bbsid, StripSpaces(Temp, 8))) {
	    Syslog('?', "Wrong QWK packet id: \"%s\"", StripSpaces(Temp, 8));
	    fclose(up);
	    unlink(temp);
	    /*      ERROR in packet */
	    poutCR(LIGHTRED, BLACK, (char *)Language(451));
	    free(temp);
	    free(otemp);
	    Pause();
	    return;
	}

	while (fread(&Qwk, sizeof(Qwk), 1, up) == 1) {
	    Area = atol(StripSpaces((char *)Qwk.Msgnum, sizeof(Qwk.Msgnum)));
	    nRec = atoi(StripSpaces((char *)Qwk.Msgrecs, sizeof(Qwk.Msgrecs)));

	    /*
	     * Test for blank records.
	     */
	    if (Area && nRec) {
		Syslog('m', "Conference %u", Area);
		Syslog('m', "Records    %d", nRec);
		Syslog('m', "To         %s", tlcap(StripSpaces((char *)Qwk.MsgTo, sizeof(Qwk.MsgTo))));
		Syslog('m', "From       %s", tlcap(StripSpaces((char *)Qwk.MsgFrom, sizeof(Qwk.MsgFrom))));
		Syslog('m', "Subject    %s", StripSpaces((char *)Qwk.MsgSubj, sizeof(Qwk.MsgSubj)));
		snprintf(Temp, 128, "%s", StripSpaces((char *)Qwk.Msgdate, sizeof(Qwk.Msgdate)));
		Syslog('m', "Date       %s %s", Temp, StripSpaces((char *)Qwk.Msgtime, sizeof(Qwk.Msgtime)));

		if (strcmp("MBSEQWK", StripSpaces((char *)Qwk.MsgTo, sizeof(Qwk.MsgTo))) == 0) {
		    Syslog('m', "Command %s", StripSpaces((char *)Qwk.MsgSubj, sizeof(Qwk.MsgSubj)));
		    snprintf(otemp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
		    if ((op = fopen(otemp, "r+")) != NULL) {

			snprintf(otemp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
			if ((mf = fopen(otemp, "r")) != NULL) {
			    fread(&msgshdr, sizeof(msgshdr), 1, mf);
			    fseek(mf, ((Area -1) * (msgshdr.recsize + msgshdr.syssize)) + msgshdr.hdrsize, SEEK_SET);
			    fread(&msgs, msgshdr.recsize, 1, mf);
			    fseek(op, (Area -1) * sizeof(olrtagrec), SEEK_SET);
			    fread(&olrtagrec, sizeof(olrtagrec), 1, op);

			    if (strcmp((char *)"ADD", StripSpaces((char *)Qwk.MsgSubj, sizeof(Qwk.MsgSubj))) == 0) {
				if (msgs.Active && Access(exitinfo.Security, msgs.RDSec) &&
						    strlen(msgs.QWKname) && !olrtagrec.Tagged) {
				    olrtagrec.Tagged = TRUE;
				    fseek(op, - sizeof(olrtagrec), SEEK_CUR);
				    Syslog('m', "%d", fwrite(&olrtagrec, sizeof(olrtagrec), 1, op));
				    Syslog('+', "QWK added area   %s", msgs.QWKname);
				}
			    }

			    if (strcmp((char *)"DROP", StripSpaces((char *)Qwk.MsgSubj, sizeof(Qwk.MsgSubj))) == 0) {
				if (!msgs.OLR_Forced && olrtagrec.Tagged) {
				    olrtagrec.Tagged = FALSE;
				    fseek(op, - sizeof(olrtagrec), SEEK_CUR);
				    fwrite(&olrtagrec, sizeof(olrtagrec), 1, op);
				    Syslog('+', "QWK dropped area %s", msgs.QWKname);
				}
			    }

			    fclose(mf);
			}
			fclose(op);
		    }
		} else {
		    /*
		     * Normal message
		     */
		    Syslog('m', "Message");
		    HasTear = FALSE;
		    snprintf(otemp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
		    if ((mf = fopen(otemp, "r+")) != NULL) {
			fread(&msgshdr, sizeof(msgshdr), 1, mf);
			if ((fseek(mf, ((Area -1) * (msgshdr.recsize + msgshdr.syssize)) + msgshdr.hdrsize, SEEK_SET) == 0) &&
			    (fread(&msgs, msgshdr.recsize, 1, mf) == 1)) {
			    Syslog('m', "pos %d, should be %d", ftell(mf), ((Area) * (msgshdr.recsize + msgshdr.syssize)) + msgshdr.hdrsize - msgshdr.syssize);

			    /*
			     * Check access to this area
			     */
			    Syslog('m', "%s %s", msgs.QWKname, msgs.Base);
			    if (msgs.Active && strlen(msgs.QWKname) && Access(exitinfo.Security, msgs.WRSec) && 
					(msgs.MsgKinds != RONLY)) {
				if (Open_Msgbase(msgs.Base, 'w')) {
				    Msg_New();
				    pLine = szLine;
				    nCol = 0;
				    Syslog('m', "Msgbase open and locked");
				    strcpy(Msg.From, tlcap(StripSpaces((char *)Qwk.MsgFrom, sizeof(Qwk.MsgFrom))));
				    strcpy(Msg.To, tlcap(StripSpaces((char *)Qwk.MsgTo, sizeof(Qwk.MsgTo))));
				    strcpy(Msg.Subject, StripSpaces((char *)Qwk.MsgSubj, sizeof(Qwk.MsgSubj)));
				    if ((Qwk.Msgstat == '*') || (Qwk.Msgstat == '+'))
					Msg.Private = TRUE;
				    strcpy(Temp, StripSpaces((char *)Qwk.Msgdate, sizeof(Qwk.Msgdate)));
				    ltm = malloc(sizeof(struct tm));
				    memset(ltm, 0, sizeof(struct tm));
				    ltm->tm_mday = atoi(&Temp[3]);
				    ltm->tm_mon = atoi(&Temp[0]) -1;
				    ltm->tm_year = atoi(&Temp[6]);
				    if (ltm->tm_year < 96)
					ltm->tm_year += 100;
				    strcpy(Temp, StripSpaces((char *)Qwk.Msgtime, sizeof(Qwk.Msgtime)));
				    ltm->tm_hour = atoi(&Temp[0]);
				    ltm->tm_min = atoi(&Temp[3]);
				    ltm->tm_sec = 0;
				    Msg.Written = mktime(ltm);
				    free(ltm);
				    Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
				    Msg.Local  = TRUE;
				    memset(&dest, 0, sizeof(dest));
//				    dest.zone  = Upr.destzone;
//				    dest.net   = Upr.destnet;
//				    dest.node  = Upr.destnode;
//				    dest.point = Upr.destpoint;
				    Add_Headkludges(fido2faddr(dest), FALSE);

				    for (r = 1; r < nRec; r++) {
					nReaded = fread(Temp, 1, 128, up);
					Syslog('m', "nReaded=%d", nReaded);
					if (r == (nRec - 1)) {
					    x = 127;
					    while (x > 0 && Temp[x] == ' ') {
						nReaded--;
						x--;
					    }
//					    Syslog('m', "Final=%d", nReaded);
					}

					for (i = 0, pBuff = Temp; i < nReaded; i++, pBuff++) {
					    if (*pBuff == '\r' || *pBuff == (char)0xE3) {
						*pLine = '\0';
//						Syslog('m', "1 Len=%d \"%s\"", strlen(szLine), printable(szLine, 0));
						MsgText_Add2(szLine);
						if (strncmp(szLine, (char *)"--- ", 4) == 0)
						    HasTear = TRUE;
						pLine = szLine;
						nCol = 0;
					    } else if (*pBuff != '\n') {
						*pLine++ = *pBuff;
						nCol++;
						if (nCol >= nWidth) {
						    *pLine = '\0';
						    while (nCol > 1 && *pLine != ' ') {
							nCol--;
							pLine--;
						    }
						    if (nCol > 0) {
							while (*pLine == ' ')
							    pLine++;
							strcpy (szWrp, pLine);
						    }
						    *pLine = '\0';
						    Syslog('m', "2 Len=%d \"%s\"", strlen(szLine), printable(szLine, 0));
						    MsgText_Add2(szLine);
						    if (strncmp(szLine, (char *)"--- ", 4) == 0)
							HasTear = TRUE;
						    strcpy(szLine, szWrp);
						    pLine = strchr(szLine, '\0');
						    nCol = (short)strlen (szLine);
						}
					    }
					}
				    }
				    if (nCol > 0) {
					*pLine = '\0';
					Syslog('m', "3 Len=%d \"%s\"", strlen(szLine), printable(szLine, 0));
					MsgText_Add2(szLine);
					if (strncmp(szLine, (char *)"--- ", 4) == 0)
					    HasTear = TRUE;
				    }

				    Add_Footkludges(FALSE, NULL, HasTear);
				    Msg_AddMsg();
				    Msg_UnLock();

				    Syslog('+', "Msg (%ld) to \"%s\", \"%s\", in %s", Msg.Id, Msg.To, Msg.Subject, msgs.QWKname);
				    nPosted++;
				    fseek(mf, ((Area -1) * (msgshdr.recsize + msgshdr.syssize)) + msgshdr.hdrsize, SEEK_SET);
				    msgs.Posted.total++;
				    msgs.Posted.tweek++;
				    msgs.Posted.tdow[Diw]++;
				    msgs.Posted.month[Miy]++;
				    msgs.LastPosted = time(NULL);
				    fwrite(&msgs, msgshdr.recsize, 1, mf);

				    /*
				     * Add quick mailscan info
				     */
				    if (msgs.Type != LOCALMAIL) {
					snprintf(temp, PATH_MAX, "%s/tmp/%smail.jam", getenv("MBSE_ROOT"),
					    ((msgs.Type == ECHOMAIL) || (msgs.Type == LIST))? "echo" : "net");
					if ((fp = fopen(temp, "a")) != NULL) {
					    fprintf(fp, "%s %u\n", msgs.Base, Msg.Id);
					    fclose(fp);
					}
				    }
				    Close_Msgbase(msgs.Base);
				}
			    } else {
				Syslog('+', "Can't post messages in area %u", Area);
			    }
			    fclose(mf);
			} else {
			    WriteError("$Can't read message area %u", Area);
			}
		    }
		}
	    } else {
		Syslog('m', "Skip blank record");
	    }
	}

	fclose(up);
    }

    Enter(1);
    if (nPosted) {
	/*         Messages imported */
	snprintf(temp, 81, "%d %s", nPosted, (char *)Language(454));
	poutCR(CFG.TextColourF, CFG.TextColourB, temp);
	ReadExitinfo();
	exitinfo.iPosted += nPosted;
	mib_posted += nPosted;
	WriteExitinfo();
	do_mailout = TRUE;
    }
    snprintf(temp, PATH_MAX, "%s/%s", Dirpath, Filename);
    Syslog('m', "Unlink %s rc=%d", temp, unlink(temp));
    free(temp);
    free(otemp);
    Pause();
}



union Converter {
    unsigned char   uc[10];
    unsigned short  ui[5];
    unsigned int    ul[2];
    float	    f[2];
    double	    d[1];
};



float IEEToMSBIN(float f)
{
    int		    sign, expo;
    union Converter t;

    t.f[0] = f;
    sign = t.uc[3] / 0x80;
    expo = ((t.ui[1] >> 7) - 0x7F + 0x81) & 0xFF;
    t.ui[1] = (t.ui[1] & 0x7F) | (sign << 7) | (expo << 8);

    return t.f[0];
}



float MSBINToIEEE(float f)
{
    union Converter t;
    int		    sign, expo;

    t.f[0] = f;
    sign = t.uc[2] / 0x80;
    expo = (t.uc[3] - 0x81 + 0x7f) & 0xff;
    t.ui[1] = (t.ui[1] & 0x7f) | (expo << 7) | (sign << 15);
    return t.f[0];
}



/*
 * Pack messages in one mail area
 */
unsigned int QWK_PackArea(unsigned int ulLast, int Area)
{
    FILE	    *fdm, *fdi, *fdp;
    float	    out, in;
    char	    *Work, *Temp, *Text, msg[81];
    unsigned int    Number, Pos, Size, Blocks;
    int		    Pack = FALSE;
    struct tm	    *tp;

    Number = ulLast;
    Current = Personal = 0L;

    Temp = calloc(PATH_MAX, sizeof(char));
    Work = calloc(PATH_MAX, sizeof(char));
    snprintf(Work, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);

    snprintf(Temp, PATH_MAX, "%s/%03d.NDX", Work, Area);
    fdi = fopen(Temp, "a+");

    snprintf(Temp, PATH_MAX, "%s/PERSONAL.NDX", Work);
    fdp = fopen(Temp, "a+");

    /*
     * Open MESSAGES.DAT, if it doesn't exist, create it and write
     * the header. Then reopen the file in r/w mode.
     */
    snprintf(Temp, PATH_MAX, "%s/MESSAGES.DAT", Work);
    if ((fdm = fopen (Temp, "r+")) == NULL) {
	Syslog('m', "Creating new %s", Temp);
	fdm = fopen(Temp, "a+");
	// ----------------------------------------------------------------------
	// The first record of the MESSAGE.DAT file must be the Sparkware id
	// block, otherwise some applications may complain.
	// ----------------------------------------------------------------------
	fprintf(fdm, "Produced by Qmail...");
	fprintf(fdm, "Copywright (c) 1987 by Sparkware.  ");
	fprintf(fdm, "All Rights Reserved");
	memset(Temp, ' ', 54);
	fwrite(Temp, 54, 1, fdm);
	fclose(fdm);
	snprintf(Temp, PATH_MAX, "%s/MESSAGES.DAT", Work);
	fdm = fopen(Temp, "r+");
    }

    if ((fdm != NULL) && (fdp != NULL) && (fdi != NULL)) {
	fseek(fdm, 0, SEEK_END);
	if (Msg_Next(&Number)) {
	    do {
		Msg_ReadHeader(Number);
		Msg_Read(Number, 79);
		Pack = TRUE;
		if ((strcasecmp(Msg.To, exitinfo.sUserName) == 0) || (strcasecmp(Msg.To, exitinfo.sHandle) == 0)) {
		    Personal++;
		    TotalPersonal++;
		    fwrite(&out, sizeof(float), 1, fdp);
		    fwrite("", 1, 1, fdp);
		} else if (msgs.Type == NETMAIL) {
		    Pack = FALSE;
		} else if (msgs.MsgKinds == PRIVATE ) {
		    Pack = FALSE;
		} else if (msgs.MsgKinds == BOTH ) {
		    if (Msg.Private == TRUE ) 
			Pack = FALSE;
		}

		if (Pack) {
		    /*
		     * Calculate the recordnumber from the current file
		     * position and store it in M$oft BIN format.
		     */
                    Pos = ftell(fdm);
		    Blocks = (Pos / 128L) + 1L;
		    snprintf(Temp, 6, "%u", Blocks);
		    in = atof(Temp);
		    out = IEEToMSBIN(in);
		    fwrite(&out, sizeof(float), 1, fdi);
		    fwrite(" ", 1, 1, fdi);
		    Current++;
		    Total++;

		    memset(&Qwk, ' ', sizeof(Qwk));
		    snprintf(Temp, 81, "%-*u", (int)sizeof(Qwk.Msgnum), (int)Number);
		    memcpy(Qwk.Msgnum, Temp, sizeof(Qwk.Msgnum));
		    tp = localtime(&Msg.Written);
		    snprintf(Temp, 81, "%02d-%02d-%02d", tp->tm_mon+1, tp->tm_mday, tp->tm_year % 100);
		    memcpy(Qwk.Msgdate, Temp, sizeof(Qwk.Msgdate));
		    snprintf(Temp, 81, "%02d:%02d", tp->tm_hour, tp->tm_min);
		    memcpy(Qwk.Msgtime, Temp, sizeof(Qwk.Msgtime));
		    Msg.From[sizeof(Qwk.MsgFrom) - 1] = '\0';
		    memcpy(Qwk.MsgFrom, Msg.From, strlen(Msg.From));
		    Msg.To[sizeof(Qwk.MsgTo) - 1] = '\0';
		    memcpy(Qwk.MsgTo, Msg.To, strlen(Msg.To));
		    Msg.Subject[sizeof(Qwk.MsgSubj) - 1] = '\0';
		    memcpy(Qwk.MsgSubj, Msg.Subject, strlen(Msg.Subject));
		    Qwk.Msglive = 0xE1;
		    Qwk.Msgarealo = (unsigned char)(Area & 0xFF);
		    Qwk.Msgareahi = (unsigned char)((Area & 0xFF00) >> 8);
		    fwrite(&Qwk, sizeof(Qwk), 1, fdm);
		    Size = 128L;
		    if ((Text = (char *)MsgText_First()) != NULL) {
			do {
			    if ((Text[0] != 0x01) || (strncmp(Text, "\001MSGID", 6) == 0) ||
				((strncmp(Text, "\001FMPT ", 6) == 0) && (msgs.Type == NETMAIL)) || (exitinfo.OL_ExtInfo)) {
				Size += fwrite(Text, 1, strlen(Text), fdm);
				Size += fwrite("\xE3", 1, 1, fdm);
			    }
			} while ((Text = (char *)MsgText_Next()) != NULL);

			if ((Size % 128L) != 0) {
			    memset(Temp, ' ', 128);
			    Size += fwrite(Temp, (int)(128L - (Size % 128L)), 1, fdm);
			}

			snprintf((char *)Qwk.Msgrecs, 6, "%-*u", (int)sizeof(Qwk.Msgrecs), (int)((ftell(fdm) - Pos) / 128L));
			fseek(fdm, Pos, SEEK_SET);
			fwrite(&Qwk, sizeof(Qwk), 1, fdm);
			fseek(fdm, 0L, SEEK_END);
			if ((Total % 16L) == 0L)
			    msleep(1);
		    }

		    if (BarWidth != (unsigned short)((Total * 61L) / TotalPack)) {
			BarWidth = (unsigned short)((Total * 61L) / TotalPack);
			PUTCHAR('\r');
			snprintf(msg, 81, "%.*s", BarWidth, "ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл");
			pout(CYAN, BLACK, msg);
		    }
		}
	    } while (Msg_Next(&Number));
	}
    } else {
	WriteError("Not all files open");
    }

    if (fdm != NULL)
	fclose(fdm);
    if (fdi != NULL)
	fclose(fdi);
    if (fdp != NULL)
	fclose(fdp);
    free(Work);
    free(Temp);

    return Number;
}



char *StripSpaces(char *String, int Size)
{
    int	x;

    memcpy(TempStr, String, Size);
    TempStr[Size] = '\0';
    if ((x = (Size - 1)) > 0) {
	while (x > 0 && TempStr[x] == ' ')
	    TempStr[x--] = '\0';
    }

    return TempStr;
}


/*****************************************************************************
 *
 *  ASCII Offline Functions
 *
 */


void OLR_DownASCII(void)
{
    char            Pktname[32], *Work, *Temp, *cwd = NULL, Atag[60], Kinds[12], *p;
    int		    Area = 0, i, rc = 0;
    FILE            *fp = NULL, *tf, *mf, *af, *inf;
    unsigned int    Start, High, mycrc;
    msg_high        *tmp, *mhl = NULL;

    if (strlen(CFG.bbsid) == 0) {
	poutCR(LIGHTRED, BLACK, (char *)"System configuration error, inform sysop");
	WriteError("Config OLR bbsid not configured");
	Pause();
	return;
    }

    if (!OLR_Prescan())
	return;

    Total = TotalPersonal = 0L;
    clear();
    /*      ASCII Offline Download */
    poutCR(LIGHTBLUE, BLACK, (char *)Language(460));

    Work = calloc(PATH_MAX, sizeof(char));
    Temp = calloc(PATH_MAX, sizeof(char));
	
    Syslog('+', "Preparing ASCII packet");

    snprintf(Temp, PATH_MAX, "%s.MSG", CFG.bbsid);
    snprintf(Pktname, 32, "%s", tl(Temp));
    snprintf(Work, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);

    snprintf(Temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((mf = fopen(Temp, "r")) == NULL) {
	WriteError("$Can't open %s", Temp);
	fclose(fp);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, mf);

    snprintf(Temp, PATH_MAX, "%s/%s/.olrtags", CFG.bbs_usersdir, exitinfo.Name);
    if ((tf = fopen(Temp, "r")) == NULL) {
	WriteError("$Can't open %s", Temp);
	fclose(fp);
	fclose(mf);
	return;
    }

    snprintf(Atag, 9, CFG.bbsid);
    tl(Atag);
    snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s.info", CFG.bbs_usersdir, exitinfo.Name, Atag);
    if ((inf = fopen(Temp, "a+")) == NULL) {
	WriteError("$Can't create %s", Temp);
	fclose(tf);
	fclose(fp);
	fclose(mf);
	return;
    }

    /*
     * Write generic bbs info
     */
    fprintf(inf, "BBS %s\n", CFG.bbs_name);
    for (i = 0; i < 40; i++) {
	if (CFG.akavalid[i])
	    fprintf(inf, "Aka %s\n", aka2str(CFG.aka[i]));
    }
    fprintf(inf, "Domain %s\n", CFG.sysdomain);
    fprintf(inf, "Version mbsebbs v%s\n", VERSION);
    fprintf(inf, "\n");

    Area = 0;
    DrawBar(Pktname);
    fseek(mf, sizeof(msgshdr), SEEK_SET);
    fseek(tf, 0, SEEK_SET);


    while (fread(&msgs, msgshdr.recsize, 1, mf) == 1) {
	fseek(mf, msgshdr.syssize, SEEK_CUR);
	fread(&olrtagrec, sizeof(olrtagrec), 1, tf);
	Area++;

	if (msgs.Active && Access(exitinfo.Security, msgs.RDSec)) {
	    if (strlen(msgs.Tag))
		snprintf(Atag, 51, msgs.Tag);
	    else
		snprintf(Atag, 51, "%d", Area);
	    switch (msgs.Type) {
		case LOCALMAIL:	snprintf(Temp, 81, "Local");
				break;
		case NETMAIL:	snprintf(Temp, 81, "Net");
				break;
		case ECHOMAIL:	snprintf(Temp, 81, "Echo");
				break;
		case NEWS:	snprintf(Temp, 81, "News");
				break;
		case LIST:	snprintf(Temp, 81, "List");
				break;
	    }
	    switch (msgs.MsgKinds) {
		case BOTH:	snprintf(Kinds, 12, "PvtPub");
				break;
		case PRIVATE:	snprintf(Kinds, 12, "Pvt");
				break;
		case PUBLIC:	snprintf(Kinds, 12, "Pub");
				break;
		case RONLY:	snprintf(Kinds, 12, "RO");
				break;
		case FTNMOD:	snprintf(Kinds, 12, "FtnMod");
				break;
		case USEMOD:	snprintf(Kinds, 12, "UseMod");
				break;
	    }			
	    fprintf(inf, "Area %s,%s,%s,%s,%s\n", Atag, Temp, Kinds, olrtagrec.Tagged ?"S":"U", msgs.Name);
	}

	if (olrtagrec.Tagged) {
	    if (Msg_Open(msgs.Base)) {
		Current = Personal = 0;
		if (Msg_Highest() != 0) {
		    memset(&LR, 0, sizeof(LR));
		    p = xstrcpy(exitinfo.sUserName);
		    mycrc = StringCRC32(tl(p));
		    free(p);
		    LR.UserCRC = mycrc;
		    LR.UserID = grecno;
		    if (Msg_GetLastRead(&LR))
			Start = LR.HighReadMsg;
		    else
			Start = Msg_Lowest() -1;
		    if (Start > Msg_Highest())
			Start = Msg_Highest();
		    if (Start < Msg_Highest()) {
			if (strlen(msgs.Tag))
			    snprintf(Atag, 60, "%s_%s", CFG.bbsid, msgs.Tag);
			else
			    snprintf(Atag, 60, "%s_%d", CFG.bbsid, Area);
			for (i = 0; i < strlen(Atag); i++) {
			    if (Atag[i] == '.')
				Atag[i] = '_';
			    Atag[i] = tolower(Atag[i]);
			}
			High = ASCII_PackArea(Start, Area, Atag);
			fill_high(&mhl, Area, Atag, High, Personal);
		    }
		}
		Syslog('+', "Area %-20s %5ld (%ld personal)", msgs.QWKname, Current, Personal);
		Msg_Close();
	    }
	}
    }
    fclose(inf);

    /*
     * In ASCII mode we allways download, even if there are no messages
     * packed. However we do have a .info file for the user.
     */
    Enter(1);
    /*        Packing with */
    PUTSTR((char *)Language(446));
    PUTCHAR(' ');
    snprintf(Temp, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
    if ((af = fopen(Temp, "r")) != NULL) {
        fread(&archiverhdr, sizeof(archiverhdr), 1, af);
        while (fread(&archiver, archiverhdr.recsize, 1, af) == 1) {
	    if (archiver.available && (!strcmp(archiver.name, exitinfo.Archiver))) {
	        Syslog('+', "Archiver %s", archiver.comment);
	        PUTSTR(archiver.comment);
	        PUTCHAR(' ');
	        cwd = getcwd(cwd, PATH_MAX);
	        chdir(Work);
	        alarm_on();

		snprintf(Temp, PATH_MAX, "%s.info", CFG.bbsid);
		tl(Temp);
		AddArc(Temp, Pktname);
		
	        for (tmp = mhl; tmp; tmp = tmp->next) {
		    snprintf(Temp, PATH_MAX, "%s.text", tmp->Tag);
		    AddArc(Temp, Pktname);
		}
	        chdir(cwd);
	        free(cwd);
	        cwd = NULL;
	        snprintf(Temp, PATH_MAX, "%s/%s/tmp/%s", CFG.bbs_usersdir, exitinfo.Name, Pktname);
	        rc = DownloadDirect(Temp, FALSE);
	        unlink(Temp);
	    }
	}
	fclose(af);
    }

    if (rc) {
	Syslog('+', "ASCII download failed");
	/*      Download failed */
	pout(CFG.HiliteF, CFG.HiliteB, (char *)Language(447));
    } else {
	Syslog('+', "ASCII download successfull");
	PUTCHAR('\r');
	/*        Download successfull */
	poutCR(CFG.HiliteF, CFG.HiliteB, (char *)Language(448));

	if (mhl != NULL)
	    UpdateLR(mhl, mf);
    }
    fclose(mf);
    tidy_high(&mhl);
    free(Temp);
    free(Work);
    Enter(2);
    Pause();
}



/*
 * Pack messages in one mail area
 */
unsigned int ASCII_PackArea(unsigned int ulLast, int Area, char *Atag)
{
    FILE            *fp;
    char            *Work, *Temp, *Text, msg[81];
    unsigned int    Number;
    int             Pack = FALSE;
    time_t	    now;

    Number = ulLast;
    Current = Personal = 0L;

    Temp = calloc(PATH_MAX, sizeof(char));
    Work = calloc(PATH_MAX, sizeof(char));
    snprintf(Work, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);

    snprintf(Temp, PATH_MAX, "%s/%s.text", Work, Atag);
    if ((fp = fopen(Temp, "a+")) != NULL) {
	if (Msg_Next(&Number)) {
	    do {
		Msg_ReadHeader(Number);
		Msg_Read(Number, 79);
		Pack = TRUE;
		if ((strcasecmp(Msg.To, exitinfo.sUserName) == 0) || (strcasecmp(Msg.To, exitinfo.sHandle) == 0)) {
                    Personal++;
		    TotalPersonal++;
		} else if (msgs.Type == NETMAIL) {
		    Pack = FALSE;
		} else if ( msgs.MsgKinds == PRIVATE ) {
		    Pack = FALSE;
		} else if (msgs.MsgKinds == BOTH ) {
		    if ( Msg.Private == TRUE ) 
			Pack = FALSE;
		}

                if (Pack) {
		    fprintf(fp, "#@ olrmsg\n");
//		    fprintf(fp, "  Msg: #%d of %d (%s)\n", Number, Msg_Number(), msgs.Name);
//		    tp = localtime(&Msg.Written);
//		    fprintf(fp, " Date: %d %s %d %2d:%02d\n", tp->tm_mday, 
//						GetMonth(tp->tm_mon + 1), tp->tm_year, tp->tm_hour, tp->tm_min);
		    now = (time_t)Msg.Written;
		    fprintf(fp, "Date: %s\n", rfcdate(now));
		    fprintf(fp, "From: %s\n", Msg.From);
		    if (strlen(Msg.FromAddress))
			fprintf(fp, "From-ftn: %s\n", Msg.FromAddress);
		    fprintf(fp, "Subject: %s\n", Msg.Subject);
		    if (Msg.To[0]) {
			fprintf (fp, "To: %s\n", Msg.To);
			if (strlen(Msg.ToAddress))
			    fprintf(fp, "To-ftn: %s\n", Msg.ToAddress);
		    }

		    /*
		     * If present, add the msgid, origin line and reply.
		     */
		    if ((Text = (char *)MsgText_First()) != NULL) {
			do {
			    if (strncmp(Text, "\001MSGID: ", 8) == 0) {
				fprintf(fp, "Message-ID: %s\n", Text+8);
			    }
			    if (strncmp(Text, "\001REPLY: ", 8) == 0) {
				fprintf(fp, "In-Reply-To: %s\n", Text+8);
			    }
			    if (strncmp(Text, " * Origin: ", 11) == 0) {
				fprintf(fp, "Organisation: %s\n", Text+11);
			    }
			} while ((Text = (char *)MsgText_Next()) != NULL);
		    }
		    fprintf(fp, "\n");
		    Current++;
		    Total++;

		    /*
		     * Add message body
		     */
		    if ((Text = (char *)MsgText_First()) != NULL) {
			do {
			    if (Text[0] != 0x01 && strncmp(Text, "SEEN-BY: ", 9) && strncmp(Text, " * Origin: ", 11)) {
				if (strcmp(Text, "#@ olrmsg") == 0)
				    fprintf(fp, "-");
				fprintf(fp, "%s\n", Text);
			    }
			} while ((Text = (char *)MsgText_Next()) != NULL);
		    }

		    if ((Total % 16L) == 0L)
			msleep(1);

		    if (BarWidth != (unsigned short)((Total * 61L) / TotalPack)) {
			BarWidth = (unsigned short)((Total * 61L) / TotalPack);
			PUTCHAR('\r');
			snprintf(msg, 81, "%.*s", BarWidth, "ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл");
			pout(CYAN, BLACK, msg);
		    }
		}
	    } while (Msg_Next(&Number));
	}
	fclose(fp);
    } else {
	WriteError("Not all files open");
    }

    free(Work);
    free(Temp);
    return Number;
}


