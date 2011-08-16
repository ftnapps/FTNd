/*****************************************************************************
 *
 * $Id: noderecord.c,v 1.7 2004/02/21 14:24:04 mbroek Exp $
 * Purpose ...............: Load noderecord
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
#include "mbselib.h"
#include "users.h"
#include "mbsedb.h"



int noderecord(faddr *addr)
{
	fidoaddr	fa;

	memset(&fa, 0, sizeof(fa));
	fa.zone   = addr->zone;
	fa.net    = addr->net;
	fa.node   = addr->node;
	fa.point  = addr->point;

	if (!(TestNode(fa)))
		if (!SearchNode(fa)) {
			return FALSE;
		}

	return TRUE;
}


