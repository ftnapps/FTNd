/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: File forward to a node
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
#include "../lib/dbnode.h"
#include "../lib/dbtic.h"
#include "../lib/diesel.h"
#include "tic.h"
#include "sendmail.h"
#include "rollover.h"
#include "mgrutil.h"
#include "forward.h"




void ForwardFile(fidoaddr Node, fa_list *sbl)
{
	char		*subject = NULL, *temp, *fwdfile = NULL, *ticfile = NULL, fname[PATH_MAX], *ticname;
	FILE		*fp, *fi, *net;
	char		flavor;
	faddr		*dest, *route, *Fa;
	int		i, z, n;
	time_t		now;
	fa_list		*tmp;

	if (!SearchNode(Node)) {
		WriteError("TIC forward in %s, node %s not in setup but defined in area setup", TIC.TicIn.Area, aka2str(Node));
		return;
	}
	Syslog('+', "Forward file to %s, %s netmail", aka2str(Node), nodes.Message?"with":"whithout");

	/*
	 * Hier moet een nieuwe SEEN-BY check komen, maar dan wel zo dat
	 * de net toegevoegde seenby niet getest wordt.
	 */

	/*
	 * If Costsharing active for this node
	 */
	if (nodes.Billing) {
	    /*
	     * Check if this node has enough credits for this file.
	     */
	    T_File.Cost = TIC.FileCost + (TIC.FileCost * nodes.AddPerc / 1000);
	    if ((nodes.Credit < (nodes.StopLevel + T_File.Cost))  && (!TIC.Charge)) {
		Syslog('!', "No forward to %s, not enough credit left", aka2str(Node));
		exit;
	    }

	    /*
	     * Check if we are passing the warning level
	     */
	    if ((nodes.Credit > nodes.WarnLevel) && ((nodes.Credit - T_File.Cost) <= nodes.WarnLevel)) {
		Syslog('+', "Low credit warning to %s", aka2str(Node));
		/* CREATE NETMAIL */
	    }
	}

	fwdfile = calloc(PATH_MAX, sizeof(char));
	/*
	 * Create the full filename
	 */
	if (TIC.SendOrg) {
		sprintf(fwdfile, "%s/%s", TIC.Inbound, TIC.RealName);
		subject = xstrcpy(TIC.RealName);
	} else {
		sprintf(fwdfile, "%s/%s", TIC.BBSpath, TIC.NewName);
		subject = xstrcpy(TIC.NewName);
	}

	flavor = 'f';
	if (nodes.Crash) 
		flavor = 'c';
	if (nodes.Hold)
		flavor = 'h';

	if (nodes.RouteVia.zone)
		route = fido2faddr(nodes.RouteVia);
	else
		route = fido2faddr(Node);
	dest = fido2faddr(Node);
	attach(*route, fwdfile, LEAVE, flavor);

//	if (strlen(CFG.dospath))
//		subject = xstrcpy(Unix2Dos(fwdfile));
//	else
//		subject = xstrcpy(fwdfile);

	ticfile = calloc(PATH_MAX, sizeof(char));
	ticname = calloc(15, sizeof(char));
	if (nodes.Tic) {
		sprintf(ticname, "%08lx.tic", sequencer());
		subject = xstrcat(subject, (char *)" ");
		subject = xstrcat(subject, ticname);
		sprintf(ticfile, "%s/%s", CFG.ticout, ticname);
	}
	free(ticname);

	/*
	 *  Send netmail message if the node has it turned on.
	 */
	if (nodes.Message) {
	    if ((net = SendMgrMail(fido2faddr(Node), CFG.ct_KeepMgr, TRUE, (char *)"Filemgr", subject, NULL)) != NULL) {
		if ((fi = OpenMacro("forward.tic", nodes.Language, FALSE)) != NULL) {
		    MacroVars("abcdfghijmns", "ssdssddsssss", TIC.TicIn.Area, tic.Comment, TIC.FileCost, fgroup.Comment,
							    TIC.TicIn.FullName, TIC.FileSize, TIC.FileSize / 1024, 
							    TIC.TicIn.Crc, TIC.TicIn.Origin, rfcdate(TIC.FileDate), 
							    TIC.TicIn.Desc, nodes.Sysop);
		    if (TIC.SendOrg)
			MacroVars("e", "s", TIC.RealName);
		    else
			MacroVars("e", "s", TIC.NewName);
		    if (strlen(TIC.TicIn.Magic))
			MacroVars("k", "s", TIC.TicIn.Magic);
		    if (strlen(TIC.TicIn.Replace))
			MacroVars("l", "s", TIC.TicIn.Replace);
		    MacroRead(fi, net);
		    fprintf(net, "%s\r", TearLine());
		    CloseMail(net, fido2faddr(Node));
		}
	    } else {
		WriteError("$Can't create netmail");
	    }
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

			if ((TIC.PassThru) || (TIC.SendOrg))
				subject = xstrcpy(TIC.RealName);
			else
				subject = xstrcpy(TIC.NewName);
			/*
			 * Create 8.3 filename if this is a long filename. In normal
			 * cases mbcico will transmit the long filename to the other
			 * node. If they can't process the TIC files which has a short
			 * 8.3 filename and they have a long filename in the inbound
			 * then in mbsetup these nodes need to be set to 8.3 filenames.
			 * The mailer will then transmit the file with a 8.3 name.
			 * Thank the inventors of the 8.3 filenames for this.
			 */
			temp = xstrcpy(subject);
			name_mangle(temp);
			fprintf(fp, "File %s\r\n", temp);
			fprintf(fp, "Fullname %s\r\n", subject);
			free(temp);
			free(subject);

			fprintf(fp, "Size %ld\r\n", (long)(TIC.FileSize));
			fprintf(fp, "Desc %s\r\n", TIC.TicIn.Desc);
			fprintf(fp, "Crc %s\r\n", TIC.TicIn.Crc);
			if (nodes.AdvTic) {
				fprintf(fp, "To %s, %s\r\n", nodes.Sysop, ascfnode(dest, 0x1f));
				fprintf(fp, "Areadesc %s\r\n", tic.Comment);
				fprintf(fp, "Fdn %s\r\n", fgroup.Comment);
				/*
				 * According to Harald Harms this field must
				 * be multiplied with 100.
				 */
				if (TIC.FileCost && nodes.Billing) 
					fprintf(fp, "Cost %ld.00\r\n", T_File.Cost);
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
			fprintf(fp, "Path %s %lu %s %s\r\n", ascfnode(bestaka_s(dest), 0x1f),
						       mktime(localtime(&now)), subject, tzname[0]);

			if (nodes.AdvTic) {
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
						subject = xstrcat(subject, ascfnode(tmp->addr, 0x0e));
						z = tmp->addr->zone;
					} else { 
						if (n != tmp->addr->net) {
							subject = xstrcat(subject, ascfnode(tmp->addr, 0x06));
							n = tmp->addr->net;
						} else {
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
			attach(*route, ticfile, KFS, flavor);
		} else {
			WriteError("$Can't create %s", ticfile);
		}
	}

	if (TIC.Charge && nodes.Billing) {
		nodes.Credit -= TIC.FileCost;
		Syslog('-', "Cost: %d  Left: %d", TIC.FileCost, nodes.Credit);

		/*
		 * Add an entry to the billing file, each node has his own
		 * billing file.
		 */
		sprintf(fname, "%s/tmp/%d.%d.%d.%d.bill", getenv("MBSE_ROOT"), 
			nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node, nodes.Aka[0].point);
		if ((fp = fopen(fname, "a+")) != NULL) {
			memset(&bill, 0, sizeof(bill));
			bill.Node = nodes.Aka[0];
			strcpy(bill.FileName, TIC.NewName);
			strcpy(bill.FileEcho, TIC.TicIn.Area);
			bill.Size = TIC.FileSize;
			bill.Cost = TIC.FileCost;
			fwrite(&bill, sizeof(bill), 1, fp);
			fclose(fp);
		} else {
			WriteError("$Can't create %s", fname);
		}
	}

	/*
	 * Update the nodes statistic counters
	 */
	StatAdd(&nodes.FilesSent, 1L);
	StatAdd(&nodes.F_KbSent, T_File.SizeKb);
	UpdateNode();
	SearchNode(Node);
	free(ticfile);
	free(fwdfile);
	tidy_faddr(route);
}


