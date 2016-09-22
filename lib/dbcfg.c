/*****************************************************************************
 *
 * dbcfg.c
 * Purpose ...............: Config Database.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
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
#include "ftnd.h"
#include "users.h"
#include "ftnddb.h"




void InitConfig(void)
{
	if ((getenv("FTND_ROOT")) == NULL) {
		printf("Could not get FTND_ROOT environment variable\n");
		printf("Please set the environment variable ie:\n");
		printf("\"FTND_ROOT=/opt/ftnd;export FTND_ROOT\"\n\n");
		exit(FTNERR_INIT_ERROR);
	}
	LoadConfig();
}



void LoadConfig(void)
{
	FILE	*pDataFile;
	char	*FileName;

	FileName = calloc(PATH_MAX, sizeof(char));
	snprintf(FileName, PATH_MAX -1, "%s/etc/config.data", getenv("FTND_ROOT"));
	if ((pDataFile = fopen(FileName, "r")) == NULL) {
		perror("\n\nFATAL ERROR:");
		printf(" Can't open %s\n", FileName);
		printf("Please run ftnsetup to create configuration file.\n");
		printf("Or check that your FTND_ROOT variable is set to the correct path!\n\n");
		free(FileName);
		exit(FTNERR_CONFIG_ERROR);
	}

	free(FileName);
	fread(&CFG, sizeof(CFG), 1, pDataFile);
	fclose(pDataFile);
}



int IsOurAka(fidoaddr taka)
{
	int	i;

	for (i = 0; i < 40; i++) {
		if ((taka.zone == CFG.aka[i].zone) &&
		    (taka.net == CFG.aka[i].net) &&
		    (taka.node == CFG.aka[i].node) &&
		    (taka.point == CFG.aka[i].point) &&
		    (CFG.akavalid[i])) 
			return TRUE;
	}
	return FALSE;
}



