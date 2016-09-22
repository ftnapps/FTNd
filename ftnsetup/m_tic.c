/*****************************************************************************
 *
 * m_tic.c
 * Purpose ...............: TIC Setup Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2012   Robert James Clay <jame@rocasa.us>
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
#include "../lib/ftndlib.h"
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
		ftnd_mvprintw( 5, 6, "10.   FILEECHO SETUP");
		set_color(CYAN, BLACK);
		ftnd_mvprintw( 7, 6, "1.    Edit Fileecho Groups");
		ftnd_mvprintw( 8, 6, "2.    Edit Fileecho Areas");
		ftnd_mvprintw( 9, 6, "3.    Edit Hatch Manager");
		ftnd_mvprintw(10, 6, "4.    Edit Magic files");

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


