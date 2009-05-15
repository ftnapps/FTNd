/*****************************************************************************
 *
 * $Id: rfcdate.c,v 1.9 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Date utilities
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


static char *wdays[]={(char *)"Sun",(char *)"Mon",(char *)"Tue",(char *)"Wed",
		      (char *)"Thu",(char *)"Fri",(char *)"Sat"};

static char *months[]={(char *)"Jan",(char *)"Feb",(char *)"Mar",
		       (char *)"Apr",(char *)"May",(char *)"Jun",
		       (char *)"Jul",(char *)"Aug",(char *)"Sep",
		       (char *)"Oct",(char *)"Nov",(char *)"Dec"};



time_t parsefdate(char *str, void *now)
{
	struct tm	tm, *pnow;
	int		i, rc;
	time_t		Now;
	char		*dummy, *pday, *pmon, *pyear, *phour, *pminute, *psecond;
	char		*buf;

	Now = time(NULL);
	pnow = localtime(&Now);
	dummy = pday = pmon = pyear = phour = pminute = psecond = NULL;

	if (str == NULL) {
		WriteError("parsefdate entered NULL");
		return (time_t)0;
	}

	buf = xstrcpy(str);
	rc = 1;
	memset(&tm, 0, sizeof(tm));

	if ((strncasecmp(str,"Sun ",4) == 0) ||
	    (strncasecmp(str,"Mon ",4) == 0) ||
	    (strncasecmp(str,"Tue ",4) == 0) ||
	    (strncasecmp(str,"Wed ",4) == 0) ||
	    (strncasecmp(str,"Thu ",4) == 0) ||
	    (strncasecmp(str,"Fri ",4) == 0) ||
	    (strncasecmp(str,"Sat ",4) == 0)) {
		/*
		 * SEAdog mode
		 */
		if ((dummy = strtok(str, " ")) != NULL)
		    if ((pday = strtok(NULL, " ")) != NULL)
			if ((pmon = strtok(NULL, " ")) != NULL)
			    if ((pyear = strtok(NULL, " ")) != NULL)
				if ((phour = strtok(NULL, ": ")) != NULL)
				    if ((pminute = strtok(NULL, ": ")) != NULL)
					rc = 0;
		psecond = xstrcpy((char *)"00");
	} else {
		/*
		 * FTS-0001 Standard mode
		 */
		if ((pday = strtok(str, " ")) != NULL)
		    if ((pmon = strtok(NULL, " ")) != NULL)
			if ((pyear = strtok(NULL, " ")) != NULL)
			    if ((phour = strtok(NULL, ": ")) != NULL)
				if ((pminute = strtok(NULL, ": ")) != NULL)
				    if ((psecond = strtok(NULL, ": ")) != NULL)
					rc = 0;
	}
	if (rc == 1) {
		WriteError("Could not parse date \"%s\"", str);
		return (time_t)0;
	}

	tm.tm_sec   = atoi(psecond);
	tm.tm_min   = atoi(pminute);
	tm.tm_hour  = atoi(phour);
	tm.tm_mday  = atoi(pday);
	tm.tm_isdst = pnow->tm_isdst;

	for (i = 0; i < 12; i++)
		if (strncasecmp(months[i], pmon, 3) == 0)
			break;
	tm.tm_mon = i;

	tm.tm_year = atoi(pyear);
	if (tm.tm_year < 0) {
		rc = 1;
	} else if (tm.tm_year < 100) {		/* Correct date field 	    */
		while (pnow->tm_year - tm.tm_year > 50) {
			tm.tm_year +=100;	/* Sliding window adaption  */
		}
	} else if (tm.tm_year < 1900) {		/* Field contains year like */
		rc = 2;				/* Timed/Netmgr bug	    */
	} else {
		tm.tm_year -= 1900;		/* 4 Digit year field	    */
		rc = 2;
	}

	/*
	 * Log if something isn't right
	 */
	if (rc)
		Syslog('+', "fdate \"%s\" to %02d-%02d-%d %02d:%02d:%02d rc=%d", buf, 
			tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900, 
			tm.tm_hour, tm.tm_min, tm.tm_sec, rc);

	free(buf);
	return mktime(&tm) - (gmt_offset((time_t)0) * 60);
}



char *rfcdate(time_t now)
{
	static char	buf[40];
	struct tm	ptm, gtm;
	char		sign;
	int		hr, min;
	int		offset;

	if (!now) 
		now = time(NULL);
	ptm = *localtime(&now);

	/*
	 * To get the timezone, compare localtime with GMT.
	 */
	gtm = *gmtime(&now);

	/*
	 * Assume we are never more than 24 hours away.
	 */
	offset = gtm.tm_yday - ptm.tm_yday;
	if (offset > 1)
	    offset = -24;
	else if (offset < -1)
	    offset = 24;
	else
	    offset *= 24;

	/*
	 * Scale in the hours and minutes; ignore seconds.
	 */
	offset += gtm.tm_hour - ptm.tm_hour;
	offset *= 60;
	offset += gtm.tm_min - ptm.tm_min;

	if (offset <= 0) {
		sign = '+';
		offset = -offset;
	} else 
		sign = '-';
	hr = offset / 60L;
	min = offset % 60L;

	snprintf(buf, 40, "%s, %02d %s %04d %02d:%02d:%02d %c%02d%02d", wdays[ptm.tm_wday], ptm.tm_mday, months[ptm.tm_mon],
		ptm.tm_year + 1900, ptm.tm_hour, ptm.tm_min, ptm.tm_sec, sign, hr, min);
	return(buf);
}



