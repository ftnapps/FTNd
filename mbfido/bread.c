/*****************************************************************************
 *
 * File ..................: mbmail/bread.c
 * Purpose ...............: MBSE BBS Mail Gate
 * Last modification date : 20-Jan-2001
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

#include "../lib/libs.h"
#include "bread.h"


extern int pgpsigned;



/*
 * read short (16bit) integer in "standart" byte order
 */
int iread(FILE *fp)
{
	unsigned char lo,hi;

	fread(&lo,1,1,fp);
	fread(&hi,1,1,fp);
	return (hi<<8) | lo;
}



/*
 * read long (32bit) integer in "standart" byte order
 */
long lread(FILE *fp)
{
	int		c;
	unsigned char	buf;
	long		ret = 0L;

	for (c = 0; c < 32; c += 8) {
		fread(&buf, 1, 1, fp);
		ret |= ((unsigned long)buf << c);
	}
	return ret;
}



static int at_zero=0;

char *aread(char *s, int count, FILE *fp)
{
	int i,c,next;

	if (feof(fp)) 
		return(NULL);
	if (s == NULL) 
		return NULL;
	if (at_zero) {
		at_zero=0;
		return NULL;
	}

	for (i = 0, next = 1; (i<count-1) && next;)
		switch (c=getc(fp)) {
		case '\n':	break;
		case '\r':	s[i]='\n';
				i++;
				next=0;
				break;
		case 0x8d:	if (!pgpsigned) 
					s[i]=' ';
				else 
					s[i]=c;
				i++;
				break;
		case '\0':	at_zero=1;
				next=0;
				break;
		default:	s[i]=c;
				i++;
				break;
		}
	s[i]='\0';
	return s;
}


