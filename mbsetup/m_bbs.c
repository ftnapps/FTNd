/*****************************************************************************
 *
 * File ..................: m_bbs.c
 * Purpose ...............: BBS Setup Program 
 * Last modification date : 17-Jul-1999
 * Todo ..................: 
 *			    Edit BBS lists
 *                          Edit timebank data
 *  		            Edit safe data
 *
 *****************************************************************************
 * Copyright (C) 1997-1999
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/common.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_lang.h"
#include "m_protocol.h"
#include "m_ol.h"
#include "m_fgroup.h"
#include "m_farea.h"
#include "m_menu.h"
#include "m_bbs.h"
#include "m_limits.h"


void bbs_menu(void)
{
	for (;;) {

		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "8.    BBS SETUP");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.    Edit Security Limits");
		mvprintw( 8, 6, "2.    Edit Language Setup");
		mvprintw( 9, 6, "3.    Edit BBS Menus");
		mvprintw(10, 6, "4.    Edit File Areas");
		mvprintw(11, 6, "5.    Edit Transfer Protocols");
		mvprintw(12, 6, "6.    Edit BBS List Data");
		mvprintw(13, 6, "7.    Edit Oneliners");
		mvprintw(14, 6, "8.    Edit TimeBank data");
		mvprintw(15, 6, "9.    Edit Safe Cracker data");

		switch(select_menu(9)) {
		case 0:
			return;

		case 1:
			EditLimits();
			break;

		case 2:
			EditLanguage();
			break;

		case 3:
			EditMenus();
			break;

		case 4:
			EditFilearea();
			break;

		case 5:
			EditProtocol();
			break;

		case 6:
			break;

		case 7:
			ol_menu();
			break;
		}
	}
}



int bbs_doc(FILE *fp, FILE *toc, int page)
{
	page = newpage(fp, page);
	addtoc(fp, toc, 8, 0, page, (char *)"BBS setup");

	page = bbs_limits_doc(fp, toc, page);
	page = bbs_lang_doc(fp, toc, page);
	page = bbs_menu_doc(fp, toc, page);
	page = bbs_file_doc(fp, toc, page);
	page = bbs_prot_doc(fp, toc, page);

	return page;
}


