/*****************************************************************************
 *
 * $Id: m_modem.c,v 1.18 2005/10/11 20:49:49 mbse Exp $
 * Purpose ...............: Setup Modem structure.
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
#include "m_global.h"
#include "m_modem.h"



int	ModemUpdated = 0;


/*
 * Count nr of modem records in the database.
 * Creates the database if it doesn't exist.
 */
int CountModem(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/modem.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			modemhdr.hdrsize = sizeof(modemhdr);
			modemhdr.recsize = sizeof(modem);
			fwrite(&modemhdr, sizeof(modemhdr), 1, fil);

			/*
			 *  Create some default modem types
			 */
			memset(&modem, 0, sizeof(modem));
			snprintf(modem.modem,       31, "Dynalink 1428EXTRA");
			snprintf(modem.init[0],     61, "AT Z\\r");
			snprintf(modem.init[1],     61, "AT &F &C1 &D2 X4 W2 B0 M0 \\\\V1 \\\\G0 &K3 S37=0\\r");
			snprintf(modem.ok,          11, "OK");
			snprintf(modem.dial,        41, "ATDT\\T\\r");
			snprintf(modem.connect[0],  31, "CONNECT 56000");
			snprintf(modem.connect[1],  31, "CONNECT 48000");
			snprintf(modem.connect[2],  31, "CONNECT 44000");
			snprintf(modem.connect[3],  31, "CONNECT 41333");
			snprintf(modem.connect[4],  31, "CONNECT 38000");
			snprintf(modem.connect[5],  31, "CONNECT 33600");
			snprintf(modem.connect[6],  31, "CONNECT 31200");
			snprintf(modem.connect[7],  31, "CONNECT 28800");
			snprintf(modem.connect[8],  31, "CONNECT 26400");
			snprintf(modem.connect[9],  31, "CONNECT 24000");
			snprintf(modem.connect[10], 31, "CONNECT 21600");
			snprintf(modem.connect[11], 31, "CONNECT 19200");
			snprintf(modem.connect[12], 31, "CONNECT 16800");
			snprintf(modem.connect[13], 31, "CONNECT 14400");
			snprintf(modem.connect[14], 31, "CONNECT 12000");
			snprintf(modem.connect[15], 31, "CONNECT 9600");
			snprintf(modem.connect[16], 31, "CONNECT 7200");
			snprintf(modem.connect[17], 31, "CONNECT 4800");
			snprintf(modem.connect[18], 31, "CONNECT 2400");
			snprintf(modem.connect[19], 31, "CONNECT");
			snprintf(modem.reset,       61, "AT&F&C1&D2X4W2B0M0&K3\\r");
			snprintf(modem.error[0],    21, "BUSY");
			snprintf(modem.error[1],    21, "NO CARRIER");
			snprintf(modem.error[2],    21, "NO DIALTONE");
			snprintf(modem.error[3],    21, "NO ANSWER");
			snprintf(modem.error[4],    21, "RING\\r");
			snprintf(modem.error[5],    21, "ERROR");
			snprintf(modem.error[6],    21, "CONNECT VOICE");
			snprintf(modem.speed,       16, "28800");
			modem.available = TRUE;
			modem.costoffset = 6;
			fwrite(&modem, sizeof(modem), 1, fil);

			snprintf(modem.modem,       31, "ISDN Dynalink");
			snprintf(modem.init[0],     61, "ATZ\\r");
			snprintf(modem.init[1],     61, "AT&E3306018793\\r");
			snprintf(modem.init[2],     61, "AT&B512\\r");
			snprintf(modem.hangup,      41, "ATH0\\r");
			snprintf(modem.speed,       16, "64000");
			modem.costoffset = 1;
			fwrite(&modem, sizeof(modem), 1, fil);

			snprintf(modem.modem,       31, "Standard Hayes V34");
                        snprintf(modem.init[0],     61, "ATZ\\r");
			memset(&modem.init[1], 0, sizeof(modem.init[1]));
			memset(&modem.init[2], 0, sizeof(modem.init[2]));
                        memset(&modem.hangup, 0, sizeof(modem.hangup));
                        snprintf(modem.speed,       16, "33600");
                        modem.costoffset = 6;
			fwrite(&modem, sizeof(modem), 1, fil);

			memset(&modem, 0, sizeof(modem));
                        snprintf(modem.modem,       31, "ISDN Linux");
                        snprintf(modem.init[0],     61, "AT Z\\r");
                        snprintf(modem.ok,          11, "OK");
                        snprintf(modem.dial,        41, "ATDT\\T\\r");
			snprintf(modem.info,        41, "ATI2\\r");
			snprintf(modem.hangup,      41, "\\d\\p\\p\\p+++\\d\\p\\p\\pATH0\\r");
                        snprintf(modem.connect[0],  31, "CONNECT 64000");
                        snprintf(modem.connect[1],  31, "CONNECT");
                        snprintf(modem.error[0],    21, "BUSY");
                        snprintf(modem.error[1],    21, "NO CARRIER");
                        snprintf(modem.error[2],    21, "NO DIALTONE");
                        snprintf(modem.error[3],    21, "NO ANSWER");
                        snprintf(modem.error[4],    21, "RING\\r");
                        snprintf(modem.error[5],    21, "ERROR");
                        snprintf(modem.speed,       16, "64000");
                        modem.available = TRUE;
                        modem.costoffset = 1;
                        fwrite(&modem, sizeof(modem), 1, fil);

			fclose(fil);
			chmod(ffile, 0640);
			return 4;
		} else
			return -1;
	}

	fread(&modemhdr, sizeof(modemhdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - modemhdr.hdrsize) / modemhdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenModem(void);
int OpenModem(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/modem.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&modemhdr, sizeof(modemhdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = modemhdr.recsize;
			if (oldsize != sizeof(modem)) {
				ModemUpdated = 1;
				Syslog('+', "Updated %s, format changed", fnin);
			} else
				ModemUpdated = 0;
			modemhdr.hdrsize = sizeof(modemhdr);
			modemhdr.recsize = sizeof(modem);
			fwrite(&modemhdr, sizeof(modemhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&modem, 0, sizeof(modem));
			while (fread(&modem, oldsize, 1, fin) == 1) {
				fwrite(&modem, sizeof(modem), 1, fout);
				memset(&modem, 0, sizeof(modem));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseModem(int);
void CloseModem(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*mdm = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/modem.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/modem.temp", getenv("MBSE_ROOT"));

	if (ModemUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&modemhdr, modemhdr.hdrsize, 1, fi);
			fwrite(&modemhdr, modemhdr.hdrsize, 1, fo);

			while (fread(&modem, modemhdr.recsize, 1, fi) == 1)
				if (!modem.deleted)
					fill_stlist(&mdm, modem.modem, ftell(fi) - modemhdr.recsize);
			sort_stlist(&mdm);

			for (tmp = mdm; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&modem, modemhdr.recsize, 1, fi);
				fwrite(&modem, modemhdr.recsize, 1, fo);
			}

			tidy_stlist(&mdm);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"modem.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendModem(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&modem, 0, sizeof(modem));
		snprintf(modem.init[0], 61, "ATZ\\r");
		snprintf(modem.ok, 11, "OK");
		snprintf(modem.dial, 41, "ATDT\\T\\r");
		snprintf(modem.connect[0], 31, "CONNECT 56000");
		snprintf(modem.connect[1], 31, "CONNECT 48000");
		snprintf(modem.connect[2], 31, "CONNECT 44000");
		snprintf(modem.connect[3], 31, "CONNECT 41333");
		snprintf(modem.connect[4], 31, "CONNECT 38000");
		snprintf(modem.connect[5], 31, "CONNECT 33600");
		snprintf(modem.connect[6], 31, "CONNECT 31200");
		snprintf(modem.connect[7], 31, "CONNECT 28800");
		snprintf(modem.connect[8], 31, "CONNECT 26400");
		snprintf(modem.connect[9], 31, "CONNECT 24000");
		snprintf(modem.connect[10], 31, "CONNECT 21600");
		snprintf(modem.connect[11], 31, "CONNECT 19200");
		snprintf(modem.connect[12], 31, "CONNECT 16800");
		snprintf(modem.connect[13], 31, "CONNECT 14400");
		snprintf(modem.connect[14], 31, "CONNECT 12000");
		snprintf(modem.connect[15], 31, "CONNECT 9600");
		snprintf(modem.connect[16], 31, "CONNECT 7200");
		snprintf(modem.connect[17], 31, "CONNECT 4800");
		snprintf(modem.connect[18], 31, "CONNECT 2400");
		snprintf(modem.connect[19], 31, "CONNECT");
		snprintf(modem.error[0], 21, "BUSY");
		snprintf(modem.error[1], 21, "NO CARRIER");
		snprintf(modem.error[2], 21, "NO DIALTONE");
		snprintf(modem.error[3], 21, "NO ANSWER");
		snprintf(modem.error[4], 21, "RING\\r");
		snprintf(modem.error[5], 21, "ERROR");
		snprintf(modem.speed, 16, "28800");
		snprintf(modem.reset, 61, "AT\\r");
		modem.available = TRUE;
		fwrite(&modem, sizeof(modem), 1, fil);
		fclose(fil);
		ModemUpdated = 1;
		return 0;
	} else
		return -1;
}



void Modem_Screen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "5.  EDIT MODEM TYPE");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.  Type");
	mbse_mvprintw( 8, 2, "2.  Init 1");
	mbse_mvprintw( 9, 2, "3.  Init 2");
	mbse_mvprintw(10, 2, "4.  Init 3");
	mbse_mvprintw(11, 2, "5.  Reset");
	mbse_mvprintw(12, 2, "6.  Hangup");
	mbse_mvprintw(13, 2, "7.  Dial");
	mbse_mvprintw(14, 2, "8.  Info");
	mbse_mvprintw(15, 2, "9.  Ok");
	mbse_mvprintw(16, 2, "10. Offset");
	mbse_mvprintw(17, 2, "11. Speed");

	mbse_mvprintw(15,30, "12. Available");
	mbse_mvprintw(16,30, "13. Deleted");
	mbse_mvprintw(17,30, "14. Stripdash");

	mbse_mvprintw(15,58, "15. Connect strings");
	mbse_mvprintw(16,58, "16. Error strings");
}



void EditConnect(void)
{
	int	i, j;

	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "5.15  EDIT MODEM CONNECT STRINGS");
	set_color(CYAN, BLACK);

	for (i = 0; i < 10; i++) {
		mbse_mvprintw( 7+i, 2, (char *)"%2d.", i+1);
		mbse_mvprintw( 7+i,42, (char *)"%2d.", i+11);
	}

	for (;;) {
		set_color(WHITE, BLACK);
		for (i = 0; i < 10; i++) {
			show_str( 7+i, 8, 30, modem.connect[i]);
			show_str( 7+i,48, 30, modem.connect[i+10]);
		}

		j = select_menu(20);
		if (j == 0)
			return;
		if (j < 11)
			strcpy(modem.connect[j-1], edit_str(6+j, 8,30, modem.connect[j-1], (char *)"^Connect^ string"));
		else
			strcpy(modem.connect[j-1], edit_str(j-4,48,30, modem.connect[j-1], (char *)"^Connect^ string"));
	}
}



void EditError(void)
{
	int	i, j;

	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "5.16  EDIT MODEM ERROR STRINGS");
	set_color(CYAN, BLACK);

	for (i = 0; i < 10; i++) {
		mbse_mvprintw( 7+i, 2, (char *)"%2d.", i+1);
	}

	for (;;) {
		set_color(WHITE, BLACK);
		for (i = 0; i < 10; i++) {
			show_str( 7+i, 8, 20, modem.error[i]);
		}

		j = select_menu(10);
		if (j == 0)
			return;
		strcpy(modem.error[j-1], edit_str(6+j, 8,20, modem.error[j-1], (char *)"^Error^ string"));
	}
}



int EditModemRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	int	offset;
	int	j;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Modem");

	snprintf(mfile, PATH_MAX, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(modemhdr) + ((Area -1) * sizeof(modem));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&modem, sizeof(modem), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&modem, crc, sizeof(modem));
	Modem_Screen();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,14,30, modem.modem);
		show_str( 8,14,60, modem.init[0]);
		show_str( 9,14,60, modem.init[1]);
		show_str(10,14,60, modem.init[2]);
		show_str(11,14,60, modem.reset);
		show_str(12,14,40, modem.hangup);
		show_str(13,14,40, modem.dial);
		show_str(14,14,40, modem.info);
		show_str(15,14,10, modem.ok);
		show_int(16,14,    modem.costoffset);
		show_str(17,14,15, modem.speed);

		show_bool(15,44, modem.available);
		show_bool(16,44, modem.deleted);
		show_bool(17,44, modem.stripdash);

		j = select_menu(16);
		switch(j) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&modem, crc1, sizeof(modem));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&modem, sizeof(modem), 1, fil);
					fclose(fil);
					ModemUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1: E_STR( 7,14,30, modem.modem,      "The ^Type^ or brand of this modem")
		case 2: E_STR( 8,14,60, modem.init[0],    "The ^first init^ string for this modem")
		case 3: E_STR( 9,14,60, modem.init[1],    "The ^second init^ string for this modem")
		case 4: E_STR(10,14,60, modem.init[2],    "The ^third init^ string for this modem")
		case 5: E_STR(11,14,60, modem.reset,      "The ^reset^ string for this modem")
		case 6: E_STR(12,14,40, modem.hangup,     "The ^hangup^ string for this modem (Leave empty for ^DTR-drop^ hangup)")
		case 7: E_STR(13,14,40, modem.dial,       "The ^dial^ command for this modem, ^\\T^ is translated phonenumber")
		case 8: E_STR(14,14,40, modem.info,       "The command to get connection ^info^ from this modem after call")
		case 9: E_STR(15,14,10, modem.ok,         "The ^OK^ string to get from the modem")
		case 10:E_IRC(16,14,    modem.costoffset, 0, 60, "The ^offset^ time in seconds between answer and connect string (0..60)")
		case 11:E_STR(17,14,15, modem.speed,      "The ^EMSI speed^ message for this modem")
		case 12:E_BOOL(15,44,   modem.available,  "If this modem is ^available^")
		case 13:E_BOOL(16,44,   modem.deleted,    "If this modem is to be ^deleted^ from the setup")
		case 14:E_BOOL(17,44,   modem.stripdash,  "Strip ^dashes (-)^ from dial command strings if needed")
		case 15:
			EditConnect();
			Modem_Screen();
			break;
		case 16:
			EditError();
			Modem_Screen();
			break;
		}
	}

	return 0;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
void EditModem(void)
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

	records = CountModem();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenModem() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "5.  MODEM TYPES SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&modemhdr, sizeof(modemhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(modemhdr) + ((i - 1) * modemhdr.recsize);
					fseek(fil, offset, 0);
					fread(&modem, modemhdr.recsize, 1, fil);
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if (modem.available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					snprintf(temp, 81, "%3d.  %-30s", i, modem.modem);
					temp[37] = 0;
					mbse_mvprintw(y, x, temp);
					y++;
				}
				fclose(fil);
			}
			/* Show records here */
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseModem(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendModem() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditModemRec(atoi(pick));
	}
}



void InitModem(void)
{
    CountModem();
    OpenModem();
    CloseModem(TRUE);
}



char *PickModem(char *shdr)
{
	int	records, i, o = 0, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;
	static char buf[31];

	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return '\0';
	}

	records = CountModem();
	if (records == -1) {
		working(2, 0, 0);
		return '\0';
	}


	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.  MODEM SELECT", shdr);
		mbse_mvprintw(5,3,temp);
		set_color(CYAN, BLACK);
		if (records) {
			snprintf(temp, PATH_MAX, "%s/etc/modem.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&modemhdr, sizeof(modemhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(modemhdr) + (((o + i) - 1) * modemhdr.recsize);
						fseek(fil, offset, SEEK_SET);
						fread(&modem, modemhdr.recsize, 1, fil);
						if (modem.available)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						snprintf(temp, 81, "%3d.  %-31s", o + i, modem.modem);
						temp[38] = '\0';
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_pick(records, 20));

		if (strncmp(pick, "-", 1) == 0)
			return '\0';

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o += 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o -= 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			snprintf(temp, PATH_MAX, "%s/etc/modem.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				offset = modemhdr.hdrsize + ((atoi(pick) - 1) * modemhdr.recsize);
				fseek(fil, offset, SEEK_SET);
				fread(&modem, modemhdr.recsize, 1, fil);
				fclose(fil);
				if (modem.available) {
					snprintf(buf, 31, "%s", modem.modem);
					return buf;
				}
			}
		}
	}
}



int modem_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *ti, *wp, *ip, *mdm;
    int	    refs, nr = 0, i, j;

    snprintf(temp, PATH_MAX, "%s/etc/modem.data", getenv("MBSE_ROOT"));
    if ((mdm = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 5, 0, page, (char *)"Modem types information");
    j = 0;

    fprintf(fp, "\n\n");
    fread(&modemhdr, sizeof(modemhdr), 1, mdm);

    ip = open_webdoc((char *)"modem.html", (char *)"Modems", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Nr</TH><TH align='left'>Comment</TH><TH align='left'>Available</TH></TR>\n");

    while ((fread(&modem, modemhdr.recsize, 1, mdm)) == 1) {
	if (j == 1) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	nr++;
	fprintf(ip, " <TR><TD><A HREF=\"modem_%d.html\">%d</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		nr, nr, modem.modem, getboolean(modem.available));
	snprintf(temp, 81, "modem_%d.html", nr);
	if ((wp = open_webdoc(temp, (char *)"Modem", modem.modem))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"modem.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Modem type", modem.modem);
	    for (i = 0; i < 3; i++)
		if (strlen(modem.init[i])) {
		    snprintf(temp, 81, "Init string %d", i+1);
		    add_webtable(wp, temp, modem.init[i]);
		}
	    add_webtable(wp, (char *)"OK string", modem.ok);
	    add_webtable(wp, (char *)"Hangup", modem.hangup);
	    add_webtable(wp, (char *)"Info command", modem.info);
	    add_webtable(wp, (char *)"Dial command", modem.dial);
	    for (i = 0; i < 20; i++)
		if (strlen(modem.connect[i]))
		    add_webtable(wp, (char *)"Connect", modem.connect[i]);
	    add_webtable(wp, (char *)"Reset", modem.reset);
	    for (i = 0; i < 10; i++)
		if (strlen(modem.error[i]))
		    add_webtable(wp, (char *)"Error string", modem.error[i]);
	    add_webdigit(wp, (char *)"Cost offset", modem.costoffset);
	    add_webtable(wp, (char *)"EMSI speed", modem.speed);
	    add_webtable(wp, (char *)"Strip dashes", getboolean(modem.stripdash));
	    add_webtable(wp, (char *)"Available", getboolean(modem.available));
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>TTY Lines Reference</H3>\n");
	    refs = 0;
	    snprintf(temp, PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	    if ((ti = fopen(temp, "r"))) {
		fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, ti);
		fseek(ti, 0, SEEK_SET);
		fread(&ttyinfo, ttyinfohdr.hdrsize, 1, ti);
		while ((fread(&ttyinfo, ttyinfohdr.recsize, 1, ti)) == 1) {
		    if (ttyinfo.available && (strcmp(modem.modem, ttyinfo.modem) == 0)) {
			if (refs == 0) {
			    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			    fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
			    fprintf(wp, "<TBODY>\n");
			}
			fprintf(wp, "<TR><TD><A HREF=\"ttyinfo_%s.html\">%s</A></TD><TD>%s</TD></TR>\n", 
				ttyinfo.tty, ttyinfo.tty, ttyinfo.comment);
			refs++;
		    }
		}
		fclose(ti);
	    }
	    if (refs == 0)
		fprintf(wp, "No TTY Lines References\n");
	    else {
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
	    }
	    close_webdoc(wp);
	}

	fprintf(fp, "     Modem type   %s\n", modem.modem);
	for (i = 0; i < 3; i++)
	    fprintf(fp, "     Init string  %s\n", modem.init[i]);
	fprintf(fp, "     OK string    %s\n", modem.ok);
	fprintf(fp, "     Hangup       %s\n", modem.hangup);
	fprintf(fp, "     Info command %s\n", modem.info);
	fprintf(fp, "     Dial command %s\n", modem.dial);
	for (i = 0; i < 20; i++)
	    fprintf(fp, "     Connect      %s\n", modem.connect[i]);
	fprintf(fp, "     Reset cmd    %s\n", modem.reset);
	for (i = 0; i < 10; i++)
	    fprintf(fp, "     Error string %s\n", modem.error[i]);
	fprintf(fp, "     Cost offset  %d\n", modem.costoffset);
	fprintf(fp, "     EMSI speed   %s\n", modem.speed);
	fprintf(fp, "     Strip dashes %s\n", getboolean(modem.stripdash));
	fprintf(fp, "     Available    %s\n", getboolean(modem.available));
	fprintf(fp, "\n\n\n");
	j++;
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	
    fclose(mdm);
    return page;
}


