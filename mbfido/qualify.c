/*****************************************************************************
 *
 * $Id: qualify.c,v 1.4 2004/02/21 17:22:02 mbroek Exp $
 * Purpose ...............: List of qualified systems
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "qualify.h"



void tidy_qualify(qualify **qal)
{
    qualify	*tmp, *old;

    for (tmp = *qal; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *qal = NULL;
}



void fill_qualify(qualify **qal, fidoaddr aka, int orig, int insb)
{
    qualify		*tmp;

    tmp = (qualify *)malloc(sizeof(qualify));
    tmp->next     = *qal;
    tmp->aka      = aka;
    tmp->inseenby = insb;
    tmp->send     = ((!insb) && (!orig));
    tmp->orig     = orig;
    *qal = tmp;
}


