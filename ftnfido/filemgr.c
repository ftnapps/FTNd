/*****************************************************************************
 *
 * $Id: filemgr.c,v 1.41 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: FileMgr
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/mbsedb.h"
#include "../lib/diesel.h"
#include "sendmail.h"
#include "mgrutil.h"
#include "createf.h"
#include "filemgr.h"



/*
 * External declarations
 */
extern	int	do_quiet;



/*
 * Global variables
 */
extern	int	net_bad;		/* Bad netmails (tracking errors    */

int	filemgr = 0;			/* Nr of FileMgr messages	    */
int	f_help  = FALSE;		/* Send FileMgr help		    */
int	f_list	= FALSE;		/* Send FileMgr list		    */
int	f_query = FALSE;		/* Send FileMgr query		    */
int	f_stat	= FALSE;		/* Send FileMgr status		    */
int	f_unlnk	= FALSE;		/* Send FileMgr unlink		    */



void F_Help(faddr *, char *);
void F_Help(faddr *t, char *replyid)
{
    FILE	*fp, *fi;
    char	*subject;

    Mgrlog("FileMgr: Help");
    subject=calloc(255,sizeof(char));
    snprintf(subject,255,"FileMgr help");
    GetRpSubject("filemgr.help",subject,254);

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", subject, replyid)) != NULL) {
	if ((fi = OpenMacro("filemgr.help", nodes.Language, FALSE)) != NULL ){
	    MacroVars("s", "s", nodes.Sysop);
	    MacroVars("A", "s", (char *)"Filemgr");
	    MacroVars("Y", "s", ascfnode(bestaka_s(t), 0xf));
	    MacroVars("P", "s", nodes.Fpasswd);
	    MacroRead(fi, fp);
	    MacroClear();
	    fclose(fi);
	}
	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
    } else
	WriteError("Can't create netmail");
    free(subject);
}



void F_Query(faddr *, char *);
void F_Query(faddr *t, char *replyid)
{
    F_List(t, replyid, LIST_QUERY);
}



void F_List(faddr *t, char *replyid, int Notify)
{
    FILE	*qp, *gp, *fp, *fi = NULL;
    char	*temp, *Group, *subject;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[4];
    faddr	*f, *g, *Temp;
    sysconnect	System;
    int		msgptr;
    fpos_t      fileptr,fileptr1,fileptr2;

    subject = calloc(255, sizeof(char));
    f = bestaka_s(t);
    MacroVars("s", "s", nodes.Sysop);
    MacroVars("K", "d", Notify);
    MacroVars("y", "s", ascfnode(t, 0xff));
    MacroVars("Y", "s", ascfnode(f, 0xff));
 
    switch (Notify) {
	case LIST_NOTIFY:   Mgrlog("FileMgr: Notify to %s", ascfnode(t, 0xff));
			    snprintf(subject,255,"FileMgr Notify");
			    GetRpSubject("filemgr.notify.list",subject,255);
			    fi=OpenMacro("filemgr.notify.list", nodes.Language, FALSE);
			    break;
	case LIST_LIST:	    Mgrlog("FileMgr: List");
			    snprintf(subject,255,"FileMgr list");
			    GetRpSubject("filemgr.list",subject,255);
			    fi=OpenMacro("filemgr.list", nodes.Language, FALSE);
			    break;
	case LIST_QUERY:    Mgrlog("FileMgr: Query");
			    snprintf(subject,255,"FileMgr Query");
			    GetRpSubject("filemgr.query",subject,255);
			    fi=OpenMacro("filemgr.query", nodes.Language, FALSE);
			    break;
	default:	    Mgrlog("FileMgr: Unlinked");
			    snprintf(subject,255,"FileMgr: Unlinked areas");
			    GetRpSubject("filemgr.unlink",subject,254);
			    fi=OpenMacro("filemgr.unlink", nodes.Language, FALSE);
			    break;
    }

    if (fi == NULL) {
	MacroClear();
	free(subject);
	return;
    }

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", subject, replyid)) != NULL) {

	/*
	 * Mark begin of message in .pkt
	 */
	msgptr = ftell(qp);

	if ((Notify == LIST_LIST) || (Notify == LIST_NOTIFY))
            WriteFileGroups(qp, f);

	MacroRead(fi, qp);
	fgetpos(fi,&fileptr);
	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    free(subject);
	    MacroClear();
	    fclose(fi);
	    return;
	}
	fread(&tichdr, sizeof(tichdr), 1, fp);
	Cons = tichdr.syssize / sizeof(System);

	snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    free(subject);
	    MacroClear(); 
	    fclose(fp);
	    fclose(fi);
	    return;
	}
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);
	free(temp);

	while (TRUE) {
	    Group = GetNodeFileGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
		Temp = fido2faddr(fgroup.UseAka);
		g = bestaka_s(Temp);
		if ((!strcmp(fgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) && (g->node  == f->node) && (g->point == f->point)) {
		    SubTot = 0;
		    MacroVars("G", "s", fgroup.Name);
		    MacroVars("J", "s", fgroup.Comment);
		    MacroVars("I", "s", aka2str(fgroup.UseAka));
		    fsetpos(fi,&fileptr);
		    MacroRead(fi, qp);
		    fgetpos(fi,&fileptr1);
		    fseek(fp, tichdr.hdrsize, SEEK_SET);

		    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
			if (!strcmp(Group, tic.Group) && tic.Active && Access(nodes.Security, tic.LinkSec)) {
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
			    if (    (Notify == LIST_LIST)
			         || (Notify == LIST_NOTIFY)
			         || ((Notify == LIST_QUERY) && ((Stat[0]=='S') || (Stat[1]=='R')))
			         || ((Notify >= LIST_UNLINK) && ((Stat[0]!='S') && (Stat[1]!='R')))) {
				MacroVars("X", "s", Stat);
				MacroVars("D", "s", tic.Name);
				MacroVars("E", "s", tic.Comment);
				MacroVars("s", "d", (Stat[0] == 'S'));
				MacroVars("r", "d", (Stat[1] == 'R'));
				MacroVars("p", "d", (Stat[2] == 'P'));
				fsetpos(fi,&fileptr1);
				MacroRead(fi, qp);
				fgetpos(fi,&fileptr2);
			    	SubTot++;
			    	Total++;
				/*
				 * Panic message split
				 */
				if (((ftell(qp) - msgptr) / 1024) >= CFG.new_force) {
				    MacroVars("Z","d",1);
				    Syslog('-', "  Forced splitting message at %ld bytes", ftell(qp) - msgptr);
				    CloseMail(qp, t);
				    qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", subject, replyid);
				    msgptr = ftell(qp);
				}
			    }
			} else
			    fseek(fp, tichdr.syssize, SEEK_CUR);
		    }
		    MacroVars("ZA", "dd", (int) 0 , SubTot );
		    fsetpos(fi,&fileptr2);
		    if (((ftell(qp) - msgptr) / 1024) >= CFG.new_split) {
			MacroVars("Z","d",1);
			Syslog('-', "  Splitting message at %ld bytes", ftell(qp) - msgptr);
			CloseMail(qp, t);
			qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", subject, replyid);
			msgptr = ftell(qp);
		    }
		    MacroRead(fi, qp);
		}
		tidy_faddr(Temp);
	    }
	}

 	MacroVars("B", "d", Total );
	MacroRead(fi, qp);
 	MacroClear();
 	fclose(fi);
	fclose(fp);
	fclose(gp);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
    } else
	WriteError("Can't create netmail");
    free(subject);
    MacroClear();
}



void F_Status(faddr *, char *);
void F_Status(faddr *t, char *replyid)
{
    FILE    *fp, *fi;
    int	    i;
    char    *subject;

    subject = calloc(255, sizeof(char));
    snprintf(subject,255,"FileMgr Status");
    Mgrlog("FileMgr: Status");
    if (Miy == 0)
	i = 11;
    else
	i = Miy - 1;
    MacroVars("A", "d", nodes.Message);
    MacroVars("B", "d", nodes.Tic);
    MacroVars("C", "d", nodes.AdvTic);
    MacroVars("D", "d", nodes.Notify);
    MacroVars("a", "d", nodes.FilesSent.lweek);
    MacroVars("b", "d", nodes.FilesSent.month[i]);
    MacroVars("c", "d", nodes.FilesSent.total);
    MacroVars("d", "d", nodes.F_KbSent.lweek);
    MacroVars("e", "d", nodes.F_KbSent.month[i]);
    MacroVars("f", "d", nodes.F_KbSent.total);
    MacroVars("g", "d", nodes.FilesRcvd.lweek);
    MacroVars("h", "d", nodes.FilesRcvd.month[i]);
    MacroVars("i", "d", nodes.FilesRcvd.total);
    MacroVars("j", "d", nodes.F_KbRcvd.lweek);
    MacroVars("k", "d", nodes.F_KbRcvd.month[i]);
    MacroVars("l", "d", nodes.F_KbRcvd.total);
    MacroVars("s", "s", nodes.Sysop);
    GetRpSubject("filemgr.status",subject,254);

    if ((fi = OpenMacro("filemgr.status", nodes.Language, FALSE)) == NULL ) {
	free(subject);
	MacroClear();
	return;
    }

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", subject, replyid)) != NULL) {
	MacroRead(fi, fp);
 	MacroClear();
 	fclose(fi);
	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
    } else
	WriteError("Can't create netmail");
    MacroClear();
    free(subject);
}



void F_Unlinked(faddr *, char *);
void F_Unlinked(faddr *t, char *replyid)
{
    F_List(t, replyid, LIST_UNLINK);
}



void F_Disconnect(faddr *, char *, FILE *);
void F_Disconnect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group;
    faddr	*b, *Temp;
    sysconnect	Sys;

    Mgrlog("FileMgr: disconnect %s", Area);
    ShiftBuf(Area, 1);
    for (i = 0; i < strlen(Area); i++)
	Area[i] = toupper(Area[i]);
    
    if (!SearchTic(Area)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_NOTFOUND",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Syslog('+', "  Area not found");
	MacroClear();
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
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_NOTGROUP",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  Group %s not available for %s", fgroup.Name, ascfnode(t, 0x1f));
	MacroClear();
	return;
    }

    b = bestaka_s(t);
    Temp = fido2faddr(tic.Aka);
    i = metric(b, Temp);
    tidy_faddr(Temp);
    Syslog('m', "Aka match level is %d", i);

    if (i >= METRIC_NET) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_BADADD",Area,ascfnode(t, 0x1f),"","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  %s may not disconnect from group %s", ascfnode(t, 0x1f), fgroup.Name);
	MacroClear();
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = FALSE;
    Sys.receivefrom = FALSE;

    if (!TicSystemConnected(Sys)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_NC",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  %s is not connected to %s", ascfnode(t, 0x1f), Area);
	MacroClear();
	return;
    }

    if (TicSystemConnect(&Sys, FALSE)) {

	/*
	 * Make sure to write an overview afterwards
	 */
	f_list = TRUE;
	Mgrlog("Disconnected file area %s", Area);
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "ssssss","OK_DISC",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	MacroClear();
	return;
    }

    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
    MacroVars("RABCDE", "ssssss","ERR_DISC_NOTAVAIL",Area,"","","","");
    MsgResult("filemgr.responses",tmp,'\n');
    Mgrlog("Didn't disconnect %s from mandatory file area %s", ascfnode(t, 0x1f), Area);
    MacroClear();
}



void F_Connect(faddr *, char *, FILE *);
void F_Connect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group, *temp;
    faddr	*b, *Temp;
    sysconnect	Sys;
    FILE	*gp;

    Mgrlog("FileMgr: connect %s", Area);

    if (Area[0] == '+')
	ShiftBuf(Area, 1);
    for (i = 0; i < strlen(Area); i++)
	Area[i] = toupper(Area[i]);

    if (!SearchTic(Area)) {
	/*
	 * Close noderecord, autocreate will destroy it.
	 */
	UpdateNode();

	Syslog('f', "  Area not found, trying to create");
	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    return;
	}
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);
	fseek(gp, fgrouphdr.hdrsize, SEEK_SET);

	while ((fread(&fgroup, fgrouphdr.recsize, 1, gp)) == 1) {
	    if ((fgroup.UseAka.zone == t->zone) && (fgroup.UseAka.net == t->net) && fgroup.UpLink.zone &&
		strlen(fgroup.AreaFile) && fgroup.Active && fgroup.UserChange) {
		if (CheckTicGroup(Area, fgroup.UpLink.zone, t) == 0) {
		    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
		    MacroVars("RABCDE", "ssssss","ERR_CONN_FORWARD",Area,aka2str(fgroup.UpLink),"","","");
		    MsgResult("filemgr.responses",tmp,'\n');
		    break;
		}
	    }
	}
	fclose(gp);
	free(temp);

	/*
	 * Restore noderecord and try to load area again
	 */
	SearchNodeFaddr(t);
	if (!SearchTic(Area)) {
	    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Filemgr");
	    MacroVars("RABCDE", "ssssss","ERR_CONN_NOTFOUND",Area,"","","","");
	    MsgResult("filemgr.responses",tmp,'\n');
	    Mgrlog("Area %s not found", Area);
	    MacroClear();
	    return;
	}
    }

    Syslog('m', "  Found %s group %s", tic.Name, fgroup.Name);

    First = TRUE;
    while ((Group = GetNodeFileGrp(First)) != NULL) {
	First = FALSE;
	if (strcmp(Group, fgroup.Name) == 0)
	    break;
    }
    if (Group == NULL) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_CONN_NOTGROUP",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  Group %s not available for %s", fgroup.Name, ascfnode(t, 0x1f));
	MacroClear();
	return;
    }

    b = bestaka_s(t);
    Temp = fido2faddr(tic.Aka);
    i = metric(b, Temp);
    tidy_faddr(Temp);
    Syslog('m', "Aka match level is %d", i);

    if (i >= METRIC_NET) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_CONN_BADADD",Area,ascfnode(t, 0x1f),"","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  Node %s may not connect to group %s", ascfnode(t, 0x1f), fgroup.Name);
	MacroClear();
	return;
    }

    if (! Access(nodes.Security, tic.LinkSec)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	/*
	 * If node has no access by flags, we lie and say "Area not found"
	 */
	MacroVars("RABCDE", "ssssss","ERR_CONN_NOTFOUND",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  %s has no access to %s", ascfnode(t, 0x1f), Area);
	MacroClear();
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = TRUE;
    if (tic.NewSR)
	Sys.receivefrom = TRUE;

    if (TicSystemConnected(Sys)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "ssssss","ERR_CONN_ALREADY",Area,"","","","");
	MsgResult("filemgr.responses",tmp,'\n');
	Mgrlog("  %s is already connected to %s", ascfnode(t, 0x1f), Area);
	MacroClear();
	return;
    }

    if (TicSystemConnect(&Sys, TRUE)) {

	/*
	 * Make sure to write an overview afterwards
	 */
	f_list = TRUE;
	Mgrlog("Connected to file area %s", Area);
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "ssssss","OK_CONN",Area,aka2str(tic.Aka),"","","");
	MsgResult("filemgr.responses",tmp,'\n');
	MacroClear();
	return;
    }
    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
    MacroVars("RABCDE", "ssssss","ERR_CONN_NOTAVAIL",Area,"","","","");
    MsgResult("filemgr.responses",tmp,'\n');
    WriteError("Can't connect node %s to file area %s", ascfnode(t, 0x1f), Area);
    MacroClear();
}



void F_All(faddr *, int, FILE *, char *);
void F_All(faddr *t, int Connect, FILE *tmp, char *Grp)
{
    FILE	*fp, *gp;
    char	*Group, *temp;
    faddr	*f, *Temp;
    int		i, Link, First = TRUE, Cons;
    sysconnect	Sys;
    int		Pos;

    if (Grp == NULL) {
	if (Connect)
	    Mgrlog("FileMgr: Connect All");
	else
	    Mgrlog("FileMgr: Disconnect All");
    } else {
	if (Connect)
	    Mgrlog("FileMgr: Connect group %s", Grp);
	else
	    Mgrlog("FileMgr: Disconnect group %s", Grp);
    }

    f = bestaka_s(t);
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }
    fread(&tichdr, sizeof(tichdr), 1, fp);
    Cons = tichdr.syssize / sizeof(Sys);

    snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	fclose(fp);
	return;
    }
    fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);
    free(temp);

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

		    Temp = fido2faddr(tic.Aka);
		    if ((!strcmp(Group, tic.Group)) && tic.Active && strlen(tic.Name) &&
			(metric(Temp, f) < METRIC_NET) && Access(nodes.Security, tic.LinkSec)) {

			if (Connect) {
			    Link = FALSE;
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, fp);
				tidy_faddr(Temp);
				Temp = fido2faddr(Sys.aka);
				if (metric(Temp, t) == METRIC_EQUAL)
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
					if (tic.NewSR)
					    Sys.receivefrom = TRUE;
					fseek(fp, - sizeof(Sys), SEEK_CUR);
					fwrite(&Sys, sizeof(Sys), 1, fp);
					Mgrlog("FileMgr: Connected %s", tic.Name);
					MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
		    			MacroVars("RABCDE", "ssssss","OK_CONN",tic.Name,aka2str(tic.Aka),"","","");
		    			MsgResult("filemgr.responses",tmp,'\n');
					MacroClear();
					f_list = TRUE;
					break;
				    }
				}
				fseek(fp, Pos, SEEK_SET);
			    }
			} else {
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, fp);
				tidy_faddr(Temp);
				Temp = fido2faddr(Sys.aka);
				if (metric(Temp, t) == METRIC_EQUAL) {
				    memset(&Sys, 0, sizeof(Sys));
				    fseek(fp, - sizeof(Sys), SEEK_CUR);
				    fwrite(&Sys, sizeof(Sys), 1, fp);
				    Mgrlog("FileMgr: Disconnected %s", tic.Name);
				    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
		    		    MacroVars("RABCDE", "ssssss","OK_DISC",tic.Name,"","","","");
		    		    MsgResult("filemgr.responses",tmp,'\n');
				    MacroClear();
				    f_list = TRUE;
				}
			    }
			}
		    } else
			fseek(fp, tichdr.syssize, SEEK_CUR);
		    tidy_faddr(Temp);
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
    int	    i;
    
    ShiftBuf(Area, 2);
    CleanBuf(Area);
    for (i = 0; i < strlen(Area); i++)
	Area[i] = toupper(Area[i]);
    F_All(t, Connect, tmp, Area);
}



void F_Pause(faddr *, int, FILE *);
void F_Pause(faddr *t, int Pause, FILE *tmp)
{
    FILE	*fp;
    faddr	*f, *Temp;
    int		i, Cons;
    sysconnect	Sys;
    char	*temp;
    
    if (Pause)
	Mgrlog("FileMgr: Pause");
    else
	Mgrlog("FileMgr: Resume");

    f = bestaka_s(t);
    Syslog('m', "Bestaka for %s is %s", ascfnode(t, 0x1f), ascfnode(f, 0x1f));

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }
    fread(&tichdr, sizeof(tichdr), 1, fp);
    Cons = tichdr.syssize / sizeof(Sys);

    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
	if (tic.Active) {
	    for (i = 0; i < Cons; i++) {
		fread(&Sys, sizeof(Sys), 1, fp);
		Temp = fido2faddr(Sys.aka);
		if ((metric(Temp, t) == METRIC_EQUAL) && (!Sys.cutoff)) {
		    Sys.pause = Pause;
		    fseek(fp, - sizeof(Sys), SEEK_CUR);
		    fwrite(&Sys, sizeof(Sys), 1, fp);
		    Mgrlog("FileMgr: %s area %s",  Pause?"Pause":"Resume", tic.Name);
		    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"FileMgr");
		    MacroVars("RABCDE", "ssdsss","OK_PAUSE",tic.Name,Pause,"","","");
		    MsgResult("filemgr.responses",tmp,'\n');
		    f_list = TRUE;
		}
		tidy_faddr(Temp);
	    }
	} else {
	    fseek(fp, tichdr.syssize, SEEK_CUR);
	}
    }
    fclose(fp);
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
    Mgrlog("FileMgr: Message %s", nodes.Message?"Yes":"No");
    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
    MacroVars("RABCDE", "sdssss","OK_MESSAGE",nodes.Message,"","","","");
    MsgResult("filemgr.responses",tmp,'\n');
    MacroClear();
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
    Mgrlog("FileMgr: Tick %s, Advanced %s", nodes.Tic?"Yes":"No", nodes.AdvTic?"Yes":"No");
    if (nodes.Tic)
	if (nodes.AdvTic){
	    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	    MacroVars("RABCDE", "sddsss","OK_TIC_ADV",nodes.Tic,nodes.AdvTic,"","","");
	    MsgResult("filemgr.responses",tmp,'\n');
	}else{
	    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	    MacroVars("RABCDE", "sddsss","OK_TIC_NORM",nodes.Tic,nodes.AdvTic,"","","");
	    MsgResult("filemgr.responses",tmp,'\n');
    }else{
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "sddsss","OK_TIC_OFF",nodes.Tic,nodes.AdvTic,"","","");
	MsgResult("filemgr.responses",tmp,'\n');
    }
    MacroClear();
}



int FileMgr(faddr *f, faddr *t, char *replyid, char *subj, time_t mdate, int flags, FILE *fp)
{
    int		i, rc = 0, spaces;
    char	*Buf, *subject;
    FILE	*tmp, *np;

    f_help = f_stat = f_unlnk = f_list = f_query = FALSE;
    filemgr++;
    if (SearchFidonet(f->zone))
	f->domain = xstrcpy(fidonet.domain);

    Mgrlog("FileMgr request from %s start", ascfnode(f, 0xff));

    /*
     * If the password failed, we return silently and don't respond.
     */
    if ((!strlen(subj)) || (strcasecmp(subj, nodes.Fpasswd))) {
	WriteError("FileMgr: password expected \"%s\", got \"%s\"", nodes.Fpasswd, subj);
	Mgrlog("FileMgr: password expected \"%s\", got \"%s\"", nodes.Fpasswd, subj);
	Mgrlog("FileMgr request from %s finished", ascfnode(f, 0xff));
	net_bad++;
	return FALSE;
    }

    if ((tmp = tmpfile()) == NULL) {
	WriteError("$FileMsr: Can't open tmpfile()");
	net_bad++;
	return FALSE;
    }
    
    Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
    rewind(fp);

    while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {

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
	    else if (!strncasecmp(Buf, "%+all", 5) && CFG.ct_PlusAll)
		F_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-all", 5) && CFG.ct_PlusAll)
		F_All(f, FALSE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%+*", 3) && CFG.ct_PlusAll)
		F_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-*", 3) && CFG.ct_PlusAll)
		F_All(f, FALSE, tmp, NULL); 
	    else if (!strncasecmp(Buf, "%+", 2))
		F_Group(f, Buf, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%-", 2))
		F_Group(f, Buf, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%pause", 6) && CFG.ct_Pause)
		F_Pause(f, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%passive", 8) && CFG.ct_Pause)
		F_Pause(f, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%resume", 7) && CFG.ct_Pause)
		F_Pause(f, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%active", 7) && CFG.ct_Pause)
		F_Pause(f, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%password", 9) && CFG.ct_Passwd)
		MgrPasswd(f, Buf, tmp, 9, 1);
	    else if (!strncasecmp(Buf, "%pwd", 4) && CFG.ct_Passwd)
		MgrPasswd(f, Buf, tmp, 4, 1);
	    else if (!strncasecmp(Buf, "%notify", 7) && CFG.ct_Notify)
		MgrNotify(f, Buf, tmp, 1);
	    else if (!strncasecmp(Buf, "%message", 8) && CFG.ct_Message)
		F_Message(f, Buf, tmp);
	    else if (!strncasecmp(Buf, "%tick", 5) && CFG.ct_TIC)
		F_Tick(f, Buf, tmp);
	    else if (*(Buf) == '-')
		F_Disconnect(f, Buf, tmp);
	    else
		F_Connect(f, Buf, tmp);
	}
    }

    if (ftell(tmp)) {
        subject=calloc(256,sizeof(char));
        MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Filemgr");
	MacroVars("RABCDE", "ssssss","","","","","","");
	snprintf(subject,256,"Your FileMgr request");
	GetRpSubject("filemgr.responses",subject,256);
	if ((np = SendMgrMail(f, CFG.ct_KeepMgr, FALSE, (char *)"Filemgr", subject, replyid)) != NULL) {
	    MacroVars("RABCDE", "ssssss","WELLCOME","","","","","");
	    MsgResult("filemgr.responses",np,'\r');
	    fprintf(np, "\r");
	    fseek(tmp, 0, SEEK_SET);

	    while ((fgets(Buf, MAX_LINE_LENGTH, tmp)) != NULL) {
		while ((Buf[strlen(Buf) - 1]=='\n') || (Buf[strlen(Buf) - 1]=='\r')) {
		    Buf[strlen(Buf) - 1] = '\0';
		}
		fprintf(np, "%s\r", Buf);
	    }

	    fprintf(np, "\r");
	    MacroVars("RABCDE", "ssssss","GOODBYE","","","","","");
	    MsgResult("filemgr.responses",np,'\r');
	    fprintf(np, "\r%s\r", TearLine());
	    CloseMail(np, t);
	} else 
	    WriteError("Can't create netmail");
	free(subject);    
    }
    MacroClear();
    free(Buf);
    fclose(tmp);

    if (f_stat)
	F_Status(f, replyid);

    if (f_query)
	F_Query(f, replyid);

    if (f_list)
	F_List(f, replyid, LIST_LIST);

    if (f_unlnk)
	F_Unlinked(f, replyid);

    if (f_help)
	F_Help(f, replyid);

    Mgrlog("FileMgr request from %s finished", ascfnode(f, 0xff));
    return rc;
}

