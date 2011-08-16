/****************************************************************************
 *
 * Purpose ...............: Global Setup Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2010
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
#include "../paths.h"
#include "../lib/mbselib.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_node.h"
#include "m_marea.h"
#include "m_ticarea.h"
#include "m_new.h"
#include "m_fgroup.h"
#include "m_mgroup.h"
#include "m_limits.h"
#include "m_global.h"
#include "m_lang.h"


char	    *some_fn;
int	    some_fd;
extern int  exp_golded;


//  #define WRLONG cnt = write(some_fd, &longvar, sizeof(longvar));



void config_check(char *path)
{
	static char	buf[PATH_MAX];

	snprintf(buf, PATH_MAX, "%s/etc/config.data", path);
	some_fn = buf;

	/*
	 *  Check if the configuration file exists. If not, exit.
	 */
	some_fd = open(some_fn, O_RDONLY);
	if (some_fd == -1) {
		perror("");
		fprintf(stderr, "Fatal, %s/etc/config.data not found, is mbtask running?\n", path);
		exit(MBERR_CONFIG_ERROR);
	}
	close(some_fd); 
}



int config_read(void)
{
	some_fd = open(some_fn, O_RDONLY);
	if (some_fd == -1) 
		return -1;

	memset(&CFG, 0, sizeof(CFG));
	read(some_fd, &CFG, sizeof(CFG));
	close(some_fd);
	return 0;
}



int config_write(void)
{
	some_fd = open(some_fn, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
	if (some_fd == -1)
		return -1;

	write(some_fd, &CFG, sizeof(CFG));
	close(some_fd);
	chmod(some_fn, 0640);
	exp_golded = TRUE;
	return 0;
}



int cf_open(void)
{
	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Global");
	if (0 == config_read()) {
		return 0;
	} 

	working(2, 0, 0);
	return -1;
}



void cf_close(void)
{
	working(1, 0, 0);
	if (config_write() != 0)
		working(2, 0, 0);
}



void e_reginfo(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "1.2 EDIT REGISTRATION INFO");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.  BBS name");
    mbse_mvprintw( 8, 2, "2.  Maildomain");
    mbse_mvprintw( 9, 2, "3.  Sysop uid");
    mbse_mvprintw(10, 2, "4.  Sysop Fido");
    mbse_mvprintw(11, 2, "5.  Location");
    mbse_mvprintw(12, 2, "6.  OLR id");
    mbse_mvprintw(13, 2, "7.  Comment");
    mbse_mvprintw(14, 2, "8.  Origin");
    mbse_mvprintw(15, 2, "9.  Newuser");
    mbse_mvprintw(16, 2, "10. My FQDN");

    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,17,35, CFG.bbs_name);
	show_str( 8,17,35, CFG.sysdomain);
	show_str( 9,17, 8, CFG.sysop);
	show_str(10,17,35, CFG.sysop_name);
	show_str(11,17,35, CFG.location);
	show_str(12,17, 8, CFG.bbsid);
	show_str(13,17,55, CFG.comment);
	show_str(14,17,50, CFG.origin);  
	show_str(15,17, 8, CFG.startname);
	show_str(16,17,63, CFG.myfqdn);

	switch(select_menu(10)) {
	    case 0: return;
	    case 1: E_STR( 7,17,35, CFG.bbs_name,   "Name of this ^BBS^ system")
	    case 2: E_STR( 8,17,35, CFG.sysdomain,  "Internet ^mail domain^ name of this system")
	    case 3: E_STR( 9,17, 8, CFG.sysop,      "^Unix name^ of the sysop")
	    case 4: E_STR(10,17,35, CFG.sysop_name, "^Fidonet name^ of the sysop")
	    case 5: E_STR(11,17,35, CFG.location,   "^Location^ (city/country) of this system")
	    case 6: E_UPS(12,17, 8, CFG.bbsid,      "^QWK/Bluewave^ packets name")
	    case 7: E_STR(13,17,55, CFG.comment,    "Some ^comment^ you may like to give")
	    case 8: E_STR(14,17,50, CFG.origin,     "Default ^origin^ line under echomail messages")
	    case 9: E_STR(15,17, 8, CFG.startname,  "The ^Unix username^ for new users that is used to start the bbs")
	    case 10:E_STR(16,17,63, CFG.myfqdn,     "My real internet ^Full Qualified Domain Name^ or IP address if not in the DNS")
	}
    }
} 




void e_filenames(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "1.3   EDIT GLOBAL FILENAMES");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.   System logfile");
    mbse_mvprintw( 8, 6, "2.   Error logfile");
    mbse_mvprintw( 9, 6, "3.   Debug logfile");
    mbse_mvprintw(10, 6, "4.   Mgr logfile");
    mbse_mvprintw(11, 6, "5.   Default Menu");
    mbse_mvprintw(12, 6, "6.   Chat Logfile");
    mbse_mvprintw(13, 6, "7.   Welcome Logo");
    
    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,28,14, CFG.logfile);
	show_str( 8,28,14, CFG.error_log);
	show_str( 9,28,14, CFG.debuglog);
	show_str(10,28,14, CFG.mgrlog);
	show_str(11,28,14, CFG.default_menu);
	show_str(12,28,14, CFG.chat_log);
	show_str(13,28,14, CFG.welcome_logo);

	switch(select_menu(7)) {
	    case 0: return;
	    case 1: E_STR( 7,28,14, CFG.logfile,          "The name of the ^system^ logfile.")
	    case 2: E_STR( 8,28,14, CFG.error_log,        "The name of the ^errors^ logfile.")
	    case 3: E_STR( 9,28,14, CFG.debuglog,         "The name of the ^debug^ logfile.")
	    case 4: E_STR(10,28,14, CFG.mgrlog,           "The name of the ^area-/filemgr^ logfile.")
	    case 5: E_STR(11,28,14, CFG.default_menu,     "The name of the ^default^ (top) ^menu^.")
	    case 6: E_STR(12,28,14, CFG.chat_log,         "The name of the ^chat^ logfile.")
	    case 7: E_STR(13,28,14, CFG.welcome_logo,     "The name of the ^BBS logo^ file.")
	}
    }
}



void e_global2(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 4, 6, "1.4 EDIT GLOBAL PATHS - 2");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 6, 2, "1.  Nodelists");
    mbse_mvprintw( 7, 2, "2.  Inbound");
    mbse_mvprintw( 8, 2, "3.  Prot inb.");
    mbse_mvprintw( 9, 2, "4.  Outbound");
    mbse_mvprintw(10, 2, "5.  Out queue");
    mbse_mvprintw(11, 2, "6.  *.msgs");
    mbse_mvprintw(12, 2, "7.  Bad TIC's");
    mbse_mvprintw(13, 2, "8.  TIC queue");
    mbse_mvprintw(14, 2, "9.  TMail DOS");
    mbse_mvprintw(15, 2, "10. TMail Win");
    
    for (;;) {
        set_color(WHITE, BLACK);
        show_str( 6,16,64, CFG.nodelists);
	show_str( 7,16,64, CFG.inbound);
	show_str( 8,16,64, CFG.pinbound);
	show_str( 9,16,64, CFG.outbound);
	show_str(10,16,64, CFG.out_queue);
	show_str(11,16,64, CFG.msgs_path);
	show_str(12,16,64, CFG.badtic);
	show_str(13,16,64, CFG.ticout);
	show_str(14,16,64, CFG.tmailshort);
	show_str(15,16,64, CFG.tmaillong);

        switch (select_menu(10)) {
            case 0: return;
	    case 1: E_PTH( 6,16,64, CFG.nodelists,  "The path to the ^nodelists^.", 0750)
	    case 2: E_PTH( 7,16,64, CFG.inbound,    "The path to the ^inbound^ for unknown systems.", 0750)
	    case 3: E_PTH( 8,16,64, CFG.pinbound,   "The path to the ^nodelists^ for protected systems.", 0750)
	    case 4: E_PTH( 9,16,64, CFG.outbound,   "The path to the base ^outbound^ directory.", 0750)
	    case 5: E_PTH(10,16,64, CFG.out_queue,  "The path to the ^temp outbound queue^ directory.", 0750)
	    case 6: E_PTH(11,16,64, CFG.msgs_path,  "The path to the ^*.msgs^ directory.", 0750)
	    case 7: E_PTH(12,16,64, CFG.badtic,     "The path to the ^bad tic files^.", 0750)
	    case 8: E_PTH(13,16,64, CFG.ticout,     "The path to the ^outgoing TIC^ files.", 0750)
	    case 9: if (strlen(CFG.tmailshort) == 0)
			snprintf(CFG.tmailshort, 65, "%s/var/tmail/short", getenv("MBSE_ROOT"));
		    E_PTH(14,16,64, CFG.tmailshort, "The ^T-Mail 8.3 basepath^ (blank = disable)", 0770)
	    case 10:if (strlen(CFG.tmaillong) == 0)
			snprintf(CFG.tmaillong, 65, "%s/var/tmail/long", getenv("MBSE_ROOT"));
		    E_PTH(15,16,64, CFG.tmaillong,  "The ^T-Mail long basepath^ (blank = disable)", 0770)
        }
    }
}



void s_global(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 4, 6, "1.4 EDIT GLOBAL PATHS");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 6, 2, "1.  Home dirs");
    mbse_mvprintw( 7, 2, "2.  Ftp base");
    mbse_mvprintw( 8, 2, "3.  Arealists");
    mbse_mvprintw( 9, 2, "4.  Ext. edit");
    mbse_mvprintw(10, 2, "5.  Rules dir");
    mbse_mvprintw(11, 2, "6.  Magic's");
    mbse_mvprintw(12, 2, "7.  DOS path");
    mbse_mvprintw(13, 2, "8.  Unix path");
    mbse_mvprintw(14, 2, "9.  LeaveCase");
    mbse_mvprintw(15, 2, "10. Next Screen");
}



void e_global(void)
{
    s_global();

    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  6,16,64, CFG.bbs_usersdir);
        show_str(  7,16,64, CFG.ftp_base);
	show_str(  8,16,64, CFG.alists_path);
	show_str(  9,16,64, CFG.externaleditor);
	show_str( 10,16,64, CFG.rulesdir);
        show_str( 11,16,64, CFG.req_magic);
	show_str( 12,16,64, CFG.dospath);
	show_str( 13,16,64, CFG.uxpath);
	show_bool(14,16,    CFG.leavecase);

	switch (select_menu(10)) {
	    case 0: return;
	    case 1: E_PTH(  6,16,64, CFG.bbs_usersdir,   "The path to the ^users home^ directories.", 0770)
            case 2: E_PTH(  7,16,64, CFG.ftp_base,       "The ^FTP home^ directory to strip of the real directory", 0750)
	    case 3: E_PTH(  8,16,64, CFG.alists_path,    "The path where ^area lists^ and ^filebone lists^ are stored.", 0750)
	    case 4: E_STR(  9,16,64, CFG.externaleditor, "The full path and filename to the ^external msg editor^ (blank=disable)")
	    case 5: E_PTH( 10,16,64, CFG.rulesdir,       "The path where the ^arearules^ are stored", 0750)
	    case 6: E_PTH( 11,16,64, CFG.req_magic,      "The path to the ^magic filerequest^ files.", 0750)
	    case 7: E_STR( 12,16,64, CFG.dospath,        "The translated ^DOS^ drive and path, empty disables translation")
	    case 8: E_PTH( 13,16,64, CFG.uxpath,         "The translated ^Unix^ path.", 0750)
	    case 9: E_BOOL(14,16,    CFG.leavecase,      "^Leave^ outbound flo filenames as is, ^No^ forces uppercase.")
	    case 10:e_global2();
		    s_global();
		    break;
	}
    }
}



void b_screen(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "1.5  EDIT GLOBAL SETTINGS");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.   Exclude Sysop");
    mbse_mvprintw( 8, 2, "2.   Show Connect");
    mbse_mvprintw( 9, 2, "3.   Ask Protocols");
    mbse_mvprintw(10, 2, "4.   Sysop Level");
    mbse_mvprintw(11, 2, "5.   Password Length");
    mbse_mvprintw(12, 2, "6.   Passwd Character");
    mbse_mvprintw(13, 2, "7.   Idle timeout");
    mbse_mvprintw(14, 2, "8.   Login Enters");
    mbse_mvprintw(15, 2, "9.   Homedir Quota");
    mbse_mvprintw(16, 2, "10.  Location length");
    mbse_mvprintw(17, 2, "11.  Show new msgarea");
    mbse_mvprintw(18, 2, "12.  OLR Max. msgs.");

    mbse_mvprintw( 7,37, "13.  OLR Newfile days");
    mbse_mvprintw( 8,37, "14.  OLR Max Filereq");
    mbse_mvprintw( 9,37, "15.  BBS Log Level");
    mbse_mvprintw(10,37, "16.  Utils loglevel");
    mbse_mvprintw(11,37, "17.  Utils slowly");
    mbse_mvprintw(12,37, "18.  CrashMail level");
    mbse_mvprintw(13,37, "19.  FileAttach level");
    mbse_mvprintw(14,37, "20.  Min diskspace MB");
    mbse_mvprintw(15,37, "21.  Simult. logins");
    mbse_mvprintw(16,37, "22.  Child priority");
    mbse_mvprintw(17,37, "23.  Filesystem sync");
    mbse_mvprintw(18,37, "24.  Default language");

    set_color(WHITE, BLACK);
    show_bool( 7,24, CFG.exclude_sysop);
    show_bool( 8,24, CFG.iConnectString);
    show_bool( 9,24, CFG.iAskFileProtocols);
    show_int( 10,24, CFG.sysop_access);
    show_int( 11,24, CFG.password_length);
    show_int( 12,24, CFG.iPasswd_Char);
    show_int( 13,24, CFG.idleout);
    show_int( 14,24, CFG.iCRLoginCount);
    show_int( 15,24, CFG.iQuota);
    show_int( 16,24, CFG.CityLen);
    show_bool(17,24, CFG.NewAreas);
    show_int( 18,24, CFG.OLR_MaxMsgs);

    show_int(  7,59, CFG.OLR_NewFileLimit);
    show_int(  8,59, CFG.OLR_MaxReq);
    show_logl( 9,59, CFG.bbs_loglevel);
    show_logl(10,59, CFG.util_loglevel);
    show_bool(11,59, CFG.slow_util);
    show_int( 12,59, CFG.iCrashLevel);
    show_int( 13,59, CFG.iAttachLevel);
    show_int( 14,59, CFG.freespace);
    show_int( 15,59, CFG.max_logins);
    show_int( 16,59, CFG.priority);
    show_bool(17,59, CFG.do_sync);
    show_str( 18,59, 10, CFG.deflang);
}



void e_bbsglob(void)
{
    b_screen();

    for (;;) {
	switch(select_menu(24)) {
	    case 0: return;
	    case 1: E_BOOL( 7,24, CFG.exclude_sysop,         "^Exclude^ sysop from lists.")
	    case 2: E_BOOL( 8,24, CFG.iConnectString,        "Show ^connect string^ at logon")
	    case 3: E_BOOL( 9,24, CFG.iAskFileProtocols,     "Ask ^file protocol^ before every up- download")
	    case 4: E_INT( 10,24, CFG.sysop_access,          "Sysop ^access level^")
	    case 5: E_IRC( 11,24, CFG.password_length, 2, 8, "Mimimum ^password^ length (2..8)")
	    case 6: E_IRC( 12,24, CFG.iPasswd_Char, 33, 126, "Ascii number of ^password^ character (33..126)")
	    case 7: E_IRC( 13,24, CFG.idleout, 2, 60,        "^Idle timeout^ in minutes (2..60)")
	    case 8: E_INT( 14,24, CFG.iCRLoginCount,         "Maximum ^Login Return^ count")
	    case 9: E_INT( 15,24, CFG.iQuota,                "Maximum ^Quota^ in MBytes in users homedirectory");
	    case 10:E_IRC( 16,24, CFG.CityLen, 3, 6,         "Minimum ^Location name^ length (3..6)")
	    case 11:E_BOOL(17,24, CFG.NewAreas,              "Show ^new^ or ^deleted^ message areas to the user at login.")
	    case 12:E_INT( 18,24, CFG.OLR_MaxMsgs,           "^Maximum messages^ to pack for download (0=unlimited)")

	    case 13:E_INT(  7,59, CFG.OLR_NewFileLimit,      "^Limit Newfiles^ listing for maximum days")
	    case 14:E_INT(  8,59, CFG.OLR_MaxReq,            "Maximum ^Filerequests^ to honor")
	    case 15:E_LOGL(CFG.bbs_loglevel, "1.5.15", b_screen)
	    case 16:E_LOGL(CFG.util_loglevel, "1.5.16", b_screen)
	    case 17:E_BOOL(11,59, CFG.slow_util,             "Let background utilities run ^slowly^")
	    case 18:E_INT( 12,59, CFG.iCrashLevel,           "The user level to allow sending ^CrashMail^")
	    case 19:E_INT( 13,59, CFG.iAttachLevel,          "The user level to allow sending ^File Attaches^")
	    case 20:E_IRC( 14,59, CFG.freespace, 2, 1000,    "Minimum ^free diskspace^ in MBytes on filesystems (2..1000)")
	    case 21:E_INT( 15,59, CFG.max_logins,            "Maximum ^simultaneous logins^ allowed, 0 means unlimited")
	    case 22:E_IRC( 16,59, CFG.priority, 0, 15,       "Subproces ^nice priority^, 0=high, 15=low CPU load")
	    case 23:E_BOOL(17,59, CFG.do_sync,               "Call ^sync^ before and after execute, use Yes on GNU/Linux")
	    case 24:PickLanguage((char *)"1.5.24");
		    snprintf(CFG.deflang, 10, "%s", lang.lc);
		    b_screen();
		    break;
	}
    }
}



void s_newuser(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "1.7   EDIT NEW USERS DEFAULTS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 6, "1.    Access level");  
	mbse_mvprintw( 8, 6, "2.    Cap. Username");
	mbse_mvprintw( 9, 6, "3.    Ask Sex");
	mbse_mvprintw(10, 6, "4.    Ask Voicephone");
	mbse_mvprintw(11, 6, "5.    Ask Dataphone");
	mbse_mvprintw(12, 6, "6.    Telephone scan");
	mbse_mvprintw(13, 6, "7.    Ask Handle");
	mbse_mvprintw(14, 6, "8.    Ask Birth date");
	mbse_mvprintw(15, 6, "9..   Ask Location");
	
	mbse_mvprintw( 8,46, "10.   Ask Hot-Keys");
	mbse_mvprintw( 9,46, "11.   One word names");
	mbse_mvprintw(10,46, "12.   Ask Address");
	mbse_mvprintw(11,46, "13.   Give email");
	mbse_mvprintw(12,46, "14.   Do Newmail");
	mbse_mvprintw(13,46, "15.   Do newfiles");
}



void e_newuser(void)
{
	s_newuser();
	for (;;) {
		set_color(WHITE, BLACK);
		show_sec(  7,28, CFG.newuser_access);
		show_bool( 8,28, CFG.iCapUserName);
		show_bool( 9,28, CFG.iSex);
		show_bool(10,28, CFG.iVoicePhone);
		show_bool(11,28, CFG.iDataPhone);
		show_bool(12,28, CFG.iTelephoneScan);
		show_bool(13,28, CFG.iHandle);
		show_bool(14,28, CFG.iDOB);
		show_bool(15,28, CFG.iLocation);

		show_bool( 8,68, CFG.iHotkeys);
		show_bool( 9,68, CFG.iOneName);
		show_bool(10,68, CFG.AskAddress);
		show_bool(11,68, CFG.GiveEmail);
		show_asktype(12,68, CFG.AskNewmail);
		show_asktype(13,68, CFG.AskNewfiles);

		switch(select_menu(15)) {
		case 0:	return;
		case 1: E_SEC(  7,28, CFG.newuser_access, "1.7.1 NEWUSER SECURITY", s_newuser)
		case 2:	E_BOOL( 8,28, CFG.iCapUserName, "^Capitalize^ username")
		case 3:	E_BOOL( 9,28, CFG.iSex, "Ask users ^sex^")
		case 4:	E_BOOL(10,28, CFG.iVoicePhone, "Ask users ^Voice^ phone number")
		case 5:	E_BOOL(11,28, CFG.iDataPhone, "Ask users ^Data^ phone number")
		case 6:	E_BOOL(12,28, CFG.iTelephoneScan, "Perform ^Telephone^ number scan")
		case 7:	E_BOOL(13,28, CFG.iHandle, "Ask users ^handle^")
		case 8:	E_BOOL(14,28, CFG.iDOB, "Ask users ^Date of Birth^")
		case 9: E_BOOL(15,28, CFG.iLocation, "Ask users ^Location^")

		case 10:E_BOOL( 8,68, CFG.iHotkeys, "Ask user if he wants ^Hot-Keys^")
		case 11:E_BOOL( 9,68, CFG.iOneName, "Allow ^one word^ (not in Unixmode) usernames")
		case 12:E_BOOL(10,68, CFG.AskAddress, "Ask users ^home address^ in 3 lines")
		case 13:E_BOOL(11,68, CFG.GiveEmail, "Give new users an ^private email^ box")
		case 14:CFG.AskNewmail = edit_asktype(12,68,CFG.AskNewmail, 
				(char *)"Set ^new mail^ check at login, toggle wit space, Enter when done");
			break;
		case 15:CFG.AskNewfiles = edit_asktype(13,68,CFG.AskNewfiles, 
				(char *)"Set ^new files^ check at login, toggle wit space, Enter when done");
			break;
		}
	};
}



void e_colors(void)
{
	int fg, bg;

/*
 * With this macro intermediate variables are passed to the color editor to prevent SIGBUS
 * on some CPU's (Sparc).
 */
#define ED_COL(f, b, t, h) fg = f; bg = b; edit_color(&fg, &bg, (char *)t, (char *)h); f = fg; b = bg; break;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 6, "1.8   EDIT TEXT COLOURS");
		set_color(CYAN, BLACK);
		mbse_mvprintw( 7, 6, "1.    Normal text");  
		mbse_mvprintw( 8, 6, "2.    Underline");
		mbse_mvprintw( 9, 6, "3.    Input lines");
		mbse_mvprintw(10, 6, "4.    CR text");
		mbse_mvprintw(11, 6, "5.    More prompt");
		mbse_mvprintw(12, 6, "6.    Hilite text");
		mbse_mvprintw(13, 6, "7.    File name");
		mbse_mvprintw(14, 6, "8.    File size");
		mbse_mvprintw(15, 6, "9.    File date");
		mbse_mvprintw(16, 6, "10.   File descr.");
		mbse_mvprintw(17, 6, "11.   Msg. input");
		S_COL( 7,24, "Normal Text     ", CFG.TextColourF, CFG.TextColourB)
		S_COL( 8,24, "Underline Text  ", CFG.UnderlineColourF, CFG.UnderlineColourB)
		S_COL( 9,24, "Input Text      ", CFG.InputColourF, CFG.InputColourB)
		S_COL(10,24, "CR Text         ", CFG.CRColourF, CFG.CRColourB)
		S_COL(11,24, "More Prompt     ", CFG.MoreF, CFG.MoreB)
		S_COL(12,24, "Hilite Text     ", CFG.HiliteF, CFG.HiliteB)
		S_COL(13,24, "File Name       ", CFG.FilenameF, CFG.FilenameB)
		S_COL(14,24, "File Size       ", CFG.FilesizeF, CFG.FilesizeB)
		S_COL(15,24, "File Date       ", CFG.FiledateF, CFG.FiledateB)
		S_COL(16,24, "File Description", CFG.FiledescF, CFG.FiledescB)
		S_COL(17,24, "Message Input   ", CFG.MsgInputColourF, CFG.MsgInputColourB)

		switch(select_menu(11)) {
		case 0:	return;
		case 1: ED_COL(CFG.TextColourF, CFG.TextColourB, "1.8.1  EDIT COLOR", "normal text")
		case 2: ED_COL(CFG.UnderlineColourF, CFG.UnderlineColourB, "1.8.2  EDIT COLOR", "underline")
		case 3: ED_COL(CFG.InputColourF, CFG.InputColourB, "1.8.3  EDIT COLOR", "input")
		case 4: ED_COL(CFG.CRColourF, CFG.CRColourB, "1.8.4  EDIT COLOR", "<Carriage Return>")
		case 5: ED_COL(CFG.MoreF, CFG.MoreB, "1.8.5  EDIT COLOR", "more prompt")
		case 6: ED_COL(CFG.HiliteF, CFG.HiliteB, "1.8.6  EDIT COLOR", "hilite text")
		case 7: ED_COL(CFG.FilenameF, CFG.FilenameB, "1.8.7  EDIT COLOR", "file name")
		case 8: ED_COL(CFG.FilesizeF, CFG.FilesizeB, "1.8.8  EDIT COLOR", "file size")
		case 9: ED_COL(CFG.FiledateF, CFG.FiledateB, "1.8.9  EDIT COLOR", "file date")
		case 10:ED_COL(CFG.FiledescF, CFG.FiledescB, "1.8.10 EDIT COLOR", "file description")
		case 11:ED_COL(CFG.MsgInputColourF, CFG.MsgInputColourB, "1.8.11 EDIT COLOR", "message input")
		}
	};
}



void e_paging(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "1.9   EDIT SYSOP PAGING");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.    Page Length");
    mbse_mvprintw( 8, 2, "2.    Page Times");
    mbse_mvprintw( 9, 2, "3.    Sysop Area");
    mbse_mvprintw(10, 2, "4.    Ask Reason");
    mbse_mvprintw(11, 2, "5.    Log Chat");
    mbse_mvprintw(12, 2, "6.    Prompt Chk.");
    mbse_mvprintw(13, 2, "7.    Freeze Time");

    for (;;) {
	set_color(WHITE, BLACK);
	show_int(  7,20, CFG.iPageLength);
	show_int(  8,20, CFG.iMaxPageTimes);
	show_int(  9,20, CFG.iSysopArea);
	show_bool(10,20, CFG.iAskReason);
	show_bool(11,20, CFG.iAutoLog);
	show_bool(12,20, CFG.iChatPromptChk);
	show_bool(13,20, CFG.iStopChatTime);

	switch(select_menu(7)) {
	    case 0: return;
	    case 1: E_IRC(  7,20, CFG.iPageLength, 5, 120,  "The ^Length^ of paging in seconds (5..120)")
	    case 2: E_IRC(  8,20, CFG.iMaxPageTimes, 1, 10, "The ^Maximum times^ a user may page in a session (1..10)")
	    case 3: E_INT(  9,20, CFG.iSysopArea,           "The ^Message Area^ for ^Message to sysop^ when page fails")
	    case 4: E_BOOL(10,20, CFG.iAskReason,           "Ask the user the ^reason for chat^")
	    case 5: E_BOOL(11,20, CFG.iAutoLog,             "^Automatic log^ chat sessions")
	    case 6: E_BOOL(12,20, CFG.iChatPromptChk,       "Check for chat at the ^prompt^")
	    case 7: E_BOOL(13,20, CFG.iStopChatTime,        "^Stop^ users time during chat")
	}
    }
}



void e_flags(int Users)
{
    int	    i, x, y, z;
    char    temp[80];

    clr_index();
    set_color(WHITE, BLACK);
    if (Users)
	mbse_mvprintw( 5, 6, "1.6   EDIT USER FLAG DESCRIPTIONS");
    else
	mbse_mvprintw( 5, 6, "1.16  EDIT MANAGER FLAG DESCRIPTIONS");
    
    set_color(CYAN, BLACK);
    for (i = 0; i < 32; i++) {
	if (i < 11) 
	    mbse_mvprintw(i + 7, 2, (char *)"%d.", i+1);
	else
	    if (i < 22) 
		mbse_mvprintw(i - 4, 28, (char *)"%d.", i+1);
	    else
		mbse_mvprintw(i - 15, 54, (char *)"%d.", i+1);
    }
    
    for (;;) {
	set_color(WHITE, BLACK);
	for (i = 0; i < 32; i++) {
	    if (i < 11) {
		if (Users)	
		    show_str(i + 7, 6, 16, CFG.fname[i]);
		else
		    show_str(i + 7, 6, 16, CFG.aname[i]);
	    } else {
		if (i < 22) {
		    if (Users)
			show_str(i - 4, 32, 16, CFG.fname[i]);
		    else
			show_str(i - 4, 32, 16, CFG.aname[i]);
		} else {
		    if (Users)
			show_str(i -15, 58, 16, CFG.fname[i]);
		    else
			show_str(i - 15,58, 16, CFG.aname[i]);
		}
	    }
	}

	z = select_menu(32);
	if (z == 0)
	    return;

	if (z < 12) {
	    x = 6;
	    y = z + 6;
	} else {
	    if (z < 23) {
		x = 32;
		y = z - 5;
	    } else {
		x = 58;
		y = z - 16;
	    }
	}

	snprintf(temp, 80, "Enter a short ^description^ of flag bit %d", z);
	if (Users) {
	    strcpy(CFG.fname[z-1], edit_str(y, x, 16, CFG.fname[z-1], temp));
	} else {
	    strcpy(CFG.aname[z-1], edit_str(y, x, 16, CFG.aname[z-1], temp));
	}
    }
}



void e_ticconf(void)
{
    int	temp;
    
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "1.10   EDIT FILEECHO PROCESSING");
    set_color(CYAN, BLACK);

    mbse_mvprintw( 7, 2, "1.  Keep days");
    mbse_mvprintw( 8, 2, "2.  Hatch pwd");
    mbse_mvprintw( 9, 2, "3.  Systems");
    mbse_mvprintw(10, 2, "4.  Groups");
    mbse_mvprintw(11, 2, "5.  Max. dupes");
    mbse_mvprintw(12, 2, "6.  Keep date");
    mbse_mvprintw(13, 2, "7.  Keep netm");

    mbse_mvprintw( 7,42, "8.  Plus all");
    mbse_mvprintw( 8,42, "9.  Notify");
    mbse_mvprintw( 9,42, "10. Passwd");
    mbse_mvprintw(10,42, "11. Message");
    mbse_mvprintw(11,42, "12. Tic on/off");
    mbse_mvprintw(12,42, "13. Pause");

    for (;;) {
	set_color(WHITE, BLACK);

	show_int( 7,18, CFG.tic_days);
	show_str( 8,18,20, (char *)"********************");
	show_int( 9,18, CFG.tic_systems);
	show_int(10,18, CFG.tic_groups);
	show_int(11,18, CFG.tic_dupes);
	show_bool(12,18, CFG.ct_KeepDate);
	show_bool(13,18, CFG.ct_KeepMgr);

	show_bool( 7,58, CFG.ct_PlusAll);
	show_bool( 8,58, CFG.ct_Notify);
	show_bool( 9,58, CFG.ct_Passwd);
	show_bool(10,58, CFG.ct_Message);
	show_bool(11,58, CFG.ct_TIC);
	show_bool(12,58, CFG.ct_Pause);  

	switch(select_menu(13)) {
	    case 0:	return;

	    case 1: E_INT(  7,18,    CFG.tic_days,     "Number of days to ^keep^ files on hold.")
	    case 2: E_STR(  8,18,20, CFG.hatchpasswd,  "Enter the internal ^hatch^ password.")
	    case 3: temp = CFG.tic_systems;
		    temp = edit_int( 9,18, temp, (char *)"Enter the maximum number of ^connected systems^ in the database.");
		    if (temp < CountNoderec()) {
			errmsg("You have %d nodes defined", CountNoderec());
			show_int( 9,18, CFG.tic_systems);
		    } else {
			CFG.tic_systems = temp;
			if ((OpenTicarea() == 0))
			    CloseTicarea(TRUE);
		    }
		    break;
	    case 4: temp =  CFG.tic_groups;
		    temp = edit_int(10,18, temp, (char *)"Enter the maximum number of ^fileecho groups^ in the database.");
		    if (temp < CountFGroup()) {
			errmsg("You have %d groups defined", CountFGroup());
			show_int(10,18, CFG.tic_groups);
		    } else {
			CFG.tic_groups = temp;
			if ((OpenNoderec() == 0))
			    CloseNoderec(TRUE);
		    }
		    break;
	    case 5: E_INT( 11,18,    CFG.tic_dupes,    "Enter the maximum number of ^dupes^ in the dupe database.")
	    case 6: E_BOOL(12,18,    CFG.ct_KeepDate,  "^Keep^ original filedate on import")
	    case 7: E_BOOL(13,18,    CFG.ct_KeepMgr,   "Keep ^Areamgr^ netmails.")
	    case 8: E_BOOL( 7,58,    CFG.ct_PlusAll,   "Allow ^+%*^ (Plus all) in FileMgr requests.")
	    case 9: E_BOOL( 8,58,    CFG.ct_Notify,    "Allow turning ^Notify^ messages on or off.")
	    case 10:E_BOOL( 9,58,    CFG.ct_Passwd,    "Allow changing the AreaMgr/FileMgr ^password^.")
	    case 11:E_BOOL(10,58,    CFG.ct_Message,   "Allow turning FileMgr ^messages^ on or off.")
	    case 12:E_BOOL(11,58,    CFG.ct_TIC,       "Allow turning ^TIC^ files on or off.")
	    case 13:E_BOOL(12,58,    CFG.ct_Pause,     "Allow the ^Pause^ FileMgr command.")
	}
    }
}



void s_fidomailcfg(void);
void s_fidomailcfg(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 5, "1.11   EDIT FIDONET MAIL AND ECHOMAIL PROCESSING");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1. Badboard");
    mbse_mvprintw( 8, 2, "2. Dupeboard");
    mbse_mvprintw( 9, 2, "3. Pktdate");
    mbse_mvprintw(10, 2, "4. Max pkts.");
    mbse_mvprintw(11, 2, "5. Max arcs.");
    mbse_mvprintw(12, 2, "6. Keep days");
    mbse_mvprintw(13, 2, "7. Echo dupes");
    mbse_mvprintw(14, 2, "8. Reject old");
    mbse_mvprintw(15, 2, "9. Max msgs");
    mbse_mvprintw(16, 1, "10. Days old");
    mbse_mvprintw(17, 1, "11. Max systems");
    mbse_mvprintw(18, 1, "12. Max groups");
    
    mbse_mvprintw(12,42, "13. 4d address");
    mbse_mvprintw(13,42, "14. Split at");
    mbse_mvprintw(14,42, "15. Force at");
    mbse_mvprintw(15,42, "16. Allow +*");
    mbse_mvprintw(16,42, "17. Notify");
    mbse_mvprintw(17,42, "18. Passwd");
    mbse_mvprintw(18,42, "19. Pause");

    set_color(WHITE, BLACK);
    show_str( 7,16,64, CFG.badboard);
    show_str( 8,16,64, CFG.dupboard);
    show_str( 9,16,64, CFG.pktdate);
    show_int( 10,16, CFG.maxpktsize);
    show_int( 11,16, CFG.maxarcsize);
    show_int( 12,16, CFG.toss_days);
    show_int( 13,16, CFG.toss_dupes);
    show_int( 14,16, CFG.toss_old);
    show_int( 15,16, CFG.defmsgs);
    show_int( 16,16, CFG.defdays);
    show_int( 17,16, CFG.toss_systems);
    show_int( 18,16, CFG.toss_groups);
        
    show_bool(12,58, CFG.addr4d);
    show_int( 13,58, CFG.new_split);
    show_int( 14,58, CFG.new_force);
    show_bool(15,58, CFG.ca_PlusAll);
    show_bool(16,58, CFG.ca_Notify);
    show_bool(17,58, CFG.ca_Passwd);
    show_bool(18,58, CFG.ca_Pause);
}



void e_fidomailcfg(void)
{
    int	    temp;

    s_fidomailcfg();
    for (;;) { 
	switch(select_menu(19)) {
	    case 0: return;
	    case 1: E_JAM(  7,16,64, CFG.badboard,     "The path to the ^bad echomail^ board.")
	    case 2: E_JAM(  8,16,64, CFG.dupboard,     "The path to the ^dupe echomail^ board.")
	    case 3: E_STR(  9,16,64, CFG.pktdate,      "The filename and parameters to the ^pktdate^ program.")
	    case 4: E_INT( 10,16,    CFG.maxpktsize,   "The maximum size in KB for mail ^packets^, 0 if unlimited.")
	    case 5: E_INT( 11,16,    CFG.maxarcsize,   "The maximum size in KB for ^arcmail^ archives, 0 if unlimited.")
	    case 6: E_INT( 12,16,    CFG.toss_days,    "The number of ^days^ to keep mail on hold.")
	    case 7: E_INT( 13,16,    CFG.toss_dupes,   "The number of ^dupes^ to store in the echomail dupes database.")
	    case 8: E_INT( 14,16,    CFG.toss_old,     "^Reject^ mail older then days, 0 means never reject.")
	    case 9: E_INT( 15,16,    CFG.defmsgs,      "The default maximum number of ^messages^ in each mail area.")
	    case 10:E_INT( 16,16,    CFG.defdays,      "The default maximum ^age in days^ in each mail area.")
	    case 11:temp = CFG.toss_systems;
		    temp = edit_int(17,16, temp, (char *)"The maximum number of connected ^systems^ in the database.");
		    if (temp < CountNoderec()) {
			errmsg("You have %d nodes defined", CountNoderec());
			show_int( 17,16, CFG.toss_systems);
		    } else {
			Syslog('+', "Changing number of systems from %d to %d", CFG.toss_systems, temp);
			CFG.toss_systems = temp;
			if ((OpenMsgarea() == 0))
			    CloseMsgarea(TRUE);
		    }
		    break;
	    case 12:temp = CFG.toss_groups;
		    temp = edit_int(18,16, temp, (char *)"The maximum number of ^groups^ in the database.");
		    if (temp < CountMGroup()) {
			errmsg("You have %d groups defined", CountMGroup());
			show_int( 18,16, CFG.toss_groups);
		    } else {
			Syslog('+', "Changing number of groups from %d to %d", CFG.toss_groups, temp);
			CFG.toss_groups = temp;
			if ((OpenNoderec() == 0))
			    CloseNoderec(TRUE);
		    }
		    break;
	    case 13:E_BOOL(12,58, CFG.addr4d,            "Use ^4d^ addressing instead of ^5d^ addressing.")
	    case 14:E_IRC( 13,58, CFG.new_split, 12, 60, "Gently ^split^ newfiles reports after n kilobytes (12..60).")
	    case 15:E_IRC( 14,58, CFG.new_force, 16, 64, "Force ^split^ of newfiles reports after n kilobytes (16..64).")
	    case 16:E_BOOL(15,58, CFG.ca_PlusAll,        "Allow ^+%*^ (Plus all) in AreaMgr requests.")
	    case 17:E_BOOL(16,58, CFG.ca_Notify,         "Allow turning ^Notify^ messages on or off.")
	    case 18:E_BOOL(17,58, CFG.ca_Passwd,         "Allow changing the AreaMgr/FileMgr ^password^.")
	    case 19:E_BOOL(18,58, CFG.ca_Pause,          "Allow the ^Pause^ AreaMgr command.")
	}
    }
}



void s_intmailcfg(void);
void s_intmailcfg(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 5, "1.12   EDIT INTERNET MAIL AND NEWS PROCESSING");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1. POP3 node");
    mbse_mvprintw( 8, 2, "2. Usr@domain");
    mbse_mvprintw( 9, 2, "3. SMTP node");
    switch (CFG.newsfeed) {
	case FEEDINN:	mbse_mvprintw(10, 2, "4. N/A");
        		mbse_mvprintw(11, 2, "5. NNTP node");
			mbse_mvprintw(12, 2, "6. NNTP port");
			mbse_mvprintw(13, 2, "7. NNTP m.r.");
			mbse_mvprintw(14, 2, "8. NNTP user");
			mbse_mvprintw(15, 2, "9. NNTP pass");
			mbse_mvprintw(16, 1, "10. NNTP force");
			break;
	case FEEDRNEWS: mbse_mvprintw(10, 2, "4. Path rnews");
			mbse_mvprintw(11, 2, "5. N/A");
			mbse_mvprintw(12, 2, "6. N/A");
			mbse_mvprintw(13, 2, "7. N/A");
                        mbse_mvprintw(14, 2, "8. N/A");
                        mbse_mvprintw(15, 2, "9. N/A");
			mbse_mvprintw(16, 1, "10. N/A");
			break;
	case FEEDUUCP:	mbse_mvprintw(10, 2, "4. UUCP path");
			mbse_mvprintw(11, 2, "5. UUCP node");
                        mbse_mvprintw(12, 2, "6. N/A");
			mbse_mvprintw(13, 2, "7. N/A");
                        mbse_mvprintw(14, 2, "8. N/A");
                        mbse_mvprintw(15, 2, "9. N/A");
			mbse_mvprintw(16, 1, "10. N/A");
			break;
    }
    mbse_mvprintw(17, 1, "11. Email aka");
    mbse_mvprintw(18, 1, "12. UUCP aka");
    mbse_mvprintw(19, 1, "13. Emailmode");

    mbse_mvprintw(13,48, "14. News dupes");
    mbse_mvprintw(14,48, "15. Articles");
    mbse_mvprintw(15,48, "16. News mode");
    mbse_mvprintw(16,48, "17. Split at");
    mbse_mvprintw(17,48, "18. Force at");
    mbse_mvprintw(18,48, "19. Control ok");
    mbse_mvprintw(19,48, "20. No regate");

    set_color(WHITE, BLACK);
    show_str( 7,16,64, CFG.popnode);
    show_bool(8,16,    CFG.UsePopDomain);
    show_str( 9,16,64, CFG.smtpnode);
    switch (CFG.newsfeed) {
	case FEEDINN:	show_str(11,16,64, CFG.nntpnode);
			show_int(12,16,    CFG.nntpport);
			show_bool(13,16,   CFG.modereader);
			show_str(14,16,31, CFG.nntpuser);
			show_str(15,16,31, (char *)"*******************************");
			show_bool(16,16,   CFG.nntpforceauth);
			break;
	case FEEDRNEWS:	show_str(10,16,64, CFG.rnewspath);
			break;
	case FEEDUUCP:	show_str(10,16,64, CFG.rnewspath);
			show_str(11,16,64, CFG.nntpnode);
			break;
    }

    show_aka(17,16,    CFG.EmailFidoAka);
    show_aka(18,16,    CFG.UUCPgate);
    show_emailmode(19,16, CFG.EmailMode);

    show_int( 13,65, CFG.nntpdupes);
    show_int( 14,65, CFG.maxarticles);
    show_newsmode(15,65, CFG.newsfeed);
    show_int( 16,65, CFG.new_split);
    show_int( 17,65, CFG.new_force);
    show_bool(18,65, CFG.allowcontrol);
    show_bool(19,65, CFG.dontregate);
}



/*
 * Edit UUCP gateway, return -1 if there are errors, 0 if ok.
 */
void e_uucp(void)
{
    int j;

    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "1.12  EDIT UUCP GATEWAY");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.    Zone");
    mbse_mvprintw( 8, 6, "2.    Net");
    mbse_mvprintw( 9, 6, "3.    Node");
    mbse_mvprintw(10, 6, "4.    Point");
    mbse_mvprintw(11, 6, "5.    Domain");

    for (;;) {
        set_color(WHITE, BLACK);
        show_int( 7,19,   CFG.UUCPgate.zone);
        show_int( 8,19,   CFG.UUCPgate.net);
        show_int( 9,19,   CFG.UUCPgate.node);
        show_int(10,19,   CFG.UUCPgate.point);
        show_str(11,19,8, CFG.UUCPgate.domain);

        j = select_menu(5);
        switch(j) {
            case 0: return;
            case 1: E_IRC(  7,19,   CFG.UUCPgate.zone, 1, 32767,  "The ^zone^ number for the UUCP gateway (1..32767)")
            case 2: E_IRC(  8,19,   CFG.UUCPgate.net, 0, 32767,   "The ^Net^ number for the UUCP gateway (0..32767)")
            case 3: E_IRC(  9,19,   CFG.UUCPgate.node, 0, 32767,  "The ^Node^ number for the UUCP gateway (0..32767)")
            case 4: E_IRC( 10,19,   CFG.UUCPgate.point, 0, 32767, "The ^Point^ number for the UUCP gateway (0..32767)")
            case 5: E_STR( 11,19,8, CFG.UUCPgate.domain,          "The ^FTN Domain^ for the UUCP gateway without a dot")
        }
    }
}



void e_intmailcfg(void)
{
    int     tmp;

    s_intmailcfg();
    for (;;) {
	switch(select_menu(20)) {
	    case 0: return;
	    case 1: E_STR(  7,16,64, CFG.popnode,      "The ^FQDN^ of the node where the ^POP3^ server runs.")
	    case 2: E_BOOL( 8,16,    CFG.UsePopDomain, "Use ^user@maildomain^ to login the POP3 server.")
	    case 3: E_STR(  9,16,64, CFG.smtpnode,     "The ^FQDN^ of the node where the ^SMTP^ server runs.")
	    case 4: if (CFG.newsfeed == FEEDRNEWS)
			strcpy(CFG.rnewspath, edit_pth(10,16,64, CFG.rnewspath, (char *)"The path and filename to the ^rnews^ command.", 0775));
		    if (CFG.newsfeed == FEEDUUCP)
			strcpy(CFG.rnewspath, edit_pth(10,16,64, CFG.rnewspath, (char *)"The path to the ^uucppublic^ directory.", 0775));
		    break;
            case 5: if (CFG.newsfeed == FEEDINN)
			strcpy(CFG.nntpnode, edit_str(11,16,64, CFG.nntpnode, (char *)"The ^FQDN^ of the node where the ^NNTP^ server runs."));
		    if (CFG.newsfeed == FEEDUUCP)
			strcpy(CFG.nntpnode, edit_str(11,16,64, CFG.nntpnode, (char *)"The ^UUCP^ nodename of the remote UUCP system"));
		    break;
	    case 6: if (CFG.newsfeed == FEEDINN)
			CFG.nntpport = edit_int(12,16, CFG.nntpport, (char *)"The NNTP ^port^ number to connect to");
		    if (CFG.nntpport == 0) {
			CFG.nntpport = 119;
			s_intmailcfg();
		    }
		    break;
            case 7: if (CFG.newsfeed == FEEDINN)
			CFG.modereader = edit_bool(13,16,    CFG.modereader, (char *)"Does the NNTP server needs the ^Mode Reader^ command.");
		    break;
            case 8: if (CFG.newsfeed == FEEDINN)
			strcpy(CFG.nntpuser, edit_str( 14,16,31, CFG.nntpuser, (char *)"The ^Username^ for the NNTP server if needed."));
		    break;
            case 9: if (CFG.newsfeed == FEEDINN) {
			strcpy(CFG.nntppass, edit_str(15,16,31, CFG.nntppass, (char *)"The ^Password^ for the NNTP server if needed."));
			s_intmailcfg();
		    }
		    break;
	    case 10:if (CFG.newsfeed == FEEDINN)
			CFG.nntpforceauth = edit_bool(16,16, CFG.nntpforceauth, (char *)"Force ^authentication^ on connect to the news server");
		    break;
	    case 11:tmp = PickAka((char *)"1.12.11", FALSE);
                    if (tmp != -1)
                                CFG.EmailFidoAka = CFG.aka[tmp];
                    s_intmailcfg();
                    break;
            case 12:e_uucp();
                    s_intmailcfg();
                    break;
            case 13:CFG.EmailMode = edit_emailmode(19,16, CFG.EmailMode);
                    s_intmailcfg();
                    break;

	    case 14:E_INT( 13,65, CFG.nntpdupes,      "The number of ^dupes^ to store in the news articles dupes database.")
	    case 15:E_INT( 14,65, CFG.maxarticles,    "Default maximum ^news articles^ to fetch")
	    case 16:CFG.newsfeed = edit_newsmode(15,65, CFG.newsfeed);
		    s_intmailcfg();
		    break;
            case 17:E_IRC( 16,65, CFG.new_split, 12, 60, "Gently ^split^ messages after n kilobytes (12..60).")
            case 18:E_IRC( 17,65, CFG.new_force, 16, 64, "Force ^split^ of messages after n kilobytes (16..64).")
            case 19:E_BOOL(18,65, CFG.allowcontrol,      "^Allow control^ messages for news to be gated.")
            case 20:E_BOOL(19,65, CFG.dontregate,        "Don't ^regate^ already gated messages.")
	}
    }
}



void s_newfiles(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "1.13 ALLFILES & NEWFILES LISTINGS");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.   New days");
    mbse_mvprintw( 8, 2, "2.   Security");
    mbse_mvprintw( 9, 2, "3.   Groups");
    mbse_mvprintw(10, 2, "4.   WWW log");
    mbse_mvprintw(11, 2, "5.   FTP log");
}



void e_newfiles(void)
{
    int	    temp;
    char    *logfile;
	
    s_newfiles();
    for (;;) {
	set_color(WHITE, BLACK);
	show_int( 7,16,    CFG.newdays);
	show_sec( 8,16,    CFG.security);
	show_int( 9,16,    CFG.new_groups);
	show_str(10,16,64, CFG.www_logfile);
	show_str(11,16,64, CFG.ftp_logfile);

	switch(select_menu(5)) {
	    case 0: return;
	    case 1: E_INT(7,16,    CFG.newdays,    "Add files younger than this in newfiles report.")
	    case 2: E_SEC(8,16,    CFG.security,   "1.13  NEWFILES REPORTS SECURITY", s_newfiles)
	    case 3: temp = CFG.new_groups;
		    temp = edit_int( 9, 16, temp, (char *)"The maximum of ^newfiles^ groups in the newfiles database");
		    if (temp < CountNewfiles()) {
			errmsg("You have %d newfiles reports defined", CountNewfiles());
			show_int( 9,16, CFG.new_groups);
		    } else {
			CFG.new_groups = temp;
			if (OpenNewfiles() == 0)
			    CloseNewfiles(TRUE);
		    }
		    break;
	    case 4: logfile = calloc(81, sizeof(char));
		    strcpy(logfile, edit_str(10,16,64, CFG.www_logfile, 
				(char *)"The name of the ^apache logfile^ in common format"));
		    if (strlen(logfile)) {
			if (file_exist(logfile, R_OK)) {
			    errmsg("Logfile \"%s\" doesn't exist", logfile);
			} else {
			    snprintf(CFG.www_logfile, 81, "%s", logfile);
			}
		    } else {
			CFG.www_logfile[0] = '\0';
		    }
		    free(logfile);
		    break;
	    case 5: logfile = calloc(81, sizeof(char));
		    strcpy(logfile, edit_str(11,16,64, CFG.ftp_logfile,
				    (char *)"The name of the ^ftp server logfile^ in xferlog format"));
		    if (strlen(logfile)) {
			if (file_exist(logfile, R_OK)) {
			    errmsg("Logfile \"%s\" doesn't exist", logfile);
			} else {
			    snprintf(CFG.ftp_logfile, 81, "%s", logfile);
			}
		    } else {
			CFG.ftp_logfile[0] = '\0';
		    }
		    free(logfile);
		    break;
	}
    }
}



/*
 * Edit one aka, return -1 if there are errors, 0 if ok.
 */
void e_aka(int Area)
{
    int	j;

    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "1.1   EDIT AKA");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.    Zone");
    mbse_mvprintw( 8, 6, "2.    Net");
    mbse_mvprintw( 9, 6, "3.    Node");
    mbse_mvprintw(10, 6, "4.    Point");
    mbse_mvprintw(11, 6, "5.    Domain");
    mbse_mvprintw(12, 6, "6.    Active");

    for (;;) {
	set_color(WHITE, BLACK);
	show_int( 7,19,   CFG.aka[Area].zone);
	show_int( 8,19,   CFG.aka[Area].net);
	show_int( 9,19,   CFG.aka[Area].node);
	show_int(10,19,   CFG.aka[Area].point);
	show_str(11,19,8, CFG.aka[Area].domain);
	show_bool(12,19,  CFG.akavalid[Area]);

	j = select_menu(6);
	switch(j) {
	    case 0: return;
	    case 1: E_IRC(  7,19,   CFG.aka[Area].zone, 0, 32767,  "The ^zone^ number for this aka (1..32767)")
	    case 2: E_IRC(  8,19,   CFG.aka[Area].net, 0, 32767,   "The ^Net^ number for this aka (0..32767)")
	    case 3: E_IRC(  9,19,   CFG.aka[Area].node, 0, 32767,  "The ^Node^ number for this aka (0..32767)")
	    case 4: E_IRC( 10,19,   CFG.aka[Area].point, 0, 32767, "The ^Point^ number for this node (0..32767)")
	    case 5: E_STR( 11,19,8, CFG.aka[Area].domain,          "The ^FTN Domain^ for this aka without a dot (ie no .org)")
	    case 6: E_BOOL(12,19,   CFG.akavalid[Area],            "Is this aka ^available^")
	}
    }
}



void e_fidoakas(void)
{
    int		i, j, k, x, y, o = 0, error, from, too;
    char	pick[12];
    char	temp[121];

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "1.1   EDIT FIDONET AKA'S");
	set_color(CYAN, BLACK);
	x = 2;
	y = 7;
	set_color(CYAN, BLACK);
	for (i = 1; i <= 20; i++) {
	    if (i == 11) {
		x = 42;
		y = 7;
	    }
	    if ((o + i) <= 40) {
		if (CFG.akavalid[o+i-1])
		    set_color(CYAN, BLACK);
		else
		    set_color(LIGHTBLUE, BLACK);
		if (CFG.akavalid[o+i-1]) {
		    snprintf(temp, 81, "%3d   %s", o+i, aka2str(CFG.aka[o+i-1]));
		    temp[38] = '\0';
		} else
		    snprintf(temp, 81, "%3d", o+i);
		mbse_mvprintw(y, x, temp);
		y++;
	    }
	}
	strcpy(pick, select_aka(40, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    error = FALSE;
	    /*
	     * Various checks on the system aka's.
	     */
	    if ((! CFG.aka[0].zone) && (! CFG.akavalid[0])) {
		errmsg("First aka (main aka) must be valid");
		error = TRUE;
	    }
	    if (error == FALSE) {
		/*
		 * Check if aka's are in one continues block
		 */
		k = 0;
		for (j = 0; j < 40; j++)
		    if (CFG.akavalid[j] && CFG.aka[j].zone)
			k++;
		for (j = k; j < 40; j++)
		    if (CFG.akavalid[j] || CFG.aka[j].zone)
			error = TRUE;
		if (error)
		    errmsg("All aka's must be in one continues block");
	    }
	    if (error == FALSE) {
		for (j = 0; j < 40; j++) {
		    if (CFG.akavalid[j] && CFG.aka[j].zone && (strlen(CFG.aka[j].domain) == 0)) {
			error = TRUE;
			errmsg("Aka %d has no domain set", j+1);
		    }
		}
	    }
	    if (! error)
		return;
	}

	if (strncmp(pick, "N", 1) == 0)
	    if ((o + 20) < 40)
		o = o + 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o = o - 20;

	if (strncmp(pick, "M", 1) == 0) {
	    from = too = 0;
	    mbse_mvprintw(LINES -3, 6, "Enter aka number (1..40) to move >");
	    from = edit_int(LINES -3, 42, from, (char *)"Enter record number");
	    mbse_locate(LINES -3, 6);
	    clrtoeol();
	    mbse_mvprintw(LINES -3, 6, "Enter new position (1..40) >");
	    too = edit_int(LINES -3, 36, too, (char *)"Enter destination record number");
	    if ((from == too) || (from == 0) || (too == 0) || (from > 40) || (too > 40)) {
		errmsg("That makes no sense");
	    } else if (CFG.akavalid[from - 1] == FALSE) {
		errmsg("Origin aka is invalid");
	    } else if (CFG.akavalid[too - 1]) {
		errmsg("Destination record is in use");
	    } else if (yes_no((char *)"Proceed move")) {
		CFG.aka[too -1].zone   = CFG.aka[from -1].zone;
		CFG.aka[too -1].net    = CFG.aka[from -1].net;
		CFG.aka[too -1].node   = CFG.aka[from -1].node;
		CFG.aka[too -1].point  = CFG.aka[from -1].point;
		strcpy(CFG.aka[too -1].domain, CFG.aka[from -1].domain);
		CFG.akavalid[too -1] = TRUE;
		CFG.akavalid[from -1] = FALSE;
		memset(&CFG.aka[from -1], 0, sizeof(fidoaddr));
	    }
	}

	if ((atoi(pick) >= 1) && (atoi(pick) <= 40))
	    e_aka(atoi(pick)-1);
    }
}



void s_mailer(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "1.14 EDIT MAILER SETTINGS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.   Mailer logl.");
	mbse_mvprintw( 8, 2, "2.   TCP/IP \"phone\"");
	mbse_mvprintw( 9, 2, "3.   TCP/IP flags");
	mbse_mvprintw(10, 2, "4.   TCP/IP speed");
	mbse_mvprintw(11, 2, "5.   Timeout reset");
	mbse_mvprintw(12, 2, "6.   Timeout connect");
	mbse_mvprintw(13, 2, "7.   Dial delay");
	mbse_mvprintw(14, 2, "8.   No Filerequests");
	mbse_mvprintw(15, 2, "9.   No callout");
	mbse_mvprintw(16, 2, "10.  No EMSI session");
	mbse_mvprintw(17, 2, "11.  No Yooho/2U2");

	mbse_mvprintw(13,31, "12.  No Zmodem");
	mbse_mvprintw(14,31, "13.  No Zedzap");
	mbse_mvprintw(15,31, "14.  No Hydra");
	mbse_mvprintw(16,31, "15.  No MD5");
	mbse_mvprintw(17,31, "16.  Zero Locks OK");

	mbse_mvprintw(12,59, "17.  Phonetrans  1-10");
	mbse_mvprintw(13,59, "18.  Phonetrans 11-20");
	mbse_mvprintw(14,59, "19.  Phonetrans 21-30");
	mbse_mvprintw(15,59, "20.  Phonetrans 31-40");
	mbse_mvprintw(16,59, "21.  Max. files");
	mbse_mvprintw(17,59, "22.  Max. MB.");
}



void e_trans(int start, int item)
{
	int	i, j;
	char	temp[21];

	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "1.14.%d EDIT PHONE TRANSLATION", item);
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 12, "String to match       String to replace");
	for (i = 0; i < 10; i++) {
		snprintf(temp, 21, "%2d.", i+1);
		mbse_mvprintw( 9+i, 6, temp);
	}
	for (;;) {
		set_color(WHITE, BLACK);
		for (i = 0; i < 10; i++) {
			show_str( 9+i,12,20,CFG.phonetrans[i+start].match);
			show_str( 9+i,34,20,CFG.phonetrans[i+start].repl);
		}

		j = select_menu(10);
		if (j == 0) {
			s_mailer();
			return;
		}
		strcpy(CFG.phonetrans[j+start-1].match, edit_str(j+8,12,20, CFG.phonetrans[j+start-1].match, (char *)"The phone entry to ^match^"));
		strcpy(CFG.phonetrans[j+start-1].repl,  edit_str(j+8,34,20, CFG.phonetrans[j+start-1].repl,  (char *)"The phone string to ^replace^"));
	}
}



void e_mailer(void)
{
    s_mailer();
    for (;;) {
	set_color(WHITE, BLACK);
	show_logl( 7,23,   CFG.cico_loglevel);
	show_str(  8,23,20,CFG.IP_Phone);
	show_str(  9,23,30,CFG.IP_Flags);
	show_uint(10,23,   CFG.IP_Speed);
	show_int( 11,23,   CFG.timeoutreset);
	show_int( 12,23,   CFG.timeoutconnect);
	show_int( 13,23,   CFG.dialdelay);
	show_bool(14,23,   CFG.NoFreqs);
	show_bool(15,23,   CFG.NoCall);
	show_bool(16,23,   CFG.NoEMSI);
	show_bool(17,23,   CFG.NoWazoo);

	show_bool(13,52, CFG.NoZmodem);
	show_bool(14,52, CFG.NoZedzap);
	show_bool(15,52, CFG.NoHydra);
	show_bool(16,52, CFG.NoMD5);
	show_bool(17,52, CFG.ZeroLocks);

	show_int( 16,75, CFG.Req_Files);
	show_int( 17,75, CFG.Req_MBytes);

	switch(select_menu(22)) {
	    case 0: return;
	    case 1: E_LOGL(CFG.cico_loglevel, "1.14.1", s_mailer)
	    case 2: E_STR(  8,23,20,CFG.IP_Phone,       "The mailer ^TCP/IP \"phone\" number^ for this system, empty is no TCP/IP")
	    case 3: E_STR(  9,23,30,CFG.IP_Flags,       "The mailer ^TCP/IP capability flags^ for this system")
	    case 4: E_UINT(10,23,   CFG.IP_Speed,       "The mailer ^TCP/IP linespeed^ for this system")
	    case 5: E_INT( 11,23,   CFG.timeoutreset,   "The modem ^reset timeout^ in seconds")
	    case 6: E_INT( 12,23,   CFG.timeoutconnect, "The modem ^wait for connect timeout^ in seconds")
	    case 7: E_INT( 13,23,   CFG.dialdelay,      "The ^random dialdelay^ in seconds ((^n^ <= delay) and (^n^ > (delay / 10)))")
	    case 8: E_BOOL(14,23,   CFG.NoFreqs,        "Set to true if ^No Filerequests^ are allowed")
	    case 9: E_BOOL(15,23,   CFG.NoCall,         "Set to true if ^No Calls^ are allowed")
	    case 10:E_BOOL(16,23,   CFG.NoEMSI,         "If set then ^EMSI handshake^ is diabled")
	    case 11:E_BOOL(17,23,   CFG.NoWazoo,        "If set then ^YooHoo/2U2^ (FTSC-0006) is disabled")

	    case 12:E_BOOL(13,52,   CFG.NoZmodem,       "If set then the ^Zmodem^ protocol is disabled")
	    case 13:E_BOOL(14,52,   CFG.NoZedzap,       "If set then the ^Zedzap^ protocol is disabled")
	    case 14:E_BOOL(15,52,   CFG.NoHydra,        "If set then the ^Hydra^ protocol is disabled")
	    case 15:E_BOOL(16,52,   CFG.NoMD5,          "Disable ^MD5 crypted^ passwords with binkp sessions")
	    case 16:E_BOOL(17,52,   CFG.ZeroLocks,	"Allow ^zero byte node lockfiles^ created by another OS")

	    case 17:e_trans(0, 17);  break;
	    case 18:e_trans(10, 18); break;
	    case 19:e_trans(20, 19); break;
	    case 20:e_trans(30, 20); break;
	    case 21:E_INT(16,75,    CFG.Req_Files,       "Maximum ^files^ to request, 0 is unlimited")
	    case 22:E_INT(17,75,    CFG.Req_MBytes,      "Maximum ^MBytes^ to request, 0 is unlimited")
	}
    }
}



void e_html(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "1.15 EDIT HTML SETTINGS");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.  Docs root");
    mbse_mvprintw( 8, 2, "2.  Link to ftp");
    mbse_mvprintw( 9, 2, "3.  URL name");
    mbse_mvprintw(10, 2, "4.  Charset");
    mbse_mvprintw(11, 2, "5.  Author name");
    mbse_mvprintw(12, 2, "6.  Convert cmd");
    mbse_mvprintw(13, 2, "7.  Files/page");
    mbse_mvprintw(14, 2, "8.  Hist. limit");

    set_color(WHITE, BLACK);
    show_str( 7,18,59, CFG.www_root);
    show_str( 8,18,20, CFG.www_link2ftp);
    show_str( 9,18,40, CFG.www_url);
    show_str(10,18,20, CFG.www_charset);
    show_str(11,18,40, CFG.www_author);
    show_str(12,18,59, CFG.www_convert);
    show_int(13,18,    CFG.www_files_page);
    show_int(14,18,    CFG.www_mailerlines);

    for (;;) {
        set_color(WHITE, BLACK);

        switch(select_menu(8)) {
            case 0: return;
            case 1: E_STR( 7,18,59, CFG.www_root,        "The ^Document root^ of your http server")
            case 2: E_STR( 8,18,20, CFG.www_link2ftp,    "The ^link^ name from the Document root to the FTP base directory")
            case 3: E_STR( 9,18,40, CFG.www_url,         "The ^URL^ name of your http server")
            case 4: E_STR(10,18,20, CFG.www_charset,     "The ^ISO character set^ name to include in the web pages")
            case 5: E_STR(11,18,40, CFG.www_author,      "The ^Author name^ to include in the http headers")
            case 6: E_STR(12,18,59, CFG.www_convert,     "The ^convert^ command to create thumbnails")
	    case 7: E_INT(13,18,    CFG.www_files_page,  "The number of files on each web page")
	    case 8: E_INT(14,18,    CFG.www_mailerlines, "Limit the number of ^mailer history^ lines, 0 is unlimited")
        }
    }
}



void global_menu(void)
{
    unsigned int    crc, crc1;
    int		    i;
    char	    *temp;

    if (! check_free())
	return;

    if (cf_open() == -1)
	return;

    Syslog('+', "Opened main config");
    crc = 0xffffffff;
    crc = upd_crc32((char *)&CFG, crc, sizeof(CFG));

    if (strlen(CFG.out_queue) == 0) {
	snprintf(CFG.out_queue, 65, "%s/var/queue", getenv("MBSE_ROOT"));
	Syslog('+', "Main config, upgraded for new queue");
    }

    if (strlen(CFG.mgrlog) == 0) {
	snprintf(CFG.mgrlog, 15, "manager.log");
	for (i = 0; i < 32; i++)
	    snprintf(CFG.aname[i], 17, "Flags %d", i+1);
	snprintf(CFG.aname[0], 17, "Everyone");
	Syslog('+', "Main config, upgraded for manager security");
    }

    if (strlen(CFG.debuglog) == 0) {
	snprintf(CFG.debuglog, 15, "debug.log");
	Syslog('+', "Main config, upgraded for new debug logfile");
    }

    if (!CFG.ca_PlusAll && !CFG.ca_Notify && !CFG.ca_Passwd && !CFG.ca_Pause && !CFG.ca_Check) {
	CFG.ca_PlusAll = TRUE;
	CFG.ca_Notify  = TRUE;
	CFG.ca_Passwd  = TRUE;
	CFG.ca_Pause   = TRUE;
	CFG.ca_Check   = TRUE;
	Syslog('+', "Main config, upgraded for AreaMgr flags");
    }

    if (strlen(CFG.rulesdir) == 0) {
	snprintf(CFG.rulesdir, 65, "%s/var/rules", getenv("MBSE_ROOT"));
	Syslog('+', "Main config, upgraded rules directory");
    }

    if (!strlen(CFG.www_convert) && strlen(_PATH_CONVERT)) {
	snprintf(CFG.www_convert, 81, "%s -geometry x100", _PATH_CONVERT);
	Syslog('+', "Main config, installed convert for thumbnails");
    }

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/magic", getenv("MBSE_ROOT"));
    if (strcmp(CFG.req_magic, temp) == 0) {
	snprintf(CFG.req_magic, 65, "%s/var/magic", getenv("MBSE_ROOT"));
	Syslog('+', "Main config, magic dir moved to %s", CFG.req_magic);
    }
    free(temp);
    
    if (strlen(CFG.deflang) == 0) {
	snprintf(CFG.deflang, 10, "%s", (char *)"en");
	Syslog('+', "Main config, upgraded default language to \"en\"");
    }

    if (!CFG.is_upgraded) {
	CFG.priority = 15;
#ifdef __linux__
	CFG.do_sync = TRUE;
#endif
	CFG.is_upgraded = TRUE;
	Syslog('+', "Main config, upgraded execute settings");
    }

    if (strlen(CFG.xnntpuser) && ! strlen(CFG.nntpuser)) {
	Syslog('+', "Main config, nntp username length increased");
	strncpy(CFG.nntpuser, CFG.xnntpuser, 16);
	memset(&CFG.xnntpuser, 0, sizeof(CFG.xnntpuser));
    }

    if (strlen(CFG.xnntppass) && ! strlen(CFG.nntppass)) {
	Syslog('+', "Main config, nntp password length increased");
	strncpy(CFG.nntppass, CFG.xnntppass, 16);
	memset(&CFG.xnntppass, 0, sizeof(CFG.xnntppass));
    }

    if (! CFG.nntpport) {
	Syslog('+', "Main config, set default nntp port 119");
	CFG.nntpport = 119;
    }

    for (;;) {

	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "1.    GLOBAL SETUP");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 6, "1.    Edit Fidonet Aka's");
	mbse_mvprintw( 8, 6, "2.    Edit Registration Info");
	mbse_mvprintw( 9, 6, "3.    Edit Global Filenames");
	mbse_mvprintw(10, 6, "4.    Edit Global Paths");
	mbse_mvprintw(11, 6, "5.    Edit Global Settings");
	mbse_mvprintw(12, 6, "6.    Edit User flag Descriptions");
	mbse_mvprintw(13, 6, "7.    Edit New Users defaults");
	mbse_mvprintw(14, 6, "8.    Edit Text Colors");

	mbse_mvprintw( 7,46, "9.    Edit Sysop Paging");
	mbse_mvprintw( 8,46, "10.   Edit Files Processing");
	mbse_mvprintw( 9,46, "11.   Edit Fidonet Mail/Echomail");
	mbse_mvprintw(10,46, "12.   Edit Internet Mail/News");
	mbse_mvprintw(11,46, "13.   Edit All-/Newfiles lists");
	mbse_mvprintw(12,46, "14.   Edit Mailer global setup");
	mbse_mvprintw(13,46, "15.   Edit HTML pages setup");
	mbse_mvprintw(14,46, "16.   Edit Mgr flag descriptions");

	switch(select_menu(16)) {
	    case 0:
		    crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&CFG, crc1, sizeof(CFG));
		    if (crc != crc1) {
			if (yes_no((char *)"Configuration is changed, save") == 1) {
			    cf_close();
			    disk_reset();
			    Syslog('+', "Saved main config");
			    working(6, 0, 0);
			}
		    }
		    open_bbs();
		    return;
            case 1: 
		    e_fidoakas();
		    break;
	    case 2:
		    e_reginfo();
		    break;
	    case 3:
		    e_filenames();
		    break;
	    case 4:
		    e_global();
		    break;
	    case 5:
		    e_bbsglob();
		    break;
            case 6: 
		    e_flags(TRUE);
		    break;
	    case 7:
		    e_newuser();
		    break;
	    case 8:
		    e_colors();
		    break;
	    case 9:
		    e_paging();
		    break;
	    case 10:
		    e_ticconf();
		    break;
	    case 11:
		    e_fidomailcfg();
		    break;
	    case 12:
		    e_intmailcfg();
		    break;
	    case 13:
		    e_newfiles();
		    break;
	    case 14:
		    e_mailer();
		    break;
	    case 15:
		    e_html();
		    break;
	    case 16:
		    e_flags(FALSE);
		    break;
	}
    }
}



int PickAka(char *msg, int openit)
{
	char		temp[81];
	static char	pick[12];
	int		i, o = 0, x, y;

	if (openit) {
		if (cf_open() == -1)
			return -1;
		cf_close();
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.   AKA SELECT", msg);
		mbse_mvprintw( 5, 4, temp);	
		set_color(CYAN, BLACK);
		x = 2;
		y = 7;
		for (i = 1; i <= 20; i++) {
			if (i == 11) {
				x = 42;
				y = 7;
			}
			if ((o + i) <= 40) {
				if (CFG.akavalid[o+i-1]) {
					set_color(CYAN, BLACK);
					snprintf(temp, 81, "%3d   %s", o+i, aka2str(CFG.aka[o+i-1]));
					temp[38] = '\0';
				} else {
					set_color(LIGHTBLUE, BLACK);
					snprintf(temp, 81, "%3d", o+i);
				}
				mbse_mvprintw(y, x, temp);
				y++;
			}
		}
		strcpy(pick, select_pick(40, 20));
		
		if (strncmp(pick, "-", 1) == 0)
			return -1;

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < 40)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= 40) && (CFG.akavalid[atoi(pick)-1]))
			return (atoi(pick) -1);
	}
}



void web_secflags(FILE *fp, char *name, securityrec sec)
{
    int	    i;

    fprintf(fp, "<TR><TH align='left'>%s</TH><TD>%d (%s)</TD></TR>\n", name, sec.level, get_limit_name(sec.level));
    for (i = 0; i < 32; i++) {
	if ((sec.flags >> i) & 1) {
	    fprintf(fp, "<TR><TH>&nbsp;</TH><TD>.and. bit %d (%s)</TD></TR>\n", i, CFG.fname[i]);
	} else if ((sec.notflags >> i) & 1) {
	    fprintf(fp, "<TR><TH>&nbsp;</TH><TD>.and not. bit %d (%s)</TD></TR>\n", i, CFG.fname[i]);
	}
    }
}



int global_doc(FILE *fp, FILE *toc, int page)
{
    int		    i, j;
    struct utsname  utsbuf;
    time_t	    now;
    char	    *p, temp[1024];
    FILE	    *wp;
    
    if (config_read())
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 0, 0, page, (char *)"System information");
    addtoc(fp, toc, 0, 1, page, (char *)"System information");

    wp = open_webdoc((char *)"global.html", (char *)"Global Configuration", NULL);
    fprintf(wp, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(wp, "<UL>\n");
    fprintf(wp, " <LI><A HREF=\"#_host\">Host System Information</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_akas\">System fidonet addresses</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_reginfo\">Registration information</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_filenames\">Global filenames</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_pathnames\">Pathnames</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_global\">Global settings</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_security\">Users flag descriptions</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_newusers\">New users defaults</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_colors\">Text colors</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_paging\">Sysop paging</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_fileecho\">Fileecho processing</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_fidomail\">Fidonet Mail and Echomail processing</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_inetmail\">Internet Mail and News processing</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_newfiles\">Newfiles reports</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_mailer\">Mailer setup</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_www\">WWW server setup</A></LI>\n");
    fprintf(wp, " <LI><A HREF=\"#_manager\">Manager flag descriptions</A></LI>\n");
    fprintf(wp, "</UL>\n");
    fprintf(wp, "<HR>\n");

    fprintf(wp, "<A NAME=\"_host\"></A><H3>Host System Information</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    memset(&utsbuf, 0, sizeof(utsbuf));
    if (uname(&utsbuf) == 0) {
	add_webtable(wp, (char *)"Node name", utsbuf.nodename);
	fprintf(fp, "      Node name        %s\n", utsbuf.nodename);
#if defined(__USE_GNU)
	add_webtable(wp, (char *)"Domain name", utsbuf.domainname);
	fprintf(fp, "      Domain name      %s\n", utsbuf.domainname);
#elif defined(__linux__)
	add_webtable(wp, (char *)"Domain name", utsbuf.__domainname);
	fprintf(fp, "      Domain name      %s\n", utsbuf.__domainname);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	/* No domainname in struct utsname */
#else
#error "Don't know utsbuf.domainname on this OS"
#endif
	snprintf(temp, 81, "%s %s", utsbuf.sysname, utsbuf.release);
	add_webtable(wp, (char *)"Operating system", temp);
	fprintf(fp, "      Operating system %s %s\n", utsbuf.sysname, utsbuf.release);
	add_webtable(wp, (char *)"Kernel version", utsbuf.version);
	fprintf(fp, "      Kernel version   %s\n", utsbuf.version);
	add_webtable(wp, (char *)"Machine type", utsbuf.machine);
	fprintf(fp, "      Machine type     %s\n", utsbuf.machine);
    }
    add_webtable(wp, (char *)"MBSE_ROOT", getenv("MBSE_ROOT"));
    fprintf(fp, "      MBSE_ROOT        %s\n", getenv("MBSE_ROOT"));
    now = time(NULL);
    add_webtable(wp, (char *)"Date created", ctime(&now));
    fprintf(fp, "      Date created     %s", ctime(&now));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
 
    addtoc(fp, toc, 1, 0, page, (char *)"Global system setup");
    addtoc(fp, toc, 1, 1, page, (char *)"System fidonet addresses");

    fprintf(wp, "<A NAME=\"_akas\"></A><H3>System fidonet addresses</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    for (i = 0; i < 40; i++) {
	if (CFG.akavalid[i]) {
	    fprintf(fp, "      Aka %2d    %s\n", i+1, aka2str(CFG.aka[i]));
	    snprintf(temp, 81, "Aka %d", i+1);
	    add_webtable(wp, temp, aka2str(CFG.aka[i]));
	}
    }
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);

    fprintf(wp, "<A NAME=\"_reginfo\"></A><H3>Registration information</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"System name", CFG.bbs_name);
    add_webtable(wp, (char *)"Mail domain", CFG.sysdomain);
    add_webtable(wp, (char *)"My FQDN", CFG.myfqdn);
    add_webtable(wp, (char *)"Sysop unix name", CFG.sysop);
    add_webtable(wp, (char *)"Sysop fido name", CFG.sysop_name);
    add_webtable(wp, (char *)"System location", CFG.location);
    add_webtable(wp, (char *)"QWK/Bluewave id", CFG.bbsid);
    add_webtable(wp, (char *)"Comment", CFG.comment);
    add_webtable(wp, (char *)"Origin line", CFG.origin);
    add_webtable(wp, (char *)"Start unix name", CFG.startname);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    addtoc(fp, toc, 1, 2, page, (char *)"Registration information");
    fprintf(fp, "      System name      %s\n", CFG.bbs_name);
    fprintf(fp, "      Mail domain      %s\n", CFG.sysdomain);
    fprintf(fp, "      My FQDN          %s\n", CFG.myfqdn);
    fprintf(fp, "      Sysop unix name  %s\n", CFG.sysop);
    fprintf(fp, "      Sysop fido name  %s\n", CFG.sysop_name);
    fprintf(fp, "      System location  %s\n", CFG.location);
    fprintf(fp, "      QWK/Bluewave id  %s\n", CFG.bbsid);
    fprintf(fp, "      Comment          %s\n", CFG.comment);
    fprintf(fp, "      Origin line      %s\n", CFG.origin);
    fprintf(fp, "      Start unix name  %s\n", CFG.startname);

    fprintf(wp, "<A NAME=\"_filenames\"></A><H3>Global filenames</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"System logfile", CFG.logfile);
    add_webtable(wp, (char *)"Error logfile", CFG.error_log);
    add_webtable(wp, (char *)"Debug logfile", CFG.debuglog);
    add_webtable(wp, (char *)"Manager logfile", CFG.mgrlog);
    add_webtable(wp, (char *)"Default menu", CFG.default_menu);
    add_webtable(wp, (char *)"Chat logfile", CFG.chat_log);
    add_webtable(wp, (char *)"Welcome logo", CFG.welcome_logo);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    addtoc(fp, toc, 1, 3, page, (char *)"Global filenames");
    fprintf(fp, "      System logfile   %s\n", CFG.logfile);
    fprintf(fp, "      Error logfile    %s\n", CFG.error_log);
    fprintf(fp, "      Debug logfile    %s\n", CFG.debuglog);
    fprintf(fp, "      Manager logfile  %s\n", CFG.mgrlog);
    fprintf(fp, "      Default menu     %s\n", CFG.default_menu);
    fprintf(fp, "      Chat logfile     %s\n", CFG.chat_log);
    fprintf(fp, "      Welcome logo     %s\n", CFG.welcome_logo);

    fprintf(wp, "<A NAME=\"_pathnames\"></A><H3>Pathnames</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"Users homedirs", CFG.bbs_usersdir);
    add_webtable(wp, (char *)"Nodelists", CFG.nodelists);
    add_webtable(wp, (char *)"Unsafe inbound", CFG.inbound);
    add_webtable(wp, (char *)"Known inbound", CFG.pinbound);
    add_webtable(wp, (char *)"Outbound", CFG.outbound);
    add_webtable(wp, (char *)"Outbound queue", CFG.out_queue);
    add_webtable(wp, (char *)"*.msgs path", CFG.msgs_path);
    add_webtable(wp, (char *)"Bad TIC's", CFG.badtic);
    add_webtable(wp, (char *)"TIC queue", CFG.ticout);
    add_webtable(wp, (char *)"Magic filerequests", CFG.req_magic);
    add_webtable(wp, (char *)"DOS path", CFG.dospath);
    add_webtable(wp, (char *)"Unix path", CFG.uxpath);
    add_webtable(wp, (char *)"Leave case as is", getboolean(CFG.leavecase));
    add_webtable(wp, (char *)"FTP base path", CFG.ftp_base);
    add_webtable(wp, (char *)"Area lists", CFG.alists_path);
    add_webtable(wp, (char *)"External editor", CFG.externaleditor);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    addtoc(fp, toc, 1, 4, page, (char *)"Pathnames");
    fprintf(fp, "      Users homedirs   %s\n", CFG.bbs_usersdir);
    fprintf(fp, "      Nodelists        %s\n", CFG.nodelists);
    fprintf(fp, "      Unsafe inbound   %s\n", CFG.inbound);
    fprintf(fp, "      Known inbound    %s\n", CFG.pinbound);
    fprintf(fp, "      Outbound         %s\n", CFG.outbound);
    fprintf(fp, "      Out queue        %s\n", CFG.out_queue);
    fprintf(fp, "      *.msgs path      %s\n", CFG.msgs_path);
    fprintf(fp, "      Bad TIC's        %s\n", CFG.badtic);
    fprintf(fp, "      TIC queue        %s\n", CFG.ticout);
    fprintf(fp, "      Magic filereq.   %s\n", CFG.req_magic);
    fprintf(fp, "      DOS path         %s\n", CFG.dospath);
    fprintf(fp, "      Unix path        %s\n", CFG.uxpath);
    fprintf(fp, "      Leave case as is %s\n", getboolean(CFG.leavecase));
    fprintf(fp, "      FTP base path    %s\n", CFG.ftp_base);
    fprintf(fp, "      Area lists       %s\n", CFG.alists_path);
    fprintf(fp, "      External editor  %s\n", CFG.externaleditor);

    fprintf(wp, "<A NAME=\"_global\"></A><H3>Global settings</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"Show new message areas", getboolean(CFG.NewAreas));
    add_webtable(wp, (char *)"Exclude sysop from lists", getboolean(CFG.exclude_sysop));
    add_webtable(wp, (char *)"Show connect info", getboolean(CFG.iConnectString));
    add_webtable(wp, (char *)"Ask protocols", getboolean(CFG.iAskFileProtocols));
    add_webdigit(wp, (char *)"Sysop security level", CFG.sysop_access);
    add_webdigit(wp, (char *)"Minimum password length", CFG.password_length);
    add_webtable(wp, (char *)"BBS loglevel", getloglevel(CFG.bbs_loglevel));
    add_webtable(wp, (char *)"Util loglevel", getloglevel(CFG.util_loglevel));
    snprintf(temp, 81, "%c", CFG.iPasswd_Char);
    add_webtable(wp, (char *)"Password char", temp);
    add_webdigit(wp, (char *)"Idle timeout in minutes", CFG.idleout);
    add_webdigit(wp, (char *)"Login enters", CFG.iCRLoginCount);
    add_webdigit(wp, (char *)"Homedir quota in MByte", CFG.iQuota);
    add_webdigit(wp, (char *)"Minimum location length", CFG.CityLen);
    add_webdigit(wp, (char *)"OLR Max. messages", CFG.OLR_MaxMsgs);
    add_webdigit(wp, (char *)"OLR Newfile days", CFG.OLR_NewFileLimit);
    add_webdigit(wp, (char *)"OLR Max. filerequests", CFG.OLR_MaxReq);
    add_webtable(wp, (char *)"Slowdown utilities", getboolean(CFG.slow_util));
    add_webdigit(wp, (char *)"CrashMail security level", CFG.iCrashLevel);
    add_webdigit(wp, (char *)"FileAttach security level", CFG.iAttachLevel);
    add_webdigit(wp, (char *)"Free diskspace in MBytes", CFG.freespace);
    if (CFG.max_logins)
	snprintf(temp, 81, "%d", CFG.max_logins);
    else
	snprintf(temp, 81, "Unlimited");
    add_webtable(wp, (char *)"Simultaneous logins", temp);
    add_webdigit(wp, (char *)"Child priority", CFG.priority);
    add_webtable(wp, (char *)"Sync on execute", getboolean(CFG.do_sync));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 5, page, (char *)"Global settings");
    fprintf(fp, "      Show new msgarea %s\n", getboolean(CFG.NewAreas));
    fprintf(fp, "      Exclude sysop    %s\n", getboolean(CFG.exclude_sysop));
    fprintf(fp, "      Show connect     %s\n", getboolean(CFG.iConnectString));
    fprintf(fp, "      Ask protocols    %s\n", getboolean(CFG.iAskFileProtocols)); 
    fprintf(fp, "      Sysop level      %d\n", CFG.sysop_access); 
    fprintf(fp, "      Password length  %d\n", CFG.password_length);
    p = getloglevel(CFG.bbs_loglevel);
    fprintf(fp, "      BBS loglevel     %s\n", p);
    free(p);
    p = getloglevel(CFG.util_loglevel);
    fprintf(fp, "      Util loglevel    %s\n", p);
    free(p);
    fprintf(fp, "      Password char    %c\n", CFG.iPasswd_Char);
    fprintf(fp, "      Idle timeout     %d mins\n", CFG.idleout);
    fprintf(fp, "      Login enters     %d\n", CFG.iCRLoginCount);
    fprintf(fp, "      Homedir quota    %d MB.\n", CFG.iQuota);
    fprintf(fp, "      Location length  %d\n", CFG.CityLen);
    fprintf(fp, "      OLR Max. msgs.   %d\n", CFG.OLR_MaxMsgs);
    fprintf(fp, "      OLR Newfile days %d\n", CFG.OLR_NewFileLimit);
    fprintf(fp, "      OLR Max Freq's   %d\n", CFG.OLR_MaxReq);
    fprintf(fp, "      Slow utilities   %s\n", getboolean(CFG.slow_util));
    fprintf(fp, "      CrashMail level  %d\n", CFG.iCrashLevel);
    fprintf(fp, "      FileAttach level %d\n", CFG.iAttachLevel);
    fprintf(fp, "      Free diskspace   %d MB.\n", CFG.freespace);
    if (CFG.max_logins)
	fprintf(fp, "      Simult. logins   %d\n", CFG.max_logins);
    else
	fprintf(fp, "      Simult. logins   unlimited\n");
    fprintf(fp, "      Child priority   %d\n", CFG.priority);
    fprintf(fp, "      Sync on execute  %s\n", getboolean(CFG.do_sync));

    fprintf(wp, "<A NAME=\"_security\"></A><H3>Users flag descriptions</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    for (i = 0; i < 32; i++) {
	snprintf(temp, 81, "Bit %d", i+1);
	add_webtable(wp, temp, CFG.fname[i]);
    }
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 6, page, (char *)"Users flag descriptions");
    fprintf(fp, "               1    1    2    2    3 3\n");
    fprintf(fp, "      1   5    0    5    0    5    0 2\n");
    fprintf(fp, "      --------------------------------\n");
    fprintf(fp, "      ||||||||||||||||||||||||||||||||\n");
    for (i = 0; i < 32; i++) {
	fprintf(fp, "      ");
        for (j = 0; j < (31 - i); j++)
	   fprintf(fp, "|");
	fprintf(fp, "+");
	for (j = (32 - i); j < 32; j++)
	    fprintf(fp, "-");
	fprintf(fp, " %s\n", CFG.fname[31 - i]);
    }

    fprintf(wp, "<A NAME=\"_newusers\"></A><H3>New users defaults</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    web_secflags(wp, (char *)"Access level", CFG.newuser_access);
    add_webtable(wp, (char *)"Cap. username", getboolean(CFG.iCapUserName));
    add_webtable(wp, (char *)"Ask Sex", getboolean(CFG.iSex));
    add_webtable(wp, (char *)"Ask Voicephone", getboolean(CFG.iVoicePhone));
    add_webtable(wp, (char *)"Ask Dataphone", getboolean(CFG.iDataPhone));
    add_webtable(wp, (char *)"Telephone Scan", getboolean(CFG.iTelephoneScan));
    add_webtable(wp, (char *)"Ask Handle", getboolean(CFG.iHandle));
    add_webtable(wp, (char *)"Ask Birthdate", getboolean(CFG.iDOB));
    add_webtable(wp, (char *)"Ask Location", getboolean(CFG.iLocation));
    add_webtable(wp, (char *)"Ask Hotkeys", getboolean(CFG.iHotkeys));
    add_webtable(wp, (char *)"Allow one word names", getboolean(CFG.iOneName));
    add_webtable(wp, (char *)"Ask Address", getboolean(CFG.AskAddress));
    add_webtable(wp, (char *)"Give email box", getboolean(CFG.GiveEmail));
    add_webtable(wp, (char *)"Do newmail check", get_asktype(CFG.AskNewmail));
    add_webtable(wp, (char *)"Do newfiles check", get_asktype(CFG.AskNewfiles));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 7, page, (char *)"New users defaults");
    fprintf(fp, "      Access level      %s\n", get_secstr(CFG.newuser_access));
    fprintf(fp, "      Cap. username     %s\n", getboolean(CFG.iCapUserName));
    fprintf(fp, "      Ask Sex           %s\n", getboolean(CFG.iSex));
    fprintf(fp, "      Ask voicephone    %s\n", getboolean(CFG.iVoicePhone));
    fprintf(fp, "      Ask dataphone     %s\n", getboolean(CFG.iDataPhone));
    fprintf(fp, "      Telephone scan    %s\n", getboolean(CFG.iTelephoneScan));
    fprintf(fp, "      Ask handle        %s\n", getboolean(CFG.iHandle));
    fprintf(fp, "      Ask birthdate     %s\n", getboolean(CFG.iDOB));
    fprintf(fp, "      Ask location      %s\n", getboolean(CFG.iLocation));
    fprintf(fp, "      Ask hotkeys       %s\n", getboolean(CFG.iHotkeys));
    fprintf(fp, "      One word names    %s\n", getboolean(CFG.iOneName));
    fprintf(fp, "      Ask address       %s\n", getboolean(CFG.AskAddress));
    fprintf(fp, "      Give email box    %s\n", getboolean(CFG.GiveEmail));
    fprintf(fp, "      Do newmail check  %s\n", get_asktype(CFG.AskNewmail));
    fprintf(fp, "      Do newfiles check %s\n", get_asktype(CFG.AskNewfiles));

    fprintf(wp, "<A NAME=\"_colors\"></A><H3>Text colors</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_colors(wp, (char *)"Normal text", CFG.TextColourF, CFG.TextColourB);
    add_colors(wp, (char *)"Underline text", CFG.UnderlineColourF, CFG.UnderlineColourB);
    add_colors(wp, (char *)"Input text", CFG.InputColourF, CFG.InputColourB);
    add_colors(wp, (char *)"CR text", CFG.CRColourF, CFG.CRColourB);
    add_colors(wp, (char *)"More prompt", CFG.MoreF, CFG.MoreB);
    add_colors(wp, (char *)"Hilite text", CFG.HiliteF, CFG.HiliteB);
    add_colors(wp, (char *)"File name", CFG.FilenameF, CFG.FilenameB);
    add_colors(wp, (char *)"File size", CFG.FilesizeF, CFG.FilesizeB);
    add_colors(wp, (char *)"File date", CFG.FiledateF, CFG.FiledateB);
    add_colors(wp, (char *)"File description", CFG.FiledescF, CFG.FiledescB);
    add_colors(wp, (char *)"Message input", CFG.MsgInputColourF, CFG.MsgInputColourB);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    addtoc(fp, toc, 1, 8, page, (char *)"Text colors");
    fprintf(fp, "      Normal text      %s on %s\n", get_color(CFG.TextColourF), get_color(CFG.TextColourB));
    fprintf(fp, "      Underline text   %s on %s\n", get_color(CFG.UnderlineColourF), get_color(CFG.UnderlineColourB));
    fprintf(fp, "      Input text       %s on %s\n", get_color(CFG.InputColourF), get_color(CFG.InputColourB));
    fprintf(fp, "      CR text          %s on %s\n", get_color(CFG.CRColourF), get_color(CFG.CRColourB));
    fprintf(fp, "      More prompt      %s on %s\n", get_color(CFG.MoreF), get_color(CFG.MoreB));
    fprintf(fp, "      Hilite text      %s on %s\n", get_color(CFG.HiliteF), get_color(CFG.HiliteB));
    fprintf(fp, "      File name        %s on %s\n", get_color(CFG.FilenameF), get_color(CFG.FilenameB));
    fprintf(fp, "      File size        %s on %s\n", get_color(CFG.FilesizeF), get_color(CFG.FilesizeB));
    fprintf(fp, "      File date        %s on %s\n", get_color(CFG.FiledateF), get_color(CFG.FiledateB));
    fprintf(fp, "      File description %s on %s\n", get_color(CFG.FiledescF), get_color(CFG.FiledescB));
    fprintf(fp, "      Message input    %s on %s\n", get_color(CFG.MsgInputColourF), get_color(CFG.MsgInputColourB));

    fprintf(wp, "<A NAME=\"_paging\"></A><H3>Sysop Paging</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webdigit(wp, (char *)"Page length in seconds", CFG.iPageLength);
    add_webdigit(wp, (char *)"Maximum page times", CFG.iMaxPageTimes);
    add_webdigit(wp, (char *)"Sysop message area", CFG.iSysopArea);
    add_webtable(wp, (char *)"Ask chat reason", getboolean(CFG.iAskReason));
    add_webtable(wp, (char *)"Auto log chat session", getboolean(CFG.iAutoLog));
    add_webtable(wp, (char *)"Check at menu prompt", getboolean(CFG.iChatPromptChk));
    add_webtable(wp, (char *)"Freeze online time", getboolean(CFG.iStopChatTime));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 9, page, (char *)"Sysop paging");
    fprintf(fp, "      Page length        %d seconds\n", CFG.iPageLength);
    fprintf(fp, "      Page times         %d\n", CFG.iMaxPageTimes);
    fprintf(fp, "      Sysop msg area     %d\n", CFG.iSysopArea);
    fprintf(fp, "      Ask chat reason    %s\n", getboolean(CFG.iAskReason));
    fprintf(fp, "      Log chat           %s\n", getboolean(CFG.iAutoLog));
    fprintf(fp, "      Check at prompt    %s\n", getboolean(CFG.iChatPromptChk));
    fprintf(fp, "      Freeze online time %s\n", getboolean(CFG.iStopChatTime));

    fprintf(wp, "<A NAME=\"_fileecho\"></A><H3>Fileecho Processing</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webdigit(wp, (char *)"Keep days on hold", CFG.tic_days);
    add_webtable(wp, (char *)"Hatch password", CFG.hatchpasswd);
    add_webdigit(wp, (char *)"Maximum connected systems", CFG.tic_systems);
    add_webdigit(wp, (char *)"Max files groups", CFG.tic_groups);
    add_webdigit(wp, (char *)"Max dupes in database", CFG.tic_dupes);
    add_webtable(wp, (char *)"Keep filedate", getboolean(CFG.ct_KeepDate));
    add_webtable(wp, (char *)"Keep FileMgr netmail", getboolean(CFG.ct_KeepMgr));
    add_webtable(wp, (char *)"FileMgr: allow +%%*", getboolean(CFG.ct_PlusAll));
    add_webtable(wp, (char *)"FileMgr: notify", getboolean(CFG.ct_Notify));
    add_webtable(wp, (char *)"FileMgr: passwd", getboolean(CFG.ct_Passwd));
    add_webtable(wp, (char *)"FileMgr: message", getboolean(CFG.ct_Message));
    add_webtable(wp, (char *)"FileMgr: TIC", getboolean(CFG.ct_TIC));
    add_webtable(wp, (char *)"FileMgr: pause", getboolean(CFG.ct_Pause));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");    
    addtoc(fp, toc, 1, 10, page, (char *)"Fileecho processing");
    fprintf(fp, "      Keep days on hold  %d\n", CFG.tic_days);
    fprintf(fp, "      Hatch password     %s\n", CFG.hatchpasswd);
    fprintf(fp, "      Max. systems       %d\n", CFG.tic_systems);
    fprintf(fp, "      Max. groups        %d\n", CFG.tic_groups);
    fprintf(fp, "      Max. dupes         %d\n", CFG.tic_dupes);
    fprintf(fp, "      Keep filedate      %s\n", getboolean(CFG.ct_KeepDate));
    fprintf(fp, "      Keep mgr netmail   %s\n", getboolean(CFG.ct_KeepMgr));
    fprintf(fp, "      FileMgr: allow +%%* %s\n", getboolean(CFG.ct_PlusAll));
    fprintf(fp, "      FileMgr: notify    %s\n", getboolean(CFG.ct_Notify));
    fprintf(fp, "      FileMgr: passwd    %s\n", getboolean(CFG.ct_Passwd));
    fprintf(fp, "      FileMgr: message   %s\n", getboolean(CFG.ct_Message));
    fprintf(fp, "      FileMgr: TIC       %s\n", getboolean(CFG.ct_TIC));
    fprintf(fp, "      FileMgr: pause     %s\n", getboolean(CFG.ct_Pause));

    fprintf(wp, "<A NAME=\"_fidomail\"></A><H3>Fidonet Mail and Echomail processing</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webdigit(wp, (char *)"Maximum .pkt size", CFG.maxpktsize);
    add_webdigit(wp, (char *)"Maximum archive size", CFG.maxarcsize);
    add_webtable(wp, (char *)"Bad mail board", CFG.badboard);
    add_webtable(wp, (char *)"Dupe mail board", CFG.dupboard);
    add_webtable(wp, (char *)"Pktdate program", CFG.pktdate);
    add_webdigit(wp, (char *)"Keep days on hold", CFG.toss_days);
    add_webdigit(wp, (char *)"Dupes in database", CFG.toss_dupes);
    add_webdigit(wp, (char *)"Default maximum msgs", CFG.defmsgs);
    add_webdigit(wp, (char *)"Default maximum days old", CFG.defdays);
    add_webdigit(wp, (char *)"Reject messages older then", CFG.toss_old);
    add_webdigit(wp, (char *)"Maximum connected systems", CFG.toss_systems);
    add_webdigit(wp, (char *)"Maximum message groups", CFG.toss_groups);
    add_webtable(wp, (char *)"Use 4d addressing", getboolean(CFG.addr4d));
    add_webtable(wp, (char *)"AreaMgr: allow +%%*", getboolean(CFG.ca_PlusAll));
    add_webtable(wp, (char *)"AreaMgr: notify", getboolean(CFG.ca_Notify));
    add_webtable(wp, (char *)"AreaMgr: passwd", getboolean(CFG.ca_Passwd));
    add_webtable(wp, (char *)"AreaMgr: pause", getboolean(CFG.ca_Pause));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 11, page, (char *)"Fidonet Mail and Echomail processing");
    fprintf(fp, "      Max .pkt size      %d Kb.\n", CFG.maxpktsize);
    fprintf(fp, "      Max archive size   %d Kb.\n", CFG.maxarcsize);
    fprintf(fp, "      Bad mail board     %s\n", CFG.badboard);
    fprintf(fp, "      Dupe mail board    %s\n", CFG.dupboard);
    fprintf(fp, "      Pktdate program    %s\n", CFG.pktdate);
    fprintf(fp, "      Keep on hold       %d days\n", CFG.toss_days);
    fprintf(fp, "      Dupes in database  %d\n", CFG.toss_dupes);
    fprintf(fp, "      Default max msgs   %d\n", CFG.defmsgs);
    fprintf(fp, "      Default days       %d\n", CFG.defdays);
    fprintf(fp, "      Reject older then  %d days\n", CFG.toss_old);
    fprintf(fp, "      Maximum systems    %d\n", CFG.toss_systems);
    fprintf(fp, "      Maximum groups     %d\n", CFG.toss_groups);
    fprintf(fp, "      Use 4d addressing  %s\n", getboolean(CFG.addr4d));
    fprintf(fp, "      AreaMgr: allow +%%* %s\n", getboolean(CFG.ca_PlusAll));
    fprintf(fp, "      AreaMgr: notify    %s\n", getboolean(CFG.ca_Notify));
    fprintf(fp, "      AreaMgr: passwd    %s\n", getboolean(CFG.ca_Passwd));
    fprintf(fp, "      AreaMgr: pause     %s\n", getboolean(CFG.ca_Pause));

    fprintf(wp, "<A NAME=\"_inetmail\"></A><H3>Internet Mail and News processing</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webdigit(wp, (char *)"Split messages at KByte", CFG.new_split);
    add_webdigit(wp, (char *)"Force split at KByte", CFG.new_force);
    add_webtable(wp, (char *)"ISP Email Mode", getemailmode(CFG.EmailMode));
    add_webtable(wp, (char *)"Email fido aka", aka2str(CFG.EmailFidoAka));
    add_webtable(wp, (char *)"UUCP gateway", aka2str(CFG.UUCPgate));
    add_webtable(wp, (char *)"POP3 host", CFG.popnode);
    add_webtable(wp, (char *)"POP3 user@domain login", getboolean(CFG.UsePopDomain));
    add_webtable(wp, (char *)"SMTP host", CFG.smtpnode);
    add_webtable(wp, (char *)"News transfermode", getnewsmode(CFG.newsfeed));
    addtoc(fp, toc, 1, 12, page, (char *)"Internet Mail and News processing");
    fprintf(fp, "      Split messages at  %d KBytes\n", CFG.new_split);
    fprintf(fp, "      Force split at     %d KBytes\n", CFG.new_force);
    fprintf(fp, "      ISP Email Mode     %s\n", getemailmode(CFG.EmailMode));
    fprintf(fp, "      Email fido aka     %s\n", aka2str(CFG.EmailFidoAka));
    fprintf(fp, "      UUCP gateway       %s\n", aka2str(CFG.UUCPgate));
    fprintf(fp, "      POP3 host          %s\n", CFG.popnode);
    fprintf(fp, "      POP3 user@domain   %s\n", getboolean(CFG.UsePopDomain));
    fprintf(fp, "      SMTP host          %s\n", CFG.smtpnode);
    fprintf(fp, "      News transfermode  %s\n", getnewsmode(CFG.newsfeed));
    switch (CFG.newsfeed) {
	case FEEDINN:	fprintf(fp, "      NNTP host          %s:%d\n", CFG.nntpnode, CFG.nntpport);
			fprintf(fp, "      NNTP mode reader   %s\n", getboolean(CFG.modereader));
			fprintf(fp, "      NNTP username      %s\n", CFG.nntpuser);
			fprintf(fp, "      NNTP password      %s\n", getboolean(strlen(CFG.nntppass)));
			fprintf(fp, "      NNTP force auth    %s\n", getboolean(CFG.nntpforceauth));
			add_webtable(wp, (char *)"NNTP host", CFG.nntpnode);
			add_webdigit(wp, (char *)"NNTP port", CFG.nntpport);
			add_webtable(wp, (char *)"NNTP mode reader", getboolean(CFG.modereader));
			add_webtable(wp, (char *)"NNTP username", CFG.nntpuser);
			add_webtable(wp, (char *)"NNTP password", CFG.nntppass);
			add_webtable(wp, (char *)"NNTP force auth", getboolean(CFG.nntpforceauth));
			break;
	case FEEDRNEWS:	fprintf(fp, "      Path to rnews      %s\n", CFG.rnewspath);
			add_webtable(wp, (char *)"Path to rnews", CFG.rnewspath);
			break;
	case FEEDUUCP:	fprintf(fp, "      NNTP host          %s\n", CFG.nntpnode);
			fprintf(fp, "      Path to rnews      %s\n", CFG.rnewspath);
			add_webtable(wp, (char *)"NNTP host", CFG.nntpnode);
			add_webtable(wp, (char *)"Path to rnews", CFG.rnewspath);
			break;
    }
    add_webdigit(wp, (char *)"Max articles fetch", CFG.maxarticles);
    add_webtable(wp, (char *)"Allow control msgs", getboolean(CFG.allowcontrol));
    add_webtable(wp, (char *)"Don't regate msgs", getboolean(CFG.dontregate));
    fprintf(fp, "      Max articles fetch %d\n", CFG.maxarticles);
    fprintf(fp, "      Allow control msgs %s\n", getboolean(CFG.allowcontrol));
    fprintf(fp, "      Don't regate msgs  %s\n", getboolean(CFG.dontregate));
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    
    fprintf(wp, "<A NAME=\"_newfiles\"></A><H3>Newfile reports</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");    
    add_webdigit(wp, (char *)"New files days", CFG.newdays);
    web_secflags(wp, (char *)"Highest security level", CFG.security);
    add_webdigit(wp, (char *)"Max. newfile groups", CFG.new_groups);
    add_webtable(wp, (char *)"WWW logfile", CFG.www_logfile);
    add_webtable(wp, (char *)"FTP logfile", CFG.ftp_logfile);
    addtoc(fp, toc, 1, 13, page, (char *)"Newfile reports");
    fprintf(fp, "      New files days     %d\n", CFG.newdays);
    fprintf(fp, "      Highest sec. level %s\n", get_secstr(CFG.security));
    fprintf(fp, "      Max. newfile grps  %d\n", CFG.new_groups);
    fprintf(fp, "      WWW logfile        %s\n", CFG.www_logfile);
    fprintf(fp, "      FTP logfile        %s\n", CFG.ftp_logfile);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");

    fprintf(wp, "<A NAME=\"_mailer\"></A><H3>Mailer setup</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"Mailer loglevel", getloglevel(CFG.cico_loglevel));
    add_webdigit(wp, (char *)"Reset modem timeout", CFG.timeoutreset);
    add_webdigit(wp, (char *)"Connect timeout", CFG.timeoutconnect);
    add_webdigit(wp, (char *)"Random dialdelay", CFG.dialdelay);
    add_webtable(wp, (char *)"TCP/IP \"phone\" number", CFG.IP_Phone);
    add_webdigit(wp, (char *)"TCP/IP linespeed", CFG.IP_Speed);
    add_webtable(wp, (char *)"TCP/IP flags", CFG.IP_Flags);
    add_webtable(wp, (char *)"No Filerequests", getboolean(CFG.NoFreqs));
    add_webtable(wp, (char *)"No Calls", getboolean(CFG.NoCall));
    add_webtable(wp, (char *)"No EMSI", getboolean(CFG.NoEMSI));
    add_webtable(wp, (char *)"No YooHoo/2U2", getboolean(CFG.NoWazoo));
    add_webtable(wp, (char *)"No Zmodem", getboolean(CFG.NoZmodem));
    add_webtable(wp, (char *)"No Zedzap", getboolean(CFG.NoZedzap));
    add_webtable(wp, (char *)"No Hydra", getboolean(CFG.NoHydra));
    add_webtable(wp, (char *)"No MD5 passwords", getboolean(CFG.NoMD5));
    add_webtable(wp, (char *)"0 bytes lockfiles are OK", getboolean(CFG.ZeroLocks));
    add_webdigit(wp, (char *)"Max request files", CFG.Req_Files);
    add_webdigit(wp, (char *)"Max request MBytes", CFG.Req_MBytes);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 14, page, (char *)"Mailer setup");
    p = getloglevel(CFG.cico_loglevel);
    fprintf(fp, "      Mailer loglevel    %s\n",  p);
    free(p);
    fprintf(fp, "      Res. modem timeout %d\n", CFG.timeoutreset);
    fprintf(fp, "      Connect timeout    %d\n", CFG.timeoutconnect);
    fprintf(fp, "      Random dialdelay   %d\n", CFG.dialdelay);
    fprintf(fp, "      TCP/IP phone nr.   %s\n",  CFG.IP_Phone);
    fprintf(fp, "      TCP/IP speed       %u\n", CFG.IP_Speed);
    fprintf(fp, "      TCP/IP flags       %s\n",  CFG.IP_Flags);
    fprintf(fp, "      No Filerequests    %s\n",  getboolean(CFG.NoFreqs));
    fprintf(fp, "      No Calls           %s\n",  getboolean(CFG.NoCall));
    fprintf(fp, "      No EMSI            %s\n",  getboolean(CFG.NoEMSI));
    fprintf(fp, "      No YooHoo/2U2      %s\n",  getboolean(CFG.NoWazoo));
    fprintf(fp, "      No Zmodem          %s\n",  getboolean(CFG.NoZmodem));
    fprintf(fp, "      No Zedzap          %s\n",  getboolean(CFG.NoZedzap));
    fprintf(fp, "      No Hydra           %s\n",  getboolean(CFG.NoHydra));
    fprintf(fp, "      No MD5 passwords   %s\n",  getboolean(CFG.NoMD5));
    fprintf(fp, "      0 bytes locks OK   %s\n",  getboolean(CFG.ZeroLocks));
    fprintf(fp, "      Max request files  %d\n",  CFG.Req_Files);
    fprintf(fp, "      Max request MBytes %d\n",  CFG.Req_MBytes);
    fprintf(wp, "<P>\n");
    fprintf(wp, "<TABLE width='360' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='50%%'><COL width='50%%'>\n");
    fprintf(wp, "<TBODY>\n");
    fprintf(wp, "<TR><TH colspan='2'>Phone number translation</TH></TR>\n");
    fprintf(wp, "<TR><TH align='left'>From</TH><TH align='left'>To</TH></TR>\n");
    for (i = 0; i < 40; i++)
	if ((CFG.phonetrans[i].match[0] != '\0') || (CFG.phonetrans[i].repl[0] != '\0')) {
	    fprintf(fp, "      Translate          %-20s %s\n", CFG.phonetrans[i].match, CFG.phonetrans[i].repl);
	    fprintf(wp, "<TR><TD>%s</TD><TD>%s</TD></TR>\n", CFG.phonetrans[i].match, CFG.phonetrans[i].repl);
	}
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    
    fprintf(wp, "<A NAME=\"_www\"></A><H3>WWW server setup</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    add_webtable(wp, (char *)"HTML root", CFG.www_root);
    add_webtable(wp, (char *)"Link to FTP base", CFG.www_link2ftp);
    add_webtable(wp, (char *)"Webserver URL", CFG.www_url);
    add_webtable(wp, (char *)"Character set", CFG.www_charset);
    add_webtable(wp, (char *)"Author name", CFG.www_author);
    add_webtable(wp, (char *)"Convert command", CFG.www_convert);
    add_webdigit(wp, (char *)"Files per webpage", CFG.www_files_page);
    add_webdigit(wp, (char *)"Mailer history lines", CFG.www_mailerlines);
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    fprintf(wp, "<HR>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1, 15, page, (char *)"WWW server setup");
    fprintf(fp, "      HTML root            %s\n", CFG.www_root);
    fprintf(fp, "      Link to FTP base     %s\n", CFG.www_link2ftp);
    fprintf(fp, "      Webserver URL        %s\n", CFG.www_url);
    fprintf(fp, "      Character set        %s\n", CFG.www_charset);
    fprintf(fp, "      Author name          %s\n", CFG.www_author);
    fprintf(fp, "      Convert command      %s\n", CFG.www_convert);
    fprintf(fp, "      Files per webpage    %d\n", CFG.www_files_page);
    fprintf(fp, "      Mailer history lines %d\n", CFG.www_mailerlines);

    fprintf(wp, "<A NAME=\"_manager\"></A><H3>Manager flag descriptions</H3>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
    fprintf(wp, "<TBODY>\n");
    for (i = 0; i < 32; i++) {
	snprintf(temp, 81, "Bit %d", i+1); 
	add_webtable(wp, temp, CFG.aname[i]);
    }
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    fprintf(wp, "<A HREF=\"#_top\">Top</A>\n");
    page = newpage(fp, page);
    addtoc(fp, toc, 1,16, page, (char *)"Manager flag descriptions");
    fprintf(fp, "               1    1    2    2    3 3\n");
    fprintf(fp, "      1   5    0    5    0    5    0 2\n");
    fprintf(fp, "      --------------------------------\n");
    fprintf(fp, "      ||||||||||||||||||||||||||||||||\n");
    for (i = 0; i < 32; i++) {
	fprintf(fp, "      ");
        for (j = 0; j < (31 - i); j++)
	    fprintf(fp, "|");
	fprintf(fp, "+");
        for (j = (32 - i); j < 32; j++)
	    fprintf(fp, "-");
	fprintf(fp, " %s\n", CFG.aname[31 - i]);
    }

    close_webdoc(wp);
    return page;
}



