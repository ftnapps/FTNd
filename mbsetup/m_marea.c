/*****************************************************************************
 *
 * File ..................: m_marea.c
 * Purpose ...............: Message Areas Setup
 * Last modification date : 19-Oct-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "grlist.h"
#include "m_global.h"
#include "m_node.h"
#include "m_mgroup.h"
#include "m_marea.h"


int		MsgUpdated = 0;
unsigned long	MsgCrc;
FILE		*tfil = NULL;



/*
 * Count nr of msgs records in the database.
 * Creates the database if it doesn't exist.
 */
int CountMsgarea(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			msgshdr.hdrsize = sizeof(msgshdr);
			msgshdr.recsize = sizeof(msgs);
			msgshdr.syssize = CFG.toss_systems * sizeof(sysconnect);
			msgshdr.lastupd = time(NULL);
			fwrite(&msgshdr, sizeof(msgshdr), 1, fil);
			fclose(fil);
			return 0;
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
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize, oldsys;
	struct	_sysconnect syscon;
	int	i, oldsystems;

	sprintf(fnin,  "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
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
			if ((oldsize != sizeof(msgs)) || (CFG.toss_systems != oldsystems)) {
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
				if ((oldsize != sizeof(msgs)) && !msgs.Rfccode) {
					msgs.Rfccode = CHRS_DEFAULT_RFC;
					msgs.Ftncode = CHRS_DEFAULT_FTN;
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
					 * The size is increased, fill with
					 * blank records.
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

	sprintf(fin, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/mareas.temp", getenv("MBSE_ROOT"));

	if (MsgUpdated == 1) {
		if (Force || (yes_no((char *)"Messages database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			Syslog('+', "Updated \"mareas.data\"");
			return;
		}
	}
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
	msgs.Rfccode = CHRS_DEFAULT_RFC;
	msgs.Ftncode = CHRS_DEFAULT_FTN;
	strcpy(msgs.Origin, CFG.origin);
}



int AppendMsgarea(void);
int AppendMsgarea()
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	struct	_sysconnect syscon;
	int	i;

	sprintf(ffile, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
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
			mvprintw( 5,6, "9.2.26 EDIT CONNECTION");
			set_color(CYAN, BLACK);
			mvprintw( 7,6, "1.     Aka");
			mvprintw( 8,6, "2.     Send to");
			mvprintw( 9,6, "3.     Recv from");
			mvprintw(10,6, "4.     Pause");
			mvprintw(11,6, "5.     Excluded");
			mvprintw(12,6, "6.     Delete");
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
			case 1:	S.aka = PullUplink((char *)"9.2.29");
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
	long		offset;
	char		pick[12];
	sysconnect	System;
	char		status[5];
	char		temp[41];

	systems = msgshdr.syssize / sizeof(sysconnect);
	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 5, "9.2.29  MESSAGE AREA CONNECTIONS");
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
					sprintf(temp, "%3d. %s %d:%d/%d.%d@%s", o+i, status, System.aka.zone, System.aka.net, System.aka.node, System.aka.point, System.aka.domain);
				} else {
					set_color(LIGHTBLUE, BLACK);
					sprintf(temp, "%3d.", o+i);
				}
				mvprintw(y, x, temp);
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
	mvprintw( 4, 2, "9.2 EDIT MESSAGE AREA");
	set_color(CYAN, BLACK);
	mvprintw( 6, 2, "1.  Area Name");
	mvprintw( 7, 2, "2.  FTN area");
	mvprintw( 8, 2, "3.  Newsgroup");
	mvprintw( 9, 2, "4.  Moderator");
	mvprintw(10, 2, "5.  JAM base");
	mvprintw(11, 2, "6.  Origin");
	mvprintw(12, 2, "7.  Fido Aka");
	mvprintw(13, 2, "8.  QWK name");
	mvprintw(14, 2, "9.  Group");
	mvprintw(15, 2, "10. Distrib.");
	mvprintw(16, 2, "11. Area Type");
	mvprintw(17, 2, "12. Msg Kinds");
	mvprintw(18, 2, "13. FTN chars");
	mvprintw(19, 2, "14. RFC chars");

	mvprintw(13,36, "15. Days Old");
	mvprintw(14,36, "16. Max. Msgs");
	mvprintw(15,36, "17. Netreply");
	mvprintw(16,36, "18. Active");
	mvprintw(17,36, "19. Read Sec.");
	mvprintw(18,36, "20. Write Sec.");
	mvprintw(19,36, "21. Sysop Sec.");

	mvprintw(12,60, "22. User Del.");
	mvprintw(13,60, "23. Aliases");
	mvprintw(14,60, "24. Quotes");
	mvprintw(15,60, "25. Mandatory");
	mvprintw(16,60, "26. UnSecure");
	mvprintw(17,60, "27. OLR Default");
	mvprintw(18,60, "28. OLR Forced");
	mvprintw(19,60, "29. Connections");
} 



long LoadMsgRec(int, int);
long LoadMsgRec(int Area, int work)
{
	FILE		*fil;
	char		mfile[PATH_MAX];
	long		offset;
	sysconnect	System;
	int		i;

	if (work)
		working(1, 0, 0);

	sprintf(mfile, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
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
	if (work)
		working(0, 0, 0);

	return offset;
}



int SaveMsgRec(int, int);
int SaveMsgRec(int Area, int work)
{
	int		i;
	FILE		*fil;
	long		offset;
	char		mfile[PATH_MAX];
	sysconnect	System;

	if (work)
		working(1, 0, 0);
	sprintf(mfile, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
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
	if (work)
		working(0, 0, 0);
	return 0;
}



void ShowStatus(sysconnect);
void ShowStatus(sysconnect S)
{
	clr_index();
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "Aka");
	mvprintw( 8, 6, "Send to");
	mvprintw( 9, 6, "Recv from");
	mvprintw(10, 6, "Pause");
	mvprintw(11, 6, "Excluded");
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
	gr_list		*mgr = NULL, *tmp;
	char		*p, mfile[PATH_MAX];
	FILE		*fil;
	fidoaddr	a1, a2;
	int		menu = 0, marea, Areas, akan = 0, Found;
	int		Total, Done, netbrd, daysold, maxmsgs;
	long		offset;
	securityrec	rs, ws, ss;
	sysconnect	S, Sc;

	/*
	 * Build the groups select array
	 */
	working(1, 0, 0);
	sprintf(mfile, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) != NULL) {
		fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);

		while (fread(&mgroup, mgrouphdr.recsize, 1, fil) == 1)
			fill_grlist(&mgr, mgroup.Name);
	
		fclose(fil);
		sort_grlist(&mgr);
	}
	working(0, 0, 0);

	/*
	 * Initialize some variables
	 */
	memset(&rs, 0, sizeof(securityrec));
	memset(&ws, 0, sizeof(securityrec));
	memset(&ss, 0, sizeof(securityrec));
	memset(&S, 0, sizeof(sysconnect));
	S.sendto = TRUE;
	S.receivefrom = TRUE;
	memset(&mfile, 0, sizeof(mfile));
	sprintf(mfile, "%s", CFG.origin);
	daysold = CFG.defdays;
	maxmsgs = CFG.defmsgs;
	netbrd = 1;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "9.2 GLOBAL EDIT MESSAGE AREAS");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.  Delete connection");
		mvprintw( 8, 6, "2.  Add new connection");
		mvprintw( 9, 6, "3.  Replace connection");
		mvprintw(10, 6, "4.  Change connection status");
		mvprintw(11, 6, "5.  Change days old");
		mvprintw(12, 6, "6.  Change max. messages");
		mvprintw(13, 6, "7.  Change security");
		mvprintw(14, 6, "8.  Change aka to use");
		mvprintw(15, 6, "9.  Change origin line");
		mvprintw(16, 6, "10. Change netmail reply");
		mvprintw(17, 6, "11. Delete message area");

		memset(&a1, 0, sizeof(fidoaddr));
		memset(&a2, 0, sizeof(fidoaddr));

		menu = select_menu(11);
		switch (menu) {
			case 0: return;
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
			case 5: mvprintw(LINES -3, 5, "Days old");
				E_INT(LINES -3, 14, daysold, (char *)"Enter new number of ^days old^")
			case 6: mvprintw(LINES -3, 5, "Max. messages");
				E_INT(LINES -3, 19, maxmsgs, (char *)"Enter ^maximum messages^")
			case 7: rs = edit_sec(6, 5, rs, (char *)"9.2.7 READ SECURITY");
				ws = edit_sec(7, 5, ws, (char *)"9.2.7 WRITE SECURITY");
				ss = edit_sec(8, 5, ss, (char *)"9.2.7 SYSOP SECURITY");
				break;
			case 8: akan = PickAka((char *)"9.2.8", TRUE);
				break;
			case 9: E_STR(LINES -3, 5, 64, mfile, "Enter new ^origin^ line");
			case 10:mvprintw(LINES -3, 5, "Netmail reply board");
				E_INT(LINES -3, 25, netbrd, (char *)"The ^netmail reply^ board number")
		}

		E_Group(&mgr, (char *)"SELECT MESSAGE GROUPS TO CHANGE");

		/*
		 * Show settings before proceeding
		 */
		switch (menu) {
			case 1:	mvprintw(7, 6, "Delete aka %s", aka2str(a1));
				break;
			case 2: mvprintw(7, 6, "Add aka %s", aka2str(a2));
				break;
			case 3: p = xstrcpy(aka2str(a1));
				mvprintw(7, 6, "Replace aka %s with %s", p, aka2str(a2));
				free(p);
				break;
			case 4: ShowStatus(S);
				mvprintw(14, 6, "Change the link status");
				break;
			case 5: mvprintw(7, 6, "Change days old to %d", daysold);
				break;
			case 6: mvprintw(7, 6, "Change maximum messages to %d", maxmsgs);
				break;
			case 7:	set_color(CYAN, BLACK);
				mvprintw(7, 6, "Read security");
				mvprintw(8, 6, "Write security");
				mvprintw(9, 6, "Sysop security");
				set_color(WHITE, BLACK);
				show_sec(7, 21, rs);
				show_sec(8, 21, ws);
				show_sec(9, 21, ss);
				break;
			case 8:	if (akan != -1)
					mvprintw( 7, 6, "Set %s as new aka to use", aka2str(CFG.aka[akan]));
				break;
			case 9:	mvprintw(7, 6, "Origin: %s", mfile);
				break;
			case 10:mvprintw(7, 6, "New netmail reply board %d", netbrd);
				break;
			case 11:mvprintw(7, 6, "Delete message areas");
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
									if ((Sc.aka.zone == a1.zone) &&
									    (Sc.aka.net == a1.net) &&
									    (Sc.aka.node == a1.node) &&
									    (Sc.aka.point == a1.point)) {
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
									if ((Sc.aka.zone == a2.zone) && 
									    (Sc.aka.net == a2.net) &&
									    (Sc.aka.node == a2.node) &&
									    (Sc.aka.point == a2.point)) {
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
										sprintf(Sc.aka.domain, "%s", a2.domain);
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
									if ((Sc.aka.zone == a1.zone) &&
									    (Sc.aka.net == a1.net) &&
									    (Sc.aka.node == a1.node) &&
									    (Sc.aka.point == a1.point)) {
										Sc.aka.zone = a2.zone;
										Sc.aka.net = a2.net;
										Sc.aka.node = a2.node;
										Sc.aka.point = a2.point;
										sprintf(Sc.aka.domain, "%s", a2.domain);
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
							case 4:	fseek(tfil, 0, SEEK_SET);
								while (fread(&Sc, sizeof(sysconnect), 1, tfil) == 1) {
									if ((Sc.aka.zone == S.aka.zone) &&
									    (Sc.aka.net == S.aka.net) &&
									    (Sc.aka.node == S.aka.node) &&
									    (Sc.aka.point == S.aka.point)) {
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
							case 6:	if (maxmsgs != msgs.MaxMsgs) {
									msgs.MaxMsgs = maxmsgs;
									if (SaveMsgRec(marea, FALSE) == 0) {
										Done++;
										Syslog('+', "Changed max. messages to %d in area %s", maxmsgs, msgs.Tag);
									}
								}
								break;
							case 7:	if ((msgs.RDSec.level != rs.level) ||
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
										Syslog('+', "Updated security levels in area %s", msgs.Tag);
									}
								}
								break;
							case 8:	if (akan != -1) {
									if ((msgs.Aka.zone != CFG.aka[akan].zone) ||
									    (msgs.Aka.net != CFG.aka[akan].net) ||
									    (msgs.Aka.node != CFG.aka[akan].node) ||
									    (msgs.Aka.point != CFG.aka[akan].point)) {
										msgs.Aka.zone = CFG.aka[akan].zone;
										msgs.Aka.net = CFG.aka[akan].net;
										msgs.Aka.node = CFG.aka[akan].node;
										msgs.Aka.point = CFG.aka[akan].point;
										sprintf(msgs.Aka.domain, "%s", CFG.aka[akan].domain);
										if (SaveMsgRec(marea, FALSE) == 0) {
											Done++;
											Syslog('+', "Area %s now uses aka %s", msgs.Tag, aka2str(msgs.Aka));
										}
									}
								}
								break;
							case 9:	if (strcmp(msgs.Origin, mfile)) {
									sprintf(msgs.Origin, "%s", mfile);
									if (SaveMsgRec(marea, FALSE) == 0) {
										Done++;
										Syslog('+', "Changed origin line in area %s", msgs.Tag);
									}
								}
								break;
							case 10:if (netbrd != msgs.NetReply) {
									msgs.NetReply = netbrd;
									if (SaveMsgRec(marea, FALSE) == 0) {
										Done++;
										Syslog('+', "Changed netmail board to %d in area %s", netbrd, msgs.Tag);
									}
								}
								break;
							case 11:if (msgs.Active) {
									msgs.Active = FALSE;
									memset(&msgs.Name, 0, sizeof(msgs.Name));
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
			}

			working(0, 0, 0);
			mvprintw(LINES -3, 6,"Made %d changes in %d possible areas", Done, Total);
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
	unsigned long	crc1;
	int		tmp, i, changed = FALSE;
	sysconnect	System;

	clr_index();
	IsDoing("Edit Msg Area");

	if (LoadMsgRec(Area, TRUE) == -1)
		return -1;

	SetScreen();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 6,16,40, msgs.Name);
		show_str( 7,16,50, msgs.Tag);
		show_str( 8,16,64, msgs.Newsgroup);
		show_str( 9,16,64, msgs.Moderator);
		show_str(10,16,64, msgs.Base);
		show_str(11,16,64, msgs.Origin);
		show_aka(12,16,    msgs.Aka);
		show_str(13,16,13, msgs.QWKname);
		show_str(14,16,12, msgs.Group);
		show_str(15,16,16, msgs.Distribution);
		show_msgtype(16,16, msgs.Type);
		show_msgkinds(17,16, msgs.MsgKinds);
		show_str(18,16,16, printable(getchrs(msgs.Ftncode), 0));
		show_str(19,16,16, printable(getchrs(msgs.Rfccode), 0));

		show_int( 13,52, msgs.DaysOld);
		show_int( 14,52, msgs.MaxMsgs);
		show_int( 15,52, msgs.NetReply);
		show_bool(16,52, msgs.Active);
		show_int( 17,52, msgs.RDSec.level);
		show_int( 18,52, msgs.WRSec.level);
		show_int( 19,52, msgs.SYSec.level);

		show_bool(12,76, msgs.UsrDelete);
		show_bool(13,76, msgs.Aliases);
		show_bool(14,76, msgs.Quotes);
		show_bool(15,76, msgs.Mandatory);
		show_bool(16,76, msgs.UnSecure);
		show_bool(17,76, msgs.OLR_Default);
		show_bool(18,76, msgs.OLR_Forced);

		switch(select_menu(29)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&msgs, crc1, msgshdr.recsize);
			fseek(tfil, 0, 0);
			for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
				fread(&System, sizeof(sysconnect), 1, tfil);
				crc1 = upd_crc32((char *)&System, crc1, sizeof(sysconnect));
			}
			if ((MsgCrc != crc1) || (changed)) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					if (SaveMsgRec(Area, TRUE) == -1)
						return -1;
					MsgUpdated = 1;
					Syslog('+', "Saved record %d", Area);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR(  6,16,40,msgs.Name,       "The ^Name^ of this area")
		case 2:	strcpy(msgs.Tag, edit_ups(7,16,50, msgs.Tag, (char *)"The ^Area Tag^ for Echomail"));
			if (!strlen(msgs.QWKname)) {
				memset(&msgs.QWKname, '\0', strlen(msgs.QWKname));
				strncpy(msgs.QWKname, msgs.Tag, 13);
			}
			break;
		case 3: E_STR(  8,16,64,msgs.Newsgroup,  "The ^Newsgroup^ name of this area")
		case 4: E_STR(  9,16,64,msgs.Moderator,  "The ^Moderator^ if this is a moderated area")
		case 5: E_JAM( 10,16,64,msgs.Base,       "The path to the ^JAM Message Base^")
		case 6:	E_STR( 11,16,64,msgs.Origin,     "The ^Origin line^ to append to Echomail messages")
                case 7:	tmp = PickAka((char *)"9.2.7", TRUE);
                        if (tmp != -1)
                                msgs.Aka = CFG.aka[tmp];
                        SetScreen(); break;
		case 8:	E_UPS( 13,16,13,msgs.QWKname,      "The name for ^QWK or Bluewave^ message packets")
		case 9:	strcpy(msgs.Group, PickMGroup((char *)"9.2.9"));
			if (strlen(msgs.Group)) {
				msgs.Aka = mgroup.UseAka;
				msgs.Active = TRUE;
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
			}
			SetScreen(); 
			break;
		case 10:E_STR( 15,16,16,msgs.Distribution,  "The ^Distribution^ name if this is a newsgroup")
		case 11:msgs.Type = edit_msgtype(16,16, msgs.Type); break;
		case 12:msgs.MsgKinds = edit_msgkinds(17,16, msgs.MsgKinds); break;

		case 15:E_INT( 13,52,   msgs.DaysOld,       "Maximum ^days^ to keep mail in this area")
		case 16:E_INT( 14,52,   msgs.MaxMsgs,       "The ^maximum^ amount of messages in this area")
		case 17:E_INT( 15,52,   msgs.NetReply,      "The ^Area Number^ for netmail replies")
		case 18:E_BOOL(16,52,   msgs.Active,        "Is this area ^Active^")
		case 19:E_SEC( 17,52,   msgs.RDSec,         "9.2 EDIT READ SECURITY", SetScreen)
		case 20:E_SEC( 18,52,   msgs.WRSec,         "9.2 EDIT WRITE SECURITY", SetScreen)
		case 21:E_SEC( 19,52,   msgs.SYSec,         "9.2 EDIT SYSOP SECURITY", SetScreen)

		case 22:E_BOOL(12,76,   msgs.UsrDelete,     "Allow users to ^Delete^ their messages")
		case 23:E_BOOL(13,76,   msgs.Aliases,       "Allow ^aliases^ or real names only")
		case 24:E_BOOL(14,76,   msgs.Quotes,        "Add random ^quotes^ to new messages")
		case 25:E_BOOL(15,76,   msgs.Mandatory,     "Is this area ^mandatory^ for nodes")
		case 26:E_BOOL(16,76,   msgs.UnSecure,      "Toss messages ^UnSecure^, ie: no originating check")
		case 27:E_BOOL(17,76,   msgs.OLR_Default,   "Area is ^default^ for ^offline^ users.")
		case 28:E_BOOL(18,76,   msgs.OLR_Forced,    "Area is ^always on^ for ^offline^ users.")
		case 29:if (EditConnections(tfil))
				changed = TRUE;
			SetScreen(); break;
		}
	}
}



void EditMsgarea(void)
{
	int		records, i, o, y;
	char		pick[12];
	FILE		*fil;
	char		temp[PATH_MAX];
	long		offset;
	int		from, too;
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
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 3, "9.2 MESSAGE AREA SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/mareas.temp", getenv("MBSE_ROOT"));
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
							sprintf(temp, "%3d. %-8s %-25s %-40s", o + i, getmsgtype(msgs.Type), msgs.Tag, msgs.Name);
						} else {
							set_color(LIGHTBLUE, BLACK);
							sprintf(temp, "%3d.", o+i);
						}
						mvprintw(y, 2, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_area(records, 10));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseMsgarea(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendMsgarea() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
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
			mvprintw(LINES -3, 5, "From");
			from = edit_int(LINES -3, 10, from, (char *)"Wich ^area^ you want to move");
			mvprintw(LINES -3,15, "To");
			too  = edit_int(LINES -3, 18, too,  (char *)"Too which ^area^ to move");
			if ((LoadMsgRec(from, TRUE) == -1) || (!msgs.Active)) {
				errmsg((char *)"The originating area is invalid");
				fclose(tfil);
			} else {
				fclose(tfil);
				if ((LoadMsgRec(too, TRUE) == -1) || (msgs.Active)) {
					errmsg((char *)"The destination area is invalid");
					fclose(tfil);
				} else {
					fclose(tfil);
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
				}
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
	long		offset;
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

	working(0, 0, 0);

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		sprintf(temp, "%s.  MESSAGE AREA SELECT", shdr);
		mvprintw(5, 3, temp);
		set_color(CYAN, BLACK);

		if (records) {
			sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
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
						sprintf(temp, "%3d.  %-31s", o + i, msgs.Name);
						temp[38] = '\0';
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
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
			sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				offset = msgshdr.hdrsize + ((atoi(pick) - 1) * (msgshdr.recsize + msgshdr.syssize));
				fseek(fil, offset, SEEK_SET);
				fread(&msgs, msgshdr.recsize, 1, fil);
				fclose(fil);
				if (msgs.Active) {
					memset(&Buf, 0, sizeof(Buf));
					sprintf(Buf, "%s", msgs.Base);
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

	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
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

	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
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



void gold_areas(FILE *fp)
{
	char	*temp;
	FILE	*no;
	int	i = 0;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return;

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
			switch (msgs.Type) {
				case LOCALMAIL	: fprintf(fp, "0 Local");	break;
				case NETMAIL	: fprintf(fp, "N Net");   	break;
				case ECHOMAIL	: fprintf(fp, "C Echo");	break;
				case NEWS	: fprintf(fp, "I News");	break;
			}
			fprintf(fp, " JAM %s . ", msgs.Base);
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
	char		temp[PATH_MAX], status[5];
	FILE		*no;
	int		i = 0, j, systems, First = TRUE;
	sysconnect	System;
	int		LMiy;
	struct tm	*t;
	time_t		Now;

	Now = time(NULL);
	t = localtime(&Now);
	
	Diw = t->tm_wday;
	Miy = t->tm_mon;
	if (Miy == 0)
		LMiy = 11;
	else
		LMiy = Miy - 1;
	
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	fread(&msgshdr, sizeof(msgshdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&msgshdr, msgshdr.hdrsize, 1, no);
	systems = msgshdr.syssize / sizeof(sysconnect);

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

			fprintf(fp, "    Area number      %d\n", i);
			fprintf(fp, "    Area name        %s\n", msgs.Name);
			fprintf(fp, "    Area tag         %s\n", msgs.Tag);
			fprintf(fp, "    Newsgroup name   %s\n", msgs.Newsgroup);
			fprintf(fp, "    Distribution     %s\n", msgs.Distribution);
			fprintf(fp, "    Moderator        %s\n", msgs.Moderator);
			fprintf(fp, "    JAM message base %s\n", msgs.Base);
			fprintf(fp, "    Offline name     %s\n", msgs.QWKname);
			fprintf(fp, "    Area type        %s\n", getmsgtype(msgs.Type));
			fprintf(fp, "    Messages type    %s\n", getmsgkinds(msgs.MsgKinds));
			fprintf(fp, "    FTN charset      %s\n", printable(getchrs(msgs.Ftncode), 0));
			fprintf(fp, "    RFC charset      %s\n", printable(getchrs(msgs.Rfccode), 0));
			fprintf(fp, "    Days old msgs.   %d\n", msgs.DaysOld);
			fprintf(fp, "    Maximum msgs.    %d\n", msgs.MaxMsgs);
			fprintf(fp, "    Users delete     %s\n", getboolean(msgs.UsrDelete));
			fprintf(fp, "    Read security    %s\n", get_secstr(msgs.RDSec));
			fprintf(fp, "    Write security   %s\n", get_secstr(msgs.WRSec));
			fprintf(fp, "    Sysop security   %s\n", get_secstr(msgs.SYSec));
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
			fprintf(fp, "    Last msg rcvd.   %s",   ctime(&msgs.LastRcvd));
			fprintf(fp, "    Last msg posted  %s",   ctime(&msgs.LastPosted));

			for (j = 0; j < systems; j++) {
				fread(&System, sizeof(sysconnect), 1, no);
				if (System.aka.zone) {
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
				}
			}
			fprintf(fp, "\n");
			fprintf(fp, "                     Total      This Month Last Month\n");
			fprintf(fp, "                     ---------- ---------- ----------\n");
			fprintf(fp, "    Msgs received    %-10ld %-10ld %-10ld\n", msgs.Received.total, msgs.Received.month[Miy], msgs.Received.month[LMiy]);
			fprintf(fp, "    Msgs posted      %-10ld %-10ld %-10ld\n", msgs.Posted.total, msgs.Posted.month[Miy], msgs.Posted.month[LMiy]);

		} else
			fseek(no, msgshdr.syssize, SEEK_CUR);
	}

	fclose(no);
	return page;
}


