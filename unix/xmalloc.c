/*****************************************************************************
 *
 * xmalloc.c
 * Purpose ...............: FTNd Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyrioght ...: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
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


/* Replacements for malloc and strdup with error checking.  Too trivial
   to be worth copyrighting :-).  I did that because a lot of code used
   malloc and strdup without checking for NULL pointer, and I like some
   message better than a core dump...  --marekm
   
   Yeh, but.  Remember that bailing out might leave the system in some
   bizarre state.  You really want to put in error checking, then add
   some back-out failure recovery code. -- jfh */


#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmalloc.h"



char *xmalloc(size_t size)
{
    char *ptr;

    ptr = malloc(size);
    if (!ptr && size) {
	fprintf(stderr, "malloc(%d) failed\n", (int) size);
	exit(13);
    }
    return ptr;
}



char *xstrdup(const char *str)
{
    return strcpy(xmalloc(strlen(str) + 1), str);
}



char *xstrcpy(char *src)
{
    char    *tmp;

    if (src == NULL) 
	return(NULL);
    tmp = xmalloc(strlen(src)+1);
    strcpy(tmp, src);
    return tmp;
}

