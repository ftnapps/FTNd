/*****************************************************************************
 *
 * $Id: hash.c,v 1.4 2005/10/11 20:49:48 mbse Exp $
 * Purpose ...............: MBSE BBS Mail Gate
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
#include "../lib/mbselib.h"
#include "hash.h"
#include "lhash.h"

#ifndef	USE_NEWSGATE

void hash_update_s(unsigned int *id, char *mod)
{
	*id ^= lh_strhash(mod);
}



void hash_update_n(unsigned int *id, unsigned int mod)
{
	char	buf[32];

	snprintf(buf,32,"%030u",mod);
	*id ^= lh_strhash(buf);
}


#endif
