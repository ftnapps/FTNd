/*****************************************************************************
 *
 * sectest.c
 * Purpose ...............: Security flags access test
 *
 *****************************************************************************
 * Copyright (C) 1997-2004 Michiel Broek <mbse@mbse.eu>
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


/*
 * Security Access Check
 */
int Access(securityrec us, securityrec ref)
{
    if (us.level < ref.level)
	return FALSE;

    if ((ref.notflags & ~us.flags) != ref.notflags)
	return FALSE;

    if ((ref.flags & us.flags) != ref.flags)
	return FALSE;

    return TRUE;
}



/*
 * The same test, for menus which are written in machine endian independant way.
 * The second parameter MUST be the menu parameter.
 */
int Le_Access(securityrec us, securityrec ref)
{
    if (us.level < le_int(ref.level))
	return FALSE;

    if ((ref.notflags & ~us.flags) != ref.notflags)
	return FALSE;

    if ((ref.flags & us.flags) != ref.flags)
	return FALSE;

    return TRUE;
}


