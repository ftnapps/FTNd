/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: FileMgr
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbtic.h"
#include "../lib/dbdupe.h"
#include "../lib/dbuser.h"
#include "../lib/dbftn.h"
#include "sendmail.h"
#include "mgrutil.h"
#include "filemgr.h"



/*
 * External declarations
 */
extern	int	do_quiet;



/*
 * Global variables
 */
extern	int	net_in;			/* Netmails received		    */
extern	int	net_out;		/* Netmails forwarded		    */
extern	int	net_bad;		/* Bad netmails (tracking errors    */
extern	int	echo_in;		/* Echomail received		    */
extern	int	echo_imp;		/* Echomail imported		    */
extern	int	echo_out;		/* Echomail forwarded		    */
extern	int	echo_bad;		/* Bad fileecho			    */
extern	int	echo_dupe;		/* Dupe fileecho		    */

int	filemgr = 0;			/* Nr of FileMgr messages	    */
int	f_help  = FALSE;
int	f_list	= FALSE;
int	f_query = FALSE;
int	f_stat	= FALSE;
int	f_unlnk	= FALSE;



void F_Help(faddr *, char *);
void F_Help(faddr *t, char *replyid)
{
    FILE	*fp;

    Syslog('+', "FileMgr: Help");

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", (char *)"FileMgr help", replyid)) != NULL) {
	fprintf(fp, "Address all requests to '%s' (without quotes)\r", (char *)"Filemgr");
	fprintf(fp, "Youre FileMgr password goes on the subject line.\r\r");

	fprintf(fp, "In the body of the message to FileMgr:\r\r");

	fprintf(fp, "+<ticname>            To connect to an fileecho area\r");
	fprintf(fp, "-<ticname>            To disconnect from an fileecho area\r");
	fprintf(fp, "%%+ALL                 To connect to all fileecho areas\r");
	fprintf(fp, "%%-ALL                 To disconnect from all fileecho areas\r");
	fprintf(fp, "%%+<group>             To connect all fileecho areas of a group\r");
	fprintf(fp, "%%-<group>             To disconnect from all fileecho areas of a group\r");
	fprintf(fp, "%%HELP                 To request this help message\r");
	fprintf(fp, "%%LIST                 To request a list of available fileecho areas\r");
	fprintf(fp, "%%QUERY                To request a list of active fileecho areas\r");
	fprintf(fp, "%%UNLINKED             To request a list of available fileecho areas\r");
	fprintf(fp, "                      to which you are not already connected\r");
	fprintf(fp, "%%STATUS               To request a status report for your system\r");
//	fprintf(fp, "%%PAUSE                To temporary disconnect from the connected areas\r");
//	fprintf(fp, "%%RESUME               To reconnect the temporary disconnected areas\r);
	fprintf(fp, "%%PWD=newpwd           To set a new AreaMgr and FileMgr password\r");
//	fprintf(fp, "%%RESCAN <area>        To request all files from 'area' again\r");
	fprintf(fp, "%%MESSGAE=On/Off       To switch the message function on or off\r");
	fprintf(fp, "%%TICK=On/Off/Advanced To set the tic file mode off, normal or advanced\r");
	fprintf(fp, "%%NOTIFY=On/Off        To switch the notify function on or off\r");
//	fprintf(fp, "%%RESEND <name>        To resend file 'name' with tic file\r");
	fprintf(fp, "[---]                 Everything below the tearline is ignored\r\r");

	fprintf(fp, "Example:\r\r");

	fprintf(fp, "  By: %s\r", nodes.Sysop);
	fprintf(fp, "  To: %s, %s\r", (char *)"Filemgr", ascfnode(bestaka_s(t), 0xf));
	fprintf(fp, "  Re: %s\r", nodes.Fpasswd);
	fprintf(fp, "  St: Pvt Local Kill\r");
	fprintf(fp, "  ----------------------------------------------------------\r");
	fprintf(fp, "  +MBSE_BBS\r");
	fprintf(fp, "  -NODELIST\r");
	fprintf(fp, "  %%QUERY\r");
	fprintf(fp, "  %%LIST\r\r");

	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void F_Query(faddr *, char *);
void F_Query(faddr *t, char *replyid)
{
    FILE	*qp, *gp, *fp;
    char	*temp, *Group;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[4];
    faddr	*f, *g;
    sysconnect	System;

    Syslog('+', "FileMgr: Query");
    f = bestaka_s(t);
    Syslog('f', "Bestaka for %s is %s", ascfnode(t, 0x0f), ascfnode(f, 0x0f));

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", (char *)"Your query request", replyid)) != NULL) {

	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	fp = fopen(temp, "r");
	fread(&tichdr, sizeof(tichdr), 1, fp);
	Cons = tichdr.syssize / sizeof(System);
	sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	gp = fopen(temp, "r");
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);

	fprintf(qp, "The following is a list of all connected file areas\r\r");

	while (TRUE) {
	    Group = GetNodeFileGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(fgroup.UseAka));
		if ((!strcmp(fgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) &&
		    (g->node  == f->node) && (g->point == f->point)) {
		    Syslog('f', "Group %s, aka %s", fgroup.Name, aka2str(fgroup.UseAka));
		    SubTot = 0;
		    fprintf(qp, "Group %s - %s (%s)\r\r", fgroup.Name, fgroup.Comment, aka2str(fgroup.UseAka));
		    fprintf(qp, "Con File tic            Description\r");
		    fprintf(qp, "------------------------------------------------------------------------\r");
		    fseek(fp, tichdr.hdrsize, SEEK_SET);

		    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
			if (!strcmp(Group, tic.Group) && tic.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     *  Now check if this node is connected,
			     *  if so, set the Stat bits.
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, fp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if (System.receivefrom)
					Stat[0] = 'S';
				    if (System.sendto)
					Stat[1] = 'R';
				    if (System.pause)
					Stat[2] = 'P';

				    if (System.sendto || System.receivefrom) {
					fprintf(qp, "%s %-20s %s\r", Stat, tic.Name, tic.Comment);
					SubTot++;
					Total++;
				    }
				}
			    }
			} else
			    fseek(fp, tichdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "------------------------------------------------------------------------\r");
		    fprintf(qp, "%d available area(s)\r\r\r", SubTot);
		}
	    }
	}

	fclose(fp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " R  - You receive files from my system\r");
	fprintf(qp, " S  - You may send files in this area\r");
	fprintf(qp, " P  - The file area is temporary paused\r\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
	free(temp);
    } else
	WriteError("Can't create netmail");
}



void F_List(faddr *t, char *replyid, int Notify)
{
    FILE	*qp, *gp, *fp;
    char	*temp, *Group;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[4];
    faddr	*f, *g;
    sysconnect	System;

    if (Notify)
	Syslog('+', "FileMgr: Notify to %s", ascfnode(t, 0xff));
    else
	Syslog('+', "FileMgr: List");
    f = bestaka_s(t);

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", (char *)"FileMgr List", replyid)) != NULL) {

	WriteFileGroups(qp, f);
	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	fp = fopen(temp, "r");
	fread(&tichdr, sizeof(tichdr), 1, fp);
	Cons = tichdr.syssize / sizeof(System);
	sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	gp = fopen(temp, "r");
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);

	fprintf(qp, "The following is a list of all file areas\r\r");

	while (TRUE) {
	    Group = GetNodeFileGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(fgroup.UseAka));
		if ((!strcmp(fgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) &&
		    (g->node  == f->node) && (g->point == f->point)) {
		    SubTot = 0;
		    fprintf(qp, "Group %s - %s (%s)\r\r", fgroup.Name, fgroup.Comment, aka2str(fgroup.UseAka));
		    fprintf(qp, "Con File tic             Description\r");
		    fprintf(qp, "------------------------------------------------------------------------\r");
		    fseek(fp, tichdr.hdrsize, SEEK_SET);

		    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
			if (!strcmp(Group, tic.Group) && tic.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     *  Now check if this node is connected, if so, set the Stat bits.
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, fp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if (System.receivefrom)
					Stat[0] = 'S';
				    if (System.sendto)
					Stat[1] = 'R';
				    if (System.pause)
					Stat[2] = 'P';
				}
			    }
			    fprintf(qp, "%s %-20s %s\r", Stat, tic.Name, tic.Comment);
			    SubTot++;
			    Total++;
			} else
			    fseek(fp, tichdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "------------------------------------------------------------------------\r");
		    fprintf(qp, "%d available area(s)\r\r\r", SubTot);
		}
	    }
	}

	fclose(fp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " R  - You receive files from my system\r");
	fprintf(qp, " S  - You may send files in this area\r");
	fprintf(qp, " P  - The file area is temporary paused\r\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
	free(temp);
    } else
	WriteError("Can't create netmail");
}



void F_Status(faddr *, char *);
void F_Status(faddr *t, char *replyid)
{
    FILE    *fp;
    int	    i;

    Syslog('+', "FileMgr: Status");
    if (Miy == 0)
	i = 11;
    else
	i = Miy - 1;

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", (char *)"FileMgr Status", replyid)) != NULL) {

	fprintf(fp, "Here is your fileecho status:\r\r");

	fprintf(fp, "Netmail message     %s\r", GetBool(nodes.Message));
	fprintf(fp, "TIC files           %s\r", GetBool(nodes.Tic));
	if (nodes.Tic)
	    fprintf(fp, "Andvanced TIC files %s\r", GetBool(nodes.AdvTic));
	fprintf(fp, "Notify messages     %s\r", GetBool(nodes.Notify));
	fprintf(fp, "Cost sharing        %s\r", GetBool(nodes.Billing));
	if (nodes.Billing) {
	    fprintf(fp, "Send bill direct    %s\r", GetBool(nodes.BillDirect));
	    fprintf(fp, "Units debet         %ld\r", nodes.Debet);
	    fprintf(fp, "Units credit        %ld\r", nodes.Credit);
	    fprintf(fp, "Warning level       %ld\r", nodes.WarnLevel);
	}

	fprintf(fp, "\r\rRecent flow:\r\r");

	fprintf(fp, "                 Last week  Last month Total ever\r");
	fprintf(fp, "                 ---------- ---------- ----------\r");
	fprintf(fp, "Files sent       %-10ld %-10ld %-10ld\r", nodes.FilesSent.lweek, nodes.FilesSent.month[i], nodes.FilesSent.total);
	fprintf(fp, "KBytes sent      %-10ld %-10ld %-10ld\r", nodes.F_KbSent.lweek,  nodes.F_KbSent.month[i],  nodes.F_KbSent.total);
	fprintf(fp, "Files received   %-10ld %-10ld %-10ld\r", nodes.FilesRcvd.lweek, nodes.FilesRcvd.month[i], nodes.FilesRcvd.total);
	fprintf(fp, "KBytes received  %-10ld %-10ld %-10ld\r", nodes.F_KbRcvd.lweek,  nodes.F_KbRcvd.month[i],  nodes.F_KbRcvd.total);

	fprintf(fp, "\rWith regards, %s\r\r", CFG.sysop_name);

	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void F_Unlinked(faddr *, char *);
void F_Unlinked(faddr *t, char *replyid)
{
    FILE	*qp, *gp, *fp;
    char	*temp, *Group;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[4];
    faddr	*f, *g;
    sysconnect	System;

    Syslog('+', "FileMgr: Unlinked");
    f = bestaka_s(t);

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", (char *)"Your unlinked request", replyid)) != NULL) {

	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	fp = fopen(temp, "r");
	fread(&tichdr, sizeof(tichdr), 1, fp);
	Cons = tichdr.syssize / sizeof(System);
	sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	gp = fopen(temp, "r");
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);

	fprintf(qp, "The following is a list of all available file areas\r\r");

	while (TRUE) {
	    Group = GetNodeFileGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(fgroup.UseAka));
		if ((!strcmp(fgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) &&
		    (g->node  == f->node) && (g->point == f->point)) {
		    SubTot = 0;
		    fprintf(qp, "Group %s - %s (%s)\r\r", fgroup.Name, fgroup.Comment, aka2str(fgroup.UseAka));
		    fprintf(qp, "Con File tic            Description\r");
		    fprintf(qp, "------------------------------------------------------------------------\r");
		    fseek(fp, tichdr.hdrsize, SEEK_SET);

		    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
			if (!strcmp(Group, tic.Group) && tic.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     *  Now check if this node is connected, if so, set the Stat bits.
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, fp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if (System.receivefrom)
					Stat[0] = 'S';
				    if (System.sendto)
					Stat[1] = 'R';
				    if (System.pause)
					Stat[2] = 'P';
				}
			    }

			    if ((!System.sendto) && (!System.receivefrom)) {
				fprintf(qp, "%s %-20s %s\r", Stat, tic.Name, tic.Comment);
				SubTot++;
				Total++;
			    }
			} else
			    fseek(fp, tichdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "------------------------------------------------------------------------\r");
		    fprintf(qp, "%d available area(s)\r\r\r", SubTot);
		}
	    }
	}

	fclose(fp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " R  - You receive files from my system\r");
	fprintf(qp, " S  - You may send files in this area\r");
	fprintf(qp, " P  - The file area is temporary paused\r\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
	free(temp);
    } else
	WriteError("Can't create netmail");
}



void F_Global(faddr *, char *, FILE *);
void F_Global(faddr *t, char *Cmd, FILE *tmp)
{
	ShiftBuf(Cmd, 1);
	Syslog('m', "  FileMgr node %s global %s", ascfnode(t, 0x1f), Cmd);
}



void F_Disconnect(faddr *, char *, FILE *);
void F_Disconnect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group;
    faddr	*b;
    sysconnect	Sys;

    Syslog('+', "FileMgr: %s", Area);
    ShiftBuf(Area, 1);

    if (!SearchTic(Area)) {
	fprintf(tmp, "Area %s not found\n", Area);
	Syslog('m', "  Area not found");
	return;
    }

    Syslog('m', "  Found %s group %s", tic.Name, fgroup.Name);

    First = TRUE;
    while ((Group = GetNodeFileGrp(First)) != NULL) {
	First = FALSE;
	if (strcmp(Group, fgroup.Name) == 0)
	    break;
    }
    if (Group == NULL) {
	fprintf(tmp, "You may not disconnect from area %s\n", Area);
	Syslog('m', "  Group %s not available for node", fgroup.Name);
	return;
    }

    b = bestaka_s(t);
    i = metric(b, fido2faddr(fgroup.UseAka));
    Syslog('m', "Aka match level is %d", i);

    if (i > METRIC_POINT) {
	fprintf(tmp, "You may not disconnect area %s with nodenumber %s\n", Area, ascfnode(t, 0x1f));
	Syslog('m', "  Node may not disconnect from group %s", fgroup.Name);
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = FALSE;
    Sys.receivefrom = FALSE;

    if (!TicSystemConnected(Sys)) {
	fprintf(tmp, "You are not connected to %s\n", Area);
	Syslog('m', "  Node is not connected to %s", Area);
	return;
    }

    if (!TicSystemConnect(&Sys, FALSE)) {

	/*
	 * Make sure to write an overview afterwards
	 */
	f_list = TRUE;
	Syslog('+', "Disconnected file area %s", Area);
	fprintf(tmp, "Disconnected from area %s\n", Area);
	return;
    }

    fprintf(tmp, "You may not disconnect area %s, area is mandatory\n", Area);
    Syslog('+', "Didn't disconnect %s from mandatory file area %s", ascfnode(t, 0x1f), Area);
}



void F_Connect(faddr *, char *, FILE *);
void F_Connect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group;
    faddr	*b;
    sysconnect	Sys;

    Syslog('+', "FileMgr: %s", Area);

    if (Area[0] == '+')
	ShiftBuf(Area, 1);

    if (!SearchTic(Area)) {
	fprintf(tmp, "Area %s not found\n", Area);
	Syslog('m', "  Area not found");
	/* SHOULD CHECK FOR AREAS FILE AND ASK UPLINK
	   CHECK ALL GROUPRECORDS FOR AKA MATCH
	   IF MATCH CHECK FOR UPLINK AND AREAS FILE
	   IF FOUND, CREATE TIC AREA, CONNECT UPLINK
	   RESTORE NODERECORD (IS GONE!)
	   FALLTHRU TO CONNECT DOWNLINK
	*/
	return;
    }

    Syslog('m', "  Found %s group %s", tic.Name, fgroup.Name);

    First = TRUE;
    while ((Group = GetNodeFileGrp(First)) != NULL) {
	First = FALSE;
	if (strcmp(Group, fgroup.Name) == 0)
	    break;
    }
    if (Group == NULL) {
	fprintf(tmp, "You may not connect to area %s\n", Area);
	Syslog('m', "  Group %s not available for node %s", fgroup.Name);
	return;
    }

    b = bestaka_s(t);
    i = metric(b, fido2faddr(fgroup.UseAka));
    Syslog('m', "Aka match level is %d", i);

    if (i > METRIC_POINT) {
	fprintf(tmp, "You may not connect area %s with nodenumber %s\n", Area, ascfnode(t, 0x1f));
	Syslog('m', "  Node may not connect to group %s", fgroup.Name);
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = TRUE;

    if (TicSystemConnected(Sys)) {
	fprintf(tmp, "You are already connected to %s\n", Area);
	Syslog('m', "  Node is already connected to %s", Area);
	return;
    }

    if (TicSystemConnect(&Sys, TRUE)) {

	/*
	 * Make sure to write an overview afterwards
	 */
	f_list = TRUE;
	Syslog('+', "Connected to file area %s", Area);
	fprintf(tmp, "Connected to area %s\n", Area);
	return;
    }

    fprintf(tmp, "Not connected to %s, internal error, sysop is notified\n", Area);
    WriteError("Can't connect node %s to file area %s", ascfnode(t, 0x1f), Area);
}



void F_All(faddr *, int, FILE *, char *);
void F_All(faddr *t, int Connect, FILE *tmp, char *Grp)
{
    FILE	*fp, *gp;
    char	*Group, temp[PATH_MAX];
    faddr	*f;
    int		i, Link, First = TRUE, Cons;
    sysconnect	Sys;
    long	Pos;

    if (Grp == NULL) {
	if (Connect)
	    Syslog('+', "FileMgr: Connect All");
	else
	    Syslog('+', "FileMgr: Disconnect All");
    } else {
	if (Connect)
	    Syslog('+', "FileMgr: Connect group %s", Grp);
	else
	    Syslog('+', "FileMgr: Disconnect group %s", Grp);
    }

    f = bestaka_s(t);
    sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
    fp = fopen(temp, "r+");
    fread(&tichdr, sizeof(tichdr), 1, fp);
    Cons = tichdr.syssize / sizeof(Sys);
    sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    gp = fopen(temp, "r");
    fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);

    while (TRUE) {
	Group = GetNodeFileGrp(First);
	if (Group == NULL)
	    break;
	First = FALSE;

	fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
	while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
	    if ((!strcmp(fgroup.Name, Group)) && ((Grp == NULL) || (!strcmp(Group, Grp)))) {

		fseek(fp, tichdr.hdrsize, SEEK_SET);
		while (fread(&tic, tichdr.recsize, 1, fp) == 1) {

		    if ((!strcmp(Group, tic.Group)) && tic.Active && (metric(fido2faddr(fgroup.UseAka), f) == METRIC_EQUAL)) {

			if (Connect) {
			    Link = FALSE;
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, fp);
				if (metric(fido2faddr(Sys.aka), t) == METRIC_EQUAL)
				Link = TRUE;
			    }
			    if (!Link) {
				Pos = ftell(fp);
				fseek(fp, - tichdr.syssize, SEEK_CUR);
				for (i = 0; i < Cons; i++) {
				    fread(&Sys, sizeof(Sys), 1, fp);
				    if (!Sys.aka.zone) {
					memset(&Sys, 0, sizeof(Sys));
					memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
					Sys.sendto = TRUE;
					fseek(fp, - sizeof(Sys), SEEK_CUR);
					fwrite(&Sys, sizeof(Sys), 1, fp);
					Syslog('+', "FileMgr: Connected %s", tic.Name);
					fprintf(tmp, "Connected area %s\n", tic.Name);
					f_list = TRUE;
					break;
				    }
				}
				fseek(fp, Pos, SEEK_SET);
			    }
			} else {
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, fp);
				if (metric(fido2faddr(Sys.aka), t) == METRIC_EQUAL) {
				    memset(&Sys, 0, sizeof(Sys));
				    fseek(fp, - sizeof(Sys), SEEK_CUR);
				    fwrite(&Sys, sizeof(Sys), 1, fp);
				    Syslog('+', "FileMgr: Disconnected %s", tic.Name);
				    fprintf(tmp, "Disconnected area %s\n", tic.Name);
				    f_list = TRUE;
				}
			    }
			}
		    } else
			fseek(fp, tichdr.syssize, SEEK_CUR);
		}
	    }
	}
    }
    fclose(gp);
    fclose(fp);
}



void F_Group(faddr *, char *, int, FILE *);
void F_Group(faddr *t, char *Area, int Connect, FILE *tmp)
{
    ShiftBuf(Area, 2);
    CleanBuf(Area);
    F_All(t, Connect, tmp, Area);
}



void F_Pause(faddr *, int, FILE *);
void F_Pause(faddr *t, int Pause, FILE *tmp)
{
    if (Pause)
	Syslog('+', "FileMgr: Pause");
    else
	Syslog('+', "FileMgr: Resume");
}



void F_Message(faddr *t, char *Buf, FILE *tmp)
{
    ShiftBuf(Buf, 8);
    CleanBuf(Buf);

    if (!strncasecmp(Buf, "on", 2))
	nodes.Message = TRUE;
    else if (!strncasecmp(Buf, "off", 3))
	nodes.Message = FALSE;
    else
	return;

    UpdateNode();
    SearchNodeFaddr(t);
    Syslog('+', "FileMgr: Message %s", GetBool(nodes.Message));
    fprintf(tmp, "FileMgr Message file is %s\n", GetBool(nodes.Message));
}



void F_Tick(faddr *t, char *Buf, FILE *tmp)
{
    ShiftBuf(Buf, 5);
    CleanBuf(Buf);
    
    if (!strncasecmp(Buf, "on", 2)) {
	nodes.Tic = TRUE;
	nodes.AdvTic = FALSE;
    } else if (!strncasecmp(Buf, "off", 3)) {
	nodes.Tic = nodes.AdvTic = FALSE;
    } else if (!strncasecmp(Buf, "advanced", 8)) {
	nodes.Tic = nodes.AdvTic = TRUE;
    } else
	return;

    UpdateNode();
    SearchNodeFaddr(t);
    Syslog('+', "FileMgr: Tick %s, Advanced %s", GetBool(nodes.Tic), GetBool(nodes.AdvTic));
    if (nodes.Tic)
	if (nodes.AdvTic)
	    fprintf(tmp, "Tick mode is advanced\n");
	else
	    fprintf(tmp, "Tick mode is normal\n");
    else
	fprintf(tmp, "Tick mode is off\n");
}



int FileMgr(faddr *f, faddr *t, char *replyid, char *subj, time_t mdate, int flags, FILE *fp)
{
    int		i, rc = 0, spaces;
    char	*Buf;
    FILE	*tmp, *np;

    f_help = f_stat = f_unlnk = f_list = f_query = FALSE;
    filemgr++;
    if (SearchFidonet(f->zone))
	f->domain = xstrcpy(fidonet.domain);

    Syslog('+', "FileMgr msg from %s", ascfnode(f, 0xff));

    /*
     * If the password failed, we return silently and don't respond.
     */
    if ((!strlen(subj)) || (strcasecmp(subj, nodes.Fpasswd))) {
	WriteError("FileMgr: password expected \"%s\", got \"%s\"", nodes.Fpasswd, subj);
	net_bad++;
	return FALSE;
    }

    if ((tmp = tmpfile()) == NULL) {
	WriteError("$FileMsr: Can't open tmpfile()");
	net_bad++;
	return FALSE;
    }
    
    Buf = calloc(2049, sizeof(char));
    rewind(fp);

    while ((fgets(Buf, 2048, fp)) != NULL) {

	/*
	 * Make sure we refresh the nodes record.
	 */
	SearchNodeFaddr(f);

	spaces = 0;
	for (i = 0; i < strlen(Buf); i++) {
	    if (*(Buf + i) == ' ')
		spaces++;
	    if (*(Buf + i) == '\t')
		spaces++;
	    if (*(Buf + i) == '\0')
		break;
	    if (*(Buf + i) == '\n')
		*(Buf + i) = '\0';
	    if (*(Buf + i) == '\r')
		*(Buf + i) = '\0';
	}

	if (!strncmp(Buf, "---", 3))
	    break;

	if (strlen(Buf) && (*(Buf) != '\001') && (spaces <= 1)) {

	if (!strncasecmp(Buf, "%help", 5))
		f_help = TRUE;
	    else if (!strncasecmp(Buf, "%query", 6))
		f_query = TRUE;
	    else if (!strncasecmp(Buf, "%linked", 7))
		f_query = TRUE;
	    else if (!strncasecmp(Buf, "%list", 5))
		f_list = TRUE;
	    else if (!strncasecmp(Buf, "%status", 7))
		f_stat = TRUE;
	    else if (!strncasecmp(Buf, "%unlinked", 9))
		f_unlnk = TRUE;
	    else if (!strncasecmp(Buf, "%+all", 5))
		F_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-all", 5))
		F_All(f, FALSE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%+*", 3))
		F_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-*", 3))
		F_All(f, FALSE, tmp, NULL); 
	    else if (!strncasecmp(Buf, "%+", 2))
		F_Group(f, Buf, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%-", 2))
		F_Group(f, Buf, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%pause", 6))
		F_Pause(f, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%resume", 7))
		F_Pause(f, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%password", 9))
		MgrPasswd(f, Buf, tmp, 9);
	    else if (!strncasecmp(Buf, "%pwd", 4))
		MgrPasswd(f, Buf, tmp, 4);
	    else if (!strncasecmp(Buf, "%notify", 7))
		MgrNotify(f, Buf, tmp);
	    else if (!strncasecmp(Buf, "%message", 8))
		F_Message(f, Buf, tmp);
	    else if (!strncasecmp(Buf, "%tick", 5))
		F_Tick(f, Buf, tmp);
	    else if (*(Buf) == '%')
		F_Global(f, Buf, tmp);
	    else if (*(Buf) == '-')
		F_Disconnect(f, Buf, tmp);
	    else
		F_Connect(f, Buf, tmp);
	}
    }

    if (ftell(tmp)) {
	if ((np = SendMgrMail(f, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", (char *)"Your FileMgr request", replyid)) != NULL) {

	    fprintf(np, "     Dear %s\r\r", nodes.Sysop);
	    fprintf(np, "Here is the result of your FileMgr request:\r\r");
	    fseek(tmp, 0, SEEK_SET);

	    while ((fgets(Buf, 2048, tmp)) != NULL) {
		Buf[strlen(Buf)-1] = '\0';
		fprintf(np, "%s\r", Buf);
		Syslog('m', "Rep: %s", Buf);
	    }

	    fprintf(np, "\rWith regards, %s\r\r", CFG.sysop_name);
	    fprintf(np, "%s\r", TearLine());
	    CloseMail(np, t);
	    net_out++;
	} else 
	    WriteError("Can't create netmail");
    }

    free(Buf);
    fclose(tmp);

    if (f_stat)
	F_Status(f, replyid);

    if (f_query)
	F_Query(f, replyid);

    if (f_list)
	F_List(f, replyid, FALSE);

    if (f_unlnk)
	F_Unlinked(f, replyid);

    if (f_help)
	F_Help(f, replyid);

    return rc;
}


