/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Nodes Setup Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "grlist.h"
#include "stlist.h"
#include "m_global.h"
#include "m_lang.h"
#include "m_ticarea.h"
#include "m_marea.h"
#include "m_node.h"


int	NodeUpdated = 0;


/*
 * Count nr of nodes records in the database.
 * Creates the database if it doesn't exist.
 */
int CountNoderec(void);
int CountNoderec(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			nodeshdr.hdrsize = sizeof(nodeshdr);
			nodeshdr.recsize = sizeof(nodes);
			nodeshdr.filegrp = CFG.tic_groups * 13;
			nodeshdr.mailgrp = CFG.toss_groups * 13;
			fwrite(&nodeshdr, sizeof(nodeshdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 0;
		} else
			return -1;
	}

	fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
	fseek(fil, 0, SEEK_SET);
	fread(&nodeshdr, nodeshdr.hdrsize, 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - nodeshdr.hdrsize) / (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp);
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenNoderec(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	char	group[13];
	long	oldsize;
	int	i, old_fgroups, old_mgroups; 
	long	oldfilegrp, oldmailgrp;

	sprintf(fnin,  "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			NodeUpdated = 0;
			fread(&nodeshdr, sizeof(nodeshdr), 1, fin);
			fseek(fin, 0, SEEK_SET);
			fread(&nodeshdr, nodeshdr.hdrsize, 1, fin);
			if (nodeshdr.hdrsize != sizeof(nodeshdr)) {
				nodeshdr.hdrsize = sizeof(nodeshdr);
				nodeshdr.lastupd = time(NULL);
				NodeUpdated = 1;
			}

			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize    = nodeshdr.recsize;
			oldfilegrp = nodeshdr.filegrp;
			oldmailgrp = nodeshdr.mailgrp;
			old_fgroups = oldfilegrp / 13;
			old_mgroups = oldmailgrp / 13;
			if ((oldsize != sizeof(nodes) || 
			    (CFG.tic_groups != old_fgroups) || 
			    (CFG.toss_groups != old_mgroups))) {
				NodeUpdated = 1;
				if (oldsize != sizeof(nodes))
					Syslog('+', "Upgraded %s, format changed", fnin);
				else if (CFG.tic_groups != old_fgroups)
					Syslog('+', "Upgraded %s, nr of tic groups is now %d", fnin, CFG.tic_groups);
				else if (CFG.toss_groups != old_mgroups)
					Syslog('+', "Upgraded %s, nr of mail groups is now %d", fnin, CFG.toss_groups);
			}
			nodeshdr.hdrsize = sizeof(nodeshdr);
			nodeshdr.recsize = sizeof(nodes);
			nodeshdr.filegrp = CFG.tic_groups * 13;
			nodeshdr.mailgrp = CFG.toss_groups * 13;
			fwrite(&nodeshdr, sizeof(nodeshdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&nodes, 0, sizeof(nodes));
			while (fread(&nodes, oldsize, 1, fin) == 1) {
				fwrite(&nodes, sizeof(nodes), 1, fout);
				memset(&nodes, 0, sizeof(nodes));
				/*
				 * Copy the existing file groups
				 */
				for (i = 1; i <= old_fgroups; i++) {
					fread(&group, 13, 1, fin);
					if (i <= CFG.tic_groups)
						fwrite(&group, 13, 1, fout);
				}
				if (old_fgroups < CFG.tic_groups) {
					/*
					 * The size increased, fill with
					 * blank records
					 */
					memset(&group, 0, 13);
					for (i = (old_fgroups + 1); i <= CFG.tic_groups; i++)
						fwrite(&group, 13, 1, fout);
				}
				/*
				 * Copy the existing mail groups
				 */
				for (i = 1; i <= old_mgroups; i++) {
					fread(&group, 13, 1, fin);
					if (i <= CFG.toss_groups)
						fwrite(&group, 13, 1, fout);
				}
				if (old_mgroups < CFG.toss_groups) {
					memset(&group, 0, 13);
					for (i = (old_mgroups + 1); i <= CFG.toss_groups; i++)
						fwrite(&group, 13, 1, fout);
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



void CloseNoderec(int Force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	int	i;
	char	group[13];
	st_list	*nod = NULL, *tmp;

	sprintf(fin, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/nodes.temp", getenv("MBSE_ROOT"));

	if (NodeUpdated == 1) {
		if (Force || (yes_no((char *)"Nodes database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&nodeshdr, nodeshdr.hdrsize, 1, fi);
			fwrite(&nodeshdr, nodeshdr.hdrsize, 1, fo);

			while (fread(&nodes, nodeshdr.recsize, 1, fi) == 1) {
				if (!nodes.Deleted)
					fill_stlist(&nod, nodes.Sysop, ftell(fi) - nodeshdr.recsize);
				fseek(fi, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
			}
			sort_stlist(&nod);

			for (tmp = nod; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&nodes, nodeshdr.recsize, 1, fi);
				fwrite(&nodes, nodeshdr.recsize, 1, fo);
				for (i = 0; i < ((nodeshdr.filegrp + nodeshdr.mailgrp) / sizeof(group)); i++) {
					fread(&group, sizeof(group), 1, fi);
					fwrite(&group, sizeof(group), 1, fo);
				}
			}

			tidy_stlist(&nod);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"nodes.data\"");
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendNoderec(void);
int AppendNoderec(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	char	group[13];
	int	i;

	sprintf(ffile, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&nodes, 0, sizeof(nodes));
		/*
		 * Fill in the defaults
		 */
		nodes.Tic = TRUE;
		nodes.Notify = FALSE;
		nodes.AdvTic = FALSE;
		nodes.Hold = TRUE;
		nodes.ARCmailCompat = TRUE;
		nodes.ARCmailAlpha = TRUE;
		nodes.StartDate = time(NULL);
		fwrite(&nodes, sizeof(nodes), 1, fil);
		memset(&group, 0, 13);
		for (i = 1; i <= CFG.tic_groups; i++)
			fwrite(&group, 13, 1, fil);
		for (i = 1; i <= CFG.toss_groups; i++)
			fwrite(&group, 13, 1, fil);
		fclose(fil);
		NodeUpdated = 1;
		return 0;
	} else
		return -1;
}



int GroupInNode(char *Group, int Mail)
{
	char	temp[PATH_MAX], group[13];
	FILE	*no;
	int	i, groups, Area = 0, RetVal = 0;

	sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return FALSE;

	fread(&nodeshdr, sizeof(nodeshdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&nodeshdr, nodeshdr.hdrsize, 1, no);

	while ((fread(&nodes, nodeshdr.recsize, 1, no)) == 1) {
		Area++;
		groups = nodeshdr.filegrp / sizeof(group);
		for (i = 0; i < groups; i++) {
			fread(&group, sizeof(group), 1, no);
			if (strlen(group) && !Mail) {
				if (!strcmp(group, Group)) {
					RetVal++;
					Syslog('-', "File group %s in %d: %s", Group, Area, aka2str(nodes.Aka[0]));
				}
			}
		}
		groups = nodeshdr.mailgrp / sizeof(group);
		for (i = 0; i < groups; i++) {
			fread(&group, sizeof(group), 1, no);
			if (strlen(group) && Mail) {
				if (!strcmp(group, Group)) {
					RetVal++;
					Syslog('-', "Mail group %s found in %d: %s", Group, Area, aka2str(nodes.Aka[0]));
				}
			}
		}
	}

	fclose(no);
	return RetVal;
}



int CheckAka(fidoaddr);
int CheckAka(fidoaddr A)
{
	int	mcnt, tcnt;

	working(1, 0, 0);
	mcnt = NodeInMarea(A);
	tcnt = NodeInTic(A);
	working(0, 0, 0);
	if (mcnt || tcnt) {
		errmsg((char *)"Error aka connected to %d message and/or %d tic areas", mcnt, tcnt);
		return TRUE;
	}
	return FALSE;
}



void E_Mail(void);
void E_Mail(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "7.14  EDIT NODE - MAIL PROCESSING");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.  Session pwd");
	mvprintw( 8, 6, "2.  Check PKT pwd");
	mvprintw( 9, 6, "3.  UplMgr program");
	mvprintw(10, 6, "4.  UplMgr passwd");
	mvprintw(11, 6, "5.  Mail forward");
	mvprintw(12, 6, "6.  ARCmail comp.");
	mvprintw(13, 6, "7.  ARCmail a..z");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,25,15, (char *)"***************");
		show_bool(8,25, nodes.MailPwdCheck);
		show_str( 9,25, 8, nodes.UplAmgrPgm);
		show_str(10,25,15, (char *)"***************");
		show_bool(11,25, nodes.MailFwd);
		show_bool(12,25, nodes.ARCmailCompat);
		show_bool(13,25, nodes.ARCmailAlpha);

		switch(select_menu(7)) {
		case 0:	return;
		case 1:	E_STR(  7,25,15,nodes.Epasswd,       "The ^Session/Mail password^ for this node")
		case 2:	E_BOOL( 8,25,   nodes.MailPwdCheck,  "Check the ^mail PKT^ password")
		case 3:	E_STR(  9,25, 8,nodes.UplAmgrPgm,    "Name of the uplink ^areamanager program^")
		case 4:	E_STR( 10,25,15,nodes.UplAmgrPass,   "Uplink ^areamanager password^ for this node")
		case 5:	E_BOOL(11,25,   nodes.MailFwd,       "^Forward^ echomail for this node")
		case 6: E_BOOL(12,25,   nodes.ARCmailCompat, "Use ^ARCmail 0.60^ file naming convention for out of zone mail")
		case 7: E_BOOL(13,25,   nodes.ARCmailAlpha,  "Allow ^0..9 and a..z^ filename extensions for ARCmail archives")
		}
	}
}



void E_Files(void);
void E_Files(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "7.16  EDIT NODE - FILES PROCESSING");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.  Files password");
	mvprintw( 8, 6, "2.  Mgr password");
	mvprintw( 9, 6, "3.  UplMgr program");
	mvprintw(10, 6, "4.  UplMgr passwd");
	mvprintw(11, 6, "5.  UplMgr Add +");
	mvprintw(12, 6, "6.  Incl. message");
	mvprintw(13, 6, "7.  Send TIC file");
	mvprintw(14, 6, "8.  Advanced TIC");
	mvprintw(15, 6, "9.  File forward");
	mvprintw(16, 6, "10. Billing (CSO)");
	mvprintw( 7,46, "11. Bill direct");
	mvprintw( 8,46, "12. Credit");
	mvprintw( 9,46, "13. Debet");
	mvprintw(10,46, "14. Add %");
	mvprintw(11,46, "15. Warn level");
	mvprintw(12,46, "16. Stop level");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,25,15, (char *)"***************");
		show_str( 8,25,15, (char *)"***************");
		show_str( 9,25, 8, nodes.UplFmgrPgm);
		show_str(10,25,15, (char *)"***************");
		show_bool(11,25, nodes.AddPlus);
		show_bool(12,25, nodes.Message);
		show_bool(13,25, nodes.Tic);
		show_bool(14,25, nodes.AdvTic);
		show_bool(15,25, nodes.FileFwd);
		show_bool(16,25, nodes.Billing);
		show_bool( 7,65, nodes.BillDirect);
		show_int( 8,65, nodes.Credit);
		show_int( 9,65, nodes.Debet);
		show_int(10,65, nodes.AddPerc);
		show_int(11,65, nodes.WarnLevel);
		show_int(12,65, nodes.StopLevel);

		switch(select_menu(16)) {
		case 0:	return;
		case 1:	E_STR(  7,25,15,nodes.Fpasswd,    "The ^TIC^ files ^password^ for this node")
		case 2:	E_STR(  8,25,15,nodes.Apasswd,    "The filemanager ^password^ for this node")
		case 3:	E_STR(  9,25,8, nodes.UplFmgrPgm, "The name of the uplink ^filemanager^ program")
		case 4:	E_STR( 10,25,15,nodes.UplFmgrPass,"The uplink filemanager ^password^")
		case 5:	E_BOOL(11,25,   nodes.AddPlus,    "Add ^+^ in uplink manager requests for new areas")
		case 6:	E_BOOL(12,25,   nodes.Message,    "Send ^messages^ with files send to this node")
		case 7:	E_BOOL(13,25,   nodes.Tic,        "Send ^TIC^ files to this node")
		case 8:	E_BOOL(14,25,   nodes.AdvTic,     "Send ^advanced^ TIC files to this node")
		case 9:	E_BOOL(15,25,   nodes.FileFwd,    "^Forward TIC^ files for this node")
		case 10:E_BOOL(16,25,   nodes.Billing,    "Send ^bills^ to this node, Costsharing is active")
		case 11:E_BOOL( 7,65,   nodes.BillDirect, "Send bills ^direct^ after file processing")
		case 12:E_INT(  8,65,   nodes.Credit,     "The ^credit^ this node has for costsharing")
		case 13:E_INT(  9,65,   nodes.Debet,      "The ^debet^ in cents we have credit from this node")
		case 14:E_INT( 10,65,   nodes.AddPerc,    "The + or - ^promille^ factor for this node")
		case 15:E_INT( 11,65,   nodes.WarnLevel,  "Credit level in cents to ^Warn^ node for low credit")
		case 16:E_INT( 12,65,   nodes.StopLevel,  "Credit level in cents to ^Stop^ sending files")
		}
	}
}



void S_Stat(void);
void S_Stat(void)
{
	time_t		Now;
	struct tm	*t;
	int		LMiy;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 7, "7.18 NODE STATISTICS");
	set_color(CYAN, BLACK);
	mvprintw( 8,18, " This week  Last week This month Last month      Total");
	mvprintw( 9,18, "---------- ---------- ---------- ---------- ----------");
	mvprintw(10,6, "Files sent");
	mvprintw(11,6, "Kbytes sent");
	mvprintw(12,6, "Files rcvd");
	mvprintw(13,6, "Kbytes rcvd");
	mvprintw(14,6, "Mail sent");
	mvprintw(15,6, "Mail rcvd");
	set_color(WHITE, BLACK);

	Now = time(NULL);
	t = localtime(&Now);

	Diw = t->tm_wday;
	Miy = t->tm_mon;
	if (Miy == 0)
		LMiy = 11;
	else
		LMiy = Miy - 1;

	mvprintw(10,18, (char *)"%10lu %10lu %10lu %10lu %10lu", nodes.FilesSent.tweek, nodes.FilesSent.lweek, nodes.FilesSent.month[Miy], nodes.FilesSent.month[LMiy], nodes.FilesSent.total);
	mvprintw(11,18, (char *)"%10lu %10lu %10lu %10lu %10lu", nodes.F_KbSent.tweek, nodes.F_KbSent.lweek, nodes.F_KbSent.month[Miy], nodes.F_KbSent.month[LMiy], nodes.F_KbSent.total);
	mvprintw(12,18, (char *)"%10lu %10lu %10lu %10lu %10lu", nodes.FilesRcvd.tweek, nodes.FilesRcvd.lweek, nodes.FilesRcvd.month[Miy], nodes.FilesRcvd.month[LMiy], nodes.FilesRcvd.total);
	mvprintw(13,18, (char *)"%10lu %10lu %10lu %10lu %10lu", nodes.F_KbRcvd.tweek, nodes.F_KbRcvd.lweek, nodes.F_KbRcvd.month[Miy], nodes.F_KbRcvd.month[LMiy], nodes.F_KbRcvd.total);
	mvprintw(14,18, (char *)"%10lu %10lu %10lu %10lu %10lu", nodes.MailSent.tweek, nodes.MailSent.lweek, nodes.MailSent.month[Miy], nodes.MailSent.month[LMiy], nodes.MailSent.total);
	mvprintw(15,18, (char *)"%10lu %10lu %10lu %10lu %10lu", nodes.MailRcvd.tweek, nodes.MailRcvd.lweek, nodes.MailRcvd.month[Miy], nodes.MailRcvd.month[LMiy], nodes.MailRcvd.total);
	set_color(CYAN, BLACK);
	center_addstr(LINES - 4, (char *)"Press any key");
	readkey(LINES - 4, COLS / 2 + 8, LIGHTGRAY, BLACK);
}



fidoaddr e_a(fidoaddr, int);
fidoaddr e_a(fidoaddr n, int x)
{
	FILE	*fil;
	char	temp[PATH_MAX];
	int	i;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, (char *)"7.%d EDIT AKA", x);
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.  Zone");
		mvprintw( 8, 6, "2.  Net");
		mvprintw( 9, 6, "3.  Node");
		mvprintw(10, 6, "4.  Point");
		mvprintw(11, 6, "    Domain");
		set_color(WHITE, BLACK);
		show_int( 7,17, n.zone);
		show_int( 8,17, n.net);
		show_int( 9,17, n.node);
		show_int(10,17, n.point);
		show_str(11,17,12, n.domain);

		switch(select_menu(4)) {
		case 0:	return n;
		case 1:	n.zone = edit_int(7, 17, n.zone, (char *)"The ^zone^ number 1..4095");
			sprintf(temp, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&fidonethdr, sizeof(fidonethdr), 1, fil);

				while (fread(&fidonet, fidonethdr.recsize, 1, fil) == 1) {
					if (fidonet.available) {
						for (i = 0; i < 6; i++) {
							if (fidonet.zone[i] == n.zone) {
								memset(&n.domain, 0, sizeof(n.domain));
								strcpy(n.domain, fidonet.domain);
							}
						}
					}
				}
				fclose(fil);
			}
			break;
		case 2:	E_INT( 8,17,n.net,  "The ^net^ number 1..65535")
		case 3:	E_INT( 9,17,n.node, "The ^node^ number 1..65535")
		case 4:	E_INT(10,17,n.point,"The ^point^ number 0..65535")
		}
	}
}



void N_Akas(void)
{
	int	i, j, m, x, y;
	fidoaddr a[20];

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "7.2  EDIT NODES AKA'S");
		set_color(CYAN, BLACK);
		y = 7; x = 6;
		for (i = 0; i < 20; i++) {
			if (i == 10) {
				y = 7; x = 46;
			}
			mvprintw( y, x, (char *)"%d.", i + 1);
			if (nodes.Aka[i].zone)
				mvprintw(y, x + 5, (char *)"%s", aka2str(nodes.Aka[i]));
			y++;
		}

		m = select_menu(20);
		switch(m) {
			case 0:
				/*
				 * Pack the aka's
				 */
				for (i = 0; i < 20; i++) {
					a[i] = nodes.Aka[i];
					memset(&nodes.Aka[i], 0, sizeof(fidoaddr));
				}
				j = 0;
				for (i = 0; i < 20; i++) {
					if (a[i].zone) {
						nodes.Aka[j] = a[i];
						j++;
					}
				}
				return;

			default:
				if (! CheckAka(nodes.Aka[m - 1]))
					nodes.Aka[m - 1] = e_a(nodes.Aka[m - 1], 2);
				break;
		}

	}
}



void NScreen(void);
void NScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "7.  EDIT NODE");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Sysop name");
	mvprintw( 8, 2, "2.  Fido aka's");
	mvprintw( 9, 2, "3.  Dial command");
	mvprintw(10, 2, "4.  Phone number 1");
	mvprintw(11, 2, "5.  Phone number 2");
	mvprintw(12, 2, "6.  Route via");
	mvprintw(13, 2, "7.  Netmail direct");
	mvprintw(14, 2, "8.  Netmail crash");
	mvprintw(15, 2, "9.  Netmail hold");
	mvprintw(16, 2, "10. Pack netmail");
	mvprintw(17, 2, "11. Send notify");
	mvprintw(18, 2, "12. Language");
	mvprintw(19, 2, "13. Deleted");

	mvprintw(14,31, "14. Mail setup");
	mvprintw(15,31, "15. Mail groups");
	mvprintw(16,31, "16. File setup");
	mvprintw(17,31, "17. File groups");
	mvprintw(18,31, "18. Statistics");
	mvprintw(19,31, "19. No EMSI");

	mvprintw(10,51, "20. No YooHoo/2U2");
	mvprintw(11,51, "21. No Filerequest");
	mvprintw(12,51, "22. Don't call");
	mvprintw(13,51, "23. No Hold mail");
	mvprintw(14,51, "24. Pickup primary");
	mvprintw(15,51, "25. No Zmodem");
	mvprintw(16,51, "26. No Zedzap");
	mvprintw(17,51, "27. No Hydra");
	mvprintw(18,51, "28. No TCP/IP");
	mvprintw(19,51, "29. 8.3 names");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditNodeRec(int);
int EditNodeRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX];
	long		offset;
	unsigned long	crc, crc1;
	gr_list		*fgr = NULL, *egr = NULL, *tmp;
	char		group[13];
	int		count, groups, i, j, GrpChanged = FALSE;
	char		*temp, temp1[32];

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Fido Node");

	sprintf(mfile, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) != NULL) {
		fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);

		while (fread(&mgroup, mgrouphdr.recsize, 1, fil) == 1)
			fill_grlist(&egr, mgroup.Name);

		fclose(fil);
		sort_grlist(&egr);
	}

	sprintf(mfile, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) != NULL) {
		fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);

		while (fread(&fgroup, fgrouphdr.recsize, 1, fil) == 1)
			fill_grlist(&fgr, fgroup.Name);

		fclose(fil);
		sort_grlist(&fgr);
	}

	sprintf(mfile, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		tidy_grlist(&egr);
		tidy_grlist(&fgr);
		return -1;
	}

	fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
	offset = nodeshdr.hdrsize + ((Area -1) * (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		tidy_grlist(&egr);
		tidy_grlist(&fgr);
		return -1;
	} 

	fread(&nodes, nodeshdr.recsize, 1, fil);
	groups = nodeshdr.filegrp / 13;

	for (i = 0; i < groups; i++) {
		fread(&group, sizeof(group), 1, fil);
		if (strlen(group)) {
			for (tmp = fgr; tmp; tmp = tmp->next)
				if (!strcmp(tmp->group, group))
					tmp->tagged = TRUE;
		}
	}

	groups = nodeshdr.mailgrp / 13;

	for (i = 0; i < groups; i++) {
		fread(&group, sizeof(group), 1, fil);
		if (strlen(group)) {
			for (tmp = egr; tmp; tmp = tmp->next)
				if (!strcmp(tmp->group, group))
					tmp->tagged = TRUE;
		}
	}

	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&nodes, crc, nodeshdr.recsize);
	working(0, 0, 0);
	NScreen();
	
	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,21,35, nodes.Sysop);
		temp = xstrcpy((char *)"");
		for (i = 0; i < 7; i++) {
			if (nodes.Aka[i].zone) {
				if (nodes.Aka[i].point)
					sprintf(temp1, "%d:%d/%d.%d ", nodes.Aka[i].zone, nodes.Aka[i].net, nodes.Aka[i].node, nodes.Aka[i].point);
				else
					sprintf(temp1, "%d:%d/%d ", nodes.Aka[i].zone, nodes.Aka[i].net, nodes.Aka[i].node);
				temp = xstrcat(temp, temp1);
			}
		}
		show_str( 8,21,58, temp);
		free(temp);
		show_str( 9,21,40, nodes.dial);
		show_str(10,21,20, nodes.phone[0]);
		show_str(11,21,20, nodes.phone[1]);
		if (nodes.RouteVia.zone)
			show_aka(12,21,nodes.RouteVia);
		show_bool(13,21, nodes.Direct);
		show_bool(14,21, nodes.Crash);
		show_bool(15,21, nodes.Hold);
		show_bool(16,21, nodes.PackNetmail);
		show_bool(17,21, nodes.Notify);
		sprintf(temp1, "%c", nodes.Language);
		show_str(18,21,1, temp1);
		show_bool(19,21, nodes.Deleted);
		show_bool(19,47, nodes.NoEMSI);
		show_bool(10,70, nodes.NoWaZOO);
		show_bool(11,70, nodes.NoFreqs);
		show_bool(12,70, nodes.NoCall);
		show_bool(13,70, nodes.NoHold);
		show_bool(14,70, nodes.NoPUA);
		show_bool(15,70, nodes.NoZmodem);
		show_bool(16,70, nodes.NoZedzap);
		show_bool(17,70, nodes.NoHydra);
		show_bool(18,70, nodes.NoTCP);
		show_bool(19,70, nodes.FNC);

		switch(select_menu(29)) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&nodes, crc1, nodeshdr.recsize);
			if ((crc != crc1) || GrpChanged) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					Syslog('+', "Updated node record %s", nodes.Sysop);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&nodes, nodeshdr.recsize, 1, fil);

					groups = nodeshdr.filegrp / 13;
					i = 0;
					for (tmp = fgr; tmp; tmp = tmp->next)
						if (tmp->tagged) {
							i++;
							memset(&group, 0, 13);
							sprintf(group, "%s", tmp->group);
							fwrite(&group, 13, 1, fil);
						}

					memset(&group, 0, sizeof(group));
					for (j = i; j < groups; j++)
						fwrite(&group, 13, 1, fil);

					groups = nodeshdr.mailgrp / 13;
					i = 0;
					for (tmp = egr; tmp; tmp = tmp->next)
						if (tmp->tagged) {
							i++;
							memset(&group, 0, 13);
							sprintf(group, "%s", tmp->group);
							fwrite(&group, 13, 1, fil);
						}

					memset(&group, 0, sizeof(group));
					for (j = i; j < groups; j++)
						fwrite(&group, 13, 1, fil);

					fclose(fil);
					NodeUpdated = 1;
					working(0, 0, 0);
				}
			}
			tidy_grlist(&egr);
			tidy_grlist(&fgr);
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR(7,21,35, nodes.Sysop,     "The name of the ^sysop^ for this node")
		case 2:	N_Akas(); NScreen(); break;
		case 3: E_STR( 9,21,40, nodes.dial,     "If needed, give a special modem ^dial command^ for this node")
		case 4: E_STR(10,21,20, nodes.phone[0], "Enter ^phone number^ to override the nodelist")
		case 5: E_STR(11,21,20, nodes.phone[1], "Enter ^phone number^ to override the nodelist")
		case 6:	nodes.RouteVia = e_a(nodes.RouteVia, 6); NScreen(); break;
		case 7:	E_BOOL(13,21, nodes.Direct,     "Set the ^direct^ flag on netmail")
		case 8:	nodes.Crash = edit_bool(14,21, nodes.Crash, (char *)"Set the ^crash^ flags for this node");
			if (nodes.Crash)
				nodes.Hold = FALSE;
			break;
		case 9:	nodes.Hold = edit_bool(15,21, nodes.Hold, (char *)"Set the ^hold^ flag for this node");
			if (nodes.Hold)
				nodes.Crash = FALSE;
			break;
		case 10:E_BOOL(16,21, nodes.PackNetmail, "^Pack netmail^ for this node")
		case 11:E_BOOL(17,21, nodes.Notify,  "Send ^notify^ messages to this node")
		case 12:nodes.Language = PickLanguage((char *)"7.12"); NScreen(); break;
		case 13:count = 0;
			working(1, 0, 0);
			for (i = 0; i < 20; i++)
				if (nodes.Aka[i].zone)
					count += NodeInMarea(nodes.Aka[i]);
			if (count) {
				working(0, 0, 0);
				errmsg((char *)"Node is connected to %d message areas", count);
				break;
			}
			count = 0;
			for (i = 0; i < 20; i++)
				if (nodes.Aka[i].zone)
					count += NodeInTic(nodes.Aka[i]);
			working(0, 0, 0);
			if (count) {
				errmsg((char *)"Node is connected to %d tic areas", count);
				break;
			}
			E_BOOL(19,21, nodes.Deleted, "Is this node ^Deleted^")
		case 14:E_Mail(); NScreen(); break;
		case 15:if (E_Group(&egr, (char *)"7.15  MAIL GROUPS"))
				GrpChanged = TRUE;
			NScreen(); break;
		case 16:E_Files();
			NScreen(); break;
		case 17:if (E_Group(&fgr, (char *)"7.17  FILE GROUPS"))
				GrpChanged = TRUE;
			NScreen(); break;
		case 18:S_Stat(); NScreen(); break;
		case 19:E_BOOL(19,47, nodes.NoEMSI,   "Disable ^EMSI handshake^ with this node")
		case 20:E_BOOL(10,70, nodes.NoWaZOO,  "Disable ^YooHoo/2U2 handshake^ (FTSC-0006) with this node")
		case 21:E_BOOL(11,70, nodes.NoFreqs,  "Disallow ^file requests^ from this node")
		case 22:E_BOOL(12,70, nodes.NoCall,   "Don't ^call^ this node")
		case 23:E_BOOL(13,70, nodes.NoHold,   "Don't ^hold hold-mail^ when we call (no = only pickup)")
		case 24:E_BOOL(14,70, nodes.NoPUA,    "Only pickup mail from the ^primary^ address")
		case 25:E_BOOL(15,70, nodes.NoZmodem, "Disable ^Zmodem^ protocol with this node")
		case 26:E_BOOL(16,70, nodes.NoZedzap, "Disable ^Zedzap^ protocol with this node")
		case 27:E_BOOL(17,70, nodes.NoHydra,  "Disable ^Hydra^ protocol with this node")
		case 28:E_BOOL(18,70, nodes.NoTCP,    "Disable ^TCP/IP^ protocol whith this node")
		case 29:E_BOOL(19,70, nodes.FNC,      "Node needs ^DOS 8.3^ filenames")
		}
	}
}



void EditNodes(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountNoderec();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenNoderec() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "7.  NODES SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(nodeshdr) + (((o + i) - 1) * (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp));
						fseek(fil, offset, 0);
						fread(&nodes, nodeshdr.recsize, 1, fil);
						if (strlen(nodes.Sysop))
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK); 
						sprintf(temp, "%3d.  %s (%s)", o + i, nodes.Sysop, strtok(aka2str(nodes.Aka[0]), "@"));
						temp[37] = 0;
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseNoderec(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendNoderec() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < records) 
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditNodeRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitNodes(void)
{
    CountNoderec();
    OpenNoderec();
    CloseNoderec(TRUE);
}



fidoaddr PullUplink(char *Hdr)
{
	static fidoaddr	uplink;
	int		records, m, i, o, x, y;
	char		pick[12];
	FILE		*fil;
	char		temp[PATH_MAX];
	long		offset;

	memset(&uplink, 0, sizeof(uplink));
	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return uplink;
	}

	records = CountNoderec();
	if (records == -1) {
		working(2, 0, 0);
		return uplink;
	}

	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "%s.  UPLINK SELECT", Hdr);
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(nodeshdr) + (((o + i) - 1) * (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp));
						fseek(fil, offset, 0);
						fread(&nodes, nodeshdr.recsize, 1, fil);
						if (strlen(nodes.Sysop))
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK); 
						sprintf(temp, "%3d.  %s (%s)", o + i, nodes.Sysop, strtok(aka2str(nodes.Aka[0]), "@"));
						temp[37] = 0;
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_pick(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			return uplink;
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < records) 
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
				fseek(fil, ((atoi(pick) -1) * (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp)) + nodeshdr.hdrsize, SEEK_SET);
				if (fread(&nodes, nodeshdr.recsize, 1, fil) != 1) {
					fclose(fil);
					return uplink;
				}
				fclose(fil);

				for (;;) {
					clr_index();
					set_color(WHITE, BLACK);
					mvprintw( 5, 6, "%s.  SELECT NODE AKA", Hdr);
					set_color(CYAN, BLACK);
					y = 7; x = 6;
					for (i = 0; i < 20; i++) {
						if (i == 10) {
							y = 7; x = 46;
						}
						mvprintw( y, x, (char *)"%d.", i + 1);
						if (nodes.Aka[i].zone)
							mvprintw(y, x + 5, (char *)"%s", aka2str(nodes.Aka[i]));
						y++;
					}

					m = select_menu(20);
					switch(m) {
						case 0:
							return uplink;
			
						default:
							uplink = nodes.Aka[m - 1];
							return uplink;
					}

				}
			}
			return uplink;
		}
	}
}



int node_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[PATH_MAX];
	FILE	*no;
	int	groups, i, First = TRUE;
	char	group[13];

	sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	fread(&nodeshdr, sizeof(nodeshdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&nodeshdr, nodeshdr.hdrsize, 1, no);

	while ((fread(&nodes, nodeshdr.recsize, 1, no)) == 1) {

		page = newpage(fp, page);

		if (First) {
			addtoc(fp, toc, 7, 0, page, (char *)"Fidonet nodes");
			First = FALSE;
			fprintf(fp, "\n");
		} else
			fprintf(fp, "\n\n");

		fprintf(fp, "     Sysop         %s\n", nodes.Sysop);
		fprintf(fp, "     First date    %s", ctime(&nodes.StartDate));
		fprintf(fp, "     Last date     %s\n", ctime(&nodes.LastDate));
		for (i = 0; i < 20; i++)
			if (nodes.Aka[i].zone)
				fprintf(fp, "     Aka %2d        %s\n", i+1, aka2str(nodes.Aka[i]));
		if (nodes.RouteVia.zone)
			fprintf(fp, "     Route via     %s\n", aka2str(nodes.RouteVia));

		fprintf(fp, "     Language      %c\n", nodes.Language);
		fprintf(fp, "     Files passwd  %s\n", nodes.Fpasswd);
		fprintf(fp, "     Session pwd   %s\n", nodes.Epasswd);
		fprintf(fp, "     Areamgr pwd   %s\n\n", nodes.Apasswd);

		fprintf(fp, "     Uplink mgrs   Program   Password\n");
		fprintf(fp, "     ------------  --------- ---------------\n");
		fprintf(fp, "     Files         %s %s\n", padleft(nodes.UplFmgrPgm, 9, ' '), nodes.UplFmgrPass);
		fprintf(fp, "     Mail          %s %s\n\n", padleft(nodes.UplAmgrPgm, 9, ' '), nodes.UplAmgrPass);

		fprintf(fp, "     Mail direct   %s", getboolean(nodes.Direct));
		fprintf(fp, "     Mail crash    %s", getboolean(nodes.Crash));
		fprintf(fp, "     Mail hold     %s\n", getboolean(nodes.Hold));
		fprintf(fp, "     Send message  %s", getboolean(nodes.Message));
		fprintf(fp, "     Send .TIC     %s", getboolean(nodes.Tic));
		fprintf(fp, "     Send notify   %s\n", getboolean(nodes.Notify));
		fprintf(fp, "     File forward  %s", getboolean(nodes.FileFwd));
		fprintf(fp, "     Mail forward  %s", getboolean(nodes.MailFwd));
		fprintf(fp, "     Advanced TIC  %s\n", getboolean(nodes.AdvTic));
		fprintf(fp, "     Billing       %s", getboolean(nodes.Billing));
		fprintf(fp, "     Bill direct   %s", getboolean(nodes.BillDirect));
		fprintf(fp, "     Uplink add +  %s\n", getboolean(nodes.AddPlus));
		fprintf(fp, "     Check mailpwd %s", getboolean(nodes.MailPwdCheck));
		fprintf(fp, "     No EMSI       %s", getboolean(nodes.NoEMSI));
		fprintf(fp, "     No YooHoo/2U2 %s\n", getboolean(nodes.NoWaZOO));
		fprintf(fp, "     No Requests   %s", getboolean(nodes.NoFreqs));
		fprintf(fp, "     Don't call    %s", getboolean(nodes.NoCall));
		fprintf(fp, "     No hold mail  %s\n", getboolean(nodes.NoHold));
		fprintf(fp, "     No Pickup all %s", getboolean(nodes.NoPUA));
		fprintf(fp, "     No Zmodem     %s", getboolean(nodes.NoZmodem));
		fprintf(fp, "     No Zedzap     %s\n", getboolean(nodes.NoZedzap));
		fprintf(fp, "     No Hydra      %s", getboolean(nodes.NoHydra));
		fprintf(fp, "     No TCP/IP     %s", getboolean(nodes.NoTCP));
		fprintf(fp, "     Pack Netmail  %s\n", getboolean(nodes.PackNetmail));
		fprintf(fp, "     ARCmail comp. %s", getboolean(nodes.ARCmailCompat));
		fprintf(fp, "     ACRmail a..z  %s", getboolean(nodes.ARCmailAlpha));
		fprintf(fp, "     8.3 filenames %s\n\n", getboolean(nodes.FNC));

		fprintf(fp, "     Statistics    Send     KBytes   Received KBytes\n");
		fprintf(fp, "     ------------  -------- -------- -------- --------\n");
		fprintf(fp, "     Total files   %-8lu %-8lu %-8lu %-8lu\n", nodes.FilesSent.total, nodes.F_KbSent.total, nodes.FilesRcvd.total, nodes.F_KbSent.total);
		fprintf(fp, "     Total mail    %-8lu          %-8lu\n", nodes.MailSent.total, nodes.MailRcvd.total);

		fprintf(fp, "     Credit units  %-8ld   Warnlevel  %ld\n", nodes.Credit, nodes.WarnLevel);
		fprintf(fp, "     Debet units   %-8ld   Stoplevel  %ld\n", nodes.Debet, nodes.StopLevel);
		fprintf(fp, "     Add promille  %ld\n", nodes.AddPerc);
		fprintf(fp, "\n     File groups:\n     ");
		groups = nodeshdr.filegrp / sizeof(group);
		for (i = 0; i < groups; i++) {
			fread(&group, sizeof(group), 1, no);
			if (strlen(group)) {
				fprintf(fp, "%-12s ", group);
				if (((i+1) % 5) == 0)
					fprintf(fp, "\n     ");
			}
		}
		if ((i+1) % 5)
			fprintf(fp, "\n");
		fprintf(fp, "\n     Mail groups:\n     ");
		groups = nodeshdr.mailgrp / sizeof(group);
		for (i = 0; i < groups; i++) {
			fread(&group, sizeof(group), 1, no);
			if (strlen(group)) {
				fprintf(fp, "%-12s ", group);
				if (((i+1) % 5) == 0)
					fprintf(fp, "\n     ");
			}
		}
		fprintf(fp, "\n");
	}

	fclose(no);
	return page;
}


