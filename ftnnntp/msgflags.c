/*****************************************************************************
 *
 * $Id: msgflags.c,v 1.2 2004/06/20 14:38:11 mbse Exp $
 * Purpose ...............: MBSE BBS Mail Gate
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "msgflags.h"

#ifndef	USE_NEWSGATE

static char *flnm[] = {
	(char *)"PVT",(char *)"CRS",(char *)"RCV",(char *)"SNT",
	(char *)"ATT",(char *)"TRN",(char *)"ORP",(char *)"K/S",
	(char *)"LOC",(char *)"HLD",(char *)"RSV",(char *)"FRQ",
	(char *)"RRQ",(char *)"RRC",(char *)"ARQ",(char *)"FUP"
};



int flagset(char *s)
{
	char	*buf,*p;
	int	i;
	int	fl=0;

	buf=xstrcpy(s);
	for (p=strtok(buf," ,\t\n"); p; p=strtok(NULL," ,\t\n")) {
		for (i=0;i<16;i++)
			if (!strcasecmp(p,flnm[i])) {
				fl |= (1 << i);
			}
	}
	free(buf);
	return fl;
}



char *compose_flags(int flags, char *fkludge)
{
	int	i;
	char	*buf = NULL, *p;

	if ((fkludge == NULL) && (!flags))
		return buf;

	if (fkludge) {
		if (!isspace(fkludge[0])) 
			buf=xstrcpy((char *)" ");
		buf=xstrcat(buf,fkludge);
		p=buf+strlen(buf)-1;
		while (isspace(*p)) 
			*p--='\0';
	}

	for (i = 0; i < 16; i++)
		if ((flags & (1 << i)) && (!flag_on(flnm[i],buf))) {
			buf=xstrcat(buf,(char *)" ");
			buf=xstrcat(buf,flnm[i]);
		}
	return buf;
}



char *strip_flags(char *flags)
{
	char	*p,*q=NULL,*tok;
	int	canonic,i;

	if (flags == NULL) 
		return NULL;

	p=xstrcpy(flags);
	for (tok=strtok(flags,", \t\n");tok;tok=strtok(NULL,", \t\n")) {
		canonic=0;
		for (i=0;i<16;i++)
			if (strcasecmp(tok,flnm[i]) == 0)
				canonic=1;
		if (!canonic) {
			q=xstrcat(q,(char *)" ");
			q=xstrcat(q,tok);
		}
	}
	free(p);
	return q;
}



int flag_on(char *flag, char *flags)
{
	char	*p, *tok;
	int	up = FALSE;

	if (flags == NULL)
		return FALSE;

	p = xstrcpy(flags);
	for (tok = strtok(p, ", \t\n"); tok; tok = strtok(NULL, ", \t\n")) {
		if (strcasecmp(flag, tok) == 0) 
			up = TRUE;
	}
	free(p);
	return up;
}

#endif
