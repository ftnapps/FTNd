/*****************************************************************************
 *
 * $Id: filetime.c,v 1.5 2004/02/21 17:22:01 mbroek Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "filetime.h"



time_t gmtoff(time_t);
time_t gmtoff(time_t tt)
{
	struct tm	lt;
#ifndef HAVE_TM_GMTOFF
	struct tm	gt;
	time_t		offset;

	lt = *localtime(&tt);
	gt = *gmtime(&tt);
	offset = gt.tm_yday - lt.tm_yday;
	if (offset > 1) 
		offset =- 24;
	else 
		if (offset < -1) 
			offset = 24;
		else 
			offset *= 24;

	offset += gt.tm_hour - lt.tm_hour;
	offset *= 60;
	offset += gt.tm_min - lt.tm_min;
	offset *= 60;
	offset += gt.tm_sec - lt.tm_sec;
	return offset;
#else
	lt = *localtime(&tt);
	return -lt.tm_gmtoff;
#endif
}



/*
 * SEAlink time conversion
 *   FIXME: I think there is one year difference, spec starts at 1 jan 1979, mtime starts at 1 jan 1980
 */
time_t mtime2sl(time_t tt)
{
	return tt - gmtoff(tt);
}



time_t sl2mtime(time_t tt)
{
	return tt + gmtoff(tt);
}



/*
 * Telink time conversion
 */
time_t mtime2tl(time_t tt)
{
	time_t tlt=0L;
	struct tm *tm;

	tm=localtime(&tt);
	tlt |= ((tm->tm_year)-1980) << 25;
	tlt |= (tm->tm_mon) << 21;
	tlt |= (tm->tm_mday) << 16;
	tlt |= (tm->tm_hour) << 11;
	tlt |= (tm->tm_min) << 5;
	tlt |= (tm->tm_sec) >> 1;
	return tlt;
}



time_t tl2mtime(time_t tt)
{
	struct tm	tm;

	tm.tm_year = ((tt >> 25) & 0x7f) + 1980;
	tm.tm_mon  =  (tt >> 21) & 0x0f;
	tm.tm_mday =  (tt >> 16) & 0x1f;
	tm.tm_hour =  (tt >> 11) & 0x1f;
	tm.tm_min  =  (tt >> 5 ) & 0x3f;
	tm.tm_sec =  ((tt      ) & 0x1f) * 2;

	return mktime(&tm);
}

