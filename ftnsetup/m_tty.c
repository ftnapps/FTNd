/*****************************************************************************
 *
 * $Id: m_tty.c,v 1.29 2005/10/11 20:49:49 mbse Exp $
 * Purpose ...............: Setup Ttyinfo structure.
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
#include "stlist.h"
#include "m_modem.h"
#include "m_global.h"
#include "m_tty.h"



int	TtyUpdated = 0;


/*
 * Count nr of ttyinfo records in the database.
 * Creates the database if it doesn't exist.
 */
int CountTtyinfo(void)
{
    FILE    *fil;
    char    *ffile;
    int	    count = 0, i;

    ffile = calloc(PATH_MAX, sizeof(char));
    snprintf(ffile, PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));

    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    ttyinfohdr.hdrsize = sizeof(ttyinfohdr);
	    ttyinfohdr.recsize = sizeof(ttyinfo);
	    fwrite(&ttyinfohdr, sizeof(ttyinfohdr), 1, fil);

#if defined(__linux__)
	    /*
	     * Linux has 6 virtual consoles
	     */
	    for (i = 0; i < 6; i++) {
                memset(&ttyinfo, 0, sizeof(ttyinfo));
                snprintf(ttyinfo.comment, 41, "Console port %d", i+1);
                snprintf(ttyinfo.tty,      7, "tty%d", i+1);
                snprintf(ttyinfo.speed,   21, "10 mbit");
                ttyinfo.type = LOCAL;
                ttyinfo.available = TRUE;
                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
		count++;
            }
#endif

#if defined(__FreeBSD__)
            /*
             * FreeBSD has 8 virtual consoles
             */
            for (i = 0; i < 8; i++) {
                memset(&ttyinfo, 0, sizeof(ttyinfo));
                snprintf(ttyinfo.comment, 41, "Console port %d", i+1);
                snprintf(ttyinfo.tty,      7, "ttyv%d", i);
                snprintf(ttyinfo.speed,   21, "10 mbit");
                ttyinfo.type = LOCAL;
                ttyinfo.available = TRUE;
                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
                count++;
            }
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__)
	    /*
	     * By default, xxxBSD systems have only one console
	     */
	    memset(&ttyinfo, 0, sizeof(ttyinfo));
	    snprintf(ttyinfo.comment, 41, "Console port 1");
	    snprintf(ttyinfo.tty,      7, "console");
	    snprintf(ttyinfo.speed,   21, "10 mbit");
	    ttyinfo.type = LOCAL;
	    ttyinfo.available = TRUE;
	    fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
	    count++;
#endif

            for (i = 0; i < 4; i++) {
                memset(&ttyinfo, 0, sizeof(ttyinfo));
                snprintf(ttyinfo.comment, 41, "ISDN line %d", i+1);
#if defined(__linux__)
                snprintf(ttyinfo.tty,      7, "ttyI%d", i);
#elif defined(__FreeBSD__)
		snprintf(ttyinfo.tty,      7, "cuaia%d", i);
#elif defined(__NetBSD__)
		snprintf(ttyinfo.tty,      7, "ttyi%c", i + 'a'); // NetBSD on a Sparc, how about PC's? 
#elif defined(__OpenBSD__)
		snprintf(ttyinfo.tty,      7, "cuaia%d", i);	// I think this is wrong!
#else
#error "Don't know the tty name for ISDN on this OS"
#endif
                snprintf(ttyinfo.speed,   21, "64 kbits");
		snprintf(ttyinfo.flags,   31, "XA,X75,CM");
                ttyinfo.type = ISDN;
                ttyinfo.available = FALSE;
		ttyinfo.callout = TRUE;
		ttyinfo.honor_zmh = TRUE;
		snprintf(ttyinfo.name,    36, "ISDN line #%d", i+1);
                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
		count++;
            }

            for (i = 0; i < 4; i++) {
                memset(&ttyinfo, 0, sizeof(ttyinfo));
                snprintf(ttyinfo.comment, 41, "Modem line %d", i+1);
#if defined(__linux__)
		snprintf(ttyinfo.tty,      7, "ttyS%d", i);
#elif defined(__FreeBSD__)
		snprintf(ttyinfo.tty,      7, "cuaa%d", i);
#elif defined(__NetBSD__)
		snprintf(ttyinfo.tty,      7, "tty%c", i + 'a'); // NetBSD on a Sparc, how about PC's?
#elif defined(__OpenBSD__)
		snprintf(ttyinfo.tty,	  7, "tty0%d", i);
#else
#error "Don't know the tty name of the serial ports on this OS"
#endif
                snprintf(ttyinfo.speed,   21, "33.6 kbits");
		snprintf(ttyinfo.flags,   31, "CM,XA,V32B,V42B,V34");
		ttyinfo.type = POTS;
                ttyinfo.available = FALSE;
                ttyinfo.callout = TRUE;
                ttyinfo.honor_zmh = TRUE;
#ifdef __sparc__
		ttyinfo.portspeed = 38400;	// Safe, ULTRA has a higher maxmimum speed
#else
		ttyinfo.portspeed = 57600;
#endif
                snprintf(ttyinfo.name,    36, "Modem line #%d", i+1);
                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
		count++;
            }

	    fclose(fil);
	    chmod(ffile, 0640);
	    Syslog('+', "Creaded new %s with %d ttys", ffile, count);
	    free(ffile);
	    return count;
	} else {
	    free(ffile);
	    return -1;
	}
    }

    fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - ttyinfohdr.hdrsize) / ttyinfohdr.recsize;
    fclose(fil);
    free(ffile);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenTtyinfo(void);
int OpenTtyinfo(void)
{
    FILE	*fin, *fout;
    char	*fnin, *fnout;
    int		oldsize;

    fnin  = calloc(PATH_MAX, sizeof(char));
    fnout = calloc(PATH_MAX, sizeof(char));
    snprintf(fnin,  PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));

    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fin);
	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize = ttyinfohdr.recsize;
	    if (oldsize != sizeof(ttyinfo)) {
		TtyUpdated = 1;
		Syslog('+', "Updated %s, format changed", fnin);
	    } else
		TtyUpdated = 0;
	    ttyinfohdr.hdrsize = sizeof(ttyinfohdr);
	    ttyinfohdr.recsize = sizeof(ttyinfo);
	    fwrite(&ttyinfohdr, sizeof(ttyinfohdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&ttyinfo, 0, sizeof(ttyinfo));
	    while (fread(&ttyinfo, oldsize, 1, fin) == 1) {
		/*
		 * If network ports available, set updated so the records will
		 * be deleted during close.
		 */
		if (ttyinfo.type == NETWORK) {
		    TtyUpdated = 1;
		    ttyinfo.deleted = TRUE;
		    ttyinfo.available = FALSE;
		}
		fwrite(&ttyinfo, sizeof(ttyinfo), 1, fout);
		memset(&ttyinfo, 0, sizeof(ttyinfo));
	    }

	    fclose(fin);
	    fclose(fout);
	    free(fnin);
	    free(fnout);
	    return 0;
	}
    }

    free(fnin);
    free(fnout);
    return -1;
}



void CloseTtyinfo(int);
void CloseTtyinfo(int force)
{
    char	*fin, *fout;
    FILE	*fi, *fo;
    st_list	*tty = NULL, *tmp;

    fin  = calloc(PATH_MAX, sizeof(char));
    fout = calloc(PATH_MAX, sizeof(char));
    snprintf(fin,  PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    snprintf(fout, PATH_MAX, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));

    if (TtyUpdated == 1) {
	if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
	    working(1, 0, 0);
	    fi = fopen(fout, "r");
	    fo = fopen(fin,  "w");
	    fread(&ttyinfohdr, ttyinfohdr.hdrsize, 1, fi);
	    fwrite(&ttyinfohdr, ttyinfohdr.hdrsize, 1, fo);

	    while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fi) == 1)
		if (!ttyinfo.deleted)
		    fill_stlist(&tty, ttyinfo.comment, ftell(fi) - ttyinfohdr.recsize);
	    sort_stlist(&tty);

	    for (tmp = tty; tmp; tmp = tmp->next) {
		fseek(fi, tmp->pos, SEEK_SET);
	        fread(&ttyinfo, ttyinfohdr.recsize, 1, fi);
	        fwrite(&ttyinfo, ttyinfohdr.recsize, 1, fo);
	    }

	    tidy_stlist(&tty);
	    fclose(fi);
	    fclose(fo);
	    unlink(fout);
	    chmod(fin, 0640);
	    Syslog('+', "Updated \"ttyinfo.data\"");
	    if (!force)
	        working(6, 0, 0);
	    free(fin);
	    free(fout);
	    return;
	}
    }

    free(fin);
    free(fout);
    chmod(fin, 0640);
    working(1, 0, 0);
    unlink(fout); 
}



int AppendTtyinfo(void)
{
    FILE    *fil;
    char    *ffile;

    ffile = calloc(PATH_MAX, sizeof(char));
    snprintf(ffile, PATH_MAX, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
    
    if ((fil = fopen(ffile, "a")) != NULL) {
	memset(&ttyinfo, 0, sizeof(ttyinfo));
	fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
	fclose(fil);
	TtyUpdated = 1;
	free(ffile);
	return 0;
    }

    free(ffile);
    return -1;
}



void TtyScreen(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "6.  EDIT TTY LINE");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.  Comment");
    mbse_mvprintw( 8, 6, "2.  TTY Device");
    mbse_mvprintw( 9, 6, "3.  Phone nr.");
    mbse_mvprintw(10, 6, "4.  Line Speed");
    mbse_mvprintw(11, 6, "5.  Fido Flags");
    mbse_mvprintw(12, 6, "6.  Line Type");
    mbse_mvprintw(13, 6, "7.  Available");
    mbse_mvprintw(14, 6, "8.  Honor ZMH");
    mbse_mvprintw(15, 6, "9.  Deleted");
    mbse_mvprintw(16, 6, "10. Callout");

    mbse_mvprintw(14,31, "11. Portspeed");
    mbse_mvprintw(15,31, "12. Modemtype");
    mbse_mvprintw(16,31, "13. EMSI name");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditTtyRec(int Area)
{
    FILE	    *fil;
    char	    *mfile;
    int		    offset;
    int		    j;
    unsigned int    crc, crc1;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Ttyinfo");
    mfile = calloc(PATH_MAX, sizeof(char));
    snprintf(mfile, PATH_MAX, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
    
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	free(mfile);
	return -1;
    }

    offset = sizeof(ttyinfohdr) + ((Area -1) * sizeof(ttyinfo));
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	fclose(fil);
	free(mfile);
	return -1;
    }

    fread(&ttyinfo, sizeof(ttyinfo), 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&ttyinfo, crc, sizeof(ttyinfo));
    TtyScreen();
	
    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,21,40, ttyinfo.comment);
	show_str( 8,21, 6, ttyinfo.tty);
	show_str( 9,21,25, ttyinfo.phone);
	show_str(10,21,20, ttyinfo.speed);
	show_str(11,21,30, ttyinfo.flags);
	show_linetype(12,21, ttyinfo.type);
	show_bool(13,21,   ttyinfo.available);
	show_bool(14,21,   ttyinfo.honor_zmh);
	show_bool(15,21,   ttyinfo.deleted);
	show_bool(16,21,   ttyinfo.callout);
	show_int( 14,45,   ttyinfo.portspeed);
	show_str( 15,45,30,ttyinfo.modem);
	show_str( 16,45,35,ttyinfo.name);

	j = select_menu(13);
	switch(j) {
	    case 0: crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&ttyinfo, crc1, sizeof(ttyinfo));
		    if (crc != crc1) {
			if (yes_no((char *)"Record is changed, save") == 1) {
			    working(1, 0, 0);
			    if ((fil = fopen(mfile, "r+")) == NULL) {
				working(2, 0, 0);
				free(mfile);
				return -1;
			    }
			    fseek(fil, offset, 0);
			    fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
			    fclose(fil);
			    TtyUpdated = 1;
			    working(6, 0, 0);
			}
		    }
		    IsDoing("Browsing Menu");
		    free(mfile);
		    return 0;
	    case 1: E_STR(  7,21,40,ttyinfo.comment,  "The ^Comment^ for this record")
	    case 2: E_STR(  8,21,7, ttyinfo.tty,      "The ^Device name^ of this tty line")
	    case 3: E_STR(  9,21,25,ttyinfo.phone,    "The ^Phone number^ or ^Hostname^ or ^IP address^ of this tty line")
	    case 4: E_STR( 10,21,20,ttyinfo.speed,    "The ^Speed^ of this device")
	    case 5: E_STR( 11,21,30,ttyinfo.flags,    "The ^Fidonet Capability Flags^ for this tty line")
	    case 6: ttyinfo.type = edit_linetype(12,21, ttyinfo.type);
		    if (ttyinfo.type == POTS) {
			if (!ttyinfo.portspeed)
			    ttyinfo.portspeed = 57600;
		    } else {
			ttyinfo.portspeed = 0;
		    }
		    break;
	    case 7: ttyinfo.available = edit_bool(13,21, ttyinfo.available, 
				(char *)"Switch if this tty line is ^Available^ for use.");
		    if (ttyinfo.available) {
			if ((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) {
			    ttyinfo.callout = TRUE;
			    ttyinfo.honor_zmh = TRUE;
			}
			if (ttyinfo.type == POTS) {
			    if (!ttyinfo.portspeed)
				ttyinfo.portspeed = 57600;
			} else {
			    ttyinfo.portspeed = 0;
			}
		    }
		    break;
	    case 8: E_BOOL(14,21,   ttyinfo.honor_zmh,"Honor ^Zone Mail Hour^ for bbs users on this tty")
	    case 9: E_BOOL(15,21,   ttyinfo.deleted,  "Is this tty line ^deleted") 
	    case 10:E_BOOL(16,21,   ttyinfo.callout,  "Is this line available for ^calling out^")
	    case 11:E_INT (14,45,   ttyinfo.portspeed,"The ^locked speed^ of this tty port (only for modems)")
	    case 12:if ((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) {
			strcpy(ttyinfo.modem, PickModem((char *)"6.12")); 
			TtyScreen();
		    }
		    break;
	    case 13:E_STR( 16,45,30,ttyinfo.name,     "The ^EMSI name^ for this tty line")
	}
    }

    return 0;
}



void EditTtyinfo(void)
{
    int	    records, i, o, x, y;
    char    pick[12];
    FILE    *fil;
    char    temp[PATH_MAX];
    int	    offset;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    records = CountTtyinfo();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenTtyinfo() == -1) {
	working(2, 0, 0);
	return;
    }
    o = 0;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 4, "6.  TTY LINES SETUP");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fil);
		x = 2;
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 20 ; i++) {
		    if (i == 11) {
			x = 42;
			y = 7;
		    }
		    if ((o + i) <= records) {
			offset = sizeof(ttyinfohdr) + (((o + i) - 1) * ttyinfohdr.recsize);
			fseek(fil, offset, 0);
			fread(&ttyinfo, ttyinfohdr.recsize, 1, fil);
			if (ttyinfo.deleted)
			    set_color(LIGHTRED, BLACK);
			else if (ttyinfo.available)
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			snprintf(temp, 81, "%3d.  %-6s %-25s", o+i, ttyinfo.tty, ttyinfo.comment);
			temp[37] = 0;
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_record(records, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseTtyinfo(FALSE);
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    working(1, 0, 0);
	    if (AppendTtyinfo() == 0) {
		records++;
		working(1, 0, 0);
	    } else
		working(2, 0, 0);
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 20) < records) 
		o = o + 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o = o - 20;

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    EditTtyRec(atoi(pick));
	    o = ((atoi(pick) -1) / 20) * 20;
	}
    }
}



void InitTtyinfo(void)
{
    CountTtyinfo();
    OpenTtyinfo();
    CloseTtyinfo(TRUE);
}



int tty_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *tty;
    int	    j;

    snprintf(temp, PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    if ((tty = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 6, 0, page, (char *)"TTY lines information");
    j = 0;

    fprintf(fp, "\n\n");
    fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, tty);

    ip = open_webdoc((char *)"ttyinfo.html", (char *)"TTY Lines", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>TTY</TH><TH align='left'>Comment</TH><TH align='left'>Available</TH></TR>\n");

    while ((fread(&ttyinfo, ttyinfohdr.recsize, 1, tty)) == 1) {
	if (j == 3) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	snprintf(temp, 81, "ttyinfo_%s.html", ttyinfo.tty);
	fprintf(ip, "<TR><TD><A HREF=\"%s\">%s</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, ttyinfo.tty, ttyinfo.comment, getboolean(ttyinfo.available));
	if ((wp = open_webdoc(temp, (char *)"TTY Line", ttyinfo.comment))) {
	    /*
	     * There are devices like pts/1, this will create a subdir for the
	     * pts lines, we need a different return path.
	     */
	    if (strchr(ttyinfo.tty, '/'))
		fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"../ttyinfo.html\">Back</A>\n");
	    else
		fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"ttyinfo.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"TTY name", ttyinfo.comment);
	    add_webtable(wp, (char *)"Device name", ttyinfo.tty);
	    add_webtable(wp, (char *)"Phone or DNS", ttyinfo.phone);
	    add_webtable(wp, (char *)"Line speed", ttyinfo.speed);
	    add_webtable(wp, (char *)"Fido flags", ttyinfo.flags);
	    add_webtable(wp, (char *)"Equipment", getlinetype(ttyinfo.type));
	    add_webtable(wp, (char *)"Available", getboolean(ttyinfo.available));
	    add_webtable(wp, (char *)"Honor ZMH", getboolean(ttyinfo.honor_zmh));
	    add_webtable(wp, (char *)"Callout", getboolean(ttyinfo.callout));
	    add_webtable(wp, (char *)"Modem type", ttyinfo.modem);
	    add_webdigit(wp, (char *)"Locked speed", ttyinfo.portspeed);
	    add_webtable(wp, (char *)"EMSI name", ttyinfo.name);
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}

	fprintf(fp, "     TTY name     %s\n", ttyinfo.comment);
	fprintf(fp, "     Device name  %s\n", ttyinfo.tty);
	fprintf(fp, "     Phone or DNS %s\n", ttyinfo.phone);
	fprintf(fp, "     Line speed   %s\n", ttyinfo.speed);
	fprintf(fp, "     Fido flags   %s\n", ttyinfo.flags);
	fprintf(fp, "     Equipment    %s\n", getlinetype(ttyinfo.type));
	fprintf(fp, "     Available    %s\n", getboolean(ttyinfo.available));
	fprintf(fp, "     Honor ZMH    %s\n", getboolean(ttyinfo.honor_zmh));
	fprintf(fp, "     Callout      %s\n", getboolean(ttyinfo.callout));
	fprintf(fp, "     Modem type   %s\n", ttyinfo.modem);
	fprintf(fp, "     Locked speed %d\n", ttyinfo.portspeed);
	fprintf(fp, "     EMSI name    %s\n", ttyinfo.name);
	fprintf(fp, "\n\n");
	j++;
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	    
    fclose(tty);
    return page;
}


