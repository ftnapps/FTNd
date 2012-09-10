/*****************************************************************************
 *
 * m_bbs.c
 * Purpose ...............: BBS Setup Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2012   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
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
		ftnd_mvprintw( 5, 6, "8.    BBS SETUP");
		set_color(CYAN, BLACK);
		ftnd_mvprintw( 7, 6, "1.    Edit Security Limits");
		ftnd_mvprintw( 8, 6, "2.    Edit Language Setup");
		ftnd_mvprintw( 9, 6, "3.    Edit BBS Menus");
		ftnd_mvprintw(10, 6, "4.    Edit File Areas");
		ftnd_mvprintw(11, 6, "5.    Edit Transfer Protocols");
		ftnd_mvprintw(12, 6, "6.    Edit Oneliners");

		switch(select_menu(6)) {
		case 0: return;

		case 1: EditLimits();
			break;

		case 2: EditLanguage();
			break;

		case 3: EditMenus();
			break;

		case 4: EditFilearea();
			break;

		case 5: EditProtocol();
			break;

		case 6: ol_menu();
			break;
		}
	}
}



int bbs_doc(FILE *fp, FILE *toc, int page)
{
    page = newpage(fp, page);
    addtoc(fp, toc, 8, 0, page, (char *)"BBS setup");

    page = bbs_limits_doc(fp, toc, page);
    dotter();
    page = bbs_lang_doc(fp, toc, page);
    dotter();
    page = bbs_menu_doc(fp, toc, page);
    dotter();
    page = bbs_file_doc(fp, toc, page);
    dotter();
    page = bbs_prot_doc(fp, toc, page);

    return page;
}


