/*****************************************************************************
 *
 * File ..................: mbsebbs/mball.c
 * Purpose ...............: Creates allfiles listings
 * Last modification date : 28-Jun-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "../lib/mbse.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/dbcfg.h"
#include "../lib/clcomm.h"
#include "mball.h"


extern int	do_quiet;		/* Supress screen output	*/
int		do_zip   = FALSE;	/* Create ZIP archives		*/
int		do_list  = FALSE;	/* Create filelist		*/
int		do_index = FALSE;	/* Create 00index files		*/
extern	int	e_pid;			/* Pid of child			*/
extern	int	show_log;		/* Show logging			*/
time_t		t_start;		/* Start time			*/
time_t		t_end;			/* End time			*/
struct		tm *l_date;		/* Structure for Date		*/
int		lastfile;		/* Last file number		*/


static char *wdays[]={(char *)"Sun",(char *)"Mon",(char *)"Tue",(char *)"Wed",(char *)"Thu",(char *)"Fri",(char *)"Sat"};
static char *months[]={(char *)"Jan",(char *)"Feb",(char *)"Mar",(char *)"Apr",(char *)"May",(char *)"Jun",
                (char *)"Jul",(char *)"Aug",(char *)"Sep",(char *)"Oct",(char *)"Nov",(char *)"Dec"};



void ProgName()
{
	if (do_quiet)
		return;

	colour(15, 0);
	printf("\nMBALL: MBSE BBS %s Allfiles Listing Creator\n", VERSION);
	colour(14, 0);
	printf("       %s\n", Copyright);
}



void die(int onsig)
{
	/*
	 * First check if a child is running, if so, kill it.
	 */
	if (e_pid) {
		if ((kill(e_pid, SIGTERM)) == 0)
			Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
		else {
			if ((kill(e_pid, SIGKILL)) == 0)
				Syslog('+', "SIGKILL to pid %d succeded", e_pid);
			else
				WriteError("$Failed to kill pid %d", e_pid);
		}

		/*
		 * In case the child had the tty in raw mode...
		 */
		system("stty sane");
	}

	signal(onsig, SIG_IGN);

	if (onsig) {
		if (onsig <= NSIG)
			WriteError("$Terminated on signal %d (%s)", onsig, SigName[onsig]);
		else
			WriteError("Terminated with error %d", onsig);
	}

	time(&t_end);
	Syslog(' ', "MBALL finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet) {
		colour(7, 0);
		printf("\n");
	}
	ExitClient(onsig);
}



void Help()
{
	do_quiet = FALSE;
	ProgName();

	colour(11, 0);
	printf("\nUsage:	mball [command] <options>\n\n");
	colour(9, 0);
	printf("	Commands are:\n\n");
	colour(3, 0);
	printf("	i  index	Create \"00index\" files and WWW pages in areas\n");
	printf("	l  list		Create allfiles and newfiles lists\n");
	colour(9, 0);
	printf("\n	Options are:\n\n");
	colour(3, 0);
	printf("	-q -quiet	Quiet mode\n");
	printf("	-z -zip		Create .zip archives\n");
	colour(7, 0);
	printf("\n");
	die(0);
}



int main(int argc, char **argv)
{
	int	i;
	char	*cmd;
	struct	passwd *pw;

#ifdef MEMWATCH
	mwInit();
#endif

	InitConfig();
	TermInit(1);
	time(&t_start);
	umask(000);

	/*
	 * Catch all signals we can, and ignore the rest.
	 */
	for (i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGKILL) || (i == SIGBUS) ||
		    (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
			signal(i, (void (*))die);
		else
			signal(i, SIG_IGN);
	}

	if(argc < 2)
		Help();

	cmd = xstrcpy((char *)"Command line: mball");

	for (i = 1; i < argc; i++) {

		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);

		if (!strncasecmp(argv[i], "l", 1))
			do_list = TRUE;
		if (!strncasecmp(argv[i], "i", 1))
			do_index = TRUE;
		if (!strncasecmp(argv[i], "-q", 2))
			do_quiet = TRUE;
		if (!strncasecmp(argv[i], "-z", 2))
			do_zip = TRUE;
	}

	if (!(do_list || do_index))
		Help();

	ProgName();
	pw = getpwuid(getuid());
	InitClient(pw->pw_name, (char *)"mball", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);

	Syslog(' ', " ");
	Syslog(' ', "MBALL v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet) {
		colour(3, 0);
		printf("\n");
	}

	if (do_list) {
		Masterlist();
		if (do_zip)
			MakeArc();
	}

	if (do_index)
		MakeIndex();

	if (do_list)
		CreateSema((char *)"mailin");

	if (!do_quiet)
		printf("Done!\n");

	die(0);
	return 0;
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
	static char	 buf[40];
	struct tm	 ptm;

	ptm = *gmtime(&now);

	sprintf(buf,"%s, %02d %s %04d %02d:%02d:%02d GMT",
		wdays[ptm.tm_wday], ptm.tm_mday, months[ptm.tm_mon],
		ptm.tm_year + 1900, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
	return(buf);
}



void pagelink(FILE *, char *, int, int);
void pagelink(FILE *fa, char *Path, int inArea, int Current)
{
	char	nr[20];

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
	char		linebuf[1024], outbuf[1024];
	static FILE*	fa;

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
	char	*temp1, *temp2;

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
	fa = NULL;
}



void MakeIndex()
{
	FILE	*fp, *fm, *fa, *pAreas, *pFile;
	int	AreaNr = 0, j, z, x = 0, Areas = 0;
	int     iTotal = 0, aTotal = 0, inArea = 0;
	long    iSize = 0L, aSize = 0;
	char	*sAreas, *fAreas;
	char	temp[81], fn[PATH_MAX], linebuf[1024], outbuf[1024];
	time_t	last = 0L, later;

	sAreas	= calloc(PATH_MAX, sizeof(char));
	fAreas	= calloc(PATH_MAX, sizeof(char));
	later = time(NULL) + 86400;

	IsDoing("Create Indexes");

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("$Can't open File Areas File: %s", sAreas);
		colour(7,0);
		die(1);
	}
	fread(&areahdr, sizeof(areahdr), 1, pAreas);

	if (!do_quiet)
		printf("Processing index lists\n");

        sprintf(fn, "%s/index.temp", CFG.ftp_base);
        if ((fm = fopen(fn, "w")) == NULL) {
                WriteError("$Can't create %s", fn);
                die(0);
        }

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

	while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {

		AreaNr++;

		if (area.Available && !area.CDrom &&
		    (strncmp(CFG.ftp_base, area.Path, strlen(CFG.ftp_base)) == 0)) {

			sprintf(temp, "%s/00index", area.Path);

			if ((fp = fopen(temp, "w")) == NULL) {
				WriteError("$Can't create %s", temp);
			} else {

				sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), AreaNr);

				if ((pFile = fopen (fAreas, "r")) == NULL) {
					WriteError("$Can't open Area %d (%s)! Skipping ...", AreaNr, area.Name);
				} else {
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
							 * The next is to reduce system
							 * loading.
							 */
							x++;
							iTotal++;
							aTotal++;
							if (CFG.slow_util && do_quiet && ((x % 3) == 0))
									usleep(1);

							for (z = 0; z <= 25; z++) {
								if (strlen(file.Desc[z])) {
									if (z == 0)
										fprintf(fp, "%-12s %7luK %s ", file.Name, 
											file.Size / 1024, 
											StrDateDMY(file.UploadDate));
									else
										fprintf(fp, "                                 ");
									if ((file.Desc[z][0] == '@') && (file.Desc[z][1] == 'X'))
										fprintf(fp, "%s\n", file.Desc[z]+4);
									else
										fprintf(fp, "%s\n", file.Desc[z]);
								}
						   	}

							fprintf(fa, "<TR><TD align=right>%d</TD>", aTotal);
							/*
							 * Check if this is a .gif or .jpg file, if so then
							 * check if a thumbnail file exists. If not try to
							 * create a thumbnail file to add to the html listing.
							 */
							if (strstr(file.Name, ".gif") || strstr(file.Name, ".jpg")) {
								sprintf(linebuf, "%s/%s", area.Path, file.Name);
								sprintf(outbuf, "%s/.%s", area.Path, file.Name);
								if (file_exist(outbuf, R_OK)) {
									if ((j = execute(CFG.www_convert, linebuf, outbuf,
										(char *)"/dev/null", (char *)"/dev/null", 
										(char *)"/dev/null"))) {
										Syslog('+', "Failed to create thumbnail for %s, rc=%d", file.Name, j);
									}
								}
								fprintf(fa, "<TD align=center><PRE><A HREF=\"%s/%s%s/%s\">",
									CFG.www_url, CFG.www_link2ftp, 
									area.Path+strlen(CFG.ftp_base), file.Name);
								fprintf(fa, "<IMG SRC=\"%s/%s%s/.%s\" ALT=\"%s\" BORDER=0>", 
									CFG.www_url, CFG.www_link2ftp,
									area.Path+strlen(CFG.ftp_base), file.Name, file.Name);
								fprintf(fa, "</A></PRE></TD>");
							} else {
								fprintf(fa, "<TD><PRE><A HREF=\"%s/%s%s/%s\">%s</A></PRE></TD>", 
									CFG.www_url, CFG.www_link2ftp, 
									area.Path+strlen(CFG.ftp_base), file.Name, file.Name);
							}
							fprintf(fa, "<TD><PRE>%s</PRE></TD>", StrDateDMY(file.FileDate));
							fprintf(fa, "<TD align=right><PRE>%lu Kb.</PRE></TD>", file.Size / 1024);
							fprintf(fa, "<TD><PRE>%8ld</PRE></TD>", 
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
							iSize += file.Size;
							if (file.FileDate > last)
								last = file.FileDate;
							if ((aTotal % CFG.www_files_page) == 0) {
								closepage(fa, area.Path, inArea, aTotal);
								fa = newpage(area.Path, area.Name, later, inArea, aTotal);
							}

					  	} /* if (!file.deletd) */
					}
					fclose(pFile);
					closepage(fa, area.Path, inArea, aTotal);
				}
				fclose(fp);
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
	} /* End of While Loop Checking for Areas Done */

        fprintf(fm, "<TR align=right><TH>&nbsp;</TH><TH>Total</TH><TD>%d</TD><TD>%ld Mb.</TD><TD>&nbsp;</TD></TR>\n", 
                        iTotal, iSize / 1048576);
        fprintf(fm, "</TABLE><P>\n");
        fprintf(fm, "<A HREF=\"/index.html\"><IMG SRC=\"/icons/%s\" ALT=\"%s\" BORDER=0>%s</A>\n",
                        CFG.www_icon_home, CFG.www_name_home, CFG.www_name_home);
        fprintf(fm, "</BODY></HTML>\n");
        fclose(fm);
        sprintf(linebuf, "%s/index.html", CFG.ftp_base);
        rename(fn, linebuf);

	fclose(pAreas);
	free(sAreas);
	free(fAreas);
	Syslog('+', "Created %d indexes with %d files", Areas, iTotal);
}



void MidLine(char *txt, FILE *fp, int doit)
{
	char	temp[81];
	int	x, y, z;

	if (!doit)
		return;

	z = strlen(txt);
	x = 77 - z;
	x /= 2;
	strcpy(temp, "");

	for (y = 0; y < x; y++)
		strcat(temp, " ");

	strcat(temp, txt);
	z = strlen(temp);
	x = 77 - z;

	for (y = 0; y < x; y++)
		strcat(temp, " ");

	fprintf(fp, "%c", 179);
	fprintf(fp, "%s", temp);
	fprintf(fp, "%c\r\n", 179);
}



void TopBox(FILE *fp, int doit)
{
	int	y;
	
	if (!doit)
		return;

	fprintf(fp, "\r\n%c", 213);
	for(y = 0; y < 77; y++)
		fprintf(fp, "%c", 205);
	fprintf(fp, "%c\r\n", 184);
}



void BotBox(FILE *fp, int doit)
{
	int	y;

	if (!doit)
		return;

	fprintf(fp, "%c", 212);
	for (y = 0; y < 77; y++)
		fprintf(fp, "%c", 205);
	fprintf(fp, "%c\r\n\r\n", 190);
}



void Masterlist()
{
	FILE	*fp, *np, *pAreas, *pFile, *pHeader;
	int	AreaNr = 0, z, x = 0, New;
	long	AllFiles = 0, AllBytes = 0, NewFiles = 0, NewBytes = 0;
	int	AllAreaFiles, AllAreaBytes, popdown, down;
	int	NewAreaFiles, NewAreaBytes;
	char	*sAreas, *fAreas;
	char	temp[81], pop[81];

	sAreas	= calloc(PATH_MAX, sizeof(char));
	fAreas	= calloc(PATH_MAX, sizeof(char));

	IsDoing("Create Allfiles list");

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if(( pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("Can't open File Areas File: %s", sAreas);
		colour(7,0);
		die(1);
	}
	fread(&areahdr, sizeof(areahdr), 1, pAreas);

	if (!do_quiet)
		printf("Processing file areas\n");

	if ((fp = fopen("allfiles.tmp", "a+")) == NULL) {
	 	WriteError("$Can't open allfiles.tmp");
		die(1);
	}
	if ((np = fopen("newfiles.tmp", "a+")) == NULL) {
		WriteError("$Can't open newfiles.tmp");
		fclose(fp);
		die(1);
	}

	TopBox(fp, TRUE);
	TopBox(np, TRUE);
	sprintf(temp, "All available files at %s", CFG.bbs_name);
	MidLine(temp, fp, TRUE);
	sprintf(temp, "New available files since %d days at %s", CFG.newdays, CFG.bbs_name);
	MidLine(temp, np, TRUE);
	BotBox(fp, TRUE);
	BotBox(np, TRUE);

	sprintf(temp, "%s/etc/header.txt", getenv("MBSE_ROOT"));
	if(( pHeader = fopen(temp, "r")) != NULL) {
		Syslog('+', "Inserting %s", temp);

		while( fgets(temp, 80 ,pHeader) != NULL) {
			Striplf(temp);
			fprintf(fp, "%s\r\n", temp);
			fprintf(np, "%s\r\n", temp);
		}
	}

	while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {
		AreaNr++;
		AllAreaFiles = 0;
		AllAreaBytes = 0;
		NewAreaFiles = 0;
		NewAreaBytes = 0;

		if (area.Available && (area.LTSec.level <= CFG.security.level)) {

			sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), AreaNr);

			if ((pFile = fopen (fAreas, "r")) == NULL) {
				WriteError("$Can't open Area %d (%s)! Skipping ...", AreaNr, area.Name);
			} else {
				popdown = 0;
				while (fread(&file, sizeof(file), 1, pFile) == 1) {
					if ((!file.Deleted) && (!file.Missing)) {
						/*
						 * The next is to reduce system
						 * loading.
						 */
						x++;
						if (CFG.slow_util && do_quiet && ((x % 3) == 0))
							usleep(1);
						AllFiles++;
						AllBytes += file.Size;
						AllAreaFiles++;
						AllAreaBytes += file.Size;
						down = file.TimesDL + file.TimesFTP + file.TimesReq;
						if (down > popdown) {
							popdown = down;
							sprintf(pop, "%s", file.Name);
						}
						if (((t_start - file.UploadDate) / 84400) <= CFG.newdays) {
							NewFiles++;
							NewBytes += file.Size;
							NewAreaFiles++;
							NewAreaBytes += file.Size;
						}
					}
				}

				/*
				 * If there are files to report do it.
				 */
				if (AllAreaFiles) {
					TopBox(fp, TRUE);
					TopBox(np, NewAreaFiles);

					sprintf(temp, "Area %d - %s", AreaNr, area.Name);
					MidLine(temp, fp, TRUE);
					MidLine(temp, np, NewAreaFiles);

					sprintf(temp, "File Requests allowed");
					MidLine(temp, fp, area.FileReq);
					MidLine(temp, np, area.FileReq && NewAreaFiles);

					sprintf(temp, "%d KBytes in %d files", AllAreaBytes / 1024, AllAreaFiles);
					MidLine(temp, fp, TRUE);
					sprintf(temp, "%d KBytes in %d files", NewAreaBytes / 1024, NewAreaFiles);
					MidLine(temp, np, NewAreaFiles);
					if (popdown) {
						sprintf(temp, "Most popular file is %s", pop);
						MidLine(temp, fp, TRUE);
					}

					BotBox(fp, TRUE);
					BotBox(np, NewAreaFiles);

					fseek(pFile, 0, SEEK_SET);
					while (fread(&file, sizeof(file), 1, pFile) == 1) {
						if((!file.Deleted) && (!file.Missing)) {
							New = (((t_start - file.UploadDate) / 84400) <= CFG.newdays);
							sprintf(temp, "%-12s%10lu K %s [%04ld] Uploader: %s",
								file.Name, file.Size / 1024, StrDateDMY(file.UploadDate), 
								file.TimesDL + file.TimesFTP + file.TimesReq, 
								strlen(file.Uploader)?file.Uploader:"");
							fprintf(fp, "%s\r\n", temp);
							if (New)
								fprintf(np, "%s\r\n", temp);
	
							for (z = 0; z <= 25; z++) {
								if (strlen(file.Desc[z])) {
									if ((file.Desc[z][0] == '@') && (file.Desc[z][1] == 'X')) {
										fprintf(fp, "                         %s\r\n",file.Desc[z]+4);
										if (New)
											fprintf(np, "                         %s\r\n",file.Desc[z]+4);
									} else {
										fprintf(fp, "                         %s\r\n",file.Desc[z]);
										if (New)
											fprintf(np, "                         %s\r\n",file.Desc[z]);
									}
								}
						   	}
					  	}
					}
				}
				fclose(pFile);
			}
		}
	} /* End of While Loop Checking for Areas Done */

	fclose(pAreas);

	TopBox(fp, TRUE);
	TopBox(np, TRUE);
	sprintf(temp, "Total %ld files, %ld KBytes", AllFiles, AllBytes / 1024);
	MidLine(temp, fp, TRUE);
	sprintf(temp, "Total %ld files, %ld KBytes", NewFiles, NewBytes / 1024);
	MidLine(temp, np, TRUE);

	MidLine((char *)"", fp, TRUE);
	MidLine((char *)"", np, TRUE);

	sprintf(temp, "Created by MBSE BBS v%s (Linux) at %s", VERSION, StrDateDMY(t_start));
	MidLine(temp, fp, TRUE);
	MidLine(temp, np, TRUE);

	BotBox(fp, TRUE);
	BotBox(np, TRUE);

	sprintf(temp, "%s/etc/footer.txt", getenv("MBSE_ROOT"));
	if(( pHeader = fopen(temp, "r")) != NULL) {
		Syslog('+', "Inserting %s", temp);

		while( fgets(temp, 80 ,pHeader) != NULL) {
			Striplf(temp);
			fprintf(fp, "%s\r\n", temp);
			fprintf(np, "%s\r\n", temp);
		}
	}

	fclose(fp);
	fclose(np);

	if ((rename("allfiles.tmp", "allfiles.txt")) == 0)
		unlink("allfiles.tmp");
	if ((rename("newfiles.tmp", "newfiles.txt")) == 0)
		unlink("newfiles.tmp");

	Syslog('+', "Allfiles: %ld, %ld MBytes", AllFiles, AllBytes / 1048576);
	Syslog('+', "Newfiles: %ld, %ld MBytes", NewFiles, NewBytes / 1048576);
	free(sAreas);
	free(fAreas);
}



void MakeArc()
{
	char	*cmd;

	if (!getarchiver((char *)"ZIP")) {
		WriteError("ZIP Archiver not available");
		return;
	}

	cmd = xstrcpy(archiver.farc);

	if (cmd == NULL) {
		WriteError("ZIP archive command not available");
		return;
	}

	if (!do_quiet)
		printf("Creating allfiles.zip\n");
	if (!execute(cmd, (char *)"allfiles.zip allfiles.txt", (char *)NULL, (char *)"/dev/null", 
			(char *)"/dev/null", (char *)"/dev/null") == 0)
		WriteError("Create allfiles.zip failed");

	if (!do_quiet)
		printf("Creating newfiles.zip\n");
	if (!execute(cmd, (char *)"newfiles.zip newfiles.txt", (char *)NULL, (char *)"/dev/null", 
			(char *)"/dev/null", (char  *)"/dev/null") == 0)
		WriteError("Create newfiles.zip failed");
	free(cmd);
}


