/*****************************************************************************
 *
 * File ..................: mbmail/hash.c
 * Purpose ...............: MBSE BBS Mail Gate
 * Last modification date : 25-Aug-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
#include "hash.h"
#include "lhash.h"


void hash_update_s(unsigned long *id, char *mod)
{
	*id ^= lh_strhash(mod);
}



void hash_update_n(unsigned long *id, unsigned long mod)
{
	char	buf[32];

	sprintf(buf,"%030lu",mod);
	*id ^= lh_strhash(buf);
}


