/*****************************************************************************
 *
 * File ..................: setup/m_modem.c
 * Purpose ...............: Setup Modem structure.
 * Last modification date : 19-Oct-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
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

	sprintf(ffile, "%s/etc/modem.data", getenv("MBSE_ROOT"));
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
			sprintf(modem.modem,       "Dynalink 1428EXTRA");
			sprintf(modem.init[0],     "AT Z\\r");
			sprintf(modem.init[1],     "AT &F &C1 &D2 X4 W2 B0 M0 \\\\V1 \\\\G0 &K3 S37=0\\r");
			sprintf(modem.ok,          "OK");
			sprintf(modem.dial,        "ATDT\\T\\r");
			sprintf(modem.connect[0],  "CONNECT 56000");
			sprintf(modem.connect[1],  "CONNECT 48000");
			sprintf(modem.connect[2],  "CONNECT 44000");
			sprintf(modem.connect[3],  "CONNECT 41333");
			sprintf(modem.connect[4],  "CONNECT 38000");
			sprintf(modem.connect[5],  "CONNECT 33600");
			sprintf(modem.connect[6],  "CONNECT 31200");
			sprintf(modem.connect[7],  "CONNECT 28800");
			sprintf(modem.connect[8],  "CONNECT 26400");
			sprintf(modem.connect[9],  "CONNECT 24000");
			sprintf(modem.connect[10], "CONNECT 21600");
			sprintf(modem.connect[11], "CONNECT 19200");
			sprintf(modem.connect[12], "CONNECT 16800");
			sprintf(modem.connect[13], "CONNECT 14400");
			sprintf(modem.connect[14], "CONNECT 12000");
			sprintf(modem.connect[15], "CONNECT 9600");
			sprintf(modem.connect[16], "CONNECT 7200");
			sprintf(modem.connect[17], "CONNECT 4800");
			sprintf(modem.connect[18], "CONNECT 2400");
			sprintf(modem.connect[19], "CONNECT");
			sprintf(modem.reset,       "AT&F&C1&D2X4W2B0M0&K3\\r");
			sprintf(modem.error[0],    "BUSY");
			sprintf(modem.error[1],    "NO CARRIER");
			sprintf(modem.error[2],    "NO DIALTONE");
			sprintf(modem.error[3],    "NO ANSWER");
			sprintf(modem.error[4],    "RING\\r");
			sprintf(modem.error[5],    "ERROR");
			sprintf(modem.error[6],    "CONNECT VOICE");
			sprintf(modem.speed,       "28800");
			modem.available = TRUE;
			modem.costoffset = 6;
			fwrite(&modem, sizeof(modem), 1, fil);

			sprintf(modem.modem,       "ISDN Dynalink");
			sprintf(modem.init[0],     "ATZ\\r");
			sprintf(modem.init[1],     "AT&E3306018793\\r");
			sprintf(modem.init[2],     "AT&B512\\r");
			sprintf(modem.hangup,      "ATH0\\r");
			sprintf(modem.speed,       "64000");
			modem.costoffset = 1;
			fwrite(&modem, sizeof(modem), 1, fil);

			sprintf(modem.modem,       "Standard Hayes V34");
                        sprintf(modem.init[0],     "ATZ\\r");
			memset(&modem.init[1], 0, sizeof(modem.init[1]));
			memset(&modem.init[2], 0, sizeof(modem.init[2]));
                        memset(&modem.hangup, 0, sizeof(modem.hangup));
                        sprintf(modem.speed,       "33600");
                        modem.costoffset = 6;
			fwrite(&modem, sizeof(modem), 1, fil);

			memset(&modem, 0, sizeof(modem));
                        sprintf(modem.modem,       "ISDN Linux");
                        sprintf(modem.init[0],     "AT Z\\r");
                        sprintf(modem.ok,          "OK");
                        sprintf(modem.dial,        "ATDT\\T\\r");
			sprintf(modem.info,        "ATI2\\r");
			sprintf(modem.hangup,      "\\d\\p\\p\\p+++\\d\\p\\p\\pATH0\\r");
                        sprintf(modem.connect[0],  "CONNECT 64000");
                        sprintf(modem.connect[1],  "CONNECT");
                        sprintf(modem.error[0],    "BUSY");
                        sprintf(modem.error[1],    "NO CARRIER");
                        sprintf(modem.error[2],    "NO DIALTONE");
                        sprintf(modem.error[3],    "NO ANSWER");
                        sprintf(modem.error[4],    "RING\\r");
                        sprintf(modem.error[5],    "ERROR");
                        sprintf(modem.speed,       "64000");
                        modem.available = TRUE;
                        modem.costoffset = 1;
                        fwrite(&modem, sizeof(modem), 1, fil);

			fclose(fil);
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
	long	oldsize;

	sprintf(fnin,  "%s/etc/modem.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
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

	sprintf(fin, "%s/etc/modem.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/modem.temp", getenv("MBSE_ROOT"));

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
			Syslog('+', "Updated \"modem.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
}



int AppendModem(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&modem, 0, sizeof(modem));
		sprintf(modem.init[0], "ATZ\\r");
		sprintf(modem.ok, "OK");
		sprintf(modem.dial, "ATDT\\T\\r");
		sprintf(modem.connect[0], "CONNECT 56000");
		sprintf(modem.connect[1], "CONNECT 48000");
		sprintf(modem.connect[2], "CONNECT 44000");
		sprintf(modem.connect[3], "CONNECT 41333");
		sprintf(modem.connect[4], "CONNECT 38000");
		sprintf(modem.connect[5], "CONNECT 33600");
		sprintf(modem.connect[6], "CONNECT 31200");
		sprintf(modem.connect[7], "CONNECT 28800");
		sprintf(modem.connect[8], "CONNECT 26400");
		sprintf(modem.connect[9], "CONNECT 24000");
		sprintf(modem.connect[10], "CONNECT 21600");
		sprintf(modem.connect[11], "CONNECT 19200");
		sprintf(modem.connect[12], "CONNECT 16800");
		sprintf(modem.connect[13], "CONNECT 14400");
		sprintf(modem.connect[14], "CONNECT 12000");
		sprintf(modem.connect[15], "CONNECT 9600");
		sprintf(modem.connect[16], "CONNECT 7200");
		sprintf(modem.connect[17], "CONNECT 4800");
		sprintf(modem.connect[18], "CONNECT 2400");
		sprintf(modem.connect[19], "CONNECT");
		sprintf(modem.error[0], "BUSY");
		sprintf(modem.error[1], "NO CARRIER");
		sprintf(modem.error[2], "NO DIALTONE");
		sprintf(modem.error[3], "NO ANSWER");
		sprintf(modem.error[4], "RING\\r");
		sprintf(modem.error[5], "ERROR");
		sprintf(modem.speed, "28800");
		sprintf(modem.reset, "AT\\r");
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
	mvprintw( 5, 2, "5.  EDIT MODEM TYPE");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Type");
	mvprintw( 8, 2, "2.  Init 1");
	mvprintw( 9, 2, "3.  Init 2");
	mvprintw(10, 2, "4.  Init 3");
	mvprintw(11, 2, "5.  Reset");
	mvprintw(12, 2, "6.  Hangup");
	mvprintw(13, 2, "7.  Dial");
	mvprintw(14, 2, "8.  Info");
	mvprintw(15, 2, "9.  Ok");
	mvprintw(16, 2, "10. Offset");
	mvprintw(17, 2, "11. Speed");

	mvprintw(15,30, "12. Available");
	mvprintw(16,30, "13. Deleted");
	mvprintw(17,30, "14. Stripdash");

	mvprintw(15,58, "15. Connect strings");
	mvprintw(16,58, "16. Error strings");
}



void EditConnect(void)
{
	int	i, j;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "5.15  EDIT MODEM CONNECT STRINGS");
	set_color(CYAN, BLACK);

	for (i = 0; i < 10; i++) {
		mvprintw( 7+i, 2, (char *)"%2d.", i+1);
		mvprintw( 7+i,42, (char *)"%2d.", i+11);
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
	mvprintw( 5, 2, "5.16  EDIT MODEM ERROR STRINGS");
	set_color(CYAN, BLACK);

	for (i = 0; i < 10; i++) {
		mvprintw( 7+i, 2, (char *)"%2d.", i+1);
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
	long	offset;
	int	j;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Modem");

	sprintf(mfile, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
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
	working(0, 0, 0);
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
					working(1, 0, 0);
					working(0, 0, 0);
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
		case 10:E_INT(16,14,    modem.costoffset, "The ^offset^ time in seconds between answer and connect string")
		case 11:E_STR(17,14,15, modem.speed,      "The ^EMSI speed^ message for this modem")
		case 12:E_BOOL(15,44,   modem.available,  "If this modem is ^available^")
		case 13:E_BOOL(16,44,   modem.deleted,    "If this modem is to be ^deleted^ from the setup")
		case 14:E_BOOL(17,44,   modem.stripdash,  "Stript ^dashes (-)^ from dial command strings if this modem needs it")
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
	long	offset;

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
	working(0, 0, 0);

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "5.  MODEM TYPES SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/modem.temp", getenv("MBSE_ROOT"));
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
					sprintf(temp, "%3d.  %-30s", i, modem.modem);
					temp[37] = 0;
					mvprintw(y, x, temp);
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
			working(0, 0, 0);
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
	long	offset;
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

	working(0, 0, 0);

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		sprintf(temp, "%s.  MODEM SELECT", shdr);
		mvprintw(5,3,temp);
		set_color(CYAN, BLACK);
		if (records) {
			sprintf(temp, "%s/etc/modem.data", getenv("MBSE_ROOT"));
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
						sprintf(temp, "%3d.  %-31s", o + i, modem.modem);
						temp[38] = '\0';
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
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
			sprintf(temp, "%s/etc/modem.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				offset = modemhdr.hdrsize + ((atoi(pick) - 1) * modemhdr.recsize);
				fseek(fil, offset, SEEK_SET);
				fread(&modem, modemhdr.recsize, 1, fil);
				fclose(fil);
				if (modem.available) {
					sprintf(buf, "%s", modem.modem);
					return buf;
				}
			}
		}
	}
}



int modem_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[PATH_MAX];
	FILE	*mdm;
	int	i, j;

	sprintf(temp, "%s/etc/modem.data", getenv("MBSE_ROOT"));
	if ((mdm = fopen(temp, "r")) == NULL)
		return page;

	page = newpage(fp, page);
	addtoc(fp, toc, 5, 0, page, (char *)"Modem types information");
	j = 0;

	fprintf(fp, "\n\n");
	fread(&modemhdr, sizeof(modemhdr), 1, mdm);

	while ((fread(&modem, modemhdr.recsize, 1, mdm)) == 1) {
		if (j == 1) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
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

	fclose(mdm);
	return page;
}


