/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Pack filebase
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "mbfutil.h"
#include "mbfpack.h"



extern int	do_quiet;		/* Suppress screen output	*/
extern int	do_index;		/* Reindex filebases		*/


/*
 *  Removes records who are marked for deletion. If there is still a file
 *  on disk, it will be removed too.
 */
void PackFileBase(void)
{
	FILE	*fp, *pAreas, *pFile;
	int	i, iAreas, iAreasNew = 0, rc;
	int	iTotal = 0, iRemoved = 0;
	char	*sAreas, *fAreas, *fTmp, fn[PATH_MAX];

	sAreas = calloc(PATH_MAX, sizeof(char));
	fAreas = calloc(PATH_MAX, sizeof(char));
	fTmp   = calloc(PATH_MAX, sizeof(char));

	IsDoing("Pack filebase");
	if (!do_quiet) {
		colour(3, 0);
		printf("Packing file database...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("Can't open %s", sAreas);
		die(0);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	fseek(pAreas, 0, SEEK_END);
	iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

	for (i = 1; i <= iAreas; i++) {

		fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
		fread(&area, areahdr.recsize, 1, pAreas);

		if (area.Available && !area.CDrom) {

			if (!diskfree(CFG.freespace))
				die(101);

			if (!do_quiet) {
				printf("\r%4d => %-44s", i, area.Name);
				fflush(stdout);
			}
			Marker();

			sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);
			sprintf(fTmp,   "%s/fdb/fdb%d.temp", getenv("MBSE_ROOT"), i);

			if ((pFile = fopen(fAreas, "r")) == NULL) {
				Syslog('!', "Creating new %s", fAreas);
				if ((pFile = fopen(fAreas, "a+")) == NULL) {
					WriteError("$Can't create %s", fAreas);
					die(0);
				}
			} 

			if ((fp = fopen(fTmp, "a+")) == NULL) {
				WriteError("$Can't create %s", fTmp);
				die(0);
			}

			while (fread(&file, sizeof(file), 1, pFile) == 1) {

				iTotal++;

				if ((!file.Deleted) && (!file.Double) && (strcmp(file.Name, "") != 0)) {
					fwrite(&file, sizeof(file), 1, fp);
				} else {
					iRemoved++;
					if (file.Double) {
					    Syslog('+', "Removed double record file \"%s\" from area %d", file.LName, i);
					} else {
					    Syslog('+', "Removed file \"%s\" from area %d", file.LName, i);
					    sprintf(fn, "%s/%s", area.Path, file.LName);
					    rc = unlink(fn);
					    if (rc)
						Syslog('+', "Unlink %s failed, result %d", fn, rc);
					    /*
					     * If a dotted version (thumbnail) exists, remove it silently
					     */
					    sprintf(fn, "%s/.%s", area.Path, file.LName);
					    unlink(fn);
					}
					do_index = TRUE;
				}
			}

			fclose(fp);
			fclose(pFile);

			if ((rename(fTmp, fAreas)) == 0) {
				unlink(fTmp);
				chmod(fAreas, 00660);
			}
			iAreasNew++;

		} /* if area.Available */
	}

	fclose(pAreas);
	Syslog('+', "Pack  Areas [%5d] Files [%5d] Removed [%5d]", iAreasNew, iTotal, iRemoved);

	if (!do_quiet) {
		printf("\r                                                              \r");
		fflush(stdout);
	}

	free(fTmp);
	free(sAreas);
	free(fAreas);
}



