/*****************************************************************************
 *
 * $Id: dbnode.c,v 1.10 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: Noderecord Access
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
#include "mbselib.h"
#include "users.h"
#include "mbsedb.h"



char		nodes_fil[PATH_MAX];	/* Nodes database filename	    */
int		nodes_pos = -1;		/* Noderecord position		    */
int		nodes_fgp = -1;		/* Nodes files group position	    */
int		nodes_mgp = -1;		/* Nodes message group position	    */
unsigned int	nodes_crc = -1;		/* Noderecord crc value		    */



int InitNode(void)
{
	FILE	*fil;

	memset(&nodes, 0, sizeof(nodes));
	LoadConfig();

	snprintf(nodes_fil, PATH_MAX -1, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(nodes_fil, "r")) == NULL)
		return FALSE;

	fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	nodes_cnt = (ftell(fil) - nodeshdr.hdrsize) / (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp);
	fclose(fil);

	return TRUE;
}



int TestNode(fidoaddr aka)
{
	int	i, nodeok = FALSE;

	for (i = 0; i < 20; i++) {
		if (((aka.zone == 0) || (aka.zone == nodes.Aka[i].zone)) &&
		    (aka.net == nodes.Aka[i].net) &&
		    (aka.node == nodes.Aka[i].node) &&
		    (aka.point == nodes.Aka[i].point))
			nodeok = TRUE;
	}
	return(nodeok);
}


int SearchNodeFaddr(faddr *n)
{
    fidoaddr	Sys;

    memset(&Sys, 0, sizeof(Sys));
    Sys.zone  = n->zone;
    Sys.net   = n->net;
    Sys.node  = n->node;
    Sys.point = n->point;
    if (n->domain != NULL)
	strncpy(Sys.domain, n->domain, 12);

    return SearchNode(Sys);
}



int SearchNode(fidoaddr aka)
{
	FILE	*fil;

	nodes_pos = -1;
	nodes_crc = -1;

	if ((fil = fopen(nodes_fil, "r")) == NULL)
		return FALSE;

	fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
	while (fread(&nodes, nodeshdr.recsize, 1, fil) == 1) {
		fseek(fil, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
		if (TestNode(aka)) {
			nodes_pos = ftell(fil) - (nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp);
			fclose(fil);
			nodes_crc = 0xffffffff;
			nodes_crc = upd_crc32((char *)&nodes, nodes_crc, nodeshdr.recsize);
			return TRUE;
		}
	}

	memset(&nodes, 0, sizeof(nodes));
	fclose(fil);
	return FALSE;
}



/*
 * Update current noderecord if changed.
 */
int UpdateNode()
{
	unsigned int	crc;
	FILE		*fil;

	if (nodes_pos == -1)
		return FALSE;

	crc = 0xffffffff;
	crc = upd_crc32((char *)&nodes, crc, nodeshdr.recsize);
	if (crc != nodes_crc) {
		if ((fil = fopen(nodes_fil, "r+")) == NULL)
			return FALSE;
		fseek(fil, nodes_pos, SEEK_SET);
		fwrite(&nodes, nodeshdr.recsize, 1, fil);
		fclose(fil);
	}

	nodes_crc = -1;
	nodes_pos = -1;
	memset(&nodes, 0, sizeof(nodes));
	return TRUE;
}



char *GetNodeMailGrp(int First)
{
	FILE	*fil;
	char	group[13], *gr;

	if (nodes_pos == -1)
		return NULL;

	if (First)
		nodes_mgp = nodes_pos + nodeshdr.recsize + nodeshdr.filegrp;
	
	if (nodes_mgp > (nodes_pos + nodeshdr.recsize + nodeshdr.filegrp + nodeshdr.mailgrp))
		return NULL;

	if ((fil = fopen(nodes_fil, "r")) == NULL)
		return NULL;

	fseek(fil, nodes_mgp, SEEK_SET);
	fread(&group, sizeof(group), 1, fil);
	fclose(fil);

	nodes_mgp += sizeof(group);

	if (group[0] == '\0')
		return NULL;

	gr = xstrcpy(group);
	return gr;	
}



char *GetNodeFileGrp(int First)
{
	FILE	*fil;
	char	group[13], *gr;

	if (nodes_pos == -1)
		return NULL;

	if (First)
		nodes_fgp = nodes_pos + nodeshdr.recsize;

	if (nodes_fgp > (nodes_pos + nodeshdr.recsize + nodeshdr.filegrp))
		return NULL;

	if ((fil = fopen(nodes_fil, "r")) == NULL)
		return NULL;

	fseek(fil, nodes_fgp, SEEK_SET);
	fread(&group, sizeof(group), 1, fil);
	fclose(fil);

	nodes_fgp += sizeof(group);

	if (group[0] == '\0')
		return NULL;

	gr = xstrcpy(group);
	return gr;
}


