/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Menu Utils
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h" 
#include "mutil.h"



unsigned char readkey(int y, int x, int fg, int bg)
{
	int		rc = -1, i;
	unsigned char 	ch = 0;

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 9");
		exit(1);
	}
	Setraw();

	i = 0;
	while (rc == -1) {
		if ((i % 10) == 0)
			show_date(fg, bg, 0, 0);

		locate(y, x);
		fflush(stdout);
		rc = Waitchar(&ch, 5);
		if ((rc == 1) && (ch != KEY_ESCAPE))
			break;

		if ((rc == 1) && (ch == KEY_ESCAPE))
			rc = Escapechar(&ch);

		if (rc == 1)
			break;
		i++;
		Nopper();
	}

	Unsetraw();
	close(ttyfd);

	return ch;
}



unsigned char testkey(int y, int x)
{
	int		rc;
	unsigned char	ch = 0;

	locate(y, x);
	fflush(stdout);

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 9");
		exit(1);
	}
	Setraw();

	rc = Waitchar(&ch, 100);
	if (rc == 1) {
		if (ch == KEY_ESCAPE)
			rc = Escapechar(&ch);
	}

	Unsetraw();
	close(ttyfd);

	if (rc == 1)
		return ch;
	else
		return '\0';
}



int newpage(FILE *fp, int page)
{
	page++;
	fprintf(fp, "\f   MBSE BBS v%-53s   page %d\n", VERSION, page);
	return page;
}



void addtoc(FILE *fp, FILE *toc, int chapt, int par, int page, char *title)
{
	char	temp[81];
	char	*tit;

	sprintf(temp, "%s ", title);
	tit = xstrcpy(title);
	tu(tit);

	if (par) {
		fprintf(toc, "     %2d.%-3d   %s %d\n", chapt, par, padleft(temp, 50, '.'), page);
		fprintf(fp, "\n\n   %d.%d. %s\n\n", chapt, par, tit);
	} else {
		fprintf(toc, "     %2d     %s %d\n", chapt, padleft(temp, 52, '.'), page);
		fprintf(fp, "\n\n   %d. %s\n", chapt, tit);
	}
	free(tit);
}



