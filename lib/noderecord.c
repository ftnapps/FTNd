/*****************************************************************************
 *
 * noderecord.c
 * Purpose ...............: Load noderecord
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
#include "users.h"
#include "ftnddb.h"



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


