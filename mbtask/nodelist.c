/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Read nodelists information
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek		FIDO:	2:280/2802
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

#include "libs.h"
#include "../lib/structs.h"
#include "taskutil.h"
#include "nodelist.h"


#define NULLDOMAIN "nulldomain"


struct _pkey pkey[] = {
	{(char *)"Down",	NL_NODE,	NL_DOWN},
	{(char *)"Hold",	NL_NODE,	NL_HOLD},
	{(char *)"Region",	NL_REGION,	NL_REGION},
	{(char *)"Host",	NL_HOST,	NL_HOST},
	{(char *)"Hub",		NL_HUB,		NL_HUB},
	{(char *)"Point",	NL_POINT,	NL_POINT},
	{(char *)"Pvt",		NL_NODE,	NL_NODE},
	{NULL,			0,		0}
};



struct _okey okey[] = {
	{(char *)"CM",	OL_CM},
	{(char *)"MO",	OL_MO},
	{(char *)"LO",	OL_LO},
	{(char *)"MN",  OL_MN},
	{NULL, 0}
};

struct _fkey fkey[] = {
	{(char *)"V22",	NL_V22},
	{(char *)"V29",	NL_V29},
	{(char *)"V32",	NL_V32},
	{(char *)"V32B",NL_V32B | NL_V32},
	{(char *)"V34",	NL_V34},
	{(char *)"V42",	NL_V42  | NL_MNP},
	{(char *)"V42B",NL_V42B | NL_V42  | NL_MNP},
	{(char *)"MNP",	NL_MNP},
	{(char *)"H96",	NL_H96},
	{(char *)"HST",	NL_HST  | NL_MNP},
	{(char *)"H14",	NL_H14  | NL_HST  | NL_MNP},
	{(char *)"H16",	NL_H16  | NL_H14  | NL_HST | NL_MNP  | NL_V42 | NL_V42B},
	{(char *)"MAX",	NL_MAX},
	{(char *)"PEP",	NL_PEP},
	{(char *)"CSP",	NL_CSP},
	{(char *)"V32T",NL_V32T | NL_V32B | NL_V32},
	{(char *)"VFC", NL_VFC},
	{(char *)"ZYX",	NL_ZYX  | NL_V32B | NL_V32 | NL_V42B | NL_V42 | NL_MNP},
	{(char *)"X2C", NL_X2C  | NL_X2S  | NL_V34},
	{(char *)"X2S", NL_X2S  | NL_V34},
	{(char *)"V90C",NL_V90C | NL_V90S | NL_V34},
	{(char *)"V90S",NL_V90S | NL_V34},
	{(char *)"Z19", NL_Z19  | NL_V32B | NL_V32 | NL_V42B | NL_V42 | NL_MNP | NL_ZYX},
	{NULL, 0}
};

struct _xkey xkey [] = {
	{(char *)"XA",	RQ_XA},
	{(char *)"XB",	RQ_XB},
	{(char *)"XC",	RQ_XC},
	{(char *)"XP",	RQ_XP},
	{(char *)"XR",	RQ_XR},
	{(char *)"XW",	RQ_XW},
	{(char *)"XX",	RQ_XX},
	{NULL,	0}
};

struct _dkey dkey [] = {
	{(char *)"V110L", ND_V110L},
	{(char *)"V110H", ND_V110H},
	{(char *)"V120L", ND_V120L},
	{(char *)"V120H", ND_V120H},
	{(char *)"X75",   ND_X75},
	{NULL, 0}
};

struct _ikey ikey [] = {
	{(char *)"IBN", IP_IBN},
	{(char *)"IFC", IP_IFC},
	{(char *)"ITN", IP_ITN},
	{(char *)"IVM", IP_IVM},
	{(char *)"IP",  IP_IP},
	{(char *)"IFT", IP_IFT},
	{NULL, 0}
};

extern struct sysconfig        CFG;


int initnl(void)
{
	int		rc = 0;
	FILE		*dbf, *fp;
	char		*filexnm, *path;
	struct _nlfil	fdx;

	filexnm = xstrcpy(CFG.nodelists);
	filexnm	= xstrcat(filexnm,(char *)"/node.files");

	if ((dbf = fopen(filexnm, "r")) == NULL) {
		tasklog('?', "$Can't open %s", filexnm);
		rc = 101;
	} else {
		path = calloc(PATH_MAX, sizeof(char));

		while (fread(&fdx, sizeof(fdx), 1, dbf) == 1) {
			sprintf(path, "%s/%s", CFG.nodelists, fdx.filename);
			if ((fp = fopen(path, "r")) == NULL) {
				tasklog('?', "$Can't open %s", path);
				rc = 101;
			} else {
				fclose(fp);
			}
		}

		fclose(dbf);
		free(path);
	}

	free(filexnm);
	return rc;
}



int comp_node(struct _nlidx, struct _ixentry);
int comp_node(struct _nlidx fap1, struct _ixentry fap2)
{
	if (fap1.zone != fap2.zone)
		return (fap1.zone - fap2.zone);
	else if (fap1.net != fap2.net)
		return (fap1.net - fap2.net);
	else if (fap1.node != fap2.node)
		return (fap1.node - fap2.node);
	else
		return (fap1.point - fap2.point);
}



node *getnlent(faddr *addr)
{
	FILE		*fp;
	static node	nodebuf;
	static char	buf[256], *p, *q;
	struct _ixentry	xaddr;
	int		i, j, Found = FALSE;
	int		ixflag, stdflag;
	char		*mydomain, *path;
	struct _nlfil	fdx;
	struct _nlidx	ndx;
	long		lowest, highest, current;

	mydomain = xstrcpy(CFG.aka[0].domain);
	if (mydomain == NULL) 
		mydomain = (char *)NULLDOMAIN;

	nodebuf.addr.domain = NULL;
	nodebuf.addr.zone   = 0;
	nodebuf.addr.net    = 0;
	nodebuf.addr.node   = 0;
	nodebuf.addr.point  = 0;
	nodebuf.addr.name   = NULL;
	nodebuf.upnet	    = 0;
	nodebuf.upnode      = 0;
	nodebuf.region      = 0;
	nodebuf.type        = 0;
	nodebuf.pflag       = 0;
	nodebuf.name        = NULL;
	nodebuf.location    = NULL;
	nodebuf.sysop       = NULL;
	nodebuf.phone       = NULL;
	nodebuf.speed       = 0;
	nodebuf.mflags      = 0L;
	nodebuf.oflags      = 0L;
	nodebuf.xflags      = 0L;
	nodebuf.iflags      = 0L;
	nodebuf.dflags      = 0L;
	nodebuf.uflags[0]   = NULL;
	nodebuf.t1	    = '\0';
	nodebuf.t2	    = '\0';

	if (addr == NULL) 
		goto retdummy;

	if (addr->zone == 0)
		addr->zone = CFG.aka[0].zone;
	xaddr.zone = addr->zone;
	nodebuf.addr.zone = addr->zone;
	xaddr.net = addr->net;
	nodebuf.addr.net = addr->net;
	xaddr.node = addr->node;
	nodebuf.addr.node = addr->node;
	xaddr.point = addr->point;
	nodebuf.addr.point = addr->point;

	if (initnl())
		goto retdummy;

	/*
	 *  First, lookup node in index. NOTE -- NOT 5D YET
	 */
	path = calloc(128, sizeof(char));
	sprintf(path, "%s/%s", CFG.nodelists, "node.index");
	if ((fp = fopen(path, "r")) == NULL) {
		tasklog('?', "$Can't open %s", path);
		free(path);
		goto retdummy;
	}

	fseek(fp, 0, SEEK_END);
	highest = ftell(fp) / sizeof(ndx);
	lowest = 0;

	while (TRUE) {
		current = ((highest - lowest) / 2) + lowest;
		fseek(fp, current * sizeof(ndx), SEEK_SET);
		if (fread(&ndx, sizeof(ndx), 1, fp) != 1)
			break;

		if (comp_node(ndx, xaddr) == 0) {
			Found = TRUE;
			break;
		}
		if (comp_node(ndx, xaddr) < 0)
			lowest = current;
		else
			highest = current;
		if ((highest - lowest) <= 1)
			break;
	}

	fclose(fp);

	if (!Found) {
		free(path);
		goto retdummy;
	}

	sprintf(path, "%s/%s", CFG.nodelists, "node.files");
	if ((fp = fopen(path, "r")) == NULL) {
		tasklog('?', "$Can't open %s", path);
		free(path);
		goto retdummy;
	}

	/*
	 *  Get filename from node.files
	 */
	for (i = 0; i < (ndx.fileno +1); i++)
		fread(&fdx, sizeof(fdx), 1, fp);
	fclose(fp);

	/* CHECK DOMAIN HERE */

	/*
	 *  Open and read in real nodelist
	 */
	sprintf(path, "%s/%s", CFG.nodelists, fdx.filename);
	if ((fp = fopen(path, "r")) == NULL) {
		tasklog('?', "$Can't open %s", path);
		free(path);
		goto retdummy;
	}
	free(path);

	if (fseek(fp, ndx.offset, SEEK_SET) != 0) {
		tasklog('?', "$Seek failed for nodelist entry");
		fclose(fp);
		goto retdummy;
	}

	if (fgets(buf, sizeof(buf)-1, fp) == NULL) {
		tasklog('?', "$fgets failed for nodelist entry");
		fclose(fp);
		goto retdummy;
	}
	fclose(fp);

	nodebuf.type = ndx.type;
	nodebuf.pflag = ndx.pflag;

	if (*(p = buf + strlen(buf) -1) == '\n') 
		*p = '\0';
	if (*(p = buf + strlen(buf) -1) == '\r') 
		*p = '\0';
	for (p = buf; *p; p++) 
		if (*p == '_') 
			*p = ' ';

	p = buf;

	if ((q = strchr(p,','))) 
		*q++ = '\0';

	p = q;
	if (p == NULL) 
		goto badsyntax;

	if ((q=strchr(p,','))) 
		*q++ = '\0';
	p = q;
	if (p == NULL) 
		goto badsyntax;

	if ((q=strchr(p,','))) 
		*q++ = '\0';
	nodebuf.name = p;
	p = q;
	if (p == NULL) 
		goto badsyntax;

	if ((q=strchr(p,','))) 
		*q++ = '\0';
	nodebuf.location = p;
	p = q;
	if (p == NULL) 
		goto badsyntax;

	if ((q=strchr(p,','))) 
		*q++ = '\0';
	nodebuf.sysop = p;
	p = q;
	if (p == NULL) 
		goto badsyntax;

	if ((q=strchr(p,','))) 
		*q++ = '\0';
	if (strcasecmp(p, "-Unpublished-") == 0)
		nodebuf.phone = NULL;
	else
		nodebuf.phone = p;
	p = q;
	if (p == NULL) 
		goto badsyntax;

	if ((q=strchr(p,','))) 
		*q++ = '\0';
	nodebuf.speed = atoi(p);

	/*
	 * Process the nodelist flags.
	 */
	ixflag = 0;
	stdflag = TRUE;
	for (p = q; p; p = q) {
		if ((q = strchr(p, ','))) 
			*q++ = '\0';
		if ((strncasecmp(p, "U", 1) == 0) && (strlen(p) == 1)) {
			stdflag = FALSE;
		} else {
			/*
			 * Experimental: process authorized flags and 
			 * User flags both as authorized.
			 */
			for (j = 0; fkey[j].key; j++)
				if (strcasecmp(p, fkey[j].key) == 0)
					nodebuf.mflags |= fkey[j].flag;
			for (j = 0; okey[j].key; j++)
				if (strcasecmp(p, okey[j].key) == 0) 
					nodebuf.oflags |= okey[j].flag;
			for (j = 0; dkey[j].key; j++)
				if (strcasecmp(p, dkey[j].key) == 0)
					nodebuf.dflags |= dkey[j].flag;
			for (j = 0; ikey[j].key; j++)
				if (strncasecmp(p, ikey[j].key, strlen(ikey[j].key)) == 0)
					nodebuf.iflags |= ikey[j].flag;
			for (j = 0; xkey[j].key; j++)
				if (strcasecmp(p, xkey[j].key) == 0)
					nodebuf.xflags |= xkey[j].flag;
			if ((p[0] == 'T') && (strlen(p) == 3)) {
				/*
				 * System open hours flag
				 */
				nodebuf.t1 = p[1];
				nodebuf.t2 = p[2];
			}
			if (!stdflag) {
				if (ixflag < MAXUFLAGS) {
					nodebuf.uflags[ixflag++] = p;
					if (ixflag < MAXUFLAGS) 
						nodebuf.uflags[ixflag] = NULL;
				}
			}
		}
	}

	nodebuf.addr.name = nodebuf.sysop;
	nodebuf.addr.domain = xstrcpy(fdx.domain);
	nodebuf.upnet  = ndx.upnet;
	nodebuf.upnode = ndx.upnode;
	nodebuf.region = ndx.region;
	if (addr->domain == NULL) 
		addr->domain = xstrcpy(nodebuf.addr.domain);

	free(mydomain);
	return &nodebuf;

badsyntax:
	tasklog('?', "nodelist %d offset +%lu: bad syntax in line \"%s\"",
		ndx.fileno, (unsigned long)ndx.offset, buf);
	/* fallthrough */

retdummy:
	memset(&nodebuf, 0, sizeof(nodebuf));
	nodebuf.pflag = NL_DUMMY;
	nodebuf.name = (char *)"Unknown";
	nodebuf.location = (char *)"Nowhere";
	nodebuf.sysop = (char *)"Sysop";
	nodebuf.phone = NULL;
	nodebuf.speed = 2400;
	free(mydomain);

	return &nodebuf;
}



