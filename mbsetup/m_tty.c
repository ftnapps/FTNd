/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Ttyinfo structure.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count, i;

	sprintf(ffile, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Creaded new %s", ffile);
			ttyinfohdr.hdrsize = sizeof(ttyinfohdr);
			ttyinfohdr.recsize = sizeof(ttyinfo);
			fwrite(&ttyinfohdr, sizeof(ttyinfohdr), 1, fil);

			for (i = 0; i < 10; i++) {
				memset(&ttyinfo, 0, sizeof(ttyinfo));
				sprintf(ttyinfo.comment, "Network port %d", i+11);
				sprintf(ttyinfo.tty,     "pts/%d", i);
				sprintf(ttyinfo.speed,   "10 mbit");
				sprintf(ttyinfo.flags,   "IBN,IFC,XX");
				ttyinfo.type = NETWORK;
				ttyinfo.available = TRUE;
				sprintf(ttyinfo.name,    "Network port #%d", i+11);
				fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
			}

                        for (i = 0; i < 10; i++) {
                                memset(&ttyinfo, 0, sizeof(ttyinfo));
                                sprintf(ttyinfo.comment, "Network port %d", i+1);
                                sprintf(ttyinfo.tty,     "ttyp%d", i);
                                sprintf(ttyinfo.speed,   "10 mbit");
                                sprintf(ttyinfo.flags,   "IBN,IFC,XX");
                                ttyinfo.type = NETWORK;
                                ttyinfo.available = TRUE;
                                sprintf(ttyinfo.name,    "Network port #%d", i+1);
                                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
                        }

			for (i = 0; i < 6; i++) {
                                memset(&ttyinfo, 0, sizeof(ttyinfo));
                                sprintf(ttyinfo.comment, "Console port %d", i+1);
                                sprintf(ttyinfo.tty,     "tty%d", i);
                                sprintf(ttyinfo.speed,   "10 mbit");
                                ttyinfo.type = LOCAL;
                                ttyinfo.available = TRUE;
                                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
                        }

                        for (i = 0; i < 4; i++) {
                                memset(&ttyinfo, 0, sizeof(ttyinfo));
                                sprintf(ttyinfo.comment, "ISDN line %d", i+1);
#ifdef __linux__
                                sprintf(ttyinfo.tty,     "ttyI%d", i);
#endif
#ifdef __FreeBSD__
				sprintf(ttyinfo.tty,     "cuaia%d", i);
#endif
#ifdef __NetBSD__
				sprintf(ttyinfo.tty,     "ttyi%c", i + 'a'); // NetBSD on a Sparc, how about PC's? 
#endif
#ifdef __OpenBSD__
				sprintf(ttyinfo.tty,     "cuaia%d", i);	// I think this is wrong!
#endif
                                sprintf(ttyinfo.speed,   "64 kbits");
				sprintf(ttyinfo.flags,   "XA,X75,CM");
                                ttyinfo.type = ISDN;
                                ttyinfo.available = FALSE;
				ttyinfo.callout = TRUE;
				ttyinfo.honor_zmh = TRUE;
				sprintf(ttyinfo.name,    "ISDN line #%d", i+1);
                                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
                        }

                        for (i = 0; i < 4; i++) {
                                memset(&ttyinfo, 0, sizeof(ttyinfo));
                                sprintf(ttyinfo.comment, "Modem line %d", i+1);
#ifdef __linux__
                                sprintf(ttyinfo.tty,     "ttyS%d", i);
#endif
#ifdef __FreeBSD__
				sprintf(ttyinfo.tty,     "cuaa%d", i);
#endif
#ifdef __NetBSD__
				sprintf(ttyinfo.tty,     "tty%c", i + 'a'); // NetBSD on a Sparc, how about PC's?
#endif
#ifdef __OpenBSD__
				sprintf(ttyinfo.tty,	"tty0%d", i);
#endif
                                sprintf(ttyinfo.speed,   "33.6 kbits");
				sprintf(ttyinfo.flags,   "CM,XA,V32B,V42B,V34");
                                ttyinfo.type = POTS;
                                ttyinfo.available = FALSE;
                                ttyinfo.callout = TRUE;
                                ttyinfo.honor_zmh = TRUE;
#ifdef __sparc__
				ttyinfo.portspeed = 38400;	// Safe, ULTRA has a higher maxmimum speed
#else
				ttyinfo.portspeed = 57600;
#endif
                                sprintf(ttyinfo.name,    "Modem line #%d", i+1);
                                fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
                        }

			fclose(fil);
			chmod(ffile, 0640);
			return 34;
		} else
			return -1;
	}

	fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - ttyinfohdr.hdrsize) / ttyinfohdr.recsize;
	fclose(fil);

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
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
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
				fwrite(&ttyinfo, sizeof(ttyinfo), 1, fout);
				memset(&ttyinfo, 0, sizeof(ttyinfo));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseTtyinfo(int);
void CloseTtyinfo(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*tty = NULL, *tmp;

	sprintf(fin, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));

	if (TtyUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&ttyinfohdr, ttyinfohdr.hdrsize, 1, fi);
			fwrite(&ttyinfohdr, ttyinfohdr.hdrsize, 1, fo);

			while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fi) == 1)
				if (!ttyinfo.deleted)
					fill_stlist(&tty, ttyinfo.tty, ftell(fi) - ttyinfohdr.recsize);
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
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendTtyinfo(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&ttyinfo, 0, sizeof(ttyinfo));
		fwrite(&ttyinfo, sizeof(ttyinfo), 1, fil);
		fclose(fil);
		TtyUpdated = 1;
		return 0;
	} else
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
	mbse_mvprintw(14, 6, "8.  Auth. log");
	mbse_mvprintw(15, 6, "9.  Honor ZMH");
	mbse_mvprintw(16, 6, "10. Deleted");
	mbse_mvprintw(17, 6, "11. Callout");

	mbse_mvprintw(15,31, "12. Portspeed");
	mbse_mvprintw(16,31, "13. Modemtype");
	mbse_mvprintw(17,31, "14. EMSI name");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditTtyRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Ttyinfo");

	sprintf(mfile, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(ttyinfohdr) + ((Area -1) * sizeof(ttyinfo));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
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
		show_bool(14,21,   ttyinfo.authlog);
		show_bool(15,21,   ttyinfo.honor_zmh);
		show_bool(16,21,   ttyinfo.deleted);
		show_bool(17,21,   ttyinfo.callout);
		show_int( 15,45,   ttyinfo.portspeed);
		show_str( 16,45,30,ttyinfo.modem);
		show_str( 17,45,35,ttyinfo.name);

		j = select_menu(14);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&ttyinfo, crc1, sizeof(ttyinfo));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
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
			return 0;
		case 1:	E_STR(  7,21,40,ttyinfo.comment,  "The ^Comment^ for this record")
		case 2:	E_STR(  8,21,7, ttyinfo.tty,      "The ^Device name^ of this tty line")
		case 3:	E_STR(  9,21,25,ttyinfo.phone,    "The ^Phone number^ or ^Hostname^ or ^IP address^ of this tty line")
		case 4:	E_STR( 10,21,20,ttyinfo.speed,    "The ^Speed^ of this device")
		case 5:	E_STR( 11,21,30,ttyinfo.flags,    "The ^Fidonet Capability Flags^ for this tty line")
		case 6:	ttyinfo.type = edit_linetype(12,21, ttyinfo.type);
			if (ttyinfo.type == POTS) {
			    if (!ttyinfo.portspeed)
				ttyinfo.portspeed = 57600;
			} else {
			    ttyinfo.portspeed = 0;
			}
			break;
		case 7:	ttyinfo.available = edit_bool(13,21, ttyinfo.available, 
				(char *)"Switch if this tty line is ^Available^ for use.");
			if (ttyinfo.available) {
			    if ((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) {
				ttyinfo.callout = TRUE;
				ttyinfo.authlog = TRUE;
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
		case 8:	E_BOOL(14,21,   ttyinfo.authlog,  "Is mgetty ^Auth^ logging available")
		case 9: E_BOOL(15,21,   ttyinfo.honor_zmh,"Honor ^Zone Mail Hour^ on this tty line")
		case 10:E_BOOL(16,21,   ttyinfo.deleted,  "Is this tty line ^deleted") 
		case 11:E_BOOL(17,21,   ttyinfo.callout,  "Is this line available for ^calling out^")
		case 12:E_INT (15,45,   ttyinfo.portspeed,"The ^locked speed^ of this tty port (only for modems)")
		case 13:strcpy(ttyinfo.modem, PickModem((char *)"6.13")); TtyScreen(); break;
		case 14:E_STR( 17,45,30,ttyinfo.name,     "The ^EMSI name^ for this tty line")
		}
	}

	return 0;
}



void EditTtyinfo(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;

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
			sprintf(temp, "%s/etc/ttyinfo.temp", getenv("MBSE_ROOT"));
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
						if (ttyinfo.available)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-6s %-25s", o+i, ttyinfo.tty, ttyinfo.comment);
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

    sprintf(temp, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    if ((tty = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 6, 0, page, (char *)"TTY lines information");
    j = 0;

    fprintf(fp, "\n\n");
    fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, tty);

    ip = open_webdoc((char *)"ttyinfo.html", (char *)"TTY Lines", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<UL>\n");
		    
    while ((fread(&ttyinfo, ttyinfohdr.recsize, 1, tty)) == 1) {
	if (j == 3) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	sprintf(temp, "ttyinfo_%s.html", ttyinfo.tty);
	fprintf(ip, "<LI><A HREF=\"%s\">%s</A></LI>\n", temp, ttyinfo.comment);
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
	    add_webtable(wp, (char *)"Auth. log", getboolean(ttyinfo.authlog));
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
	fprintf(fp, "     Auth. log    %s\n", getboolean(ttyinfo.authlog));
	fprintf(fp, "     Honor ZMH    %s\n", getboolean(ttyinfo.honor_zmh));
	fprintf(fp, "     Callout      %s\n", getboolean(ttyinfo.callout));
	fprintf(fp, "     Modem type   %s\n", ttyinfo.modem);
	fprintf(fp, "     Locked speed %ld\n", ttyinfo.portspeed);
	fprintf(fp, "     EMSI name    %s\n", ttyinfo.name);
	fprintf(fp, "\n\n");
	j++;
    }

    fprintf(ip, "</UL>\n");
    close_webdoc(ip);
	    
    fclose(tty);
    return page;
}


