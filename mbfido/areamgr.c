/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: AreaMgr
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
#include "../lib/dbmsgs.h"
#include "../lib/dbdupe.h"
#include "../lib/dbuser.h"
#include "../lib/dbftn.h"
#include "sendmail.h"
#include "mgrutil.h"
#include "scan.h"
#include "areamgr.h"



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
extern	int	echo_bad;		/* Bad echomail			    */
extern	int	echo_dupe;		/* Dupe echomail		    */

int		areamgr = 0;		/* Nr of AreaMgr messages	    */
int		a_help	= FALSE;	/* Send AreaMgr help		    */
int		a_list	= FALSE;	/* Send AreaMgr list		    */
int		a_query	= FALSE;	/* Send AreaMgr query		    */
int		a_stat	= FALSE;	/* Send AreaMgr status		    */
int		a_unlnk	= FALSE;	/* Send AreaMgr unlinked	    */
int		a_flow  = FALSE;	/* Send AreaMgr flow		    */
unsigned long	a_msgs = 0;		/* Messages to rescan		    */



void A_Help(faddr *, char *);
void A_Help(faddr *t, char *replyid)
{
    FILE    *fp;

    Syslog('+', "AreaMgr: Help");

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"AreaMgr help", replyid)) != NULL) {
	fprintf(fp, "Address all requests to '%s' (without quotes)\r", (char *)"Areamgr");
	fprintf(fp, "Your AreaMgr password goes on the subject line.\r\r");

	fprintf(fp, "In the body of the message to AreaMgr:\r\r");

	fprintf(fp, "+<areaname>    To connect to an echomail area\r");
	fprintf(fp, "-<areaname>    To disconnect from an echomail area\r");
	fprintf(fp, "%%+ALL          To connect to all echomail areas\r");
	fprintf(fp, "%%-ALL          To disconnect from all echomail areas\r");
	fprintf(fp, "%%+<group>      To connect all echomail areas of a group\r");
	fprintf(fp, "%%-<group>      To disconnect from all echomail areas of a group\r");
	fprintf(fp, "%%HELP          To request this help message\r");
	fprintf(fp, "%%LIST          To request a list of echomail areas available to you\r");
	fprintf(fp, "%%QUERY         To request a list of echomail areas for which you are active\r");
	fprintf(fp, "%%UNLINKED      To request a list of echomail areas available to you\r");
	fprintf(fp, "               to which you are not already connected\r");
	fprintf(fp, "%%FLOW          To request a flow report of available areas\r");
	fprintf(fp, "%%STATUS        To request a status report for your system\r");
	fprintf(fp, "%%PAUSE         To temporary disconnect from the connected echomail areas\r");
	fprintf(fp, "%%RESUME        To reconnect the temporary disconnected echomail areas\r");
	fprintf(fp, "%%PWD=newpwd    To set a new AreaMgr and FileMgr password\r");
	fprintf(fp, "%%MSGS <n>      To set max. number of messages to be rescanned\r");
	fprintf(fp, "%%RESCAN <area> To request messages from 'area' again\r");
	fprintf(fp, "%%NOTIFY=On/Off To switch the notify function on or off\r");
	fprintf(fp, "[---]          Everything below the tearline is ignored\r\r");

	fprintf(fp, "Example:\r\r");

	fprintf(fp, "  By: %s\r", nodes.Sysop);
	fprintf(fp, "  To: %s, %s\r", (char *)"Areamgr", ascfnode(bestaka_s(t), 0xf));
	fprintf(fp, "  Re: %s\r", nodes.Apasswd);
	fprintf(fp, "  St: Pvt Local Kill\r");
	fprintf(fp, "  ----------------------------------------------------------\r");
	fprintf(fp, "  +SYSOPS\r");
	fprintf(fp, "  -GENERAL\r");
	fprintf(fp, "  %%QUERY\r");
	fprintf(fp, "  %%LIST\r\r");
	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void A_Query(faddr *, char *);
void A_Query(faddr *t, char *replyid)
{
    FILE	*qp, *gp, *mp;
    char	*temp, *Group;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[5];
    faddr	*f, *g;
    sysconnect	System;

    Syslog('+', "AreaMgr: Query");
    f = bestaka_s(t);

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"Your query request", replyid)) != NULL) {

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((mp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    return;
	}
	fread(&msgshdr, sizeof(msgshdr), 1, mp);
	Cons = msgshdr.syssize / sizeof(System);

	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    fclose(mp);
	    return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	free(temp);

	fprintf(qp, "The following is a list of all connected message areas\r\r");

	while (TRUE) {
	    Group = GetNodeMailGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(mgroup.UseAka));
		if ((!strcmp(mgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) &&
		    (g->node  == f->node) && (g->point == f->point)) {
		    SubTot = 0;
		    fprintf(qp, "Group %s - %s (%s)\r\r", mgroup.Name, mgroup.Comment, aka2str(mgroup.UseAka));
		    fprintf(qp, "Con  Message area              Description\r");
		    fprintf(qp, "----------------------------------------------------------------------------\r");
		    fseek(mp, msgshdr.hdrsize, SEEK_SET);

		    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
			if (!strcmp(Group, msgs.Group) && msgs.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     * Now check if this node is connected, if so, set the Stat bits
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, mp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if (System.receivefrom)
					Stat[0] = 'S';
				    if (System.sendto)
					Stat[1] = 'R';
				    if (System.pause)
					Stat[2] = 'P';
				    if (System.cutoff)
					Stat[3] = 'C';
				}
			    }

			    if (Stat[0] == 'S' || Stat[1] == 'R') {
				fprintf(qp, "%s %-25s %s\r", Stat, msgs.Tag, msgs.Name);
				SubTot++;
				Total++;
			    }
			} else
			    fseek(mp, msgshdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "----------------------------------------------------------------------------\r");
		    fprintf(qp, "%d connected area(s)\r\r\r", SubTot);
		}
	    }
	}
	fprintf(qp, "Total: %d connected area(s)\r\r\r", Total);

	fclose(mp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " R  - You receive mail from my system\r");
	fprintf(qp, " S  - You may send mail to my system\r");
	fprintf(qp, " P  - The message area is temporary paused\r");
	fprintf(qp, " C  - You are cutoff from this area\r\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void A_List(faddr *t, char *replyid, int Notify)
{
    FILE	*qp, *gp, *mp;
    char	*temp, *Group;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[5];
    faddr	*f, *g;
    sysconnect	System;

    if (Notify)
	Syslog('+', "AreaMgr: Notify to %s", ascfnode(t, 0xff));
    else
	Syslog('+', "AreaMgr: List");
    f = bestaka_s(t);

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"AreaMgr List", replyid)) != NULL) {

	WriteMailGroups(qp, f);

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((mp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    return;
	}
	fread(&msgshdr, sizeof(msgshdr), 1, mp);
	Cons = msgshdr.syssize / sizeof(System);

	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    fclose(mp);
	    return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	free(temp);

	fprintf(qp, "The following is a list of all message areas\r\r");

	while (TRUE) {
	    Group = GetNodeMailGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(mgroup.UseAka));
		if ((!strcmp(mgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) &&
		    (g->node  == f->node) && (g->point == f->point)) {
		    SubTot = 0;
		    fprintf(qp, "Group %s - %s (%s)\r\r", mgroup.Name, mgroup.Comment, aka2str(mgroup.UseAka));
		    fprintf(qp, "Con  Message area              Description\r");
		    fprintf(qp, "----------------------------------------------------------------------------\r");
		    fseek(mp, msgshdr.hdrsize, SEEK_SET);

		    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
			if (!strcmp(Group, msgs.Group) && msgs.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     * Now check if this node is connected, if so, set the Stat bits
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, mp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if (System.receivefrom)
					Stat[0] = 'S';
				    if (System.sendto)
					Stat[1] = 'R';
				    if (System.pause)
					Stat[2] = 'P';
				    if (System.cutoff)
					Stat[3] = 'C';
				}
			    }
			    fprintf(qp, "%s %-25s %s\r", Stat, msgs.Tag, msgs.Name);
			    SubTot++;
			    Total++;
			} else
			    fseek(mp, msgshdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "----------------------------------------------------------------------------\r");
		    fprintf(qp, "%d available area(s)\r\r\r", SubTot);
		}
	    }
	}
	fprintf(qp, "Total: %d available area(s)\r\r\r", Total);

	fclose(mp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " R  - You receive mail from my system\r");
	fprintf(qp, " S  - You may send mail to my system\r");
	fprintf(qp, " P  - The message area is temporary paused\r");
	fprintf(qp, " C  - You are cutoff from this area\r\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void A_Flow(faddr *t, char *replyid, int Notify)
{
    FILE	*qp, *gp, *mp;
    char	*temp, *Group;
    int		i, First = TRUE, Cons;
    char	Stat[2];
    faddr	*f, *g;
    sysconnect	System;
    time_t	Now;
    struct tm	*tt;
    int		lmonth;
    long	lw, lm;

    Now = time(NULL);
    tt = localtime(&Now);
    lmonth = tt->tm_mon;
    if (lmonth)
	lmonth--;
    else
	lmonth = 11;

    if (Notify)
	Syslog('+', "AreaMgr: Flow report to %s", ascfnode(t, 0xff));
    else
	Syslog('+', "AreaMgr: Flow report");
    f = bestaka_s(t);

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"AreaMgr Flow report", replyid)) != NULL) {

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((mp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    return;
	}
	fread(&msgshdr, sizeof(msgshdr), 1, mp);
	Cons = msgshdr.syssize / sizeof(System);
	
	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    fclose(mp);
	    return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	free(temp);

	fprintf(qp, "The following is a flow report of all message areas\r\r");

	while (TRUE) {
	    Group = GetNodeMailGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    lm = lw = 0;
	    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(mgroup.UseAka));
		if ((!strcmp(mgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) &&
		    (g->node  == f->node) && (g->point == f->point)) {
		    fprintf(qp, "Group %s - %s\r\r", mgroup.Name, mgroup.Comment);
//				          1         2         3         4         5         6         7
//				 12345678901234567890123456789012345678901234567890123456789012345678901234567890
		    fprintf(qp, "Con Message area                                       Last week Last Month\r");
		    fprintf(qp, "---------------------------------------------------------------------------\r");
		    fseek(mp, msgshdr.hdrsize, SEEK_SET);

		    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
			if (!strcmp(Group, msgs.Group) && msgs.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     * Now check if this node is connected, if so, set the Stat bits
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, mp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if ((System.receivefrom || System.sendto) && (!System.pause) && (!System.cutoff))
					Stat[0] = 'C';
				}
			    }
			    fprintf(qp, "%s   %s %9lu %10lu\r", Stat, padleft(msgs.Tag, 50, ' '), 
					msgs.Received.lweek, msgs.Received.month[lmonth]);
			    lm += msgs.Received.month[lmonth];
			    lw += msgs.Received.lweek;
			} else
			    fseek(mp, msgshdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "---------------------------------------------------------------------------\r");
		    fprintf(qp, "Total %58lu %10lu\r\r\r", lw, lm);
		}
	    }
	}

	fclose(mp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " C  - You connected to this area\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void A_Status(faddr *, char *);
void A_Status(faddr *t, char *replyid)
{
    FILE    *fp;
    int	    i;

    Syslog('+', "AreaMgr: Status");
    if (Miy == 0)
	i = 11;
    else
	i = Miy - 1;

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"AreaMgr status", replyid)) != NULL) {

	fprintf(fp, "Here is your (echo)mail status:\r\r");

	fprintf(fp, "Netmail direct    %s\r", GetBool(nodes.Direct));
	fprintf(fp, "Netmail crash     %s\r", GetBool(nodes.Crash));
	fprintf(fp, "Netmail hold      %s\r", GetBool(nodes.Hold));
	if (nodes.RouteVia.zone)
	fprintf(fp, "Route via         %s\r", aka2str(nodes.RouteVia));

	fprintf(fp, "\r\rMailflow:\r\r");

	fprintf(fp, "                  Last week  Last month Total ever\r");
	fprintf(fp, "                  ---------- ---------- ----------\r");
	fprintf(fp, "Messages to you   %-10ld %-10ld %-10ld\r", nodes.MailSent.lweek, 
				    nodes.MailSent.month[i], nodes.MailSent.total);
	fprintf(fp, "Messages from you %-10ld %-10ld %-10ld\r", nodes.MailRcvd.lweek, 
				    nodes.MailRcvd.month[i], nodes.MailRcvd.total);

	fprintf(fp, "\rWith regards, %s\r\r", CFG.sysop_name);

	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void A_Unlinked(faddr *, char *);
void A_Unlinked(faddr *t, char *replyid)
{
    FILE	*qp, *gp, *mp;
    char	*temp, *Group;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[5];
    faddr	*f, *g;
    sysconnect	System;

    Syslog('+', "AreaMgr: Unlinked");
    f = bestaka_s(t);

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"Your unlinked request", replyid)) != NULL) {

	WriteMailGroups(qp, f);

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((mp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    return;
	}
	fread(&msgshdr, sizeof(msgshdr), 1, mp);
	Cons = msgshdr.syssize / sizeof(System);

	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    fclose(mp);
	    return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	free(temp);

	fprintf(qp, "The following is a list of all available message areas\r\r");

	while (TRUE) {
	    Group = GetNodeMailGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
		g = bestaka_s(fido2faddr(mgroup.UseAka));
		if ((!strcmp(mgroup.Name, Group)) &&
		    (g->zone  == f->zone) &&
		    (g->net   == f->net) &&
		    (g->node  == f->node) &&
		    (g->point == f->point)) {
		    SubTot = 0;
		    fprintf(qp, "Group %s - %s\r\r", mgroup.Name, mgroup.Comment);
		    fprintf(qp, "Con  Message area              Description\r");
		    fprintf(qp, "----------------------------------------------------------------------------\r");
		    fseek(mp, msgshdr.hdrsize, SEEK_SET);

		    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
			if (!strcmp(Group, msgs.Group) && msgs.Active) {
			    memset(&Stat, ' ', sizeof(Stat));
			    Stat[sizeof(Stat)-1] = '\0';

			    /*
			     * Now check if this node is connected, if so, set the Stat bits
			     */
			    for (i = 0; i < Cons; i++) {
				fread(&System, sizeof(System), 1, mp);
				if ((t->zone  == System.aka.zone) && (t->net   == System.aka.net) &&
				    (t->node  == System.aka.node) && (t->point == System.aka.point)) {
				    if (System.receivefrom)
					Stat[0] = 'S';
				    if (System.sendto)
					Stat[1] = 'R';
				    if (System.pause)
					Stat[2] = 'P';
				    if (System.cutoff)
					Stat[3] = 'C';
				}
			    }

			    if ((!System.sendto) && (!System.receivefrom)) {
				fprintf(qp, "%s %-25s %s\r", Stat, msgs.Tag, msgs.Name);
				SubTot++;
				Total++;
			    }
			} else
			    fseek(mp, msgshdr.syssize, SEEK_CUR);
		    }

		    fprintf(qp, "----------------------------------------------------------------------------\r");
		    fprintf(qp, "%d available area(s)\r\r\r", SubTot);
		}
	    }
	}

	fclose(mp);
	fclose(gp);

	fprintf(qp, "Con means:\r");
	fprintf(qp, " R  - You receive mail from my system\r");
	fprintf(qp, " S  - You may send mail to my system\r");
	fprintf(qp, " P  - The message area is temporary paused\r");
	fprintf(qp, " C  - You are cutoff from this area\r\r");
	fprintf(qp, "With regards, %s\r\r", CFG.sysop_name);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
	net_out++;
    } else
	WriteError("Can't create netmail");
}



void A_Global(faddr *, char *, FILE *);
void A_Global(faddr *t, char *Cmd, FILE *tmp)
{
    ShiftBuf(Cmd, 1);
    Syslog('m', "  AreaMgr node %s global %s", ascfnode(t, 0x1f), Cmd);
}



void A_Disconnect(faddr *, char *, FILE *);
void A_Disconnect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group;
    faddr	*b;
    sysconnect	Sys;

    Syslog('+', "AreaMgr: \"%s\"", Area);
    ShiftBuf(Area, 1);
    for (i=0; i < strlen(Area); i++ ) 
	Area[i]=toupper(Area[i]);

    if (!SearchMsgs(Area)) {
	fprintf(tmp, "Area %s not found\n", Area);
	Syslog('+', "  Area not found");
	return;
    }

    Syslog('m', "  Found %s group %s", msgs.Tag, mgroup.Name);

    First = TRUE;
    while ((Group = GetNodeMailGrp(First)) != NULL) {
	First = FALSE;
	if (strcmp(Group, mgroup.Name) == 0)
	    break;
    }
    if (Group == NULL) {
	fprintf(tmp, "You may not disconnect from area %s\n", Area);
	Syslog('+', "  Group %s not available for %s", mgroup.Name, ascfnode(t, 0x1f));
	return;
    }

    b = bestaka_s(t);
    i = metric(b, fido2faddr(mgroup.UseAka));
    Syslog('m', "Aka match level is %d", i);

    if (i > METRIC_POINT) {
	fprintf(tmp, "You may not disconnect area %s with nodenumber %s\n", Area, ascfnode(t, 0x1f));
	Syslog('+', "  %s may not disconnect from group %s", ascfnode(t, 0x1f), mgroup.Name);
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = FALSE;
    Sys.receivefrom = FALSE;

    if (!MsgSystemConnected(Sys)) {
	fprintf(tmp, "You are not connected to %s\n", Area);
	Syslog('+', "  %s is not connected to %s", ascfnode(t, 0x1f), Area);
	return;
    }

    if (MsgSystemConnect(&Sys, FALSE)) {

	/*
	 *  Make sure to write an overview afterwards.
	 */
	a_list = TRUE;
	Syslog('+', "Disconnected echo area %s", Area);
	fprintf(tmp, "Disconnected from area %s\n", Area);
	return;
    }

    fprintf(tmp, "You may not disconnect area %s\n", Area);
    Syslog('+', "Didn't disconnect %s from mandatory or cutoff echo area %s", ascfnode(t, 0x1f), Area);
}



void A_Connect(faddr *, char *, FILE *);
void A_Connect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group;
    faddr	*b;
    sysconnect	Sys;

    Syslog('+', "AreaMgr: \"%s\"", printable(Area, 0));

    if (Area[0] == '+')
	ShiftBuf(Area, 1);
    for (i=0; i < strlen(Area); i++ ) 
	Area[i]=toupper(Area[i]);

    if (!SearchMsgs(Area)) {
	fprintf(tmp, "Area %s not found\n", Area);
	Syslog('m', "  Area not found");
	/* SHOULD CHECK FOR AREAS FILE AND ASK UPLINK 
	   CHECK ALL GROUPRECORDS FOR AKA MATCH
	   IF MATCH CHECK FOR UPLINK AND AREAS FILE
	   IF FOUND, CREATE MSG AREA, CONNECT UPLINK
	   RESTORE NODERECORD (IS GONE!)
	   FALLTHRU TO CONNECT DOWNLINK
	*/
	return;
    }

    Syslog('m', "  Found %s group %s", msgs.Tag, mgroup.Name);

    First = TRUE;
    while ((Group = GetNodeMailGrp(First)) != NULL) {
	First = FALSE;
	if (strcmp(Group, mgroup.Name) == 0)
	    break;
    }
    if (Group == NULL) {
	fprintf(tmp, "You may not connect to area %s\n", Area);
	Syslog('+', "  Group %s not available for node %s", mgroup.Name, ascfnode(t, 0x1f));
	return;
    }

    b = bestaka_s(t);
    i = metric(b, fido2faddr(mgroup.UseAka));
    Syslog('m', "Aka match level is %d", i);

    if (i > METRIC_POINT) {
	fprintf(tmp, "You may not connect area %s with nodenumber %s\n", Area, ascfnode(t, 0x1f));
	Syslog('+', "  %s may not connect to group %s", ascfnode(t, 0x1f), mgroup.Name);
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = TRUE;
    Sys.receivefrom = TRUE;

    if (MsgSystemConnected(Sys)) {
	fprintf(tmp, "You are already connected to %s\n", Area);
	Syslog('+', "  %s is already connected to %s", ascfnode(t, 0x1f), Area);
	return;
    }

    if (MsgSystemConnect(&Sys, TRUE)) {

	/*
	 *  Make sure to write an overview afterwards.
	 */
	a_list = TRUE;
	Syslog('+', "Connected echo area %s", Area);
	fprintf(tmp, "Connected to area %s\n", Area);
	return;
    }

    fprintf(tmp, "Not connected to %s, this is not allowed\n", Area);
    WriteError("Can't connect node %s to echo area %s", ascfnode(t, 0x1f), Area);
}



void A_All(faddr *, int, FILE *, char *);
void A_All(faddr *t, int Connect, FILE *tmp, char *Grp)
{
    FILE	*mp, *gp;
    char	*Group, *temp;
    faddr	*f;
    int		i, Link, First = TRUE, Cons;
    sysconnect	Sys;
    long	Pos;

    if (Grp == NULL) {
	if (Connect)
	    Syslog('+', "AreaMgr: Connect All");
	else
	    Syslog('+', "AreaMgr: Disconnect All");
    } else {
	if (Connect)
	    Syslog('+', "AreaMgr: Connect group %s", Grp);
	else
	    Syslog('+', "AreaMgr: Disconnect group %s", Grp);
    }

    f = bestaka_s(t);
    Syslog('m', "Bestaka for %s is %s", ascfnode(t, 0x1f), ascfnode(f, 0x1f));

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((mp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, mp);
    Cons = msgshdr.syssize / sizeof(Sys);
    
    sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	fclose(mp);
	return;
    }
    fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
    free(temp);

    while (TRUE) {
	Group = GetNodeMailGrp(First);
	if (Group == NULL)
	    break;
	First = FALSE;

	fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
	    if ((!strcmp(mgroup.Name, Group)) && ((Grp == NULL) || (!strcmp(Group, Grp)))) {

		fseek(mp, msgshdr.hdrsize, SEEK_SET);

		while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
		    if ((!strcmp(Group, msgs.Group)) && (msgs.Active) && (!msgs.Mandatory) &&
			    (metric(fido2faddr(mgroup.UseAka), f) == METRIC_EQUAL)) {

			if (Connect) {
			    Link = FALSE;
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, mp);
				if (metric(fido2faddr(Sys.aka), t) == METRIC_EQUAL)
				    Link = TRUE;
			    }
			    if (!Link) {
				Pos = ftell(mp);
				fseek(mp, - msgshdr.syssize, SEEK_CUR);
				for (i = 0; i < Cons; i++) {
				    fread(&Sys, sizeof(Sys), 1, mp);
				    if (!Sys.aka.zone) {
					memset(&Sys, 0, sizeof(Sys));
					memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
					Sys.sendto = TRUE;
					Sys.receivefrom = TRUE;
					fseek(mp, - sizeof(Sys), SEEK_CUR);
					fwrite(&Sys, sizeof(Sys), 1, mp);
					Syslog('+', "AreaMgr: Connected %s", msgs.Tag);
					fprintf(tmp, "Connected area %s\n", msgs.Tag);
					a_list = TRUE;
					break;
				    }
				}
				fseek(mp, Pos, SEEK_SET);
			    }
			} else {
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, mp);
				if ((metric(fido2faddr(Sys.aka), t) == METRIC_EQUAL) && (!Sys.cutoff)) {
				    memset(&Sys, 0, sizeof(Sys));
				    fseek(mp, - sizeof(Sys), SEEK_CUR);
				    fwrite(&Sys, sizeof(Sys), 1, mp);
				    Syslog('+', "AreaMgr: Disconnected %s", msgs.Tag);
				    fprintf(tmp, "Disconnected area %s\n", msgs.Tag);
				    a_list = TRUE;
				}
			    }
			}
		    } else
			fseek(mp, msgshdr.syssize, SEEK_CUR);
		}
	    }
	}
    }
    fclose(gp);
    fclose(mp);
}



void A_Group(faddr *, char *, int, FILE *);
void A_Group(faddr *t, char *Area, int Connect, FILE *tmp)
{
    int	i;

    ShiftBuf(Area, 2);
    CleanBuf(Area);
    for (i = 0; i < strlen(Area); i++ ) 
	Area[i] = toupper(Area[i]);
    A_All(t, Connect, tmp, Area);
}



void A_Pause(faddr *, int, FILE *);
void A_Pause(faddr *t, int Pause, FILE *tmp)
{
    FILE	*mp;
    faddr	*f;
    int		i, Cons;
    sysconnect	Sys;
    char	*temp;

    if (Pause)
	Syslog('+', "AreaMgr: Pause");
    else
	Syslog('+', "AreaMgr: Resume");

    f = bestaka_s(t);
    Syslog('m', "Bestaka for %s is %s", ascfnode(t, 0x1f), ascfnode(f, 0x1f));

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((mp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }
    free(temp);
    fread(&msgshdr, sizeof(msgshdr), 1, mp);
    Cons = msgshdr.syssize / sizeof(Sys);

    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
	if (msgs.Active) {
	    for (i = 0; i < Cons; i++) {
		fread(&Sys, sizeof(Sys), 1, mp);
		if ((metric(fido2faddr(Sys.aka), t) == METRIC_EQUAL) && (!Sys.cutoff)) {
		    Sys.pause = Pause;
		    fseek(mp, - sizeof(Sys), SEEK_CUR);
		    fwrite(&Sys, sizeof(Sys), 1, mp);
		    Syslog('+', "AreaMgr: %s area %s",  Pause?"Pause":"Resume", msgs.Tag);
		    fprintf(tmp, "%s area %s\n", Pause?"Pause":"Resume", msgs.Tag);
		    a_list = TRUE;
		}
	    }
	} else {
	    fseek(mp, msgshdr.syssize, SEEK_CUR);
	}
    }
    fclose(mp);
}



void A_Rescan(faddr *, char *, FILE *);
void A_Rescan(faddr *t, char *Area, FILE *tmp)
{
    int	i,result;

    /*
     *  First strip leading garbage
     */
    ShiftBuf(Area, 7);
    CleanBuf(Area);
    for (i = 0; i < strlen(Area); i++ ) 
	Area[i] = toupper(Area[i]);
    Syslog('+', "AreaMgr: Rescan %s, MSGS=%lu", Area, a_msgs);
    result = RescanOne(t, Area, a_msgs);
    if (result == 0){
	if (a_msgs > 0) 
	    fprintf(tmp, "Rescan area %s, %lu last msgs.\n", Area, a_msgs);
	else
	    fprintf(tmp, "Rescan area %s \n", Area);
    } else if (result == 1)
	fprintf(tmp, "Can't rescan unknown area %s\n", Area);
    else if (result == 2)
	fprintf(tmp, "%s can't rescan area %s\n", ascfnode(t, 0x1f), Area);
    else
	fprintf(tmp, "Fatal Error Rescanning area %s\n", Area);         
} 



void A_Msgs(char *, int);
void A_Msgs(char *Buf, int skip)
{
    /*
     *  First strip leading garbage
     */
    ShiftBuf(Buf, skip);
    CleanBuf(Buf);
    a_msgs = strtoul( Buf, (char **)NULL, 10 );
    Syslog('+', "AreaMgr: msgs %s ", Buf );
}



int AreaMgr(faddr *f, faddr *t, char *replyid, char *subj, time_t mdate, int flags, FILE *fp)
{
    int		i, rc = 0, spaces;
    char	*Buf;
    FILE	*tmp, *np;
    fidoaddr	Node;

    a_help = a_stat = a_unlnk = a_list = a_query = FALSE;
    areamgr++;
    if (SearchFidonet(f->zone))
	f->domain = xstrcpy(fidonet.domain);

    Syslog('+', "AreaMgr msg from %s", ascfnode(f, 0xff));

    /*
     * If the password failed, we return silently and don't respond.
     */
    if ((!strlen(subj)) || (strcasecmp(subj, nodes.Apasswd))) {
	WriteError("AreaMgr: password expected \"%s\", got \"%s\"", nodes.Apasswd, subj);
	net_bad++;
	return FALSE;
    }

    if ((tmp = tmpfile()) == NULL) {
	WriteError("$AreaMgr: Can't open tmpfile()");
	net_bad++;
	return FALSE;
    }

    Buf = calloc(2049, sizeof(char));
    rewind(fp);

    while ((fgets(Buf, 2048, fp)) != NULL) {

	/*
	 * Make sure we have the nodes record loaded
	 */
	memcpy(&Node, faddr2fido(f), sizeof(fidoaddr));
	SearchNode(Node);

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
		a_help = TRUE;
	    else if (!strncasecmp(Buf, "%query", 6))
		a_query = TRUE;
	    else if (!strncasecmp(Buf, "%linked", 7))
		a_query = TRUE;
	    else if (!strncasecmp(Buf, "%list", 5))
		a_list = TRUE;
	    else if (!strncasecmp(Buf, "%status", 7))
		a_stat = TRUE;
	    else if (!strncasecmp(Buf, "%unlinked", 9))
		a_unlnk = TRUE;
	    else if (!strncasecmp(Buf, "%flow", 5))
		a_flow = TRUE;
	    else if (!strncasecmp(Buf, "%msgs", 5))
		A_Msgs(Buf, 5);
	    else if (!strncasecmp(Buf, "%rescan", 7))
		A_Rescan(f, Buf, tmp);
	    else if (!strncasecmp(Buf, "%+all", 5))
		A_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-all", 5))
		A_All(f, FALSE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%+*", 3))
		A_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-*", 3))
		A_All(f, FALSE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%+", 2))
		A_Group(f, Buf, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%-", 2))
		A_Group(f, Buf, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%pause", 6))
		A_Pause(f, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%resume", 7))
		A_Pause(f, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%password", 9))
		MgrPasswd(f, Buf, tmp, 9);
	    else if (!strncasecmp(Buf, "%pwd", 4))
		MgrPasswd(f, Buf, tmp, 4);
	    else if (!strncasecmp(Buf, "%notify", 7))
		MgrNotify(f, Buf, tmp);
	    else if (*(Buf) == '%')
		A_Global(f, Buf, tmp);
	    else if (*(Buf) == '-')
		A_Disconnect(f, Buf, tmp);
	    else
		A_Connect(f, Buf, tmp);
	}
    }

    /*
     *  If the temporary response file isn't empty,
     *  create a response netmail about what we did.
     */
    if (ftell(tmp)) {
	if ((np = SendMgrMail(f, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", (char *)"Your AreaMgr request", replyid)) != NULL) {

	    fprintf(np, "     Dear %s\r\r", nodes.Sysop);
	    fprintf(np, "Here is the result of your AreaMgr request:\r\r");
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

    if (a_stat)
	A_Status(f, replyid);

    if (a_query)
	A_Query(f, replyid);

    if (a_list)
	A_List(f, replyid, FALSE);

    if (a_flow)
	A_Flow(f, replyid, FALSE);

    if (a_unlnk)
	A_Unlinked(f, replyid);

    if (a_help)
	A_Help(f, replyid);

    return rc;
}



