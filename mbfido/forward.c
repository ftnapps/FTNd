/*****************************************************************************
 *
 * $Id: forward.c,v 1.52 2006/03/27 18:48:27 mbse Exp $
 * Purpose ...............: File forward to a node
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
#include "../lib/mbsedb.h"
#include "../lib/diesel.h"
#include "orphans.h"
#include "tic.h"
#include "sendmail.h"
#include "rollover.h"
#include "mgrutil.h"
#include "forward.h"



void ForwardFile(fidoaddr Node, fa_list *sbl)
{
    char	*subject = NULL, *fwdfile = NULL, *queuedir, *listfile, *ticfile = NULL, *ticname, flavor;
    FILE	*fp, *fi, *fl, *net;
    faddr	*dest, *routeto, *Fa, *Temp, *ba;
    int		i, z, n;
    time_t	now, ftime;
    fa_list	*tmp;

    if (!SearchNode(Node)) {
	WriteError("TIC forward in %s, node %s not in setup but defined in area setup", TIC.TicIn.Area, aka2str(Node));
	return;
    }
    Syslog('+', "Forward file to %s %s netmail", aka2str(Node), nodes.Message?"with":"without");

    fwdfile  = calloc(PATH_MAX, sizeof(char));
    queuedir = calloc(PATH_MAX, sizeof(char));
    listfile = calloc(PATH_MAX, sizeof(char));
    snprintf(queuedir, PATH_MAX, "%s/%d.%d.%d.%d", CFG.out_queue, Node.zone, Node.net, Node.node, Node.point);
    snprintf(listfile, PATH_MAX, "%s/.filelist", queuedir);
    mkdirs(listfile, 0750);
    if ((fl = fopen(listfile, "a+")) == NULL) {
	WriteError("$Can't open %s", listfile);
	free(fwdfile);
	free(listfile);
	free(queuedir);
	return;
    }
    
    /*
     * Create the full filename
     */
    if (TIC.PassThru || TIC.SendOrg) {
	snprintf(fwdfile, PATH_MAX, "%s/%s", TIC.Inbound, TIC.TicIn.File);
	subject = xstrcpy(TIC.TicIn.File);
    } else {
	/*
	 * Make sure the file attach is the 8.3 filename
	 */
	snprintf(fwdfile, PATH_MAX, "%s/%s", TIC.BBSpath, TIC.NewFile);
	subject = xstrcpy(TIC.NewFile);
    }

    flavor = 'f';
    if (nodes.Crash) 
	flavor = 'c';
    if (nodes.Hold)
	flavor = 'h';

    fprintf(fl, "%c LEAVE FDN %s\n", flavor, fwdfile);

    if (nodes.RouteVia.zone)
	routeto = fido2faddr(nodes.RouteVia);
    else
	routeto = fido2faddr(Node);
    dest = fido2faddr(Node);

    ticfile = calloc(PATH_MAX, sizeof(char));
    ticname = calloc(15, sizeof(char));
    if (nodes.Tic) {
	snprintf(ticname, 15, "%08x.tic", sequencer());
	subject = xstrcat(subject, (char *)" ");
	subject = xstrcat(subject, ticname);
	snprintf(ticfile, PATH_MAX, "%s/%s", CFG.ticout, ticname);
    }
    free(ticname);

    /*
     *  Send netmail message if the node has it turned on.
     */
    if (nodes.Message) {
	Temp = fido2faddr(Node);
	if ((net = SendMgrMail(Temp, CFG.ct_KeepMgr, TRUE, (char *)"Filemgr", subject, NULL)) != NULL) {
	    if ((fi = OpenMacro("forward.tic", nodes.Language, FALSE)) != NULL) {
		ftime = TIC.FileDate;
		MacroVars("a", "s", TIC.TicIn.Area);
		MacroVars("b", "s", tic.Comment);
		MacroVars("c", "d", TIC.FileCost);
		MacroVars("d", "s", fgroup.Comment);
		if (TIC.PassThru || TIC.SendOrg)
		    MacroVars("f", "s", TIC.TicIn.FullName);
		else
		    MacroVars("f", "s", TIC.NewFullName);
		MacroVars("g", "d", TIC.FileSize);
		MacroVars("h", "d", (TIC.FileSize / 1024));
		MacroVars("i", "s", TIC.TicIn.Crc);
		MacroVars("j", "s", TIC.TicIn.Origin);
		MacroVars("m", "s", rfcdate(ftime));
		MacroVars("n", "s", TIC.TicIn.Desc);
		MacroVars("s", "s", nodes.Sysop);
		if (TIC.PassThru || TIC.SendOrg)
		    MacroVars("e", "s", TIC.TicIn.File);
		else
		    MacroVars("e", "s", TIC.NewFile);
		if (strlen(TIC.TicIn.Magic))
		    MacroVars("k", "s", TIC.TicIn.Magic);
		if (strlen(TIC.TicIn.Replace))
		    MacroVars("l", "s", TIC.TicIn.Replace);
		MacroRead(fi, net);
		fprintf(net, "%s\r", TearLine());
		CloseMail(net, Temp);
	    }
	} else {
	    WriteError("Can't create netmail");
	}
	tidy_faddr(Temp);
    }
    free(subject);

    /*
     * If we need a .TIC file, start creating it.
     */
    if (nodes.Tic) {
	mkdirs(ticfile, 0770);
	if ((fp = fopen(ticfile, "a+")) != NULL) {
	    fprintf(fp, "Area %s\r\n", TIC.TicIn.Area);
	    fprintf(fp, "Origin %s\r\n", TIC.TicIn.Origin);
	    Fa = fido2faddr(tic.Aka);
	    fprintf(fp, "From %s\r\n", ascfnode(Fa, 0x0f));
	    free(Fa);
	    if (strlen(TIC.TicIn.Replace))
		fprintf(fp, "Replaces %s\r\n", TIC.TicIn.Replace);
	    if (strlen(TIC.TicIn.Magic))
		fprintf(fp, "Magic %s\r\n", TIC.TicIn.Magic);

	    if ((TIC.PassThru) || (TIC.SendOrg)) {
		fprintf(fp, "File %s\r\n", TIC.TicIn.File);
		if (strlen(TIC.TicIn.FullName))
		    fprintf(fp, "Fullname %s\r\n", TIC.TicIn.FullName);
	    } else {
		fprintf(fp, "File %s\r\n", TIC.NewFile);
		if (strlen(TIC.NewFullName))
		    fprintf(fp, "Fullname %s\r\n", TIC.NewFullName);
	    }
	    fprintf(fp, "Size %d\r\n", (int)(TIC.FileSize));
	    fprintf(fp, "Desc %s\r\n", TIC.TicIn.Desc);
	    fprintf(fp, "Crc %s\r\n", TIC.TicIn.Crc);
	    if (nodes.TIC_To) {
		fprintf(fp, "To %s, %s\r\n", nodes.Sysop, ascfnode(dest, 0x1f));
	    }
	    if (nodes.AdvTic) {
		fprintf(fp, "Areadesc %s\r\n", tic.Comment);
		fprintf(fp, "Fdn %s\r\n", fgroup.Comment);
		if (TIC.TicIn.TotLDesc)
		    for (i = 0; i < TIC.TicIn.TotLDesc; i++)
			fprintf(fp, "LDesc %s\r\n", TIC.TicIn.LDesc[i]);
	    }
	    fprintf(fp, "Created by MBSE BBS %s %s\r\n", VERSION, SHORTRIGHT);
	    if (TIC.TicIn.TotPath)
		for (i = 0; i < TIC.TicIn.TotPath; i++)
		    fprintf(fp, "Path %s\r\n", TIC.TicIn.Path[i]);
	    /*
	     * Add our system to the path
	     */
	    now = time(NULL);
	    subject = ctime(&now);
	    Striplf(subject);
	    ba = bestaka_s(dest);
	    fprintf(fp, "Path %s %u %s %s\r\n", ascfnode(ba, 0x1f), (int)mktime(localtime(&now)), subject, tzname[0]);
	    tidy_faddr(ba);

	    if (nodes.TIC_AdvSB) {
		/*
		 * In advanced TIC mode we send multiple seenby
		 * addresses on one line in stead of one line
		 * per system.
		 */
		z = 0;
		n = 0;
		subject = xstrcpy((char *)"Seenby");
		for (tmp = sbl; tmp; tmp = tmp->next) {
		    if (strlen(subject) > 70) {
			fprintf(fp, "%s\r\n", subject);
			z = 0;
			n = 0;
			free(subject);
			subject = xstrcpy((char *)"Seenby ");
		    } else {
			subject = xstrcat(subject, (char *)" ");
		    }

		    if (z != tmp->addr->zone) {
			if (nodes.Tic4d)
			    subject = xstrcat(subject, ascfnode(tmp->addr, 0x0f));
			else
			    subject = xstrcat(subject, ascfnode(tmp->addr, 0x0e));
			z = tmp->addr->zone;
		    } else { 
			if (n != tmp->addr->net) {
			    if (nodes.Tic4d)
				subject = xstrcat(subject, ascfnode(tmp->addr, 0x07));
			    else
				subject = xstrcat(subject, ascfnode(tmp->addr, 0x06));
			    n = tmp->addr->net;
			} else {
			    if (nodes.Tic4d)
				subject = xstrcat(subject, ascfnode(tmp->addr, 0x03));
			    else
				subject = xstrcat(subject, ascfnode(tmp->addr, 0x02));
			}
		    }
		}
		if (strlen(subject) > 7) {
		    fprintf(fp, "%s\r\n", subject);
		    free(subject);
		}
	    } else {
		/*
		 * Old style seenby lines
		 */
		for (tmp = sbl; tmp; tmp = tmp->next) {
		    fprintf(fp, "Seenby %s\r\n", ascfnode(tmp->addr, 0x0f));
		}
	    }

	    /*
	     * Now append all passthru ticlines
	     */
	    if (TIC.TicIn.Unknowns)
		for (i = 0; i < TIC.TicIn.Unknowns; i++)
		    fprintf(fp, "%s\r\n", TIC.TicIn.Unknown[i]);

	    fprintf(fp, "Pw %s\r\n", nodes.Fpasswd);
	    fclose(fp);
	    fprintf(fl, "%c KFS NOR %s\n", flavor, ticfile);
	} else {
	    WriteError("$Can't create %s", ticfile);
	}
    }
    fsync(fileno(fl));
    fclose(fl);
    
    /*
     * Update the nodes statistic counters
     */
    StatAdd(&nodes.FilesSent, 1L);
    StatAdd(&nodes.F_KbSent, T_File.SizeKb);
    UpdateNode();
    SearchNode(Node);
    free(ticfile);
    free(fwdfile);
    free(queuedir);
    free(listfile);
    tidy_faddr(routeto);
}


