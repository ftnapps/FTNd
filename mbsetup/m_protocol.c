/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Protocols.
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
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "../paths.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_protocol.h"



int	ProtUpdated = 0;


/*
 * Count nr of PROT records in the database.
 * Creates the database if it doesn't exist.
 */
int CountProtocol(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			PROThdr.hdrsize = sizeof(PROThdr);
			PROThdr.recsize = sizeof(PROT);
			fwrite(&PROThdr, sizeof(PROThdr), 1, fil);

			memset(&PROT, 0, sizeof(PROT));
			sprintf(PROT.ProtKey,      "1");
			sprintf(PROT.ProtName,     "Ymodem");
			if (strlen(_PATH_SB) && strlen(_PATH_RB)) {
			    sprintf(PROT.ProtUp,       "%s -v", _PATH_RB);
			    sprintf(PROT.ProtDn,       "%s -v -u", _PATH_SB);
			    PROT.Available = TRUE;
			} else {
			    sprintf(PROT.ProtUp,       "/usr/bin/rb -v");
			    sprintf(PROT.ProtDn,       "/usr/bin/sb -v -u");
			    PROT.Available = FALSE;
			}
			sprintf(PROT.Advice,       "Press Ctrl-X to abort");
			PROT.Batch = TRUE;
			PROT.Efficiency = 92;
			fwrite(&PROT, sizeof(PROT), 1, fil);

                        memset(&PROT, 0, sizeof(PROT));
                        sprintf(PROT.ProtKey,      "L");
                        sprintf(PROT.ProtName,     "Local disk");
                        sprintf(PROT.ProtUp,       "%s/bin/rf", getenv("MBSE_ROOT"));
                        sprintf(PROT.ProtDn,       "%s/bin/sf", getenv("MBSE_ROOT"));
                        sprintf(PROT.Advice,       "It goes before you know");
			PROT.Available = FALSE;
                        PROT.Efficiency = 100;
			PROT.Batch = TRUE;
                        fwrite(&PROT, sizeof(PROT), 1, fil);

                        memset(&PROT, 0, sizeof(PROT));
                        sprintf(PROT.ProtKey,      "Y");
                        sprintf(PROT.ProtName,     "Ymodem 1K");
			if (strlen(_PATH_SB) && strlen(_PATH_RB)) {
			    sprintf(PROT.ProtUp,       "%s -k -v", _PATH_RB);
			    sprintf(PROT.ProtDn,       "%s -k -v -u", _PATH_SB);
			    PROT.Available = TRUE;
			} else {
			    sprintf(PROT.ProtUp,       "/usr/bin/rb -k -v");
			    sprintf(PROT.ProtDn,       "/usr/bin/sb -k -v -u");
			    PROT.Available = FALSE;
			}
                        sprintf(PROT.Advice,       "Press Ctrl-X to abort");
			PROT.Batch = TRUE;
                        PROT.Efficiency = 95;
                        fwrite(&PROT, sizeof(PROT), 1, fil);

                        memset(&PROT, 0, sizeof(PROT));
                        sprintf(PROT.ProtKey,      "Z");
                        sprintf(PROT.ProtName,     "Zmodem");
			if (strlen(_PATH_SZ) && strlen(_PATH_RZ)) {
			    sprintf(PROT.ProtUp,       "%s -p -v", _PATH_RZ);
			    sprintf(PROT.ProtDn,       "%s -b -q -r -u", _PATH_SZ);
			    PROT.Available = TRUE;
			} else {
			    sprintf(PROT.ProtUp,       "/usr/bin/rz -p -v");
			    sprintf(PROT.ProtDn,       "/usr/bin/sz -b -q -r -u");
			    PROT.Available = FALSE;
			}
                        sprintf(PROT.Advice,       "Press Ctrl-X to abort");
			PROT.Batch = TRUE;
                        PROT.Efficiency = 98;
                        fwrite(&PROT, sizeof(PROT), 1, fil);

			fclose(fil);
			chmod(ffile, 0640);
			return 4;
		} else
			return -1;
	}

	fread(&PROThdr, sizeof(PROThdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - PROThdr.hdrsize) / PROThdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenProtocol(void);
int OpenProtocol(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&PROThdr, sizeof(PROThdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = PROThdr.recsize;
			if (oldsize != sizeof(PROT)) {
				ProtUpdated = 1;
				Syslog('+', "Upgraded %s, format changed", fnin);
			} else
				ProtUpdated = 0;
			PROThdr.hdrsize = sizeof(PROThdr);
			PROThdr.recsize = sizeof(PROT);
			fwrite(&PROThdr, sizeof(PROThdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&PROT, 0, sizeof(PROT));
			while (fread(&PROT, oldsize, 1, fin) == 1) {
				fwrite(&PROT, sizeof(PROT), 1, fout);
				memset(&PROT, 0, sizeof(PROT));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseProtocol(int);
void CloseProtocol(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*pro = NULL, *tmp;

	sprintf(fin, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/protocol.temp", getenv("MBSE_ROOT"));

	if (ProtUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&PROThdr, PROThdr.hdrsize, 1, fi);
			fwrite(&PROThdr, PROThdr.hdrsize, 1, fo);

			while (fread(&PROT, PROThdr.recsize, 1, fi) == 1)
				if (!PROT.Deleted)
					fill_stlist(&pro, PROT.ProtKey, ftell(fi) - PROThdr.recsize);
			sort_stlist(&pro);

			for (tmp = pro; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&PROT, PROThdr.recsize, 1, fi);
				fwrite(&PROT, PROThdr.recsize, 1, fo);
			}

			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			tidy_stlist(&pro);
			Syslog('+', "Updated \"protocol.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendProtocol(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&PROT, 0, sizeof(PROT));
		fwrite(&PROT, sizeof(PROT), 1, fil);
		fclose(fil);
		ProtUpdated = 1;
		return 0;
	} else
		return -1;
}



void s_protrec(void);
void s_protrec(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "8.5 EDIT PROTOCOL");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.  Select Key");
	mvprintw( 8, 6, "2.  Name");
	mvprintw( 9, 6, "3.  Upload");
	mvprintw(10, 6, "4.  Download");
	mvprintw(11, 6, "5.  Available");
	mvprintw(12, 6, "6.  Batching");
	mvprintw(13, 6, "7.  Bi direct");
	mvprintw(14, 6, "8.  Advice");
	mvprintw(15, 6, "9.  Efficiency");
	mvprintw(16, 6, "10. Deleted");
	mvprintw(17, 6, "11. Sec. level");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditProtRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Protocol");

	sprintf(mfile, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(PROThdr) + ((Area -1) * sizeof(PROT));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&PROT, sizeof(PROT), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&PROT, crc, sizeof(PROT));

	s_protrec();
	
	for (;;) {
		set_color(WHITE, BLACK);
		show_str(  7,21, 1, PROT.ProtKey);
		show_str(  8,21,20, PROT.ProtName);
		show_str(  9,21,50, PROT.ProtUp);
		show_str( 10,21,50, PROT.ProtDn);
		show_bool(11,21,    PROT.Available);
		show_bool(12,21,    PROT.Batch);
		show_bool(13,21,    PROT.Bidir);
		show_str( 14,21,30, PROT.Advice);
		show_int( 15,21,    PROT.Efficiency);
		show_bool(16,21,    PROT.Deleted);
		show_sec( 17,21,    PROT.Level);

		j = select_menu(11);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&PROT, crc1, sizeof(PROT));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&PROT, sizeof(PROT), 1, fil);
					fclose(fil);
					ProtUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_UPS(  7,21,1, PROT.ProtKey,   "The ^Key^ to select this protocol")
		case 2:	E_STR(  8,21,20,PROT.ProtName,  "The ^name^ of this protocol")
		case 3:	E_STR(  9,21,50,PROT.ProtUp,    "The ^Upload^ path, binary and parameters")
		case 4:	E_STR( 10,21,50,PROT.ProtDn,    "The ^Download^ path, binary and parameters")
		case 5:	E_BOOL(11,21,   PROT.Available, "Is this protocol ^available^")
		case 6:	E_BOOL(12,21,   PROT.Batch,     "Is this a ^batching^ transfer protocol")
		case 7:	E_BOOL(13,21,   PROT.Bidir,     "Is this protocol ^bidirectional^")
		case 8:	E_STR( 14,21,30,PROT.Advice,    "A small ^advice^ to the user, eg \"Press Ctrl-X to abort\"")
		case 9:	E_INT( 15,21,   PROT.Efficiency,"The ^efficiency^ in % of this protocol")
		case 10:E_BOOL(16,21,   PROT.Deleted,   "Is this protocol ^Deleted^")
		case 11:E_SEC( 17,21,   PROT.Level,     "8.5.11  PROTOCOL SECURITY LEVEL", s_protrec)
		}
	}

	return 0;
}



void EditProtocol(void)
{
	int	records, i, x;
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

	records = CountProtocol();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenProtocol() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "8.5 PROTOCOL SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&PROThdr, sizeof(PROThdr), 1, fil);
				x = 4;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(PROThdr) + ((i - 1) * PROThdr.recsize);
					fseek(fil, offset, 0);
					fread(&PROT, PROThdr.recsize, 1, fil);
					if (i == 11)
						x = 44;
					if (PROT.Available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					sprintf(temp, "%3d.  %s %-30s", i, PROT.ProtKey, PROT.ProtName);
					temp[37] = '\0';
					mvprintw(i + 6, x, temp);
				}
				fclose(fil);
			}
			/* Show records here */
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseProtocol(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendProtocol() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditProtRec(atoi(pick));
	}
}



void InitProtocol(void)
{
    CountProtocol();
    OpenProtocol();
    CloseProtocol(TRUE);
}



char *PickProtocol(int nr)
{
	static	char Prot[21] = "";
	int	records, i, x;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return Prot;
	}

	records = CountProtocol();
	if (records == -1) {
		working(2, 0, 0);
		return Prot;
	}


	clr_index();
	set_color(WHITE, BLACK);
	sprintf(temp, "%d.  PROTOCOL SELECT", nr);
	mvprintw( 5, 4, temp);
	set_color(CYAN, BLACK);
	if (records != 0) {
		sprintf(temp, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
		if ((fil = fopen(temp, "r")) != NULL) {
			fread(&PROThdr, sizeof(PROThdr), 1, fil);
			x = 2;
			set_color(CYAN, BLACK);
			for (i = 1; i <= records; i++) {
				offset = sizeof(PROThdr) + ((i - 1) * PROThdr.recsize);
				fseek(fil, offset, 0);
				fread(&PROT, PROThdr.recsize, 1, fil);
				if (i == 11)
					x = 42;
				if (PROT.Available)
					set_color(CYAN, BLACK);
				else
					set_color(LIGHTBLUE, BLACK);
				sprintf(temp, "%3d.  %s %-30s", i, PROT.ProtKey, PROT.ProtName);
				temp[37] = '\0';
				mvprintw(i + 6, x, temp);
			}
			strcpy(pick, select_pick(records, 20));

			if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
				offset = sizeof(PROThdr) + ((atoi(pick) - 1) * PROThdr.recsize);
				fseek(fil, offset, 0);
				fread(&PROT, PROThdr.recsize, 1, fil);
				strcpy(Prot, PROT.ProtName);
			}
			fclose(fil);
		}
	}
	return Prot;
}



int bbs_prot_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[PATH_MAX];
	FILE	*no;
	int	j;

	sprintf(temp, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	page = newpage(fp, page);
	addtoc(fp, toc, 8, 5, page, (char *)"BBS Transfer protocols");
	j = 0;
	fprintf(fp, "\n\n");
	fread(&PROThdr, sizeof(PROThdr), 1, no);

	while ((fread(&PROT, PROThdr.recsize, 1, no)) == 1) {

		if (j == 4) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
		}

		fprintf(fp, "   Selection key    %s\n", PROT.ProtKey);
		fprintf(fp, "   Protocol name    %s\n", PROT.ProtName);
		fprintf(fp, "   Upload command   %s\n", PROT.ProtUp);
		fprintf(fp, "   Download command %s\n", PROT.ProtDn);
		fprintf(fp, "   Available        %s\n", getboolean(PROT.Available));
		fprintf(fp, "   Batch protocol   %s\n", getboolean(PROT.Batch));
		fprintf(fp, "   Bidirectional    %s\n", getboolean(PROT.Bidir));
		fprintf(fp, "   User advice      %s\n", PROT.Advice);
		fprintf(fp, "   Efficiency       %d%%\n", PROT.Efficiency);
		fprintf(fp, "   Security level   %s\n", get_secstr(PROT.Level));
		fprintf(fp, "\n\n");

		j++;
	}

	fclose(no);
	return page;
}


