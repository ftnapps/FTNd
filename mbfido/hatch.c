/*****************************************************************************
 *
 * Purpose ...............: Hatch files
 *
 *****************************************************************************
 * Copyright (C) 1997-2009
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "utic.h"
#include "rollover.h"
#include "hatch.h"


extern int	do_quiet;
int		Days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
int		Hatched = 0;


int CheckHatch(char *);


void Hatch()
{
    char	*temp;
    FILE	*fp;
    struct tm	*Tm;
    time_t	Now;
    int		LastDay, HatchToday;

    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('+', "Pass: hatch files");
    Now = time(NULL);
    Tm = localtime(&Now);

    LastDay = Days[Tm->tm_mon];
    if (Tm->tm_mon == 1) {
	/*
	 *  Note that with this method each century change is a leapyear, 
	 *  but take in mind that fidonet will no longer exist in 2100.
	 */
	if (!(Tm->tm_year % 4))
	    LastDay++;
    }

    snprintf(temp, PATH_MAX, "%s/etc/hatch.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }

    (void)fread(&hatchhdr, sizeof(hatchhdr), 1, fp);

    while (fread(&hatch, hatchhdr.recsize, 1, fp) == 1) {
	if (hatch.Active) {
	    HatchToday = FALSE;
	    if (hatch.Days[Diw])
		HatchToday = TRUE;
	    if ((hatch.Month[Tm->tm_mday -1]) || (hatch.Month[31] && (LastDay == Tm->tm_mday)))
		HatchToday = TRUE;
	    snprintf(temp, PATH_MAX, "%s", hatch.Spec);

	    if (HatchToday)
		CheckHatch(temp);
	}
    }

    fclose(fp);
    free(temp);
}



int CheckHatch(char *temp)
{
    DIR		    *dp;
    struct dirent   *de;
    char	    *fn, *tf, tmp[4], *temp2;
    int		    i, hatched = FALSE;
    char	    mask[256];
    FILE	    *Tf;

    if (mkdirs(temp, 0775) == FALSE) {
	WriteError("Can't create dir %s", temp);
	return FALSE;
    }

    fn = xstrcpy(strrchr(temp, '/') + 1);

    while (temp[strlen(temp) -1] != '/')
	temp[strlen(temp) -1] = '\0';
    temp[strlen(temp) -1] = '\0';

    if (chdir(temp)) {
	WriteError("$Can't chdir(%s)", temp);
	free(fn);
	return FALSE;
    }

    if ((dp = opendir(temp)) == NULL) {
	WriteError("$Can't opendir(%s)", temp);
	free(fn);
	return FALSE;
    }

    strcpy(mask, re_mask(fn, FALSE));
    if ((re_comp(mask)) == NULL) {
	tf = calloc(PATH_MAX, sizeof(char));
	while ((de = readdir(dp))) {
	    if (re_exec(de->d_name)) {
		hatched = TRUE;
		Syslog('+', "Hatch \"%s\" in area %s", de->d_name, hatch.Name);
		snprintf(tf, PATH_MAX, "%s/%s", CFG.pinbound, MakeTicName());

		if ((Tf = fopen(tf, "a+")) == NULL) {
		    WriteError("Can't create %s", tf);
		} else {
		    fprintf(Tf, "Hatch\r\n");
		    fprintf(Tf, "Created MBSE BBS v%s, %s\r\n", VERSION, SHORTRIGHT);
		    fprintf(Tf, "Area %s\r\n", hatch.Name);
		    if (SearchTic(hatch.Name)) {
			fprintf(Tf, "Origin %s\r\n", aka2str(tic.Aka));
			fprintf(Tf, "From %s\r\n", aka2str(tic.Aka));
		    } else {
			fprintf(Tf, "Origin %s\r\n", aka2str(CFG.aka[0]));
			fprintf(Tf, "From %s\r\n", aka2str(CFG.aka[0]));
			Syslog('?', "Warning: TIC group not found");
		    }
		    if (strlen(hatch.Replace))
			fprintf(Tf, "Replaces %s\r\n", hatch.Replace);
		    if (strlen(hatch.Magic))
			fprintf(Tf, "Magic %s\r\n", hatch.Magic);
		    temp2 = calloc(strlen(de->d_name) + 1, sizeof(char));
		    snprintf(temp2, strlen(de->d_name) + 1, "%s", de->d_name);
		    name_mangle(temp2);
		    fprintf(Tf, "File %s\r\n", temp2);
		    free(temp2);
		    fprintf(Tf, "Fullname %s\r\n", de->d_name);
		    fprintf(Tf, "Pth %s\r\n", temp);
		    fprintf(Tf, "Desc ");
		    for (i = 0; i < strlen(hatch.Desc); i++) {
			if (hatch.Desc[i] != '%') {
			    fprintf(Tf, "%c", hatch.Desc[i]);
			} else {
			    i++;
			    memset(&tmp, 0, sizeof(tmp));
			    if (isdigit(hatch.Desc[i]))
				tmp[0] = hatch.Desc[i];
			    if (isdigit(hatch.Desc[i+1])) {
				tmp[1] = hatch.Desc[i+1];
				i++;
			    }
			    fprintf(Tf, "%c", de->d_name[atoi(tmp) -1]);
			}
		    }
		    fprintf(Tf, "\r\n");
		    fprintf(Tf, "Crc %08x\r\n", file_crc(de->d_name, CFG.slow_util && do_quiet));
		    fprintf(Tf, "Pw %s\r\n", CFG.hatchpasswd);
		    fclose(Tf);
		    Hatched++;
		    StatAdd(&hatch.Hatched , 1);
		}
	    }
	}
	free(tf);
    } else {
	WriteError("Could not create regexp from %s", fn);
    }

    closedir(dp);
    free(fn);
    return hatched;
}


