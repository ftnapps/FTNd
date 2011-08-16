/*****************************************************************************
 *
 * $Id: areamgr.c,v 1.44 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: AreaMgr
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
#include "scan.h"
#include "createm.h"
#include "areamgr.h"




/*
 * External declarations
 */
extern	int	do_quiet;



/*
 * Global variables
 */
extern	int	net_bad;		/* Bad netmails (tracking errors    */

int		areamgr = 0;		/* Nr of AreaMgr messages	    */
int		a_help	= FALSE;	/* Send AreaMgr help		    */
int		a_list	= FALSE;	/* Send AreaMgr list		    */
int		a_query	= FALSE;	/* Send AreaMgr query		    */
int		a_stat	= FALSE;	/* Send AreaMgr status		    */
int		a_unlnk	= FALSE;	/* Send AreaMgr unlinked	    */
int		a_flow  = FALSE;	/* Send AreaMgr flow		    */
unsigned int	a_msgs = 0;		/* Messages to rescan		    */



void A_Help(faddr *, char *);
void A_Help(faddr *t, char *replyid)
{
    FILE    *fp, *fi;
    char    *subject;
    faddr   *ta;

    Mgrlog("AreaMgr: Help");
    
    subject = calloc(255, sizeof(char));
    snprintf(subject,255,"AreaMgr Help");
    GetRpSubject("areamgr.help",subject,255);

    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject , replyid)) != NULL) {
	if ((fi = OpenMacro("areamgr.help", nodes.Language, FALSE)) != NULL ) {
	    ta = bestaka_s(t);
	    MacroVars("s", "s", nodes.Sysop);
	    MacroVars("A", "s", (char *)"Areamgr");
	    MacroVars("Y", "s", ascfnode(ta, 0xff));
	    MacroVars("P", "s", nodes.Apasswd);
	    tidy_faddr(ta);
	    MacroRead(fi, fp);
	    fclose(fi);
	}

	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
    } else {
	WriteError("Can't create netmail");
    }

    free(subject);
    MacroClear();
}



void A_Query(faddr *, char *);
void A_Query(faddr *t, char *replyid)
{
    A_List(t, replyid, LIST_QUERY);
}



void A_List(faddr *t, char *replyid, int Notify)
{
    FILE	*qp, *gp, *mp, *fi = NULL;
    char	*temp, *Group, *subject;
    int		i, First = TRUE, SubTot, Total = 0, Cons;
    char	Stat[5];
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
	case LIST_NOTIFY:   Mgrlog("AreaMgr: Notify to %s", ascfnode(t, 0xff));
			    snprintf(subject,255,"AreaMgr Notify");
			    GetRpSubject("areamgr.notify.list",subject,255);
			    fi = OpenMacro("areamgr.notify.list", nodes.Language, FALSE);
			    break;
	case LIST_LIST:	    Mgrlog("AreaMgr: List");
			    snprintf(subject,255,"AreaMgr list");
			    GetRpSubject("areamgr.list",subject,255);
			    fi = OpenMacro("areamgr.list", nodes.Language, FALSE);
			    break;
	case LIST_QUERY:    Mgrlog("AreaMgr: Query");
			    snprintf(subject,255,"AreaMgr Query");
			    GetRpSubject("areamgr.query",subject,255);
			    fi = OpenMacro("areamgr.query", nodes.Language, FALSE);
			    break;
	case LIST_UNLINK:   Mgrlog("AreaMgr: Unlinked");
			    snprintf(subject,255,"AreaMgr: Unlinked areas");
			    GetRpSubject("areamgr.unlink",subject,255);
			    fi = OpenMacro("areamgr.unlink", nodes.Language, FALSE);
			    break;
    }

    if (fi == NULL) {
	MacroClear();
	free(subject);
	return;
    }

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid)) != NULL) {

        /*
         * Mark begin of message in .pkt
         */
        msgptr = ftell(qp);

	if ((Notify == LIST_LIST) || (Notify == LIST_NOTIFY))
	    WriteMailGroups(qp, f);

	MacroRead(fi, qp);
	fgetpos(fi,&fileptr);

	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((mp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    free(subject);
	    MacroClear(); 
	    fclose(fi);
	    return;
	}
	fread(&msgshdr, sizeof(msgshdr), 1, mp);
	Cons = msgshdr.syssize / sizeof(System);

	snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    free(subject);
	    MacroClear(); 
	    fclose(mp);
	    fclose(fi);
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
		Temp = fido2faddr(mgroup.UseAka);
		g = bestaka_s(Temp);
		tidy_faddr(Temp);
		if ((!strcmp(mgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) && (g->node  == f->node) && (g->point == f->point)) {
		    SubTot = 0;
		    MacroVars("G", "s", mgroup.Name);
		    MacroVars("J", "s", mgroup.Comment);
		    MacroVars("I", "s", aka2str(mgroup.UseAka) );
		    fsetpos(fi,&fileptr);
		    MacroRead(fi, qp);
		    fgetpos(fi,&fileptr1);
		    fseek(mp, msgshdr.hdrsize, SEEK_SET);

		    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
			if (!strcmp(Group, msgs.Group) && msgs.Active && Access(nodes.Security, msgs.LinkSec)) {
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
			    if (    (Notify == LIST_LIST) || (Notify == LIST_NOTIFY)
			         || ((Notify == LIST_QUERY) && ((Stat[0]=='S') || (Stat[1]=='R')))
			         || ((Notify >= LIST_UNLINK) && ((Stat[0]!='S') && (Stat[1]!='R')))){
				MacroVars("X", "s", Stat);
				MacroVars("D", "s", msgs.Tag);
				MacroVars("E", "s", msgs.Name);
				MacroVars("s", "d", (Stat[0] == 'S'));
				MacroVars("r", "d", (Stat[1] == 'R'));
				MacroVars("p", "d", (Stat[2] == 'P'));
				MacroVars("c", "d", (Stat[3] == 'C'));
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
				    qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid);
				    msgptr = ftell(qp);
				}
			    }
			} else
			    fseek(mp, msgshdr.syssize, SEEK_CUR);
		    }
 	    	    MacroVars("ZC", "dd", (int) 0 , SubTot );
		    fsetpos(fi,&fileptr2);
		    if (((ftell(qp) - msgptr) / 1024) >= CFG.new_split) {
			MacroVars("Z","d",1);
			Syslog('-', "  Splitting message at %ld bytes", ftell(qp) - msgptr);
			CloseMail(qp, t);
			qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid);
			msgptr = ftell(qp);
		    }
		    MacroRead(fi, qp);
		}
	    }
	}

 	MacroVars("B", "d", Total );
	MacroRead(fi, qp);
 	MacroClear();
 	fclose(fi);
	fclose(mp);
	fclose(gp);
	fprintf(qp, "%s\r", TearLine());
	CloseMail(qp, t);
    } else
	WriteError("Can't create netmail");
    free(subject);
    MacroClear(); 
}



void A_Flow(faddr *t, char *replyid, int Notify)
{
    FILE	*qp, *gp, *mp, *fi = NULL;
    char	*temp, *Group, *subject;
    int		i, First = TRUE, Cons;
    char	Stat[2];
    faddr	*f, *g, *Temp;
    sysconnect	System;
    time_t	Now;
    struct tm	*tt;
    int		lmonth;
    int		rlw, rlm, rlt, plw, plm, plt;
    int		msgptr;
    fpos_t	fileptr, fileptr1, fileptr2; 

    Now = time(NULL);
    tt = localtime(&Now);
    lmonth = tt->tm_mon;
    if (lmonth)
	lmonth--;
    else
	lmonth = 11;

    subject = calloc(255, sizeof(char));
    f = bestaka_s(t);
    MacroVars("s", "s", nodes.Sysop);
    MacroVars("K", "d", Notify);
    MacroVars("y", "s", ascfnode(t, 0xff));
    MacroVars("Y", "s", ascfnode(f, 0xff));

    if (Notify) {
	Mgrlog("AreaMgr: Flow report to %s", ascfnode(t, 0xff));
        snprintf(subject,255,"AreaMgr Notify Flow Report");
        GetRpSubject("areamgr.notify.flow",subject,255);
	fi = OpenMacro("areamgr.notify.flow", nodes.Language, FALSE);
    } else {
	Mgrlog("AreaMgr: Flow report");
        snprintf(subject,255,"AreaMgr Flow Report");
        GetRpSubject("areamgr.flow",subject,255);
	fi = OpenMacro("areamgr.flow", nodes.Language, FALSE);
    }

    if (fi == 0) {
	free(subject);
	MacroClear();
	return;
    }

    if ((qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid)) != NULL) {
        /*
         * Mark begin of message in .pkt
         */
        msgptr = ftell(qp);

	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((mp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    free(subject);
	    fclose(fi);
	    MacroClear();
	    return;
	}
	fread(&msgshdr, sizeof(msgshdr), 1, mp);
	Cons = msgshdr.syssize / sizeof(System);
	
	snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    free(subject);
	    fclose(mp);
	    fclose(fi);
	    MacroClear();
	    return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	free(temp);

	MacroRead(fi, qp);
	fgetpos(fi,&fileptr);

	while (TRUE) {
	    Group = GetNodeMailGrp(First);
	    if (Group == NULL)
		break;
	    First = FALSE;

	    plm = plw = plt = rlm = rlw = rlt = 0;
	    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	    while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
		Temp = fido2faddr(mgroup.UseAka);
		g = bestaka_s(Temp);
		tidy_faddr(Temp);
		if ((!strcmp(mgroup.Name, Group)) &&
		    (g->zone  == f->zone) && (g->net   == f->net) && (g->node  == f->node) && (g->point == f->point)) {

		    MacroVars("G", "s", mgroup.Name);
		    MacroVars("J", "s", mgroup.Comment);
		    MacroVars("I", "s", aka2str(mgroup.UseAka));
		    fsetpos(fi,&fileptr);
		    MacroRead(fi, qp);
		    fgetpos(fi,&fileptr1);
		    fseek(mp, msgshdr.hdrsize, SEEK_SET);

		    while (fread(&msgs, msgshdr.recsize, 1, mp) == 1) {
			if (!strcmp(Group, msgs.Group) && msgs.Active && Access(nodes.Security, msgs.LinkSec)) {
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
			    MacroVars("X", "c", Stat[0]);
			    MacroVars("A", "s", msgs.Tag);
			    MacroVars("P", "d", msgs.Received.lweek);
			    MacroVars("Q", "d", msgs.Received.month[lmonth]);
			    MacroVars("R", "d", msgs.Received.total);
			    MacroVars("p", "d", msgs.Posted.lweek);
			    MacroVars("q", "d", msgs.Posted.month[lmonth]);
			    MacroVars("r", "d", msgs.Posted.total);
			    MacroVars("x", "d", (Stat[0] == 'C'));
			    fsetpos(fi,&fileptr1);
			    MacroRead(fi, qp);
			    fgetpos(fi,&fileptr2);
			    rlm += msgs.Received.month[lmonth];
			    rlw += msgs.Received.lweek;
			    rlt += msgs.Received.total;
			    plm += msgs.Posted.month[lmonth];
			    plw += msgs.Posted.lweek;
			    plt += msgs.Posted.total;
			    if (((ftell(qp) - msgptr) / 1024) >= CFG.new_force) {
				MacroVars("Z","d",1);
				Syslog('-', "  Forced splitting message at %ld bytes", ftell(qp) - msgptr);
				CloseMail(qp, t);
				qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid);
				msgptr = ftell(qp);
			    }
			} else
			    fseek(mp, msgshdr.syssize, SEEK_CUR);
		    }
		    MacroVars("ZBCDbcd", "ddddddd", (int) 0 , rlw, rlm, rlt, plw, plm, plt);
		    fsetpos(fi,&fileptr2);
		    if (((ftell(qp) - msgptr) / 1024) >= CFG.new_split) {
			MacroVars("Z","d",1);
			Syslog('-', "  Splitting message at %ld bytes", ftell(qp) - msgptr);
			CloseMail(qp, t);
			qp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid);
			msgptr = ftell(qp);
		    }
		    MacroRead(fi, qp);
		}
	    }
	}

	MacroRead(fi, qp);
 	MacroClear();
 	fclose(fi);
	CloseMail(qp, t);
	fclose(mp);
	fclose(gp);
    } else
	WriteError("Can't create netmail");
    free(subject);
}



void A_Status(faddr *, char *);
void A_Status(faddr *t, char *replyid)
{
    FILE    *fp, *fi;
    int	    i;
    char    *subject; 
    faddr   *ta;

    subject = calloc(255, sizeof(char));
    snprintf(subject,255,"AreaMgr Status");
    Mgrlog("AreaMgr: Status");

    if (Miy == 0)
	i = 11;
    else
	i = Miy - 1;
    MacroVars("D", "d", nodes.Direct);
    MacroVars("C", "d", nodes.Crash);
    MacroVars("E", "d", nodes.Hold);
    MacroVars("f", "d", nodes.Notify);
    MacroVars("G", "c", nodes.Language);
    MacroVars("v", "s", aka2str(nodes.RouteVia));
    MacroVars("P", "d", nodes.MailSent.lweek);
    MacroVars("Q", "d", nodes.MailSent.month[i]);
    MacroVars("R", "d", nodes.MailSent.total);
    MacroVars("p", "d", nodes.MailRcvd.lweek);
    MacroVars("q", "d", nodes.MailRcvd.month[i]);
    MacroVars("r", "d", nodes.MailRcvd.total);
    MacroVars("s", "s", nodes.Sysop);
    MacroVars("Y", "s", ascfnode(t, 0xff));
    ta = bestaka_s(t);
    MacroVars("y", "s", ascfnode(ta, 0xf));
    tidy_faddr(ta);

    GetRpSubject("areamgr.status",subject,255);

    if ((fi = OpenMacro("areamgr.status", nodes.Language, FALSE)) == NULL ){
	MacroClear();
	free(subject);
	return;
    }
    
    if ((fp = SendMgrMail(t, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid)) != NULL) {
	MacroRead(fi, fp);
 	fclose(fi);
	fprintf(fp, "%s\r", TearLine());
	CloseMail(fp, t);
    } else
	WriteError("Can't create netmail");
    MacroClear();
    free(subject);
}



void A_Unlinked(faddr *, char *);
void A_Unlinked(faddr *t, char *replyid)
{
    A_List(t, replyid, LIST_UNLINK);
}



void A_Disconnect(faddr *, char *, FILE *);
void A_Disconnect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group;
    faddr	*b, *Temp;
    sysconnect	Sys;

    Mgrlog("AreaMgr: disconnect \"%s\"", Area);
    ShiftBuf(Area, 1);
    for (i=0; i < strlen(Area); i++ ) 
	Area[i]=toupper(Area[i]);

    if (!SearchMsgs(Area)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_NOTFOUND",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  Area not found");
	MacroClear();
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
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_NOTGROUP",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  Group %s not available for %s", mgroup.Name, ascfnode(t, 0x1f));
	MacroClear();
	return;
    }

    b = bestaka_s(t);
    Temp = fido2faddr(msgs.Aka);
    i = metric(b, Temp);
    tidy_faddr(Temp);
    Syslog('m', "Aka match level is %d", i);

    if (i >= METRIC_NET) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_BADADD",Area,ascfnode(t, 0x1f),"","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  %s may not disconnect from group %s", ascfnode(t, 0x1f), mgroup.Name);
	MacroClear();
	return;
    }

    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = FALSE;
    Sys.receivefrom = FALSE;

    if (!MsgSystemConnected(Sys)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_DISC_NC",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  %s is not connected to %s", ascfnode(t, 0x1f), Area);
	MacroClear();
	return;
    }

    if (MsgSystemConnect(&Sys, FALSE)) {

	/*
	 *  Make sure to write an overview afterwards.
	 */
	a_list = TRUE;
	Mgrlog("Disconnected echo area %s", Area);
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	MacroVars("RABCDE", "ssssss","OK_DISC",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	MacroClear();	
	return;
    }

    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
    MacroVars("RABCDE", "ssssss","ERR_DISC_NOTAVAIL",Area,"","","","");
    MsgResult("areamgr.responses",tmp,'\n');
    Mgrlog("Didn't disconnect %s from mandatory or cutoff echo area %s", ascfnode(t, 0x1f), Area);
    MacroClear();
}



void A_Connect(faddr *, char *, FILE *);
void A_Connect(faddr *t, char *Area, FILE *tmp)
{
    int		i, First;
    char	*Group, *temp;
    faddr	*b, *Temp;
    sysconnect	Sys;
    FILE	*gp;

    Mgrlog("AreaMgr: connect \"%s\"", printable(Area, 0));

    if (Area[0] == '+')
	ShiftBuf(Area, 1);
    for (i=0; i < strlen(Area); i++ ) 
	Area[i]=toupper(Area[i]);

    if (!SearchMsgs(Area)) {
	/*
	 * Close noderecord, autocreate will destroy it.
	 */
	UpdateNode();

	Syslog('m', "  Area not found, trying to create");
	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((gp = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    free(temp);
	    return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	fseek(gp, mgrouphdr.hdrsize, SEEK_SET);

	while ((fread(&mgroup, mgrouphdr.recsize, 1, gp)) == 1) {
	    if ((mgroup.UseAka.zone == t->zone) && (mgroup.UseAka.net == t->net) && mgroup.UpLink.zone &&
		strlen(mgroup.AreaFile) && mgroup.Active && mgroup.UserChange) {
		if (CheckEchoGroup(Area, mgroup.UpLink.zone, t) == 0) {
		    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
		    MacroVars("RABCDE", "ssssss","ERR_CONN_FORWARD",Area,aka2str(mgroup.UpLink),"","","");
		    MsgResult("areamgr.responses",tmp,'\n');
		    break;
		}
	    }
	}
	fclose(gp);
	free(temp);

	/*
	 * Restore noderecord and try to load the area again
	 */
	SearchNodeFaddr(t);
	if (!SearchMsgs(Area)) {
	    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Areamgr");
	    MacroVars("RABCDE", "ssssss","ERR_CONN_NOTFOUND",Area,"","","","");
	    MsgResult("areamgr.responses",tmp,'\n');
	    Mgrlog("Area %s not found", Area);
	    MacroClear();
	    return;
	}
    }

    Syslog('m', "  Found %s group %s", msgs.Tag, mgroup.Name);

    First = TRUE;
    while ((Group = GetNodeMailGrp(First)) != NULL) {
	First = FALSE;
	if (strcmp(Group, mgroup.Name) == 0)
	    break;
    }
    if (Group == NULL) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, "Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_CONN_NOTGROUP",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  Group %s not available for node %s", mgroup.Name, ascfnode(t, 0x1f));
	MacroClear();
	return;
    }

    b = bestaka_s(t);
    Temp = fido2faddr(msgs.Aka);
    i = metric(b, Temp);
    tidy_faddr(Temp);
    Syslog('m', "Aka match level is %d", i);

    if (i >= METRIC_NET) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_CONN_BADADD",Area,ascfnode(t, 0x1f),"","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  %s may not connect to group %s", ascfnode(t, 0x1f), mgroup.Name);
	MacroClear();
	return;
    }

    if (! Access(nodes.Security, msgs.LinkSec)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	/*
	 * If node has no access by flags, we lie and say "Area not found"
	 */
	MacroVars("RABCDE", "ssssss","ERR_CONN_NOTFOUND",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  %s has no access to %s", ascfnode(t, 0x1f), Area);
	MacroClear();
	return;
    }
	
    memset(&Sys, 0, sizeof(Sys));
    memcpy(&Sys.aka, faddr2fido(t), sizeof(fidoaddr));
    Sys.sendto      = TRUE;
    Sys.receivefrom = TRUE;

    if (MsgSystemConnected(Sys)) {
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	MacroVars("RABCDE", "ssssss","ERR_CONN_ALREADY",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
	Mgrlog("  %s is already connected to %s", ascfnode(t, 0x1f), Area);
	MacroClear();
	return;
    }

    if (MsgSystemConnect(&Sys, TRUE)) {

	/*
	 *  Make sure to write an overview afterwards.
	 */
	a_list = TRUE;
	Mgrlog("Connected echo area %s", Area);
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	MacroVars("RABCDE", "ssssss","OK_CONN",Area,aka2str(msgs.Aka),"","","");
	MsgResult("areamgr.responses",tmp,'\n');
	MacroClear();
	return;
    }
    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
    MacroVars("RABCDE", "ssssss","ERR_CONN_NOTAVAIL",Area,"","","","");
    MsgResult("areamgr.responses",tmp,'\n');
    WriteError("Can't connect node %s to echo area %s", ascfnode(t, 0x1f), Area);
    MacroClear();
}



void A_All(faddr *, int, FILE *, char *);
void A_All(faddr *t, int Connect, FILE *tmp, char *Grp)
{
    FILE	*mp, *gp;
    char	*Group, *temp;
    faddr	*f, *Temp;
    int		i, Link, First = TRUE, Cons;
    sysconnect	Sys;
    int		Pos;

    if (Grp == NULL) {
	if (Connect)
	    Mgrlog("AreaMgr: Connect All");
	else
	    Mgrlog("AreaMgr: Disconnect All");
    } else {
	if (Connect)
	    Mgrlog("AreaMgr: Connect group %s", Grp);
	else
	    Mgrlog("AreaMgr: Disconnect group %s", Grp);
    }

    f = bestaka_s(t);
    temp = xstrcpy(ascfnode(t, 0x1f));
    Syslog('m', "Bestaka for %s is %s", temp, ascfnode(f, 0x1f));
    free(temp);

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((mp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, mp);
    Cons = msgshdr.syssize / sizeof(Sys);
    
    snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
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
		    Temp = fido2faddr(msgs.Aka);
		    if ((!strcmp(Group, msgs.Group)) && (msgs.Active) && (!msgs.Mandatory) && strlen(msgs.Tag) &&
			    ((msgs.Type == ECHOMAIL) || (msgs.Type == NEWS) || (msgs.Type == LIST)) &&
			    (metric(Temp, f) < METRIC_NET) && Access(nodes.Security, msgs.LinkSec)) {

			if (Connect) {
			    Link = FALSE;
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, mp);
				tidy_faddr(Temp);
				Temp = fido2faddr(Sys.aka);
				if (metric(Temp, t) == METRIC_EQUAL)
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
					Mgrlog("AreaMgr: Connected %s", msgs.Tag);
					MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
		    			MacroVars("RABCDE", "ssssss","OK_CONN",msgs.Tag,aka2str(msgs.Aka),"","","");
		    			MsgResult("areamgr.responses",tmp,'\n');
					MacroClear();						
					a_list = TRUE;
					break;
				    }
				}
				fseek(mp, Pos, SEEK_SET);
			    }
			} else {
			    for (i = 0; i < Cons; i++) {
				fread(&Sys, sizeof(Sys), 1, mp);
				tidy_faddr(Temp);
				Temp = fido2faddr(Sys.aka);
				if ((metric(Temp, t) == METRIC_EQUAL) && (!Sys.cutoff)) {
				    memset(&Sys, 0, sizeof(Sys));
				    fseek(mp, - sizeof(Sys), SEEK_CUR);
				    fwrite(&Sys, sizeof(Sys), 1, mp);
				    Mgrlog("AreaMgr: Disconnected %s", msgs.Tag);
				    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
		    		    MacroVars("RABCDE", "ssssss","OK_DISC",msgs.Tag,"","","","");
		    		    MsgResult("areamgr.responses",tmp,'\n');
				    MacroClear();
				    a_list = TRUE;
				}
			    }
			}
		    } else
			fseek(mp, msgshdr.syssize, SEEK_CUR);
		    tidy_faddr(Temp);
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
    faddr	*f, *Temp;
    int		i, Cons;
    sysconnect	Sys;
    char	*temp;

    if (Pause)
	Mgrlog("AreaMgr: Pause");
    else
	Mgrlog("AreaMgr: Resume");

    f = bestaka_s(t);
    Syslog('m', "Bestaka for %s is %s", ascfnode(t, 0x1f), ascfnode(f, 0x1f));

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
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
		Temp = fido2faddr(Sys.aka);
		if ((metric(Temp, t) == METRIC_EQUAL) && (!Sys.cutoff)) {
		    Sys.pause = Pause;
		    fseek(mp, - sizeof(Sys), SEEK_CUR);
		    fwrite(&Sys, sizeof(Sys), 1, mp);
		    Mgrlog("AreaMgr: %s area %s",  Pause?"Pause":"Resume", msgs.Tag);
		    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
		    MacroVars("RABCDE", "ssdsss","OK_PAUSE",msgs.Tag,Pause,"","","");
		    MsgResult("areamgr.responses",tmp,'\n');
		    a_list = TRUE;
		}
		tidy_faddr(Temp);
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
    Mgrlog("AreaMgr: Rescan %s, MSGS=%lu", Area, a_msgs);
    result = RescanOne(t, Area, a_msgs);
    MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
    if (result == 0){
	MacroVars("RABCDE", "ssdsss","OK_RESCAN",Area,a_msgs,"","","");
	MsgResult("areamgr.responses",tmp,'\n');
    } else if (result == 1) {
	MacroVars("RABCDE", "ssssss","ERR_RESCAN_UNK",Area,"","","","");
	MsgResult("areamgr.responses",tmp,'\n');
    } else if (result == 2) {
	MacroVars("RABCDE", "ssssss","ERR_RESCAN_NOTAVAIL",Area,ascfnode(t, 0x1f),"","","");
	MsgResult("areamgr.responses",tmp,'\n');
    } else {
	MacroVars("RABCDE", "ssssss","ERR_RESCAN_FATAL",Area,ascfnode(t, 0x1f),"","","");
	MsgResult("areamgr.responses",tmp,'\n');
    }
    MacroClear();      
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
    Mgrlog("AreaMgr: msgs %s ", Buf );
}



int AreaMgr(faddr *f, faddr *t, char *replyid, char *subj, time_t mdate, int flags, FILE *fp)
{
    int		i, rc = 0, spaces;
    char	*Buf, *subject;
    FILE	*tmp, *np;

    a_help = a_stat = a_unlnk = a_list = a_query = FALSE;
    areamgr++;
    if (SearchFidonet(f->zone))
	f->domain = xstrcpy(fidonet.domain);

    Mgrlog("AreaMgr request from %s start", ascfnode(f, 0xff));

    /*
     * If the password failed, we return silently and don't respond.
     */
    if ((!strlen(subj)) || (strcasecmp(subj, nodes.Apasswd))) {
	WriteError("AreaMgr: password expected \"%s\", got \"%s\"", nodes.Apasswd, subj);
	Mgrlog("AreaMgr request from %s finished", ascfnode(f, 0xff));
	net_bad++;
	return FALSE;
    }

    if ((tmp = tmpfile()) == NULL) {
	WriteError("$AreaMgr: Can't open tmpfile()");
	net_bad++;
	return FALSE;
    }

    Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
    rewind(fp);

    while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {

	/*
	 * Make sure we have the nodes record loaded
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
	    else if (!strncasecmp(Buf, "%+all", 5) && CFG.ca_PlusAll)
		A_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-all", 5) && CFG.ca_PlusAll)
		A_All(f, FALSE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%+*", 3) && CFG.ca_PlusAll)
		A_All(f, TRUE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%-*", 3) && CFG.ca_PlusAll)
		A_All(f, FALSE, tmp, NULL);
	    else if (!strncasecmp(Buf, "%+", 2))
		A_Group(f, Buf, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%-", 2))
		A_Group(f, Buf, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%pause", 6) && CFG.ca_Pause)
		A_Pause(f, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%resume", 7) && CFG.ca_Pause)
		A_Pause(f, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%passive", 8) && CFG.ca_Pause)
		A_Pause(f, TRUE, tmp);
	    else if (!strncasecmp(Buf, "%active", 7) && CFG.ca_Pause)
		A_Pause(f, FALSE, tmp);
	    else if (!strncasecmp(Buf, "%password", 9) && CFG.ca_Passwd)
		MgrPasswd(f, Buf, tmp, 9, 0);
	    else if (!strncasecmp(Buf, "%pwd", 4) && CFG.ca_Passwd)
		MgrPasswd(f, Buf, tmp, 4, 0);
	    else if (!strncasecmp(Buf, "%notify", 7) && CFG.ca_Notify)
		MgrNotify(f, Buf, tmp, 0);
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
        subject=calloc(256,sizeof(char));
        MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,"Areamgr");
	MacroVars("RABCDE", "ssssss","","","","","","");
	snprintf(subject,256,"Your AreaMgr request");
	GetRpSubject("areamgr.responses",subject,72);
	if ((np = SendMgrMail(f, CFG.ct_KeepMgr, FALSE, (char *)"Areamgr", subject, replyid)) != NULL) {
	    MacroVars("RABCDE", "ssssss","WELLCOME","","","","","");
	    MsgResult("areamgr.responses",np,'\r');
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
	    MsgResult("areamgr.responses",np,'\r');
	    fprintf(np, "\r%s\r", TearLine());
	    CloseMail(np, t);
	} else
	    WriteError("Can't create netmail");
	free(subject);
    }
    MacroClear();
    free(Buf);
    fclose(tmp);

    if (a_stat)
	A_Status(f, replyid);

    if (a_query)
	A_Query(f, replyid);

    if (a_list)
	A_List(f, replyid, LIST_LIST);

    if (a_flow)
	A_Flow(f, replyid, FALSE);

    if (a_unlnk)
	A_Unlinked(f, replyid);

    if (a_help)
	A_Help(f, replyid);

    Mgrlog("AreaMgr request from %s finished", ascfnode(f, 0xff));
    return rc;
}

