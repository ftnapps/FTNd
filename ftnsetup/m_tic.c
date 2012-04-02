/*****************************************************************************
 *
 * $Id: m_tic.c,v 1.7 2004/10/27 11:08:16 mbse Exp $
 * Purpose ...............: TIC Setup Program 
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_fgroup.h"
#include "m_ticarea.h"
#include "m_magic.h"
#include "m_hatch.h"
#include "m_tic.h"


void tic_menu(void)
{
	for (;;) {

		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 6, "10.   FILEECHO SETUP");
		set_color(CYAN, BLACK);
		mbse_mvprintw( 7, 6, "1.    Edit Fileecho Groups");
		mbse_mvprintw( 8, 6, "2.    Edit Fileecho Areas");
		mbse_mvprintw( 9, 6, "3.    Edit Hatch Manager");
		mbse_mvprintw(10, 6, "4.    Edit Magic files");

		switch(select_menu(4)) {
		case 0:
			return;

		case 1:
			EditFGroup();
			break;

		case 2:
			EditTicarea();
			break;

		case 3:
			EditHatch();
			break;

		case 4:
			EditMagics();
			break;
		}
	}
}



int tic_doc(FILE *fp, FILE *toc, int page)
{
	int	next;

	page = newpage(fp, page);
	addtoc(fp, toc, 10, 0, page, (char *)"Files processing");

	next = tic_group_doc(fp, toc, page);
	next = tic_areas_doc(fp, toc, next);
	next = tic_hatch_doc(fp, toc, next);
	next = tic_magic_doc(fp, toc, next);

	return next;
}


