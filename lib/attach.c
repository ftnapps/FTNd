/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Attach files to outbound
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "users.h"
#include "records.h"
#include "clcomm.h"
#include "common.h"



int attach(faddr noden, char *ofile, int mode, char flavor)
{
	FILE		*fp;
	char		*flofile;

	if (ofile == NULL)
		return FALSE;

	flofile = calloc(PATH_MAX, sizeof(char));
	sprintf(flofile, "%s", floname(&noden, flavor));

	/*
	 * Check if outbound directory exists and 
	 * create if it doesn't exist.
	 */
	mkdirs(ofile);

	/*
	 *  Attach file to .flo
	 *
	 *  Not that mbcico when connected to a node opens the file "r+",
	 *  locks it with fcntl(F_SETLK), F_RDLCK, whence=0, start=0L, len=0L.
	 *  It seems that this lock is released after the files in the .flo
	 *  files are send. I don't know what will happen if we add entries
	 *  to the .flo files, this must be tested!
	 */
	if ((fp = fopen(flofile, "a+")) == NULL) {
		WriteError("$Can't open %s", flofile);
		WriteError("May be locked by mbcico");
		free(flofile);
		return FALSE;
	}

	switch (mode) {
		case LEAVE:
			if (strlen(CFG.dospath)) {
				if (CFG.leavecase)
					fprintf(fp, "%s\r\n", Unix2Dos(ofile));
				else
					fprintf(fp, "%s\r\n", tu(Unix2Dos(ofile)));
			} else {
				fprintf(fp, "%s\r\n", ofile);
			}
			break;
		case KFS:
			if (strlen(CFG.dospath)) {
				if (CFG.leavecase)
					fprintf(fp, "^%s\r\n", Unix2Dos(ofile));
				else
					fprintf(fp, "^%s\r\n", tu(Unix2Dos(ofile)));
			} else {
				fprintf(fp, "^%s\r\n", ofile);
			}
			break;

		case TFS:
			if (strlen(CFG.dospath)) {
				if (CFG.leavecase)
					fprintf(fp, "#%s\r\n", Unix2Dos(ofile));
				else
					fprintf(fp, "#%s\r\n", tu(Unix2Dos(ofile)));
			} else {
				fprintf(fp, "#%s\r\n", ofile);
			}
			break;
	}

	fclose(fp);
	free(flofile);
	return TRUE;
}



