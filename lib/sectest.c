/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Security flags access test
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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

#include "../config.h"
#include "libs.h"
#include "structs.h"
#include "common.h"
#include "clcomm.h"


/*
 * Security Access Check
 */
int Access(securityrec us, securityrec ref)
{
    Syslog('B', "User %5d %08lx %08lx", us.level, us.flags, ~us.flags);
    Syslog('B', "Ref. %5d %08lx %08lx", ref.level, ref.flags, ref.notflags);

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
    Syslog('B', "User %5d %08lx %08lx", us.level, us.flags, ~us.flags);
    Syslog('B', "Ref. %5d %08lx %08lx", le_int(ref.level), ref.flags, ref.notflags);

    if (us.level < le_int(ref.level))
	return FALSE;

    if ((ref.notflags & ~us.flags) != ref.notflags)
	return FALSE;

    if ((ref.flags & us.flags) != ref.flags)
	return FALSE;

    return TRUE;
}


