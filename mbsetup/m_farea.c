/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: File Setup Program 
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
#include "m_fgroup.h"
#include "m_archive.h"
#include "m_farea.h"
#include "m_fgroup.h"
#include "m_ngroup.h"


int	FileUpdated = 0;


/*
 * Count nr of area records in the database.
 * Creates the database if it doesn't exist.
 */
int CountFilearea(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			areahdr.hdrsize = sizeof(areahdr);
			areahdr.recsize = sizeof(area);
			fwrite(&areahdr, sizeof(areahdr), 1, fil);
			memset(&area, 0, sizeof(area));
			sprintf(area.Name, "Local general files");
			area.New       = TRUE;
			area.Dupes     = TRUE;
			area.FileFind  = TRUE;
			area.AddAlpha  = TRUE;
			area.FileReq   = TRUE;
			area.Available = TRUE;
			area.FileFind  = TRUE;
			sprintf(area.BbsGroup, "LOCAL");
			sprintf(area.NewGroup, "LOCAL");
			sprintf(area.Path, "%s/local/common", CFG.ftp_base);
			fwrite(&area, sizeof(area), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			sprintf(ffile, "%s/foobar", area.Path);
			mkdirs(ffile, 0755);
			return 1;
		} else
			return -1;
	}

	fread(&areahdr, sizeof(areahdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - areahdr.hdrsize) / areahdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenFilearea(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&areahdr, sizeof(areahdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = areahdr.recsize;
			if (oldsize != sizeof(area)) {
				FileUpdated = 1;
				Syslog('+', "Updated %s to new format", fnin);
			} else
				FileUpdated = 0;
			areahdr.hdrsize = sizeof(areahdr);
			areahdr.recsize = sizeof(area);
			fwrite(&areahdr, sizeof(areahdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&area, 0, sizeof(area));
			while (fread(&area, oldsize, 1, fin) == 1) {
				fwrite(&area, sizeof(area), 1, fout);
				memset(&area, 0, sizeof(area));
			}
			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseFilearea(int);
void CloseFilearea(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];

	sprintf(fin, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/fareas.temp", getenv("MBSE_ROOT"));

	if (FileUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"fareas.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
}



int AppendFilearea(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&area, 0, sizeof(area));
		/*
		 * Fill in default values
		 */
		area.New      = TRUE;
		area.Dupes    = TRUE;
		area.FileFind = TRUE;
		area.AddAlpha = TRUE;
		area.FileReq  = TRUE;
		strcpy(area.Path, CFG.ftp_base);
		fwrite(&area, sizeof(area), 1, fil);
		fclose(fil);
		FileUpdated = 1;
		return 0;
	} else
		return -1;
}



void FileScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 4, 2, "8.4 EDIT FILE AREA");
	set_color(CYAN, BLACK);
	mvprintw( 6, 2, "1.  Area Name");
	mvprintw( 7, 2, "2.  Path");
	mvprintw( 8, 2, "3.  Down Sec.");
	mvprintw( 9, 2, "4.  Upl. Sec.");
	mvprintw(10, 2, "5.  List Sec.");
	mvprintw(11, 2, "6.  Files.bbs");
	mvprintw(12, 2, "7.  Available");
	mvprintw(13, 2, "8.  Check new");
	mvprintw(14, 2, "9.  Dupecheck");
	mvprintw(15, 2, "10. Free area");
	mvprintw(16, 2, "11. Direct DL");
	mvprintw(17, 2, "12. Pwd upl.");
	mvprintw(18, 2, "13. Filefind");

	mvprintw(12,30, "14. Add alpha");
	mvprintw(13,30, "15. CDrom");
	mvprintw(14,30, "16. File req.");
	mvprintw(15,30, "17. BBS Group");
	mvprintw(16,30, "18. New group");
	mvprintw(17,30, "19. Min. age");
	mvprintw(18,30, "20. Password");

	mvprintw(12,59, "21. DL days");
	mvprintw(13,59, "22. FD days");
	mvprintw(14,59, "23. Move area");
	mvprintw(15,59, "24. Cost");
	mvprintw(16,59, "25. Archiver");
	mvprintw(17,59, "26. Upload");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditFileRec(int Area)
{
	FILE	*fil, *fp;
	char	mfile[PATH_MAX], *temp;
	long	offset;
	unsigned long crc, crc1;
	int	Available, files;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit File Area");

	sprintf(mfile, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	fread(&areahdr, sizeof(areahdr), 1, fil);
	offset = areahdr.hdrsize + ((Area -1) * areahdr.recsize);
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&area, areahdr.recsize, 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&area, crc, areahdr.recsize);
	working(0, 0, 0);
	FileScreen();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 6,16,44, area.Name);
		show_str( 7,16,64, area.Path);
		show_sec( 8,16, area.DLSec);
		show_sec( 9,16, area.UPSec);
		show_sec(10,16, area.LTSec);
		show_str(11,16,64, area.FilesBbs);
		show_bool(12,16, area.Available);
		show_bool(13,16, area.New);
		show_bool(14,16, area.Dupes);
		show_bool(15,16, area.Free);
		show_bool(16,16, area.DirectDL);
		show_bool(17,16, area.PwdUP);
		show_bool(18,16, area.FileFind);

		show_bool(12,44, area.AddAlpha);
		show_bool(13,44, area.CDrom);
		show_bool(14,44, area.FileReq);
		show_str(15,44,12, area.BbsGroup);
		show_str(16,44,12, area.NewGroup);
		show_int(17,44, area.Age);
		show_str(18,44,20, (char *)"********************");

		show_int(12,73, area.DLdays);
		show_int(13,73, area.FDdays);
		show_int(14,73, area.MoveArea);
		show_int(15,73, area.Cost);
		show_str(16,73, 5, area.Archiver);
		show_int(17,73, area.Upload);

		switch(select_menu(26)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&area, crc1, areahdr.recsize);
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&area, areahdr.recsize, 1, fil);
					fclose(fil);
					FileUpdated = 1;
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR(  6,16,44, area.Name,      "The ^name^ for this area")
		case 2:	E_PTH(  7,16,64, area.Path,      "The ^path^ for the files in this area")
		case 3:	E_SEC(  8,16,    area.DLSec,     "8.4.3  DOWNLOAD SECURITY", FileScreen)
		case 4:	E_SEC(  9,16,    area.UPSec,     "8.4.4  UPLOAD SECURITY", FileScreen)
		case 5:	E_SEC( 10,16,    area.LTSec,     "8.4.5  LIST SECURITY", FileScreen)
		case 6:	E_STR( 11,16,64, area.FilesBbs,  "The path and name of \"files.bbs\" if area is on CDROM")
		case 7:	Available = edit_bool(12, 16, area.Available, (char *)"Is this area ^available^");
			temp = calloc(PATH_MAX, sizeof(char));
			sprintf(temp, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);
			if (area.Available && !Available) {
			    /*
			     * Attempt to disable this area, but check first.
			     */
			    if ((fp = fopen(temp, "r"))) {
				fseek(fp, 0, SEEK_END);
				files = ftell(fp) / sizeof(file);
				if (files) {
				    errmsg("There are stil %d files in this area", files);
				    Available = TRUE;
				}
				fclose(fp);
			    }
			    if (!Available) {
				if (yes_no((char *)"Are you sure you want to delete this area") == 0)
				    Available = TRUE;
			    }
			    if (!Available) {
				/*
				 * Make it so
				 */
				unlink(temp);
				if (strlen(area.Path) && strcmp(area.Path, CFG.ftp_base)) {
				    /*
				     * Erase file in path if path is set and not the default
				     * FTP base path
				     */
				    sprintf(temp, "rm -r -f %s", area.Path);
				    system(temp);
				}
				memset(&area, 0, sizeof(area));
				/*
				 * Fill in default values
				 */
				area.New      = TRUE;
				area.Dupes    = TRUE;
				area.FileFind = TRUE;
				area.AddAlpha = TRUE;
				area.FileReq  = TRUE;
				strcpy(area.Path, CFG.ftp_base);
				Syslog('+', "Removed file area %d", Area);
			    }
			    area.Available = Available;
			    FileScreen();
			} 
			if (!area.Available && Available) {
			    area.Available = TRUE;
			    if ((fp = fopen(temp, "a+")) == NULL) {
				WriteError("$Can't create file database %s", temp);
			    } else {
				fclose(fp);
			    }
			    chmod(temp, 0660);
			}
			free(temp);
			break;
		case 8:	E_BOOL(13,16,    area.New,       "Include this area in ^new files^ check")
		case 9:	E_BOOL(14,16,    area.Dupes,     "Check this area for ^duplicates^ during upload")
		case 10:E_BOOL(15,16,    area.Free,      "Are all files ^free^ in this area")
		case 11:E_BOOL(16,16,    area.DirectDL,  "Allow ^direct download^ from this area")
		case 12:E_BOOL(17,16,    area.PwdUP,     "Allow ^password^ on uploads")
		case 13:E_BOOL(18,16,    area.FileFind,  "Search this area for ^filefind^ requests")
		case 14:E_BOOL(12,44,    area.AddAlpha,  "Add new files ^alphabetic^ or at the end")
		case 15:E_BOOL(13,44,    area.CDrom,     "Is this area on a ^CDROM^")
		case 16:E_BOOL(14,44,    area.FileReq,   "Allow ^file requests^ from this area")
		case 17:strcpy(area.BbsGroup, PickFGroup((char *)"8.4.17"));
			FileScreen();
			break;
		case 18:strcpy(area.NewGroup, PickNGroup((char *)"8.4.18"));
			FileScreen();
			break;
		case 19:E_INT( 17,44,    area.Age,       "The ^minimum age^ to access this area")
		case 20:E_STR( 18,44,20, area.Password,  "The ^password^ to access this area")
		case 21:E_INT( 12,73,    area.DLdays,    "The not ^downloaded days^ to move/kill files")
		case 22:E_INT( 13,73,    area.FDdays,    "The ^file age^ in days to move/kill files")
		case 23:E_INT( 14,73,    area.MoveArea,  "The ^area to move^ files to, 0 is kill")
		case 24:E_INT( 15,73,    area.Cost,      "The ^cost^ to download a file")
		case 25:strcpy(area.Archiver, PickArchive((char *)"8.4"));
			FileScreen();
			break;
		case 26:E_INT( 17,73,    area.Upload,    "The ^upload^ area, 0 if upload in this area")
		}
	}
}



void EditFilearea(void)
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

	records = CountFilearea();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenFilearea() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "8.4 FILE AREA SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&areahdr, sizeof(areahdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(areahdr) + (((o + i) - 1) * areahdr.recsize);
						fseek(fil, offset, 0);
						fread(&area, areahdr.recsize, 1, fil);
						if (area.Available)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, area.Name);
						temp[37] = 0;
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseFilearea(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendFilearea() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < records) 
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditFileRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



long PickFilearea(char *shdr)
{
	int	records, i, o = 0, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;

	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return 0;
	}

	records = CountFilearea();
	if (records == -1) {
		working(2, 0, 0);
		return 0;
	}

	working(0, 0, 0);

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		sprintf(temp, "%s.  FILE AREA SELECT", shdr);
		mvprintw(5,3,temp);
		set_color(CYAN, BLACK);
		if (records) {
			sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&areahdr, sizeof(areahdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(areahdr) + (((o + i) - 1) * areahdr.recsize);
						fseek(fil, offset, SEEK_SET);
						fread(&area, areahdr.recsize, 1, fil);
						if (area.Available)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-31s", o + i, area.Name);
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
			return 0;

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o += 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o -= 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				offset = areahdr.hdrsize + ((atoi(pick) - 1) * areahdr.recsize);
				fseek(fil, offset, SEEK_SET);
				fread(&area, areahdr.recsize, 1, fil);
				fclose(fil);
				if (area.Available)
					return atoi(pick);
			}
		}
	}
}



void InitFilearea(void)
{
    CountFilearea();
    OpenFilearea();
    CloseFilearea(TRUE);
}



int bbs_file_doc(FILE *fp, FILE *toc, int page)
{
	char		temp[PATH_MAX];
	FILE		*no;
	int		i = 0, j = 0;

	sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	fread(&areahdr, sizeof(areahdr), 1, no);
	page = newpage(fp, page);	
	addtoc(fp, toc, 8, 4, page, (char *)"BBS File areas");

	while ((fread(&area, areahdr.recsize, 1, no)) == 1) {

		i++;
		if (area.Available) {

			if (j == 1) {
				page = newpage(fp, page);
				j = 0;
			} else {
				j++;
			}
			fprintf(fp, "\n\n");
			fprintf(fp, "    Area number       %d\n", i);
			fprintf(fp, "    Area name         %s\n", area.Name);
			fprintf(fp, "    Files path        %s\n", area.Path);
			fprintf(fp, "    Download sec.     %s\n", get_secstr(area.DLSec));
			fprintf(fp, "    Upload security   %s\n", get_secstr(area.UPSec));
			fprintf(fp, "    List seccurity    %s\n", get_secstr(area.LTSec));
			fprintf(fp, "    Path to files.bbs %s\n", area.FilesBbs);
			fprintf(fp, "    Newfiles scan     %s\n", getboolean(area.New));
			fprintf(fp, "    Check upl. dupes  %s\n", getboolean(area.Dupes));
			fprintf(fp, "    Files are free    %s\n", getboolean(area.Free));
			fprintf(fp, "    Allow direct DL   %s\n", getboolean(area.DirectDL));
			fprintf(fp, "    Allow pwd upl.    %s\n", getboolean(area.PwdUP));
			fprintf(fp, "    Filefind on       %s\n", getboolean(area.FileFind));
			fprintf(fp, "    Add files sorted  %s\n", getboolean(area.AddAlpha));
			fprintf(fp, "    Files in CDROM    %s\n", getboolean(area.CDrom));
			fprintf(fp, "    Allow filerequst  %s\n", getboolean(area.FileReq));
			fprintf(fp, "    BBS group         %s\n", area.BbsGroup);
			fprintf(fp, "    Newfiles group    %s\n", area.NewGroup);
			fprintf(fp, "    Minimum age       %d\n", area.Age);
			fprintf(fp, "    Area password     %s\n", area.Password);
			fprintf(fp, "    Kill DL days      %d\n", area.DLdays);
			fprintf(fp, "    Kill FD days      %d\n", area.FDdays);
			fprintf(fp, "    Move to area      %d\n", area.MoveArea);
			fprintf(fp, "    Archiver          %s\n", area.Archiver);
			fprintf(fp, "    Upload area       %d\n", area.Upload);
		}
	}

	fclose(no);
	return page;
}


