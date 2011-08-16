/*****************************************************************************
 *
 * $Id: m_node.c,v 1.69 2007/02/12 13:45:09 mbse Exp $
 * Purpose ...............: Nodes Setup Program 
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
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "grlist.h"
#include "stlist.h"
#include "m_global.h"
#include "m_lang.h"
#include "m_ticarea.h"
#include "m_marea.h"
#include "m_archive.h"
#include "m_node.h"


int	NodeUpdated = 0;


/*
 * Count nr of nodes records in the database.
 * Creates the database if it doesn't exist.
 */
int CountNoderec(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];
    int	    count;

    snprintf(ffile, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
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
    FILE    *fin, *fout;
    char    *fnin, *fnout, group[13];
    int	    i, old_fgroups, old_mgroups; 
    int	    oldsize, oldfilegrp, oldmailgrp;

    fnin  = calloc(PATH_MAX, sizeof(char));
    fnout = calloc(PATH_MAX, sizeof(char));
    snprintf(fnin,  PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));

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
	    if ((oldsize != sizeof(nodes) || (CFG.tic_groups != old_fgroups) || (CFG.toss_groups != old_mgroups))) {
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
	     * The datarecord is filled with zero's before each read,
	     * so if the format changed, the new fields will be empty.
	     */
	    memset(&nodes, 0, sizeof(nodes));
	    while (fread(&nodes, oldsize, 1, fin) == 1) {
		if (oldsize != sizeof(nodes)) {
		    if (strlen(nodes.Spasswd) == 0)
			strcpy(nodes.Spasswd, nodes.Epasswd);
		    if (nodes.Security.level == 0) {
			/*
			 * Level is not used, here it is used to mark
			 * the upgrade to default.
			 */
			nodes.Security.level = 1;
			nodes.Security.flags = 1;
		    }
		    if (strlen(nodes.Archiver) == 0) {
			snprintf(nodes.Archiver, 6, (char *)"ZIP");
		    }
		}
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
		     * The size increased, fill with blank records
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
	    free(fnin);
	    free(fnout);
	    return 0;
	} else
	    return -1;
    }

    free(fnin);
    free(fnout);
    return -1;
}



void CloseNoderec(int Force)
{
    char	    *fin, *fout, group[13];
    FILE	    *fi, *fo;
    int		    i;
    st_list	    *nod = NULL, *tmp;
    unsigned int    crc1, crc2;

    fin  = calloc(PATH_MAX, sizeof(char));
    fout = calloc(PATH_MAX, sizeof(char));
    snprintf(fin,  PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    snprintf(fout, PATH_MAX, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));

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
		else {
		    /*
		     * Remove obsolete paths
		     */
		    unlink(nodes.Dir_out_path);
		    unlink(nodes.Dir_in_path);
		    unlink(nodes.OutBox);
		}
		fseek(fi, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
	    }
	    sort_stlist(&nod);

	    crc1 = crc2 = 0xffffffff;
	    for (tmp = nod; tmp; tmp = tmp->next) {
		fseek(fi, tmp->pos, SEEK_SET);
		fread(&nodes, nodeshdr.recsize, 1, fi);
		crc2 = upd_crc32((char *)&nodes, crc2, 100);
		if (crc2 == crc1)
		    WriteError("Removing double noderecord %s %s", nodes.Sysop, aka2str(nodes.Aka[0]));
		else {
		    fwrite(&nodes, nodeshdr.recsize, 1, fo);
		    for (i = 0; i < ((nodeshdr.filegrp + nodeshdr.mailgrp) / sizeof(group)); i++) {
			fread(&group, sizeof(group), 1, fi);
			fwrite(&group, sizeof(group), 1, fo);
		    }
		}
		crc1 = crc2;
		crc2 = 0xffffffff;
	    }

	    tidy_stlist(&nod);
	    fclose(fi);
	    fclose(fo);
	    unlink(fout);
	    chmod(fin, 0640);
	    free(fin);
	    free(fout);
	    Syslog('+', "Updated \"nodes.data\"");
	    disk_reset();
	    CreateSema((char *)"scanout");
	    working(6, 0, 0);
	    return;
	}
    }

    chmod(fin, 0640);
    unlink(fout);
    working(1, 0, 0);
    free(fin);
    free(fout);
}



int AppendNoderec(void);
int AppendNoderec(void)
{
    FILE    *fil;
    char    *ffile, group[13];
    int	    i;

    ffile = calloc(PATH_MAX, sizeof(char));
    snprintf(ffile, PATH_MAX, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));

    if ((fil = fopen(ffile, "a")) != NULL) {
	memset(&nodes, 0, sizeof(nodes));
	/*
	 * Fill in the defaults
	 */
	nodes.Tic = TRUE;
	nodes.Notify = FALSE;
	nodes.AdvTic = FALSE;
	nodes.Hold = TRUE;
	nodes.PackNetmail = TRUE;
	nodes.ARCmailCompat = TRUE;
	nodes.ARCmailAlpha = TRUE;
	nodes.StartDate = time(NULL);
	nodes.Security.level = 1;
	nodes.Security.flags = 1;
	nodes.Language = 'E';
	snprintf(nodes.Archiver, 6, "ZIP");
	fwrite(&nodes, sizeof(nodes), 1, fil);
	memset(&group, 0, 13);
	for (i = 1; i <= CFG.tic_groups; i++)
	    fwrite(&group, 13, 1, fil);
	for (i = 1; i <= CFG.toss_groups; i++)
	    fwrite(&group, 13, 1, fil);
	fclose(fil);
	NodeUpdated = 1;
	free(ffile);
	return 0;
    }

    free(ffile);
    return -1;
}



int GroupInNode(char *Group, int Mail)
{
    char    *temp, group[13];
    FILE    *no;
    int	    i, groups, RetVal = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL) {
	free(temp);
	return 0;
    }
    free(temp);

    fread(&nodeshdr, sizeof(nodeshdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&nodeshdr, nodeshdr.hdrsize, 1, no);

    while ((fread(&nodes, nodeshdr.recsize, 1, no)) == 1) {
	groups = nodeshdr.filegrp / sizeof(group);
	for (i = 0; i < groups; i++) {
	    fread(&group, sizeof(group), 1, no);
	    if (strlen(group) && !Mail) {
		if (!strcmp(group, Group)) {
		    RetVal++;
		    Syslog('-', "File group %s found in node setup %s", Group, aka2str(nodes.Aka[0]));
		}
	    }
	}
	groups = nodeshdr.mailgrp / sizeof(group);
	for (i = 0; i < groups; i++) {
	    fread(&group, sizeof(group), 1, no);
	    if (strlen(group) && Mail) {
		if (!strcmp(group, Group)) {
		    RetVal++;
		    Syslog('-', "Mail group %s found in node setup %s", Group, aka2str(nodes.Aka[0]));
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
    if (mcnt || tcnt) {
	errmsg((char *)"Error aka connected to %d message and/or %d tic areas", mcnt, tcnt);
	return TRUE;
    }

    return FALSE;
}



void E_UplMgr(void);
void E_UplMgr(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "7.10 EDIT NODE - UPLINK MANAGERS");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.   Uplink AreaMgr program");
    mbse_mvprintw( 8, 6, "2.   Uplink AreaMgr password");
    mbse_mvprintw( 9, 6, "3.   Uplink AreaMgr is BBBS");
    mbse_mvprintw(10, 6, "4.   Uplink FileMgr program");
    mbse_mvprintw(11, 6, "5.   Uplink FileMgr password");
    mbse_mvprintw(12, 6, "6.   Uplink FileMgr is BBBS");
    mbse_mvprintw(13, 6, "7.   Our Area/Filemgr passwd");

    for (;;) {
	set_color(WHITE, BLACK);
        show_str(  7,35, 8, nodes.UplAmgrPgm);
	show_str(  8,35,15, (char *)"***************");
	show_bool( 9,35,    nodes.UplAmgrBbbs);
        show_str( 10,35, 8, nodes.UplFmgrPgm);
	show_str( 11,35,15, (char *)"***************");
	show_bool(12,35,    nodes.UplFmgrBbbs);
	show_str( 13,35,15, (char *)"***************");

	switch(select_menu(7)) {
	    case 0: return;
	    case 1: E_STR(  7,35, 8, nodes.UplAmgrPgm,   "Name of the uplink ^areamanager program^")
	    case 2: E_STR(  8,35,15, nodes.UplAmgrPass,  "Uplink ^areamanager password^ for this node")
	    case 3: E_BOOL( 9,35,    nodes.UplAmgrBbbs,  "Uplink ^areamanager^ is ^BBBS^ software")
            case 4: E_STR( 10,35,8,  nodes.UplFmgrPgm,   "Name of the uplink ^filemanager^ program")
	    case 5: E_STR( 11,35,15, nodes.UplFmgrPass,  "Uplink ^filemanager password^ for this node")
	    case 6: E_BOOL(12,35,    nodes.UplFmgrBbbs,  "Uplink ^filemanager^ is ^BBBS^ software")
	    case 7: E_STR( 13,35,15, nodes.Apasswd,      "The area and filemanager ^password^ for this node")
	}
    }
}



void E_Mail(void);
void E_Mail(void)
{
    char    temp[6];
    int	    show = TRUE;

    for (;;) {
	if (show) {
	    clr_index();
	    set_color(WHITE, BLACK);
	    mbse_mvprintw( 5, 6, "7.4  EDIT NODE - MAIL PROCESSING");
	    set_color(CYAN, BLACK);
	    mbse_mvprintw( 7, 6, "1.   PKT password");
	    mbse_mvprintw( 8, 6, "2.   Check PKT pwd");
	    mbse_mvprintw( 9, 6, "3.   Mail forward");
	    mbse_mvprintw(10, 6, "4.   ARCmail comp.");
	    mbse_mvprintw(11, 6, "5.   ARCmail a..z");
	    mbse_mvprintw(12, 6, "6.   Archiver");
	    show = FALSE;
	}

	set_color(WHITE, BLACK);
	show_str(  7,25,15, (char *)"***************");
	show_bool( 8,25,    nodes.MailPwdCheck);
	show_bool( 9,25,    nodes.MailFwd);
	show_bool(10,25,    nodes.ARCmailCompat);
	show_bool(11,25,    nodes.ARCmailAlpha);
	show_str( 12,25, 5, nodes.Archiver);

	switch(select_menu(6)) {
	    case 0: return;
	    case 1: E_STR(  7,25,15, nodes.Epasswd,       "The ^Mail (.pkt)^ password^ for this node")
	    case 2: E_BOOL( 8,25,    nodes.MailPwdCheck,  "Check the ^mail PKT^ password")
	    case 3: E_BOOL( 9,25,    nodes.MailFwd,       "^Forward^ echomail for this node")
	    case 4: E_BOOL(10,25,    nodes.ARCmailCompat, "Use ^ARCmail 0.60^ file naming convention for out of zone mail")
	    case 5: E_BOOL(11,25,    nodes.ARCmailAlpha,  "Allow ^0..9 and a..z^ filename extensions for ARCmail archives")
	    case 6: strcpy(temp, PickArchive((char *)"7.4", TRUE));
		    if (strlen(temp) != 0)
			strcpy(nodes.Archiver, temp);
		    show = TRUE;
		    break;
	}
    }
}



void E_Files(void);
void E_Files(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "7.6  EDIT NODE - FILES PROCESSING");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.   Files password");
    mbse_mvprintw( 8, 6, "2.   Incl. message");
    mbse_mvprintw( 9, 6, "3.   Send TIC file");
    mbse_mvprintw(10, 6, "4.   Advanced TIC");
    mbse_mvprintw(11, 6, "5.   Advanced SB");
    mbse_mvprintw(12, 6, "6.   To line in TIC");
    mbse_mvprintw(13, 6, "7.   File forward");
    mbse_mvprintw(14, 6, "8.   TIC 4d address");

    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  7,26,15, (char *)"***************");
	show_bool( 8,26,    nodes.Message);
	show_bool( 9,26,    nodes.Tic);
	show_bool(10,26,    nodes.AdvTic);
	show_bool(11,26,    nodes.TIC_AdvSB);
	show_bool(12,26,    nodes.TIC_To);
	show_bool(13,26,    nodes.FileFwd);
	show_bool(14,26,    nodes.Tic4d);

	switch(select_menu(8)) {
	    case 0: return;
	    case 1: E_STR(  7,26,15,nodes.Fpasswd,    "The ^TIC^ files ^password^ for this node")
	    case 2: E_BOOL( 8,26,   nodes.Message,    "Send ^messages^ with files send to this node")
	    case 3: E_BOOL( 9,26,   nodes.Tic,        "Send ^TIC^ files to this node")
	    case 4: E_BOOL(10,26,   nodes.AdvTic,     "Send ^advanced^ TIC files to this node")
	    case 5: E_BOOL(11,26,   nodes.TIC_AdvSB,  "Send ^advanced Seen-By^ lines in ticfiles to this node")
	    case 6: E_BOOL(12,26,   nodes.TIC_To,     "Send ^To^ line in ticfiles to this node")
	    case 7: E_BOOL(13,26,   nodes.FileFwd,    "^Forward TIC^ files for this node")
	    case 8: E_BOOL(14,26,   nodes.Tic4d,      "Use ^4d addresses^ in TIC files for this node")
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
	mbse_mvprintw( 5, 6, "7.11 NODE STATISTICS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 8,18, " This week  Last week This month Last month      Total");
	mbse_mvprintw( 9,18, "---------- ---------- ---------- ---------- ----------");
	mbse_mvprintw(10,6, "Files sent");
	mbse_mvprintw(11,6, "Kbytes sent");
	mbse_mvprintw(12,6, "Files rcvd");
	mbse_mvprintw(13,6, "Kbytes rcvd");
	mbse_mvprintw(14,6, "Mail sent");
	mbse_mvprintw(15,6, "Mail rcvd");
	set_color(WHITE, BLACK);

	Now = time(NULL);
	t = localtime(&Now);

	Diw = t->tm_wday;
	Miy = t->tm_mon;
	if (Miy == 0)
		LMiy = 11;
	else
		LMiy = Miy - 1;

	mbse_mvprintw(10,18, (char *)"%10u %10u %10u %10u %10u", nodes.FilesSent.tweek, 
		nodes.FilesSent.lweek, nodes.FilesSent.month[Miy], nodes.FilesSent.month[LMiy], nodes.FilesSent.total);
	mbse_mvprintw(11,18, (char *)"%10u %10u %10u %10u %10u", nodes.F_KbSent.tweek, 
		nodes.F_KbSent.lweek, nodes.F_KbSent.month[Miy], nodes.F_KbSent.month[LMiy], nodes.F_KbSent.total);
	mbse_mvprintw(12,18, (char *)"%10u %10u %10u %10u %10u", nodes.FilesRcvd.tweek, 
		nodes.FilesRcvd.lweek, nodes.FilesRcvd.month[Miy], nodes.FilesRcvd.month[LMiy], nodes.FilesRcvd.total);
	mbse_mvprintw(13,18, (char *)"%10u %10u %10u %10u %10u", nodes.F_KbRcvd.tweek, 
		nodes.F_KbRcvd.lweek, nodes.F_KbRcvd.month[Miy], nodes.F_KbRcvd.month[LMiy], nodes.F_KbRcvd.total);
	mbse_mvprintw(14,18, (char *)"%10u %10u %10u %10u %10u", nodes.MailSent.tweek, 
		nodes.MailSent.lweek, nodes.MailSent.month[Miy], nodes.MailSent.month[LMiy], nodes.MailSent.total);
	mbse_mvprintw(15,18, (char *)"%10u %10u %10u %10u %10u", nodes.MailRcvd.tweek, 
		nodes.MailRcvd.lweek, nodes.MailRcvd.month[Miy], nodes.MailRcvd.month[LMiy], nodes.MailRcvd.total);
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
		mbse_mvprintw( 5, 6, (char *)"7.%d EDIT AKA", x);
		set_color(CYAN, BLACK);
		mbse_mvprintw( 7, 6, "1.  Zone");
		mbse_mvprintw( 8, 6, "2.  Net");
		mbse_mvprintw( 9, 6, "3.  Node");
		mbse_mvprintw(10, 6, "4.  Point");
		mbse_mvprintw(11, 6, "    Domain");
		mbse_mvprintw(12, 6, "5.  Delete");
		set_color(WHITE, BLACK);
		show_int( 7,17, n.zone);
		show_int( 8,17, n.net);
		show_int( 9,17, n.node);
		show_int(10,17, n.point);
		show_str(11,17,12, n.domain);

		switch(select_menu(5)) {
		case 0:	return n;
		case 1:	n.zone = edit_int_range(7, 17, n.zone, 1, 4095, (char *)"The ^zone^ number 1..4095");
			snprintf(temp, PATH_MAX, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
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
		case 2:	E_IRC( 8,17,n.net,   1, 32767, "The ^net^ number 1..32767")
		case 3:	E_IRC( 9,17,n.node,  0, 32767, "The ^node^ number 0..32767")
		case 4:	E_IRC(10,17,n.point, 0, 32767, "The ^point^ number 0..32767")
		case 5: memset(&n, 0, sizeof(n));
			break;
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
		mbse_mvprintw( 5, 6, "7.2  EDIT NODES AKA'S");
		set_color(CYAN, BLACK);
		y = 7; x = 6;
		for (i = 0; i < 20; i++) {
			if (i == 10) {
				y = 7; x = 46;
			}
			mbse_mvprintw( y, x, (char *)"%d.", i + 1);
			if (nodes.Aka[i].zone)
				mbse_mvprintw(y, x + 5, (char *)"%s", aka2str(nodes.Aka[i]));
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



void GeneralScreen(void);
void GeneralScreen(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "7.1  EDIT NODE GENERAL");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.   Sysop name");
    mbse_mvprintw( 8, 2, "2.   Outbox dir");
    mbse_mvprintw( 9, 2, "3.   Pvt. phone");
    mbse_mvprintw(10, 2, "4.   Pvt. fax");
    mbse_mvprintw(11, 2, "5.   Pvt. Cellphone");
    mbse_mvprintw(12, 2, "6.   Pvt. e-mail");
    mbse_mvprintw(13, 2, "7.   Pvt. remark");
    mbse_mvprintw(14, 2, "8.   Route via");
    mbse_mvprintw(15, 2, "9.   Netmail direct");
    mbse_mvprintw(16, 2, "10.  Netmail crash");
    mbse_mvprintw(17, 2, "11.  Netmail hold");
    mbse_mvprintw(18, 2, "12.  Pack mail");

    mbse_mvprintw(16,42, "13.  Send notify");
    mbse_mvprintw(17,42, "14.  Language");
    mbse_mvprintw(18,42, "15.  Deleted");
}



void GeneralEdit(void);
void GeneralEdit(void)
{
    int	    i, count;
    char    temp1[32];

    GeneralScreen();
    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,23,35, nodes.Sysop);
	show_str( 8,23,55, nodes.OutBox);
	show_str( 9,23,16, nodes.Ct_phone);
	show_str(10,23,16, nodes.Ct_fax);
	show_str(11,23,20, nodes.Ct_cellphone);
	show_str(12,23,30, nodes.Ct_email);
	show_str(13,23,55, nodes.Ct_remark);
	if (nodes.RouteVia.zone)
	    show_aka(14,23,nodes.RouteVia);
	show_bool(15,23, nodes.Direct);
	show_bool(16,23, nodes.Crash);
	show_bool(17,23, nodes.Hold);
	show_bool(18,23, nodes.PackNetmail);

	show_bool(16,63, nodes.Notify);
	snprintf(temp1, 32, "%c", nodes.Language);
	show_str(17,63,1, temp1);
	show_bool(18,63, nodes.Deleted);

	switch(select_menu(15)) {
	case 0: return;
	case 1: E_STR( 7,23,35, nodes.Sysop,        "The name of the ^sysop^ for this node")
	case 2: if (strlen(nodes.OutBox) == 0) {
		    if (nodes.Aka[0].zone) {
			snprintf(nodes.OutBox, 65, "%s/var/boxes/node%d_%d_%d", getenv("MBSE_ROOT"), 
				nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node);
		    } else {
			snprintf(nodes.OutBox, 65, "%s/var/boxes/%s", getenv("MBSE_ROOT"), nodes.Sysop);
			for (i = (strlen(nodes.OutBox) - strlen(nodes.Sysop)); i < strlen(nodes.OutBox); i++) {
			    nodes.OutBox[i] = tolower(nodes.OutBox[i]);
			    if (nodes.OutBox[i] == ' ')
				nodes.OutBox[i] = '_';
			}
		    }
		}
		E_PTH( 8,23,55, nodes.OutBox,       "Private extra ^outbound directory^ for this node", 0770)
	case 3: E_STR( 9,23,16, nodes.Ct_phone,     "Contact info: ^private phone number^")
	case 4: E_STR(10,23,16, nodes.Ct_fax,       "Contact info: ^private fax number^")
	case 5: E_STR(11,23,20, nodes.Ct_cellphone, "Contact info: ^private cellphone/GSM^")
	case 6: E_STR(12,23,30, nodes.Ct_email,     "Contact info: ^private e-mail address^")
	case 7: E_STR(13,23,55, nodes.Ct_remark,    "Contact info: ^private remark^")
	case 8: nodes.RouteVia = e_a(nodes.RouteVia, 6); GeneralScreen(); break;
	case 9: E_BOOL(15,23, nodes.Direct,     "Set the ^direct^ flag on netmail")
	case 10:nodes.Crash = edit_bool(16,23, nodes.Crash, (char *)"Set the ^crash^ flags for this node");
		if (nodes.Crash)
		    nodes.Hold = FALSE;
		break;
	case 11:nodes.Hold = edit_bool(17,23, nodes.Hold, (char *)"Set the ^hold^ flag for this node");
                if (nodes.Hold)
                    nodes.Crash = FALSE;
                break;
	case 12:E_BOOL(18,23, nodes.PackNetmail, "^Pack mail^ for this node")
	case 13:E_BOOL(16,63, nodes.Notify,  "Send ^notify^ messages to this node")
	case 14:i = PickLanguage((char *)"7.1.14");
		if (i != '\0')
		    nodes.Language = i;
		GeneralScreen(); 
		break;
	case 15:count = 0;
		working(1, 0, 0);
		for (i = 0; i < 20; i++)
		    if (nodes.Aka[i].zone)
			count += NodeInMarea(nodes.Aka[i]);
		if (count) {
		    errmsg((char *)"Node is connected to %d message areas", count);
		    break;
		}
		count = 0;
		for (i = 0; i < 20; i++)
		    if (nodes.Aka[i].zone)
			count += NodeInTic(nodes.Aka[i]);
		if (count) {
		    errmsg((char *)"Node is connected to %d tic areas", count);
		    break;
		}
		E_BOOL(18,63, nodes.Deleted, "Is this node ^Deleted^")
	}
    }
}



void SessionScreen(void);
void SessionScreen(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "7.3  EDIT NODE SESSION");
    set_color(CYAN, BLACK);

    mbse_mvprintw( 7, 6, "1.   Session passwd");
    mbse_mvprintw( 8, 6, "2.   Dial command");
    mbse_mvprintw( 9, 6, "3.   Phone number 1");
    mbse_mvprintw(10, 6, "4.   Phone number 2");
    mbse_mvprintw(11, 6, "5.   Nodelist flags");
    mbse_mvprintw(12, 6, "6.   Inet hostname");
    mbse_mvprintw(13, 6, "7.   Outbound sess.");
    mbse_mvprintw(14, 6, "8.   Inbound sess.");
    mbse_mvprintw(15, 6, "9.   No EMSI");
    mbse_mvprintw(16, 6, "10.  No YooHoo/2U2");
    mbse_mvprintw(17, 6, "11.  No Filerequest");
    mbse_mvprintw(18, 6, "12.  Don't call");
    mbse_mvprintw(19, 6, "13.  8.3 names");
    mbse_mvprintw(20, 6, "14.  NR mode");

    mbse_mvprintw(13,41, "15.  No PLZ");
    mbse_mvprintw(14,41, "16.  No GZ/BZ2");
    mbse_mvprintw(15,41, "17.  No Zmodem");
    mbse_mvprintw(16,41, "18.  No Zedzap");
    mbse_mvprintw(17,41, "19.  No Hydra");
    mbse_mvprintw(18,41, "20.  Binkp old esc");
    mbse_mvprintw(19,41, "21.  No binkp/1.1");
    mbse_mvprintw(20,41, "22.  Ign. Hold");
}



void SessionEdit(void);
void SessionEdit(void)
{
    SessionScreen();

    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  7,26,15, (char *)"***************");
	show_str(  8,26,40, nodes.dial);
	show_str(  9,26,20, nodes.phone[0]);
	show_str( 10,26,20, nodes.phone[1]);
	show_str( 11,26,54, nodes.Nl_flags);
	show_str( 12,26,40, nodes.Nl_hostname);
	show_sessiontype(13,26,nodes.Session_out);
	show_sessiontype(14,26,nodes.Session_in);
	show_bool(15,26,    nodes.NoEMSI);
	show_bool(16,26,    nodes.NoWaZOO);
	show_bool(17,26,    nodes.NoFreqs);
	show_bool(18,26,    nodes.NoCall);
	show_bool(19,26,    nodes.FNC);
	show_bool(20,26,    nodes.DoNR);

	show_bool(13,61,    nodes.NoPLZ);
	show_bool(14,61,    nodes.NoGZ);
	show_bool(15,61,    nodes.NoZmodem);
	show_bool(16,61,    nodes.NoZedzap);
	show_bool(17,61,    nodes.NoHydra);
	show_bool(18,61,    nodes.WrongEscape);
	show_bool(19,61,    nodes.NoBinkp11);
	show_bool(20,61,    nodes.IgnHold);

	switch(select_menu(22)) {
	case 0: return;
	case 1: E_STR(  7,26,15, nodes.Spasswd,     "The ^Session password^ for this node")
	case 2: E_STR(  8,26,40, nodes.dial,        "If needed, give a special modem ^dial command^ for this node")
	case 3: E_STR(  9,26,20, nodes.phone[0],    "Enter ^phone number^ to override the nodelist")
	case 4: E_STR( 10,26,20, nodes.phone[1],    "Enter ^phone number^ to override the nodelist")
	case 5: E_STR( 11,26,54, nodes.Nl_flags,    "^Nodelist flags^ override")
	case 6: E_STR( 12,26,40, nodes.Nl_hostname, "Node internet ^hostname/IP address^ override")
	case 7: nodes.Session_out = edit_sessiontype(13,26, nodes.Session_out);
		break;
	case 8: nodes.Session_in =  edit_sessiontype(14,26, nodes.Session_in);
		break;
	case 9: E_BOOL(15,26,    nodes.NoEMSI,      "Disable ^EMSI handshake^ with this node")
	case 10:E_BOOL(16,26,    nodes.NoWaZOO,     "Disable ^YooHoo/2U2 handshake^ (FTSC-0006) with this node")
	case 11:E_BOOL(17,26,    nodes.NoFreqs,     "Disallow ^file requests^ from this node")
	case 12:E_BOOL(18,26,    nodes.NoCall,      "Don't ^call^ this node")
	case 13:E_BOOL(19,26,    nodes.FNC,         "Node needs ^DOS 8.3^ filenames")
	case 14:E_BOOL(20,26,    nodes.DoNR,	    "Use ^NR-mode^ in outgoing binkp sessions")
		
	case 15:E_BOOL(13,61,    nodes.NoPLZ,       "Disable ^Binkp PLZ^ compression with this node")
	case 16:E_BOOL(14,61,    nodes.NoGZ,        "Disable ^Binkp GZ and BZ2^ compression with this node")
	case 17:E_BOOL(15,61,    nodes.NoZmodem,    "Disable ^Zmodem^ protocol with this node")
	case 18:E_BOOL(16,61,    nodes.NoZedzap,    "Disable ^Zedzap^ protocol with this node")
	case 19:E_BOOL(17,61,    nodes.NoHydra,     "Disable ^Hydra^ protocol with this node")
	case 20:E_BOOL(18,61,    nodes.WrongEscape, "Use the ^old escape^ for long filenames (Argus, Irex)")
	case 21:E_BOOL(19,61,    nodes.NoBinkp11,   "Disable ^binkp/1.1^ (fallback to binkp/1.0) mode for this node")
	case 22:E_BOOL(20,61,    nodes.IgnHold,     "Ignore node ^Hold or Down^ nodelist status")
	}
    }
}



void DirectoryScreen(void);
void DirectoryScreen(void)
{   
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "7.8  EDIT NODE DIRECTORY SESSION");
    mbse_mvprintw( 7, 2, "     Outbound settings");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 8, 2, "1.   Files path");
    mbse_mvprintw( 9, 2, "2.   Check for lock");
    mbse_mvprintw( 9,41, "3.   Wait clear lock");
    mbse_mvprintw(10, 2, "4.   Check lockfile");
    mbse_mvprintw(11, 2, "5.   Create lock");
    mbse_mvprintw(12, 2, "6.   Create lockfile");

    set_color(WHITE, BLACK);
    mbse_mvprintw(14, 2, "     Inbound settings");
    set_color(CYAN, BLACK);
    mbse_mvprintw(15, 2, "7.   Files path");
    mbse_mvprintw(16, 2, "8.   Check for lock");
    mbse_mvprintw(16,41, "9.   Wait clear lock");
    mbse_mvprintw(17, 2, "10.  Check lockfile");
    mbse_mvprintw(18, 2, "11.  Create lock");
    mbse_mvprintw(19, 2, "12.  Create lockfile");
}



void DirectoryEdit(void);
void DirectoryEdit(void)
{
    int	    pick, temp;
    char    *p;

    DirectoryScreen();
	    
    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  8,23,56, nodes.Dir_out_path);
	show_bool( 9,23,    nodes.Dir_out_chklck);
	show_bool( 9,62,    nodes.Dir_out_waitclr);
	show_str( 10,23,56, nodes.Dir_out_clock);
	show_bool(11,23,    nodes.Dir_out_mklck);
	show_str( 12,23,56, nodes.Dir_out_mlock);

	show_str( 15,23,56, nodes.Dir_in_path);
	show_bool(16,23,    nodes.Dir_in_chklck);
	show_bool(16,62,    nodes.Dir_in_waitclr);
	show_str( 17,23,56, nodes.Dir_in_clock);
	show_bool(18,23,    nodes.Dir_in_mklck);
	show_str( 19,23,56, nodes.Dir_in_mlock);

	pick = select_menu(12);
	if (pick == 0)
	    return;

	if (pick < 7) {
	    if (nodes.Session_out != S_DIR) {
		errmsg("Outbound session is not Directory");
	    } else {
		switch(pick) {
		case 1:	if ((strlen(nodes.Dir_out_path) == 0) && (nodes.Aka[0].zone)) {
			    snprintf(nodes.Dir_out_path, 65, "%s/var/bbsftp/node%d_%d_%d/outbound", getenv("MBSE_ROOT"),
				nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node);
			}
			E_PTH( 8,23,56, nodes.Dir_out_path,    "^Outbound path^ for files and mail to this node", 0770)
		case 2: temp = edit_bool(9,23, nodes.Dir_out_chklck, (char *)"^Check^ outbound lockfile");
			if (temp && !nodes.Dir_out_chklck && (strlen(nodes.Dir_out_clock) == 0)) {
			    strcpy(nodes.Dir_out_clock, nodes.Dir_out_path);
			    p = strrchr(nodes.Dir_out_clock, '/');
			    if (p) {
				p++;
				*p = '\0';
				snprintf(p, 9, "lock.bsy");
			    }
			}
			nodes.Dir_out_chklck = temp;
			break;
		case 3: E_BOOL( 9,62,   nodes.Dir_out_waitclr, "^Wait^ for lockfile to ^clear^")
		case 4: E_STR(10,23,56, nodes.Dir_out_clock, "^Lockfile^ to check before adding files")
		case 5: temp = edit_bool(11,23, nodes.Dir_out_mklck, (char *)"^Create^ lockfile before adding files");
			if (temp && !nodes.Dir_out_mklck && (strlen(nodes.Dir_out_mlock) == 0)) {
			    strcpy(nodes.Dir_out_mlock, nodes.Dir_out_path);
			    p = strrchr(nodes.Dir_out_mlock, '/');
			    if (p) {
				p++;
				*p = '\0';
				snprintf(p, 9, "lock.bsy");
			    }
			}
			nodes.Dir_out_mklck = temp;
			break;
		case 6: E_STR(12,23,56, nodes.Dir_out_mlock, "^Lockfile^ to create while adding files")
		}
	    }
	} else {
	    if (nodes.Session_in != S_DIR) {
		errmsg("Inbound session is not Directory");
	    } else {
		switch(pick) {
                case 7: if ((strlen(nodes.Dir_in_path) == 0) && (nodes.Aka[0].zone)) {
                            snprintf(nodes.Dir_in_path, 65, "%s/var/bbsftp/node%d_%d_%d/inbound", getenv("MBSE_ROOT"),
                                nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node);
                        }
                        E_PTH(15,23,56, nodes.Dir_in_path,    "^Inbound path^ for files and mail from this node", 0770)
                case 8: temp = edit_bool(16,23, nodes.Dir_in_chklck, (char *)"^Check^ inbound lockfile");
                        if (temp && !nodes.Dir_in_chklck && (strlen(nodes.Dir_in_clock) == 0)) {
                            strcpy(nodes.Dir_in_clock, nodes.Dir_in_path);
                            p = strrchr(nodes.Dir_in_clock, '/');
                            if (p) {
                                p++;
                                *p = '\0';
                                snprintf(p, 9, "lock.bsy");
                            }
                        }
                        nodes.Dir_in_chklck = temp;
                        break;
                case 9: E_BOOL(16,62,   nodes.Dir_in_waitclr, "^Wait^ for lockfile to ^clear^")
		case 10:E_STR(17,23,56, nodes.Dir_in_clock, "^Lockfile^ to check before getting files")
		case 11:temp = edit_bool(18,23, nodes.Dir_in_mklck, (char *)"^Create^ lockfile before getting files");
                        if (temp && !nodes.Dir_in_mklck && (strlen(nodes.Dir_in_mlock) == 0)) {
                            strcpy(nodes.Dir_in_mlock, nodes.Dir_in_path);
                            p = strrchr(nodes.Dir_in_mlock, '/');
                            if (p) {
                                p++;
                                *p = '\0';
                                snprintf(p, 9, "lock.bsy");
                            }
                        }
                        nodes.Dir_in_mklck = temp;
                        break;
		case 12:E_STR(19,23,56, nodes.Dir_in_mlock, "^Lockfile^ to create while getting files")
		}
	    }
	}
    }
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditNodeRec(int);
int EditNodeRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX];
    int		    offset;
    unsigned int    crc, crc1;
    gr_list	    *fgr = NULL, *egr = NULL, *tmp;
    char	    group[13];
    int		    groups, i, j, GrpChanged = FALSE;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Fido Node");

    snprintf(mfile, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) != NULL) {
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);

	while (fread(&mgroup, mgrouphdr.recsize, 1, fil) == 1)
	    fill_grlist(&egr, mgroup.Name);

	fclose(fil);
	sort_grlist(&egr);
    }

    snprintf(mfile, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) != NULL) {
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);

	while (fread(&fgroup, fgrouphdr.recsize, 1, fil) == 1)
	    fill_grlist(&fgr, fgroup.Name);

	fclose(fil);
	sort_grlist(&fgr);
    }

    snprintf(mfile, PATH_MAX, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));
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
	
    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
        mbse_mvprintw( 5, 6, "7.   EDIT NODE -  %s, %s", nodes.Sysop, aka2str(nodes.Aka[0]));
        set_color(CYAN, BLACK);
        mbse_mvprintw( 7, 6, "1.   General setup");
	mbse_mvprintw( 8, 6, "2.   Aka's setup");
	mbse_mvprintw( 9, 6, "3.   Session setup");
	mbse_mvprintw(10, 6, "4.   Mail setup");
	mbse_mvprintw(11, 6, "5.   Mail groups");
	mbse_mvprintw(12, 6, "6.   Files setup");
	mbse_mvprintw(13, 6, "7.   Files groups");
	mbse_mvprintw(14, 6, "8.   Directory session");
	mbse_mvprintw(15, 6, "9.   Security flags");
	mbse_mvprintw(16, 6, "10.  Area/File managers");
	mbse_mvprintw(17, 6, "11.  Statistics");

	switch(select_menu(11)) {
	case 0:	if (((nodes.Session_out == S_DIR) && (strlen(nodes.Dir_out_path) == 0)) ||
		    ((nodes.Session_in == S_DIR) && (strlen(nodes.Dir_in_path) == 0))) {
		    errmsg((char *)"Set a path for directory sessions screen 7.8");
		    break;
		}
		crc1 = 0xffffffff;
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
				snprintf(group, 13, "%s", tmp->group);
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
				snprintf(group, 13, "%s", tmp->group);
				fwrite(&group, 13, 1, fil);
			    }

			memset(&group, 0, sizeof(group));
			for (j = i; j < groups; j++)
			    fwrite(&group, 13, 1, fil);

			fclose(fil);
			NodeUpdated = 1;
			working(6, 0, 0);
		    }
		}
		tidy_grlist(&egr);
		tidy_grlist(&fgr);
		IsDoing("Browsing Menu");
		return 0;
	case 1: GeneralEdit();
		break;
	case 2: N_Akas();
		break;
	case 3: SessionEdit();
		break;
	case 4: E_Mail(); 
		break;
	case 5: if (E_Group(&egr, (char *)"7.5  MAIL GROUPS"))
		    GrpChanged = TRUE;
		break;
	case 6: E_Files();
		break;
	case 7: if (E_Group(&fgr, (char *)"7.7  FILE GROUPS"))
		    GrpChanged = TRUE;
		break;
	case 8: DirectoryEdit();
		break;
	case 9:	nodes.Security = edit_nsec(nodes.Security, (char *)"7.9  NODE SECURITY FLAGS");
		break;
	case 10:E_UplMgr();
		break;
	case 11:S_Stat(); 
		break;
	}
    }
}



void EditNodes(void)
{
    int	    records, i, o, x, y;
    char    pick[12], temp[PATH_MAX];
    FILE    *fil;
    int	    offset;

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
    o = 0;
    if (! check_free())
	return;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "7.  NODES SETUP");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/nodes.temp", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
		x = 4;
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
			snprintf(temp, 81, "%3d.  %s (%s)", o + i, nodes.Sysop, strtok(aka2str(nodes.Aka[0]), "@"));
			temp[37] = 0;
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_record(records, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseNoderec(FALSE);
	    open_bbs();
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    if ((records < CFG.toss_systems) && (records < CFG.tic_systems)) {
		working(1, 0, 0);
		if (AppendNoderec() == 0) {
		    records++;
		    working(1, 0, 0);
		} else
		    working(2, 0, 0);
	    } else {
		if ((records + 1) > CFG.toss_systems) {
		    errmsg("Cannot add node, change global setting in menu 1.11.11");
		} else {
		    errmsg("Cannot add node, change global setting in menu 1.10.4");
		}
	    }
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
	int		offset;

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
		mbse_mvprintw( 5, 4, "%s.  UPLINK SELECT", Hdr);
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
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
						snprintf(temp, 81, "%3d.  %s (%s)", o + i, nodes.Sysop, strtok(aka2str(nodes.Aka[0]), "@"));
						temp[37] = 0;
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
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
			snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
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
					mbse_mvprintw( 5, 6, "%s.  SELECT NODE AKA", Hdr);
					set_color(CYAN, BLACK);
					y = 7; x = 6;
					for (i = 0; i < 20; i++) {
						if (i == 10) {
							y = 7; x = 46;
						}
						mbse_mvprintw( y, x, (char *)"%d.", i + 1);
						if (nodes.Aka[i].zone)
							mbse_mvprintw(y, x + 5, (char *)"%s", aka2str(nodes.Aka[i]));
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
    FILE	*ti, *wp, *ip, *no;
    int		systems, groups, nr, refs, i, j, k, First = TRUE;
    char	group[13];
    sysconnect	System;
    time_t	tt;

    snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    fread(&nodeshdr, sizeof(nodeshdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&nodeshdr, nodeshdr.hdrsize, 1, no);

    ip = open_webdoc((char *)"nodes.html", (char *)"Fidonet Nodes", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Node</TH><TH align='left'>Sysop</TH><TH align='left'>Flags</TH></TR>\n");

    while ((fread(&nodes, nodeshdr.recsize, 1, no)) == 1) {

	page = newpage(fp, page);

	if (First) {
	    addtoc(fp, toc, 7, 0, page, (char *)"Fidonet nodes");
	    First = FALSE;
	    fprintf(fp, "\n");
	} else
	    fprintf(fp, "\n\n");

	snprintf(temp, 81, "node_%d_%d_%d_%d_%s.html", nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node,
		nodes.Aka[0].point, nodes.Aka[0].domain);
	fprintf(ip, " <TR><TD><A HREF=\"%s\">%s</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, aka2str(nodes.Aka[0]), nodes.Sysop, nodes.Crash ? "Crash": nodes.Hold ? "Hold":"Normal");
	
	if ((wp = open_webdoc(temp, (char *)"Fidonet node", aka2str(nodes.Aka[0])))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"nodes.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Sysop", nodes.Sysop);
	    fprintf(fp, "     Sysop            %s\n", nodes.Sysop);
	    if (strlen(nodes.OutBox)) {
		fprintf(fp, "     Outbox dir       %s\n", nodes.OutBox);
		add_webtable(wp, (char *)"Outbox directory", nodes.OutBox);
	    }
	    tt = (time_t)nodes.StartDate;
	    fprintf(fp, "     First date       %s", ctime(&tt));
	    add_webtable(wp, (char *)"First date", ctime(&tt));
	    tt = (time_t)nodes.LastDate;
	    fprintf(fp, "     Last date        %s", ctime(&tt));
	    add_webtable(wp, (char *)"Last date", ctime(&tt));
	    for (i = 0; i < 20; i++)
		if (nodes.Aka[i].zone) {
		    fprintf(fp, "     Aka %2d           %s\n", i+1, aka2str(nodes.Aka[i]));
		    snprintf(temp, 81, "Aka %d", i+1);
		    add_webtable(wp, temp, aka2str(nodes.Aka[i]));
		}
	    if (nodes.RouteVia.zone) {
		fprintf(fp, "     Route via        %s\n", aka2str(nodes.RouteVia));
		add_webtable(wp, (char *)"Route via", aka2str(nodes.RouteVia));
	    }
	    fprintf(fp, "     Session pwd      %s\n", nodes.Spasswd);
	    add_webtable(wp, (char *)"Session password", nodes.Spasswd);
	    if (strlen(nodes.dial)) {
		fprintf(fp, "     Dial command     %s\n", nodes.dial);
		add_webtable(wp, (char *)"Dial command", nodes.dial);
	    }
	    if (strlen(nodes.phone[0]) || strlen(nodes.phone[1])) {
	        fprintf(fp, "     Phone numbers    %s %s\n", nodes.phone[0], nodes.phone[1]);
		snprintf(temp, 81, "%s %s", nodes.phone[0], nodes.phone[1]);
		add_webtable(wp, (char *)"Phone numbers", temp);
	    }
	    if (strlen(nodes.Nl_flags)) {
	        fprintf(fp, "     Nodelist flags   %s\n", nodes.Nl_flags);
		add_webtable(wp, (char *)"Nodelist flags", nodes.Nl_flags);
	    }
	    if (strlen(nodes.Nl_hostname)) {
	        fprintf(fp, "     Hostname         %s\n", nodes.Nl_hostname);
		add_webtable(wp, (char *)"Hostname", nodes.Nl_hostname);
	    }
	    fprintf(fp, "     PKT password     %s\n", nodes.Epasswd);
	    add_webtable(wp, (char *)"PKT password", nodes.Epasswd);
	    fprintf(fp, "     Files passwd     %s\n", nodes.Fpasswd);
	    add_webtable(wp, (char *)"Files passwd", nodes.Fpasswd);
	    fprintf(fp, "     Areamgr pwd      %s\n\n", nodes.Apasswd);
	    add_webtable(wp, (char *)"Areamgr pwd", nodes.Apasswd);
	    fprintf(wp, "<TR><TD colspan='2'>&nbsp;</TD></TR>\n");

	    fprintf(fp, "     Mail direct      %s", getboolean(nodes.Direct));
	    add_webtable(wp, (char *)"Mail direct", getboolean(nodes.Direct));
	    fprintf(fp, "     Mail crash       %s", getboolean(nodes.Crash));
	    add_webtable(wp, (char *)"Mail crash", getboolean(nodes.Crash));
	    fprintf(fp, "     Mail hold        %s\n", getboolean(nodes.Hold));
	    add_webtable(wp, (char *)"Mail hold", getboolean(nodes.Hold));
	    fprintf(fp, "     Pack mail        %s", getboolean(nodes.PackNetmail));
	    add_webtable(wp, (char *)"Pack mail", getboolean(nodes.PackNetmail));
	    fprintf(fp, "     Send notify      %s", getboolean(nodes.Notify));
	    add_webtable(wp, (char *)"Send notify messages", getboolean(nodes.Notify));
	    fprintf(fp, "     Language         %c\n", nodes.Language);
	    snprintf(temp, 81, "%c", nodes.Language);
	    add_webtable(wp, (char *)"Language", temp);
	    fprintf(fp, "     No EMSI          %s", getboolean(nodes.NoEMSI));
	    add_webtable(wp, (char *)"No EMSI", getboolean(nodes.NoEMSI));
	    fprintf(fp, "     No YooHoo/2U2    %s", getboolean(nodes.NoWaZOO));
	    add_webtable(wp, (char *)"No YooHoo/2U2", getboolean(nodes.NoWaZOO));
	    fprintf(fp, "     No Filerequests  %s\n", getboolean(nodes.NoFreqs));
	    add_webtable(wp, (char *)"No File Requests", getboolean(nodes.NoFreqs));
	    fprintf(fp, "     Don't call       %s", getboolean(nodes.NoCall));
	    add_webtable(wp, (char *)"Don't call", getboolean(nodes.NoCall));
	    fprintf(fp, "     8.3 filenames    %s", getboolean(nodes.FNC));
	    add_webtable(wp, (char *)"8.3 filenames", getboolean(nodes.FNC));
	    fprintf(fp, "     Use NR mode      %s\n", getboolean(nodes.DoNR));
	    add_webtable(wp, (char *)"Use NR mode", getboolean(nodes.DoNR));
	    fprintf(fp, "     No PLZ           %s", getboolean(nodes.NoPLZ));
	    add_webtable(wp, (char *)"No PLZ compression", getboolean(nodes.NoPLZ));
	    fprintf(fp, "     No GZ/BZ2        %s", getboolean(nodes.NoGZ));
	    add_webtable(wp, (char *)"No GZ/BZ2 compression", getboolean(nodes.NoGZ));
	    fprintf(fp, "     No Zmodem        %s\n", getboolean(nodes.NoZmodem));
	    add_webtable(wp, (char *)"No Zmodem", getboolean(nodes.NoZmodem));
	    fprintf(fp, "     No Zedzap        %s", getboolean(nodes.NoZedzap));
	    add_webtable(wp, (char *)"No Zedzap", getboolean(nodes.NoZedzap));
	    fprintf(fp, "     No Hydra         %s", getboolean(nodes.NoHydra));
	    add_webtable(wp, (char *)"No Hydra", getboolean(nodes.NoHydra));
	    fprintf(fp, "     binkp old esc    %s\n", getboolean(nodes.WrongEscape));
	    add_webtable(wp, (char *)"Binkp old esc method", getboolean(nodes.WrongEscape));
	    fprintf(fp, "     No binkp/1.1     %s", getboolean(nodes.NoBinkp11));
	    add_webtable(wp, (char *)"No binkp/1.1 sessions", getboolean(nodes.NoBinkp11));
	    fprintf(fp, "     Ignore Hold/Down %s", getboolean(nodes.IgnHold));
	    add_webtable(wp, (char *)"Ignore Hold/Down", getboolean(nodes.IgnHold));

	    fprintf(fp, "     Mail forward     %s\n", getboolean(nodes.MailFwd));
	    add_webtable(wp, (char *)"Mail forward", getboolean(nodes.MailFwd));
	    fprintf(fp, "     Check mailpwd    %s", getboolean(nodes.MailPwdCheck));
	    add_webtable(wp, (char *)"Check mailpassword", getboolean(nodes.MailPwdCheck));
	    fprintf(fp, "     ARCmail comp.    %s", getboolean(nodes.ARCmailCompat));
	    add_webtable(wp, (char *)" ARCmail compatibility", getboolean(nodes.ARCmailCompat));
	    fprintf(fp, "     ACRmail a..z     %s\n", getboolean(nodes.ARCmailAlpha));
	    add_webtable(wp, (char *)"ACRmail a..z", getboolean(nodes.ARCmailAlpha));
	    fprintf(fp, "     Send message     %s", getboolean(nodes.Message));
	    add_webtable(wp, (char *)"Send netmail with files", getboolean(nodes.Message));
	    fprintf(fp, "     Send .TIC        %s", getboolean(nodes.Tic));
	    add_webtable(wp, (char *)"Send .TIC files", getboolean(nodes.Tic));
	    fprintf(fp, "     File forward     %s\n", getboolean(nodes.FileFwd));
	    add_webtable(wp, (char *)"File forward", getboolean(nodes.FileFwd));
	    fprintf(fp, "     Advanced TIC     %s", getboolean(nodes.AdvTic));
	    add_webtable(wp, (char *)"Advanced TIC files", getboolean(nodes.AdvTic));
	    fprintf(fp, "     Advanded SB      %s", getboolean(nodes.TIC_AdvSB));
	    add_webtable(wp, (char *)" Advanded SB lines in .TIC", getboolean(nodes.TIC_AdvSB));
	    fprintf(fp, "     Sent To line     %s\n", getboolean(nodes.TIC_To));
	    add_webtable(wp, (char *)"Sent 'To' lines in .TIC", getboolean(nodes.TIC_To));
	    fprintf(fp, "     TIC 4d addresses %s\n", getboolean(nodes.Tic4d));
	    add_webtable(wp, (char *)"Use 4d addresses in .TIC", getboolean(nodes.Tic4d));
	    fprintf(fp, "     Security flags   %s\n\n", getflag(nodes.Security.flags, nodes.Security.notflags));
	    add_webtable(wp, (char *)"Security flags", getflag(nodes.Security.flags, nodes.Security.notflags));
	    fprintf(wp, "<TR><TD colspan='2'>&nbsp;</TD></TR>\n");

	    fprintf(fp, "     Outbound session %s\n", get_sessiontype(nodes.Session_out));
	    add_webtable(wp, (char *)"Outbound session", get_sessiontype(nodes.Session_out));
	    if (nodes.Session_out == S_DIR) {
		fprintf(fp, "     Path             %s\n", nodes.Dir_out_path);
		add_webtable(wp, (char *)"Path", nodes.Dir_out_path);
	        fprintf(fp, "     Check lock       %s", getboolean(nodes.Dir_out_chklck));
		add_webtable(wp, (char *)"Check lock", getboolean(nodes.Dir_out_chklck));
	        fprintf(fp, "     Wait clear lock  %s\n", getboolean(nodes.Dir_out_waitclr));
		add_webtable(wp, (char *)"Wait clear lock", getboolean(nodes.Dir_out_waitclr));
	        if (nodes.Dir_out_chklck) {
		    fprintf(fp, "     File to check    %s\n", nodes.Dir_out_clock);
		    add_webtable(wp, (char *)"File to check", nodes.Dir_out_clock);
		}
		fprintf(fp, "     Create lock      %s\n", getboolean(nodes.Dir_out_mklck));
		add_webtable(wp, (char *)"Create lock", getboolean(nodes.Dir_out_mklck));
	        if (nodes.Dir_out_mklck) {
		    fprintf(fp, "     File to create   %s\n", nodes.Dir_out_mlock);
		    add_webtable(wp, (char *)"File to create", nodes.Dir_out_mlock);
		}
	    }
	    fprintf(fp, "     Inbound session  %s\n", get_sessiontype(nodes.Session_in));
	    add_webtable(wp, (char *)"Inbound session", get_sessiontype(nodes.Session_in));
	    if (nodes.Session_in == S_DIR) {
	        fprintf(fp, "     Path             %s\n", nodes.Dir_in_path);
		add_webtable(wp, (char *)"Path", nodes.Dir_in_path);
	        fprintf(fp, "     Check lock       %s", getboolean(nodes.Dir_in_chklck));
		add_webtable(wp, (char *)"Check lock", getboolean(nodes.Dir_in_chklck));
	        fprintf(fp, "     Wait clear lock  %s\n", getboolean(nodes.Dir_in_waitclr));
		add_webtable(wp, (char *)"Wait clear lock", getboolean(nodes.Dir_in_waitclr));
	        if (nodes.Dir_in_chklck) {
		    fprintf(fp, "     File to check    %s\n", nodes.Dir_in_clock);
		    add_webtable(wp, (char *)"File to check", nodes.Dir_in_clock);
		}
		fprintf(fp, "     Create lock      %s\n", getboolean(nodes.Dir_in_mklck));
		add_webtable(wp, (char *)"Create lock", getboolean(nodes.Dir_in_mklck));
	        if (nodes.Dir_in_mklck) {
		    fprintf(fp, "     File to create   %s\n", nodes.Dir_in_mlock);
		    add_webtable(wp, (char *)"File to create", nodes.Dir_in_mlock);
		}
	    }
	    fprintf(fp, "\n");
	    fprintf(wp, "</TBODY>\n");
            fprintf(wp, "</TABLE>\n");

	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='30%%'><COL width='40%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    fprintf(wp, "<TR><TH align='left'>Uplink mgrs</TH><TH align='left'>Program</TH><TH align='left'>Password</TH><TH>BBBS</TH></TR>\n");
	    fprintf(wp, "<TR><TH align='left'>Files</TH><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n", nodes.UplFmgrPgm, nodes.UplFmgrPass, getboolean(nodes.UplFmgrBbbs));
	    fprintf(wp, "<TR><TH align='left'>Mail</TH><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n", nodes.UplAmgrPgm, nodes.UplAmgrPass, getboolean(nodes.UplAmgrBbbs));
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    fprintf(fp, "     Uplink mgrs    Program  Password        BBBS\n");
	    fprintf(fp, "     ------------   -------- --------------- ----\n");
	    fprintf(fp, "     Files          %-8s %-15s %s\n", nodes.UplFmgrPgm, nodes.UplFmgrPass, getboolean(nodes.UplFmgrBbbs));
	    fprintf(fp, "     Mail           %-8s %-15s %s\n\n", nodes.UplAmgrPgm, nodes.UplAmgrPass, getboolean(nodes.UplAmgrBbbs));

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Node Statistics</H3>\n");
	    add_statcnt(wp, (char *)"sent files", nodes.FilesSent);
	    add_statcnt(wp, (char *)"KBytes files", nodes.F_KbSent);
	    add_statcnt(wp, (char *)"received messages", nodes.MailRcvd); 
	    add_statcnt(wp, (char *)"sent messages", nodes.MailSent);
	    fprintf(fp, "     Statistics     Send     KBytes   Received KBytes\n");
	    fprintf(fp, "     ------------   -------- -------- -------- --------\n");
	    fprintf(fp, "     Total files    %-8u %-8u %-8u %-8u\n", nodes.FilesSent.total, nodes.F_KbSent.total, 
		    nodes.FilesRcvd.total, nodes.F_KbSent.total);
	    fprintf(fp, "     Total mail     %-8u          %-8u\n\n", nodes.MailSent.total, nodes.MailRcvd.total);

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Private data</H3>\n");
	    add_webtable(wp, (char *)"Private phone", nodes.Ct_phone);
	    add_webtable(wp, (char *)"Fax number", nodes.Ct_fax);
	    add_webtable(wp, (char *)"Cellphone", nodes.Ct_cellphone);
	    add_webtable(wp, (char *)"E-mail", nodes.Ct_email);
	    add_webtable(wp, (char *)"Remark", nodes.Ct_remark);

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>File Groups</H3>\n");
	    fprintf(wp, "<PRE>\n");
	    fprintf(wp, "      ");
	    fprintf(fp, "     File groups:\n      ");
	    groups = nodeshdr.filegrp / sizeof(group);
	    for (i = 0; i < groups; i++) {
		fread(&group, sizeof(group), 1, no);
		if (strlen(group)) {
		    fprintf(fp, "%-12s ", group);
		    fprintf(wp, "<A HREF=\"filegroup_%s.html\">%s</A>", group, group);
		    for (j = 0; j < (13 - strlen(group)); j++)
			fprintf(wp, " ");
		    if (((i+1) % 5) == 0) {
			fprintf(fp, "\n      ");
			fprintf(wp, "\n      ");
		    }
		}
	    }
	    if ((i+1) % 5) {
		fprintf(fp, "\n");
		fprintf(wp, "\n");
	    }
	    fprintf(wp, "</PRE>\n");

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Mail Groups</H3>\n");
	    fprintf(wp, "<PRE>\n");
	    fprintf(wp, "      ");
	    fprintf(fp, "\n     Mail groups:\n      ");
	    groups = nodeshdr.mailgrp / sizeof(group);
	    for (i = 0; i < groups; i++) {
		fread(&group, sizeof(group), 1, no);
		if (strlen(group)) {
		    fprintf(fp, "%-12s ", group);
		    fprintf(wp, "<A HREF=\"msggroup_%s.html\">%s</A>", group, group);
		    for (j = 0; j < (13 - strlen(group)); j++)
			fprintf(wp, " ");
		    if (((i+1) % 5) == 0) {
			fprintf(fp, "\n      ");
			fprintf(wp, "\n      ");
		    }
		}
	    }
	    if ((i+1) % 5) {
		fprintf(fp, "\n");
		fprintf(wp, "\n");
	    }
	    fprintf(wp, "</PRE>\n");

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>TIC Areas</H3>\n");
	    refs = 0;
	    snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	    if ((ti = fopen(temp, "r"))) {
		fread(&tichdr, tichdr.hdrsize, 1, ti);
		systems = tichdr.syssize / sizeof(sysconnect);
		while ((fread(&tic, tichdr.recsize, 1, ti)) == 1) {
		    for (i = 0; i < systems; i++) {
			fread(&System, sizeof(sysconnect), 1, ti);
			for (k = 0; k < 20; k++) {
			    if (nodes.Aka[k].zone && tic.Active &&
				    (System.aka.zone == nodes.Aka[k].zone) && (System.aka.net == nodes.Aka[k].net) && 
				    (System.aka.node == nodes.Aka[k].node) && (System.aka.point == nodes.Aka[k].point) && 
				    (strcmp(System.aka.domain, nodes.Aka[k].domain) == 0)) {
				snprintf(temp, 81, "---");
				if (System.sendto)
				    temp[0] = 'S';
				if (System.receivefrom)
				    temp[1] = 'R';
				if (System.pause)
				    temp[2] = 'P';
				if (refs == 0) {
				    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
				    fprintf(wp, "<COL width='20%%'><COL width='10%%'><COL width='70%%'>\n");
				    fprintf(wp, "<TBODY>\n");
				}
				refs++;
				fprintf(wp, "<TR><TD><A HREF=\"ticarea_%s.html\">%s</A></TD><TD>%s</TD><TD>%s</TD></TR>\n",
					tic.Name, tic.Name, temp, tic.Comment);
			    }
			}
		    }
		}
		fclose(ti);
	    }
	    if (refs) {
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
	    } else {
		fprintf(wp, "No TIC area references");
	    }

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Message Areas</H3>\n");
            nr = refs = 0;
            snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
            if ((ti = fopen(temp, "r"))) {
                fread(&msgshdr, msgshdr.hdrsize, 1, ti);
                systems = msgshdr.syssize / sizeof(sysconnect);
                while ((fread(&msgs, msgshdr.recsize, 1, ti)) == 1) {
		    nr++;
                    for (i = 0; i < systems; i++) {
                        fread(&System, sizeof(sysconnect), 1, ti);
                        for (k = 0; k < 20; k++) {
                            if (nodes.Aka[k].zone && msgs.Active &&
                                    (System.aka.zone == nodes.Aka[k].zone) && (System.aka.net == nodes.Aka[k].net) &&
                                    (System.aka.node == nodes.Aka[k].node) && (System.aka.point == nodes.Aka[k].point) &&
                                    (strcmp(System.aka.domain, nodes.Aka[k].domain) == 0)) {
                                snprintf(temp, 81, "----");
                                if (System.sendto)
                                    temp[0] = 'S';
                                if (System.receivefrom)
                                    temp[1] = 'R';
                                if (System.pause)
                                    temp[2] = 'P';
				if (System.cutoff)
				    temp[3] = 'C';
                                if (refs == 0) {
                                    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
                                    fprintf(wp, "<COL width='20%%'><COL width='10%%'><COL width='70%%'>\n");
                                    fprintf(wp, "<TBODY>\n");
                                }
                                refs++;
                                fprintf(wp, "<TR><TD><A HREF=\"msgarea_%d.html\">Area %d</A></TD><TD>%s</TD><TD>%s</TD></TR>\n",
                                        nr, nr, temp, msgs.Name);
                            }
                        }
                    }
                }
                fclose(ti);
            }
            if (refs) {
                fprintf(wp, "</TBODY>\n");
                fprintf(wp, "</TABLE>\n");
            } else {
                fprintf(wp, "No Message Area references");
            }

	    close_webdoc(wp);
	}
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	
    fclose(no);
    return page;
}


