/*****************************************************************************
 *
 * $Id: m_farea.c,v 1.48 2006/02/24 20:33:28 mbse Exp $
 * Purpose ...............: File Setup Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
#include "../lib/mbsedb.h"
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
int	FileForced = FALSE;



/*
 * Count nr of area records in the database.
 * Creates the database if it doesn't exist.
 */
int CountFilearea(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			areahdr.hdrsize = sizeof(areahdr);
			areahdr.recsize = sizeof(area);
			fwrite(&areahdr, sizeof(areahdr), 1, fil);
			memset(&area, 0, sizeof(area));
			snprintf(area.Name, 45, "Local general files");
			area.New       = TRUE;
			area.Dupes     = TRUE;
			area.FileFind  = TRUE;
			area.AddAlpha  = TRUE;
			area.FileReq   = TRUE;
			area.Available = TRUE;
			area.FileFind  = TRUE;
			area.Free      = TRUE;
			snprintf(area.BbsGroup, 13, "LOCAL");
			snprintf(area.NewGroup, 13, "LOCAL");
			snprintf(area.Path, 81, "%s/local/common", CFG.ftp_base);
			fwrite(&area, sizeof(area), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			snprintf(ffile, 81, "%s/foobar", area.Path);
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
    FILE    *fin, *fout;
    char    fnin[PATH_MAX], fnout[PATH_MAX];
    int	    oldsize;

    snprintf(fnin,  PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
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
		/*
		 * Clear obsolete fields
		 */
		if (area.xCDrom) {
		    area.xCDrom = FALSE;
		    FileUpdated = 1;
		}
		if (area.xCost) {
		    area.xCost = 0;
		    FileUpdated = 1;
		}
		if (strlen(area.xFilesBbs)) {
		    memset(&area.xFilesBbs, 0, 65);
		    FileUpdated = 1;
		}
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

	snprintf(fin,  PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));

	if (FileUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"fareas.data\"");
			disk_reset();
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendFilearea(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
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
		area.Free     = TRUE;
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
	mbse_mvprintw( 4, 2, "8.4 EDIT FILE AREA");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 6, 2, "1.  Area Name");
	mbse_mvprintw( 7, 2, "2.  Path");
	mbse_mvprintw( 8, 2, "3.  Down Sec.");
	mbse_mvprintw( 9, 2, "4.  Upl. Sec.");
	mbse_mvprintw(10, 2, "5.  List Sec.");
	mbse_mvprintw(11, 2, "6.  Available");
	mbse_mvprintw(12, 2, "7.  Check new");
	mbse_mvprintw(13, 2, "8.  Dupecheck");
	mbse_mvprintw(14, 2, "9.  Free area");
	mbse_mvprintw(15, 2, "10. Direct DL");
	mbse_mvprintw(16, 2, "11. Pwd upl.");
	mbse_mvprintw(17, 2, "12. Filefind");

	mbse_mvprintw(12,30, "13. Add alpha");
	mbse_mvprintw(13,30, "14. File req.");
	mbse_mvprintw(14,30, "15. BBS Group");
	mbse_mvprintw(15,30, "16. New group");
	mbse_mvprintw(16,30, "17. Min. age");
	mbse_mvprintw(17,30, "18. Password");

	mbse_mvprintw(12,59, "19. DL days");
	mbse_mvprintw(13,59, "20. FD days");
	mbse_mvprintw(14,59, "21. Move area");
	mbse_mvprintw(15,59, "22. Archiver");
	mbse_mvprintw(16,59, "23. Upload");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditFileRec(int Area)
{
    FILE		*fil;
    char		mfile[PATH_MAX], *temp, tpath[65], frpath[PATH_MAX], topath[PATH_MAX], *lnpath;
    unsigned int	crc, crc1;
    int			Available, files, rc, Force = FALSE, count, offset;
    DIR			*dp;
    struct dirent	*de;
    struct stat		stb;
    struct _fdbarea	*fdb_area = NULL;
    struct FILE_record  f_db;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit File Area");

    snprintf(mfile, PATH_MAX, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
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
    FileScreen();

    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 6,16,44, area.Name);
	show_str( 7,16,64, area.Path);
	show_sec( 8,16, area.DLSec);
	show_sec( 9,16, area.UPSec);
	show_sec(10,16, area.LTSec);
	show_bool(11,16, area.Available);
	show_bool(12,16, area.New);
	show_bool(13,16, area.Dupes);
	show_bool(14,16, area.Free);
	show_bool(15,16, area.DirectDL);
	show_bool(16,16, area.PwdUP);
	show_bool(17,16, area.FileFind);

	show_bool(12,44, area.AddAlpha);
	show_bool(13,44, area.FileReq);
	show_str(14,44,12, area.BbsGroup);
	show_str(15,44,12, area.NewGroup);
	show_int(16,44, area.Age);
	show_str(17,44,20, (char *)"********************");

	show_int(12,73, area.DLdays);
	show_int(13,73, area.FDdays);
	show_int(14,73, area.MoveArea);
	show_str(15,73, 5, area.Archiver);
	show_int(16,73, area.Upload);

	switch(select_menu(23)) {
	    case 0: crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&area, crc1, areahdr.recsize);
		    if (crc != crc1) {
			if (Force || yes_no((char *)"Record is changed, save") == 1) {
			    working(1, 0, 0);
			    if ((fil = fopen(mfile, "r+")) == NULL) {
				working(2, 0, 0);
				return -1;
			    }
			    fseek(fil, offset, 0);
			    fwrite(&area, areahdr.recsize, 1, fil);
			    fclose(fil);
			    FileUpdated = 1;
			    Syslog('+', "Updated file area %d", Area);
			    working(6, 0, 0);
			}
		    }
		    IsDoing("Browsing Menu");
		    return 0;
	    case 1: E_STR(  6,16,44, area.Name,      "The ^name^ for this area")
	    case 2: strcpy(tpath, area.Path);
		    strcpy(area.Path, edit_pth(7,16,64, area.Path, (char *)"The ^path^ for the files in this area", 0775));
		    if (strlen(tpath) && strlen(area.Path) && strcmp(tpath, area.Path) && strcmp(tpath, CFG.ftp_base)) {
			if ((dp = opendir(tpath)) == NULL) {
			    WriteError("Can't open directory %s", tpath);
			} else {
			    working(5, 0, 0);
			    count = 0;
			    Syslog('+', "Moving files from %s to %s", tpath, area.Path);
			    lnpath = calloc(PATH_MAX, sizeof(char));
			    while ((de = readdir(dp))) {
				snprintf(frpath, PATH_MAX, "%s/%s", tpath, de->d_name);
			        snprintf(topath, PATH_MAX, "%s/%s", area.Path, de->d_name);
			        if (lstat(frpath, &stb) == 0) {
				    if (S_ISREG(stb.st_mode)) {
				        /*
				         * The real files, also files.bbs, index.html etc.
				         */
				        rc = file_mv(frpath, topath);
				        if (rc)
					    WriteError("mv %s to %s rc=%d", frpath, topath, rc);
					else
					    count++;
					Nopper();
				    }
				    if (S_ISLNK(stb.st_mode)) {
				        /*
				         * The linked LFN
				         */
				        if ((fdb_area = mbsedb_OpenFDB(Area, 30))) {
					    while (fread(&f_db, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
					        if (strcmp(f_db.LName, de->d_name) == 0) {
						    /*
						     * Got the symlink to the LFN
						     */
						    unlink(frpath);
						    snprintf(lnpath, PATH_MAX, "%s/%s", area.Path, f_db.Name);
						    if (symlink(lnpath, topath)) {
						        WriteError("$symlink(%s, %s)", lnpath, topath);
						    }
						    break;
						}
					    }
					    mbsedb_CloseFDB(fdb_area);
					}
				    }
				}
			    }
			    closedir(dp);
			    free(lnpath);
			    if ((rc = rmdir(tpath)))
			        WriteError("rmdir %s rc=%d", tpath, rc);
			    Force = TRUE;
			    FileForced = TRUE;
			    Syslog('+', "Moved %d files", count);
			}
		    }
		    break;
	    case 3: E_SEC(  8,16,    area.DLSec,     "8.4.3  DOWNLOAD SECURITY", FileScreen)
	    case 4: E_SEC(  9,16,    area.UPSec,     "8.4.4  UPLOAD SECURITY", FileScreen)
	    case 5: E_SEC( 10,16,    area.LTSec,     "8.4.5  LIST SECURITY", FileScreen)
	    case 6: Available = edit_bool(11, 16, area.Available, (char *)"Is this area ^available^");
		    temp = calloc(PATH_MAX, sizeof(char));
		    snprintf(temp, PATH_MAX, "%s/var/fdb/file%d.data", getenv("MBSE_ROOT"), Area);
		    if (area.Available && !Available) {
		        /*
		         * Attempt to disable this area, but check first.
		         */
		        if ((fdb_area = mbsedb_OpenFDB(Area, 30))) {
	    		    fseek(fdb_area->fp, 0, SEEK_END);
			    files = ((ftell(fdb_area->fp) - fdbhdr.hdrsize) / fdbhdr.recsize);
			    if (files) {
			        errmsg("There are stil %d files in this area", files);
			        Available = TRUE;
			    }
			    mbsedb_CloseFDB(fdb_area);
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
			        snprintf(temp, PATH_MAX, "-rf %s", area.Path);
			        execute_pth((char *)"rm", temp, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
			        rmdir(area.Path);
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
		        if ((fdb_area = mbsedb_OpenFDB(Area, 30))) {
			    mbsedb_CloseFDB(fdb_area);
			}
		    }
		    free(temp);
		    break;
	    case 7: E_BOOL(12,16,    area.New,       "Include this area in ^new files^ check")
	    case 8: E_BOOL(13,16,    area.Dupes,     "Check this area for ^duplicates^ during upload")
	    case 9: E_BOOL(14,16,    area.Free,      "Are all files ^free^ in this area")
	    case 10:E_BOOL(15,16,    area.DirectDL,  "Allow ^direct download^ from this area")
	    case 11:E_BOOL(16,16,    area.PwdUP,     "Allow ^password^ on uploads")
	    case 12:E_BOOL(17,16,    area.FileFind,  "Search this area for ^filefind^ requests")
	    case 13:E_BOOL(12,44,    area.AddAlpha,  "Add new files ^alphabetic^ or at the end")
	    case 14:E_BOOL(13,44,    area.FileReq,   "Allow ^file requests^ from this area")
	    case 15:strcpy(area.BbsGroup, PickFGroup((char *)"8.4.15"));
		    FileScreen();
		    break;
	    case 16:strcpy(area.NewGroup, PickNGroup((char *)"8.4.16"));
		    FileScreen();
		    break;
	    case 17:E_INT( 16,44,    area.Age,       "The ^minimum age^ to access this area")
	    case 18:E_STR( 17,44,20, area.Password,  "The ^password^ to access this area")
	    case 19:E_INT( 12,73,    area.DLdays,    "The not ^downloaded days^ to move/kill files")
	    case 20:E_INT( 13,73,    area.FDdays,    "The ^file age^ in days to move/kill files")
	    case 21:E_INT( 14,73,    area.MoveArea,  "The ^area to move^ files to, 0 is kill")
	    case 22:strcpy(area.Archiver, PickArchive((char *)"8.4", FALSE));
		    FileScreen();
		    break;
	    case 23:E_INT( 16,73,    area.Upload,    "The ^upload^ area, 0 if upload in this area")
	}
    }
}



void EditFilearea(void)
{
	int	records, i, o, x, y, count;
	char	pick[12];
	FILE	*fil, *tfil;
	char	temp[PATH_MAX], new[PATH_MAX];
	int	offset, from, too;

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
	o = 0;
	if (! check_free())
	    return;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "8.4 FILE AREA SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
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
						snprintf(temp, 81, "%3d.  %-32s", o + i, area.Name);
						temp[37] = 0;
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_filearea(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			open_bbs();
			CloseFilearea(FileForced);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendFilearea() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if (strncmp(pick, "M", 1) == 0) {
		    from = too = 0;
		    mbse_mvprintw(LINES -3, 5, "From");
		    from = edit_int(LINES -3, 10, from, (char *)"Wich ^area^ you want to move");
		    mbse_mvprintw(LINES -3,15, "To");
		    too  = edit_int(LINES -3, 18, too,  (char *)"Too which ^area^ to move");

		    snprintf(temp, PATH_MAX, "%s/etc/fareas.temp", getenv("MBSE_ROOT"));
		    if ((fil = fopen(temp, "r+")) != NULL) {
			fread(&areahdr, sizeof(areahdr), 1, fil);
			offset = areahdr.hdrsize + ((from - 1) * areahdr.recsize);
			if ((fseek(fil, offset, 0) != 0) || (fread(&area, areahdr.recsize, 1, fil) != 1) || 
			    (area.Available == FALSE)) {
			    errmsg((char *)"The originating area is invalid");
			} else {
			    offset = areahdr.hdrsize + ((too - 1) * areahdr.recsize);
			    if ((fseek(fil, offset, 0) != 0) || (fread(&area, areahdr.recsize, 1, fil) != 1) ||
				area.Available) {
				errmsg((char *)"The destination area is invalid");
			    } else {
				/*
				 * Move the area now
				 */
				working(5, 0, 0);
				offset = areahdr.hdrsize + ((from - 1) * areahdr.recsize);
				fseek(fil, offset, 0);
				fread(&area, areahdr.recsize, 1, fil);
				offset = areahdr.hdrsize + ((too - 1) * areahdr.recsize);
				fseek(fil, offset, 0);
				fwrite(&area, areahdr.recsize, 1, fil);
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
				FileUpdated = 1;
				offset = areahdr.hdrsize + ((from - 1) * areahdr.recsize);
				fseek(fil, offset, 0);
				fwrite(&area, areahdr.recsize, 1, fil);
				snprintf(temp, PATH_MAX, "%s/var/fdb/file%d.data", getenv("MBSE_ROOT"), from);
				snprintf(new,  PATH_MAX, "%s/var/fdb/file%d.data", getenv("MBSE_ROOT"), too);
				rename(temp, new);

				/*
				 * Force database update, don't let the user decide or he will
				 * loose all files from the moved areas.
				 */
				FileForced = TRUE;

				/*
				 * Update all other areas in case we just moved the destination
				 * for MoveArea or Upload area.
				 */
				fseek(fil, areahdr.hdrsize, SEEK_SET);
				count = 0;
				while (fread(&area, areahdr.recsize, 1, fil) == 1) {
				    if (((area.Upload == from) || (area.MoveArea == from)) && area.Available) {
					if (area.Upload == from)
					    area.Upload = too;
					if (area.MoveArea == from)
					    area.MoveArea = too;
					count++;
					fseek(fil, - areahdr.recsize, SEEK_CUR);
					fwrite(&area, areahdr.recsize, 1, fil);
				    }
				}
				Syslog('+', "Updated %d fileareas", count);
				
				/*
				 * Update references in tic areas to this filearea.
				 */
				snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
				if ((tfil = fopen(temp, "r+")) == NULL) {
				    WriteError("Can't update %s", temp);
				} else {
				    count = 0;
				    fread(&tichdr, sizeof(tichdr), 1, tfil);
				    while (fread(&tic, tichdr.recsize, 1, tfil) == 1) {
					if (tic.Active && (tic.FileArea == from)) {
					    tic.FileArea = too;
					    fseek(tfil, - tichdr.recsize, SEEK_CUR);
					    fwrite(&tic, tichdr.recsize, 1, tfil);
					    count++;
					}
					fseek(tfil, tichdr.syssize, SEEK_CUR);
				    }
				    fclose(tfil);
				    Syslog('+', "Updated %d ticareas", count);
				}
				Syslog('+', "Moved filearea %d to %d", from, too);
			    }
			}
			fclose(fil);
		    }
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



int PickFilearea(char *shdr)
{
	int	records, i, o = 0, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

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


	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.  FILE AREA SELECT", shdr);
		mbse_mvprintw(5,3,temp);
		set_color(CYAN, BLACK);
		if (records) {
			snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
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
						snprintf(temp, 81, "%3d.  %-31s", o + i, area.Name);
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
			return 0;

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o += 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o -= 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
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
    char    temp[PATH_MAX];
    FILE    *ti, *wp, *ip, *no;
    int	    i = 0, j = 0, tics;

    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    fread(&areahdr, sizeof(areahdr), 1, no);
    page = newpage(fp, page);	
    addtoc(fp, toc, 8, 4, page, (char *)"BBS File areas");

    ip = open_webdoc((char *)"fileareas.html", (char *)"File Areas", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Area</TH><TH align='left'>Comment</TH></TR>\n");

    while ((fread(&area, areahdr.recsize, 1, no)) == 1) {

	i++;
	if (area.Available) {

	    if (j == 1) {
		page = newpage(fp, page);
		j = 0;
	    } else {
		j++;
	    }

	    snprintf(temp, 81, "filearea_%d.html", i);

	    fprintf(ip, " <TR><TD><A HREF=\"%s\">%d</A></TD><TD>%s</TD></TR>\n", temp, i, area.Name);
	    if ((wp = open_webdoc(temp, (char *)"File area", area.Name))) {
		fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"fileareas.html\">Back</A>\n");
		fprintf(wp, "<P>\n");
		fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
		fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
		fprintf(wp, "<TBODY>\n");
		add_webdigit(wp, (char *)"Area number", i);
		add_webtable(wp, (char *)"Area name", area.Name);
		add_webtable(wp, (char *)"Files path", area.Path);
		web_secflags(wp, (char *)"Download security", area.DLSec);
		web_secflags(wp, (char *)"Upload security", area.UPSec);
		web_secflags(wp, (char *)"List security", area.LTSec);
		add_webtable(wp, (char *)"Newfiles scan", getboolean(area.New));
		add_webtable(wp, (char *)"Check upload dupes", getboolean(area.Dupes));
		add_webtable(wp, (char *)"Files are free", getboolean(area.Free));
		add_webtable(wp, (char *)"Allow direct download", getboolean(area.DirectDL));
		add_webtable(wp, (char *)"Allow password uploads", getboolean(area.PwdUP));
		add_webtable(wp, (char *)"Filefind on", getboolean(area.FileFind));
		add_webtable(wp, (char *)"Add files sorted", getboolean(area.AddAlpha));
		add_webtable(wp, (char *)"Allow filerequest", getboolean(area.FileReq));
		fprintf(wp, "<TR><TH align='left'>BBS (tic) file group</TH><TD><A HREF=\"filegroup_%s.html\">%s</A></TD></TH>\n",
			area.BbsGroup, area.BbsGroup);
		fprintf(wp, "<TR><TH align='left'>Newfiles announce group</TH><TD><A HREF=\"newgroup.html\">%s</A></TD></TH>\n",
			area.NewGroup);
		add_webdigit(wp, (char *)"Minimum age for access", area.Age);
		add_webtable(wp, (char *)"Area password", area.Password);
		add_webdigit(wp, (char *)"Kill Download days", area.DLdays);
		add_webdigit(wp, (char *)"Kill FileDate days", area.FDdays);
		add_webdigit(wp, (char *)"Move to area", area.MoveArea);
		add_webtable(wp, (char *)"Archiver", area.Archiver);
		add_webdigit(wp, (char *)"Upload area", area.Upload);
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
		fprintf(wp, "<HR>\n");
		fprintf(wp, "<H3>TIC Areas Reference</H3>\n");
		tics = 0;
		snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
		if ((ti = fopen(temp, "r"))) {
		    fread(&tichdr, sizeof(tichdr), 1, ti);
		    fseek(ti, 0, SEEK_SET);
		    fread(&tichdr, tichdr.hdrsize, 1, ti);
		    while ((fread(&tic, tichdr.recsize, 1, ti)) == 1) {
			if (tic.FileArea == i) {
			    if (tics == 0) {
				fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
				fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
				fprintf(wp, "<TBODY>\n");
			    }
			    fprintf(wp, "<TR><TD><A HREF=\"ticarea_%s.html\">%s</A></TD><TD>%s</TD></TR>\n",
					    tic.Name, tic.Name, tic.Comment);
			    tics++;
			}
			fseek(ti, tichdr.syssize, SEEK_CUR);
		    }
		    fclose(ti);
		}
		if (tics == 0)
		    fprintf(wp, "No TIC Areas References\n");
		else {
		    fprintf(wp, "</TBODY>\n");
		    fprintf(wp, "</TABLE>\n");
		}
		close_webdoc(wp);
	    }

	    fprintf(fp, "\n\n");
	    fprintf(fp, "    Area number       %d\n", i);
	    fprintf(fp, "    Area name         %s\n", area.Name);
	    fprintf(fp, "    Files path        %s\n", area.Path);
	    fprintf(fp, "    Download sec.     %s\n", get_secstr(area.DLSec));
	    fprintf(fp, "    Upload security   %s\n", get_secstr(area.UPSec));
	    fprintf(fp, "    List seccurity    %s\n", get_secstr(area.LTSec));
	    fprintf(fp, "    Newfiles scan     %s\n", getboolean(area.New));
	    fprintf(fp, "    Check upl. dupes  %s\n", getboolean(area.Dupes));
	    fprintf(fp, "    Files are free    %s\n", getboolean(area.Free));
	    fprintf(fp, "    Allow direct DL   %s\n", getboolean(area.DirectDL));
	    fprintf(fp, "    Allow pwd upl.    %s\n", getboolean(area.PwdUP));
	    fprintf(fp, "    Filefind on       %s\n", getboolean(area.FileFind));
	    fprintf(fp, "    Add files sorted  %s\n", getboolean(area.AddAlpha));
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

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
    
    fclose(no);
    return page;
}


