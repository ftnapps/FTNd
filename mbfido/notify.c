/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Write notify messages.
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/dbnode.h"
#include "filemgr.h"
#include "areamgr.h"
#include "sendmail.h"
#include "mgrutil.h"
#include "notify.h"



extern int	do_quiet;		/* Quiet flag		     */
int		notify = 0;		/* Nr of notify messages     */



/*
 *  Write AreaMgr and FileMgr notify messages.
 */
int Notify(char *Options)
{
	short	Zones = -1, Nets = -1, Nodes = -1, Points = -1;
	short	Lzone, Lnet;
	FILE	*np;
	char	*temp, Opt[44];
	int	i;
	faddr	*Tmp;

	Syslog('+', "Notify \"%s\"", Options);

	if (!do_quiet) {
		colour(9, 0);
		printf("Writing notify messages\n");
		colour(3, 0);
	}

	if (strlen(Options)) {
		sprintf(Opt, "%s~", Options);
		if (strchr(Opt, '.') != NULL) {
			temp = strdup(strtok(Opt, ":"));
			if (atoi(temp))
				Zones = atoi(temp);
			temp = strdup(strtok(NULL, "/"));
			if (strncmp(temp, "*", 1))
				Nets = atoi(temp);
			temp = strdup(strtok(NULL, "."));
			if (strncmp(temp, "*", 1))
				Nodes = atoi(temp);
			temp = strdup(strtok(NULL, "~"));
			if (strncmp(temp, "*", 1))
				Points = atoi(temp);
			else
				Points = -1;
		} else if (strchr(Opt, '/') != NULL) {
			temp = strdup(strtok(Opt, ":"));
			if (atoi(temp))
				Zones = atoi(temp);
			temp = strdup(strtok(NULL, "/"));
			if (strncmp(temp, "*", 1))
				Nets = atoi(temp);
			temp = strdup(strtok(NULL, "~"));
			if (strncmp(temp, "*", 1)) {
				Nodes = atoi(temp);
				Points = 0;
			}
		} else if (strchr(Opt, ':') != NULL) {
			temp = strdup(strtok(Opt, ":"));
			if (atoi(temp))
				Zones = atoi(temp);
			temp = strdup(strtok(NULL, "~"));
			if (strncmp(temp, "*", 1))
				Nets = atoi(temp);
		} else {
			temp = strdup(strtok(Opt, "~"));
			if (atoi(temp))
				Zones = atoi(temp);
		}
	}
	Syslog('m', "Parsing nodes %d:%d/%d.%d", Zones, Nets, Nodes, Points);

	temp = calloc(128, sizeof(char));
	sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	if ((np = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		return FALSE;
	}
	fread(&nodeshdr, sizeof(nodeshdr), 1, np);

	while (fread(&nodes, nodeshdr.recsize, 1, np) == 1) {
		Lzone = Lnet = 0;
		for (i = 0; i < 20; i++) {
			if ((((Zones  == -1) && nodes.Notify) || (Zones  == nodes.Aka[i].zone)) &&
			    (((Nets   == -1) && nodes.Notify) || (Nets   == nodes.Aka[i].net)) &&
			    (((Nodes  == -1) && nodes.Notify) || (Nodes  == nodes.Aka[i].node)) &&
			    (((Points == -1) && nodes.Notify) || (Points == nodes.Aka[i].point)) &&
			    (nodes.Aka[i].zone) && 
			    ((Lzone != nodes.Aka[i].zone) ||
			     (Lnet  != nodes.Aka[i].net))) {
				Lzone = nodes.Aka[i].zone;
				Lnet  = nodes.Aka[i].net;

				Syslog('m', "Notify to %s", aka2str(nodes.Aka[i]));
				if (!do_quiet) {
					printf("\rNotify %-24s", aka2str(nodes.Aka[i]));
					fflush(stdout);
				}

				Tmp = fido2faddr(nodes.Aka[i]);
				if (i == 0) {
					F_Status(Tmp, NULL);
					A_Status(Tmp, NULL);
				}
				F_List(Tmp, NULL, LIST_NOTIFY);
				A_List(Tmp, NULL, LIST_NOTIFY);
				A_Flow(Tmp, NULL, TRUE);
				tidy_faddr(Tmp);
				notify++;
			}
		}
		fseek(np, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
	}

	fclose(np);
	free(temp);

	if (!do_quiet) {
		printf("\r                                                  \r");
		fflush(stdout);
	}

	if (notify)
		return TRUE;
	else
		return FALSE;
}



