/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Check little/big-endian for install of the menus.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek                FIDO:   2:280/2802
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include "endian.h"

int main(void)
{

    /*
     * First test BYTE_ORDER
     */
#ifdef BYTE_ORDER
    if (BYTE_ORDER == 1234) {
	printf("le");
    } else if (BYTE_ORDER == 4321) {
	printf("be");
    } else {
	/*
	 * If it failed do a simple CPU test
	 */
#endif
#ifdef __i386__
	printf("le");
#else
	printf("be");
#endif
#ifdef BYTE_ORDER
    }
#endif


    return 0;
}

