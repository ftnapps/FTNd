/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_global.h"
#include "m_bbs.h"
#include "m_farea.h"
#include "m_fgroup.h"
#include "m_mail.h"
#include "m_mgroup.h"
#include "m_hatch.h"
#include "m_tic.h"
#include "m_ticarea.h"
#include "m_magic.h"
#include "m_fido.h"
#include "m_lang.h"
#include "m_archive.h"
#include "m_virus.h"
#include "m_tty.h"
#include "m_limits.h"
#include "m_users.h"
#include "m_node.h"
#include "m_fdb.h"
#include "m_new.h"
#include "m_ol.h"
#include "m_protocol.h"
#include "m_ff.h"
#include "m_modem.h"
#include "m_marea.h"
#include "m_ngroup.h"
#include "m_service.h"
#include "m_domain.h"
#include "m_task.h"


mode_t		oldmask;		/* Old umask value	 	*/
extern int	do_quiet;		/* Suppress log to screen	*/
int		exp_golded = FALSE;	/* Export GoldED config		*/
int		init = FALSE;		/* Run init only		*/


static void die(int onsig)
{
	FILE	*fp;
	char	*temp;
	int	i;

	signal(onsig, SIG_IGN);
	if (!init)
	    screen_stop(); 

	if (exp_golded && (config_read() != -1)) {
		temp = calloc(128, sizeof(char));
		sprintf(temp, "%s/etc/golded.inc", getenv("MBSE_ROOT"));

		if ((fp = fopen(temp, "w")) != NULL) {
			fprintf(fp, "; GoldED.inc -- Automatic created by mbsetup %s -- Do not edit!\n\n", VERSION);
			fprintf(fp, "; Basic information\n;\n");
			if (strlen(CFG.sysop_name) && CFG.akavalid[0] && CFG.aka[0].zone) {
			    fprintf(fp, "USERNAME %s\n\n", CFG.sysop_name);
			    fprintf(fp, "ADDRESS %s\n", aka2str(CFG.aka[0]));
			    for (i = 1; i < 40; i++)
				if (CFG.akavalid[i])
					fprintf(fp, "AKA     %s\n", aka2str(CFG.aka[i]));
			    fprintf(fp, "\n");

			    gold_akamatch(fp);
			    fprintf(fp, "; JAM MessageBase Setup\n;\n");
			    fprintf(fp, "JAMPATH %s/tmp/\n", getenv("MBSE_ROOT"));
			    fprintf(fp, "JAMHARDDELETE NO\n\n");

			    fprintf(fp, "; Semaphore files\n;\n");
			    fprintf(fp, "SEMAPHORE NETSCAN    %s/sema/mailout\n", getenv("MBSE_ROOT"));
			    fprintf(fp, "SEMAPHORE ECHOSCAN   %s/sema/mailout\n\n", getenv("MBSE_ROOT"));

			    gold_areas(fp);
			}
			Syslog('+', "Created new %s", temp);
		} else {
			WriteError("$Could not create %s", temp);
		}

		free(temp);
	}

	umask(oldmask);
	if (onsig && (onsig <= NSIG))
		WriteError("MBSETUP finished on signal %s", SigName[onsig]);
	else
		Syslog(' ', "MBSETUP finished");
	ExitClient(0);
}



void soft_info(void);
void soft_info(void)
{
	char	*temp;

	temp = calloc(81, sizeof(char));
	clr_index();
	set_color(YELLOW, BLACK);
	center_addstr( 6, (char *)"MBSE BBS (%s-%s)", OsName(), OsCPU());
	set_color(WHITE, BLACK);
	center_addstr( 8, (char *)COPYRIGHT);
	set_color(YELLOW, BLACK);
	center_addstr(10, (char *)"Made in the Netherlands");
	set_color(WHITE, BLACK);
#ifdef __GLIBC__
	sprintf(temp, "Compiled on glibc v%d.%d", __GLIBC__, __GLIBC_MINOR__);
#else
#ifdef __GNU_LIBRARY__
	sprintf(temp, "Compiled on libc v%d", __GNU_LIBRARY__);
#else
	sprintf(temp, "Compiled on unknown library");
#endif
#endif
	center_addstr(12, temp);
	set_color(LIGHTCYAN, BLACK);
	center_addstr(14, (char *)"http://mbse.sourceforge.net or 2:280/2802");
	set_color(LIGHTGREEN, BLACK);
	center_addstr(LINES -7, (char *)"This is free software; released under the terms of the GNU General");
	center_addstr(LINES -6, (char *)"Public License as published by the Free Software Foundation.");
	set_color(CYAN, BLACK);
	free(temp);
	center_addstr(LINES -4, (char *)"Press any key");
	readkey(LINES - 4, COLS / 2 + 8, LIGHTGRAY, BLACK);
}



void site_docs(void);
void site_docs(void)
{
	FILE	*fp, *toc;
	char	temp[PATH_MAX], temp1[PATH_MAX];
	int	page = 0, line = 0;

	if (config_read() == -1)
		return;

	sprintf(temp, "%s/doc/site.doc", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "w")) == NULL)
		return;

	sprintf(temp1, "%s/tmp/toc.tmp", getenv("MBSE_ROOT"));
	if ((toc = fopen(temp1, "w+")) == NULL) {
		fclose(fp);
		return;
	}

	clr_index();
	working(1, 0, 0);
	IsDoing("Making Sitedocs");
	Syslog('+', "Start creating sitedocs");
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "17.  CREATING SITEDOCS");
	set_color(CYAN, BLACK);
	mvprintw( 7,11, (char *)"Create document in file %s", temp);
	fflush(stdout);

	page = global_doc(fp, toc, page);
	page = fido_doc(fp, toc, page);
	page = archive_doc(fp, toc, page);
	page = virus_doc(fp, toc, page);
	page = modem_doc(fp, toc, page);
	page = tty_doc(fp, toc, page);
	page = node_doc(fp, toc, page);
	page = bbs_doc(fp, toc, page);
	page = mail_doc(fp, toc, page);
	page = tic_doc(fp, toc, page);
	page = newf_group_doc(fp, toc, page);
	page = new_doc(fp, toc, page);
	page = ff_doc(fp, toc, page);
	page = service_doc(fp, toc, page);
	page = domain_doc(fp, toc, page);
	page = task_doc(fp, toc, page);

	/*
	 * Append table of contents
	 */
	page = newpage(fp, page);
	addtoc(fp, toc, 17, 0, page, (char *)"Table of contents");
	fprintf(fp, "\n\n");
	line = 4;
	rewind(toc);

	while (fgets(temp, 256, toc) != NULL) {
		fprintf(fp, "%s", temp);
		line++;
		if (line == 56) {
			page = newpage(fp, page);
			line = 0;
		}
	}

	fprintf(fp, "\f");
	fclose(fp);
	fclose(toc);
	unlink(temp1);

	Syslog('+', "Sitedocs created");

	page = line = 0;
	sprintf(temp, "%s/doc/xref.doc", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "w")) == NULL)
		return;

	sprintf(temp1, "%s/tmp/toc.tmp", getenv("MBSE_ROOT"));
	if ((toc = fopen(temp1, "w+")) == NULL) {
		fclose(fp);
		return;
	}

	Syslog('+', "Start creating crossreference");
	mvprintw( 8,11, (char *)"Create document in file %s", temp);
	fflush(stdout);

	page = limit_users_doc(fp, toc, page);

	/*
	 * Append table of contents
	 */
	page = newpage(fp, page);
	addtoc(fp, toc, 99, 0, page, (char *)"Table of contents");
	fprintf(fp, "\n\n");
	line = 4;
	rewind(toc);

	while (fgets(temp, 256, toc) != NULL) {
		fprintf(fp, "%s", temp);
		line++;
		if (line == 56) {
			page = newpage(fp, page);
			line = 0;
		}
	}

	fprintf(fp, "\f");
	fclose(fp);
	fclose(toc);
	unlink(temp1);

	Syslog('+', "Crossreference created");

	page = line = 0;
	sprintf(temp, "%s/doc/stat.doc", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "w")) == NULL)
		return;

	sprintf(temp1, "%s/tmp/toc.tmp", getenv("MBSE_ROOT"));
	if ((toc = fopen(temp1, "w+")) == NULL) {
		fclose(fp);
		return;
	}

	Syslog('+', "Start creating statistics");
	mvprintw( 9,11, (char *)"Create document in file %s", temp);
	fflush(stdout);

	/*
	 * Append table of contents
	 */
	page = newpage(fp, page);
	addtoc(fp, toc, 99, 0, page, (char *)"Table of contents");
	fprintf(fp, "\n\n");
	line = 4;
	rewind(toc);

	while (fgets(temp, 256, toc) != NULL) {
		fprintf(fp, "%s", temp);
		line++;
		if (line == 56) {
			page = newpage(fp, page);
			line = 0;
		}
	}

	fprintf(fp, "\f");
	fclose(fp);
	fclose(toc);
	unlink(temp1);

	Syslog('+', "Statistics created");

	working(0, 0, 0);

	center_addstr(LINES -4, (char *)"Press any key");
	readkey(LINES -4, COLS / 2 + 8, LIGHTGRAY, BLACK);
	return;
}



void initdatabases(void)
{
    if (!init) {
	clr_index();
	working(1, 0, 0);
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "     INIT DATABASES");
	IsDoing("Init Databases");
    }

    config_read();

    InitArchive();
    InitDomain();
    InitFilearea();
    InitFilefind();
    InitFGroup();
    InitFidonetdb();
    InitHatch();
    InitLanguage();
    InitLimits();
    InitMagics();
    InitMsgarea();
    InitMGroup();
    InitModem();
    InitNewfiles();
    InitNGroup();
    InitNodes();
    InitOneline();
    InitProtocol();
    InitService();
    InitTicarea();
    InitTtyinfo();
    InitUsers();
    InitVirus();

    if (!init) {
	working(0, 0, 0);
	clr_index();
    }
}



int main(int argc, char *argv[])
{
	int		loop = 1;
	struct passwd	*pw;

#ifdef MEMWATCH
	mwInit();
#endif

	/*
	 * Find out who is on the keyboard or automated the keyboard.
	 */
	pw = getpwuid(geteuid());
	if (strcmp(pw->pw_name, (char *)"mbse")) {
	    printf("ERROR: only user \"mbse\" may use this program\n");
#ifdef MEMWATCH
	    mwExit();
#endif
	    exit(1);
	}

	/*
	 * Read the global configuration data, registrate connection
	 */
	config_check(getenv("MBSE_ROOT"));
	config_read();
	InitClient(pw->pw_name, (char *)"mbsetup", CFG.location, CFG.logfile, 0x1f, CFG.error_log);

	/*
	 * Setup several signals so when the program terminate's it
	 * will properly close the curses screens.
	 */
	signal(SIGINT, (void (*))die);
	signal(SIGBUS, (void (*))die);
	signal(SIGSEGV,(void (*))die);
	signal(SIGTERM,(void (*))die);
	signal(SIGKILL,(void (*))die);

	oldmask = umask(002);

	if ((argc == 2) && (strncmp(tl(argv[1]), "i", 1) == 0))
	    init = TRUE;
	else
	    screen_start((char *)"MBsetup");

	do_quiet = TRUE;
	Syslog(' ', " ");
	Syslog(' ', "MBSETUP v%s started by %s", VERSION, pw->pw_name);
	if (init)
	    Syslog('+', "Cmd: mbsetup init");
	initdatabases();
	
	if (!init) {
	    do {
		IsDoing("Browsing Menu");
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "0.    MAIN SETUP");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.    Edit Global configuration");
		mvprintw( 8, 6, "2.    Edit Fido Networks");
		mvprintw( 9, 6, "3.    Edit Archiver Programs");
		mvprintw(10, 6, "4.    Edit Virus Scanners");
		mvprintw(11, 6, "5.    Edit Modem types");
		mvprintw(12, 6, "6.    Edit TTY lines info");
		mvprintw(13, 6, "7.    Edit Fidonet Nodes");
		mvprintw(14, 6, "8.    Edit BBS Setup");
		mvprintw(15, 6, "9.    Edit Mail Setup");
		mvprintw(16, 6, "10.   Edit File Echo's setup");
		mvprintw( 7,46, "11.   Edit Newfiles Groups");
		mvprintw( 8,46, "12.   Edit Newfiles Reports");
		mvprintw( 9,46, "13.   Edit FileFind Setup");
		mvprintw(10,46, "14.   Edit Files Database");
		mvprintw(11,46, "15.   Edit BBS Users");
		mvprintw(12,46, "16.   Edit Services");
		mvprintw(13,46, "17.   Edit Domains");
		mvprintw(14,46, "18.   Edit Task Manager");
		mvprintw(15,46, "19.   Show software information");
		mvprintw(16,46, "20.   Create site documents");
 
		switch(select_menu(20)) {
		case 0:
			loop = 0;
			break;
		case 1:
			global_menu();
			break;
		case 2:
			EditFidonet();
			break;
		case 3:
			EditArchive();
			break;
		case 4:
			EditVirus();
			break;
		case 5:
			EditModem();
			break;
		case 6:
			EditTtyinfo();
			break;
		case 7:
			EditNodes();
			break;
		case 8:
			bbs_menu();
			break;
		case 9:
			mail_menu();
			break;
		case 10:
			tic_menu();
			break;
		case 11:
			EditNGroup();
			break;
		case 12:	
			EditNewfiles();
			break;
		case 13:
			EditFilefind();
			break;
		case 14:
			EditFDB();
			break;
		case 15:
			EditUsers();
			break;
		case 16:
			EditService();
			break;
		case 17:
			EditDomain();
			break;
		case 18:
			task_menu();
			break;
		case 19:
			soft_info();
			break;
		case 20:
			site_docs();
			break;
		}
	    } while (loop == 1);
	}

	die(0);
	return 0;
}

