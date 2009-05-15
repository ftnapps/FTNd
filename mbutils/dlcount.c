/*****************************************************************************
 *
 * $Id: dlcount.c,v 1.7 2007/10/16 14:29:29 mbse Exp $
 * Purpose ...............: Count WWW and FTP downloads
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "dlcount.h"

static char *months[]={(char *)"Jan",(char *)"Feb",(char *)"Mar",
		       (char *)"Apr",(char *)"May",(char *)"Jun",
		       (char *)"Jul",(char *)"Aug",(char *)"Sep",
		       (char *)"Oct",(char *)"Nov",(char *)"Dec"};

void count_download(char *, time_t, off_t, char *);

extern int  do_quiet;


void dlcount(void)
{
    char	*temp, *p, *q = NULL, *date, *file, *base, month[20];
    FILE	*fp;
    int		i, date_ok, file_ok, result, filesize;
    time_t	filedate = (time_t)0, lastcheck;
    struct tm	tm;
    
    temp = calloc(PATH_MAX, sizeof(char));
    
    /*
     * Check if we have a mark when we did this the last time.
     * If not, create one and don't do anything. Run the next time.
     */
    snprintf(temp, PATH_MAX, "%s/var/dlcount.stat", getenv("MBSE_ROOT"));
    if ((lastcheck = file_time(temp)) == -1) {
	Syslog('+', "Checking WWW downloads never done before, creating timestamp");
	if ((fp = fopen(temp, "a"))) {
	    fclose(fp);
	}
	free(temp);
	return;
    }
    
    /*
     * Refresh timestamp
     */
    unlink(temp);
    if ((fp = fopen(temp, "a"))) {
	fclose(fp);
    }

    date = calloc(81, sizeof(char));
    file = calloc(PATH_MAX, sizeof(char));
    base = calloc(PATH_MAX, sizeof(char));

    if (strlen(CFG.www_logfile) && (fp = fopen(CFG.www_logfile, "r"))) {

	/*
	 * Check apache logfile
	 */
	if (!do_quiet)
	    printf("Checking WWW downloads\n");
	Syslog('+', "Checking WWW downloads");

	while (fgets(temp, PATH_MAX-1, fp)) {
	    date_ok = file_ok = FALSE;
	    memset(date, 0, 80);
	    memset(file, 0, PATH_MAX);
	    memset(base, 0, PATH_MAX);
	    Striplf(temp);

	    /*
	     * Parse logline, be aware for lots of garbage created by systems
	     * that try to compromise the webserver.
	     */
//	    Syslog('f', "%s", printable(temp, 100));
	    p = strchr(temp, '[');
	    if (p) {
		q = strchr(p, ']');
		if (q && ((q-p) < 40)) {
		    strncpy(date, p+1, q - p - 1);
		    tm.tm_mday = atoi(strtok(date, "/\0"));
		    snprintf(month, 20, "%s", strtok(NULL, "/\0"));
		    for (i = 0; i < 12; i++)
			if (strncasecmp(months[i], month, 3) == 0)
			    break;
		    tm.tm_mon = i;
		    tm.tm_year = atoi(strtok(NULL, ":\0")) - 1900;
		    tm.tm_hour = atoi(strtok(NULL, ":\0"));
		    tm.tm_min  = atoi(strtok(NULL, ":\0"));
		    tm.tm_sec  = atoi(strtok(NULL, ":\0"));
		    filedate = mktime(&tm);
		    if (filedate > lastcheck)
			date_ok = TRUE;
		}
	    }
	    if (date_ok && (p = strchr(temp, '"'))) {
		q = strchr(p+1, '"');
		if (q && ((q-p) < 128)) {
		    strncpy(file, p+1, q - p - 1);
		    if (strncmp(file, "GET ", 4) == 0) {
			if ((p = strstr(file, CFG.www_link2ftp))) {
			    snprintf(base, PATH_MAX, "%s%s", CFG.ftp_base, p + strlen(CFG.www_link2ftp));
			    for (i = strlen(base); i; i--) {
				if (base[i] == ' ') {
				    base[i] = '\0';
				    break;
				}
			    }
			    file_ok = TRUE;
			}
		    }
		}
	    }
	    if (file_ok) {
		p = strtok(q, (char *)" \0");
		result = atoi(strtok(NULL, (char *)" \0"));
		filesize = atoi(strtok(NULL, (char *)" \0"));
		if (result == 200) {
		    /*
		     * So far it seems that the file is possible downloaded from the bbs.
		     * Now search the download area and filerecord.
		     */
		    Syslog('f', "%s %s %d", rfcdate(filedate), base, filesize);
		    count_download(base, filedate, filesize, (char *)"WWW");
		}
	    }
	}
	fclose(fp);
    }

    if (strlen(CFG.ftp_logfile) && (fp = fopen(CFG.ftp_logfile, "r"))) {

	/*
	 * Check ftp logfile
	 */
	if (!do_quiet)
	    printf("Checking FTP downloads\n");
	Syslog('+', "Checking FTP downloads");

	while (fgets(temp, PATH_MAX-1, fp)) {
	    date_ok = file_ok = FALSE;
	    memset(date, 0, 80);
	    memset(file, 0, PATH_MAX);
	    memset(base, 0, PATH_MAX);
	    Striplf(temp);

	    /*
	     * Parse logline.
	     */
//	    Syslog('f', "%s", printable(temp, 100));
	    p = strtok(temp, " \0");	    /* Day of week	*/
	    p = strtok(NULL, " \0");	    /* Month		*/
	    for (i = 0; i < 12; i++)
		if (strncasecmp(months[i], p, 3) == 0)
		    break;
	    tm.tm_mon = i;
	    tm.tm_mday = atoi(strtok(NULL, " \0"));	    /* Day in month	*/
	    tm.tm_hour = atoi(strtok(NULL, ":\0"));	    /* Hour		*/
	    tm.tm_min  = atoi(strtok(NULL, ":\0"));	    /* Minute		*/
	    tm.tm_sec  = atoi(strtok(NULL, " \0"));	    /* Seconds		*/
	    tm.tm_year = atoi(strtok(NULL, " \0")) - 1900;  /* Year		*/
	    filedate = mktime(&tm);
	    if (filedate > lastcheck)
		date_ok = TRUE;
	    p = strtok(NULL, " \0");			    /* 0		*/
	    p = strtok(NULL, " \0");			    /* Remote host	*/
	    filesize = atoi(strtok(NULL, " \0"));	    /* Filesize		*/
	    p = strtok(NULL, " \0");			    /* Filename		*/
	    if (p == NULL)
		break;

	    snprintf(base, PATH_MAX, "%s", p);
	    if (date_ok) {
                /*
		 * So far it seems that the file is possible downloaded from the bbs.
		 * Now search the download area and filerecord.
		 */
		Syslog('f', "%s %s %d", rfcdate(filedate), base, filesize);
		count_download(base, filedate, filesize, (char *)"FTP");
	    }
	}
    }

    free(base);
    free(file);
    free(date);
    free(temp);
}



/*
 * Count download if file is present in the FDB.
 */
void count_download(char *filename, time_t filedate, off_t filesize, char *dltype)
{
    char		*temp;
    FILE		*dfp;
    int			i, j;
    struct _fdbarea     *fdb_area = NULL;
    struct FILE_record  frec;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if ((dfp = fopen(temp, "r"))) {

	fread(&areahdr, sizeof(areahdr), 1, dfp);
	snprintf(temp, PATH_MAX, "%s", filename);
	for (j = strlen(temp); j; j--)
	    if (temp[j] == '/') {
		temp[j] = '\0';
		break;
	    }
	i = 0;

	while (fread(&area, areahdr.recsize, 1, dfp) == 1) {
	    i++;
	    if (area.Available && (strcmp(temp, area.Path) == 0)) {
		snprintf(temp, PATH_MAX, "%s", basename(filename));

		if ((fdb_area = mbsedb_OpenFDB(i, 30))) {
		    while (fread(&frec, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			if (((strcasecmp(frec.Name, temp) == 0) || (strcasecmp(frec.LName, temp) == 0)) && 
				(frec.Size == filesize)) {
			    Syslog('+', "%s download %s from area %d", dltype, temp, i);
			    frec.LastDL = filedate;
			    frec.TimesDL++;
			    if (mbsedb_LockFDB(fdb_area, 30)) {
				fseek(fdb_area->fp, 0 - fdbhdr.recsize, SEEK_CUR);
				fwrite(&frec, fdbhdr.recsize, 1, fdb_area->fp);
				mbsedb_UnlockFDB(fdb_area);
			    }
			    break;
			}
		    }
		    mbsedb_CloseFDB(fdb_area);
		}
		break;
	    }
	}
	fclose(dfp);
    }

    free(temp);
}

