/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/nodelist.h"
#include "../lib/dbnode.h"
#include "session.h"
#include "config.h"

int localoptions;


static struct _ktab {
	char *key;
	int flag;
} ktab[] = {
	{(char *)"Call",	NOCALL},
	{(char *)"WaZOO",	NOWAZOO},
	{(char *)"EMSI",	NOEMSI},
	{(char *)"Freqs",	NOFREQS},
	{(char *)"Zmodem",	NOZMODEM},
	{(char *)"ZedZap",	NOZEDZAP},
	{(char *)"Hydra",	NOHYDRA},
	{NULL,		0}
};



void logoptions(void)
{
	int	i;
	char	*s = NULL;
		
	for (i=0;ktab[i].key;i++) {
		s=xstrcat(s,(char *)" ");
		if (localoptions & ktab[i].flag)
			s=xstrcat(s,(char *)"No");
		s=xstrcat(s,ktab[i].key);
	}
	
	Syslog('+', "Options:%s",s);
	free(s);
}



void rdoptions(int Loaded)
{
	localoptions=0;
	if (CFG.NoFreqs)
		localoptions |= NOFREQS;
	if (CFG.NoCall)
		localoptions |= NOCALL;
	if (CFG.NoEMSI)
		localoptions |= NOEMSI;
	if (CFG.NoWazoo)
		localoptions |= NOWAZOO;
	if (CFG.NoZmodem)
		localoptions |= NOZMODEM;
	if (CFG.NoZedzap)
		localoptions |= NOZEDZAP;
	if (CFG.NoHydra)
		localoptions |= NOHYDRA;

	if (nodes.Aka[0].zone == 0) {
		if (Loaded)
			Syslog('s', "Node not in setup, using default options");
		logoptions();
		return;
	}

	Syslog('s', "rdoptions node %s %s", nodes.Sysop, aka2str(nodes.Aka[0]));

	if (nodes.NoEMSI)
		localoptions |= NOEMSI;
	if (nodes.NoWaZOO)
		localoptions |= NOWAZOO;
	if (nodes.NoFreqs)
		localoptions |= NOFREQS;
	if (nodes.NoCall)
		localoptions |= NOCALL;
	if (nodes.NoZmodem)
		localoptions |= NOZMODEM;
	if (nodes.NoZedzap)
		localoptions |= NOZEDZAP;
	if (nodes.NoHydra)
		localoptions |= NOHYDRA;
	logoptions();
}

