/*****************************************************************************
 *
 * $Id: m_task.c,v 1.17 2008/02/28 22:05:14 mbse Exp $
 * Purpose ...............: Setup TaskManager.
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
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_task.h"



struct taskrec	TCFG;
unsigned int	crc1, crc2;



/*
 * Open database for editing.
 */
int OpenTask(void);
int OpenTask(void)
{
	FILE	*fin;
	char	fnin[PATH_MAX];

	snprintf(fnin, PATH_MAX, "%s/etc/task.data", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		fread(&TCFG, sizeof(TCFG), 1, fin);
		fclose(fin);
		crc1 = 0xffffffff;
		crc1 = upd_crc32((char *)&TCFG, crc1, sizeof(TCFG));
		return 0;
	} 
	return -1;
}



void CloseTask(void);
void CloseTask(void)
{
	char	fin[PATH_MAX];
	FILE	*fp;

	crc2 = 0xffffffff;
	crc2 = upd_crc32((char *)&TCFG, crc2, sizeof(TCFG));

	if (crc1 != crc2) {
		if (yes_no((char *)"Configuration is changed, save changes") == 1) {
			working(1, 0, 0);
			snprintf(fin, PATH_MAX, "%s/etc/task.data", getenv("MBSE_ROOT"));
			if ((fp = fopen(fin, "w+")) != NULL) {
				fwrite(&TCFG, sizeof(TCFG), 1, fp);
				fclose(fp);
				chmod(fin, 0640);
				Syslog('+', "Updated \"task.data\"");
			}
			working(6, 0, 0);
		}
	}
}



/*
 * Edit task record.
 */
int EditTask(void);
int EditTask()
{
	int	j;
	char	temp[20];

	clr_index();
	IsDoing("Edit Taskmanager");

	set_color(WHITE, BLACK);
	mbse_mvprintw( 4, 1, "18.  EDIT TASK MANAGER");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 6, 1, " 1.  Mailout");
	mbse_mvprintw( 7, 1, " 2.  Mailin");
	mbse_mvprintw( 8, 1, " 3.  Newnews");
	mbse_mvprintw( 9, 1, " 4.  Index 1");
	mbse_mvprintw(10, 1, " 5.  Index 2");
	mbse_mvprintw(11, 1, " 6.  Index 3");
	mbse_mvprintw(12, 1, " 7.  Msglink");
	mbse_mvprintw(13, 1, " 8.  Reqindex");
	mbse_mvprintw(14, 1, " 9.  Ping #1");
	mbse_mvprintw(15, 1, "10.  Ping #2");
	mbse_mvprintw(16, 1, "11.  Max TCP");
	mbse_mvprintw(17, 1, "12.  Max Load");

	mbse_mvprintw(16,41, "13.  ZMH start");
	mbse_mvprintw(17,41, "14.  ZMH end");


	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 6, 15,65, TCFG.cmd_mailout);
		show_str( 7, 15,65, TCFG.cmd_mailin);
		show_str( 8, 15,65, TCFG.cmd_newnews);
		show_str( 9, 15,65, TCFG.cmd_mbindex1);
		show_str(10, 15,65, TCFG.cmd_mbindex2);
		show_str(11, 15,65, TCFG.cmd_mbindex3);
		show_str(12, 15,65, TCFG.cmd_msglink);
		show_str(13, 15,65, TCFG.cmd_reqindex);
		show_str(14, 15,40, TCFG.isp_ping1);
		show_str(15, 15,40, TCFG.isp_ping2);
		show_int(16, 15,    TCFG.max_tcp);
		snprintf(temp, 10, "%0.2f", TCFG.maxload);
		show_str(17, 15,5, temp);

		show_str( 16,56, 5, TCFG.zmh_start);
		show_str( 17,56, 5, TCFG.zmh_end);

		j = select_menu(14);
		switch(j) {
		case 0:	return 0;
		case 1:	E_STR(  6,15,65,TCFG.cmd_mailout,    "The command to execute on semafore ^mailout^")
		case 2: E_STR(  7,15,65,TCFG.cmd_mailin,     "The command to execute on semafore ^mailin^")
		case 3: E_STR(  8,15,65,TCFG.cmd_newnews,    "The command to execute on semafore ^newnews^")
		case 4: E_STR(  9,15,65,TCFG.cmd_mbindex1,   "The compiler 1 command to execute on semafore ^mbindex^")
		case 5: E_STR( 10,15,65,TCFG.cmd_mbindex2,   "The compiler 2 command to execute on semafore ^mbindex^")
		case 6: E_STR( 11,15,65,TCFG.cmd_mbindex3,   "The compiler 3 command to execute on semafore ^mbindex^")
		case 7: E_STR( 12,15,65,TCFG.cmd_msglink,    "The command to execute on semafore ^msglink^")
		case 8: E_STR( 13,15,65,TCFG.cmd_reqindex,   "The command to execute on semafore ^reqindex^")
		case 9: E_STR( 14,15,40,TCFG.isp_ping1,      "The ^IP address^ of host 1 to check the Internet Connection")
		case 10:E_STR( 15,15,40,TCFG.isp_ping2,      "The ^IP address^ of host 2 to check the Internet Connection")
		case 11:E_INT( 16,15,   TCFG.max_tcp,        "Maximum simultanous ^TCP/IP^ connections")
		case 12:strcpy(temp, edit_str(17,15,5,temp, (char *)"^Maximum system load^ at which processing stops (1.00 .. 3.00)"));
			sscanf(temp, "%f", &TCFG.maxload);
			break;
		case 13:E_STR( 16,56,5, TCFG.zmh_start,      "^Start^ of Zone Mail Hour in UTC")
		case 14:E_STR( 17,56,5, TCFG.zmh_end,        "^End& of Zone Mail Hour in UTC")
		}
	}

	return 0;
}



void task_menu(void)
{
	OpenTask();
	EditTask();
	CloseTask();
}


int task_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *no;

    snprintf(temp, PATH_MAX, "%s/etc/task.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;
    fread(&TCFG, sizeof(TCFG), 1, no);
    fclose(no);

    wp = open_webdoc((char *)"task.html", (char *)"Task Manager", NULL);
    fprintf(wp, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(wp, "<P>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"Command on mailout", TCFG.cmd_mailout);
    add_webtable(wp, (char *)"Command on mailin", TCFG.cmd_mailin);
    add_webtable(wp, (char *)"Command on newnews", TCFG.cmd_newnews);
    add_webtable(wp, (char *)"Command on mbindex 1", TCFG.cmd_mbindex1);
    add_webtable(wp, (char *)"Command on mbindex 2", TCFG.cmd_mbindex1);
    add_webtable(wp, (char *)"Command on mbindex 3", TCFG.cmd_mbindex2);
    add_webtable(wp, (char *)"Command on msglink", TCFG.cmd_msglink);
    add_webtable(wp, (char *)"Command on reqindex", TCFG.cmd_reqindex);
    fprintf(wp, "<TR><TD colspan=2>&nbsp;</TD></TR>\n");
    add_webtable(wp, (char *)"Zone Mail Hour start", TCFG.zmh_start);
    add_webtable(wp, (char *)"Zone Mail Hour end", TCFG.zmh_end);
    fprintf(wp, "<TR><TD colspan=2>&nbsp;</TD></TR>\n");
    add_webtable(wp, (char *)"ISP ping host 1", TCFG.isp_ping1);
    add_webtable(wp, (char *)"ISP ping host 2", TCFG.isp_ping2);
    fprintf(wp, "<TR><TD colspan=2>&nbsp;</TD></TR>\n");
    snprintf(temp, 10, "%0.2f", TCFG.maxload);
    add_webtable(wp, (char *)"Maximum system load", temp);
    add_webdigit(wp, (char *)"Max TCP/IP connections", TCFG.max_tcp);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    close_webdoc(wp);
					
    page = newpage(fp, page);
    addtoc(fp, toc, 18, 0, page, (char *)"Task manager");

    fprintf(fp, "\n");
    fprintf(fp, "     Command on mailout     %s\n", TCFG.cmd_mailout);
    fprintf(fp, "     Command on mailin      %s\n", TCFG.cmd_mailin);
    fprintf(fp, "     Command on newnews     %s\n", TCFG.cmd_newnews);
    fprintf(fp, "     Command on mbindex 1   %s\n", TCFG.cmd_mbindex1);
    fprintf(fp, "     Command on mbindex 2   %s\n", TCFG.cmd_mbindex2);
    fprintf(fp, "     Command on mbindex 3   %s\n", TCFG.cmd_mbindex3);
    fprintf(fp, "     Command on msglink     %s\n", TCFG.cmd_msglink);
    fprintf(fp, "     Command on reqindex    %s\n\n", TCFG.cmd_reqindex);

    fprintf(fp, "     Zone Mail Hour start   %s\n", TCFG.zmh_start);
    fprintf(fp, "     Zone Mail Hour end     %s\n\n", TCFG.zmh_end);

    fprintf(fp, "     ISP ping host 1        %s\n", TCFG.isp_ping1);
    fprintf(fp, "     ISP ping host 2        %s\n", TCFG.isp_ping2);

    fprintf(fp, "     Maximum system load    %0.2f\n", TCFG.maxload);
    fprintf(fp, "     Max TCP/IP connections %d\n", TCFG.max_tcp);

    return page;
}


