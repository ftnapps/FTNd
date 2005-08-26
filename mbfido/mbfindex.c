/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Build index for request processor
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "../lib/diesel.h"
#include "mbfutil.h"
#include "mbfindex.h"



extern int	do_quiet;		/* Suppress screen output   */
int		lastfile;		/* Last file number	    */
long		gfilepos = 0;		/* Global file position	    */
int		TotalHtml = 0;		/* Total html files	    */
int		AreasHtml = 0;		/* Total html areas	    */



typedef struct _Index {
	struct _Index		*next;
	struct FILEIndex	idx;
} Findex;


static char *wdays[]=  {(char *)"Sun",(char *)"Mon",(char *)"Tue",
			(char *)"Wed",(char *)"Thu",(char *)"Fri",
			(char *)"Sat"};
static char *months[]= {(char *)"Jan",(char *)"Feb",(char *)"Mar",
			(char *)"Apr",(char *)"May",(char *)"Jun",
			(char *)"Jul",(char *)"Aug",(char *)"Sep",
			(char *)"Oct",(char *)"Nov",(char *)"Dec"};



/*
 * Translate a string from ANSI to safe HTML characters
 */
char *To_Html(char *);
char *To_Html(char *inp)
{
    static char	temp[256];
    int		i;
    char	*xl;

    memset(&temp, 0, sizeof(temp));

    for (i = 0; i < strlen(inp); i++) {
	if (inp[i] & 0x80) {
	    if ((xl = charset_map_c(inp[i], FALSE))) {
		while (*xl) {
		    temp[i] = *xl++;
		    if (*xl)
			i++;
		}
	    }
	} else
	    temp[i] = inp[i];
    }

    return temp;
}



void tidy_index(Findex **);
void tidy_index(Findex **fap)
{
	Findex	*tmp, *old;

	for (tmp = *fap; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fap = NULL;
}



void fill_index(struct FILEIndex, Findex **);
void fill_index(struct FILEIndex idx, Findex **fap)
{
	Findex	*tmp;

	tmp = (Findex *)malloc(sizeof(Findex));
	tmp->next = *fap;
	tmp->idx  = idx;
	*fap = tmp;
}


int comp_index(Findex **, Findex **);

void sort_index(Findex **);
void sort_index(Findex **fap)
{
	Findex	*ta, **vector;
	size_t	n = 0, i;

	if (*fap == NULL)
		return;

	for (ta = *fap; ta; ta = ta->next)
		n++;

	vector = (Findex **)malloc(n * sizeof(Findex *));

	i = 0;
	for (ta = *fap; ta; ta = ta->next)
		vector[i++] = ta;

	qsort(vector, n, sizeof(Findex *), 
		(int(*)(const void*, const void *))comp_index);

	(*fap) = vector[0];
	i = 1;

	for (ta = *fap; ta; ta = ta->next) {
		if (i < n)
			ta->next = vector[i++];
		else
			ta->next = NULL;
	}

	free(vector);
	return;
}



int comp_index(Findex **fap1, Findex **fap2)
{
	return strcasecmp((*fap1)->idx.LName, (*fap2)->idx.LName);
}



void MacroRead(FILE *, FILE *);
void MacroRead(FILE *fi, FILE *fp)
{
    char    *line, *temp;
    int     res, i;

    line = calloc(MAXSTR, sizeof(char));
    temp = calloc(MAXSTR, sizeof(char));

    while ((fgets(line, MAXSTR-2, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {
        /*
         * Skip comment lines
         */
        if (line[0] != '#') {
            Striplf(line);
            if (strlen(line) == 0) {
                /*
                 * Empty lines are just written
                 */
		fputc('\n', fp);
            } else {
                strncpy(temp, ParseMacro(line,&res), MAXSTR-1);
                if (res)
                    Syslog('!', "Macro error line: \"%s\"", line);
                /*
                 * Only output if something was evaluated
                 */
                if (strlen(temp)) {
		    for (i = 0; i < strlen(temp); i++)
			fputc(temp[i], fp);
		    fputc('\n', fp);
		}
            }
        }
    }
    free(line);
    free(temp);
    gfilepos = ftell(fi);
}



char *rfcdate(time_t);
char *rfcdate(time_t now)
{
        static char      buf[40];
        struct tm        ptm;

        ptm = *gmtime(&now);

        sprintf(buf,"%s, %02d %s %04d %02d:%02d:%02d GMT",
                wdays[ptm.tm_wday], ptm.tm_mday, months[ptm.tm_mon],
                ptm.tm_year + 1900, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
        return(buf);
}



/*
 * Create the macro's for the navigation bar.
 */
void pagelink(FILE *, char *, int, int);
void pagelink(FILE *fa, char *Path, int inArea, int Current)
{
        char    temp[256], nr[25];

        if ((Current >= CFG.www_files_page) && (inArea >= CFG.www_files_page)) {
                if (((Current / CFG.www_files_page) - 1) > 0) {
                    sprintf(nr, "%d", (Current / CFG.www_files_page) -1);
		} else {
                    nr[0] = '\0';
		}
		sprintf(temp, "%s/%s%s/index%s.html", CFG.www_url, CFG.www_link2ftp, Path+strlen(CFG.ftp_base), nr);
		MacroVars("c", "s", temp);
        } else {
	    MacroVars("c", "s", "");
	}

        if ((Current < (inArea - CFG.www_files_page)) && (inArea >= CFG.www_files_page)) {
	    sprintf(temp, "%s/%s%s/index%d.html", CFG.www_url, CFG.www_link2ftp, Path+strlen(CFG.ftp_base), 
		    (Current / CFG.www_files_page) + 1);
	    MacroVars("d", "s", temp);
        } else {
	    MacroVars("d", "s", "");
	}
}



/*
 * Start a new file areas page
 */
FILE *newpage(char *, char *, time_t, int, int, FILE *);
FILE *newpage(char *Path, char *Name, time_t later, int inArea, int Current, FILE *fi)
{
        char            linebuf[1024], outbuf[1024];
        static FILE*    fa;

	Syslog('f', "newpage %s %s %d %d", Path, Name, inArea, Current);
        lastfile = Current;
        if (Current)
                sprintf(linebuf, "%s/index%d.temp", Path, Current / CFG.www_files_page);
        else
                sprintf(linebuf, "%s/index.temp", Path);
        if ((fa = fopen(linebuf, "w")) == NULL) {
                WriteError("$Can't create %s", linebuf);
        } else {
                sprintf(linebuf, "%s", Name);
                html_massage(linebuf, outbuf);
		MacroVars("ab", "ss", rfcdate(later), outbuf);
                pagelink(fa, Path, inArea, Current);
		MacroRead(fi, fa);
                return fa;
        }
        return NULL;
}



/*
 * Finish a files area page
 */
void closepage(FILE *, char *, int, int, FILE *);
void closepage(FILE *fa, char *Path, int inArea, int Current, FILE *fi)
{
        char    *temp1, *temp2;

	Syslog('f', "closepage %s %d %d", Path, inArea, Current);

        if (fa == NULL)
                return;

        temp1 = calloc(PATH_MAX, sizeof(char));
        temp2 = calloc(PATH_MAX, sizeof(char));
	MacroRead(fi, fa);
        fclose(fa);
        if (lastfile) {
                sprintf(temp1, "%s/index%d.html", Path, lastfile / CFG.www_files_page);
                sprintf(temp2, "%s/index%d.temp", Path, lastfile / CFG.www_files_page);
        } else {
                sprintf(temp1, "%s/index.html", Path);
                sprintf(temp2, "%s/index.temp", Path);
        }
        rename(temp2, temp1);
	chmod(temp1, 0644);
        free(temp1);
        free(temp2);
        fa = NULL;
}



/*
 * Build a sorted index for the file request processor.
 */
void ReqIndex(void);
void ReqIndex(void)
{
    FILE                *pAreas, *pIndex, *fp;
    unsigned long       i, iAreas, iAreasNew = 0, record;
    int                 iTotal = 0, j, z, x = 0;
    int                 fbAreas = 0, fbFiles = 0;
    char                *sAreas, *newdir = NULL, *sIndex, *temp;
    Findex              *fdx = NULL;
    Findex              *tmp;
    struct FILEIndex    idx;
    struct _fdbarea	*fdb_area = NULL;
    time_t		db_time, obj_time;

    IsDoing("Index files");
    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("Create index files...\n");
    }

    sAreas = calloc(PATH_MAX, sizeof(char));
    sIndex = calloc(PATH_MAX, sizeof(char));
    temp   = calloc(PATH_MAX, sizeof(char));

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	die(MBERR_INIT_ERROR);
    }

    sprintf(sIndex, "%s/etc/request.index", getenv("MBSE_ROOT"));
    if ((pIndex = fopen(sIndex, "w")) == NULL) {
	WriteError("$Can't create %s", sIndex);
	die(MBERR_GENERAL);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    for (i = 1; i <= iAreas; i++) {
	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    if (enoughspace(CFG.freespace) == 0)
		die(MBERR_DISK_FULL);

	    if (!do_quiet) {
		printf("\r%4ld => %-44s    \b\b\b\b", i, area.Name);
		fflush(stdout);
	    }

	    /*
	     * Check if download directory exists, 
	     * if not, create the directory.
	     */
	    if (access(area.Path, W_OK) == -1) {
		Syslog('!', "Create dir: %s", area.Path);
		newdir = xstrcpy(area.Path);
		newdir = xstrcat(newdir, (char *)"/");
		mkdirs(newdir, 0755);
		free(newdir);
		newdir = NULL;
	    }

	    if ((fdb_area = mbsedb_OpenFDB(i, 30)) == NULL)
		die(MBERR_GENERAL);
	    sprintf(temp, "%s/var/fdb/file%ld.data", getenv("MBSE_ROOT"), i);
	    db_time = (int) file_time(temp);

	    /*
	     * Create file request index if requests are allowed in this area.
	     */
	    if (area.FileReq) {
		/*
		 * Now start creating the unsorted index.
		 */
		record = 0;
		while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		    iTotal++;
		    if ((iTotal % 10) == 0)
			Marker();
		    memset(&idx, 0, sizeof(idx));
		    sprintf(idx.Name, "%s", tu(fdb.Name));
		    sprintf(idx.LName, "%s", tu(fdb.LName));
		    idx.AreaNum = i;
		    idx.Record = record;
		    fill_index(idx, &fdx);
		    record++;
		}
		iAreasNew++;
	    } /* if area.Filereq */

	    /*
	     * Create files.bbs if needed.
	     */
	    sprintf(temp, "%s/files.bbs", area.Path);
	    obj_time = (int) file_time(temp);

	    if (obj_time < db_time) {
		/*
		 * File is outdated, recreate
		 */
		if ((fp = fopen(temp, "w")) == NULL) {
		    WriteError("$Can't create %s", temp);
		} else {
		    fbAreas++;
		    fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			if (!fdb.Deleted) {
			    fbFiles++;
			    fprintf(fp, "%-12s [%ld] %s\r\n", fdb.Name, fdb.TimesDL, fdb.Desc[0]);
			    for (j = 1; j < 25; j++)
				if (strlen(fdb.Desc[j]))
				    fprintf(fp, " +%s\r\n", fdb.Desc[j]);
			}
		    }
		    fclose(fp);
		    chmod(temp, 0644);
		}
	    }

	    /*
	     * Create 00index file if needed.
	     */
	    if (strncmp(CFG.ftp_base, area.Path, strlen(CFG.ftp_base)) == 0) {

		sprintf(temp, "%s/00index", area.Path);
		obj_time = (int) file_time(temp);

		if (obj_time < db_time) {
		    /*
		     * File is outdated, recreate
		     */
		    if ((fp = fopen(temp, "w")) == NULL) {
			WriteError("$Can't create %s", temp);
		    } else {
			fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
			while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			    if (!fdb.Deleted) {
				/*
				 * The next is to reduce system load
				 */
				x++;
				if (CFG.slow_util && do_quiet && ((x % 3) == 0))
				    msleep(1);

				for (z = 0; z <= 25; z++) {
				    if (strlen(fdb.Desc[z])) {
					if (z == 0)
					    fprintf(fp, "%-12s %7luK %s ", fdb.Name, (long)(fdb.Size / 1024), 
					    	StrDateDMY(fdb.UploadDate));
					else
					    fprintf(fp, "                                 ");
					if ((fdb.Desc[z][0] == '@') && (fdb.Desc[z][1] == 'X'))
					    fprintf(fp, "%s\n", fdb.Desc[z]+4);
					else
					    fprintf(fp, "%s\n", fdb.Desc[z]);
				    }
				}
			    }
			}
			fclose(fp);
			chmod(temp, 0644);
		    }
		}

	    }
	    mbsedb_CloseFDB(fdb_area);
    	    }
    }

    fclose(pAreas);

    sort_index(&fdx);
    for (tmp = fdx; tmp; tmp = tmp->next)
	fwrite(&tmp->idx, sizeof(struct FILEIndex), 1, pIndex);
    fclose(pIndex);
    tidy_index(&fdx);

    Syslog('+', "FREQ. Areas [%5d] Files [%5d]", iAreasNew, iTotal);
    Syslog('+', "Index Areas [%5d] Files [%5d]", fbAreas, fbFiles);

    free(sAreas);
    free(sIndex);
    free(temp);
}



/*
 * Build html index pages for download.
 */
void HtmlIndex(char *);
void HtmlIndex(char *Lang)
{
    FILE		*pAreas, *fa, *fb = NULL, *fm, *fi = NULL;
    unsigned long	i, iAreas, KSize = 0L, aSize = 0;
    int			AreaNr = 0, j, k, x = 0, isthumb;
    int			aTotal = 0, inArea = 0, filenr;
    char		*sAreas, *fn, *temp;
    char		linebuf[1024], outbuf[1024], desc[6400], namebuf[1024];
    time_t		last = 0L, later, db_time, obj_time;
    long		fileptr = 0, fileptr1 = 0;
    struct _fdbarea	*fdb_area = NULL;
    
    sAreas = calloc(PATH_MAX, sizeof(char));
    fn     = calloc(PATH_MAX, sizeof(char));
    temp   = calloc(PATH_MAX, sizeof(char));

    AreasHtml = 0;
    TotalHtml = 0;
    later = time(NULL) + 86400;

    IsDoing("Create html");
    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("\rCreate html pages...                                      \n");
    }

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	die(MBERR_INIT_ERROR);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    /*
     * Check if we are able to create the index.html pages in the
     * download directories.
     */
    if (strlen(CFG.ftp_base) && strlen(CFG.www_url) && strlen(CFG.www_author) && strlen(CFG.www_charset)) {
	sprintf(fn, "%s/index.temp", CFG.ftp_base);
	if ((fm = fopen(fn, "w")) == NULL) {
	    Syslog('+', "Can't open %s, skipping html pages creation", fn);
	}
    } else {
	fm = NULL;
	Syslog('+', "FTP/HTML not defined, skipping html pages creation");
    }

    if (fm) {
	if ((fi = OpenMacro("html.main", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	    fclose(fm);
	    unlink(fn);
	}
    }

    if (fm) {
	/*
	 * Because these web pages are dynamic, ie. they change everytime you
	 * receive new files and recreates these pages, extra HTTP headers are
	 * send to the client about these pages. It forbids proxy servers to
	 * cache these pages. The pages will expire within 24 hours.  The pages 
	 * also have an author name, this is the bbs name, and a content 
	 * description for search engines. Automatic advertising.
	 */
	MacroVars("acd", "sdd", rfcdate(later), 0, 0);
	MacroRead(fi, fm);
	fileptr = ftell(fi);
    }

    /*
     * Setup the correct table to produce file listings for the www.
     */
    charset_set_in_out((char *)"x-ibmpc", (char *)"iso-8859-1");

    for (i = 1; i <= iAreas; i++) {

	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);
	AreaNr++;

	if (area.Available) {

	    if (enoughspace(CFG.freespace) == 0)
		die(MBERR_DISK_FULL);

	    if (!do_quiet) {
		printf("\r%4ld => %-44s    \b\b\b\b", i, area.Name);
		fflush(stdout);
	    }

	    if ((fdb_area = mbsedb_OpenFDB(i, 30)) == NULL)
		die(MBERR_GENERAL);
            sprintf(temp, "%s/var/fdb/file%ld.data", getenv("MBSE_ROOT"), i);
	    db_time = (int) file_time(temp);
	    sprintf(temp, "%s/index.html", area.Path);
	    obj_time = (int) file_time(temp);

	    Syslog('f', "Area %d index.html is %s", i, (obj_time < db_time) ? "outdated" : "up todate");
    
	    /*
	     * Create index.html pages in each available download area when not up to date.
	     */
	    if (fm && (obj_time < db_time) && (strncmp(CFG.ftp_base, area.Path, strlen(CFG.ftp_base)) == 0)) {

		fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		AreasHtml++;
		inArea = 0;
		while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		    if (!fdb.Deleted)
			inArea++;
		}
		fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		aSize = 0L;
		aTotal = 0;
		last = 0L;
		if ((fb = OpenMacro("html.areas", 'E', TRUE)) == NULL) {
		    fa = NULL;
		} else {
		    fa = newpage(area.Path, area.Name, later, inArea, aTotal, fb);
		    fileptr1 = gfilepos;
		}

		while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		    if (!fdb.Deleted) {
			/*
			 * The next is to reduce system load
			 */
			x++;
			TotalHtml++;
			aTotal++;
			if (CFG.slow_util && do_quiet && ((x % 3) == 0))
			    msleep(1);
			 
			MacroVars("efghijklm", "ddsssssds", 0, 0, "", "", "", "", "", 0, "");
			MacroVars("e", "d", aTotal);

			/*
			 * Check if this is a .gif or .jpg file, if so then
			 * check if a thumbnail file exists. If not try to
			 * create a thumbnail file to add to the html listing.
			 */
			isthumb = FALSE;
			if (strstr(fdb.LName, ".gif") || strstr(fdb.LName, ".jpg") ||
			    strstr(fdb.LName, ".GIF") || strstr(fdb.LName, ".JPG")) {
			    sprintf(linebuf, "%s/%s", area.Path, fdb.LName);
			    sprintf(outbuf, "%s/.%s", area.Path, fdb.LName);
			    if (file_exist(outbuf, R_OK)) {
				if (strlen(CFG.www_convert)) {
				    if ((execute_str(CFG.www_convert, linebuf, outbuf,
						    (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null"))) {
					Syslog('+', "Failed to create thumbnail for %s", fdb.LName);
				    } else {
					chmod(outbuf, 0644);
					isthumb = TRUE;
				    }
				} else {
				    Syslog('+', "No convert program to create thumbnail %s", outbuf);
				}
			    } else {
				isthumb = TRUE;
			    }
			}
			sprintf(outbuf, "%s/%s%s/%s", CFG.www_url, CFG.www_link2ftp,
				area.Path+strlen(CFG.ftp_base), fdb.LName);
			if (isthumb) {
			    sprintf(linebuf, "%s/%s%s/.%s", CFG.www_url, CFG.www_link2ftp,
					area.Path+strlen(CFG.ftp_base), fdb.LName);
			    MacroVars("fghi", "dsss", 1, outbuf, fdb.LName, linebuf);
			} else {
			    sprintf(outbuf, "%s/%s%s/%s", CFG.www_url, CFG.www_link2ftp,
					area.Path+strlen(CFG.ftp_base), fdb.LName);
			    MacroVars("fghi", "dsss", 0, outbuf, fdb.LName, "");
			}

			sprintf(outbuf, "%lu Kb.", (long)(fdb.Size / 1024));
			MacroVars("jkl", "ssd", StrDateDMY(fdb.FileDate), outbuf, fdb.TimesDL);
			memset(&desc, 0, sizeof(desc));
			k = 0;
			for (j = 0; j < 25; j++)
			    if (strlen(fdb.Desc[j])) {
				if (j) {
				    sprintf(desc+k, "\n");
				    k += 1;
				}
			        sprintf(linebuf, "%s", To_Html(fdb.Desc[j]));
				html_massage(linebuf, outbuf);
				sprintf(desc+k, "%s", outbuf);
				k += strlen(outbuf);
			    }
			MacroVars("m", "s", desc);
			fseek(fb, fileptr1, SEEK_SET);
			MacroRead(fb, fa);
			aSize += fdb.Size;
			MacroVars("efghijklm", "ddsssssds", 0, 0, "", "", "", "", "", 0, "");
			if (fdb.FileDate > last)
			    last = fdb.FileDate;
			if ((aTotal % CFG.www_files_page) == 0) {
			    closepage(fa, area.Path, inArea, aTotal, fb);
			    fseek(fb, 0, SEEK_SET);
			    fa = newpage(area.Path, area.Name, later, inArea, aTotal, fb);
			}
		    } /* if (!file.deleted) */
		}
		if (aTotal == 0) {
		    /*
		     * Nothing written, skip skip fileblock
		     */
		    while ((fgets(linebuf, 254, fb) != NULL) && ((linebuf[0]!='@') || (linebuf[1]!='|')));
		}

		KSize += aSize / 1024;
		closepage(fa, area.Path, inArea, aTotal, fb);
		fclose(fb);

		/*
		 * If the time before there were more files in this area then now,
		 * the number of html pages may de decreased. We try to delete these
		 * files if they should exist.
		 */
		filenr = lastfile / CFG.www_files_page;
		while (TRUE) {
		    filenr++;
		    sprintf(linebuf, "%s/index%d.html", area.Path, filenr);
		    if (unlink(linebuf))
			break;
		    Syslog('+', "Removed obsolete %s", linebuf);
		}

		strcpy(linebuf, area.Name);
		html_massage(linebuf, namebuf);
		sprintf(linebuf, "%s/%s%s/index.html", CFG.www_url, CFG.www_link2ftp, area.Path+strlen(CFG.ftp_base));
		if (aSize > 1048576)
		    sprintf(outbuf, "%ld Mb.", aSize / 1048576);
		else
		    sprintf(outbuf, "%ld Kb.", aSize / 1024);
		MacroVars("efghi", "dssds", AreaNr, linebuf, namebuf, aTotal, outbuf);
		if (last == 0L)
		    MacroVars("j", "s", "&nbsp;");
		else
		    MacroVars("j", "s", StrDateDMY(last));
		fseek(fi, fileptr, SEEK_SET);
		MacroRead(fi, fm);
	    }
	    mbsedb_CloseFDB(fdb_area);
	} /* if area.Available */
    }

    if (fm) {
	sprintf(linebuf, "%ld Mb.", KSize / 1024);
	MacroVars("cd", "ds", TotalHtml, linebuf);
	MacroRead(fi, fm);
	fclose(fi);
	MacroClear();
	fclose(fm);
	sprintf(linebuf, "%s/index.html", CFG.ftp_base);
	rename(fn, linebuf);
	chmod(linebuf, 0644);
    }

    fclose(pAreas);

    if (!do_quiet) {
	printf("\r                                                          \r");
	fflush(stdout);
    }

    free(temp);
    free(sAreas);
    free(fn);
    RemoveSema((char *)"reqindex");
}



void Index(void)
{
    ReqIndex();
    HtmlIndex(NULL);
    Syslog('+', "HTML  Areas [%5d] Files [%5d]", AreasHtml, TotalHtml);
}


