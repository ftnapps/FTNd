/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup TaskManager.
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
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_task.h"



struct taskrec	TCFG;
unsigned long	crc1, crc2;



/*
 * Open database for editing.
 */
int OpenTask(void);
int OpenTask(void)
{
	FILE	*fin;
	char	fnin[PATH_MAX];

	sprintf(fnin,  "%s/etc/task.data", getenv("MBSE_ROOT"));
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
			sprintf(fin, "%s/etc/task.data", getenv("MBSE_ROOT"));
			if ((fp = fopen(fin, "w+")) != NULL) {
				fwrite(&TCFG, sizeof(TCFG), 1, fp);
				fclose(fp);
				chmod(fin, 0640);
				Syslog('+', "Updated \"task.data\"");
			}
		}
	}
	working(1, 0, 0);
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
	mvprintw( 4, 1, "18.  EDIT TASK MANAGER");
	set_color(CYAN, BLACK);
	mvprintw( 6, 1, " 1.  Mailout");
	mvprintw( 7, 1, " 2.  Mailin");
	mvprintw( 8, 1, " 3.  Newnews");
	mvprintw( 9, 1, " 4.  Index 1");
	mvprintw(10, 1, " 5.  Index 2");
	mvprintw(11, 1, " 6.  Index 3");
	mvprintw(12, 1, " 7.  Msglink");
	mvprintw(13, 1, " 8.  Reqindex");
	mvprintw(14, 1, " 9.  ISP conn");
	mvprintw(15, 1, "10.  ISP disc");
	mvprintw(16, 1, "11.  Ping #1");
	mvprintw(17, 1, "12.  Ping #2");
	mvprintw(18, 1, "13.  Max TCP");
	mvprintw(19, 1, "14.  Max Load");

	mvprintw(18,41, "15.  ZMH start");
	mvprintw(19,41, "16.  ZMH end");


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
		show_str(14, 15,65, TCFG.isp_connect);
		show_str(15, 15,65, TCFG.isp_hangup);
		show_str(16, 15,40, TCFG.isp_ping1);
		show_str(17, 15,40, TCFG.isp_ping2);
		show_int(18, 15,    TCFG.max_tcp);
		sprintf(temp, "%0.2f", TCFG.maxload);
		show_str(19, 15,5, temp);

		show_str( 18,56, 5, TCFG.zmh_start);
		show_str( 19,56, 5, TCFG.zmh_end);

		j = select_menu(16);
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
		case 9: E_STR( 14,15,65,TCFG.isp_connect,    "The command to ^connect^ the Internet Connection")
		case 10:E_STR( 15,15,65,TCFG.isp_hangup,     "The command to ^hangup^ the Internet Connection")
		case 11:E_STR( 16,15,40,TCFG.isp_ping1,      "The ^IP address^ of host 1 to check the Internet Connection")
		case 12:E_STR( 17,15,40,TCFG.isp_ping2,      "The ^IP address^ of host 2 to check the Internet Connection")
		case 13:E_INT( 18,15,   TCFG.max_tcp,        "Maximum simultanous ^TCP/IP^ connections")
		case 14:strcpy(temp, edit_str(19,15,5,temp, (char *)"^Maximum system load^ at which processing stops (1.00 .. 3.00)"));
			sscanf(temp, "%f", &TCFG.maxload);
			break;
		case 15:E_STR( 18,56,5, TCFG.zmh_start,      "^Start^ of Zone Mail Hour in UTC")
		case 16:E_STR( 19,56,5, TCFG.zmh_end,        "^End& of Zone Mail Hour in UTC")
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
	char	temp[PATH_MAX];
	FILE	*no;

	sprintf(temp, "%s/etc/task.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;
	fread(&TCFG, sizeof(TCFG), 1, no);
	fclose(no);

	page = newpage(fp, page);
	addtoc(fp, toc, 16, 0, page, (char *)"Task manager");

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

	fprintf(fp, "     ISP connect command    %s\n", TCFG.isp_connect);
	fprintf(fp, "     ISP hangup command     %s\n", TCFG.isp_hangup);
	fprintf(fp, "     ISP ping host 1        %s\n", TCFG.isp_ping1);
	fprintf(fp, "     ISP ping host 2        %s\n", TCFG.isp_ping2);

	fprintf(fp, "     Maximum system load    %0.2f\n", TCFG.maxload);
	fprintf(fp, "     Max TCP/IP connections %d\n", TCFG.max_tcp);

	return page;
}


