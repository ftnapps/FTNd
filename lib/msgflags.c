/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Mail Gate
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "libs.h"
#include "structs.h"
#include "clcomm.h"
#include "common.h"


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

	Syslog('M', "setting flags from string \"%s\"",MBSE_SS(s));
	buf=xstrcpy(s);
	for (p=strtok(buf," ,\t\n"); p; p=strtok(NULL," ,\t\n")) {
		for (i=0;i<16;i++)
			if (!strcasecmp(p,flnm[i])) {
				Syslog('M', "setting flag bit %d for \"%s\"", i,MBSE_SS(p));
				fl |= (1 << i);
			}
	}
	free(buf);
	Syslog('M', "set flags 0x%04x",fl);
	return fl;
}



char *compose_flags(int flags, char *fkludge)
{
	int	i;
	char	*buf = NULL, *p;

	if ((fkludge == NULL) && (!flags))
		return buf;

	Syslog('M', "composing flag string from binary 0x%04x and string \"%s\"", flags,MBSE_SS(fkludge));
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
			Syslog('m', "adding \"%s\"",flnm[i]);
		}
	Syslog('M', "resulting string is \"%s\"",buf);
	return buf;
}



char *strip_flags(char *flags)
{
	char	*p,*q=NULL,*tok;
	int	canonic,i;

	if (flags == NULL) 
		return NULL;

	Syslog('M', "stripping official flags from \"%s\"",MBSE_SS(flags));
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
	Syslog('M', "stripped string is \"%s\"",q);
	return q;
}



int flag_on(char *flag, char *flags)
{
	char	*p, *tok;
	int	up = FALSE;

	if (flags == NULL)
		return FALSE;

	Syslog('m', "checking flag \"%s\" in string \"%s\"", MBSE_SS(flag), MBSE_SS(flags));
	p = xstrcpy(flags);
	for (tok = strtok(p, ", \t\n"); tok; tok = strtok(NULL, ", \t\n")) {
		if (strcasecmp(flag, tok) == 0) 
			up = TRUE;
	}
	free(p);
	Syslog('m', "flag%s present", up ? "":" not");
	return up;
}


