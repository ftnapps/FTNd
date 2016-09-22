/*****************************************************************************
 *
 * bwrite.c
 * Purpose ...............: FTNd Mail Gate
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "bwrite.h"


/* ### Modified by P.Saratxaga on 29 Oct 1995 ###
 * - deleted transcodage (outtab & intab) code (now transcodage is done by
 *   strconv() function.
 */


/*
 * write short (16bit) integer in "standart" byte order
 */
int iwrite(int i, FILE *fp)
{
	putc(i & 0xff,fp);
	putc((i >> 8) & 0xff,fp);
	return 0;
}


/*
 * write 32bit integer in "standart" byte order
 */
int lwrite(int i, FILE *fp)
{
	int c;

	for (c = 0; c < 32; c += 8) 
		putc((i >> c) & 0xff,fp);
	return 0;
}


int awrite(char *s, FILE *fp)
{
	if (s) 
		while (*s) 
			putc(*(s++), fp);
	putc(0,fp);
	return 0;
}



/*
 * write an arbitrary line to message body: change \n to \r,
 */
int cwrite(char *s, FILE *fp, int postlocal)
{
	while (*s) {
		if ((*s == '\n') && !postlocal)
			putc('\r',fp);
		else if ((*s == '\r') && postlocal)
			putc('\n',fp);
		else 
			putc(*s, fp);
		s++;
	}
	return 0;
}



/* 
 * write (multiline) header to kluge: change \n to ' ' and end line with \r
 */
int kwrite(char *s, FILE *fp, int postlocal)
{
	while (*s) {
		if (*s != '\n') 
			putc(*s, fp);
		else if (*(s+1)) 
			putc(' ',fp);
		s++;
	}
	if (postlocal)
		putc('\n',fp);
	else
		putc('\r',fp);
	return 0;
}

