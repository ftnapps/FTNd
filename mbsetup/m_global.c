/****************************************************************************
 *
 * $Id$
 * Purpose ...............: Global Setup Program 
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_node.h"
#include "m_marea.h"
#include "m_ticarea.h"
#include "m_new.h"
#include "m_fgroup.h"
#include "m_mgroup.h"
#include "m_global.h"


char	    *some_fn;
int	    some_fd;
extern int  exp_golded;


#define WRLONG cnt = write(some_fd, &longvar, sizeof(longvar));



void config_check(char *path)
{
	static char	buf[PATH_MAX];

	sprintf(buf, "%s/etc/config.data", path);
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
	working(1, 0, 0);
	if (0 == config_read()) {
		working(0, 0, 0);
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
	working(0, 0, 0);
}



void e_reginfo(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "1.2   EDIT REGISTRATION INFO");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.    System name");
	mvprintw( 8, 6, "2.    Domain name");
	mvprintw( 9, 6, "3.    Sysop uid");
	mvprintw(10, 6, "4.    Sysop Fido");
	mvprintw(11, 6, "5.    Location");
	mvprintw(12, 6, "6.    QWK/Bluewave");
	mvprintw(13, 6, "7.    Omen id");
	mvprintw(14, 6, "8.    Comment");
	mvprintw(15, 6, "9.    Origin line");
	mvprintw(16, 6, "10.   Startup uid");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,25,35, CFG.bbs_name);
		show_str( 8,25,35, CFG.sysdomain);
		show_str( 9,25, 8, CFG.sysop);
		show_str(10,25,35, CFG.sysop_name);
		show_str(11,25,35, CFG.location);
		show_str(12,25, 8, CFG.bbsid);
		show_str(13,25, 2, CFG.bbsid2);
		show_str(14,25,55, CFG.comment);
		show_str(15,25,50, CFG.origin);  
		show_str(16,25, 8, CFG.startname);

		switch(select_menu(10)) {
		case 0: return;
		case 1: E_STR( 7,25,35, CFG.bbs_name,   "Name of this ^BBS^ system")
		case 2: E_STR( 8,25,35, CFG.sysdomain,  "Internet ^mail domain^ name of this system")
		case 3:	E_STR( 9,25, 8, CFG.sysop,      "^Unix name^ of the sysop")
		case 4:	E_STR(10,25,35, CFG.sysop_name, "^Fidonet name^ of the sysop")
		case 5:	E_STR(11,25,35, CFG.location,   "^Location^ (city) of this system")
		case 6:	E_UPS(12,25, 8, CFG.bbsid,      "^QWK/Bluewave^ packets name")
		case 7:	E_UPS(13,25, 2, CFG.bbsid2,     "^Omen offline^ reader ID characters")
		case 8:	E_STR(14,25,55, CFG.comment,    "Some ^comment^ you may like to give")
		case 9:	E_STR(15,25,50, CFG.origin,     "Default ^origin^ line under echomail messages")
		case 10:E_STR(16,25, 8, CFG.startname,  "The ^Unix username^ that is used to start the bbs")
		}
	};
} 




void e_filenames(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 6, "1.3   EDIT GLOBAL FILENAMES");
    set_color(CYAN, BLACK);
    mvprintw( 7, 6, "1.   System logfile");
    mvprintw( 8, 6, "2.   Error logfile");
    mvprintw( 9, 6, "3.   Mgr logfile");
    mvprintw(10, 6, "4.   Default Menu");
    mvprintw(11, 6, "5.   Default Language");
    mvprintw(12, 6, "6.   Chat Logfile");
    mvprintw(13, 6, "7.   Welcome Logo");
    
    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,28,14, CFG.logfile);
	show_str( 8,28,14, CFG.error_log);
	show_str( 9,28,14, CFG.mgrlog);
	show_str(10,28,14, CFG.default_menu);
	show_str(11,28,14, CFG.current_language);
	show_str(12,28,14, CFG.chat_log);
	show_str(13,28,14, CFG.welcome_logo);

	switch(select_menu(7)) {
	    case 0: return;
	    case 1: E_STR( 7,28,14, CFG.logfile,          "The name of the ^system^ logfile.")
	    case 2: E_STR( 8,28,14, CFG.error_log,        "The name of the ^errors^ logfile.")
	    case 3: E_STR( 9,28,14, CFG.mgrlog,           "The name of the ^area-/filemgr^ logfile.")
	    case 4: E_STR(10,28,14, CFG.default_menu,     "The name of the ^default^ (top) ^menu^.")
	    case 5: E_STR(11,28,14, CFG.current_language, "The name of the ^default language^.")
	    case 6: E_STR(12,28,14, CFG.chat_log,         "The name of the ^chat^ logfile.")
	    case 7: E_STR(13,28,14, CFG.welcome_logo,     "The name of the ^BBS logo^ file.")
	}
    }
}



void e_global2(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 4, 6, "1.4   EDIT GLOBAL PATHS - 2");
        set_color(CYAN, BLACK);
        mvprintw( 6, 2, "1.  Magic's");
        mvprintw( 7, 2, "2.  DOS path");
        mvprintw( 8, 2, "3.  Unix path");
        mvprintw( 9, 2, "4.  LeaveCase");
	mvprintw(10, 2, "5.  Ftp base");
	mvprintw(11, 2, "6.  Arealists");
	mvprintw(12, 2, "7.  Ext. edit");

        for (;;) {
                set_color(WHITE, BLACK);
                show_str( 6,16,64, CFG.req_magic);
                show_str( 7,16,64, CFG.dospath);
                show_str( 8,16,64, CFG.uxpath);
                show_bool(9,16,    CFG.leavecase);
		show_str(10,16,64, CFG.ftp_base);
		show_str(11,16,64, CFG.alists_path);
		show_str(12,16,64, CFG.externaleditor);

                switch(select_menu(7)) {
                case 0: return;
                case 1: E_PTH( 6,16,64, CFG.req_magic,      "The path to the ^magic filerequest^ files.", 0750)
                case 2: E_STR( 7,16,64, CFG.dospath,        "The translated ^DOS^ drive and path, empty disables translation")
                case 3: E_PTH( 8,16,64, CFG.uxpath,         "The translated ^Unix^ path.", 0750)
                case 4: E_BOOL(9,16,    CFG.leavecase,      "^Leave^ outbound flo filenames as is, ^No^ forces uppercase.")
		case 5: E_PTH(10,16,64, CFG.ftp_base,       "The ^FTP home^ directory to strip of the real directory", 0750)
		case 6: E_PTH(11,16,64, CFG.alists_path,    "The path where ^area lists^ and ^filebone lists^ are stored.", 0750)
		case 7: E_STR(12,16,64, CFG.externaleditor, 
				"The full path and filename to the ^external message editor^ (blank=disable)")
                }
        };
}



void s_global(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 4, 6, "1.4   EDIT GLOBAL PATHS");
        set_color(CYAN, BLACK);
        mvprintw( 6, 2, "1.  BBS menus");
        mvprintw( 7, 2, "2.  Txtfiles");
	mvprintw( 8, 2, "3.  Macro's");
        mvprintw( 9, 2, "4.  Home dirs");
        mvprintw(10, 2, "5.  Nodelists");
        mvprintw(11, 2, "6.  Inbound");
        mvprintw(12, 2, "7.  Prot inb.");
        mvprintw(13, 2, "8.  Outbound");
	mvprintw(14, 2, "9.  Out queue");
        mvprintw(15, 2, "10. *.msgs");
        mvprintw(16, 2, "11. Bad TIC's");
	mvprintw(17, 2, "12. TIC queue");
        mvprintw(18, 2, "13. Next Screen");
}



void e_global(void)
{
	s_global();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 6,16,64, CFG.bbs_menus);
		show_str( 7,16,64, CFG.bbs_txtfiles);
		show_str( 8,16,64, CFG.bbs_macros);
		show_str( 9,16,64, CFG.bbs_usersdir);
		show_str(10,16,64, CFG.nodelists);
		show_str(11,16,64, CFG.inbound);
		show_str(12,16,64, CFG.pinbound);
		show_str(13,16,64, CFG.outbound);
		show_str(14,16,64, CFG.out_queue);
		show_str(15,16,64, CFG.msgs_path);
		show_str(16,16,64, CFG.badtic);
		show_str(17,16,64, CFG.ticout);

		switch(select_menu(13)) {
		case 0:	return;
		case 1:	E_PTH( 6,16,64, CFG.bbs_menus,    "The path to the ^default menus^.", 0750)
		case 2:	E_PTH( 7,16,64, CFG.bbs_txtfiles, "The path to the ^default textfiles^.", 0750)
		case 3: E_PTH( 8,16,64, CFG.bbs_macros,   "The path to the ^default macro templates^.", 0750)
		case 4:	E_PTH( 9,16,64, CFG.bbs_usersdir, "The path to the ^users home^ directories.", 0770)
		case 5:	E_PTH(10,16,64, CFG.nodelists,    "The path to the ^nodelists^.", 0750)
		case 6:	E_PTH(11,16,64, CFG.inbound,      "The path to the ^inbound^ for unknown systems.", 0750)
		case 7:	E_PTH(12,16,64, CFG.pinbound,     "The path to the ^nodelists^ for protected systems.", 0750)
		case 8:	E_PTH(13,16,64, CFG.outbound,     "The path to the base ^outbound^ directory.", 0750)
		case 9: E_PTH(14,16,64, CFG.out_queue,    "The path to the ^temp outbound queue^ directory.", 0750)
		case 10:E_PTH(15,16,64, CFG.msgs_path,    "The path to the ^*.msgs^ directory.", 0750)
		case 11:E_PTH(16,16,64, CFG.badtic,       "The path to the ^bad tic files^.", 0750)
		case 12:E_PTH(17,16,64, CFG.ticout,       "The path to the ^outgoing TIC^ files.", 0750)
		case 13:e_global2();
			s_global();
			break;
		}
	};
}



void b_screen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "1.5  EDIT GLOBAL SETTINGS");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.   Exclude Sysop");
	mvprintw( 8, 2, "2.   Show Connect");
	mvprintw( 9, 2, "3.   Ask Protocols");
	mvprintw(10, 2, "4.   Sysop Level");
	mvprintw(11, 2, "5.   Password Length");
	mvprintw(12, 2, "6.   Passwd Character");
	mvprintw(13, 2, "7.   Idle timeout");
	mvprintw(14, 2, "8.   Login Enters");
	mvprintw(15, 2, "9.   Homedir Quota");
	mvprintw(16, 2, "10.  Location length");

	mvprintw( 7,37, "11.  Show new msgarea");
	mvprintw( 8,37, "12.  OLR Max. msgs.");
	mvprintw( 9,37, "13.  OLR Newfile days");
	mvprintw(10,37, "14.  OLR Max Filereq");
	mvprintw(11,37, "15.  BBS Log Level");
	mvprintw(12,37, "16.  Utils loglevel");
	mvprintw(13,37, "17.  Utils slowly");
	mvprintw(14,37, "18.  CrashMail level");
	mvprintw(15,37, "19.  FileAttach level");
	mvprintw(16,37, "20.  Min diskspace MB");

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

	show_bool( 7,59, CFG.NewAreas);
	show_int(  8,59, CFG.OLR_MaxMsgs);
	show_int(  9,59, CFG.OLR_NewFileLimit);
	show_int( 10,59, CFG.OLR_MaxReq);
	show_logl(11,59, CFG.bbs_loglevel);
	show_logl(12,59, CFG.util_loglevel);
	show_bool(13,59, CFG.slow_util);
	show_int( 14,59, CFG.iCrashLevel);
	show_int( 15,59, CFG.iAttachLevel);
	show_int( 16,59, CFG.freespace);
}



void e_bbsglob(void)
{
	b_screen();

	for (;;) {
		switch(select_menu(20)) {
		case 0:	return;
		case 1:	E_BOOL( 7,24, CFG.exclude_sysop,     "^Exclude^ sysop from lists.")
		case 2:	E_BOOL( 8,24, CFG.iConnectString,    "Show ^connect string^ at logon")
		case 3:	E_BOOL( 9,24, CFG.iAskFileProtocols, "Ask ^file protocol^ before every up- download")
		case 4:	E_INT( 10,24, CFG.sysop_access,      "Sysop ^access level^")
		case 5: E_INT( 11,24, CFG.password_length,   "Mimimum ^password^ length.")
		case 6: E_INT( 12,24, CFG.iPasswd_Char,      "Ascii number of ^password^ character")
		case 7: E_INT( 13,24, CFG.idleout,           "^Idle timeout^ in minutes")
		case 8: E_INT( 14,24, CFG.iCRLoginCount,     "Maximum ^Login Return^ count")
		case 9: E_INT( 15,24, CFG.iQuota,            "Maximum ^Quota^ in MBytes in users homedirectory");
		case 10:E_INT( 16,24, CFG.CityLen,           "Minimum ^Location name^ length (3..6)")

		case 11:E_BOOL( 7,59, CFG.NewAreas,          "Show ^new^ or ^deleted^ message areas to the user at login.")
		case 12:E_INT(  8,59, CFG.OLR_MaxMsgs,       "^Maximum messages^ to pack for download (0=unlimited)")
		case 13:E_INT(  9,59, CFG.OLR_NewFileLimit,  "^Limit Newfiles^ listing for maximum days")
		case 14:E_INT( 10,59, CFG.OLR_MaxReq,        "Maximum ^Filerequests^ to honor")
		case 15:E_LOGL(CFG.bbs_loglevel, "1.5.15", b_screen)
		case 16:E_LOGL(CFG.util_loglevel, "1.5.16", b_screen)
		case 17:E_BOOL(13,59, CFG.slow_util,         "Let background utilities run ^slowly^")
		case 18:E_INT( 14,59, CFG.iCrashLevel,       "The user level to allow sending ^CrashMail^")
		case 19:E_INT( 15,59, CFG.iAttachLevel,      "The user level to allow sending ^File Attaches^")
		case 20:E_INT( 16,59, CFG.freespace,         "Minimum ^free diskspace^ in MBytes on filesystems")
		}
	};
}



void s_newuser(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "1.7   EDIT NEW USERS DEFAULTS");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.    Access level");  
	mvprintw( 8, 6, "2.    Cap. Username");
	mvprintw( 9, 6, "3.    Ask ANSI");
	mvprintw(10, 6, "4.    Ask Sex");
	mvprintw(11, 6, "5.    Ask Voicephone");
	mvprintw(12, 6, "6.    Ask Dataphone");
	mvprintw(13, 6, "7.    Telephone scan");
	mvprintw(14, 6, "8.    Ask Handle");

	mvprintw( 8,46, "9.    Ask Birth date");
	mvprintw( 9,46, "10.   Ask Location");
	mvprintw(10,46, "11.   Ask Hot-Keys");
	mvprintw(11,46, "12.   One word names");
	mvprintw(12,46, "13.   Ask Address");
	mvprintw(13,46, "14.   Give email");
}



void e_newuser(void)
{
	s_newuser();
	for (;;) {
		set_color(WHITE, BLACK);
		show_sec(  7,28, CFG.newuser_access);
		show_bool( 8,28, CFG.iCapUserName);
		show_bool( 9,28, CFG.iAnsi);
		show_bool(10,28, CFG.iSex);
		show_bool(11,28, CFG.iVoicePhone);
		show_bool(12,28, CFG.iDataPhone);
		show_bool(13,28, CFG.iTelephoneScan);
		show_bool(14,28, CFG.iHandle);

		show_bool( 8,68, CFG.iDOB);
		show_bool( 9,68, CFG.iLocation);
		show_bool(10,68, CFG.iHotkeys);
		show_bool(11,68, CFG.iOneName);
		show_bool(12,68, CFG.AskAddress);
		show_bool(13,68, CFG.GiveEmail);

		switch(select_menu(14)) {
		case 0:	return;
		case 1: E_SEC(  7,28, CFG.newuser_access, "1.7.1 NEWUSER SECURITY", s_newuser)
		case 2:	E_BOOL( 8,28, CFG.iCapUserName, "^Capitalize^ username")
		case 3:	E_BOOL( 9,28, CFG.iAnsi, "Ask user if he wants ^ANSI^ colors")
		case 4:	E_BOOL(10,28, CFG.iSex, "Ask users ^sex^")
		case 5:	E_BOOL(11,28, CFG.iVoicePhone, "Ask users ^Voice^ phone number")
		case 6:	E_BOOL(12,28, CFG.iDataPhone, "Ask users ^Data^ phone number")
		case 7:	E_BOOL(13,28, CFG.iTelephoneScan, "Perform ^Telephone^ number scan")
		case 8:	E_BOOL(14,28, CFG.iHandle, "Ask users ^handle^")

		case 9:	E_BOOL( 8,68, CFG.iDOB, "Ask users ^Date of Birth^")
		case 10:E_BOOL( 9,68, CFG.iLocation, "Ask users ^Location^")
		case 11:E_BOOL(10,68, CFG.iHotkeys, "Ask user if he wants ^Hot-Keys^")
		case 12:E_BOOL(11,68, CFG.iOneName, "Allow ^one word^ (not in Unixmode) usernames")
		case 13:E_BOOL(12,68, CFG.AskAddress, "Aks users ^home address^ in 3 lines")
		case 14:E_BOOL(13,68, CFG.GiveEmail, "Give new users an ^private email^ box")
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
		mvprintw( 5, 6, "1.8   EDIT TEXT COLOURS");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.    Normal text");  
		mvprintw( 8, 6, "2.    Underline");
		mvprintw( 9, 6, "3.    Input lines");
		mvprintw(10, 6, "4.    CR text");
		mvprintw(11, 6, "5.    More prompt");
		mvprintw(12, 6, "6.    Hilite text");
		mvprintw(13, 6, "7.    File name");
		mvprintw(14, 6, "8.    File size");
		mvprintw(15, 6, "9.    File date");
		mvprintw(16, 6, "10.   File descr.");
		mvprintw(17, 6, "11.   Msg. input");
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



void e_nu_door(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "1.9  EDIT NEXT USER DOOR");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.   Text file");  
	mvprintw( 8, 2, "2.   Quote");
	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,17,49, CFG.sNuScreen); 
		show_str( 8,17,64, CFG.sNuQuote);

		switch(select_menu(2)) {
		case 0:	return;
		case 1:	E_STR(7,17,49, CFG.sNuScreen, "The ^text file^ to display to the next user.")
		case 2:	E_STR(8,17,64, CFG.sNuQuote,  "The ^quote^ to insert in the next user text.")
		}
	};
}



void e_safe_door(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "1.10 EDIT SAFE DOOR");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.   Digit 1"); 
	mvprintw( 8, 2, "2.   Digit 2");
	mvprintw( 9, 2, "3.   Digit 3");
	mvprintw(10, 2, "4.   Max trys");
	mvprintw(11, 2, "5.   Max numb");
	mvprintw(12, 2, "6.   Num gen");
	mvprintw(13, 2, "7.   Prize");
	mvprintw(14, 2, "8.   Welcome");
	mvprintw(15, 2, "9.   Opened");
	for (;;) {
		set_color(WHITE, BLACK);
		show_int( 7,16, CFG.iSafeFirstDigit);
		show_int( 8,16, CFG.iSafeSecondDigit);
		show_int( 9,16, CFG.iSafeThirdDigit);
		show_int(10,16, CFG.iSafeMaxTrys);
		show_int(11,16, CFG.iSafeMaxNumber);
		show_bool(12,16, CFG.iSafeNumGen);
		show_str(13,16,64, CFG.sSafePrize); 
		show_str(14,16,64, CFG.sSafeWelcome);
		show_str(15,16,64, CFG.sSafeOpened);

		switch(select_menu(9)) {
		case 0:	return;
		case 1:	E_INT(  7,16, CFG.iSafeFirstDigit,  "Enter ^first^ digit of the safe")
		case 2:	E_INT(  8,16, CFG.iSafeSecondDigit, "Enter ^second^ digit of the safe")
		case 3:	E_INT(  9,16, CFG.iSafeThirdDigit,  "Enter ^third^ digit of the safe")
		case 4:	E_INT( 10,16, CFG.iSafeMaxTrys,     "Maximum ^trys^ per day")
		case 5:	E_INT( 11,16, CFG.iSafeMaxNumber,   "^Maximum number^ of each digit")
		case 6:	E_BOOL(12,16, CFG.iSafeNumGen,      "^Automatic^ number generation")
		case 7:	E_STR( 13,16,64, CFG.sSafePrize,    "The ^prize^ the user wins when he opens the safe")
		case 8:	E_STR( 14,16,64, CFG.sSafeWelcome,  "The ^welcome^ screen for the safe door")
		case 9:	E_STR( 15,16,64, CFG.sSafeOpened,   "The file to display when the safe is ^opened^")
		}
	};
}



void e_timebank(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "1.11  EDIT TIME BANK");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.    Users time balance");  
	mvprintw( 8, 6, "2.    Max time withdraw");
	mvprintw( 9, 6, "3.    Max time deposit");
	mvprintw(10, 6, "4.    Users Kb. balance");
	mvprintw(11, 6, "5.    Max Kb. withdraw");
	mvprintw(12, 6, "6.    Max Kb. deposit");
	mvprintw(13, 6, "7.    Users time ratio");
	mvprintw(14, 6, "8.    Users Kb. ratio");
	for (;;) {
		set_color(WHITE, BLACK);
		show_int( 7,31, CFG.iMaxTimeBalance);
		show_int( 8,31, CFG.iMaxTimeWithdraw);
		show_int( 9,31, CFG.iMaxTimeDeposit);
		show_int(10,31, CFG.iMaxByteBalance);
		show_int(11,31, CFG.iMaxByteWithdraw);
		show_int(12,31, CFG.iMaxByteDeposit);
		show_str(13,31,6, CFG.sTimeRatio);
		show_str(14,31,6, CFG.sByteRatio);

		switch(select_menu(8)) {
		case 0:	return;
		case 1:	E_INT( 7,31,   CFG.iMaxTimeBalance,  "Maximum ^time balance^")
		case 2:	E_INT( 8,31,   CFG.iMaxTimeWithdraw, "Maximum ^time withdraw^")
		case 3: E_INT( 9,31,   CFG.iMaxTimeDeposit,  "Maximum ^time deposit^")
		case 4: E_INT(10,31,   CFG.iMaxByteBalance,  "Maximum ^bytes balance^")
		case 5: E_INT(11,31,   CFG.iMaxByteWithdraw, "Maximum ^bytes withdraw^")
		case 6: E_INT(12,31,   CFG.iMaxByteDeposit,  "Maximum ^bytes deposit^")
		case 7: E_STR(13,31,6, CFG.sTimeRatio,       "^Time ratio^")
		case 8: E_STR(14,31,6, CFG.sByteRatio,       "^Bytes ratio^")
		}
	};
}



void e_paging(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "1.12  EDIT SYSOP PAGING");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.    Ext. Chat");
	mvprintw( 8, 2, "2.    Chat Device");
	mvprintw( 9, 2, "3.    Call Script");
	mvprintw(10, 2, "4.    Page Length");
	mvprintw(11, 2, "5.    Page Times");
	mvprintw(12, 2, "6.    Sysop Area");
	mvprintw(13, 2, "7.    Ask Reason");
	mvprintw(14, 2, "8.    Use Extern");
	mvprintw(15, 2, "9.    Log Chat");
	mvprintw(16, 2, "10.   Prompt Chk.");
	mvprintw(17, 2, "11.   Freeze Time");

	mvprintw(11,42, "12.   Sunday");
	mvprintw(12,42, "13.   Monday");
	mvprintw(13,42, "14.   Tuesday");
	mvprintw(14,42, "15.   Wednesday");
	mvprintw(15,42, "16.   Thursday");
	mvprintw(16,42, "17.   Friday");
	mvprintw(17,42, "18.   Saterday");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,20,49, CFG.sExternalChat); 
		show_str( 8,20,19, CFG.sChatDevice);
		show_str( 9,20,50, CFG.sCallScript);
		show_int(10,20, CFG.iPageLength);
		show_int(11,20, CFG.iMaxPageTimes);
		show_int(12,20, CFG.iSysopArea);
		show_bool(13,20, CFG.iAskReason);
		show_bool(14,20, CFG.iExternalChat);
		show_bool(15,20, CFG.iAutoLog);
		show_bool(16,20, CFG.iChatPromptChk);
		show_bool(17,20, CFG.iStopChatTime);

		show_str(11,58,5, CFG.cStartTime[0]);
		show_str(12,58,5, CFG.cStartTime[1]);
		show_str(13,58,5, CFG.cStartTime[2]);
		show_str(14,58,5, CFG.cStartTime[3]);
		show_str(15,58,5, CFG.cStartTime[4]);
		show_str(16,58,5, CFG.cStartTime[5]);
		show_str(17,58,5, CFG.cStartTime[6]);

		show_str(11,65,5, CFG.cStopTime[0]);
		show_str(12,65,5, CFG.cStopTime[1]);
		show_str(13,65,5, CFG.cStopTime[2]);
		show_str(14,65,5, CFG.cStopTime[3]);
		show_str(15,65,5, CFG.cStopTime[4]);
		show_str(16,65,5, CFG.cStopTime[5]);
		show_str(17,65,5, CFG.cStopTime[6]);

		switch(select_menu(18)) {
		case 0:	return;

		case 1:	E_STR(  7,20,49, CFG.sExternalChat,  "The name of the ^External Chat^ program.")
		case 2:	E_STR(  8,20,19, CFG.sChatDevice,    "The ^device^ to use for chat")
		case 3:	E_STR(  9,20,50, CFG.sCallScript,    "The ^Call Script^ to connect to remote sysop")
		case 4:	E_INT( 10,20,    CFG.iPageLength,    "The ^Length^ of paging in seconds")
		case 5:	E_INT( 11,20,    CFG.iMaxPageTimes,  "The ^Maximum times^ a user may page in a session")
		case 6:	E_INT( 12,20,    CFG.iSysopArea,     "The ^Message Area^ for ^Message to sysop^ when page fails")
		case 7:	E_BOOL(13,20,    CFG.iAskReason,     "Ask the user the ^reason for chat^")
		case 8:	E_BOOL(14,20,    CFG.iExternalChat,  "Use ^External Chat^ program")
		case 9:	E_BOOL(15,20,    CFG.iAutoLog,       "^Automatic log^ chat sessions")
		case 10:E_BOOL(16,20,    CFG.iChatPromptChk, "Check for chat at the ^prompt^")
		case 11:E_BOOL(17,20,    CFG.iStopChatTime,  "^Stop^ users time during chat")
		case 12:strcpy(CFG.cStartTime[0], edit_str(11,58,5, CFG.cStartTime[0], (char *)"Start Time paging on ^Sunday^"));
			strcpy(CFG.cStopTime[0],  edit_str(11,65,5, CFG.cStopTime[0],  (char *)"Stop Time paging on ^Sunday^"));
			break;
		case 13:strcpy(CFG.cStartTime[1], edit_str(12,58,5, CFG.cStartTime[1], (char *)"Start Time paging on ^Monday^"));
			strcpy(CFG.cStopTime[1],  edit_str(12,65,5, CFG.cStopTime[1],  (char *)"Stop Time paging on ^Monday^"));
			break;
		case 14:strcpy(CFG.cStartTime[2], edit_str(13,58,5, CFG.cStartTime[2], (char *)"Start Time paging on ^Tuesday^"));
			strcpy(CFG.cStopTime[2],  edit_str(13,65,5, CFG.cStopTime[2],  (char *)"Stop Time paging on ^Tuesday^"));
			break;
		case 15:strcpy(CFG.cStartTime[3], edit_str(14,58,5, CFG.cStartTime[3], (char *)"Start Time paging on ^Wednesday^"));
			strcpy(CFG.cStopTime[3],  edit_str(14,65,5, CFG.cStopTime[3],  (char *)"Stop Time paging on ^Wednesday^"));
			break;
		case 16:strcpy(CFG.cStartTime[4], edit_str(15,58,5, CFG.cStartTime[4], (char *)"Start Time paging on ^Thursday^"));
			strcpy(CFG.cStopTime[4],  edit_str(15,65,5, CFG.cStopTime[4],  (char *)"Stop Time paging on ^Thursday^"));
			break;
		case 17:
			strcpy(CFG.cStartTime[5], edit_str(16,58,5, CFG.cStartTime[5], (char *)"Start Time paging on ^Friday^"));
			strcpy(CFG.cStopTime[5],  edit_str(16,65,5, CFG.cStopTime[5],  (char *)"Stop Time paging on ^Friday^"));
			break;
		case 18:strcpy(CFG.cStartTime[6], edit_str(17,58,5, CFG.cStartTime[6], (char *)"Start Time paging on ^Saterday^"));
			strcpy(CFG.cStopTime[6],  edit_str(17,65,5, CFG.cStopTime[6],  (char *)"Stop Time paging on ^Saterday^"));
			break;
		}
	};
}



void e_flags(int Users)
{
    int	    i, x, y, z;
    char    temp[80];

    clr_index();
    set_color(WHITE, BLACK);
    if (Users)
	mvprintw( 5, 6, "1.6   EDIT USER FLAG DESCRIPTIONS");
    else
	mvprintw( 5, 6, "1.20  EDIT MANAGER FLAG DESCRIPTIONS");
    
    set_color(CYAN, BLACK);
    for (i = 0; i < 32; i++) {
	if (i < 11) 
	    mvprintw(i + 7, 2, (char *)"%d.", i+1);
	else
	    if (i < 22) 
		mvprintw(i - 4, 28, (char *)"%d.", i+1);
	    else
		mvprintw(i - 15, 54, (char *)"%d.", i+1);
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

	sprintf(temp, "Enter a short ^description^ of flag bit %d", z);
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
    mvprintw( 5, 6, "1.13   EDIT FILEECHO PROCESSING");
    set_color(CYAN, BLACK);

    mvprintw( 7, 2, "1.  Keep days");
    mvprintw( 8, 2, "2.  Hatch pwd");
    mvprintw( 9, 2, "3.  Drv space");
    mvprintw(10, 2, "4.  Systems");
    mvprintw(11, 2, "5.  Groups");
    mvprintw(12, 2, "6.  Max. dupes");
    mvprintw(13, 2, "7.  Keep date");
    mvprintw(14, 2, "8.  Keep netm");
    mvprintw(15, 2, "9.  Loc resp");
	
    mvprintw( 7,42, "10. Plus all");
    mvprintw( 8,42, "11. Notify");
    mvprintw( 9,42, "12. Passwd");
    mvprintw(10,42, "13. Message");
    mvprintw(11,42, "14. Tic on/off");
    mvprintw(12,42, "15. Pause");

    for (;;) {
	set_color(WHITE, BLACK);

	show_int( 7,18, CFG.tic_days);
	show_str( 8,18,20, (char *)"********************");
	show_int( 9,18, CFG.drspace);
	show_int(10,18, CFG.tic_systems);
	show_int(11,18, CFG.tic_groups);
	show_int(12,18, CFG.tic_dupes);
	show_bool(13,18, CFG.ct_KeepDate);
	show_bool(14,18, CFG.ct_KeepMgr);
	show_bool(15,18, CFG.ct_LocalRep);
		
	show_bool( 7,58, CFG.ct_PlusAll);
	show_bool( 8,58, CFG.ct_Notify);
	show_bool( 9,58, CFG.ct_Passwd);
	show_bool(10,58, CFG.ct_Message);
	show_bool(11,58, CFG.ct_TIC);
	show_bool(12,58, CFG.ct_Pause);  

	switch(select_menu(15)) {
	    case 0:	return;

	    case 1: E_INT(  7,18,    CFG.tic_days,     "Number of days to ^keep^ files on hold.")
	    case 2: E_STR(  8,18,20, CFG.hatchpasswd,  "Enter the internal ^hatch^ password.")
	    case 3: E_INT(  9,18,    CFG.drspace,      "Enter the minimal ^free drivespace^ in KBytes.")
	    case 4: temp = CFG.tic_systems;
		    temp = edit_int(10,18, temp, (char *)"Enter the maximum number of ^connected systems^ in the database.");
		    if (temp < CountNoderec()) {
			errmsg("You have %d nodes defined", CountNoderec());
			show_int(10,18, CFG.tic_systems);
		    } else {
			CFG.tic_systems = temp;
			if ((OpenTicarea() == 0))
			    CloseTicarea(TRUE);
			working(0, 0, 0);
		    }
		    break;
	    case 5: temp =  CFG.tic_groups;
		    temp = edit_int(11,18, temp, (char *)"Enter the maximum number of ^fileecho groups^ in the database.");
		    if (temp < CountFGroup()) {
			errmsg("You have %d groups defined", CountFGroup());
			show_int(11,18, CFG.tic_groups);
		    } else {
			CFG.tic_groups = temp;
			if ((OpenNoderec() == 0))
			    CloseNoderec(TRUE);
			working(0, 0, 0);
		    }
		    break;
	    case 6: E_INT( 12,18,    CFG.tic_dupes,    "Enter the maximum number of ^dupes^ in the dupe database.")

	    case 7: E_BOOL(13,18,    CFG.ct_KeepDate,  "^Keep^ original filedate on import")
	    case 8: E_BOOL(14,18,    CFG.ct_KeepMgr,   "Keep ^Areamgr^ netmails.")
	    case 9: E_BOOL(15,18,    CFG.ct_LocalRep,  "Respond to local ^filesearch^ requests.")
	    case 10:E_BOOL( 7,58,    CFG.ct_PlusAll,   "Allow ^+%*^ (Plus all) in FileMgr requests.")
	    case 11:E_BOOL( 8,58,    CFG.ct_Notify,    "Allow turning ^Notify^ messages on or off.")
	    case 12:E_BOOL( 9,58,    CFG.ct_Passwd,    "Allow changing the AreaMgr/FileMgr ^password^.")
	    case 13:E_BOOL(10,58,    CFG.ct_Message,   "Allow turning FileMgr ^messages^ on or off.")
	    case 14:E_BOOL(11,58,    CFG.ct_TIC,       "Allow turning ^TIC^ files on or off.")
	    case 15:E_BOOL(12,58,    CFG.ct_Pause,     "Allow the ^Pause^ FileMgr command.")
	}
    }
}



void s_fidomailcfg(void);
void s_fidomailcfg(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 5, "1.14   EDIT FIDONET MAIL AND ECHOMAIL PROCESSING");
    set_color(CYAN, BLACK);
    mvprintw( 7, 2, "1. Badboard");
    mvprintw( 8, 2, "2. Dupeboard");
    mvprintw( 9, 2, "3. Pktdate");
    mvprintw(10, 2, "4. Max pkts.");
    mvprintw(11, 2, "5. Max arcs.");
    mvprintw(12, 2, "6. Keep days");
    mvprintw(13, 2, "7. Echo dupes");
    mvprintw(14, 2, "8. Reject old");
    mvprintw(15, 2, "9. Max msgs");
    mvprintw(16, 1, "10. Days old");
    mvprintw(17, 1, "11. Max systems");
    mvprintw(18, 1, "12. Max groups");
    
    mvprintw(12,42, "13. 4d address");
    mvprintw(13,42, "14. Split at");
    mvprintw(14,42, "15. Force at");
    mvprintw(15,42, "16. Allow %+*");
    mvprintw(16,42, "17. Notify");
    mvprintw(17,42, "18. Passwd");
    mvprintw(18,42, "19. Pause");

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
			CFG.toss_systems = temp;
			if ((OpenMsgarea() == 0))
			    CloseMsgarea(TRUE);
			working(0, 0, 0);
		    }
		    break;
	    case 12:temp = CFG.toss_groups;
		    temp = edit_int(18,16, temp, (char *)"The maximum number of ^groups^ in the database.");
		    if (temp < CountMGroup()) {
			errmsg("You have %d groups defined", CountMGroup());
			show_int( 18,16, CFG.toss_groups);
		    } else {
			CFG.toss_groups = temp;
			if ((OpenNoderec() == 0))
			    CloseNoderec(TRUE);
			working(0, 0, 0);
		    }
		    break;
	    case 13:E_BOOL(12,58, CFG.addr4d,          "Use ^4d^ addressing instead of ^5d^ addressing.")
	    case 14:E_INT( 13,58, CFG.new_split,       "Gently ^split^ newfiles reports after n kilobytes (12..60).")
	    case 15:E_INT( 14,58, CFG.new_force,       "Force ^split^ of newfiles reports after n kilobytes (16..64).")
	    case 16:E_BOOL(15,58, CFG.ca_PlusAll,      "Allow ^+%*^ (Plus all) in AreaMgr requests.")
	    case 17:E_BOOL(16,58, CFG.ca_Notify,       "Allow turning ^Notify^ messages on or off.")
	    case 18:E_BOOL(17,58, CFG.ca_Passwd,       "Allow changing the AreaMgr/FileMgr ^password^.")
	    case 19:E_BOOL(18,58, CFG.ca_Pause,        "Allow the ^Pause^ AreaMgr command.")
	}
    }
}



void s_intmailcfg(void);
void s_intmailcfg(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 5, 5, "1.15   EDIT INTERNET MAIL AND NEWS PROCESSING");
        set_color(CYAN, BLACK);
        mvprintw( 7, 2, "1. POP3 node");
        mvprintw( 8, 2, "2. SMTP node");
	switch (CFG.newsfeed) {
		case FEEDINN:	mvprintw( 9, 2, "3. N/A");
        			mvprintw(10, 2, "4. NNTP node");
				mvprintw(11, 2, "5. NNTP m.r.");
				mvprintw(12, 2, "6. NNTP user");
				mvprintw(13, 2, "7. NNTP pass");
				break;
		case FEEDRNEWS: mvprintw( 9, 2, "3. Path rnews");
				mvprintw(10, 2, "4. N/A");
				mvprintw(11, 2, "5. N/A");
                                mvprintw(12, 2, "6. N/A");
                                mvprintw(13, 2, "7. N/A");
				break;
		case FEEDUUCP:	mvprintw( 9, 2, "3. UUCP path");
				mvprintw(10, 2, "4. UUCP node");
                                mvprintw(11, 2, "5. N/A");
                                mvprintw(12, 2, "6. N/A");
                                mvprintw(13, 2, "7. N/A");
				break;
	}
        mvprintw(14, 2, "8. News dupes");
        mvprintw(15, 2, "9. Email aka");
        mvprintw(16, 1, "10. UUCP aka");
        mvprintw(17, 1, "11. Emailmode");

	mvprintw(12,42, "12. Articles");
        mvprintw(13,42, "13. News mode");
        mvprintw(14,42, "14. Split at");
        mvprintw(15,42, "15. Force at");
        mvprintw(16,42, "16. Control ok");
        mvprintw(17,42, "17. No regate");

        set_color(WHITE, BLACK);
        show_str( 7,16,64, CFG.popnode);
        show_str( 8,16,64, CFG.smtpnode);
        show_str( 9,16,64, CFG.rnewspath);
	show_str(10,16,64, CFG.nntpnode);
        show_bool(11,16,   CFG.modereader);
        show_str(12,16,15, CFG.nntpuser);
        show_str(13,16,15, (char *)"**************");

        show_int(14,16,    CFG.nntpdupes);
        show_aka(15,16,    CFG.EmailFidoAka);
        show_aka(16,16,    CFG.UUCPgate);
        show_emailmode(17,16, CFG.EmailMode);

	show_int( 12,57, CFG.maxarticles);
	show_newsmode(13,57, CFG.newsfeed);
        show_int( 14,57, CFG.new_split);
        show_int( 15,57, CFG.new_force);
        show_bool(16,57, CFG.allowcontrol);
        show_bool(17,57, CFG.dontregate);
}



/*
 * Edit UUCP gateway, return -1 if there are errors, 0 if ok.
 */
void e_uucp(void)
{
        int     j;

        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 5, 6, "1.15  EDIT UUCP GATEWAY");
        set_color(CYAN, BLACK);
        mvprintw( 7, 6, "1.    Zone");
        mvprintw( 8, 6, "2.    Net");
        mvprintw( 9, 6, "3.    Node");
        mvprintw(10, 6, "4.    Point");
        mvprintw(11, 6, "5.    Domain");

        for (;;) {
                set_color(WHITE, BLACK);
                show_int( 7,19, CFG.UUCPgate.zone);
                show_int( 8,19, CFG.UUCPgate.net);
                show_int( 9,19, CFG.UUCPgate.node);
                show_int(10,19, CFG.UUCPgate.point);
                show_str(11,19,12, CFG.UUCPgate.domain);

                j = select_menu(5);
                switch(j) {
                        case 0: return;
                        case 1: E_INT(  7,19,    CFG.UUCPgate.zone,   "The ^zone^ number for the UUCP gateway")
                        case 2: E_INT(  8,19,    CFG.UUCPgate.net,    "The ^Net^ number for the UUCP gateway")
                        case 3: E_INT(  9,19,    CFG.UUCPgate.node,   "The ^Node^ number for the UUCP gateway")
                        case 4: E_INT( 10,19,    CFG.UUCPgate.point,  "The ^Point^ number for the UUCP gateway")
                        case 5: E_STR( 11,19,11, CFG.UUCPgate.domain, "The ^FTN Domain^ for the UUCP gateway without a dot")
                }
        }
}



void e_intmailcfg(void)
{
        int     tmp;

        s_intmailcfg();
        for (;;) {
                switch(select_menu(17)) {
                case 0: return;
                case 1: E_STR(  7,16,64, CFG.popnode,      "The ^FQDN^ of the node where the ^POP3^ server runs.")
                case 2: E_STR(  8,16,64, CFG.smtpnode,     "The ^FQDN^ of the node where the ^SMTP^ server runs.")
		case 3: if (CFG.newsfeed == FEEDRNEWS)
				strcpy(CFG.rnewspath, edit_pth(9,16,64, CFG.rnewspath, (char *)"The path and filename to the ^rnews^ command.", 0775));
			if (CFG.newsfeed == FEEDUUCP)
				strcpy(CFG.rnewspath, edit_pth(9,16,64, CFG.rnewspath, (char *)"The path to the ^uucppublic^ directory.", 0775));
			break;
                case 4: if (CFG.newsfeed == FEEDINN)
				strcpy(CFG.nntpnode, edit_str(10,16,64, CFG.nntpnode, (char *)"The ^FQDN^ of the node where the ^NNTP^ server runs."));
			if (CFG.newsfeed == FEEDUUCP)
				strcpy(CFG.nntpnode, edit_str(10,16,64, CFG.nntpnode, (char *)"The ^UUCP^ nodename of the remote UUCP system"));
			break;
                case 5: E_BOOL(11,16,    CFG.modereader,   "Does the NNTP server needs the ^Mode Reader^ command.")
                case 6: E_STR( 12,16,15, CFG.nntpuser,     "The ^Username^ for the NNTP server if needed.")
                case 7: E_STR( 13,16,15, CFG.nntppass,     "The ^Password^ for the NNTP server if needed.")
                case 8: E_INT( 14,16,    CFG.nntpdupes,    "The number of ^dupes^ to store in the news articles dupes database.")
                case 9: tmp = PickAka((char *)"1.15.9", FALSE);
                        if (tmp != -1)
                                CFG.EmailFidoAka = CFG.aka[tmp];
                        s_intmailcfg();
                        break;
                case 10:e_uucp();
                        s_intmailcfg();
                        break;
                case 11:CFG.EmailMode = edit_emailmode(17,16, CFG.EmailMode);
                        s_intmailcfg();
                        break;

		case 12:E_INT( 12,57, CFG.maxarticles,    "Default maximum ^news articles^ to fetch")
		case 13:CFG.newsfeed = edit_newsmode(13,57, CFG.newsfeed);
			s_intmailcfg();
			break;
                case 14:E_INT( 14,57, CFG.new_split,       "Gently ^split^ messages after n kilobytes (12..60).")
                case 15:E_INT( 15,57, CFG.new_force,       "Force ^split^ of messages after n kilobytes (16..64).")
                case 16:E_BOOL(16,57, CFG.allowcontrol,    "^Allow control^ messages for news to be gated.")
                case 17:E_BOOL(17,57, CFG.dontregate,      "Don't ^regate^ already gated messages.")
                }
        };
}



void s_newfiles(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "1.16 ALLFILES & NEWFILES LISTINGS");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.   New days");
	mvprintw( 8, 2, "2.   Security");
	mvprintw( 9, 2, "3.   Groups");
}



void e_newfiles(void)
{
    int	temp;
	
    s_newfiles();
    for (;;) {
	set_color(WHITE, BLACK);
	show_int( 7,16, CFG.newdays);
	show_sec( 8,16, CFG.security);
	show_int( 9,16, CFG.new_groups);

	switch(select_menu(3)) {
	    case 0: return;
	    case 1: E_INT(7,16,    CFG.newdays,    "Add files younger than this in newfiles report.")
	    case 2: E_SEC(8,16,    CFG.security,   "1.16  NEWFILES REPORTS SECURITY", s_newfiles)
	    case 3: temp = CFG.new_groups;
		    temp = edit_int( 9, 16, temp, (char *)"The maximum of ^newfiles^ groups in the newfiles database");
		    if (temp < CountNewfiles()) {
			errmsg("You have %d newfiles reports defined", CountNewfiles());
			show_int( 9,16, CFG.new_groups);
		    } else {
			CFG.new_groups = temp;
			if (OpenNewfiles() == 0)
			    CloseNewfiles(TRUE);
			working(0, 0, 0);
		    }
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
	mvprintw( 5, 6, "1.1   EDIT AKA");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.    Zone");
	mvprintw( 8, 6, "2.    Net");
	mvprintw( 9, 6, "3.    Node");
	mvprintw(10, 6, "4.    Point");
	mvprintw(11, 6, "5.    Domain");
	mvprintw(12, 6, "6.    Active");

	for (;;) {
		set_color(WHITE, BLACK);
		show_int( 7,19, CFG.aka[Area].zone);
		show_int( 8,19, CFG.aka[Area].net);
		show_int( 9,19, CFG.aka[Area].node);
		show_int(10,19, CFG.aka[Area].point);
		show_str(11,19,12, CFG.aka[Area].domain);
		show_bool(12,19, CFG.akavalid[Area]);

		j = select_menu(6);
		switch(j) {
		case 0:	return;
		case 1: E_INT(  7,19,    CFG.aka[Area].zone,   "The ^zone^ number for this aka")
		case 2:	E_INT(  8,19,    CFG.aka[Area].net,    "The ^Net^ number for this aka")
		case 3:	E_INT(  9,19,    CFG.aka[Area].node,   "The ^Node^ number for this aka")
		case 4:	E_INT( 10,19,    CFG.aka[Area].point,  "The ^Point^ number for this node (if any)")
		case 5:	E_STR( 11,19,11, CFG.aka[Area].domain, "The ^FTN Domain^ for this aka without a dot (ie no .org)")
		case 6:	E_BOOL(12,19,    CFG.akavalid[Area],   "Is this aka ^available^")
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
	mvprintw( 5, 2, "1.1   EDIT FIDONET AKA'S");
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
		    sprintf(temp, "%3d   %s", o+i, aka2str(CFG.aka[o+i-1]));
		    temp[38] = '\0';
		} else
		    sprintf(temp, "%3d", o+i);
		mvprintw(y, x, temp);
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
	    mvprintw(LINES -3, 6, "Enter aka number (1..40) to move >");
	    from = edit_int(LINES -3, 42, from, (char *)"Enter record number");
	    locate(LINES -3, 6);
	    clrtoeol();
	    mvprintw(LINES -3, 6, "Enter new position (1..40) >");
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
	mvprintw( 5, 2, "1.17 EDIT MAILER SETTINGS");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.   Mailer logl.");
	mvprintw( 8, 2, "2.   Default phone");
	mvprintw( 9, 2, "3.   TCP/IP flags");
	mvprintw(10, 2, "4.   Default speed");
	mvprintw(11, 2, "5.   Timeout reset");
	mvprintw(12, 2, "6.   Timeout connect");
	mvprintw(13, 2, "7.   Dial delay");
	mvprintw(14, 2, "8.   No Filerequests");
	mvprintw(15, 2, "9.   No callout");
	mvprintw(16, 2, "10.  No EMSI session");
	mvprintw(17, 2, "11.  No Yooho/2U2");

	mvprintw(15,31, "12.  No Zmodem");
	mvprintw(16,31, "13.  No Zedzap");
	mvprintw(17,31, "14.  No Hydra");

	mvprintw(12,59, "18.  Phonetrans  1-10");
	mvprintw(13,59, "19.  Phonetrans 11-20");
	mvprintw(14,59, "20.  Phonetrans 21-30");
	mvprintw(15,59, "21.  Phonetrans 31-40");
	mvprintw(16,59, "22.  Max. files");
	mvprintw(17,59, "23.  Max. MB.");
}



void e_trans(int start)
{
	int	i, j;
	char	temp[21];

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "1.17  EDIT PHONE TRANSLATION");
	set_color(CYAN, BLACK);
	mvprintw( 7, 12, "String to match       String to replace");
	for (i = 0; i < 10; i++) {
		sprintf(temp, "%2d.", i+1);
		mvprintw( 9+i, 6, temp);
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
		show_str(  8,23,20,CFG.Phone);
		show_str(  9,23,30,CFG.Flags);
		show_int( 10,23,   CFG.Speed);
		show_int( 11,23,   CFG.timeoutreset);
		show_int( 12,23,   CFG.timeoutconnect);
		show_int( 13,23,   CFG.dialdelay);
		show_bool(14,23,   CFG.NoFreqs);
		show_bool(15,23,   CFG.NoCall);
		show_bool(16,23,   CFG.NoEMSI);
		show_bool(17,23,   CFG.NoWazoo);

		show_bool(15,52, CFG.NoZmodem);
		show_bool(16,52, CFG.NoZedzap);
		show_bool(17,52, CFG.NoHydra);

		show_int( 16,75, CFG.Req_Files);
		show_int( 17,75, CFG.Req_MBytes);

		switch(select_menu(23)) {
		case 0:	return;
		case 1: E_LOGL(CFG.cico_loglevel, "1.17.1", s_mailer)
		case 2: E_STR(  8,23,20,CFG.Phone,          "The mailer default ^phone number^ for this system")
		case 3: E_STR(  9,23,30,CFG.Flags,          "The mailer ^TCP/IP capability flags^ for this system")
		case 4: E_INT( 10,23,   CFG.Speed,          "The mailer ^default linespeed^ for this system")
		case 5: E_INT( 11,23,   CFG.timeoutreset,   "The modem ^reset timeout^ in seconds")
		case 6: E_INT( 12,23,   CFG.timeoutconnect, "The modem ^wait for connect timeout^ in seconds")
		case 7: E_INT( 13,23,   CFG.dialdelay,      "The ^random dialdelay^ in seconds ((^n^ <= delay) and (^n^ > (delay / 10)))")
		case 8: E_BOOL(14,23,   CFG.NoFreqs,        "Set to true if ^No Filerequests^ are allowed")
		case 9: E_BOOL(15,23,   CFG.NoCall,         "Set to true if ^No Calls^ are allowed")
		case 10:E_BOOL(16,23,   CFG.NoEMSI,         "If set then ^EMSI handshake^ is diabled")
		case 11:E_BOOL(17,23,   CFG.NoWazoo,        "If set then ^YooHoo/2U2^ (FTSC-0006) is disabled")

		case 12:E_BOOL(15,52,   CFG.NoZmodem,       "If set then the ^Zmodem^ protocol is disabled")
		case 13:E_BOOL(16,52,   CFG.NoZedzap,       "If set then the ^Zedzap^ protocol is disabled")
		case 14:E_BOOL(17,52,   CFG.NoHydra,        "If set then the ^Hydra^ protocol is disabled")

		case 18:e_trans(0);
			break;
		case 19:e_trans(10);
			break;
		case 20:e_trans(20);
			break;
		case 21:e_trans(30);
			break;
		case 22:E_INT(16,75,    CFG.Req_Files,       "Maximum ^files^ to request, 0 is unlimited")
		case 23:E_INT(17,75,    CFG.Req_MBytes,      "Maximum ^MBytes^ to request, 0 is unlimited")
		}
	};
}



void e_ftpd(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 5, 2, "1.18 EDIT FTPD SETTINGS");
        set_color(CYAN, BLACK);
        mvprintw( 7, 2, "1.  Upload pth");
        mvprintw( 8, 2, "2.  Banner msg");
        mvprintw( 9, 2, "3.  Pth filter");
        mvprintw(10, 2, "4.  Pth msg");
        mvprintw(11, 2, "5.  Email addr");
        mvprintw(12, 2, "6.  Shutdown");
	mvprintw(13, 2, "7.  Rdm login");
	mvprintw(14, 2, "8.  Rdm cwd*");
	mvprintw(15, 2, "9.  Msg login");
	mvprintw(16, 1,"10.  Msg cwd*");
	mvprintw(17, 1,"11.  Userslimit");
	mvprintw(18, 1,"12.  Loginfails");

	mvprintw(13,60,"13.  Compress");
	mvprintw(14,60,"14.  Tar");
	mvprintw(15,60,"15.  Mkdir ok");
	mvprintw(16,60,"16.  Log cmds");
	mvprintw(17,60,"17.  Anonymous");
	mvprintw(18,60,"18.  User mbse");

	set_color(WHITE, BLACK);
	show_str( 7,18,59, CFG.ftp_upl_path);
	show_str( 8,18,59, CFG.ftp_banner);
	show_str( 9,18,40, CFG.ftp_pth_filter);
	show_str(10,18,59, CFG.ftp_pth_message);
	show_str(11,18,40, CFG.ftp_email);
	show_str(12,18,40, CFG.ftp_msg_shutmsg);
	show_str(13,18,20, CFG.ftp_readme_login);
	show_str(14,18,20, CFG.ftp_readme_cwd);
	show_str(15,18,20, CFG.ftp_msg_login);
	show_str(16,18,20, CFG.ftp_msg_cwd);
	show_int(17,18,    CFG.ftp_limit);
	show_int(18,18,    CFG.ftp_loginfails);

	show_bool(13,75, CFG.ftp_compress);
	show_bool(14,75, CFG.ftp_tar);
	show_bool(15,75, CFG.ftp_upl_mkdir);
	show_bool(16,75, CFG.ftp_log_cmds);
	show_bool(17,75, CFG.ftp_anonymousok);
	show_bool(18,75, CFG.ftp_mbseok);

        for (;;) {
                set_color(WHITE, BLACK);

                switch(select_menu(18)) {
                case 0: return;
		case 1: E_STR( 7,18,59, CFG.ftp_upl_path,    "Public ^upload^ path, must be in the base path")
		case 2: E_STR( 8,18,59, CFG.ftp_banner,      "^Banner^ file to show before login")
		case 3: E_STR( 9,18,40, CFG.ftp_pth_filter,  "^Filter^ with allowed characters in upload filename")
		case 4: E_STR(10,18,59, CFG.ftp_pth_message, "^Message^ to display if illegal characters in filename")
		case 5:	E_STR(11,18,40, CFG.ftp_email,       "^Email^ address of the ftp server administrator")
		case 6:	E_STR(12,18,40, CFG.ftp_msg_shutmsg, "^Shutdown message^, if this file is present, login if forbidden")
		case 7: E_STR(13,18,20, CFG.ftp_readme_login,"^README^ file to display at login")
		case 8: E_STR(14,18,20, CFG.ftp_readme_cwd,  "^README^ file to display when entering a new directory")
		case 9: E_STR(15,18,20, CFG.ftp_msg_login,   "^Message^ file to display at login")
		case 10:E_STR(16,18,20, CFG.ftp_msg_cwd,     "^Message^ file to display when entering a new directory")
		case 11:E_INT(17,18,    CFG.ftp_limit,       "^Limit^ the number of concurent ftp users")
		case 12:E_INT(18,18,    CFG.ftp_loginfails,  "Maximum ^login fails^ before a user is disconnected")
		case 13:E_BOOL(13,75,   CFG.ftp_compress,    "Allow the use of the ^compress^ command")
		case 14:E_BOOL(14,75,   CFG.ftp_tar,         "Allow the use if the ^tar^ command")
		case 15:E_BOOL(15,75,   CFG.ftp_upl_mkdir,   "Allow ^mkdir^ in the upload directory")
		case 16:E_BOOL(16,75,   CFG.ftp_log_cmds,    "^Log^ all user ^commands^")
		case 17:E_BOOL(17,75,   CFG.ftp_anonymousok, "Allow ^anonymous^ users to login")
		case 18:E_BOOL(18,75,   CFG.ftp_mbseok,      "Allow the ^mbse^ user to login")
                }
        };
}



void e_html(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 5, 2, "1.19 EDIT HTML SETTINGS");
        set_color(CYAN, BLACK);
        mvprintw( 7, 2, "1.  Docs root");
        mvprintw( 8, 2, "2.  Link to ftp");
        mvprintw( 9, 2, "3.  URL name");
        mvprintw(10, 2, "4.  Charset");
        mvprintw(11, 2, "5.  Author name");
        mvprintw(12, 2, "6.  Convert cmd");
	mvprintw(13, 2, "7.  Files/page");

        set_color(WHITE, BLACK);
        show_str( 7,18,59, CFG.www_root);
        show_str( 8,18,20, CFG.www_link2ftp);
        show_str( 9,18,40, CFG.www_url);
        show_str(10,18,20, CFG.www_charset);
        show_str(11,18,40, CFG.www_author);
        show_str(12,18,59, CFG.www_convert);
	show_int(13,18,    CFG.www_files_page);

        for (;;) {
                set_color(WHITE, BLACK);

                switch(select_menu(7)) {
                case 0: return;
                case 1: E_STR( 7,18,59, CFG.www_root,       "The ^Document root^ of your http server")
                case 2: E_STR( 8,18,20, CFG.www_link2ftp,   "The ^link^ name from the Document root to the FTP base directory")
                case 3: E_STR( 9,18,40, CFG.www_url,        "The ^URL^ name of your http server")
                case 4: E_STR(10,18,20, CFG.www_charset,    "The ^ISO character set^ name to include in the web pages")
                case 5: E_STR(11,18,40, CFG.www_author,     "The ^Author name^ to include in the http headers")
                case 6: E_STR(12,18,59, CFG.www_convert,    "The ^convert^ command to create thumbnails")
		case 7: E_INT(13,18,    CFG.www_files_page, "The number of files on each web page")
                }
        };
}



void global_menu(void)
{
    unsigned long   crc, crc1;
    int		    i;

    if (! check_free())
	return;

    if (cf_open() == -1)
	return;

    Syslog('+', "Opened main config");
    crc = 0xffffffff;
    crc = upd_crc32((char *)&CFG, crc, sizeof(CFG));

    if (CFG.xmax_login) {
	/*
	 *  Do automatic upgrade for unused fields, erase them.
	 */
	Syslog('+', "Main config, clearing unused fields");
	memset(&CFG.alists_path, 0, sizeof(CFG.alists_path));
	CFG.xmax_login = 0;
	CFG.xUseSysDomain = FALSE;
	CFG.xChkMail = FALSE;
	memset(&CFG.xquotestr, 0, sizeof(CFG.xquotestr));
	CFG.xNewBytes = FALSE;
	memset(&CFG.extra4, 0, sizeof(CFG.extra4));
	memset(&CFG.xmgrname, 0, sizeof(CFG.xmgrname));
	memset(&CFG.xtoss_log, 0, sizeof(CFG.xtoss_log));
	memset(&CFG.xareamgr, 0, sizeof(CFG.xareamgr));
	CFG.xNoJanus = FALSE;
	memset(&CFG.extra5, 0, sizeof(CFG.extra5));
	sprintf(CFG.alists_path, "%s/var/arealists", getenv("MBSE_ROOT"));
    }

    if (strlen(CFG.bbs_macros) == 0) {
	sprintf(CFG.bbs_macros, "%s/english/macro", getenv("MBSE_ROOT"));
	 Syslog('+', "Main config, upgraded default macro path");
    }

    if (strlen(CFG.out_queue) == 0) {
	sprintf(CFG.out_queue, "%s/var/queue", getenv("MBSE_ROOT"));
	 Syslog('+', "Main config, upgraded for new queue");
    }

    if (strlen(CFG.mgrlog) == 0) {
	sprintf(CFG.mgrlog, "manager.log");
	for (i = 0; i < 32; i++)
	    sprintf(CFG.aname[i], "Flags %d", i+1);
	sprintf(CFG.aname[0], "Everyone");
	Syslog('+', "Main config, upgraded for manager security");
    }

    if (!CFG.ca_PlusAll && !CFG.ca_Notify && !CFG.ca_Passwd && !CFG.ca_Pause && !CFG.ca_Check) {
	CFG.ca_PlusAll = TRUE;
	CFG.ca_Notify  = TRUE;
	CFG.ca_Passwd  = TRUE;
	CFG.ca_Pause   = TRUE;
	CFG.ca_Check   = TRUE;
	Syslog('+', "Main config, upgraded for AreaMgr flags");
    }


    for (;;) {

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "1.    GLOBAL SETUP");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.    Edit Fidonet Aka's");
	mvprintw( 8, 6, "2.    Edit Registration Info");
	mvprintw( 9, 6, "3.    Edit Global Filenames");
	mvprintw(10, 6, "4.    Edit Global Paths");
	mvprintw(11, 6, "5.    Edit Global Settings");
	mvprintw(12, 6, "6.    Edit User flag Descriptions");
	mvprintw(13, 6, "7.    Edit New Users defaults");
	mvprintw(14, 6, "8.    Edit Text Colors");
	mvprintw(15, 6, "9.    Edit Next User Door");
	mvprintw(16, 6, "10.   Edit Safe Door");

	mvprintw( 7,46, "11.   Edit Time Bank Door");
	mvprintw( 8,46, "12.   Edit Sysop Paging");
	mvprintw( 9,46, "13.   Edit Files Processing");
	mvprintw(10,46, "14.   Edit Fidonet Mail/Echomail");
	mvprintw(11,46, "15.   Edit Internet Mail/News");
	mvprintw(12,46, "16.   Edit All-/Newfiles lists");
	mvprintw(13,46, "17.   Edit Mailer global setup");
	mvprintw(14,46, "18.   Edit Ftp daemon setup");
	mvprintw(15,46, "19.   Edit HTML pages setup");
	mvprintw(16,46, "20.   Edit Mgr flag descriptions");

	switch(select_menu(20)) {
	    case 0:
		    crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&CFG, crc1, sizeof(CFG));
		    if (crc != crc1) {
			if (yes_no((char *)"Configuration is changed, save") == 1) {
			    cf_close();
			    Syslog('+', "Saved main config");
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
		    e_nu_door();
		    break;
	    case 10:
		    e_safe_door();
		    break;
	    case 11:
		    e_timebank();
		    break;
	    case 12:
		    e_paging();
		    break;
	    case 13:
		    e_ticconf();
		    break;
	    case 14:
		    e_fidomailcfg();
		    break;
	    case 15:
		    e_intmailcfg();
		    break;
	    case 16:
		    e_newfiles();
		    break;
	    case 17:
		    e_mailer();
		    break;
	    case 18:
		    e_ftpd();
		    break;
	    case 19:
		    e_html();
		    break;
	    case 20:
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
		sprintf(temp, "%s.   AKA SELECT", msg);
		mvprintw( 5, 4, temp);	
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
					sprintf(temp, "%3d   %s", o+i, aka2str(CFG.aka[o+i-1]));
					temp[38] = '\0';
				} else {
					set_color(LIGHTBLUE, BLACK);
					sprintf(temp, "%3d", o+i);
				}
				mvprintw(y, x, temp);
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



int global_doc(FILE *fp, FILE *toc, int page)
{
	int	i, j;
	struct	utsname	utsbuf;
	time_t	now;
	char	*p;

	if (config_read())
		return page;

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 0, page, (char *)"Global system setup");
	addtoc(fp, toc, 1, 1, page, (char *)"Host system information");

	memset(&utsbuf, 0, sizeof(utsbuf));
	if (uname(&utsbuf) == 0) {
		fprintf(fp, "      Node name        %s\n", utsbuf.nodename);
#ifdef __USE_GNU
		fprintf(fp, "      Domain name      %s\n", utsbuf.domainname);
#else
#ifdef __linux__
		fprintf(fp, "      Domain name      %s\n", utsbuf.__domainname);
#endif
#endif
		fprintf(fp, "      Operating system %s %s\n", utsbuf.sysname, utsbuf.release);
		fprintf(fp, "      Kernel version   %s\n", utsbuf.version);
		fprintf(fp, "      Machine type     %s\n", utsbuf.machine);
	}
	fprintf(fp, "      MBSE_ROOT        %s\n", getenv("MBSE_ROOT"));
	now = time(NULL);
	fprintf(fp, "      Date created     %s", ctime(&now));

        addtoc(fp, toc, 1, 2, page, (char *)"System fidonet addresses");
	for (i = 0; i < 40; i++)
	    if (CFG.akavalid[i])
		fprintf(fp, "      Aka %2d    %s\n", i+1, aka2str(CFG.aka[i]));

	page = newpage(fp, page);

	addtoc(fp, toc, 1, 3, page, (char *)"Registration information");
	fprintf(fp, "      System name      %s\n", CFG.bbs_name);
	fprintf(fp, "      Domain name      %s\n", CFG.sysdomain);
	fprintf(fp, "      Sysop unix name  %s\n", CFG.sysop);
	fprintf(fp, "      Sysop fido name  %s\n", CFG.sysop_name);
	fprintf(fp, "      System location  %s\n", CFG.location);
	fprintf(fp, "      QWK/Bluewave id  %s\n", CFG.bbsid);
	fprintf(fp, "      Omen id          %s\n", CFG.bbsid2);
	fprintf(fp, "      Comment          %s\n", CFG.comment);
	fprintf(fp, "      Origin line      %s\n", CFG.origin);
	fprintf(fp, "      Start unix name  %s\n", CFG.startname);

	addtoc(fp, toc, 1, 4, page, (char *)"Global filenames");
	fprintf(fp, "      System logfile   %s\n", CFG.logfile);
	fprintf(fp, "      Error logfile    %s\n", CFG.error_log);
	fprintf(fp, "      Manager logfile  %s\n", CFG.mgrlog);
	fprintf(fp, "      Default menu     %s\n", CFG.default_menu);
	fprintf(fp, "      Default language %s\n", CFG.current_language);
	fprintf(fp, "      Chat logfile     %s\n", CFG.chat_log);
	fprintf(fp, "      Welcome logo     %s\n", CFG.welcome_logo);

	addtoc(fp, toc, 1, 5, page, (char *)"Pathnames");
	fprintf(fp, "      Menufiles        %s\n", CFG.bbs_menus);
	fprintf(fp, "      Textfiles        %s\n", CFG.bbs_txtfiles);
	fprintf(fp, "      Macros           %s\n", CFG.bbs_macros);
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

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 6, page, (char *)"Global settings");

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

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 7, page, (char *)"Users flag descriptions");
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

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 8, page, (char *)"New users defaults");

	fprintf(fp, "      Access level     %s\n", get_secstr(CFG.newuser_access));
	fprintf(fp, "      Cap. username    %s\n", getboolean(CFG.iCapUserName));
	fprintf(fp, "      Ask ANSI         %s\n", getboolean(CFG.iAnsi));
	fprintf(fp, "      Ask Sex          %s\n", getboolean(CFG.iSex));
	fprintf(fp, "      Ask voicephone   %s\n", getboolean(CFG.iVoicePhone));
	fprintf(fp, "      Ask dataphone    %s\n", getboolean(CFG.iDataPhone));
	fprintf(fp, "      Telephone scan   %s\n", getboolean(CFG.iTelephoneScan));
	fprintf(fp, "      Ask handle       %s\n", getboolean(CFG.iHandle));
	fprintf(fp, "      Ask birthdate    %s\n", getboolean(CFG.iDOB));
	fprintf(fp, "      Ask location     %s\n", getboolean(CFG.iLocation));
	fprintf(fp, "      Ask hotkeys      %s\n", getboolean(CFG.iHotkeys));
	fprintf(fp, "      One word names   %s\n", getboolean(CFG.iOneName));
	fprintf(fp, "      Ask address      %s\n", getboolean(CFG.AskAddress));
	fprintf(fp, "      Give email box   %s\n", getboolean(CFG.GiveEmail));

	addtoc(fp, toc, 1, 9, page, (char *)"Text colors");

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

	addtoc(fp, toc, 1, 10, page, (char *)"Next user door");
	
	fprintf(fp, "      Text file        %s\n", CFG.sNuScreen);
	fprintf(fp, "      Quote            %s\n", CFG.sNuQuote);

	addtoc(fp, toc, 1, 11, page, (char *)"Safecracker door");

	fprintf(fp, "      Digit nr 1       %d\n", CFG.iSafeFirstDigit);
	fprintf(fp, "      Digit nr 2       %d\n", CFG.iSafeSecondDigit);
	fprintf(fp, "      Digit nr 3       %d\n", CFG.iSafeThirdDigit);
	fprintf(fp, "      Maximum tries    %d\n", CFG.iSafeMaxTrys);
	fprintf(fp, "      Maximum number   %d\n", CFG.iSafeMaxNumber);
	fprintf(fp, "      Show generator   %s\n", getboolean(CFG.iSafeNumGen));
	fprintf(fp, "      Prize            %s\n", CFG.sSafePrize);
	fprintf(fp, "      Safe welcome     %s\n", CFG.sSafeWelcome);
	fprintf(fp, "      Safe opened file %s\n", CFG.sSafeOpened);

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 12, page, (char *)"Timebank door");

	fprintf(fp, "      Users time balance %d\n", CFG.iMaxTimeBalance);
	fprintf(fp, "      Max. time withdraw %d\n", CFG.iMaxTimeWithdraw);
	fprintf(fp, "      Max. time deposit  %d\n", CFG.iMaxTimeDeposit);
	fprintf(fp, "      Users kb. balance  %d\n", CFG.iMaxByteBalance);
	fprintf(fp, "      Max. Kb. withdraw  %d\n", CFG.iMaxByteWithdraw);
	fprintf(fp, "      Max. Kb. deposit   %d\n", CFG.iMaxByteDeposit);
	fprintf(fp, "      Users time ratio   %s\n", CFG.sTimeRatio);
	fprintf(fp, "      Users Kb. ratio    %s\n", CFG.sByteRatio);

	addtoc(fp, toc, 1, 13, page, (char *)"Sysop paging");

	fprintf(fp, "      Ext. Chat program  %s\n", CFG.sExternalChat);
	fprintf(fp, "      Chat device        %s\n", CFG.sChatDevice);
	fprintf(fp, "      Call sysop script  %s\n", CFG.sCallScript);
	fprintf(fp, "      Page length        %d seconds\n", CFG.iPageLength);
	fprintf(fp, "      Page times         %d\n", CFG.iMaxPageTimes);
	fprintf(fp, "      Sysop msg area     %d\n", CFG.iSysopArea);
	fprintf(fp, "      Ask chat reason    %s\n", getboolean(CFG.iAskReason));
	fprintf(fp, "      Use external chat  %s\n", getboolean(CFG.iExternalChat));
	fprintf(fp, "      Log chat           %s\n", getboolean(CFG.iAutoLog));
	fprintf(fp, "      Check at prompt    %s\n", getboolean(CFG.iChatPromptChk));
	fprintf(fp, "      Freeze online time %s\n", getboolean(CFG.iStopChatTime));

	fprintf(fp, "\n      Weekday            Start Stop\n");
	fprintf(fp, "      -------------      ----- -----\n");
	fprintf(fp, "      Sunday             %s %s\n", CFG.cStartTime[0], CFG.cStopTime[0]);
	fprintf(fp, "      Monday             %s %s\n", CFG.cStartTime[1], CFG.cStopTime[1]);
	fprintf(fp, "      Tuesday            %s %s\n", CFG.cStartTime[2], CFG.cStopTime[2]);
	fprintf(fp, "      Wednesday          %s %s\n", CFG.cStartTime[3], CFG.cStopTime[3]);
	fprintf(fp, "      Thursday           %s %s\n", CFG.cStartTime[4], CFG.cStopTime[4]);
	fprintf(fp, "      Friday             %s %s\n", CFG.cStartTime[5], CFG.cStopTime[5]);
	fprintf(fp, "      Saterday           %s %s\n", CFG.cStartTime[6], CFG.cStopTime[6]);

	addtoc(fp, toc, 1, 14, page, (char *)"Fileecho processing");

	fprintf(fp, "      Keep days on hold  %d\n", CFG.tic_days);
	fprintf(fp, "      Hatch password     %s\n", CFG.hatchpasswd);
	fprintf(fp, "      Free drivespave    %lu\n", CFG.drspace);
	fprintf(fp, "      Max. systems       %ld\n", CFG.tic_systems);
	fprintf(fp, "      Max. groups        %ld\n", CFG.tic_groups);
	fprintf(fp, "      Max. dupes         %ld\n", CFG.tic_dupes);
	fprintf(fp, "      Keep filedate      %s\n", getboolean(CFG.ct_KeepDate));
	fprintf(fp, "      Keep mgr netmail   %s\n", getboolean(CFG.ct_KeepMgr));
	fprintf(fp, "      Local requests     %s\n", getboolean(CFG.ct_LocalRep));
	fprintf(fp, "      FileMgr: allow +%%* %s\n", getboolean(CFG.ct_PlusAll));
	fprintf(fp, "      FileMgr: notify    %s\n", getboolean(CFG.ct_Notify));
	fprintf(fp, "      FileMgr: passwd    %s\n", getboolean(CFG.ct_Passwd));
	fprintf(fp, "      FileMgr: message   %s\n", getboolean(CFG.ct_Message));
	fprintf(fp, "      FileMgr: TIC       %s\n", getboolean(CFG.ct_TIC));
	fprintf(fp, "      FileMgr: pause     %s\n", getboolean(CFG.ct_Pause));

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 15, page, (char *)"Fidonet Mail and Echomail  processing");

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
	fprintf(fp, "      Maximum systems    %ld\n", CFG.toss_systems);
	fprintf(fp, "      Maximum groups     %ld\n", CFG.toss_groups);
	fprintf(fp, "      Use 4d addressing  %s\n", getboolean(CFG.addr4d));
        fprintf(fp, "      AreaMgr: allow +%%* %s\n", getboolean(CFG.ca_PlusAll));
	fprintf(fp, "      AreaMgr: notify    %s\n", getboolean(CFG.ca_Notify));
	fprintf(fp, "      AreaMgr: passwd    %s\n", getboolean(CFG.ca_Passwd));
	fprintf(fp, "      AreaMgr: pause     %s\n", getboolean(CFG.ca_Pause));

	addtoc(fp, toc, 1, 16, page, (char *)"Internet Mail and News processing");

	fprintf(fp, "      Split messages at  %d KBytes\n", CFG.new_split);
	fprintf(fp, "      Force split at     %d KBytes\n", CFG.new_force);
	fprintf(fp, "      ISP Email Mode     %s\n", getemailmode(CFG.EmailMode));
	fprintf(fp, "      Email fido aka     %s\n", aka2str(CFG.EmailFidoAka));
	fprintf(fp, "      UUCP gateway       %s\n", aka2str(CFG.UUCPgate));
	fprintf(fp, "      POP3 host          %s\n", CFG.popnode);
	fprintf(fp, "      SMTP host          %s\n", CFG.smtpnode);
	fprintf(fp, "      News transfermode  %s\n", getnewsmode(CFG.newsfeed));
	switch (CFG.newsfeed) {
	case FEEDINN:	fprintf(fp, "      NNTP host          %s\n", CFG.nntpnode);
			fprintf(fp, "      NNTP mode reader   %s\n", getboolean(CFG.modereader));
			fprintf(fp, "      NNTP username      %s\n", CFG.nntpuser);
			fprintf(fp, "      NNTP password      %s\n", getboolean(strlen(CFG.nntppass)));
			break;
	case FEEDRNEWS:	fprintf(fp, "      Path to rnews      %s\n", CFG.rnewspath);
			break;
	case FEEDUUCP:	fprintf(fp, "      NNTP host          %s\n", CFG.nntpnode);
			fprintf(fp, "      Path to rnews      %s\n", CFG.rnewspath);
			break;
	}
	fprintf(fp, "      Max articles fetch %d\n", CFG.maxarticles);
	fprintf(fp, "      Allow control msgs %s\n", getboolean(CFG.allowcontrol));
	fprintf(fp, "      Don't regate msgs  %s\n", getboolean(CFG.dontregate));

	addtoc(fp, toc, 1, 17, page, (char *)"Newfile reports");

	fprintf(fp, "      New files days     %d\n", CFG.newdays);
	fprintf(fp, "      Highest sec. level %s\n", get_secstr(CFG.security));
	fprintf(fp, "      Max. newfile grps  %ld\n", CFG.new_groups);

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 18, page, (char *)"Mailer setup");

	p = getloglevel(CFG.cico_loglevel);
	fprintf(fp, "      Mailer loglevel    %s\n",  p);
	free(p);
	fprintf(fp, "      Res. modem timeout %ld\n", CFG.timeoutreset);
	fprintf(fp, "      Connect timeout    %ld\n", CFG.timeoutconnect);
	fprintf(fp, "      Random dialdelay   %ld\n", CFG.dialdelay);
	fprintf(fp, "      Default phone nr.  %s\n",  CFG.Phone);
	fprintf(fp, "      Default speed      %lu\n", CFG.Speed);
	fprintf(fp, "      TCP/IP flags       %s\n",  CFG.Flags);
	fprintf(fp, "      No Filerequests    %s\n",  getboolean(CFG.NoFreqs));
	fprintf(fp, "      No Calls           %s\n",  getboolean(CFG.NoCall));
	fprintf(fp, "      No EMSI            %s\n",  getboolean(CFG.NoEMSI));
	fprintf(fp, "      No YooHoo/2U2      %s\n",  getboolean(CFG.NoWazoo));
	fprintf(fp, "      No Zmodem          %s\n",  getboolean(CFG.NoZmodem));
	fprintf(fp, "      No Zedzap          %s\n",  getboolean(CFG.NoZedzap));
	fprintf(fp, "      No Hydra           %s\n",  getboolean(CFG.NoHydra));
	fprintf(fp, "      Max request files  %d\n",  CFG.Req_Files);
	fprintf(fp, "      Max request MBytes %d\n",  CFG.Req_MBytes);

	for (i = 0; i < 40; i++)
		if ((CFG.phonetrans[i].match[0] != '\0') ||
		    (CFG.phonetrans[i].repl[0] != '\0'))
			fprintf(fp, "      Translate          %-20s %s\n", CFG.phonetrans[i].match, CFG.phonetrans[i].repl);

	page = newpage(fp, page);
	addtoc(fp, toc, 1, 19, page, (char *)"FTP server setup");

	fprintf(fp, "      Connections limit  %d\n", CFG.ftp_limit);
	fprintf(fp, "      Login fails        %d\n", CFG.ftp_loginfails);
	fprintf(fp, "      Allow compress     %s\n", getboolean(CFG.ftp_compress));
	fprintf(fp, "      Allow tar          %s\n", getboolean(CFG.ftp_tar));
	fprintf(fp, "      Log commands       %s\n", getboolean(CFG.ftp_log_cmds));
	fprintf(fp, "      Anonymous login    %s\n", getboolean(CFG.ftp_anonymousok));
	fprintf(fp, "      User mbse login    %s\n", getboolean(CFG.ftp_mbseok));
	fprintf(fp, "      Shutdown message   %s\n", CFG.ftp_msg_shutmsg);
	fprintf(fp, "      Upload path        %s\n", CFG.ftp_upl_path);
	fprintf(fp, "      README login       %s\n", CFG.ftp_readme_login);
	fprintf(fp, "      README cwd*        %s\n", CFG.ftp_readme_cwd);
	fprintf(fp, "      Message login      %s\n", CFG.ftp_msg_login);
	fprintf(fp, "      Message cwd*       %s\n", CFG.ftp_msg_cwd);
	fprintf(fp, "      Login banner       %s\n", CFG.ftp_banner);
	fprintf(fp, "      Email address      %s\n", CFG.ftp_email);
	fprintf(fp, "      Path filter        %s\n", CFG.ftp_pth_filter);
	fprintf(fp, "      Path message       %s\n", CFG.ftp_pth_message);

	addtoc(fp, toc, 1, 20, page, (char *)"WWW server setup");

	fprintf(fp, "      HTML root          %s\n", CFG.www_root);
	fprintf(fp, "      Link to FTP base   %s\n", CFG.www_link2ftp);
	fprintf(fp, "      Webserver URL      %s\n", CFG.www_url);
	fprintf(fp, "      Character set      %s\n", CFG.www_charset);
	fprintf(fp, "      Author name        %s\n", CFG.www_author);
	fprintf(fp, "      Convert command    %s\n", CFG.www_convert);
	fprintf(fp, "      File per webpage   %d\n", CFG.www_files_page);

	page = newpage(fp, page);
	addtoc(fp, toc, 1,21, page, (char *)"Manager flag descriptions");
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

	return page;
}



