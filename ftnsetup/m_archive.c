/*****************************************************************************
 *
 * $Id: m_archive.c,v 1.31 2006/02/26 11:19:03 mbse Exp $
 * Purpose ...............: Setup Archive structure.
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
#include "../paths.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_archive.h"



int	ArchUpdated = 0;


/*
 * Count nr of archiver records in the database.
 * Creates the database if it doesn't exist.
 */
int CountArchive(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];
    int	    count;

    snprintf(ffile, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    archiverhdr.hdrsize = sizeof(archiverhdr);
	    archiverhdr.recsize = sizeof(archiver);
	    fwrite(&archiverhdr, sizeof(archiverhdr), 1, fil);
	    /*
	     *  Create default records. Archivers found during configure
	     *  and installing this software are automatic enabled with the
	     *  right paths. Others are meant as examples.
	     */
	    memset(&archiver, 0, sizeof(archiver));
	    if (strlen(_PATH_ARC) && strlen(_PATH_NOMARCH))
		snprintf(archiver.comment, 41, "ARC and NOMARCH");
	    else
		snprintf(archiver.comment, 41, "ARC Version 5.21");
	    snprintf(archiver.name, 6,    "ARC");
	    archiver.available = FALSE;
	    if (strlen(_PATH_ARC)) {
		archiver.available = TRUE;
		snprintf(archiver.marc, 65,    "%s anw", _PATH_ARC);
		snprintf(archiver.tarc, 65,    "%s tnw", _PATH_ARC);
		snprintf(archiver.varc, 65,    "%s l",   _PATH_ARC);
		snprintf(archiver.funarc, 65,  "%s xnw", _PATH_ARC);
		snprintf(archiver.munarc, 65,  "%s enw", _PATH_ARC);
		snprintf(archiver.iunarc, 65,  "%s enw", _PATH_ARC);
	    } else {
		snprintf(archiver.marc, 65,    "/usr/bin/arc anw");
		snprintf(archiver.tarc, 65,    "/usr/bin/arc tnw");
		snprintf(archiver.varc, 65,    "/usr/bin/arc l");
		snprintf(archiver.funarc, 65,  "/usr/bin/arc xnw");
		snprintf(archiver.munarc, 65,  "/usr/bin/arc enw");
		snprintf(archiver.iunarc, 65,  "/usr/bin/arc enw");
	    }
	    /*
	     * Override arc when nomarch is available
	     */
	    if (strlen(_PATH_NOMARCH)) {
		snprintf(archiver.funarc, 65,  "%s -U", _PATH_NOMARCH);
		snprintf(archiver.munarc, 65,  "%s", _PATH_NOMARCH);
		snprintf(archiver.iunarc, 65,  "%s -U", _PATH_NOMARCH);
		snprintf(archiver.varc, 65,    "%s -l", _PATH_NOMARCH);
	    }
	    fwrite(&archiver, sizeof(archiver), 1, fil);

            memset(&archiver, 0, sizeof(archiver));
            snprintf(archiver.comment, 41, "LHarc");
	    snprintf(archiver.name, 6,    "LHA");
	    if (strlen(_PATH_LHA)) {
		archiver.available = TRUE;
		snprintf(archiver.marc, 65,    "%s aq", _PATH_LHA);
		snprintf(archiver.tarc, 65,    "%s tq", _PATH_LHA);
		snprintf(archiver.varc, 65,    "%s l", _PATH_LHA);
		snprintf(archiver.funarc, 65,  "%s xqf", _PATH_LHA);
		snprintf(archiver.munarc, 65,  "%s eqf", _PATH_LHA);
		snprintf(archiver.iunarc, 65,  "%s eqf", _PATH_LHA);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.marc, 65,    "/usr/bin/lha aq");
		snprintf(archiver.tarc, 65,    "/usr/bin/lha tq");
		snprintf(archiver.varc, 65,    "/usr/bin/lha l");
		snprintf(archiver.funarc, 65,  "/usr/bin/lha xqf");
		snprintf(archiver.munarc, 65,  "/usr/bin/lha eqf");
		snprintf(archiver.iunarc, 65,  "/usr/bin/lha eqf");
	    }
            fwrite(&archiver, sizeof(archiver), 1, fil);

            memset(&archiver, 0, sizeof(archiver));
            snprintf(archiver.comment, 41, "RAR by Eugene Roshal");
            snprintf(archiver.name, 6,    "RAR");
	    if (strlen(_PATH_RAR)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s a -y -r", _PATH_RAR);
		snprintf(archiver.marc, 65,    "%s a -y", _PATH_RAR);
		snprintf(archiver.barc, 65,    "%s c -y", _PATH_RAR);
		snprintf(archiver.tarc, 65,    "%s t -y", _PATH_RAR);
		snprintf(archiver.varc, 65,    "%s l", _PATH_RAR);
		snprintf(archiver.funarc, 65,  "%s x -o+ -y -r", _PATH_RAR);
		snprintf(archiver.munarc, 65,  "%s e -o+ -y", _PATH_RAR);
		snprintf(archiver.iunarc, 65,  "%s e -cu", _PATH_RAR);
	    } else if (strlen(_PATH_UNRAR)) {
		archiver.available = TRUE;
		snprintf(archiver.funarc, 65,  "%s x -o+ -y -r", _PATH_UNRAR);
		snprintf(archiver.munarc, 65,  "%s e -o+ -y", _PATH_UNRAR);
		snprintf(archiver.iunarc, 65,  "%s e -cu", _PATH_UNRAR);
		snprintf(archiver.varc, 65,    "%s l", _PATH_UNRAR);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/usr/bin/rar a -y -r");
		snprintf(archiver.marc, 65,    "/usr/bin/rar a -y");
		snprintf(archiver.barc, 65,    "/usr/bin/rar c -y");
		snprintf(archiver.tarc, 65,    "/usr/bin/rar t -y");
		snprintf(archiver.varc, 65,    "/usr/bin/rar l");
		snprintf(archiver.funarc, 65,  "/usr/bin/unrar x -o+ -y -r");
		snprintf(archiver.munarc, 65,  "/usr/bin/unrar e -o+ -y");
		snprintf(archiver.iunarc, 65,  "/usr/bin/unrar e -cu");
	    }
            fwrite(&archiver, sizeof(archiver), 1, fil);

            memset(&archiver, 0, sizeof(archiver));
            snprintf(archiver.comment, 41, "TAR gzip files");
            snprintf(archiver.name, 6,    "GZIP");
	    if (strlen(_PATH_TAR)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s cfz", _PATH_TAR);
		snprintf(archiver.marc, 65,    "%s Afz", _PATH_TAR);
		snprintf(archiver.tarc, 65,    "%s tfz", _PATH_TAR);
		snprintf(archiver.varc, 65,    "%s tfz", _PATH_TAR);
		snprintf(archiver.funarc, 65,  "%s xfz", _PATH_TAR);
		snprintf(archiver.munarc, 65,  "%s xfz", _PATH_TAR);
		snprintf(archiver.iunarc, 65,  "%s xfz", _PATH_TAR);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/bin/tar cfz");
		snprintf(archiver.marc, 65,    "/bin/tar Afz");
		snprintf(archiver.tarc, 65,    "/bin/tar tfz");
		snprintf(archiver.varc, 65,    "/bin/tar tfz");
		snprintf(archiver.funarc, 65,  "/bin/tar xfz");
		snprintf(archiver.munarc, 65,  "/bin/tar xfz");
		snprintf(archiver.iunarc, 65,  "/bin/tar xfz");
	    }
            fwrite(&archiver, sizeof(archiver), 1, fil);
	    snprintf(archiver.comment, 41, "TAR compressed files");
	    snprintf(archiver.name, 6,    "CMP");
	    if (strlen(_PATH_TAR)) {
		snprintf(archiver.farc, 65,    "%s cfZ", _PATH_TAR);
		snprintf(archiver.marc, 65,    "%s AfZ", _PATH_TAR);
	    } else {
		snprintf(archiver.farc, 65,    "/bin/tar cfZ");
		snprintf(archiver.marc, 65,    "/bin/tar AfZ");
	    }
	    fwrite(&archiver, sizeof(archiver), 1, fil);

	    memset(&archiver, 0, sizeof(archiver));
	    snprintf(archiver.comment, 41, "TAR bzip2 files");
	    snprintf(archiver.name, 6,    "BZIP");
	    if (strlen(_PATH_TAR)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s cfj", _PATH_TAR);
		snprintf(archiver.marc, 65,    "%s Afj", _PATH_TAR);
		snprintf(archiver.tarc, 65,    "%s tfj", _PATH_TAR);
		snprintf(archiver.varc, 65,    "%s tfj", _PATH_TAR);
		snprintf(archiver.funarc, 65,  "%s xfj", _PATH_TAR);
		snprintf(archiver.munarc, 65,  "%s xfj", _PATH_TAR);
		snprintf(archiver.iunarc, 65,  "%s xfj", _PATH_TAR);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/bin/tar cfj");
		snprintf(archiver.marc, 65,    "/bin/tar Afj");
		snprintf(archiver.tarc, 65,    "/bin/tar tfj");
		snprintf(archiver.varc, 65,    "/bin/tar tfj");
		snprintf(archiver.funarc, 65,  "/bin/tar xfj");
		snprintf(archiver.munarc, 65,  "/bin/tar xfj");
		snprintf(archiver.iunarc, 65,  "/bin/tar xfj");
	    }
	    fwrite(&archiver, sizeof(archiver), 1, fil);

	    memset(&archiver, 0, sizeof(archiver));
	    snprintf(archiver.comment, 41, "TAR files");
	    snprintf(archiver.name, 6,    "TAR");
	    if (strlen(_PATH_TAR)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s cf", _PATH_TAR);
		snprintf(archiver.marc, 65,    "%s Af", _PATH_TAR);
		snprintf(archiver.tarc, 65,    "%s tf", _PATH_TAR);
		snprintf(archiver.varc, 65,    "%s tf", _PATH_TAR);
		snprintf(archiver.funarc, 65,  "%s xf", _PATH_TAR);
		snprintf(archiver.munarc, 65,  "%s xf", _PATH_TAR);
		snprintf(archiver.iunarc, 65,  "%s xf", _PATH_TAR);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/bin/tar cf");
		snprintf(archiver.marc, 65,    "/bin/tar Af");
		snprintf(archiver.tarc, 65,    "/bin/tar tf");
		snprintf(archiver.varc, 65,    "/bin/tar tf");
		snprintf(archiver.funarc, 65,  "/bin/tar xf");
		snprintf(archiver.munarc, 65,  "/bin/tar xf");
		snprintf(archiver.iunarc, 65,  "/bin/tar xf");
	    }
	    fwrite(&archiver, sizeof(archiver), 1, fil);

            memset(&archiver, 0, sizeof(archiver));
            snprintf(archiver.comment, 41, "UNARJ by Robert K Jung");
            snprintf(archiver.name, 6,    "ARJ");
	    /*
	     * Even if it is found, we won't enable unarj if the
	     * Russion arj is found since that is more complete.
	     */
	    if (strlen(_PATH_UNARJ) && (! strlen(_PATH_ARJ)))
		archiver.available = TRUE;
	    else
		archiver.available = FALSE;
	    if (strlen(_PATH_UNARJ)) {
		snprintf(archiver.tarc, 65,    "%s t", _PATH_UNARJ);
		snprintf(archiver.varc, 65,    "%s l", _PATH_UNARJ);
		snprintf(archiver.funarc, 65,  "%s x", _PATH_UNARJ);
		snprintf(archiver.munarc, 65,  "%s e", _PATH_UNARJ);
		snprintf(archiver.iunarc, 65,  "%s e", _PATH_UNARJ);
	    } else {
		snprintf(archiver.tarc, 65,    "/usr/bin/unarj t");
		snprintf(archiver.varc, 65,    "/usr/bin/unarj l");
		snprintf(archiver.funarc, 65,  "/usr/bin/unarj x");
		snprintf(archiver.munarc, 65,  "/usr/bin/unarj e");
		snprintf(archiver.iunarc, 65,  "/usr/bin/unarj e");
	    }
            fwrite(&archiver, sizeof(archiver), 1, fil);

	    memset(&archiver, 0, sizeof(archiver));
	    snprintf(archiver.comment, 41, "ARJ from ARJ Software Russia");
	    snprintf(archiver.name, 6,    "ARJ");
	    if (strlen(_PATH_ARJ)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s -2d -y -r a", _PATH_ARJ);
		snprintf(archiver.marc, 65,    "%s -2d -y -e a", _PATH_ARJ);
		snprintf(archiver.barc, 65,    "%s -2d -y c", _PATH_ARJ);
		snprintf(archiver.tarc, 65,    "%s -y t", _PATH_ARJ);
		snprintf(archiver.varc, 65,    "%s l", _PATH_ARJ);
		snprintf(archiver.funarc, 65,  "%s -y x", _PATH_ARJ);
		snprintf(archiver.munarc, 65,  "%s -y e", _PATH_ARJ);
		snprintf(archiver.iunarc, 65,  "%s -y e", _PATH_ARJ);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/usr/bin/arj -2d -y -r a");
		snprintf(archiver.marc, 65,    "/usr/bin/arj -2d -y -e a");
		snprintf(archiver.barc, 65,    "/usr/bin/arj -2d -y c");
		snprintf(archiver.tarc, 65,    "/usr/bin/arj -y t");
		snprintf(archiver.varc, 65,    "/usr/bin/arj l");
		snprintf(archiver.funarc, 65,  "/usr/bin/arj -y x");
		snprintf(archiver.munarc, 65,  "/usr/bin/arj -y e");
		snprintf(archiver.iunarc, 65,  "/usr/bin/arj -y e");
	    }
	    fwrite(&archiver, sizeof(archiver), 1, fil);

            memset(&archiver, 0, sizeof(archiver));
            snprintf(archiver.comment, 41, "ZIP and UNZIP by Info-ZIP");
            snprintf(archiver.name, 6,    "ZIP");
	    if (strlen(_PATH_ZIP) && strlen(_PATH_UNZIP))
		archiver.available = TRUE;
	    else
		archiver.available = FALSE;
	    if (strlen(_PATH_ZIP)) {
		snprintf(archiver.farc, 65,    "%s -r -q", _PATH_ZIP);
		snprintf(archiver.marc, 65,    "%s -q", _PATH_ZIP);
		snprintf(archiver.barc, 65,    "%s -z", _PATH_ZIP);
		snprintf(archiver.tarc, 65,    "%s -T", _PATH_ZIP);
	    } else {
		snprintf(archiver.farc, 65,    "/usr/bin/zip -r -q");
		snprintf(archiver.marc, 65,    "/usr/bin/zip -q");
		snprintf(archiver.barc, 65,    "/usr/bin/zip -z");
		snprintf(archiver.tarc, 65,    "/usr/bin/zip -T");
	    }
	    if (strlen(_PATH_UNZIP)) {
		snprintf(archiver.funarc, 65,  "%s -o -q", _PATH_UNZIP);
		snprintf(archiver.munarc, 65,  "%s -o -j -L", _PATH_UNZIP);
		snprintf(archiver.iunarc, 65,  "%s -o -j", _PATH_UNZIP);
		snprintf(archiver.varc, 65,    "%s -l", _PATH_UNZIP);
	    } else {
		snprintf(archiver.funarc, 65,  "/usr/bin/unzip -o -q");
		snprintf(archiver.munarc, 65,  "/usr/bin/unzip -o -j -L");
		snprintf(archiver.iunarc, 65,  "/usr/bin/unzip -o -j -L");
		snprintf(archiver.varc, 65,    "/usr/bin/unzip -l");
	    }
            fwrite(&archiver, sizeof(archiver), 1, fil);

            memset(&archiver, 0, sizeof(archiver));
            snprintf(archiver.comment, 41, "ZOO archiver");
            snprintf(archiver.name, 6,    "ZOO");
	    if (strlen(_PATH_ZOO)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s aq", _PATH_ZOO);
		snprintf(archiver.marc, 65,    "%s aq:O", _PATH_ZOO);
		snprintf(archiver.barc, 65,    "%s aqC", _PATH_ZOO);
		snprintf(archiver.varc, 65,    "%s -list", _PATH_ZOO);
		snprintf(archiver.funarc, 65,  "%s xqO", _PATH_ZOO);
		snprintf(archiver.munarc, 65,  "%s eq:O", _PATH_ZOO);
		snprintf(archiver.iunarc, 65,  "%s eqO", _PATH_ZOO);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/usr/bin/zoo aq");
		snprintf(archiver.marc, 65,    "/usr/bin/zoo aq:O");
		snprintf(archiver.barc, 65,    "/usr/bin/zoo aqC");
		snprintf(archiver.varc, 65,    "/usr/bin/zoo -list");
		snprintf(archiver.funarc, 65,  "/usr/bin/zoo xqO");
		snprintf(archiver.munarc, 65,  "/usr/bin/zoo eq:O");
		snprintf(archiver.iunarc, 65,  "/usr/bin/zoo eqO");
	    }
            fwrite(&archiver, sizeof(archiver), 1, fil);

	    memset(&archiver, 0, sizeof(archiver));
	    snprintf(archiver.comment, 41, "HA Harri Hirvola");
	    snprintf(archiver.name, 6,    "HA");
	    if (strlen(_PATH_HA)) {
		archiver.available = TRUE;
		snprintf(archiver.farc, 65,    "%s a21rq", _PATH_HA);
		snprintf(archiver.marc, 65,    "%s a21q", _PATH_HA);
		snprintf(archiver.tarc, 65,    "%s t", _PATH_HA);
		snprintf(archiver.varc, 65,    "%s l", _PATH_HA);
		snprintf(archiver.funarc, 65,  "%s eyq", _PATH_HA);
		snprintf(archiver.munarc, 65,  "%s eyq", _PATH_HA);
		snprintf(archiver.iunarc, 65,  "%s eyq", _PATH_HA);
	    } else {
		archiver.available = FALSE;
		snprintf(archiver.farc, 65,    "/usr/bin/ha a21rq");
		snprintf(archiver.marc, 65,    "/usr/bin/ha a21q");
		snprintf(archiver.tarc, 65,    "/usr/bin/ha t");
		snprintf(archiver.varc, 65,    "/usr/bin/ha l");
		snprintf(archiver.funarc, 65,  "/usr/bin/ha eyq");
		snprintf(archiver.munarc, 65,  "/usr/bin/ha eyq");
		snprintf(archiver.iunarc, 65,  "/usr/bin/ha eyq");
	    }
	    fwrite(&archiver, sizeof(archiver), 1, fil);

	    fclose(fil);
	    chmod(ffile, 0640);
	    return 12;
	} else
	    return -1;
    }

    fread(&archiverhdr, sizeof(archiverhdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - archiverhdr.hdrsize) / archiverhdr.recsize;
    fclose(fil);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenArchive(void);
int OpenArchive(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/archiver.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&archiverhdr, sizeof(archiverhdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = archiverhdr.recsize;
			if (oldsize != sizeof(archiver)) {
				ArchUpdated = 1;
				Syslog('+', "Format of %s changed, updateing", fnin);
			} else
				ArchUpdated = 0;
			archiverhdr.hdrsize = sizeof(archiverhdr);
			archiverhdr.recsize = sizeof(archiver);
			fwrite(&archiverhdr, sizeof(archiverhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&archiver, 0, sizeof(archiver));
			while (fread(&archiver, oldsize, 1, fin) == 1) {
				fwrite(&archiver, sizeof(archiver), 1, fout);
				memset(&archiver, 0, sizeof(archiver));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseArchive(int);
void CloseArchive(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*arc = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/archiver.temp", getenv("MBSE_ROOT"));

	if (ArchUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&archiverhdr, archiverhdr.hdrsize, 1, fi);
			fwrite(&archiverhdr, archiverhdr.hdrsize, 1, fo);

			while (fread(&archiver, archiverhdr.recsize, 1, fi) == 1)
				if (!archiver.deleted)
					fill_stlist(&arc, archiver.comment, ftell(fi) - archiverhdr.recsize);
			sort_stlist(&arc);

			for (tmp = arc; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&archiver, archiverhdr.recsize, 1, fi);
				fwrite(&archiver, archiverhdr.recsize, 1, fo);
			}

			tidy_stlist(&arc);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"archiver.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendArchive(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/archiver.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&archiver, 0, sizeof(archiver));
		fwrite(&archiver, sizeof(archiver), 1, fil);
		fclose(fil);
		ArchUpdated = 1;
		return 0;
	} else
		return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditArchRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX];
    int		    offset;
    int		    j;
    unsigned int    crc, crc1;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Archiver");

    snprintf(mfile, PATH_MAX, "%s/etc/archiver.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    offset = sizeof(archiverhdr) + ((Area -1) * sizeof(archiver));
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	return -1;
    }

    fread(&archiver, sizeof(archiver), 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&archiver, crc, sizeof(archiver));

    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "3.  EDIT ARCHIVER");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.  Comment");
    mbse_mvprintw( 8, 2, "2.  Name");
    mbse_mvprintw( 9, 2, "3.  Available");
    mbse_mvprintw(10, 2, "4.  Deleted");
    mbse_mvprintw(11, 2, "5.  Arc files");
    mbse_mvprintw(12, 2, "6.  Arc mail");
    mbse_mvprintw(13, 2, "7.  Banners");
    mbse_mvprintw(14, 2, "8.  Arc test");
    mbse_mvprintw(15, 2, "9.  Un. files");
    mbse_mvprintw(16, 2, "10. Un. mail");
    mbse_mvprintw(17, 2, "11. FILE_ID");
    mbse_mvprintw(18, 2, "12. List arc");

    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,16,40, archiver.comment);
	show_str( 8,16, 5, archiver.name);
	show_bool(9,16,    archiver.available);
	show_bool(10,16,   archiver.deleted);
	show_str(11,16,64, archiver.farc);
	show_str(12,16,64, archiver.marc);
	show_str(13,16,64, archiver.barc);
	show_str(14,16,64, archiver.tarc);
	show_str(15,16,64, archiver.funarc);
	show_str(16,16,64, archiver.munarc);
	show_str(17,16,64, archiver.iunarc);
	show_str(18,16,64, archiver.varc);

	j = select_menu(12);
	switch(j) {
	    case 0: crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&archiver, crc1, sizeof(archiver));
		    if (crc != crc1) {
			if (yes_no((char *)"Record is changed, save") == 1) {
			    working(1, 0, 0);
			    if ((fil = fopen(mfile, "r+")) == NULL) {
				working(2, 0, 0);
				return -1;
			    }
			    fseek(fil, offset, 0);
			    fwrite(&archiver, sizeof(archiver), 1, fil);
			    fclose(fil);
			    ArchUpdated = 1;
			    working(6, 0, 0);
			}
		    }
		    IsDoing("Browsing Menu");
		    return 0;
	    case 1: E_STR(  7,16,40,archiver.comment,  "The ^Comment^ for this record")
	    case 2: E_STR(  8,16,5, archiver.name,     "The ^name^ of this archiver")
	    case 3: E_BOOL( 9,16,   archiver.available,"Switch if this archiver is ^Available^ for use.")
	    case 4: E_BOOL(10,16,   archiver.deleted,  "Is this archiver ^deleted^")
	    case 5: E_STR( 11,16,64,archiver.farc,     "The ^Archive^ command for files")
	    case 6: E_STR( 12,16,64,archiver.marc,     "The ^Archive^ command for mail packets")
	    case 7: E_STR( 13,16,64,archiver.barc,     "The ^Archive^ command to insert/replace banners")
	    case 8: E_STR( 14,16,64,archiver.tarc,     "The ^Archive^ command to test an archive")
	    case 9: E_STR( 15,16,64,archiver.funarc,   "The ^Unarchive^ command for files")
	    case 10:E_STR( 16,16,64,archiver.munarc,   "The ^Unarchive^ command for mail packets")
	    case 11:E_STR( 17,16,64,archiver.iunarc,   "The ^Unarchive^ command to extract the FILE_ID.DIZ file")
	    case 12:E_STR( 18,16,64,archiver.varc,     "The ^List^ command to show the archive contents")
	}
    }

    return 0;
}



void EditArchive(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountArchive();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenArchive() == -1) {
		working(2, 0, 0);
		return;
	}
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "3.  ARCHIVER SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/archiver.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&archiverhdr, sizeof(archiverhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
					    x = 42;
					    y = 7;
					}
					if ((o + i) <= records) {
					    offset = sizeof(archiverhdr) + (((o + i) - 1) * archiverhdr.recsize);
					    fseek(fil, offset, 0);
					    fread(&archiver, archiverhdr.recsize, 1, fil);
					    if (archiver.available)
						set_color(CYAN, BLACK);
					    else
						set_color(LIGHTBLUE, BLACK);
					    snprintf(temp, 81, "%3d. %-5s %-26s", i, archiver.name, archiver.comment);
					    temp[38] = 0;
					    mbse_mvprintw(y, x, temp);
					    y++;
					}
				}
				fclose(fil);
			}
			/* Show records here */
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseArchive(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendArchive() == 0) {
				records++;
				working(3, 0, 0);
			} else
				working(2, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0)
		    if ((o + 20) < records)
			o += 20;

		if (strncmp(pick, "P", 1) == 0)
		    if ((o - 20) >= 0)
			o -= 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditArchRec(atoi(pick));
			o = ((atoi(pick) -1) / 20) * 20;
		}
	}
}



void InitArchive(void)
{
    CountArchive();
    OpenArchive();
    CloseArchive(TRUE);
}



char *PickArchive(char *shdr, int mailmode)
{
    static char Arch[6] = "";
    int		records, i, y, o = 0;
    char	pick[12];
    FILE	*fil;
    char	temp[PATH_MAX];
    int		offset;


    clr_index();
    working(1, 0, 0);
    if (config_read() == -1) {
	working(2, 0, 0);
	return Arch;
    }

    records = CountArchive();
    if (records == -1) {
	working(2, 0, 0);
	return Arch;
    }

    if (records != 0) {
	snprintf(temp, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(temp, "r")) != NULL) {
	    fread(&archiverhdr, sizeof(archiverhdr), 1, fil);

	    for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.  ARCHIVER SELECT", shdr);
		mbse_mvprintw( 5, 4, temp);
		set_color(CYAN, BLACK);
		y = 7;
		for (i = 1; i <= 10; i++) {
		    if ((o + i) <= records) {
			offset = sizeof(archiverhdr) + (((o + i) - 1) * archiverhdr.recsize);
			fseek(fil, offset, 0);
			fread(&archiver, archiverhdr.recsize, 1, fil);
			if (mailmode && archiver.available && strlen(archiver.marc))
			    set_color(CYAN, BLACK);
			else if (! mailmode && archiver.available && strlen(archiver.farc))
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			snprintf(temp, 81, "%3d.  %-5s  %-32s", o+i, archiver.name, archiver.comment);
			mbse_mvprintw(y, 2, temp);
			y++;
		    }
		}
		strcpy(pick, select_pick(records, 10));

		if (strncmp(pick, "N", 1) == 0)
		    if ((o + 10) < records)
			o = o + 10;

		if (strncmp(pick, "P", 1) == 0)
		    if ((o - 10) >= 0)
			 o = o - 10;
    
		if (strncmp(pick, "-", 1) == 0) {
		    break;
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
		    offset = sizeof(archiverhdr) + ((atoi(pick) - 1) * archiverhdr.recsize);
		    fseek(fil, offset, 0);
		    fread(&archiver, archiverhdr.recsize, 1, fil);
		    if ((mailmode && archiver.available && strlen(archiver.marc)) ||
			(! mailmode && archiver.available && strlen(archiver.farc))) {
			strcpy(Arch, archiver.name);
			break;
		    } else {
			working(2, 0, 0);
		    }
		}
	    }
	    fclose(fil);
	}
    }
    return Arch;
}


int archive_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *arch, *wp, *ip;
    int	    i, j;

    snprintf(temp, PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
    if ((arch = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 3, 0, page, (char *)"Archiver programs");
    i = j = 0;

    ip = open_webdoc((char *)"archivers.html", (char *)"Archivers", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Name</TH><TH align='left'>Comment</TH><TH align='left'>Available</TH></TR>\n");

    fprintf(fp, "\n\n");
    fread(&archiverhdr, sizeof(archiverhdr), 1, arch);
    while ((fread(&archiver, archiverhdr.recsize, 1, arch)) == 1) {

	if (j == 4) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	i++;
		
	snprintf(temp, 81, "archiver_%d.html", i);

	if ((wp = open_webdoc(temp, (char *)"Archiver", archiver.comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"archivers.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Short name", archiver.name);
	    add_webtable(wp, (char *)"Available", getboolean(archiver.available));
	    add_webtable(wp, (char *)"Pack files", archiver.farc);
	    add_webtable(wp, (char *)"Pack mail", archiver.marc);
	    add_webtable(wp, (char *)"Pack banners", archiver.barc);
	    add_webtable(wp, (char *)"Test archive", archiver.tarc);
	    add_webtable(wp, (char *)"Unpack files", archiver.funarc);
	    add_webtable(wp, (char *)"Unpack mail", archiver.munarc);
	    add_webtable(wp, (char *)"Get FILE_ID.DIZ", archiver.iunarc);
	    add_webtable(wp, (char *)"List archive", archiver.varc);
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}
	fprintf(ip, "<TR><TD><A HREF=\"%s\">%s</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, archiver.name, archiver.comment, getboolean(archiver.available));

	fprintf(fp, "     Comment         %s\n", archiver.comment);
	fprintf(fp, "     Short name      %s\n", archiver.name);
	fprintf(fp, "     Available       %s\n", getboolean(archiver.available));
	fprintf(fp, "     Pack files      %s\n", archiver.farc);
	fprintf(fp, "     Pack mail       %s\n", archiver.marc);
	fprintf(fp, "     Pack banners    %s\n", archiver.barc);
	fprintf(fp, "     Test archive    %s\n", archiver.tarc);
	fprintf(fp, "     Unpack files    %s\n", archiver.funarc);
	fprintf(fp, "     Unpack mail     %s\n", archiver.munarc);
	fprintf(fp, "     Get FILE_ID.DIZ %s\n", archiver.iunarc);
	fprintf(fp, "     List archive    %s\n", archiver.varc);
	fprintf(fp, "\n\n");
	j++;
    }

    fclose(arch);
    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
    return page;
}



