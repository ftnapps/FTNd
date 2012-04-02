/*****************************************************************************
 *
 * $Id: mbfimport.c,v 1.39 2008/02/17 17:50:14 mbse Exp $
 * Purpose: File Database Maintenance - Import files with files.bbs
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
#include "mbfutil.h"
#include "mbfimport.h"



extern int	do_quiet;		/* Suppress screen output	    */
extern int	do_annon;		/* Suppress announce files	    */
extern int	do_novir;		/* Suppress virus scanning	    */


void test_file(char *, char *, char *);


/*
 * Test filename in a directory case insensitive.
 */
void test_file(char *dirpath, char *search, char *result)
{
    DIR		    *dp;
    struct dirent   *de;

    result[0] = '\0';

    Syslog('f', "test_file(%s, %s)", dirpath, search);

    if ((dp = opendir(dirpath)) == NULL) {
	WriteError("$Can't open directory %s", dirpath);
	if (!do_quiet)
	    printf("\nCan't open directory %s: %s\n", dirpath, strerror(errno));
	die(MBERR_INIT_ERROR);
    }
	
    while ((de = readdir(dp))) {
	if (strcasecmp(de->d_name, search) == 0) {
	    /*
	     * Found the right file.
	     */
	    strncpy(result, de->d_name, 80);
	    break;
	}
    }
    closedir(dp);
}



/*
 * Return 1 if imported, 0 if error.
 */
int flush_file(char *source, char *dest, char *lname, struct FILE_record f_db, int Area)
{
    int	    Doit = TRUE, rc = 0;

    Syslog('f', "flush_file(%s, %s, %s, %d)", source, dest, lname, Area);

    if (do_novir == FALSE) {
	if (!do_quiet) {
	    printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}
	if (VirScanFile(source)) {
	    Doit = FALSE;
	}
    }
    
    if (Doit) {
	if (!do_quiet) {
	    printf("Adding    \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}
	if (strcmp(f_db.Name, f_db.LName)) {
	    if (AddFile(f_db, Area, dest, source, lname)) {
		rc = 1;
	    }
	} else {
	    if (AddFile(f_db, Area, dest, source, NULL)) {
		rc = 1;
	    }
	}
    }

    return rc;
}



void ImportFiles(int Area)
{
    char		*pwd, *temp, *fod, *temp2, *tmpdir, *String, *token, *dest, *lname;
    FILE		*fbbs;
    int			Append = FALSE, Files = 0, i, line = 0, pos, x, y, Doit;
    int			Imported = 0, Errors = 0, Present = FALSE;
    struct FILE_record  f_db;
    struct stat		statfile;

    if (!do_quiet)
	mbse_colour(CYAN, BLACK);

    temp   = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "xxxxx%d", getpid());
    if ((fbbs = fopen(temp, "a+")) == NULL) {
	WriteError("$Can't write to directory");
	if (!do_quiet)
	    printf("\nCan't write to this directory, cannot import\n");
	free(temp);
	die(MBERR_INIT_ERROR);
    }
    fclose(fbbs);
    unlink(temp);
    free(temp);

    if (LoadAreaRec(Area) == FALSE)
	die(MBERR_INIT_ERROR);

    if (area.Available) {
        temp   = calloc(PATH_MAX, sizeof(char));
	temp2  = calloc(PATH_MAX, sizeof(char));
        pwd    = calloc(PATH_MAX, sizeof(char));
	tmpdir = calloc(PATH_MAX, sizeof(char));
	String = calloc(4096, sizeof(char));
	dest   = calloc(PATH_MAX, sizeof(char));
	lname  = calloc(PATH_MAX, sizeof(char));
	fod    = calloc(PATH_MAX, sizeof(char));

        getcwd(pwd, PATH_MAX);
	if (CheckFDB(Area, area.Path))
	    die(MBERR_GENERAL);

	snprintf(tmpdir, PATH_MAX, "%s/tmp/arc%d", getenv("MBSE_ROOT"), (int)getpid());
	if (create_tmpwork()) {
	    WriteError("Can't create %s", tmpdir);
	    if (!do_quiet)
		printf("\nCan't create %s\n", tmpdir);
	    die(MBERR_GENERAL);
	}
	IsDoing("Import files");

	/*
	 * Find and open files.bbs
	 */
	snprintf(temp, PATH_MAX, "FILES.BBS");
	if (getfilecase(pwd, temp) == FALSE) {
	    WriteError("Can't find files.bbs anywhere");
	    if (!do_quiet)
		printf("Can't find files.bbs anywhere\n");
	    die(MBERR_INIT_ERROR);
	}
	if ((fbbs = fopen(temp, "r")) == NULL) {
	    WriteError("Can't open files.bbs");
	    if (!do_quiet)
		printf("Can't open files.bbs\n");
	    die(MBERR_INIT_ERROR);
	}
	
	/*
	 * Process files.bbs
	 */
	while (fgets(String, 4095, fbbs) != NULL) {

	    /*
	     *  Strip cr and lf characters
	     */
	    for (i = 0; i < strlen(String); i++) {
		if (*(String + i) == '\0')
		    break;
		if (*(String + i) == '\n')
		    *(String + i) = '\0';
		if (*(String + i) == '\r')
		    *(String + i) = '\0';
	    }

	    /*
	     * Skip empty lines.
	     */
	    if (strlen(String) == 0)
		continue;

	    if ((String[0] != ' ') && (String[0] != '\t')) {
		/*
		 * New file entry, check if there has been a file that is not yet saved.
		 */
		if (Append && Present) {
		    if (flush_file(temp, dest, lname, f_db, Area))
			Imported++;
		    else
			Errors++;
		    Append = FALSE;
		    Present = FALSE;
		    line = 0;
		    pos = 0;
		}

		/*
		 * Check diskspace
		 */
		if (enoughspace(CFG.freespace) == 0)
		    die(MBERR_DISK_FULL);

		/*
		 * Refresh tmpwork
		 */
		clean_tmpwork();
		create_tmpwork();

		Files++;
		memset(&f_db, 0, sizeof(f_db));
		Present = TRUE;

		token = strtok(String, " \t\r\n\0");
		test_file(pwd, token, temp2);

		if (strlen(temp2) == 0) {
		    WriteError("Can't find file on disk, skipping: %s", token);
		    if (!do_quiet)
			printf("\nCan't find file on disk, skipping: %s\n", token);
		    Append = FALSE;
		    Present = FALSE;
		    Errors++;
		    continue;
		} else {
		    /*
		     * Check type of filename and set the right values.
		     */
		    strcpy(fod, temp2);
		    if (is_real_8_3(temp2)) {
			Syslog('f', "%s is 8.3", temp2);
			strcpy(f_db.Name, temp2);
			tl(temp2);
			strcpy(f_db.LName, temp2);
		    } else {
			Syslog('f', "%s is LFN", temp2);
			strcpy(f_db.LName, temp2);
			name_mangle(temp2);
			strcpy(f_db.Name, temp2);
		    }

		    snprintf(temp, PATH_MAX, "%s/%s", pwd, fod);
		    stat(temp, &statfile);

		    if (do_annon)
			f_db.Announced = TRUE;
		    Syslog('f', "File: %s (%s)", f_db.Name, f_db.LName);

		    if (!do_quiet) {
			printf("\rImport file: %s ", f_db.Name);
			printf("Checking  \b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }

		    IsDoing("Import %s", f_db.Name);

		    token = strtok(NULL, "\0");
		    if (token) {
			i = strlen(token);
			line = pos = 0;
			for (x = 0; x < i; x++) {
			    if ((token[x] == '\n') || (token[x] == '\r'))
				token[x] = '\0';
			}
			i = strlen(token);
			y = 0;

			if (token[0] == '[') {
			    /*
			     * Skip over download counter
			     */
			    while (token[y] != ']')
				y++;
			    y += 2;
			}

			Doit = FALSE;
			for (x = y; x < i; x++) {
			    if (!Doit) {
				if (!iscntrl(token[x]) && !isblank(token[x]))
				    Doit = TRUE;
			    }
			    if (Doit) {
				if (pos > 42) {
				    if (token[x] == ' ') {
					f_db.Desc[line][pos] = '\0';
					line++;
					pos = 0;
				    } else {
					if (pos == 48) {
					    f_db.Desc[line][pos] = '\0';
					    pos = 0;
					    line++;
					}
					f_db.Desc[line][pos] = token[x];
					pos++;
				    }
				} else {
				    f_db.Desc[line][pos] = token[x];
				    pos++;
				}
				if (line == 24)
				    break;
			    }
			}
		    } else {
			/*
			 * No file description
			 */
			Syslog('+', "No file description in files.bbs for %s", f_db.LName);
			strcpy(f_db.Desc[0], "No description");
		    }

		    snprintf(dest, PATH_MAX, "%s/%s", area.Path, f_db.Name);
		    snprintf(lname, PATH_MAX, "%s/%s", area.Path, f_db.LName);
		    Append = TRUE;
		    f_db.Size = statfile.st_size;
		    f_db.FileDate = statfile.st_mtime;
		    f_db.Crc32 = file_crc(temp, FALSE);
		    strcpy(f_db.Uploader, CFG.sysop_name);
		    f_db.UploadDate = time(NULL);
		}
	    } else if (Present) {
		/*
		 * Add multiple description lines
		 */
		if (line < 25) {
		    token = strtok(String, "\0");
		    i = strlen(token);
		    line++;
		    pos = 0;
		    Doit = FALSE;
		    for (x = 0; x < i; x++) {
			if ((token[x] == '\n') || (token[x] == '\r'))
			    token[x] = '\0';
		    }
		    for (x = 0; x < i; x++) {
			if (Doit) {
			    if (pos > 42) {
				if (token[x] == ' ') {
				    f_db.Desc[line][pos] = '\0';
				    line++;
				    pos = 0;
				} else {
				    if (pos == 48) {
					f_db.Desc[line][pos] = '\0';
					pos = 0;
					line++;
				    }
				    f_db.Desc[line][pos] = token[x];
				    pos++;
				}
			    } else {
				f_db.Desc[line][pos] = token[x];
				pos++;
			    }
			    if (line == 25)
				break;
			} else {
			    /*
			     * Skip until + or | is found
			     */
			    if ((token[x] == '+') || (token[x] == '|'))
				Doit = TRUE;
			}
		    }
		}
	    } /* End if new file entry found */
	} /* End of files.bbs */
	fclose(fbbs);

	/*
	 * Flush the last file to the database
	 */
	if (Append) {
	    if (flush_file(temp, dest, lname, f_db, Area))
		Imported++;
	    else
		Errors++;
	}

	clean_tmpwork();
	free(fod);
	free(lname);
	free(dest);
	free(String);
	free(pwd);
	free(temp2);
	free(temp);
	free(tmpdir);
    } else {
	if (!area.Available) {
	    WriteError("Area not available");
	    if (!do_quiet)
		printf("Area not available\n");
	}
    }

    if (!do_quiet) {
        printf("\r                                                              \r");
        fflush(stdout);
    }
    Syslog('+', "Import Files [%5d] Imported [%5d] Errors [%5d]", Files, Imported, Errors);
}

