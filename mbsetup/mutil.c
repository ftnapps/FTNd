/*****************************************************************************
 *
 * $Id: mutil.c,v 1.20 2007/11/25 15:49:46 mbse Exp $
 * Purpose ...............: Menu Utils
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
#include "../lib/diesel.h"
#include "screen.h" 
#include "ledit.h"
#include "mutil.h"



unsigned char readkey(int y, int x, int fg, int bg)
{
	int		rc = -1, i;
	unsigned char 	ch = 0;

	working(0, 0, 0);
	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 9");
		exit(MBERR_TTYIO_ERROR);
	}
	mbse_Setraw();

	i = 0;
	while (rc == -1) {
		if ((i % 10) == 0)
			show_date(fg, bg, 0, 0);

		mbse_locate(y, x);
		fflush(stdout);
		rc = mbse_Waitchar(&ch, 5);
		if ((rc == 1) && (ch != KEY_ESCAPE))
			break;

		if ((rc == 1) && (ch == KEY_ESCAPE))
			rc = mbse_Escapechar(&ch);

		if (rc == 1)
			break;
		i++;
		Nopper();
	}

	mbse_Unsetraw();
	close(ttyfd);

	return ch;
}



unsigned char testkey(int y, int x)
{
	int		rc;
	unsigned char	ch = 0;

	mbse_locate(y, x);
	fflush(stdout);

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 9");
		exit(MBERR_TTYIO_ERROR);
	}
	mbse_Setraw();

	rc = mbse_Waitchar(&ch, 100);
	if (rc == 1) {
		if (ch == KEY_ESCAPE)
			rc = mbse_Escapechar(&ch);
	}

	mbse_Unsetraw();
	close(ttyfd);

	if (rc == 1)
		return ch;
	else
		return '\0';
}



int newpage(FILE *fp, int page)
{
	page++;
	fprintf(fp, "\f   MBSE BBS v%-53s   page %d\n", VERSION, page);
	return page;
}



void addtoc(FILE *fp, FILE *toc, int chapt, int par, int page, char *title)
{
	char	temp[81];
	char	*tit;

	snprintf(temp, 81, "%s ", title);
	tit = xstrcpy(title);
	tu(tit);

	if (par) {
		fprintf(toc, "     %2d.%-3d   %s %d\n", chapt, par, padleft(temp, 50, '.'), page);
		fprintf(fp, "\n\n   %d.%d. %s\n\n", chapt, par, tit);
	} else {
		fprintf(toc, "     %2d     %s %d\n", chapt, padleft(temp, 52, '.'), page);
		fprintf(fp, "\n\n   %d. %s\n", chapt, tit);
	}
	free(tit);
}



void disk_reset(void)
{
    SockS("DRES:0;");
}



FILE *open_webdoc(char *filename, char *title, char *title2)
{
    char    *p, *temp;
    FILE    *fp;
    time_t  now;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/share/doc/html/%s", getenv("MBSE_ROOT"), filename);
    mkdirs(temp, 0755);

    if ((fp = fopen(temp, "w+")) == NULL) {
	WriteError("$Can't create %s", temp);
    } else {
	fprintf(fp, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(fp, "<HTML>\n");
	fprintf(fp, "<HEAD>\n");
	fprintf(fp, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\">\n");
	fprintf(fp, "<META NAME=\"Language\" content=\"en\">\n");
	if (title2)
	    fprintf(fp, "<TITLE>%s: %s</TITLE>\n", title, title2);
	else
	    fprintf(fp, "<TITLE>%s</TITLE>\n", title);
	fprintf(fp, "<STYLE type=\"text/css\">\n");
	fprintf(fp, "BODY      { font: 12pt sans-serif,helvetica,arial; }\n");
	fprintf(fp, "H1        { color: red; font: 16pt sans-serif,helvetica,arial;  font-weight: bold }\n");
	fprintf(fp, "H2        { color: red; font: 14pt sans-serif,helvetica,arial;  font-weight: bold }\n");
	fprintf(fp, "H3        { position: relative; left: 60px;  }\n");
	fprintf(fp, "H5        { color: black; font: 10pt sans-serif,helvetica,arial; }\n");
	fprintf(fp, "A:link    { color: blue }\n");
	fprintf(fp, "A:visited { color: blue }\n");
	fprintf(fp, "A:active  { color: red  }\n");
	fprintf(fp, "TH        { font-weight: bold; }\n");
	fprintf(fp, "PRE       { font-size: 10pt; color: blue; font-family: fixed; }\n");
	fprintf(fp, "</STYLE>\n");
	fprintf(fp, "</HEAD>\n");
	fprintf(fp, "<BODY bgcolor=\"#DDDDAA\" link=\"blue\" alink=\"red\" vlink=\"blue\">\n");
	fprintf(fp, "<A NAME=\"_top\"></A>\n");
	fprintf(fp, "<BLOCKQUOTE>\n");
	if (title2)
	    fprintf(fp, "<DIV Align=\"center\"><H1>%s: %s</H1></DIV>\n", title, title2);
	else
	    fprintf(fp, "<DIV Align=\"center\"><H1>%s</H1></DIV>\n", title);
	now = time(NULL);
	p = ctime(&now);
	Striplf(p);
	fprintf(fp, "<DIV align=\"right\"><H5>Created %s</H5></DIV>\n", p);
    }

    free(temp);
    return fp;
}


void close_webdoc(FILE *fp)
{
    fprintf(fp, "</BLOCKQUOTE>\n");
    fprintf(fp, "</BODY>\n");
    fprintf(fp, "</HTML>\n");
    fclose(fp);
}


void add_webtable(FILE *fp, char *hstr, char *dstr)
{
    char    left[1024], right[1024];

    html_massage(hstr, left, 1023);
    if (strlen(dstr))
	html_massage(dstr, right, 1023);
    else
	snprintf(right, 1024, "&nbsp;");
    fprintf(fp, "<TR><TH align='left'>%s</TH><TD>%s</TD></TR>\n", left, right);
}



void add_webdigit(FILE *fp, char *hstr, int digit)
{
    char    left[1024];

    html_massage(hstr, left, 1023);
    fprintf(fp, "<TR><TH align='left'>%s</TH><TD>%u</TD></TR>\n", left, digit);
}



void add_colors(FILE *fp, char *hstr, int fg, int bg)
{
    fprintf(fp, "<TR><TH align='left'>%s</TH><TD>%s on %s</TR></TR>\n", hstr, get_color(fg), get_color(bg));
}



void add_statcnt(FILE *fp, char * hstr, statcnt st)
{
    fprintf(fp, "<TABLE width='500' border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(fp, "<TBODY>\n");
    fprintf(fp, "<TR><TH colspan='9'>Weekdays overview of %s</TH></TR>\n", hstr);
    fprintf(fp, "<TR><TH>&nbsp;</TH><TH align='left'>Sun</TH><TH align='left'>Mon</TH><TH align='left'>Tue</TH><TH align='left'>Wed</TH><TH align='left'>Thu</TH><TH align='left'>Fri</TH><TH align='left'>Sat</TH><TH align='left'>Total</TH></TR>\n");
    fprintf(fp, "<TR><TH align='left'>This week</TH><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD></TR>\n", st.tdow[0], st.tdow[1], st.tdow[2], st.tdow[3], st.tdow[4], st.tdow[5], st.tdow[6], st.tweek);
    fprintf(fp, "<TR><TH align='left'>Last week</TH><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD></TR>\n", st.ldow[0], st.ldow[1], st.ldow[2], st.ldow[3], st.ldow[4], st.ldow[5], st.ldow[6], st.lweek);
    fprintf(fp, "</TBODY>\n");
    fprintf(fp, "</TABLE>\n");
    fprintf(fp, "<P>\n");
    fprintf(fp, "<TABLE width='500' border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(fp, "<TBODY>\n");    
    fprintf(fp, "<TR><TH colspan='12'>Monthly overview of %s</TH><TH align='left' rowspan='2'>Total ever</TH></TR>\n", hstr);
    fprintf(fp, "<TR><TH align='left'>Jan</TH><TH align='left'>Feb</TH><TH align='left'>Mar</TH><TH align='left'>Apr</TH><TH align='left'>May</TH><TH align='left'>Jun</TH><TH align='left'>Jul</TH><TH align='left'>Aug</TH><TH align='left'>Sep</TH><TH align='left'>Oct</TH><TH align='left'>Nov</TH><TH align='left'>Dec</TH></TR>\n");
    fprintf(fp, "<TR><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD></TR>\n", 
	    st.month[0], st.month[1], st.month[2], st.month[3], st.month[4], st.month[5], st.month[6], st.month[7], st.month[8], st.month[0], st.month[10], st.month[11], st.total);
    fprintf(fp, "</TBODY>\n");
    fprintf(fp, "</TABLE>\n");
    fprintf(fp, "<P>\n");
}



int horiz;

void dotter(void)
{
    Nopper();
    mbse_mvprintw(8, horiz++, (char *)".");
    fflush(stdout);
}

