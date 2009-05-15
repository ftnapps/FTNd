/*****************************************************************************
 *
 * $Id: m_mail.c,v 1.8 2004/10/27 11:08:15 mbse Exp $
 * Purpose ...............: Mail Setup Program 
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
#include "m_global.h"
#include "m_marea.h"
#include "m_mgroup.h"
#include "m_mail.h"




void mail_menu(void)
{
	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {

		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 6, "9.    MAIL PROCESSING");
		set_color(CYAN, BLACK);
		mbse_mvprintw( 7, 6, "1.    Edit Echomail Groups");
		mbse_mvprintw( 8, 6, "2.    Edit Echomail Areas");

		switch(select_menu(2)) {
		case 0:
			return;
		case 1:
			EditMGroup();
			break;
		case 2:
			EditMsgarea();
			break;
		}
	}
}



int mail_doc(FILE *fp, FILE *toc, int page)
{
    page = newpage(fp, page);
    addtoc(fp, toc, 9, 0, page, (char *)"Mail setup");

    page = mail_group_doc(fp, toc, page);
    dotter();
    page = mail_area_doc(fp, toc, page);

    return page;
}


