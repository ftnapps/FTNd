/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Build index for request processor
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
#include "mbfindex.h"



extern int	do_quiet;		/* Supress screen output    */
int		lastfile;		/* Last file number	    */


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



/*
 * Translate ISO 8859-1 characters to named character entities
 */
void html_massage(char *, char *);
void html_massage(char *inbuf, char *outbuf)
{
        char    *inptr = inbuf;
        char    *outptr = outbuf;

        memset(outbuf, 0, sizeof(outbuf));

        while (*inptr) {

                switch ((unsigned char)*inptr) {
                        case '"':       sprintf(outptr, "&quot;");      break;
                        case '&':       sprintf(outptr, "&amp;");       break;
                        case '<':       sprintf(outptr, "&lt;");        break;
                        case '>':       sprintf(outptr, "&gt;");        break;
                        case 160:       sprintf(outptr, "&nbsp;");      break;
                        case 161:       sprintf(outptr, "&iexcl;");     break;
                        case 162:       sprintf(outptr, "&cent;");      break;
                        case 163:       sprintf(outptr, "&pound;");     break;
                        case 164:       sprintf(outptr, "&curren;");    break;
                        case 165:       sprintf(outptr, "&yen;");       break;
                        case 166:       sprintf(outptr, "&brvbar;");    break;
                        case 167:       sprintf(outptr, "&sect;");      break;
                        case 168:       sprintf(outptr, "&uml;");       break;
                        case 169:       sprintf(outptr, "&copy;");      break;
                        case 170:       sprintf(outptr, "&ordf;");      break;
                        case 171:       sprintf(outptr, "&laquo;");     break;
                        case 172:       sprintf(outptr, "&not;");       break;
                        case 173:       sprintf(outptr, "&shy;");       break;
                        case 174:       sprintf(outptr, "&reg;");       break;
                        case 175:       sprintf(outptr, "&macr;");      break;
                        case 176:       sprintf(outptr, "&deg;");       break;
                        case 177:       sprintf(outptr, "&plumn;");     break;
                        case 178:       sprintf(outptr, "&sup2;");      break;
                        case 179:       sprintf(outptr, "&sup3;");      break;
                        case 180:       sprintf(outptr, "&acute;");     break;
                        case 181:       sprintf(outptr, "&micro;");     break;
                        case 182:       sprintf(outptr, "&para;");      break;
                        case 183:       sprintf(outptr, "&middot;");    break;
                        case 184:       sprintf(outptr, "&cedil;");     break;
                        case 185:       sprintf(outptr, "&supl;");      break;
                        case 186:       sprintf(outptr, "&ordm;");      break;
                        case 187:       sprintf(outptr, "&raquo;");     break;
                        case 188:       sprintf(outptr, "&frac14;");    break;
                        case 189:       sprintf(outptr, "&frac12;");    break;
                        case 190:       sprintf(outptr, "&frac34;");    break;
                        case 191:       sprintf(outptr, "&iquest;");    break;
                        case 192:       sprintf(outptr, "&Agrave;");    break;
                        case 193:       sprintf(outptr, "&Aacute;");    break;
                        case 194:       sprintf(outptr, "&Acirc;");     break;
                        case 195:       sprintf(outptr, "&Atilde;");    break;
                        case 196:       sprintf(outptr, "&Auml;");      break;
                        case 197:       sprintf(outptr, "&Aring;");     break;
                        case 198:       sprintf(outptr, "&AElig;");     break;
                        case 199:       sprintf(outptr, "&Ccedil;");    break;
                        case 200:       sprintf(outptr, "&Egrave;");    break;
                        case 201:       sprintf(outptr, "&Eacute;");    break;
                        case 202:       sprintf(outptr, "&Ecirc;");     break;
                        case 203:       sprintf(outptr, "&Euml;");      break;
                        case 204:       sprintf(outptr, "&Igrave;");    break;
                        case 205:       sprintf(outptr, "&Iacute;");    break;
                        case 206:       sprintf(outptr, "&Icirc;");     break;
                        case 207:       sprintf(outptr, "&Iuml;");      break;
                        case 208:       sprintf(outptr, "&ETH;");       break;
                        case 209:       sprintf(outptr, "&Ntilde;");    break;
                        case 210:       sprintf(outptr, "&Ograve;");    break;
                        case 211:       sprintf(outptr, "&Oacute;");    break;
                        case 212:       sprintf(outptr, "&Ocirc;");     break;
                        case 213:       sprintf(outptr, "&Otilde;");    break;
                        case 214:       sprintf(outptr, "&Ouml;");      break;
                        case 215:       sprintf(outptr, "&times;");     break;
                        case 216:       sprintf(outptr, "&Oslash;");    break;
                        case 217:       sprintf(outptr, "&Ugrave;");    break;
                        case 218:       sprintf(outptr, "&Uacute;");    break;
                        case 219:       sprintf(outptr, "&Ucirc;");     break;
                        case 220:       sprintf(outptr, "&Uuml;");      break;
                        case 221:       sprintf(outptr, "&Yacute;");    break;
                        case 222:       sprintf(outptr, "&THORN;");     break;
                        case 223:       sprintf(outptr, "&szlig;");     break;
                        case 224:       sprintf(outptr, "&agrave;");    break;
                        case 225:       sprintf(outptr, "&aacute;");    break;
                        case 226:       sprintf(outptr, "&acirc;");     break;
                        case 227:       sprintf(outptr, "&atilde;");    break;
                        case 228:       sprintf(outptr, "&auml;");      break;
                        case 229:       sprintf(outptr, "&aring;");     break;
                        case 230:       sprintf(outptr, "&aelig;");     break;
                        case 231:       sprintf(outptr, "&ccedil;");    break;
                        case 232:       sprintf(outptr, "&egrave;");    break;
                        case 233:       sprintf(outptr, "&eacute;");    break;
                        case 234:       sprintf(outptr, "&ecirc;");     break;
                        case 235:       sprintf(outptr, "&euml;");      break;
                        case 236:       sprintf(outptr, "&igrave;");    break;
                        case 237:       sprintf(outptr, "&iacute;");    break;
                        case 238:       sprintf(outptr, "&icirc;");     break;
                        case 239:       sprintf(outptr, "&iuml;");      break;
                        case 240:       sprintf(outptr, "&eth;");       break;
                        case 241:       sprintf(outptr, "&ntilde;");    break;
                        case 242:       sprintf(outptr, "&ograve;");    break;
                        case 243:       sprintf(outptr, "&oacute;");    break;
                        case 244:       sprintf(outptr, "&ocirc;");     break;
                        case 245:       sprintf(outptr, "&otilde;");    break;
                        case 246:       sprintf(outptr, "&ouml;");      break;
                        case 247:       sprintf(outptr, "&divide;");    break;
                        case 248:       sprintf(outptr, "&oslash;");    break;
                        case 249:       sprintf(outptr, "&ugrave;");    break;
                        case 250:       sprintf(outptr, "&uacute;");    break;
                        case 251:       sprintf(outptr, "&ucirc;");     break;
                        case 252:       sprintf(outptr, "&uuml;");      break;
                        case 253:       sprintf(outptr, "&yacute;");    break;
                        case 254:       sprintf(outptr, "&thorn;");     break;
                        case 255:       sprintf(outptr, "&yuml;");      break;
                        default:        *outptr++ = *inptr; *outptr = '\0';     break;
                }
                while (*outptr)
                        outptr++;

                inptr++;
        }
        *outptr = '\0';
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



void pagelink(FILE *, char *, int, int);
void pagelink(FILE *fa, char *Path, int inArea, int Current)
{
        char    nr[20];

        fprintf(fa, "<DIV align=center>\n");

        if ((Current >= CFG.www_files_page) && (inArea >= CFG.www_files_page)) {
                if (((Current / CFG.www_files_page) - 1) > 0)
                        sprintf(nr, "%d", (Current / CFG.www_files_page) -1);
                else
                        nr[0] = '\0';
                fprintf(fa, "<A HREF=\"%s/%s%s/index%s.html\"><IMG SRC=\"/icons/%s\" ALT=\"%s\" BORDER=0>%s</A>&nbsp;\n", 
                        CFG.www_url, CFG.www_link2ftp, Path+strlen(CFG.ftp_base), nr, 
                        CFG.www_icon_prev, CFG.www_name_prev, CFG.www_name_prev);
        }

        fprintf(fa, "<A HREF=\"%s/index.html\"><IMG SRC=\"/icons/%s\" ALT=\"%s\" BORDER=0>%s</A>&nbsp;\n",
                        CFG.www_url, CFG.www_icon_home, CFG.www_name_home, CFG.www_name_home);
        fprintf(fa, "<A HREF=\"%s/%s/index.html\"><IMG SRC=\"/icons/%s\" ALT=\"%s\" BORDER=0>%s</A>\n", 
                        CFG.www_url, CFG.www_link2ftp, CFG.www_icon_back, CFG.www_name_back, CFG.www_name_back);

        if ((Current < (inArea - CFG.www_files_page)) && (inArea >= CFG.www_files_page)) {
                fprintf(fa, "&nbsp;<A HREF=\"%s/%s%s/index%d.html\"><IMG SRC=\"/icons/%s\" ALT=\"%s\" BORDER=0>%s</A>\n", 
                        CFG.www_url, CFG.www_link2ftp, Path+strlen(CFG.ftp_base), (Current / CFG.www_files_page) + 1,
                        CFG.www_icon_next, CFG.www_name_next, CFG.www_name_next);
        }

        fprintf(fa, "</DIV><P>\n");
}



FILE *newpage(char *, char *, time_t, int, int);
FILE *newpage(char *Path, char *Name, time_t later, int inArea, int Current)
{
        char            linebuf[1024], outbuf[1024];
        static FILE*    fa;

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
                fprintf(fa, "<HTML>\n");
                fprintf(fa, "<META http-equiv=\"Expires\" content=\"%s\">\n", rfcdate(later));
                fprintf(fa, "<META http-equiv=\"Cache-Control\" content=\"no-cache, must-revalidate\">\n");
                fprintf(fa, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", CFG.www_charset);
                fprintf(fa, "<META name=\"%s\" lang=\"en\" content=\"%s\">\n", CFG.www_author, outbuf);
                fprintf(fa, "<HEAD><TITLE>%s</TITLE>\n", outbuf);
                fprintf(fa, "<LINK rel=stylesheet HREF=\"%s/css/files.css\">\n", CFG.www_url);
                fprintf(fa, "<STYLE TYPE=\"text/css\">\n");
                fprintf(fa, "</STYLE>\n</HEAD>\n<BODY>\n");
                pagelink(fa, Path, inArea, Current);
                fprintf(fa, "<H1 align=center>File index of %s</H1><P>\n", outbuf);
                fprintf(fa, "<TABLE align=center width=750>\n");
                fprintf(fa, "<TR><TH>Nr.</TH><TH>Filename</TH><TH>Date</TH><TH>Size</TH><TH>Downloads</TH><TH>Description</TH></TR>\n");
                return fa;
        }
        return NULL;
}



void closepage(FILE *, char *, int, int);
void closepage(FILE *fa, char *Path, int inArea, int Current)
{
        char    *temp1, *temp2;

        if (fa == NULL)
                return;

        temp1 = calloc(PATH_MAX, sizeof(char));
        temp2 = calloc(PATH_MAX, sizeof(char));
        fprintf(fa, "</TABLE><P>\n");
        pagelink(fa, Path, inArea, lastfile);
        fprintf(fa, "</BODY></HTML>\n");
        fclose(fa);
        if (lastfile) {
                sprintf(temp1, "%s/index%d.html", Path, lastfile / CFG.www_files_page);
                sprintf(temp2, "%s/index%d.temp", Path, lastfile / CFG.www_files_page);
        } else {
                sprintf(temp1, "%s/index.html", Path);
                sprintf(temp2, "%s/index.temp", Path);
        }
        rename(temp2, temp1);
        free(temp1);
        free(temp2);
        chmod(temp1, 0644);
        fa = NULL;
}



/*
 * Build a sorted index for the file request processor.
 * Build html index pages for download.
 */
void Index(void)
{
    FILE		*pAreas, *pFile, *pIndex, *fa, *fm, *fp;
    unsigned long	i, iAreas, iAreasNew = 0, record, KSize = 0L, aSize = 0;
    int			iTotal = 0, AreaNr = 0, j, z, x = 0, Areas = 0;
    int			Total = 0, aTotal = 0, inArea = 0, filenr;
    int			fbAreas = 0, fbFiles = 0;
    char		*sAreas, *fAreas, *newdir = NULL, *sIndex, *fn, *temp;
    char		linebuf[1024], outbuf[1024];
    time_t		last = 0L, later;
    Findex		*fdx = NULL;
    Findex		*tmp;
    struct FILEIndex	idx;

    sAreas = calloc(PATH_MAX, sizeof(char));
    fAreas = calloc(PATH_MAX, sizeof(char));
    sIndex = calloc(PATH_MAX, sizeof(char));
    fn     = calloc(PATH_MAX, sizeof(char));
    temp   = calloc(PATH_MAX, sizeof(char));

    later = time(NULL) + 86400;

    IsDoing("Index files");
    if (!do_quiet) {
	colour(3, 0);
	printf("Create index files...\n");
    }

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	die(0);
    }

    sprintf(sIndex, "%s/etc/request.index", getenv("MBSE_ROOT"));
    if ((pIndex = fopen(sIndex, "w")) == NULL) {
	WriteError("$Can't create %s", sIndex);
	die(0);
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
	/*
	 * Because these web pages are dynamic, ie. they change everytime you
	 * receive new files and recreates these pages, extra HTTP headers are
	 * send to the client about these pages. It forbids proxy servers to
	 * cache these pages. The pages will expire within 24 hours.  The pages 
	 * also have an author name, this is the bbs name, and a content 
	 * description for search engines. Automatic advertising.
	 */
	sprintf(linebuf, "File areas at %s", CFG.bbs_name);
	html_massage(linebuf, outbuf);
	fprintf(fm, "<HTML>\n");
	fprintf(fm, "<META http-equiv=\"Expires\" content=\"%s\">\n", rfcdate(later));
	fprintf(fm, "<META http-equiv=\"Cache-Control\" content=\"no-cache, must-revalidate\">\n");
	fprintf(fm, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", CFG.www_charset);
	fprintf(fm, "<META name=\"%s\" lang=\"en\" content=\"%s\">\n", CFG.www_author, outbuf);
	fprintf(fm, "<HEAD><TITLE>%s</TITLE>\n", outbuf);
	fprintf(fm, "<LINK rel=stylesheet HREF=\"%s/css/files.css\">\n", CFG.www_url);
	fprintf(fm, "<STYLE TYPE=\"text/css\">\n");
	fprintf(fm, "</STYLE>\n</HEAD>\n<BODY>\n");
	fprintf(fm, "<H2 align=center>%s</H2><P>\n", outbuf);
	fprintf(fm, "<TABLE align=center width=750>\n");
	fprintf(fm, "<TR><TH>Area</TH><TH>Description</TH><TH>Files</TH><TH>Total size</TH><TH>Last added</TH></TR>\n");
    }

    for (i = 1; i <= iAreas; i++) {

	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);
	AreaNr++;

	if (area.Available) {

	    if (!diskfree(CFG.freespace))
		die(101);

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

	    sprintf(fAreas, "%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), i);

	    /*
	     * Open the file database, if it doesn't exist,
	     * create an empty one.
	     */
	    if ((pFile = fopen(fAreas, "r+")) == NULL) {
		Syslog('!', "Creating new %s", fAreas);
		if ((pFile = fopen(fAreas, "a+")) == NULL) {
		    WriteError("$Can't create %s", fAreas);
		    die(0);
		}
	    } 

	    /*
	     * Create file request index if requests are allowed in this area.
	     */
	    if (area.FileReq) {
		/*
		 * Now start creating the unsorted index.
		 */
		record = 0;
		while (fread(&file, sizeof(file), 1, pFile) == 1) {
		    iTotal++;
		    if ((iTotal % 10) == 0)
			Marker();
		    memset(&idx, 0, sizeof(idx));
		    sprintf(idx.Name, "%s", tu(file.Name));
		    sprintf(idx.LName, "%s", tu(file.LName));
		    idx.AreaNum = i;
		    idx.Record = record;
		    fill_index(idx, &fdx);
		    record++;
		}
		iAreasNew++;
	    } /* if area.Filereq */

	    /*
	     * Create files.bbs
	     */
	    if (strlen(area.FilesBbs))
		strcpy(temp, area.FilesBbs);
	    else
		sprintf(temp, "%s/files.bbs", area.Path);
	    if ((fp = fopen(temp, "w")) == NULL) {
		WriteError("$Can't create %s", temp);
	    } else {
		fseek(pFile, 0, SEEK_SET);
		fbAreas++;

		while (fread(&file, sizeof(file), 1, pFile) == 1) {
		    if ((!file.Deleted) && (!file.Missing)) {
			fbFiles++;
			fprintf(fp, "%-12s [%ld] %s\r\n", file.Name, file.TimesDL + file.TimesFTP + file.TimesReq, file.Desc[0]);
			for (j = 1; j < 25; j++)
			    if (strlen(file.Desc[j]))
				fprintf(fp, " +%s\r\n", file.Desc[j]);
		    }
		}
		fclose(fp);
		chmod(temp, 0644);
	    }

	    /*
	     * Create 00index file and index.html pages in each available download area.
	     */
	    if (!area.CDrom && fm && (strncmp(CFG.ftp_base, area.Path, strlen(CFG.ftp_base)) == 0)) {

		sprintf(temp, "%s/00index", area.Path);
		if ((fp = fopen(temp, "w")) == NULL) {
		    WriteError("$Can't create %s", temp);
		} else {
		    fseek(pFile, 0, SEEK_SET);
		    Areas++;
		    inArea = 0;
		    while (fread(&file, sizeof(file), 1, pFile) == 1) {
			if ((!file.Deleted) && (!file.Missing))
			    inArea++;
		    }
		    fseek(pFile, 0, SEEK_SET);

		    aSize = 0L;
		    aTotal = 0;
		    last = 0L;
		    fa = newpage(area.Path, area.Name, later, inArea, aTotal);
		    while (fread(&file, sizeof(file), 1, pFile) == 1) {
			if ((!file.Deleted) && (!file.Missing)) {
			    /*
			     * The next is to reduce system load
			     */
			    x++;
			    Total++;
			    aTotal++;
			    if (CFG.slow_util && do_quiet && ((x % 3) == 0))
				usleep(1);
			    for (z = 0; z <= 25; z++) {
				if (strlen(file.Desc[z])) {
				    if (z == 0)
					fprintf(fp, "%-12s %7luK %s ", file.Name, (long)(file.Size / 1024), 
						StrDateDMY(file.UploadDate));
				    else
					fprintf(fp, "                                 ");
				    if ((file.Desc[z][0] == '@') && (file.Desc[z][1] == 'X'))
					fprintf(fp, "%s\n", file.Desc[z]+4);
				    else
					fprintf(fp, "%s\n", file.Desc[z]);
				}
			    }
			    
			    fprintf(fa, "<TR><TD align=right valign=top>%d</TD>", aTotal);
			    /*
			     * Check if this is a .gif or .jpg file, if so then
			     * check if a thumbnail file exists. If not try to
			     * create a thumbnail file to add to the html listing.
			     */
			    if (strstr(file.LName, ".gif") || strstr(file.LName, ".jpg") ||
				strstr(file.LName, ".GIF") || strstr(file.LName, ".JPG")) {
				sprintf(linebuf, "%s/%s", area.Path, file.LName);
				sprintf(outbuf, "%s/.%s", area.Path, file.LName);
				if (file_exist(outbuf, R_OK)) {
				    if ((j = execute(CFG.www_convert, linebuf, outbuf,
						    (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null"))) {
					Syslog('+', "Failed to create thumbnail for %s, rc=% d", file.LName, j);
				    } else {
					chmod(outbuf, 0644);
				    }
				}
				fprintf(fa, "<TD align=center valign=top><A HREF=\"%s/%s%s/%s\">",
					CFG.www_url, CFG.www_link2ftp,
					area.Path+strlen(CFG.ftp_base), file.LName);
				fprintf(fa, "<IMG SRC=\"%s/%s%s/.%s\" ALT=\"%s\" BORDER=0>",
					CFG.www_url, CFG.www_link2ftp,
					area.Path+strlen(CFG.ftp_base), file.LName, file.LName);
				fprintf(fa, "</A></TD>");
			    } else {
				fprintf(fa, "<TD valign=top><A HREF=\"%s/%s%s/%s\">%s</A></TD>",
					CFG.www_url, CFG.www_link2ftp,
					area.Path+strlen(CFG.ftp_base), file.LName, file.LName);
			    }
			    fprintf(fa, "<TD valign=top>%s</TD>", StrDateDMY(file.FileDate));
			    fprintf(fa, "<TD align=right valign=top>%lu Kb.</TD>",
				    (long)(file.Size / 1024));
			    fprintf(fa, "<TD valign=top>%8ld</TD>",
				    file.TimesDL + file.TimesFTP + file.TimesReq);
			    fprintf(fa, "<TD><PRE>");
			    for (j = 0; j < 25; j++)
				if (strlen(file.Desc[j])) {
				    if (j)
					fprintf(fa, "\n");
				    sprintf(linebuf, "%s", strkconv(file.Desc[j], CHRS_DEFAULT_FTN, CHRS_DEFAULT_RFC));
				    html_massage(linebuf, outbuf);
				    fprintf(fa, "%s", outbuf);
				}
			    fprintf(fa, "</PRE></TD></TR>\n");
			    aSize += file.Size;
			    if (file.FileDate > last)
				last = file.FileDate;
			    if ((aTotal % CFG.www_files_page) == 0) {
				closepage(fa, area.Path, inArea, aTotal);
				fa = newpage(area.Path, area.Name, later, inArea, aTotal);
			    }
			} /* if (!file.deleted) */
		    }
		    KSize += aSize / 1024;
		    closepage(fa, area.Path, inArea, aTotal);
		    fclose(fp);
		    chmod(temp, 0644);

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

		    fprintf(fm, "<TR><TD align=right>%d</TD><TD><A HREF=\"%s/%s%s/index.html\">%s</A></TD>",
			    AreaNr, CFG.www_url, CFG.www_link2ftp, area.Path+strlen(CFG.ftp_base), area.Name);
		    fprintf(fm, "<TD align=right>%d</TD>", aTotal);
		    if (aSize > 1048576)
			fprintf(fm, "<TD align=right>%ld Mb.</TD>", aSize / 1048576);
		    else
			fprintf(fm, "<TD align=right>%ld Kb.</TD>", aSize / 1024);
		    if (last == 0L)
			fprintf(fm, "<TD>&nbsp;</TD></TR>\n");
		    else
			fprintf(fm, "<TD align=center>%s</TD></TR>\n",  StrDateDMY(last));
		}
	    }
	    fclose(pFile);

	} /* if area.Available */
    }

    if (fm) {
	fprintf(fm, "<TR align=right><TH>&nbsp;</TH><TH>Total</TH><TD>%d</TD><TD>%ld Mb.</TD><TD>&nbsp;</TD></TR>\n",
		Total, KSize / 1024);
	fprintf(fm, "</TABLE><P>\n");
	fprintf(fm, "<A HREF=\"/index.html\"><IMG SRC=\"/icons/%s\" ALT=\"%s\" BORDER=0>%s</A>\n",
		CFG.www_icon_home, CFG.www_name_home, CFG.www_name_home);
	fprintf(fm, "</BODY></HTML>\n");
	fclose(fm);
	sprintf(linebuf, "%s/index.html", CFG.ftp_base);
	rename(fn, linebuf);
	chmod(linebuf, 0644);
    }

    fclose(pAreas);

    sort_index(&fdx);
    for (tmp = fdx; tmp; tmp = tmp->next)
	fwrite(&tmp->idx, sizeof(struct FILEIndex), 1, pIndex);
    fclose(pIndex);
    tidy_index(&fdx);

    Syslog('+', "Index Areas [%5d] Files [%5d]", iAreasNew, iTotal);
    Syslog('+', "Files Areas [%5d] Files [%5d]", fbAreas, fbFiles);
    Syslog('+', "HTML  Areas [%5d] Files [%5d]", Areas, Total);

    if (!do_quiet) {
	printf("\r                                                          \r");
	fflush(stdout);
    }

    free(sIndex);
    free(sAreas);
    free(fAreas);
    free(fn);
    free(temp);
    RemoveSema((char *)"reqindex");
}



