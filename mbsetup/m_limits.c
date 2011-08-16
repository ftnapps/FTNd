/*****************************************************************************
 *
 * $Id: m_limits.c,v 1.18 2005/10/11 20:49:49 mbse Exp $
 * Purpose ...............: Setup Limits.
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
#include "../lib/users.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_limits.h"



int	LimUpdated = 0;


/*
 * Count nr of LIMIT records in the database.
 * Creates the database if it doesn't exist.
 */
int CountLimits(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			LIMIThdr.hdrsize = sizeof(LIMIThdr);
			LIMIThdr.recsize = sizeof(LIMIT);
			fwrite(&LIMIThdr, sizeof(LIMIThdr), 1, fil);

			/*
			 *  Create default limits
			 */
			memset(&LIMIT, 0, sizeof(LIMIT));
			LIMIT.Security = 0;
			LIMIT.Time = 5;
			LIMIT.DownK = 1;
			LIMIT.DownF = 1;
			snprintf(LIMIT.Description, 41, "Twit");
			LIMIT.Available = TRUE;
			fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 5;
                        LIMIT.Time = 15;
                        LIMIT.DownK = 100;
                        LIMIT.DownF = 2;
                        snprintf(LIMIT.Description, 41, "New User");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 20;
                        LIMIT.Time = 60;
                        LIMIT.DownK = 10240;
                        LIMIT.DownF = 25;
                        snprintf(LIMIT.Description, 41, "Normal User");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 50;
                        LIMIT.Time = 90;
                        LIMIT.DownK = 20480;
                        LIMIT.DownF = 100;
                        snprintf(LIMIT.Description, 41, "V.I.P. User");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 80;
                        LIMIT.Time = 120;
                        LIMIT.DownK = 40960;
                        snprintf(LIMIT.Description, 41, "Fellow Sysop or Point");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 100;
                        LIMIT.Time = 180;
                        LIMIT.DownK = 40960;
                        snprintf(LIMIT.Description, 41, "Co-Sysop");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 32000;
                        LIMIT.Time = 240;
                        LIMIT.DownK = 40960;
                        snprintf(LIMIT.Description, 41, "Sysop");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

			fclose(fil);
			chmod(ffile, 0640);
			return 7;
		} else
			return -1;
	}

	fread(&LIMIThdr, sizeof(LIMIThdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - LIMIThdr.hdrsize) / LIMIThdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenLimits(void);
int OpenLimits(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&LIMIThdr, sizeof(LIMIThdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = LIMIThdr.recsize;
			if (oldsize != sizeof(LIMIT)) {
				LimUpdated = 1;
				Syslog('+', "Updated %s, format changed");
			} else
				LimUpdated = 0;
			LIMIThdr.hdrsize = sizeof(LIMIThdr);
			LIMIThdr.recsize = sizeof(LIMIT);
			fwrite(&LIMIThdr, sizeof(LIMIThdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&LIMIT, 0, sizeof(LIMIT));
			while (fread(&LIMIT, oldsize, 1, fin) == 1) {
				fwrite(&LIMIT, sizeof(LIMIT), 1, fout);
				memset(&LIMIT, 0, sizeof(LIMIT));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseLimits(int);
void CloseLimits(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX], temp[20];
	FILE	*fi, *fo;
	st_list	*lim = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/limits.temp", getenv("MBSE_ROOT"));

	if (LimUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&LIMIThdr, LIMIThdr.hdrsize, 1, fi);
			fwrite(&LIMIThdr, LIMIThdr.hdrsize, 1, fo);

			while (fread(&LIMIT, LIMIThdr.recsize, 1, fi) == 1)
				if (!LIMIT.Deleted) {
					snprintf(temp, 20, "%014d", LIMIT.Security);
					fill_stlist(&lim, temp, ftell(fi) - LIMIThdr.recsize);
				}
			sort_stlist(&lim);

			for (tmp = lim; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&LIMIT, LIMIThdr.recsize, 1, fi);
				fwrite(&LIMIT, LIMIThdr.recsize, 1, fo);
			}

			tidy_stlist(&lim);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"limits.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendLimits(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&LIMIT, 0, sizeof(LIMIT));
		fwrite(&LIMIT, sizeof(LIMIT), 1, fil);
		fclose(fil);
		LimUpdated = 1;
		return 0;
	} else
		return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditLimRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	int	offset;
	int	j;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Limits");

	snprintf(mfile, PATH_MAX, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(LIMIThdr) + ((Area -1) * sizeof(LIMIT));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&LIMIT, sizeof(LIMIT), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&LIMIT, crc, sizeof(LIMIT));

	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "8.1 EDIT SECURITY LIMIT");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 6, "1.  Access Level");
	mbse_mvprintw( 8, 6, "2.  Maximum Time");
	mbse_mvprintw( 9, 6, "3.  Download Kb.");
	mbse_mvprintw(10, 6, "4.  Download Files");
	mbse_mvprintw(11, 6, "5.  Description");
	mbse_mvprintw(12, 6, "6.  Available");
	mbse_mvprintw(13, 6, "7.  Deleted");

	for (;;) {
		set_color(WHITE, BLACK);
		show_int( 7,25,    LIMIT.Security);
		show_int( 8,25,    LIMIT.Time);
		show_int( 9,25,    LIMIT.DownK);
		show_int(10,25,    LIMIT.DownF);
		show_str(11,25,40, LIMIT.Description);
		show_bool(12,25,   LIMIT.Available);
		show_bool(13,25,   LIMIT.Deleted);

		j = select_menu(7);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&LIMIT, crc1, sizeof(LIMIT));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&LIMIT, sizeof(LIMIT), 1, fil);
					fclose(fil);
					LimUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_INT(  7,25,   LIMIT.Security,   "The ^Security^ level for this limit")
		case 2:	E_INT(  8,25,   LIMIT.Time,       "The maxmimum ^Time online^ per day for this limit, zero to disable")
		case 3:	E_INT(  9,25,   LIMIT.DownK,      "The ^Kilobytes^ download limit per day, 0 = don't care")
		case 4:	E_INT( 10,25,   LIMIT.DownF,      "The ^nr of files^ to download per day, 0 = don't care")
		case 5:	E_STR( 11,25,40,LIMIT.Description,"A short ^description^ for this limit")
		case 6:	E_BOOL(12,25,   LIMIT.Available,  "Is this record ^avaiable^")
		case 7: E_BOOL(13,25,   LIMIT.Deleted,    "Is this level ^Deleted^")
		}
	}

	return 0;
}



void EditLimits(void)
{
	int	records, i, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountLimits();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenLimits() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 7, "8.1 LIMITS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&LIMIThdr, sizeof(LIMIThdr), 1, fil);
				x = 5;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(LIMIThdr) + ((i - 1) * LIMIThdr.recsize);
					fseek(fil, offset, 0);
					fread(&LIMIT, LIMIThdr.recsize, 1, fil);
					if (i == 11) {
						x = 45;
						y = 7;
					}
					if (LIMIT.Available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					snprintf(temp, 81, "%3d.  %-6d %-40s", i, LIMIT.Security, LIMIT.Description);
					temp[37] = '\0';
					mbse_mvprintw(y, x, temp);
					y++;
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseLimits(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendLimits() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditLimRec(atoi(pick));
	}
}



void InitLimits(void)
{
    CountLimits();
    OpenLimits();
    CloseLimits(TRUE);
}



char *PickLimits(int nr)
{
	static	char Lim[21] = "";
	int	records, i, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return Lim;
	}

	records = CountLimits();
	if (records == -1) {
		working(2, 0, 0);
		return Lim;
	}


	clr_index();
	set_color(WHITE, BLACK);
	snprintf(temp, 81, "%d.  LIMITS SELECT", nr);
	mbse_mvprintw( 5, 4, temp);
	set_color(CYAN, BLACK);
	if (records != 0) {
		snprintf(temp, PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
		if ((fil = fopen(temp, "r")) != NULL) {
			fread(&LIMIThdr, sizeof(LIMIThdr), 1, fil);
			x = 2;
			y = 7;
			set_color(CYAN, BLACK);
			for (i = 1; i <= records; i++) {
				offset = sizeof(LIMIThdr) + ((i - 1) * LIMIThdr.recsize);
				fseek(fil, offset, 0);
				fread(&LIMIT, LIMIThdr.recsize, 1, fil);
				if (i == 11) {
					x = 42;
					y = 7;
				}
				if (LIMIT.Available)
					set_color(CYAN, BLACK);
				else
					set_color(LIGHTBLUE, BLACK);
				snprintf(temp, 81, "%3d.  %-6d %-40s", i, LIMIT.Security, LIMIT.Description);
				temp[37] = '\0';
				mbse_mvprintw(y, x, temp);
				y++;
			}
			strcpy(pick, select_pick(records, 20));

			if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
				offset = sizeof(LIMIThdr) + ((atoi(pick) - 1) * LIMIThdr.recsize);
				fseek(fil, offset, 0);
				fread(&LIMIT, LIMIThdr.recsize, 1, fil);
				snprintf(Lim, 21, "%d", LIMIT.Security);
			}
			fclose(fil);
		}
	}
	return Lim;
}



char *get_limit_name(int level)
{
    static char	buf[41];
    char	temp[PATH_MAX];
    FILE	*fp;

    snprintf(buf, 41, "N/A");
    snprintf(temp, PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL)
	return buf;

    fread(&LIMIThdr, sizeof(LIMIThdr), 1, fp);

    while ((fread(&LIMIT, LIMIThdr.recsize, 1, fp)) == 1) {
	if (level == LIMIT.Security) {
	    snprintf(buf, 41, "%s", LIMIT.Description);
	    break;
	}
    }
    fclose(fp);
    return buf;
}



int bbs_limits_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *up, *ip, *no;
    int	    nr;

    snprintf(temp, PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    addtoc(fp, toc, 8, 1, page, (char *)"BBS user limits");

    ip = open_webdoc((char *)"limits.html", (char *)"BBS User Security Limits", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Access Level</TH><TH align='left'>Max. time</TH>");
    fprintf(ip, "<TH align='left'>Down Kb.</TH><TH align='left'>Down files</TH>");
    fprintf(ip, "<TH align='left'>Active</TH><TH align='left'>Description</TH><TR>\n");
    fread(&LIMIThdr, sizeof(LIMIThdr), 1, no);

    fprintf(fp, "\n");
    fprintf(fp, "     Access   Max.   Down   Down\n");
    fprintf(fp, "      Level   time    Kb.  files Active Description\n");
    fprintf(fp, "     ------ ------ ------ ------ ------ ------------------------------\n");

    while ((fread(&LIMIT, LIMIThdr.recsize, 1, no)) == 1) {
	fprintf(fp, "     %6d %6d %6d %6d %s    %s\n", 
	    LIMIT.Security, LIMIT.Time, LIMIT.DownK, LIMIT.DownF, getboolean(LIMIT.Available), LIMIT.Description);
	fprintf(ip, "<TR><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%s</TD><TD>%s</TD></TR>\n",
	    LIMIT.Security, LIMIT.Time, LIMIT.DownK, LIMIT.DownF, getboolean(LIMIT.Available), LIMIT.Description);
    }	

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    fprintf(ip, "<HR>\n");
    fprintf(ip, "<H3>Users in security levels</H3>\n");
    fprintf(ip, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<COL width='20%%'><COL width='40%%'><COL width='40%%'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Access Level</TH><TH align='left'>User</TH><TH align='left'>Location</TH></TR>\n");
    fseek(no, LIMIThdr.hdrsize, SEEK_SET);

    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((up = fopen(temp, "r"))) {
	fread(&usrconfighdr, sizeof(usrconfighdr), 1, up);
	
	while ((fread(&LIMIT, LIMIThdr.recsize, 1, no)) == 1) {
	    fseek(up, usrconfighdr.hdrsize, SEEK_SET);
	    nr = 0;
	    while (fread(&usrconfig, usrconfighdr.recsize, 1, up) == 1) {
		nr++;
		if (strlen(usrconfig.sUserName) && (usrconfig.Security.level == LIMIT.Security)) {
		    fprintf(ip, "<TR><TD>%d</TD><TD><A HREF=\"user_%d.html\">%s</A></TD><TD>%s</TD></TR>\n", 
			LIMIT.Security, nr, usrconfig.sUserName, usrconfig.sLocation);
		}
	    }
	}
	fclose(up);
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
    fclose(no);
    return page;
}

