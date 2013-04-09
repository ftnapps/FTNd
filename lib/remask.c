/*****************************************************************************
 *
 * remask.c
 * Purpose ...............: Regular Expression Mask
 *
 *****************************************************************************
 * Copyright (C) 1997-2007 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
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
#include "ftndlib.h"


char *re_mask(char *nm, int forceupper)
{
    char	*p, *q;
    static char	mask[256];

    memset(&mask, 0, sizeof(mask));
    if (forceupper)
	p = tl(nm);
    else
	p = nm;
    q = mask;
    *q++ = '^';
    while ((*p) && (q < (mask + sizeof(mask) - 4))) {
	switch (*p) {
	    case '\\':  *q++ = '\\'; 
			*q++ = '\\'; 
			break;
	    case '?':   *q++ = '.';  
			break;
	    case '.':   *q++ = '\\'; 
			*q++ = '.'; 
			break;
	    case '+':   *q++ = '\\'; 
			*q++ = '+'; 
			break;
	    case '*':   *q++ = '.';  
			*q++ = '*'; 
			break;
	    case '@':   snprintf(q, 9, "[A-Za-z]"); 
			while (*q) 
			    q++; 
			break;
	    case '#':   snprintf(q, 6, "[0-9]"); 
			while (*q) 
			    q++; 
			break;
	    default:    if (forceupper)
			    *q++ = toupper(*p);
			else
			    *q++ = *p;   
			break;
	}
	p++;
    }
    *q++ = '$';
    *q++ = '\0';
    return mask;
}


