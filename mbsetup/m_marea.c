/*****************************************************************************
 *
 * $Id: m_marea.c,v 1.58 2007/03/02 15:29:51 mbse Exp $
 * Purpose ...............: Message Areas Setup
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/msg.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "grlist.h"
#include "m_global.h"
#include "m_node.h"
#include "m_mgroup.h"
#include "m_marea.h"


int		MsgUpdated = 0;
unsigned int	MsgCrc;
FILE		*tfil = NULL;
extern int	exp_golded;
int		MailForced = FALSE;


/*
 * Count nr of msgs records in the database.
 * Creates the database if it doesn't exist.
 */
int CountMsgarea(void)
{
    FILE		*fil;
    char		ffile[PATH_MAX];
    int			count, i;
    struct _sysconnect	syscon;

    snprintf(ffile, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    msgshdr.hdrsize = sizeof(msgshdr);
	    msgshdr.recsize = sizeof(msgs);
	    msgshdr.syssize = CFG.toss_systems * sizeof(sysconnect);
	    msgshdr.lastupd = time(NULL);
	    fwrite(&msgshdr, sizeof(msgshdr), 1, fil);
	    /*
	     * Default first message area
	     */
	    memset(&msgs, 0, sizeof(msgs));
	    snprintf(msgs.Name, 41, "Local users chat");
	    snprintf(msgs.Base, 65, "%s/var/mail/local/users", getenv("MBSE_ROOT"));
	    snprintf(msgs.QWKname, 21, "LOC_USERS");
	    snprintf(msgs.Group, 13, "LOCAL");
	    msgs.Active = TRUE;
	    msgs.Type = LOCALMAIL;
	    msgs.MsgKinds = PUBLIC;
	    msgs.SYSec.level = 32000;
	    msgs.UsrDelete = TRUE;
	    msgs.Aliases = TRUE;
	    msgs.Quotes = TRUE;
	    msgs.DaysOld = CFG.defdays;
	    msgs.MaxMsgs = CFG.defmsgs;
	    msgs.Charset = FTNC_NONE;
	    strcpy(msgs.Origin, CFG.origin);
	    msgs.Created = time(NULL);
	    fwrite(&msgs, sizeof(msgs), 1, fil);
	    mkdirs(msgs.Base, 0770);
	    if (Msg_Open(msgs.Base))
		Msg_Close();
	    memset(&syscon, 0, sizeof(syscon));
	    for (i = 1; i <= CFG.toss_systems; i++)
		fwrite(&syscon, sizeof(syscon), 1, fil);
	    /*
	     * Default message area for badmail
	     */
	    memset(&msgs, 0, sizeof(msgs));
	    snprintf(msgs.Name, 41, "Bad mail");
	    snprintf(msgs.Base, 65, "%s/var/mail/badmail", getenv("MBSE_ROOT"));
	    snprintf(msgs.QWKname, 21, "BADMAIL");
	    snprintf(msgs.Group, 13, "LOCAL");
	    msgs.Active = TRUE;
	    msgs.Type = LOCALMAIL;
	    msgs.MsgKinds = PUBLIC;
	    msgs.RDSec.level = 32000;
	    msgs.WRSec.level = 32000;
	    msgs.SYSec.level = 32000;
	    msgs.DaysOld = CFG.defdays;
	    msgs.MaxMsgs = CFG.defmsgs;
	    msgs.Charset = FTNC_NONE;
	    msgs.Created = time(NULL);
	    fwrite(&msgs, sizeof(msgs), 1, fil);
	    mkdirs(msgs.Base, 0770);
	    if (Msg_Open(msgs.Base))
		Msg_Close();
	    for (i = 1; i <= CFG.toss_systems; i++)
		fwrite(&syscon, sizeof(syscon), 1, fil);
	    /*
	     * Default dupemail message area
	     */
	    memset(&msgs, 0, sizeof(msgs));
	    snprintf(msgs.Name, 41, "Dupe mail");
	    snprintf(msgs.Base, 65, "%s/var/mail/dupemail", getenv("MBSE_ROOT"));
	    snprintf(msgs.QWKname, 21, "DUPEMAIL");
	    snprintf(msgs.Group, 13, "LOCAL");
	    msgs.Active = TRUE;
	    msgs.Type = LOCALMAIL;
	    msgs.MsgKinds = PUBLIC;
	    msgs.RDSec.level = 32000;
	    msgs.WRSec.level = 32000;
	    msgs.SYSec.level = 32000;
	    msgs.DaysOld = CFG.defdays;
	    msgs.MaxMsgs = CFG.defmsgs;
	    msgs.Charset = FTNC_NONE;
	    msgs.Created = time(NULL);
	    fwrite(&msgs, sizeof(msgs), 1, fil);
	    mkdirs(msgs.Base, 0770);
	    if (Msg_Open(msgs.Base))
		Msg_Close();
	    for (i = 1; i <= CFG.toss_systems; i++)
		fwrite(&syscon, sizeof(syscon), 1, fil);

	    fclose(fil);
	    exp_golded = TRUE;
	    chmod(ffile, 0660);
	    return 3;
	} else
	    return -1;
    }

    fread(&msgshdr, sizeof(msgshdr), 1, fil);
    fseek(fil, 0, SEEK_SET);
    fread(&msgshdr, msgshdr.hdrsize, 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize);
    fclose(fil);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenMsgarea(void);
int OpenMsgarea(void)
{
    FILE    *fin, *fout;
    char    fnin[PATH_MAX], fnout[PATH_MAX];
    int	    oldsize, oldsys;
    struct  _sysconnect syscon;
    int	    i, oldsystems;
    time_t  start = 900000000; /* Faked startdate of 09-07-1998 17:00:00 */

    /*
     * Find system startdate, we use this to update the area records
     * if they don't have a creation date. All new areas will get the
     * right date.
     */
    snprintf(fnin, PATH_MAX, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
    if ((fin = fopen(fnin, "r"))) {
	fread(&SYSINFO, sizeof(SYSINFO), 1, fin);
	start = SYSINFO.StartDate;
	fclose(fin);
    }

    snprintf(fnin,  PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    MsgUpdated = 0;
	    fread(&msgshdr, sizeof(msgshdr), 1, fin);
	    fseek(fin, 0, SEEK_SET);
	    fread(&msgshdr, msgshdr.hdrsize, 1, fin);
	    if (msgshdr.hdrsize != sizeof(msgshdr)) {
		msgshdr.hdrsize = sizeof(msgshdr);
		msgshdr.lastupd = time(NULL);
		MsgUpdated = 1;
		Syslog('+', "Updated %s, format changed", fnin);
	    }

	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize = msgshdr.recsize;
	    oldsys  = msgshdr.syssize;
	    oldsystems = oldsys / sizeof(syscon);
	    if (oldsize != sizeof(msgs)) {
		MsgUpdated = 1;
		Syslog('+', "Updated %s, recordsize is changed", fnin);
	    }
	    if (CFG.toss_systems != oldsystems) {
		MsgUpdated = 1;
		Syslog('+', "Updated %s, nr of systems is changed to %d", fnin, CFG.toss_systems);
	    }
	    msgshdr.hdrsize = sizeof(msgshdr);
	    msgshdr.recsize = sizeof(msgs);
	    msgshdr.syssize = sizeof(syscon) * CFG.toss_systems;
	    fwrite(&msgshdr, sizeof(msgshdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&msgs, 0, sizeof(msgs));
	    while (fread(&msgs, oldsize, 1, fin) == 1) {
		if ((oldsize != sizeof(msgs)) && !msgs.LinkSec.level) {
		    msgs.LinkSec.level = 1;
		    msgs.LinkSec.flags = 1;
		}
		if ((int)msgs.Created == 0) {
		    msgs.Created = start;
#ifndef	USE_NEWSGATE
		    if ((strlen(msgs.Newsgroup) == 0) && (msgs.Type == ECHOMAIL) && strlen(msgs.Group)) {
			snprintf(msgs.Newsgroup, 81, "%s.%s", GetFidoDomain(msgs.Aka.zone), msgs.Tag);
			for (i = 0; i < strlen(msgs.Newsgroup); i++) {
			    msgs.Newsgroup[i] = tolower(msgs.Newsgroup[i]);
			    if (msgs.Newsgroup[i] == '_')
				msgs.Newsgroup[i] = '.';
			}
			Syslog('+', "Area %s created newsgroup name %s", msgs.Tag, msgs.Newsgroup);
		    }
#endif
		}
		fwrite(&msgs, sizeof(msgs), 1, fout);
		memset(&msgs, 0, sizeof(msgs));
		/*
		 * Copy the existing connections
		 */
		for (i = 1; i <= oldsystems; i++) {
		    fread(&syscon, sizeof(syscon), 1, fin);
		    /* 
		     * Write only valid records
		     */
		    if (i <= CFG.toss_systems) 
			fwrite(&syscon, sizeof(syscon), 1, fout);
		}
		if (oldsystems < CFG.toss_systems) {
		    /*
		     * The size is increased, fill with blank records.
		     */
		    memset(&syscon, 0, sizeof(syscon));
		    for (i = (oldsystems + 1); i <= CFG.toss_systems; i++)
			fwrite(&syscon, sizeof(syscon), 1, fout);
		}
	    }

	    fclose(fin);
	    fclose(fout);
	    return 0;
	} else
	    return -1;
    }
    return -1;
}



void CloseMsgarea(int Force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];

	snprintf(fin,  PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));

	if (MsgUpdated == 1) {
		if (Force || (yes_no((char *)"Messages database is changed, save changes") == 1)) {
			working(1, 0, 0);
			exp_golded = TRUE;
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0660);
			Syslog('+', "Updated \"mareas.data\"");
			disk_reset();
			if (!Force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0660);
	working(1, 0, 0);
	unlink(fout); 
}



void InitMsgRec(void);
void InitMsgRec(void)
{
    memset(&msgs, 0, sizeof(msgs));
    /*
     * Fill in default values
     */
    msgs.DaysOld = CFG.defdays;
    msgs.MaxMsgs = CFG.defmsgs;
    msgs.Type = ECHOMAIL;
    msgs.MsgKinds = PUBLIC;
    msgs.UsrDelete = TRUE;
    msgs.Charset = FTNC_CP437;
    msgs.MaxArticles = CFG.maxarticles;
    strcpy(msgs.Origin, CFG.origin);
    msgs.LinkSec.level = 1;
    msgs.LinkSec.flags = 1;
    msgs.Created = time(NULL);
}



int AppendMsgarea(void);
int AppendMsgarea()
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	struct	_sysconnect syscon;
	int	i;

	snprintf(ffile, PATH_MAX, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		InitMsgRec();
		fwrite(&msgs, sizeof(msgs), 1, fil);
		memset(&syscon, 0, sizeof(syscon));
		for (i = 1; i <= CFG.toss_systems; i++)
			fwrite(&syscon, sizeof(syscon), 1, fil);
		fclose(fil);
		MsgUpdated = 1;
		return 0;
	} else
		return -1;
}



void EditSystem(sysconnect *);
void EditSystem(sysconnect *Sys)
{
	sysconnect	S;
	unsigned short	zone = 0;
	int		refresh = TRUE;

	S = (* Sys);
	for (;;) {
		if (refresh) {
			clr_index();
			set_color(WHITE, BLACK);
			mbse_mvprintw( 5,6, "9.2.26 EDIT CONNECTION");
			set_color(CYAN, BLACK);
			mbse_mvprintw( 7,6, "1.     Aka");
			mbse_mvprintw( 8,6, "2.     Send to");
			mbse_mvprintw( 9,6, "3.     Recv from");
			mbse_mvprintw(10,6, "4.     Pause");
			mbse_mvprintw(11,6, "5.     Excluded");
			mbse_mvprintw(12,6, "6.     Delete");
			refresh = FALSE;
		}
		set_color(WHITE, BLACK);
		show_str(  7,23,23, aka2str(S.aka));
		show_bool( 8,23, S.sendto);
		show_bool( 9,23, S.receivefrom);
		show_bool(10,23, S.pause);
		show_bool(11,23, S.cutoff);
		zone = S.aka.zone;

		switch(select_menu(6)) {
			case 0:	(* Sys) = S;
				return;
			case 1:	S.aka = PullUplink((char *)"9.2.26");
				refresh = TRUE;
				break;
			case 2: E_BOOL( 8,23, S.sendto,      "^Send^ mail ^to^ this node")
			case 3: E_BOOL( 9,23, S.receivefrom, "^Receive^ mail ^from^ this node")
			case 4: E_BOOL(10,23, S.pause,       "Is this node ^paused^")
			case 5: E_BOOL(11,23, S.cutoff,      "Is this node ^excluded (cutoff)^ by a moderator")
			case 6: if (yes_no((char *)"Delete this entry")) {
					memset(&S, 0, sizeof(sysconnect));
					(* Sys) = S;
					return;
				}
				break;
		}

		/*
		 * Set sendto and receivefrom to on when a new
		 * zone is entered.
		 */
		if ((S.aka.zone) && (!zone)) {
			S.sendto = 1;
			S.receivefrom = 1;
		}
	}
}



int EditConnections(FILE *);
int EditConnections(FILE *fil)
{
	int		systems, o = 0, i, y, x;
	int		offset;
	char		pick[12];
	sysconnect	System;
	char		status[5];
	char		temp[41];

	systems = msgshdr.syssize / sizeof(sysconnect);
	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 5, "9.2.26  MESSAGE AREA CONNECTIONS");
		set_color(CYAN, BLACK);
		y = 7;
		x = 2;
		for (i = 1; i <= 20; i++) {
			if ((o+i-1) < systems) {
				if (i == 11) {
					y = 7;
					x = 42;
				}
				offset = (o+i-1) * sizeof(sysconnect);
				if ((fseek(fil, offset, 0)) != 0) {
					working(2, 0, 0);
					return FALSE;
				}
				fread(&System, sizeof(sysconnect), 1, fil);
				memset(&status, 0, 5);
				memset(&status, '-', 4);
				if (System.sendto)
					status[0] = 'S';
				if (System.receivefrom)
					status[1] = 'R';
				if (System.pause)
					status[2] = 'P';
				if (System.cutoff)
					status[3] = 'C';

				if (System.aka.zone) {
					set_color(CYAN,BLACK);
					snprintf(temp, 41, "%3d. %s %s", o+i, status, aka2str(System.aka));
				} else {
					set_color(LIGHTBLUE, BLACK);
					snprintf(temp, 41, "%3d.", o+i);
				}
				mbse_mvprintw(y, x, temp);
				y++;
			}
		}
		strcpy(pick, select_pick(systems, 20));

		if (strncmp(pick, "-", 1) == 0) {
			return FALSE;
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < systems)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if (((atoi(pick) > 0) && (atoi(pick) <= systems))) {
			offset = (atoi(pick) -1) * sizeof(sysconnect);
			fseek(fil, offset, 0);
			fread(&System, sizeof(sysconnect), 1, fil);
			EditSystem(&System);
			fseek(fil, offset, 0);
			fwrite(&System, sizeof(sysconnect), 1, fil);
		}
	}
}



void SetScreen(void);
void SetScreen()
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 4, 2, "9.2 EDIT MESSAGE AREA");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 6, 2, "1.  Area Name");
    mbse_mvprintw( 7, 2, "2.  FTN area");
    mbse_mvprintw( 8, 2, "3.  Group");
    mbse_mvprintw( 9, 2, "4.  Newsgroup");
    mbse_mvprintw(10, 2, "5.  JAM base");
    mbse_mvprintw(11, 2, "6.  Origin");
    mbse_mvprintw(12, 2, "7.  Fido Aka");
    mbse_mvprintw(13, 2, "8.  QWK name");
    mbse_mvprintw(14, 2, "9.  Area Type");
    mbse_mvprintw(15, 2, "10. Msg Kinds");
    mbse_mvprintw(16, 2, "11. Charset");
    mbse_mvprintw(17, 2, "12. Active");
    mbse_mvprintw(18, 2, "13. Days Old");
    mbse_mvprintw(19, 2, "14. Max. Msgs");

    switch (msgs.Type) {
	case ECHOMAIL:  mbse_mvprintw(14,34, "15. Netreply");
			break;
	case NEWS:	mbse_mvprintw(14,34, "15. Articles");
			break;
	default:	mbse_mvprintw(14,34, "15. N/A");
			break;
    }
    mbse_mvprintw(15,34, "16. Read Sec.");
    mbse_mvprintw(16,34, "17. Write Sec.");
    mbse_mvprintw(17,34, "18. Sysop Sec.");
    mbse_mvprintw(18,34, "19. User Del.");
    mbse_mvprintw(19,34, "20. Aliases");

    mbse_mvprintw(13,58, "21. Quotes");
    mbse_mvprintw(14,58, "22. Mandatory");
    mbse_mvprintw(15,58, "23. UnSecure");
    mbse_mvprintw(16,58, "24. OLR Default");
    mbse_mvprintw(17,58, "25. OLR Forced");
    switch (msgs.Type) {
	case ECHOMAIL:
	case NEWS:
	case LIST:  mbse_mvprintw(18,58, "26. Connections");
		    break;
    }
    mbse_mvprintw(19,58, "27. Security");
} 



int LoadMsgRec(int, int);
int LoadMsgRec(int Area, int work)
{
    FILE	*fil;
    char	mfile[PATH_MAX];
    int		offset;
    sysconnect	System;
    int		i;

    snprintf(mfile, PATH_MAX, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    if ((tfil = tmpfile()) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    fread(&msgshdr, sizeof(msgshdr), 1, fil);
    offset = msgshdr.hdrsize + (((Area -1) * (msgshdr.recsize + msgshdr.syssize)));
    if (fseek(fil, offset, SEEK_SET) != 0) {
	fclose(tfil);
	tfil = NULL;
	working(2, 0, 0);
	return -1;
    }

    fread(&msgs, msgshdr.recsize, 1, fil);
    MsgCrc = 0xffffffff;
    MsgCrc = upd_crc32((char *)&msgs, MsgCrc, msgshdr.recsize);
    for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
	fread(&System, sizeof(sysconnect), 1, fil);
	fwrite(&System, sizeof(sysconnect), 1, tfil);
	MsgCrc = upd_crc32((char *)&System, MsgCrc, sizeof(sysconnect));
    }
    fclose(fil);

    return offset;
}



int SaveMsgRec(int, int);
int SaveMsgRec(int Area, int work)
{
	int		i;
	FILE		*fil;
	int		offset;
	char		mfile[PATH_MAX];
	sysconnect	System;

	if (work)
		working(1, 0, 0);
	snprintf(mfile, PATH_MAX, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r+")) == 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&msgshdr, sizeof(msgshdr), 1, fil);
	offset = msgshdr.hdrsize + (((Area -1) * (msgshdr.recsize + msgshdr.syssize)));
	if (fseek(fil, offset, SEEK_SET)) {
		fclose(fil);
		working(2, 0, 0);
		return -1;
	}

	fwrite(&msgs, msgshdr.recsize, 1, fil);
	fseek(tfil, 0, SEEK_SET);
	for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
		fread(&System, sizeof(sysconnect), 1, tfil);
		fwrite(&System, sizeof(sysconnect), 1, fil);
	}
	fclose(fil);
	fclose(tfil);
	tfil = NULL;
	return 0;
}



void DeleteRules(char *);
void DeleteRules(char *filename)
{
    DIR		    *dp;
    struct dirent   *de;
    char	    temp[PATH_MAX];

    if (strlen(filename) == 0)
	return;

    if ((dp = opendir(CFG.rulesdir)) == NULL)
	return;

    while ((de = readdir(dp))) {
	if (de->d_name[0] != '.') {
	    strcpy(temp, msgs.Tag);
	    if (strcasecmp(de->d_name, temp) == 0) {
		snprintf(temp, PATH_MAX, "%s/%s", CFG.rulesdir, de->d_name);
		Syslog('+', "unlink(%s) rc=%d", temp, unlink(temp));
		break;
	    }
	    snprintf(temp, PATH_MAX, "%s.rul", msgs.Tag);
	    if (strcasecmp(de->d_name, temp) == 0) {
		snprintf(temp, PATH_MAX, "%s/%s", CFG.rulesdir, de->d_name);
		Syslog('+', "unlink(%s) rc=%d", temp, unlink(temp));
		break;
	    }
	    memset(&temp, 0, sizeof(temp));
	    strncpy(temp, msgs.Tag, 8);
	    snprintf(temp, PATH_MAX, "%s.rul", temp);
	    if (strcasecmp(de->d_name, temp) == 0) {
		snprintf(temp, PATH_MAX, "%s/%s", CFG.rulesdir, de->d_name);
		Syslog('+', "unlink(%s) rc=%d", temp, unlink(temp));
		break;
	    }
	}
    }
    closedir(dp);
}



void ShowStatus(sysconnect);
void ShowStatus(sysconnect S)
{
	clr_index();
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 6, "Aka");
	mbse_mvprintw( 8, 6, "Send to");
	mbse_mvprintw( 9, 6, "Recv from");
	mbse_mvprintw(10, 6, "Pause");
	mbse_mvprintw(11, 6, "Excluded");
	set_color(WHITE, BLACK);
	show_str(  7,16,23, aka2str(S.aka));
	show_bool( 8,16, S.sendto);
	show_bool( 9,16, S.receivefrom);
	show_bool(10,16, S.pause);
	show_bool(11,16, S.cutoff);
}



void MsgGlobal(void);
void MsgGlobal(void)
{
    gr_list	*mgr = NULL, *tmp;
    char	*p, mfile[PATH_MAX];
    FILE	*fil;
    fidoaddr	a1, a2;
    int		menu = 0, marea, Areas, akan = 0, Found, charset = FTNC_CP437;
    int		Total, Done, netbrd, daysold, maxmsgs, maxarticles;
    int		offset;
    securityrec	rs, ws, ss, as;
    sysconnect	S, Sc;

    /*
     * Build the groups select array
     */
    working(1, 0, 0);
    snprintf(mfile, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) != NULL) {
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);

	while (fread(&mgroup, mgrouphdr.recsize, 1, fil) == 1)
	    fill_grlist(&mgr, mgroup.Name);
	
	fclose(fil);
	sort_grlist(&mgr);
    }

    /*
     * Initialize some variables
     */
    memset(&rs, 0, sizeof(securityrec));
    memset(&ws, 0, sizeof(securityrec));
    memset(&ss, 0, sizeof(securityrec));
    memset(&as, 0, sizeof(securityrec));
    as.level = 1;
    as.flags = 1;
    memset(&S, 0, sizeof(sysconnect));
    S.sendto = TRUE;
    S.receivefrom = TRUE;
    memset(&mfile, 0, sizeof(mfile));
    snprintf(mfile, 81, "%s", CFG.origin);
    daysold = CFG.defdays;
    maxmsgs = CFG.defmsgs;
    maxarticles = CFG.maxarticles;
    netbrd = 1;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "9.2 GLOBAL EDIT MESSAGE AREAS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 6, "1.  Delete connection");
	mbse_mvprintw( 8, 6, "2.  Add new connection");
	mbse_mvprintw( 9, 6, "3.  Replace connection");
	mbse_mvprintw(10, 6, "4.  Change connection status");
	mbse_mvprintw(11, 6, "5.  Change days old");
	mbse_mvprintw(12, 6, "6.  Change max. messages");
	mbse_mvprintw(13, 6, "7.  Change max. articles");
	mbse_mvprintw(14, 6, "8.  Change bbs security");
	mbse_mvprintw(15, 6, "9.  Change link security");
	mbse_mvprintw(16, 6, "10. Change aka to use");
	mbse_mvprintw( 7,41, "11. Change origin line");
	mbse_mvprintw( 8,41, "12. Change netmail reply");
	mbse_mvprintw( 9,41, "13. Change character set");
	mbse_mvprintw(10,41, "14. Delete message area");

	memset(&a1, 0, sizeof(fidoaddr));
	memset(&a2, 0, sizeof(fidoaddr));

	menu = select_menu(14);
	switch (menu) {
	    case 0: tidy_grlist(&mgr);
		    return;
	    case 1: a1 = PullUplink((char *)"AKA TO DELETE");
		    break;
	    case 2: a2 = PullUplink((char *)"AKA TO ADD");
		    break;
	    case 3: a1 = PullUplink((char *)"AKA TO REPLACE");
		    a2 = PullUplink((char *)"NEW AKA");
		    break;
	    case 4: S.aka = PullUplink((char *)"AKA TO CHANGE STATUS");
		    ShowStatus(S);
		    S.sendto = edit_bool(8,16,S.sendto, (char *)"^Send^ mail to this node");
		    S.receivefrom = edit_bool(9,16,S.receivefrom, (char *)"^Receive^ mail from this node");	
		    S.pause = edit_bool(10,16,S.pause, (char *)"Is this node ^paused^");
		    S.cutoff = edit_bool(11,16,S.cutoff, (char *)"Is this node ^excluded^");
		    break;
	    case 5: mbse_mvprintw(LINES -3, 5, "Days old");
		    E_INT(LINES -3, 14, daysold, (char *)"Enter new number of ^days old^")
	    case 6: mbse_mvprintw(LINES -3, 5, "Max. messages");
		    E_INT(LINES -3, 19, maxmsgs, (char *)"Enter ^maximum messages^")
	    case 7: mbse_mvprintw(LINES -3, 6, "Max. articles");
		    E_INT(LINES -3, 19, maxarticles, (char *)"Enter ^maximum news articles^ to fetch")
	    case 8: rs = edit_sec(6, 5, rs, (char *)"9.2.8 READ SECURITY");
		    ws = edit_sec(7, 5, ws, (char *)"9.2.8 WRITE SECURITY");
		    ss = edit_sec(8, 5, ss, (char *)"9.2.8 SYSOP SECURITY");
		    break;
	    case 9: as = edit_asec(as, (char *)"9.2.9 LINK SECURITY");
		    break;
	    case 10:akan = PickAka((char *)"9.2.8", TRUE);
		    break;
	    case 11:E_STR(LINES -3, 5, 64, mfile, "Enter new ^origin^ line");
	    case 12:mbse_mvprintw(LINES -3, 5, "Netmail reply board");
		    E_INT(LINES -3, 25, netbrd, (char *)"The ^netmail reply^ board number")
	    case 13:mbse_mvprintw(LINES -4, 6, "Character set to set");
		    charset = edit_charset(LINES -4,27, charset);
		    break;
	}

	E_Group(&mgr, (char *)"SELECT MESSAGE GROUPS TO CHANGE");

	/*
	 * Show settings before proceeding
	 */
	switch (menu) {
	    case 1: mbse_mvprintw(7, 6, "Delete aka %s", aka2str(a1));
		    break;
	    case 2: mbse_mvprintw(7, 6, "Add aka %s", aka2str(a2));
		    break;
	    case 3: p = xstrcpy(aka2str(a1));
		    mbse_mvprintw(7, 6, "Replace aka %s with %s", p, aka2str(a2));
		    free(p);
		    break;
	    case 4: ShowStatus(S);
		    mbse_mvprintw(14, 6, "Change the link status");
		    break;
	    case 5: mbse_mvprintw(7, 6, "Change days old to %d", daysold);
		    break;
	    case 6: mbse_mvprintw(7, 6, "Change maximum messages to %d", maxmsgs);
		    break;
	    case 7: mbse_mvprintw(7, 6, "Change maximum news articles to fetch to %d", maxarticles);
		    break;
	    case 8: set_color(CYAN, BLACK);
		    mbse_mvprintw(7, 6, "Read security");
		    mbse_mvprintw(8, 6, "Write security");
		    mbse_mvprintw(9, 6, "Sysop security");
		    set_color(WHITE, BLACK);
		    show_sec(7, 21, rs);
		    show_sec(8, 21, ws);
		    show_sec(9, 21, ss);
		    break;
	    case 9: set_color(CYAN, BLACK);
		    mbse_mvprintw(7, 6, "Link security");
		    set_color(WHITE, BLACK);
		    mbse_mvprintw(7,21, getflag(as.flags, as.notflags));
		    break;
	    case 10:if (akan != -1)
			mbse_mvprintw( 7, 6, "Set %s as new aka to use", aka2str(CFG.aka[akan]));
		    break;
	    case 11:mbse_mvprintw(7, 6, "Origin: %s", mfile);
		    break;
	    case 12:mbse_mvprintw(7, 6, "New netmail reply board %d", netbrd);
		    break;
	    case 13:mbse_mvprintw(7, 6, "New character set %s", getftnchrs(charset));
		    break;
	    case 14:mbse_mvprintw(7, 6, "Delete message areas");
		    break;
	}

	if (yes_no((char *)"Perform changes")) {
	    working(1, 0, 0);
	    Areas = CountMsgarea();
	    Total = Done = 0;

	    for (marea = 1; marea <= Areas; marea++) {
		offset = LoadMsgRec(marea, FALSE);
		if (msgs.Active && strlen(msgs.Group)) {
		    for (tmp = mgr; tmp; tmp = tmp->next) {
			if (tmp->tagged && (strcmp(tmp->group, msgs.Group) == 0)) {
			    Total++;
			    switch (menu) {
				case 1:	fseek(tfil, 0, SEEK_SET);
					while (fread(&Sc, sizeof(sysconnect), 1, tfil) == 1) {
					    if ((Sc.aka.zone == a1.zone) && (Sc.aka.net == a1.net) &&
						(Sc.aka.node == a1.node) && (Sc.aka.point == a1.point)) {
						fseek(tfil, - sizeof(sysconnect), SEEK_CUR);
						memset(&Sc, 0, sizeof(sysconnect));
						fwrite(&Sc, sizeof(sysconnect), 1, tfil);
						if (SaveMsgRec(marea, FALSE) == 0) {
						    Done++;
						    Syslog('+', "Deleted %s from %s", aka2str(a1), msgs.Tag);
						}
						break;
					    }
					}
					break;
				case 2:	fseek(tfil, 0, SEEK_SET);
					Found = FALSE;
					while (fread(&Sc, sizeof(sysconnect), 1, tfil) == 1)
					    if ((Sc.aka.zone == a2.zone) && (Sc.aka.net == a2.net) &&
						(Sc.aka.node == a2.node) && (Sc.aka.point == a2.point)) {
						Found = TRUE;
						break;
					    }
					    if (Found)
						break;
					    fseek(tfil, 0, SEEK_SET);
					    while (fread(&Sc, sizeof(sysconnect), 1, tfil) == 1) {
						if (Sc.aka.zone == 0) {
						    fseek(tfil, - sizeof(sysconnect), SEEK_CUR);
						    memset(&Sc, 0, sizeof(sysconnect));
						    Sc.aka.zone = a2.zone;
						    Sc.aka.net = a2.net;
						    Sc.aka.node = a2.node;
						    Sc.aka.point = a2.point;
						    Sc.sendto = TRUE;
						    Sc.receivefrom = TRUE;
						    snprintf(Sc.aka.domain, 13, "%s", a2.domain);
						    fwrite(&Sc, sizeof(sysconnect), 1, tfil);
						    if (SaveMsgRec(marea, FALSE) == 0) {
							Done++;
							Syslog('+', "Added %s to area %s", aka2str(a2), msgs.Tag);
						    }
						    break;
						}
					    }
					break;
				case 3: fseek(tfil, 0, SEEK_SET);
					while (fread(&Sc, sizeof(sysconnect), 1, tfil) == 1) {
					    if ((Sc.aka.zone == a1.zone) && (Sc.aka.net == a1.net) &&
						(Sc.aka.node == a1.node) && (Sc.aka.point == a1.point)) {
						Sc.aka.zone = a2.zone;
						Sc.aka.net = a2.net;
						Sc.aka.node = a2.node;
						Sc.aka.point = a2.point;
						snprintf(Sc.aka.domain, 13, "%s", a2.domain);
						fseek(tfil, - sizeof(sysconnect), SEEK_CUR);
						fwrite(&Sc, sizeof(sysconnect), 1, tfil);
						if (SaveMsgRec(marea, FALSE) == 0) {
						    Done++;
						    p = xstrcpy(aka2str(a1));
						    Syslog('+', "Changed %s into %s in area %s", p, aka2str(a2), msgs.Tag);
						    free(p);
						}
						break;
					    }
					}
					break;
				case 4: fseek(tfil, 0, SEEK_SET);
					while (fread(&Sc, sizeof(sysconnect), 1, tfil) == 1) {
					    if ((Sc.aka.zone == S.aka.zone) && (Sc.aka.net == S.aka.net) &&
						(Sc.aka.node == S.aka.node) && (Sc.aka.point == S.aka.point)) {
						Sc.sendto = S.sendto;
						Sc.receivefrom = S.receivefrom;
						Sc.pause = S.pause;
						Sc.cutoff = S.cutoff;
						fseek(tfil, - sizeof(sysconnect), SEEK_CUR);
						fwrite(&Sc, sizeof(sysconnect), 1, tfil);
						if (SaveMsgRec(marea, FALSE) == 0) {
						    Done++;
						    Syslog('+', "Changed status of %s in area %s", aka2str(S.aka), msgs.Tag);
						}
						break;
					    }
					}
					break;
				case 5: if (daysold != msgs.DaysOld) {
					    msgs.DaysOld = daysold;
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Changed days old to %d in area %s", daysold, msgs.Tag);
					    }
					}
					break;
				case 6: if (maxmsgs != msgs.MaxMsgs) {
					    msgs.MaxMsgs = maxmsgs;
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Changed max. messages to %d in area %s", maxmsgs, msgs.Tag);
					    }
					}
					break;
				case 7: if (maxarticles != msgs.MaxArticles) {
					    msgs.MaxArticles = maxarticles;
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Changed max news articles to fetch to %d in area %s", 
							    maxarticles, msgs.Tag);
					    }
					}
					break;
				case 8: if ((msgs.RDSec.level != rs.level) ||
					    (msgs.RDSec.flags != rs.flags) ||
					    (msgs.RDSec.notflags != rs.notflags) ||
					    (msgs.WRSec.level != ws.level) ||
					    (msgs.WRSec.flags != ws.flags) ||
					    (msgs.WRSec.notflags != ws.notflags) ||
					    (msgs.SYSec.level != ss.level) ||
					    (msgs.SYSec.flags != ss.flags) ||
					    (msgs.SYSec.notflags != ss.notflags)) {
					    memcpy(&msgs.RDSec, &rs, sizeof(securityrec));
					    memcpy(&msgs.WRSec, &ws, sizeof(securityrec));
					    memcpy(&msgs.SYSec, &ss, sizeof(securityrec));
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Updated bbs security levels in area %s", msgs.Tag);
					    }
					}
					break;
				case 9: if ((msgs.LinkSec.flags != as.flags) || (msgs.LinkSec.notflags != as.notflags)) {
					    memcpy(&msgs.LinkSec, &as, sizeof(securityrec));
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Updated link security flags in area %s", msgs.Tag);
					    }
					}
					break;
				case 10:if (akan != -1) {
					    if ((msgs.Aka.zone != CFG.aka[akan].zone) || (msgs.Aka.net != CFG.aka[akan].net) ||
						(msgs.Aka.node != CFG.aka[akan].node) || (msgs.Aka.point != CFG.aka[akan].point)) {
						msgs.Aka.zone = CFG.aka[akan].zone;
						msgs.Aka.net = CFG.aka[akan].net;
						msgs.Aka.node = CFG.aka[akan].node;
						msgs.Aka.point = CFG.aka[akan].point;
						snprintf(msgs.Aka.domain, 13, "%s", CFG.aka[akan].domain);
						if (SaveMsgRec(marea, FALSE) == 0) {
						    Done++;
						    Syslog('+', "Area %s now uses aka %s", msgs.Tag, aka2str(msgs.Aka));
						}
					    }
					}
					break;
				case 11:if (strcmp(msgs.Origin, mfile)) {
					    snprintf(msgs.Origin, 65, "%s", mfile);
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Changed origin line in area %s", msgs.Tag);
					    }
					}
					break;
				case 12:if (netbrd != msgs.NetReply) {
					    msgs.NetReply = netbrd;
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Changed netmail board to %d in area %s", netbrd, msgs.Tag);
					    }
					}
					break;
				case 13:if (charset != msgs.Charset) {
					    msgs.Charset = charset;
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Changed character set to %s in area %s", 
							getftnchrs(charset), msgs.Tag);
					    }
					}
					break;
				case 14:if (msgs.Active) {
					    msgs.Active = FALSE;
					    Msg_DeleteMsgBase(msgs.Base);
					    DeleteRules(msgs.Tag);
					    memset(&msgs, 0, sizeof(msgs));
					    if (SaveMsgRec(marea, FALSE) == 0) {
						Done++;
						Syslog('+', "Deleted message area %s", msgs.Tag);
					    }
					}
					break;
			    }
			}
		    }
		}
		if (tfil != NULL)
		    fclose(tfil);
		tfil = NULL;
	    }

	    mbse_mvprintw(LINES -3, 6,"Made %d changes in %d possible areas", Done, Total);
	    (void)readkey(LINES -3, 50, LIGHTGRAY, BLACK);
	    if (Done)
		MsgUpdated = TRUE;
	}
    }

    tidy_grlist(&mgr);
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditMsgRec(int);
int EditMsgRec(int Area)
{
    unsigned int    crc1;
    int		    tmp, i, connections = 0, changed = FALSE, Active, Forced = FALSE;
    sysconnect	    System;
    char	    *temp, oldpath[81];
    FILE	    *fil;

    clr_index();
    IsDoing("Edit Msg Area");

    if (LoadMsgRec(Area, TRUE) == -1)
	return -1;

    SetScreen();

    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 6,16,40, msgs.Name);
	show_str( 7,16,50, msgs.Tag);
	show_str( 8,16,12, msgs.Group);
	show_str( 9,16,64, msgs.Newsgroup);
	show_str(10,16,64, msgs.Base);
	show_str(11,16,64, msgs.Origin);
	show_aka(12,16,    msgs.Aka);
	show_str(13,16,13, msgs.QWKname);
	show_msgtype(14,16, msgs.Type);
	show_msgkinds(15,16, msgs.MsgKinds);
	show_charset(16,16, msgs.Charset);
	show_bool(17,16,   msgs.Active);
	show_int( 18,16, msgs.DaysOld);
	show_int( 19,16, msgs.MaxMsgs);

	switch (msgs.Type) {
	    case ECHOMAIL:  show_int( 14,50, msgs.NetReply);
			    break;
	    case NEWS:	    show_int( 14,50, msgs.MaxArticles);
			    break;
	}
	show_int( 15,50, msgs.RDSec.level);
	show_int( 16,50, msgs.WRSec.level);
	show_int( 17,50, msgs.SYSec.level);
	show_bool(18,50, msgs.UsrDelete);
	show_bool(19,50, msgs.Aliases);

	show_bool(13,74, msgs.Quotes);
	show_bool(14,74, msgs.Mandatory);
	show_bool(15,74, msgs.UnSecure);
	show_bool(16,74, msgs.OLR_Default);
	show_bool(17,74, msgs.OLR_Forced);
	connections = 0;
	switch (msgs.Type) {
	    case ECHOMAIL:
	    case NEWS:
	    case LIST:  fseek(tfil, 0, SEEK_SET);
			while (fread(&System, sizeof(System), 1, tfil) == 1)
			    if (System.aka.zone)
				connections++;
			show_int(18,74, connections);
			break;
	}

	switch (select_menu(27)) {
	    case 0: crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&msgs, crc1, msgshdr.recsize);
		    fseek(tfil, 0, 0);
		    for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
			fread(&System, sizeof(sysconnect), 1, tfil);
			crc1 = upd_crc32((char *)&System, crc1, sizeof(sysconnect));
		    }
		    if (msgs.Active)
			test_jam(msgs.Base);
		    if ((MsgCrc != crc1) || (changed)) {
			if (msgs.Active && !strlen(msgs.Base)) {
			    errmsg((char *)"JAM message base is not set");
			    break;
			} else if (msgs.Active && !strlen(msgs.Group) && 
				(msgs.Type == ECHOMAIL || msgs.Type == NEWS || msgs.Type == LIST)) {
			    errmsg((char *)"Message area has no group assigned");
			    break;
			} else if (Forced || yes_no((char *)"Record is changed, save") == 1) {
			    if (SaveMsgRec(Area, TRUE) == -1)
				return -1;
			    MsgUpdated = 1;
			    Syslog('+', "Saved message area record %d", Area);
			    working(6, 0, 0);
			}
		    }
		    if (tfil != NULL)
			fclose(tfil);
		    tfil = NULL;
		    IsDoing("Browsing Menu");
		    return 0;
	    case 1: E_STR(  6,16,40,msgs.Name,       "The ^Name^ of this area")
	    case 2: strcpy(msgs.Tag, edit_ups(7,16,50, msgs.Tag, (char *)"The ^Area Tag^ for Echomail"));
		    if (!strlen(msgs.QWKname)) {
			memset(&msgs.QWKname, '\0', strlen(msgs.QWKname));
			strncpy(msgs.QWKname, msgs.Tag, 13);
		    }
		    break;
	    case 3: tmp = strlen(msgs.Group);
		    strcpy(msgs.Group, PickMGroup((char *)"9.2.3"));
		    if (strlen(msgs.Group) && !tmp) {
			/*
			 * If set for the first time, set some defaults
			 */
			msgs.Aka = mgroup.UseAka;
			msgs.Active = TRUE;
			msgs.MsgKinds = PUBLIC;
			msgs.Type = ECHOMAIL;
			msgs.DaysOld = CFG.defdays;
			msgs.MaxMsgs = CFG.defmsgs;
			msgs.UsrDelete = mgroup.UsrDelete;
			msgs.RDSec = mgroup.RDSec;
			msgs.WRSec = mgroup.WRSec;
			msgs.SYSec = mgroup.SYSec;
			msgs.Aliases = mgroup.Aliases;
			msgs.NetReply = mgroup.NetReply;
			msgs.Quotes = mgroup.Quotes;
			msgs.MaxArticles = CFG.maxarticles;
			msgs.Charset = mgroup.Charset;
			strncpy(msgs.Origin, CFG.origin, 50);
			msgs.LinkSec = mgroup.LinkSec;

			/*
			 * If there is an uplink defined in the group,
			 * and the first connected system is empty,
			 * copy the uplink as default connection.
			 */
			if (mgroup.UpLink.zone) {
			    fseek(tfil, 0, SEEK_SET);
			    fread(&System, sizeof(sysconnect), 1, tfil);
			    if (!System.aka.zone) {
				memset(&System, 0, sizeof(sysconnect));
				System.aka = mgroup.UpLink;
				System.sendto = TRUE;
				System.receivefrom = TRUE;
				fseek(tfil, 0, SEEK_SET);
				fwrite(&System, sizeof(sysconnect), 1, tfil);
			    }
			}
			if (!strlen(msgs.Base) && strlen(msgs.Tag)) {
			    /*
			     * Invent the place for the JAM message base.
			     */
			    temp = calloc(PATH_MAX, sizeof(char));
			    if (strlen(mgroup.BasePath)) {
				snprintf(temp, 81, "%s", msgs.Tag);
				for (i = 0; i < strlen(temp); i++) {
				    if (isupper(temp[i]))
					temp[i] = tolower(temp[i]);
				    /*
				     * If dots in the group or area tag, replace them
				     * with directory slashes to create a group tree.
				     */
				    if (temp[i] == '.')
					temp[i] = '/';
				}
				snprintf(msgs.Base, 65, "%s/%s", mgroup.BasePath, temp);
			    } else {
				snprintf(temp, 81, "%s/%s", msgs.Group, msgs.Tag);
				for (i = 0; i < strlen(temp); i++) {
				    if (isupper(temp[i]))
					temp[i] = tolower(temp[i]);
				    /*
				     * If dots in the group or area tag, replace them
				     * with directory slashes to create a group tree.
				     */
				    if (temp[i] == '.')
					temp[i] = '/';
				}
				snprintf(msgs.Base, 65, "%s/var/mail/%s", getenv("MBSE_ROOT"), temp);
			    }
			    free(temp);
			    /*
			     * Now fix any spaces in the path.
			     */
			    for (i = 0; i < strlen(msgs.Base); i++)
				if (msgs.Base[i] == ' ')
				    msgs.Base[i] = '_';
			}
		    }
		    SetScreen(); 
		    break;
	    case 4: E_STR(  9,16,64,msgs.Newsgroup,  "The ^Newsgroup^ name of this area")
	    case 5: snprintf(oldpath, 81, "%s", msgs.Base);
		    strcpy(msgs.Base, edit_jam(10,16,64,msgs.Base ,(char *)"The path to the ^JAM Message Base^"));
		    if (strcmp(oldpath, msgs.Base)) {
			i = 0;
			temp = calloc(PATH_MAX, sizeof(char));
			snprintf(temp, PATH_MAX, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r+")) != NULL) {
			    fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fil);
			    while (fread(&scanmgr, scanmgrhdr.recsize, 1, fil) == 1) {
				if (strcmp(oldpath, scanmgr.ScanBoard) == 0) {
				    i++;
				    snprintf(scanmgr.ScanBoard, 51, "%s", msgs.Base);
				    fseek(fil, - scanmgrhdr.recsize, SEEK_CUR);
				    fwrite(&scanmgr, scanmgrhdr.recsize, 1, fil);
				}
				if (strcmp(oldpath, scanmgr.ReplBoard) == 0) {
				    i++;
				    snprintf(scanmgr.ReplBoard, 51, "%s", msgs.Base);
				    fseek(fil, - scanmgrhdr.recsize, SEEK_CUR);
				    fwrite(&scanmgr, scanmgrhdr.recsize, 1, fil);
				}
			    }
			    fclose(fil);
			    if (i)
				Syslog('+', "Updated %d paths in %s", i, temp);
			}

			i = 0;
			snprintf(temp, PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r+")) != NULL) {
			    fread(&newfileshdr, sizeof(newfileshdr), 1, fil);
			    while (fread(&newfiles, newfileshdr.recsize, 1, fil) == 1) {
				if (strcmp(oldpath, newfiles.Area) == 0) {
				    i++;
				    snprintf(newfiles.Area, 51, "%s", msgs.Base);
				    fseek(fil, - newfileshdr.recsize, SEEK_CUR);
				    fwrite(&newfiles, newfileshdr.recsize, 1, fil);
				}
				fseek(fil, newfileshdr.grpsize, SEEK_CUR);
			    }
			    fclose(fil);
			    if (i)
				Syslog('+', "Updated %d paths in %s", i, temp);
			}
			free(temp);
			Forced = TRUE;
			MailForced = TRUE;
		    }
		    break;
	    case 6: E_STR( 11,16,64,msgs.Origin,     "The ^Origin line^ to append to Echomail messages")
	    case 7: tmp = PickAka((char *)"9.2.7", TRUE);
		    if (tmp != -1)
			msgs.Aka = CFG.aka[tmp];
		    SetScreen(); 
		    break;
	    case 8: E_UPS( 13,16,13,msgs.QWKname,      "The name for ^QWK or Bluewave^ message packets")
	    case 9: msgs.Type = edit_msgtype(14,16, msgs.Type); 
		    SetScreen();
		    break;
	    case 10:msgs.MsgKinds = edit_msgkinds(15,16, msgs.MsgKinds); break;
	    case 11:msgs.Charset = edit_charset(16,16, msgs.Charset); break;
	    case 12:Active = edit_bool(17,16, msgs.Active, (char *)"Is this area ^Active^");
		    if (msgs.Active && !Active) {
			/*
			 * Attempt to deactivate area, do some checks.
			 */
			if (connections) {
			    if (yes_no((char *)"There are nodes connected, disconnect them") == 0)
				Active = TRUE;
			}
			if (!Active) {
			    temp = calloc(PATH_MAX, sizeof(char));
			    snprintf(temp, PATH_MAX, "%s.jhr", msgs.Base);
			    if (strlen(msgs.Base) && (file_size(temp) != 1024)) {
				if (yes_no((char *)"There are messages in this area, delete them") == 0)
				    Active = TRUE;
			    }
			    free(temp);
			}
			if (!Active) {
			    /*
			     * Make it so
			     */
			    Msg_DeleteMsgBase(msgs.Base);
			    DeleteRules(msgs.Tag);
			    memset(&System, 0, sizeof(System));
			    fseek(tfil, 0, SEEK_SET);
			    for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++)
				fwrite(&System, sizeof(System), 1, tfil);
			    InitMsgRec();
			    Syslog('+', "Deleted message area %d", Area);
			    changed = TRUE;
			}
		    }
		    if (!msgs.Active && Active)
			msgs.Active = TRUE;
		    SetScreen();
		    break;
	    case 13:E_INT( 18,16,   msgs.DaysOld,       "Maximum ^days^ to keep mail in this area")
	    case 14:E_INT( 19,16,   msgs.MaxMsgs,       "The ^maximum^ amount of messages in this area")
	    case 15:switch (msgs.Type) {
			case ECHOMAIL:	msgs.NetReply = edit_int(14,50,msgs.NetReply,
						    (char *)"The ^Area Number^ for netmail replies");
					break;
			case NEWS:      msgs.MaxArticles = edit_int(14,50,msgs.MaxArticles,
						    (char *)"The ^maximum news articles^ to fetch");
					break;
		    }
		    break;
	    case 16:E_SEC( 15,50,   msgs.RDSec,         "9.2 EDIT READ SECURITY", SetScreen)
	    case 17:E_SEC( 16,50,   msgs.WRSec,         "9.2 EDIT WRITE SECURITY", SetScreen)
	    case 18:E_SEC( 17,50,   msgs.SYSec,         "9.2 EDIT SYSOP SECURITY", SetScreen)
	    case 19:E_BOOL(18,50,   msgs.UsrDelete,     "Allow users to ^Delete^ their messages")
	    case 20:E_BOOL(19,50,   msgs.Aliases,       "Allow ^aliases^ or real names only")

	    case 21:E_BOOL(13,74,   msgs.Quotes,        "Add random ^quotes^ to new messages")
	    case 22:E_BOOL(14,74,   msgs.Mandatory,     "Is this area ^mandatory^ for nodes")
	    case 23:E_BOOL(15,74,   msgs.UnSecure,      "Toss messages ^UnSecure^, ie: no originating check")
	    case 24:E_BOOL(16,74,   msgs.OLR_Default,   "Area is ^default^ for ^offline^ users.")
	    case 25:E_BOOL(17,74,   msgs.OLR_Forced,    "Area is ^always on^ for ^offline^ users.")
	    case 26:switch (msgs.Type) {
			case ECHOMAIL:
			case NEWS:
			case LIST:  if (EditConnections(tfil))
					changed = TRUE;
				    SetScreen();
				    break;
		    }
		    break;
	    case 27:msgs.LinkSec = edit_asec(msgs.LinkSec, (char *)"9.2 EDIT LINK SECURITY");
		    SetScreen();
		    break;
	}
    }
}



void EditMsgarea(void)
{
    int		records, rc, i, o, y, from, too;
    char	pick[12], temp[PATH_MAX];
    FILE	*fil;
    int		offset;
    sysconnect	System;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    records = CountMsgarea();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenMsgarea() == -1) {
	working(2, 0, 0);
	return;
    }
    o = 0;
    if (! check_free())
	return;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 3, "9.2 MESSAGE AREA SETUP");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&msgshdr, sizeof(msgshdr), 1, fil);
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 10; i++) {
		    if ((o + i) <= records) {
			offset = sizeof(msgshdr) + (((o + i) - 1) * (msgshdr.recsize + msgshdr.syssize));
			fseek(fil, offset, 0);
			fread(&msgs, msgshdr.recsize, 1, fil);
			if (msgs.Active) {
			    set_color(CYAN, BLACK);
			    snprintf(temp, 81, "%3d. %-8s %-23s %-40s", o + i, getmsgtype(msgs.Type), msgs.Tag, msgs.Name);
			} else {
			    set_color(LIGHTBLUE, BLACK);
			    snprintf(temp, 81, "%3d.", o+i);
			}
			mbse_mvprintw(y, 2, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_area(records, 10));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseMsgarea(MailForced);
	    open_bbs();
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    working(1, 0, 0);
	    if (AppendMsgarea() == 0) {
		records++;
		working(1, 0, 0);
	    } else
		working(2, 0, 0);
	}

	if (strncmp(pick, "G", 1) == 0) {
	    MsgGlobal();
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 10) < records) 
		o = o + 10;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 10) >= 0)
		o = o - 10;

	if (strncmp(pick, "M", 1) == 0) {
	    from = too = 0;
	    mbse_mvprintw(LINES -3, 5, "From");
	    from = edit_int(LINES -3, 10, from, (char *)"Wich ^area^ you want to move");
	    mbse_mvprintw(LINES -3,15, "To");
	    too  = edit_int(LINES -3, 18, too,  (char *)"Too which ^area^ to move");
	    rc = 0;

	    /*
	     * Check originating area
	     */
	    if (from == 0) {
		errmsg((char *)"The originating area cannot be zero");
		rc = 1;
	    } 
	    if (!rc && LoadMsgRec(from, TRUE) == -1) {
		errmsg((char *)"The originating area does not exist");
		rc = 1;
	    }
	    if (!rc)
		fclose(tfil);
	    if (!rc && !msgs.Active) {
		errmsg((char *)"The originating area is not in use");
		rc = 1;
	    }

	    if (!rc && (too == 0)) {
		errmsg((char *)"The destination area cannot be zero");
		rc = 1;
	    }
	    if (!rc && LoadMsgRec(too, TRUE) == -1) {
		errmsg((char *)"The destination area does not exist");
		rc = 1;
	    }
	    if (!rc)
		fclose(tfil);
	    if (!rc && (msgs.Active)) {
		errmsg((char *)"The destination area is in use");
		rc = 1;
	    }

	    if (!rc) {
		LoadMsgRec(from, TRUE);
		SaveMsgRec(too, TRUE);
		LoadMsgRec(from, TRUE);
		InitMsgRec();
		fseek(tfil, 0, SEEK_SET);
		memset(&System, 0, sizeof(sysconnect));
		for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
		    fwrite(&System, sizeof(sysconnect), 1, tfil);
		}
		SaveMsgRec(from, TRUE);
		MsgUpdated = 1;
		Syslog('+', "Moved message area %d to %d", from, too);
	    }
	}

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    EditMsgRec(atoi(pick));
	    o = ((atoi(pick) - 1) / 10) * 10;
	}
    }
}



void InitMsgarea(void)
{
    CountMsgarea();
    OpenMsgarea();
    CloseMsgarea(TRUE);
}



char *PickMsgarea(char *shdr)
{
	int		records, i, o = 0, x, y;
	char		pick[12];
	FILE 		*fil;
	char		temp[PATH_MAX];
	int		offset;
	static char	Buf[81];

	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return '\0';
	}

	records = CountMsgarea();
	if (records == -1) {
		working(2, 0, 0);
		return '\0';
	}


	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.  MESSAGE AREA SELECT", shdr);
		mbse_mvprintw(5, 3, temp);
		set_color(CYAN, BLACK);

		if (records) {
			snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&msgshdr, sizeof(msgshdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(msgshdr) + (((o + i) - 1) * (msgshdr.recsize + msgshdr.syssize));
						fseek(fil, offset, SEEK_SET);
						fread(&msgs, msgshdr.recsize, 1, fil);
						if (msgs.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						snprintf(temp, 81, "%3d.  %-31s", o + i, msgs.Name);
						temp[38] = '\0';
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_pick(records, 20));

		if (strncmp(pick, "-", 1) == 0)
			return '\0';

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o += 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o -= 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				offset = msgshdr.hdrsize + ((atoi(pick) - 1) * (msgshdr.recsize + msgshdr.syssize));
				fseek(fil, offset, SEEK_SET);
				fread(&msgs, msgshdr.recsize, 1, fil);
				fclose(fil);
				if (msgs.Active) {
					memset(&Buf, 0, sizeof(Buf));
					snprintf(Buf, 81, "%s", msgs.Base);
					return Buf;
				}
			}
		}
	}
}



int GroupInMarea(char *Group)
{
	int	Area = 0, RetVal = 0, systems;
	FILE	*no;
	char	temp[PATH_MAX];

	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return 0;

	fread(&msgshdr, sizeof(msgshdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&msgshdr, msgshdr.hdrsize, 1, no);
	systems = msgshdr.syssize / sizeof(sysconnect);

	while (fread(&msgs, msgshdr.recsize, 1, no) == 1) {
		Area++;

		if (msgs.Active && !strcmp(msgs.Group, Group) && strlen(msgs.Group)) {
			RetVal++;
			Syslog('-', "Group %s in msg area %d: %s", Group, Area, msgs.Tag);
		}

		fseek(no, msgshdr.syssize, SEEK_CUR);
	}

	fclose(no);
	return RetVal;
}



int NodeInMarea(fidoaddr A)
{
	int		i, Area = 0, RetVal = 0, systems;
	FILE		*no;
	char		temp[PATH_MAX];
	sysconnect	S;

	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return 0;

	fread(&msgshdr, sizeof(msgshdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&msgshdr, msgshdr.hdrsize, 1, no);
	systems = msgshdr.syssize / sizeof(sysconnect);

	while (fread(&msgs, msgshdr.recsize, 1, no) == 1) {
		Area++;
		for (i = 0; i < systems; i++) {
			fread(&S, sizeof(sysconnect), 1, no);
			if (S.aka.zone && (S.aka.zone == A.zone) && (S.aka.net == A.net) &&
			    (S.aka.node == A.node) && (S.aka.point == A.point) && msgs.Active) {
				RetVal++;
				Syslog('-', "Node %s connected to msg area %d: %s", aka2str(A), Area, msgs.Tag);
			}
		}
	}

	fclose(no);
	return RetVal;
}



void msged_areas(FILE *fp)
{
    char    *temp, *aka;
    FILE    *no;
    int     i = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
        return;

    fread(&msgshdr, sizeof(msgshdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&msgshdr, msgshdr.hdrsize, 1, no);

    while (fread(&msgs, msgshdr.recsize, 1, no) == 1) {

	i++;
	if (msgs.Active) {

	    fprintf(fp, "Jam ");

	    switch (msgs.Type) {
		case LOCALMAIL:	fprintf(fp, "l");   break;
		case NETMAIL:	fprintf(fp, "mp");  break;
		case ECHOMAIL:	fprintf(fp, "e");   break;
		case NEWS:	fprintf(fp, "e");   break;
	    }

	    if (((msgs.Type == NEWS) || (msgs.Type == ECHOMAIL)) && strlen(msgs.Tag) && strlen(msgs.Newsgroup)) {
		fprintf(fp, "u");
	    }
	    fprintf(fp, "8");
	    fprintf(fp, " \"%s\" %s", msgs.Name, msgs.Base);
	    if (msgs.Type == ECHOMAIL)
		fprintf(fp, " %s", msgs.Tag);
	    if (msgs.Type != LOCALMAIL) {
		aka = xstrcpy(strtok(aka2str(msgs.Aka), "@"));
		fprintf(fp, " %s", aka);
		free(aka);
	    }
	    fprintf(fp, "\n");
	}
	fseek(no, msgshdr.syssize, SEEK_CUR);
    }

    fclose(no);
    free(temp);
}


/*
 * AREADEF Syntax: Echoid "Description" Groupid Type Format Path or board Aka (Attributes) "Origin"
 */
void gold_areas(FILE *fp)
{
    char    *temp, *aka, groupid[13];
    FILE    *no, *fil;
    int	    i = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL) {
	free(temp);
	return;
    }

    fread(&msgshdr, sizeof(msgshdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&msgshdr, msgshdr.hdrsize, 1, no);

    fprintf(fp, "; Message Areas\n;\n");
    fprintf(fp, "AREASCAN *\n");
    fprintf(fp, "AREATYPEORDER Net Email Echo News Local\n");

    while (fread(&msgs, msgshdr.recsize, 1, no) == 1) {

	i++;
	if (msgs.Active) {
	    fprintf(fp, "AREADEF ");
	    if (strlen(msgs.Tag))
		fprintf(fp, "%s", msgs.Tag);
	    else
		fprintf(fp, "AREA%d", i);
	    fprintf(fp, " \"%s\" ", msgs.Name);
	    /*
	     * Set a default groupid
	     */
	    switch (msgs.Type) {
		case LOCALMAIL	: snprintf(groupid, 13, "O");	break;
		case NETMAIL	: snprintf(groupid, 13, "N");	break;
		case ECHOMAIL	: snprintf(groupid, 13, "C");	break;
		case NEWS	: snprintf(groupid, 13, "I");	break;
	    }
	    /*
	     * Now try to find a real groupid
	     */
	    if (((msgs.Type == ECHOMAIL) || (msgs.Type == NEWS)) && strlen(msgs.Group)) {
		snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
		if ((fil = fopen(temp, "r")) != NULL) {
		    fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
		    while (fread(&mgroup, mgrouphdr.recsize, 1, fil) == 1) {
			if (mgroup.Active && !strcmp(msgs.Group, mgroup.Name) && mgroup.GoldEDgroup) {
			    snprintf(groupid, 13, "#%03d", mgroup.GoldEDgroup);
			    break;
			}
		    }
		    fclose(fil);
		}
	    }
	    fprintf(fp, "%s ", groupid);

	    switch (msgs.Type) {
		case LOCALMAIL	: fprintf(fp, "Local");	break;
		case NETMAIL	: fprintf(fp, "Net");   break;
		case ECHOMAIL	: fprintf(fp, "Echo");	break;
		case NEWS	: fprintf(fp, "News");	break;
	    }
	    aka = xstrcpy(strtok(aka2str(msgs.Aka), "@"));
	    fprintf(fp, " JAM %s %s ", msgs.Base, aka);
	    free(aka);
	    if (msgs.Type == NETMAIL)
		fprintf(fp, "(Loc Pvt)");
	    else
		fprintf(fp, "(Loc)");
	    fprintf(fp, " \"%s\"\n", msgs.Origin);
	}
	fseek(no, msgshdr.syssize, SEEK_CUR);
    }

    fclose(no);
    free(temp);
    fprintf(fp, "\n");
}

	

int mail_area_doc(FILE *fp, FILE *toc, int page)
{
    char	temp[PATH_MAX], status[5];
    FILE	*ti, *wp, *ip, *no;
    int		i = 0, j, k, systems, refs, First = TRUE, LMiy;
    sysconnect	System;
    struct tm	*t;
    time_t	Now, tt;

    Now = time(NULL);
    t = localtime(&Now);
	
    Diw = t->tm_wday;
    Miy = t->tm_mon;
    if (Miy == 0)
	LMiy = 11;
    else
	LMiy = Miy - 1;
	
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    fread(&msgshdr, sizeof(msgshdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&msgshdr, msgshdr.hdrsize, 1, no);
    systems = msgshdr.syssize / sizeof(sysconnect);

    ip = open_webdoc((char *)"msgareas.html", (char *)"Message Areas", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Area</TH><TH align='left'>Comment</TH><TH align='left'>Tag</TH></TR>\n");

    while (fread(&msgs, msgshdr.recsize, 1, no) == 1) {

	i++;
	if (msgs.Active) {
	    page = newpage(fp, page);

	    if (First) {
		addtoc(fp, toc, 9, 2, page, (char *)"Mail areas");
		First = FALSE;
		fprintf(fp, "\n");
	    } else
		fprintf(fp, "\n\n");

	    snprintf(temp, 81, "msgarea_%d.html", i);
	    fprintf(ip, " <TR><TD><A HREF=\"%s\">%d</A></TD><TD>%s</TD><TD>%s</TD>\n", 
		    temp, i, msgs.Name, strlen(msgs.Tag) ? msgs.Tag : "&nbsp;");
	    if ((wp = open_webdoc(temp, (char *)"File area", msgs.Name))) {
		fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"msgareas.html\">Back</A>\n");
		fprintf(wp, "<P>\n");
		fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
		fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
		fprintf(wp, "<TBODY>\n");
		add_webdigit(wp, (char *)"Area number", i);
		add_webtable(wp, (char *)"Area name", msgs.Name);
		add_webtable(wp, (char *)"Area tag", msgs.Tag);
		add_webtable(wp, (char *)"Newsgroup name", msgs.Newsgroup);
		add_webtable(wp, (char *)"JAM message base", msgs.Base);
		add_webtable(wp, (char *)"Offline name", msgs.QWKname);
		add_webtable(wp, (char *)"Area type", getmsgtype(msgs.Type));
		add_webtable(wp, (char *)"Messages type", getmsgkinds(msgs.MsgKinds));
		add_webtable(wp, (char *)"Character set", getftnchrs(msgs.Charset));
		add_webdigit(wp, (char *)"Days old msgs", msgs.DaysOld);
		add_webdigit(wp, (char *)"Maximum msgs", msgs.MaxMsgs);
		add_webdigit(wp, (char *)"Max articles", msgs.MaxArticles);
		add_webtable(wp, (char *)"Users delete", getboolean(msgs.UsrDelete));
		web_secflags(wp, (char *)"Read security", msgs.RDSec);
		web_secflags(wp, (char *)"Write security", msgs.WRSec);
		web_secflags(wp, (char *)"Sysop security", msgs.SYSec);
		add_webtable(wp, (char *)"Link security", getflag(msgs.LinkSec.flags, msgs.LinkSec.notflags));
		add_webdigit(wp, (char *)"Minimum age", msgs.Age);
		add_webtable(wp, (char *)"Password", msgs.Password);
		add_webtable(wp, (char *)"Group", msgs.Group);
		add_webtable(wp, (char *)"Fido address", aka2str(msgs.Aka));
		add_webdigit(wp, (char *)"Netmail board", msgs.NetReply);
		add_webtable(wp, (char *)"Origin line", msgs.Origin);
		add_webtable(wp, (char *)"Allow aliases", getboolean(msgs.Aliases));
		add_webtable(wp, (char *)"OLR mandatory", getboolean(msgs.OLR_Forced));
		add_webtable(wp, (char *)"OLR default on", getboolean(msgs.OLR_Default));
		add_webtable(wp, (char *)"Append quotes", getboolean(msgs.Quotes));
		add_webtable(wp, (char *)"Nodes mandatory", getboolean(msgs.Mandatory));
		add_webtable(wp, (char *)"UnSecure toss", getboolean(msgs.UnSecure));
		tt = (time_t)msgs.LastRcvd;
		add_webtable(wp, (char *)"Last msg rcvd", ctime(&tt));
		tt = (time_t)msgs.LastPosted;
		add_webtable(wp, (char *)"Last msg posted", ctime(&tt));
		tt = (time_t)msgs.Created;
		add_webtable(wp, (char *)"Area created at", ctime(&tt));
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
		fprintf(wp, "<HR>\n");
		fprintf(wp, "<H3>Nodes Reference</H3>\n");
	    }

	    fprintf(fp, "    Area number      %d\n", i);
	    fprintf(fp, "    Area name        %s\n", msgs.Name);
	    fprintf(fp, "    Area tag         %s\n", msgs.Tag);
	    fprintf(fp, "    Newsgroup name   %s\n", msgs.Newsgroup);
	    fprintf(fp, "    JAM message base %s\n", msgs.Base);
	    fprintf(fp, "    Offline name     %s\n", msgs.QWKname);
	    fprintf(fp, "    Area type        %s\n", getmsgtype(msgs.Type));
	    fprintf(fp, "    Messages type    %s\n", getmsgkinds(msgs.MsgKinds));
	    fprintf(fp, "    Character set    %s\n", getftnchrs(msgs.Charset));
	    fprintf(fp, "    Days old msgs.   %d\n", msgs.DaysOld);
	    fprintf(fp, "    Maximum msgs.    %d\n", msgs.MaxMsgs);
	    fprintf(fp, "    Max articles     %d\n", msgs.MaxArticles);
	    fprintf(fp, "    Users delete     %s\n", getboolean(msgs.UsrDelete));
	    fprintf(fp, "    Read security    %s\n", get_secstr(msgs.RDSec));
	    fprintf(fp, "    Write security   %s\n", get_secstr(msgs.WRSec));
	    fprintf(fp, "    Sysop security   %s\n", get_secstr(msgs.SYSec));
	    fprintf(fp, "    Link security          %s\n", getflag(msgs.LinkSec.flags, msgs.LinkSec.notflags));
	    fprintf(fp, "    Minimum age      %d\n", msgs.Age);
	    fprintf(fp, "    Password         %s\n", msgs.Password);
	    fprintf(fp, "    Group            %s\n", msgs.Group);
	    fprintf(fp, "    Fido address     %s\n", aka2str(msgs.Aka));
	    fprintf(fp, "    Netmail board    %d\n", msgs.NetReply);
	    fprintf(fp, "    Origin line      %s\n", msgs.Origin);
	    fprintf(fp, "    Allow aliases    %s\n", getboolean(msgs.Aliases));
	    fprintf(fp, "    OLR mandatory    %s\n", getboolean(msgs.OLR_Forced));
	    fprintf(fp, "    OLR default on   %s\n", getboolean(msgs.OLR_Default));
	    fprintf(fp, "    Append quotes    %s\n", getboolean(msgs.Quotes));
	    fprintf(fp, "    Nodes mandatory  %s\n", getboolean(msgs.Mandatory));
	    fprintf(fp, "    UnSecure toss    %s\n", getboolean(msgs.UnSecure));
	    tt = (time_t)msgs.LastRcvd;
	    fprintf(fp, "    Last msg rcvd.   %s",   ctime(&tt));
	    tt = (time_t)msgs.LastPosted;
	    fprintf(fp, "    Last msg posted  %s",   ctime(&tt));
	    tt = (time_t)msgs.Created;
	    fprintf(fp, "    Area created at  %s",   ctime(&tt));

	    refs = 0;
	    for (j = 0; j < systems; j++) {
		fread(&System, sizeof(sysconnect), 1, no);
		if (System.aka.zone) {
		    if ((refs == 0) && (wp != NULL)) {
			fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			fprintf(wp, "<COL width='20%%'><COL witdh='10%%'><COL width='70%%'>\n");
			fprintf(wp, "<TBODY>\n");
		    }
		    refs++;
		    memset(&status, 0, 5);
		    memset(&status, '-', 4);
		    if (System.sendto)
			status[0] = 'S';
		    if (System.receivefrom)
			status[1] = 'R';
		    if (System.pause)
			status[2] = 'P';
		    if (System.cutoff)
			status[3] = 'C';

		    fprintf(fp, "    Link %2d          %s %s\n", j+1, status, aka2str(System.aka));
		    if (wp != NULL) {
			snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
			if ((ti = fopen(temp, "r"))) {
			    fread(&nodeshdr, sizeof(nodeshdr), 1, ti);
			    fseek(ti, 0, SEEK_SET);
			    fread(&nodeshdr, nodeshdr.hdrsize, 1, ti);
			    while ((fread(&nodes, nodeshdr.recsize, 1, ti)) == 1) {
				fseek(ti, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
				for (k = 0; k < 20; k++) {
				    if ((nodes.Aka[k].zone == System.aka.zone) &&
					(nodes.Aka[k].net == System.aka.net ) &&
					(nodes.Aka[k].node == System.aka.node) &&
					(nodes.Aka[k].point == System.aka.point) &&
					(strcmp(nodes.Aka[k].domain, System.aka.domain) == 0)) {
					fprintf(wp, "<TR><TD><A HREF=\"node_%d_%d_%d_%d_%s.html\">%s</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
						nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node, nodes.Aka[0].point, 
						nodes.Aka[0].domain, aka2str(nodes.Aka[0]), status, nodes.Sysop);
				    }
				}
			    }
			    fclose(ti);
			}
		    }
		}
	    }
	    if (wp != NULL) {
		if (refs == 0)
		    fprintf(wp, "No Nodes References\n");
		else {
		    fprintf(wp, "</TBODY>\n");
		    fprintf(wp, "</TABLE>\n");
		}
		fprintf(wp, "<HR>\n");
                fprintf(wp, "<H3>Area Statistics</H3>\n");
		add_statcnt(wp, (char *)"received msgs", msgs.Received);
		add_statcnt(wp, (char *)"posted msgs", msgs.Posted);
		close_webdoc(wp);
	    }
	    fprintf(fp, "\n");
	    fprintf(fp, "                     Total      This Month Last Month\n");
	    fprintf(fp, "                     ---------- ---------- ----------\n");
	    fprintf(fp, "    Msgs received    %-10d %-10d %-10d\n", msgs.Received.total, msgs.Received.month[Miy], msgs.Received.month[LMiy]);
	    fprintf(fp, "    Msgs posted      %-10d %-10d %-10d\n", msgs.Posted.total, msgs.Posted.month[Miy], msgs.Posted.month[LMiy]);

	} else
	    fseek(no, msgshdr.syssize, SEEK_CUR);
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	
    fclose(no);
    return page;
}


