/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: All the file functions. 
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "filesub.h"
#include "file.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "exitinfo.h"
#include "whoson.h"
#include "change.h"
#include "dispfile.h"



extern	long	arecno;		/* File area number in xxxScan() functions   */
int	Strlen = 0;
int	FileRecno = 0;



int CheckFile(char *, int);
int CheckFile(char *File, int iArea)
{
    FILE    *pFileB;
    char    *sFileArea;

    sFileArea = calloc(PATH_MAX, sizeof(char));
    sprintf(sFileArea,"%s/fdb/file%d.data", getenv("MBSE_ROOT"), iArea); 

    if ((pFileB = fopen(sFileArea,"r+")) == NULL) {
	mkdir(sFileArea, 775);
        return FALSE;
    }
    free(sFileArea);
    fread(&fdbhdr, sizeof(fdbhdr), 1, pFileB);

    /*
     * Check long and short filenames, case insensitive
     */
    while (fread(&fdb, fdbhdr.recsize, 1, pFileB) == 1) {
        if (((strcasecmp(fdb.Name, File)) == 0) || ((strcasecmp(fdb.LName, File)) == 0)) {
            fclose(pFileB);
	    return TRUE;
        }
    }

    fclose(pFileB);
    return FALSE;
}



/*
 * Show filelist from current area, called from the menu.
 */
void File_List()
{
    FILE	*pFile;
    int		FileCount = 0;
    unsigned	FileBytes = 0;
    _Tag	T;

    iLineCount = 0;
    WhosDoingWhat(FILELIST, NULL);

    Syslog('+', "Listing File Area # %d", iAreaNumber);

    if (Access(exitinfo.Security, area.LTSec) == FALSE) {
	colour(14, 0);
	/* You don't have enough security to list this area */
	printf("\n%s\n", (char *) Language(236));
	Pause();
	return;
    }

    InitTag();

    if ((pFile = OpenFileBase(iAreaNumber, FALSE)) == NULL)
	return;

    clear();
    Header();
    if (iLC(2) == 1) {
	fclose(pFile);
	return;
    }

    while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {

	memset(&T, 0, sizeof(T));
	T.Area   = iAreaNumber;
	T.Active = FALSE;
	T.Size   = fdb.Size;
	strncpy(T.SFile, fdb.Name, 12);
	strncpy(T.LFile, fdb.LName, 80);
	SetTag(T);

	if (ShowOneFile() == 1) {
	    fclose(pFile);
	    return;
	}

	if (fdb.Deleted)
	    /* D E L E T E D */ /* Uploaded by: */
	    printf(" -- %-12s     %s     [%4ld] %s%s\n", fdb.Name, (char *) Language(239), 
				fdb.TimesDL, (char *) Language(238), fdb.Uploader);

	FileCount++;			/* Increase File Counter by 1 */
	FileBytes += fdb.Size;		/* Increase File Byte Count   */
    }

    Mark();
	
    colour(11,0);
    /* Total Files: */
    printf("\n%s%d / %d bytes\n\n", (char *) Language(242), FileCount, FileBytes);

    iLineCount = 0;
    fclose(pFile);
    Pause();
}



/*
 * Download files already tagged, called from the menu.
 */
void Download(void)
{
    DIR		    *dirp;
    struct dirent   *dp;
    FILE	    *tf, *fp, *fd;
    int		    i, err, Count = 0;
    int		    OldArea;
    char	    *symTo, *symFrom;
    char	    *temp;
    long	    Size = 0, CostSize = 0;
    time_t	    ElapstimeStart, ElapstimeFin, iTime;
    long	    iTransfer = 0;

    Enter(2);
    OldArea = iAreaNumber;
    WhosDoingWhat(DOWNLOAD, NULL);
    execute_pth((char *)"rm", (char *)"-f ./tag/*", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");

    if ((tf = fopen("taglist", "r+")) == NULL) {
	Syslog('+', "Download command but no files marked");
	/* No files marked for download. */
	pout(12, 0, (char *) Language(258));
	Enter(2);
	Pause();
	return;
    }

    symTo   = calloc(PATH_MAX, sizeof(char));
    symFrom = calloc(PATH_MAX, sizeof(char));
    colour(13, 0);
    /* Checking your marked downloads, please wait... */
    printf("%s\n\n", (char *) Language(255));

    ReadExitinfo();
    while (fread(&Tag, sizeof(Tag), 1, tf) == 1) {
	if (Tag.Active) {

	    SetFileArea(Tag.Area);

	    /*
	     * Check password for selected file  FIXME: Where???
	     */
	    memset(&fdb, 0, sizeof(fdb));
	    if ((fp = OpenFileBase(Tag.Area, FALSE)) != NULL) {
		while (fread(&fdb, fdbhdr.recsize, 1, fp) == 1) {
		    if (strcmp(fdb.LName, Tag.LFile) == 0)
			break;
		}
		fclose(fp);
	    } 

	    if (strcmp(fdb.LName, Tag.LFile) == 0) {
		Syslog('b', "Found file %s in area %d", fdb.LName, Tag.Area);
		if (fdb.Deleted) {
		    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(248));
		    /* Sorry that file is unavailable for download */
		    printf("%s (%s)\n", (char *) Language(248), fdb.LName);
		    Tag.Active = FALSE;
		    Syslog('+', "File %s in area %d unavailable for download, deleted", fdb.LName, Tag.Area);
		}
	    }

	    if (Tag.Active) {
		/*
		 * Create/Append file description list while we're
		 * busy checking. If the users doesn't want it we
		 * can unlink it aftwerwards. We also insert CR
		 * characters to please the poor DOS (M$oft) users.
		 */
		sprintf(symTo, "./tag/filedesc.%ld", exitinfo.Downloads % 256);
		if ((fd = fopen(symTo, "a")) != NULL) {
		    fprintf(fd, "%s (%s)\r\n", fdb.LName, fdb.Name);
		    for (i = 0; i < 25; i++) {
			if (strlen(fdb.Desc[i]) > 1)
			    fprintf(fd, "  %s\r\n", fdb.Desc[i]);
		    }
		    fprintf(fd, "\r\n");
		    fclose(fd);
		    Syslog('b', "Added info to %s", symTo);
		} else {
		    WriteError("Can't add info to %s", symTo);
		}

		/*
		 * Make a symlink to the users download dir.
		 * First unlink, in case there was an old one.
		 * The shortname is linked to the original longname.
		 */
		chdir("./tag");
		unlink(Tag.SFile);
		sprintf(symFrom, "%s", Tag.SFile);
		sprintf(symTo, "%s/%s", sAreaPath, Tag.LFile);
		if (symlink(symTo, symFrom)) {
		    WriteError("$Can't create symlink %s %s %d", symTo, symFrom, errno);
		    Tag.Active = FALSE;
		} else {
		    Syslog('b', "Created symlink %s -> %s", symFrom, symTo);
		}
		if ((access(symFrom, R_OK)) != 0) {
		    /*
		     * Extra check, is symlink really there?
		     */
		    WriteError("Symlink %s check failed, unmarking download", symFrom);
		    Tag.Active = FALSE;
		}
		Home();
	    } 

	    if (!Tag.Active) {
		/*
		 * Update the download active flag in the
		 * taglist
		 */
		fseek(tf, - sizeof(Tag), SEEK_CUR);
		fwrite(&Tag, sizeof(Tag), 1, tf);
		Syslog('b', "Download file %s marked inactive in taglist", Tag.LFile);
	    } else {
		/*
		 * Count file and sizes.
		 */
		Count++;
		Size += fdb.Size;
		if (!area.Free)
		    CostSize += fdb.Size;
	    }
	}
    }
    fclose(tf);

    /*
     * If anything left to download...
     */
    if (!Count) {
	SetFileArea(OldArea);
	unlink("taglist");
	/* No files marked for download */
	pout(12, 0, (char *) Language(258));
	Enter(2);
	Pause();
	free(symTo);
	free(symFrom);
	Syslog('+', "No files left to download");
	return;
    }

    colour(14, 0);
    /* You have */ /* files( */ /* bytes) marked for download */
    printf("%s %d %s%ld %s\n\n", (char *) Language(249), Count, (char *) Language(280), Size, (char *) Language(281));

    /*
     * If user has no default protocol, make sure he has one.
     */
    if (!ForceProtocol()) {
	SetFileArea(OldArea);
	free(symTo);
	free(symFrom);
	return;
    }

    if (!CheckBytesAvailable(CostSize)) {
	SetFileArea(OldArea);
	free(symTo);
	free(symFrom);
	return;
    }

    Pause();

    clear();
    /* File(s)     : */
    pout(14, 0, (char *) Language(349)); printf("%d\n", Count);
    /* Size        : */
    pout( 3, 0, (char *) Language(350)); printf("%lu\n", Size);
    /* Protocol    : */
    pout( 3, 0, (char *) Language(351)); printf("%s\n", sProtName);

    Syslog('+', "Download tagged files start, protocol: %s", sProtName);

    printf("%s\n\n", sProtAdvice);
    fflush(stdout);
    fflush(stdin);

    /*
     * Wait a while before download
     */
    sleep(2);
    ElapstimeStart = time(NULL);
	
    /*
     * Transfer the files. Set the Client/Server time at the maximum
     * time the user has plus 10 minutes. The overall timer 10 seconds
     * less. Not a nice but working solution.
     */
    alarm_set(((exitinfo.iTimeLeft + 10) * 60) - 10);
    Altime((exitinfo.iTimeLeft + 10) * 60);

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
    if ((dirp = opendir(temp)) == NULL) {
	WriteError("$Download: Can't open dir: %s", temp);
	free(temp);
    } else {
	chdir(temp);
	free(temp);
	temp = NULL;
	while ((dp = readdir(dirp)) != NULL ) {
	    if (*(dp->d_name) != '.') {
		if (temp != NULL) {
		    temp = xstrcat(temp, (char *)" ");
		    temp = xstrcat(temp, dp->d_name);
		} else {
		    temp = xstrcpy(dp->d_name);
		}
	    }
	}
	if (temp != NULL) {
	    if ((err = execute_str(sProtDn, temp, NULL, NULL, NULL, NULL))) {
		perror("");
		colour(CFG.HiliteF, CFG.HiliteB);
		WriteError("Download error %d, prot: %s", err, sProtDn);
	    }
	    free(temp);
	} else {
	    WriteError("No filebatch created");
	}
	closedir(dirp);
    }
    Altime(0);
    alarm_off();
    alarm_on();
    fflush(stdout);
    fflush(stdin);
    Home();
    ElapstimeFin = time(NULL);

    /*
     * Get time from Before Download and After Download to get
     * download time, if the time is zero, it will be one.
     */
    iTime = ElapstimeFin - ElapstimeStart;
    if (!iTime)
	iTime = 1;

    /*
     * Checking the successfull sent files, they are missing from
     * the ./tag directory. Failed files are still there.
     */
    colour(11, 0);
    /* Updating download counters, please wait ... */
    printf("\r%s\n\n", (char *) Language(352));
    fflush(stdout);
    Count = Size = 0; 

    if ((tf = fopen("taglist", "r+")) != NULL) {
	while (fread(&Tag, sizeof(Tag), 1, tf) == 1) {
	    if (Tag.Active) {
		sprintf(symTo, "./tag/%s", Tag.SFile);
		/*
		 * If symlink is gone the file is sent.
		 */
		if ((access(symTo, R_OK)) != 0) {
		    Syslog('+', "File %s from area %d sent ok", Tag.LFile, Tag.Area);
		    Tag.Active = FALSE;
		    fseek(tf, - sizeof(Tag), SEEK_CUR);
		    fwrite(&Tag, sizeof(Tag), 1, tf);

		    /*
		     * Update the download counter and the last download date.
		     */
		    SetFileArea(Tag.Area);
		    if ((fp = OpenFileBase(Tag.Area, TRUE)) != NULL) {
			while (fread(&fdb, fdbhdr.recsize, 1, fp) == 1) {
			    if (strcmp(fdb.LName, Tag.LFile) == 0)
				break;
			}
			Size += fdb.Size;
			fdb.TimesDL++;
			fdb.LastDL = time(NULL);
			fseek(fp, - fdbhdr.recsize, SEEK_CUR);
			fwrite(&fdb, fdbhdr.recsize, 1, fp);
			fclose(fp);
			Count++;
		    }
		} else {
		    Syslog('+', "Failed to sent %s from area %d", Tag.LFile, Tag.Area);
		}
	    }
	}
	fclose(tf);
    }

    /*
     * Work out transfer rate in seconds by dividing the
     * Size of the File by the amount of time it took to download 
     * the file.
     */
    iTransfer = Size / iTime;
    Syslog('+', "Download time %ld seconds (%lu cps), %d files", iTime, iTransfer, Count);


    /*
     * Update the users record.
     */
    ReadExitinfo();

    exitinfo.Downloads += Count;          /* Increase download counter      */
    exitinfo.DownloadK += (Size / 1024);  /* Increase amount download today */

    /*
     * Minus the amount downloaded today from downloadktoday
     * if less than zero, it won't let the user download anymore.
     */
    if (LIMIT.DownK || LIMIT.DownF) {
	exitinfo.DownloadKToday -= (Size / 1024);
	exitinfo.iTransferTime = iTransfer;
    }

    WriteExitinfo();
    Pause();
    SetFileArea(OldArea);
    free(symTo);
    free(symFrom);
}



/*
 * Show Raw directory
 */
void File_RawDir(char *OpData)
{
	DIR	*dirp;
	char	*FileName, *temp;
	int	iFileCount = 0;
	int	LineCount = 2;
	int	iBytes = 0;
	struct	dirent *dp;
	struct	stat statfile;

	FileName = calloc(PATH_MAX, sizeof(char));
	temp     = calloc(PATH_MAX, sizeof(char));

	if((strcmp(OpData, "/F")) == 0)
		strcpy(temp, sAreaPath);
	else
		strcpy(temp, OpData);

	if ((dirp = opendir(temp)) == NULL) {
		clear();
		WriteError("$RawDir: Can't open dir: %s", temp);
		printf("\nCan't open directory for raw listing!\n\n");
		Pause();
	} else {
		clear();
		colour(CFG.HiliteF, CFG.HiliteB);
		/* Filename            Size        Date */
		printf("%s\n", (char *) Language(261));
		fLine(42);

		while ((dp = readdir( dirp )) != NULL ) {
			 sprintf(FileName, "%s/%s", temp, dp->d_name);

		 	 if (*(dp->d_name) != '.') {
				iFileCount++;
				if (stat(FileName, &statfile) != 0)
					printf("Can't stat file %s\n", FileName);
				iBytes += statfile.st_size;

				colour(14,0);
				printf("%-12s " , dp->d_name);

				colour(13,0);
				printf("%-12ld", (long)(statfile.st_size));

				colour(10,0);
				printf("%-10s\n", StrDateDMY(statfile.st_mtime));

				LineCount++;
				if (LineCount == exitinfo.iScreenLen) {
					Pause();
					LineCount = 0;
				}
			}
		}

		colour(CFG.HiliteF, CFG.HiliteB);
		fLine(42);
		/* Total Files: */ /* Bytes */
		printf("%s %d, %d %s\n\n", (char *) Language(242), iFileCount, iBytes, (char *) Language(354));

		Pause();
		closedir(dirp);
	}
	free(temp);
	free(FileName);
}



/*
 * Search for keyword, called from menu.
 */
int KeywordScan()
{
    FILE	    *pAreas, *pFile;
    int		    i, z, y, Found, Count = 0;
    char	    *Name, *tmpname, *BigDesc, temp[81];
    _Tag	    T;
    unsigned long   OldArea;


    Name     = calloc(81, sizeof(char));
    tmpname  = calloc(81, sizeof(char));
    BigDesc  = calloc(1230, sizeof(char));
    OldArea  = iAreaNumber;

    iLineCount = 2; /* Reset Line Counter to Zero */
    arecno     = 1; /* Reset Area Number to One */

    Enter(2);
    /* Enter keyword to use for Search: */
    pout(11, 0, (char *) Language(267));

    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(Name, 80);

    if ((strcmp(Name, "")) == 0)
	return 0;

    strcpy(tmpname, tl(Name));
    strcpy(Name, "");
    y = strlen(tmpname);
    for (z = 0; z <  y; z++) {
	if (tmpname[z] != '*') {
	    sprintf(temp, "%c", tmpname[z]);
	    strcat(Name, temp);
	}
    }
    Syslog('+', "KeywordScan(): \"%s\"", Name);

    clear();
    /* File search by keyword */
    pout(15, 0, (char *) Language(268));
    Enter(1);
    InitTag();

    if ((pAreas = OpenFareas(FALSE)) == NULL)
	return 0;

    while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {

	if ((Access(exitinfo.Security, area.LTSec)) && (area.Available) && (strlen(area.Password) == 0)) {

	    if ((pFile = OpenFileBase(arecno, FALSE)) != NULL) {

		Nopper();
		Found = FALSE;
		Sheader();

		while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {

		    for (i = 0; i < 25; i++)
			sprintf(BigDesc, "%s%s", BigDesc, *(fdb.Desc + i));

		    if ((strstr(fdb.Name,Name) != NULL) || (strstr(tl(BigDesc), Name) != NULL)) {

			if (!Found) {
			    Enter(2);
			    if (iLC(2) == 1) {
				free(BigDesc);
				free(Name);
				free(tmpname);
				SetFileArea(OldArea);
				return 1;
			    }
			    Found = TRUE;
			}

			memset(&T, 0, sizeof(T));
			T.Area   = arecno;
			T.Active = FALSE;
			T.Size   = fdb.Size;
			strncpy(T.SFile, fdb.Name, 12);
			strncpy(T.LFile, fdb.LName, 80);
			SetTag(T);
			Count++;
			if (ShowOneFile() == 1) {
			    free(BigDesc);
			    free(Name);
			    free(tmpname);
			    SetFileArea(OldArea);
			    return 1;
			}
		    }
		    strcpy(BigDesc, "");  /* Clear BigDesc */

		} /* while */

		fclose(pFile);
		if (Found) {
		    Enter(2);
		    if (iLC(2) == 1) {
			free(BigDesc);
			free(Name);
			free(tmpname);
			SetFileArea(OldArea);
			return 1;
		    }
		}

	    } /* End check for LTSec */
	} /* if access */
	arecno++; /* Go to next file area */
    } /* End of Main */

    Syslog('+', "Found %d files", Count);
    free(BigDesc);
    free(Name);
    free(tmpname);
    fclose(pAreas);
    printf("\n");
    if (Count)
	Mark();
    else
	Pause();
    SetFileArea(OldArea);
    return 1;
}



/*
 * Search for a file, called from the menu.
 */
int FilenameScan()
{
    FILE	    *pAreas, *pFile;
    int		    Found, Count = 0;
    char	    *p, *q, mask[256];
    char	    *Name;
    _Tag	    T;
    unsigned long   OldArea;

    Name     = calloc(81, sizeof(char));
    OldArea  = iAreaNumber;

    iLineCount = 2; /* Reset Line Counter to Zero */
    arecno     = 1; /* Reset Area Number to One  */

    Enter(2);
    /* Accepts wildcards such as : *.zip, *.gz, *.* */
    pout(15, 0, (char *) Language(269));

    Enter(2);
    /* Enter filename to search for : */
    pout(11, 0, (char *) Language(271));

    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(Name, 80);

    if ((strcmp(Name, "")) == 0) {
	free(Name);
	return 0;
    }

    /*
     * Make a regexp string for the users search mask.
     */
    p = tl(Name);
    q = mask;
    *q++ = '^';
    while ((*p) && (q < (mask + sizeof(mask) - 4))) {
	switch (*p) {
	    case '\\':  *q++ = '\\'; *q++ = '\\'; break;
	    case '?':   *q++ = ','; break;
	    case '.':   *q++ = '\\'; *q++ = '.'; break;
	    case '+':   *q++ = '\\'; *q++ = '+'; break;
	    case '*':   *q++ = '.';  *q++ = '*'; break;
	    default:    *q++ = toupper(*p);   break;
	}
	p++;
    }
    *q++ = '$';
    *q++ = '\0';
    Syslog('+', "FilenameScan(): \"%s\" -> \"%s\"", Name, mask);
    free(Name);
    re_comp(mask);

    clear();
    /* File Search by Filename */
    pout(15, 0, (char *) Language(272));
    Enter(1);
    InitTag();

    if ((pAreas = OpenFareas(FALSE)) == NULL)
	return 0;

    while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {
	if ((Access(exitinfo.Security, area.LTSec)) && (area.Available) && (strlen(area.Password) == 0)) {

	    if ((pFile = OpenFileBase(arecno, FALSE)) != NULL) {

		Found = FALSE;
		Sheader();
		Nopper();

		while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {

		    if (re_exec(fdb.Name) || re_exec(fdb.LName)) {
			if (!Found) {
			    Enter(2);
			    if (iLC(2) == 1) {
				SetFileArea(OldArea);
				return 1;
			    }
			    Found = TRUE;
			}

			memset(&T, 0, sizeof(T));
			T.Area   = arecno;
			T.Active = FALSE;
			T.Size   = fdb.Size;
			strncpy(T.SFile, fdb.Name, 12);
			strncpy(T.LFile, fdb.LName, 81);
			SetTag(T);
			Count++;
			if (ShowOneFile() == 1) {
			    SetFileArea(OldArea);
			    return 1;
			}
		    }

		} /* End of while */

		fclose(pFile);
		if (Found) {
		    Enter(2);
		    if (iLC(2) == 1) {
			SetFileArea(OldArea);
			return 1;
		    }
		}

	    } /* End Check for LTSec */
	} /* if access */
	arecno++; /* Go to next file area */

    } /* End of Main */

    Syslog('+', "Found %d files", Count);
    fclose(pAreas);
    printf("\n");
    if (Count)
	Mark();
    else
	Pause();
    SetFileArea(OldArea);
    return 1;
}



/*
 * Scan for new files, called from menu.
 */
int NewfileScan(int AskStart) 
{ 
    FILE    *pAreas, *pFile;
    long    ifDate, itDate;
    char    *temp, *Date;
    int	    Found, Count = 0;
    _Tag    T;

    Date     = calloc(81, sizeof(char));
    temp     = calloc(81, sizeof(char));

    iLineCount = 2;
    arecno     = 1; /* Reset Area Number to One  */

    if (AskStart) {
	Enter(2);
	/* Search for new since your last call [Y/n]: */
	pout(11, 0, (char *) Language(273));
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);

	if (toupper(Getone()) == Keystroke(273, 1)) {
	    Enter(1);
	    /* Enter new date to search for [DD-MM-YYYY]: */
	    pout(2, 0, (char *) Language(274));
	    colour(CFG.InputColourF, CFG.InputColourB);
	    fflush(stdout);
	    GetDate(temp, 10);
	} else
	    strcpy(temp, LastLoginDate);
    } else
	strcpy(temp, LastLoginDate);

    Syslog('+', "NewfileScan() since %s", temp);
    clear();
    /* File Search by Date */
    pout(15, 0, (char *) Language(275));
    Enter(2);

    Date[0] = temp[6];	/* Swap the date around      */
    Date[1] = temp[7];	/* Instead of   DD-MM-YYYY   */
    Date[2] = temp[8];	/* Let it equal YYYYMMDD     */
    Date[3] = temp[9];	/* Swap the date around      */
    Date[4] = temp[3];	/* Swap the date around      */
    Date[5] = temp[4];	/* because when you convert  */
    Date[6] = temp[0];	/* a string to an int you    */
    Date[7] = temp[1];	/* loose the front Zero      */
    Date[8] = '\0';   	/* making the number smaller */
    itDate = atol(Date);

    InitTag();

    if ((pAreas = OpenFareas(FALSE)) == NULL)
	return 0;

    while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {

	if ((Access(exitinfo.Security, area.LTSec)) && (area.Available) && (strlen(area.Password) == 0) && (area.New)) {

	    if ((pFile = OpenFileBase(arecno, FALSE)) != NULL ) {

		Sheader();
		Found = FALSE;
		Nopper();

		while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {
		    strcpy(temp, StrDateDMY(fdb.UploadDate));	/* Realloc Space for Date */
		    Date[0] = temp[6];	    /* Swap the date around      */
		    Date[1] = temp[7];	    /* Instead of   DD-MM-YYYY   */
		    Date[2] = temp[8];	    /* Let it equal YYYYMMDD     */
		    Date[3] = temp[9];	    /* Swap the date around      */
		    Date[4] = temp[3];	    /* Swap the date around      */
		    Date[5] = temp[4];	    /* because when you convert  */
		    Date[6] = temp[0];	    /* a string to an int you    */
		    Date[7] = temp[1];	    /* loose the front Zero      */
		    Date[8] = '\0';	    /* making the number smaller */
					    /* and invalid to this cause */
		    ifDate = atol(Date);

		    if (ifDate >= itDate) {
			if (!Found) {
			    printf("\n\n");
			    if (iLC(2) == 1) {
				free(Date);
				free(temp);
				fclose(pFile);
				fclose(pAreas);
				return 1;
			    }
			    Found = TRUE;
			}

			memset(&T, 0, sizeof(T));
			T.Area   = arecno;
			T.Active = FALSE;
			T.Size   = fdb.Size;
			strncpy(T.SFile, fdb.Name, 12);
			strncpy(T.LFile, fdb.LName, 80);
			SetTag(T);

			Count++;
			if (ShowOneFile() == 1) {
			    free(Date);
			    free(temp);
			    fclose(pFile);
			    fclose(pAreas);
			    return 1;
			}

		    } /* End of if */
		} /* End of while */

		fclose(pFile);

		/*
		 * Add 2 blank lines after found files.
		 */
		if (Found) {
		    printf("\n\n");
		    if (iLC(2) == 1) {
			free(Date);
			free(temp);
			fclose(pAreas);
			return 1;
		    }
		}

	    } /* End of open filebase */

	} /* End of check new files scan */
	arecno++; /* Go to next file area */

    } /* End of Main */

    if (Count)
	Syslog('+', "Found %d new files", Count);
    fclose(pAreas);
    printf("\n");
    if (Count)
	Mark();
    else
	Pause();

    free(temp);
    free(Date);
    return 1;
}



/*
 * Upload a file.
 */
int Upload()
{
	char		File[81], temp[81];
	int		Area, x = 0;
	int		i, err;
	unsigned long	OldArea;
	time_t		ElapstimeStart, ElapstimeFin, iTime;
	DIR		*dirp;
	struct dirent	*dp;
	struct stat	statfile;
	char		*arc;


	WhosDoingWhat(UPLOAD, NULL); 

	/*
	 * Select default protocol if users hasn't any.
	 */
	if (!ForceProtocol())
		return 0;

	Enter(1);
	Area = OldArea = iAreaNumber;

	/*
	 * If there is a special upload area for the current area
	 * then select it.
	 */
	if (area.Upload)
		Area = area.Upload;
	SetFileArea(Area);

	/*
	 * Only ask for a filename for non-batching protocols,
	 * ie. the stone age Xmodem for example.
	 */
	if (!uProtBatch) {
		/* Please enter file to upload: */
		pout(14, 0, (char *) Language(276));

		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(File, 80);

		if((strcmp(File, "")) == 0)
			return 0;

		if (*(File) == '.' || *(File) == '*' || *(File) == ' ' || *(File) == '/') {
			colour(CFG.HiliteF, CFG.HiliteB);
			/* Illegal Filename! */
			printf("\n%s\n\n", (char *) Language(247));
			Pause();
			return 0;
		}

		Strlen = strlen(File);
		Strlen--;

		if (*(File + Strlen) == '.' || *(File + Strlen) == '/' || *(File + Strlen) == ' ') {
			colour(CFG.HiliteF, CFG.HiliteB);
			/* Illegal Filename! */
			printf("\n%s\n\n", (char *) Language(247));
			Pause();
			return 0;
		}

		if ((!strcmp(File, "files.bbs")) || (!strcmp(File, "00index")) || (strstr(File, (char *)".html"))) {
			colour(CFG.HiliteF, CFG.HiliteB);
			/* Illegal Filename! */
			printf("\n%s\n\n", (char *) Language(247));
			Syslog('!', "Attempted to upload %s", File);
			Pause();
			return 0;
		}

		for (i = 0; i < strlen(File); i++)
			printf("%d ", File[i]);

		/*
		 * Check for a space or ; in filename being uploaded
		 */
		if (((strchr(File, 32)) != NULL) || ((strchr(File, ';')) != NULL)) {
			colour(CFG.HiliteF, CFG.HiliteB);
			/* Illegal Filename! */
			printf("\n%s\n\n", (char *) Language(247));
			Pause();
			return 0;
		}

		/* MOET IN ALLE AREAS ZOEKEN */
		if (area.Dupes) {
			x = CheckFile(File, Area);
			if (x) {
				Enter(1);
				/* The file already exists on the system */
				pout(15, 3, (char *) Language(282));
				Enter(2);
				SetFileArea(OldArea);
				Pause();
				return 0;
			}
		}
		SetFileArea(OldArea);
	}

	SetFileArea(Area);
	Syslog('+', "Upload area is %d %s", Area, area.Name);

	/*
	 * Check upload access for the real upload directory.
	 */
	if (!Access(exitinfo.Security, area.UPSec)) {
		colour(CFG.HiliteF, CFG.HiliteB);
		/* You do not have enough access to upload to this area */
		printf("\n%s\n\n", (char *) Language(278));
		SetFileArea(OldArea);
		Pause();
		return 0;
	}

	clear();
	colour(CFG.HiliteF, CFG.HiliteB);
	/* Please start your upload now ...*/
	printf("\n\n%s, %s\n\n", sProtAdvice, (char *) Language(283));
	if (uProtBatch)
		Syslog('+', "Upload using %s", sProtName);
	else
		Syslog('+', "Upload \"%s\" using %s", File, sProtName);

	sprintf(temp, "%s/%s/upl", CFG.bbs_usersdir, exitinfo.Name);
	if (chdir(temp)) {
		WriteError("$Can't chdir to %s", temp);
		SetFileArea(OldArea);
		return 0;
	}

	fflush(stdout);
	fflush(stdin);
	sleep(2);
	ElapstimeStart = time(NULL);
	
	/*
	 * Get the file(s). Set the Client/Server time to 2 hours.
	 * This is not a nice solution, at least it works and prevents
	 * that the bbs will hang.
	 */
	Altime(7200);
	alarm_set(7190);
	if ((err = execute_str(sProtUp, (char *)"", NULL, NULL, NULL, NULL))) {
		/*
		 * Log any errors
		 */
		colour(CFG.HiliteF, CFG.HiliteB);
		WriteError("$Upload error %d, prot: %s", err, sProtUp);
	}
	Altime(0);
	alarm_off();
	alarm_on();
	printf("\n\n\n");
	fflush(stdout);
	fflush(stdin);
	ElapstimeFin = time(NULL);

	/*
	 * Get time from Before Upload and After Upload to get
	 * upload time, if the time is zero, it will be one.
	 */
	iTime = ElapstimeFin - ElapstimeStart;
	if (!iTime)
		iTime = 1;

	Syslog('b', "Transfer time %ld", iTime);

	if ((dirp = opendir(".")) == NULL) {
		WriteError("$Upload: can't open ./upl");
		Home();
		SetFileArea(OldArea);
		return 1;
	}

	pout(CFG.UnderlineColourF, CFG.UnderlineColourB, (char *)"\n\nChecking your upload(s)\n\n");

	while ((dp = readdir(dirp)) != NULL) {

		if (*(dp->d_name) != '.') {
			stat(dp->d_name, &statfile);
			Syslog('+', "Uploaded \"%s\", %ld bytes", dp->d_name, statfile.st_size);

			if ((arc = GetFileType(dp->d_name)) == NULL) {
				/*
				 * If the filetype is unknown, it is probably 
				 * a textfile or so. Import it direct.
				 */
				Syslog('b', "Unknown file type");
				if (!ScanDirect(dp->d_name))
				    ImportFile(dp->d_name, Area, FALSE, iTime, statfile.st_size);
			} else {
				/*
				 * We figured out the type of the uploaded file.
				 */
				Syslog('b', "File type is %s", arc);

				/*
				 * MS-DOS executables are handled direct.
				 */
				if ((strcmp("EXE", arc) == 0) || (strcmp("COM", arc) == 0)) {
					if (!ScanDirect(dp->d_name)) 
						ImportFile(dp->d_name, Area, FALSE, iTime, statfile.st_size);
				} else {
					switch(ScanArchive(dp->d_name, arc)) {

					case 0:
						ImportFile(dp->d_name, Area, TRUE, iTime, statfile.st_size);
						break;

					case 1:
						break;

					case 2:
						break;

					case 3:
						/*
						 * No valid unarchiver found, just import after scanning,
						 * may catch macro viri.
						 */
						if (!ScanDirect(dp->d_name))
						    ImportFile(dp->d_name, Area, FALSE, iTime, statfile.st_size);
						break;
					}
				}
			}
		}
	}
	closedir(dirp);

	Home();
	SetFileArea(OldArea);
	Pause();
	return 1;
}



/*
 * Function will download a specific file
 */
int DownloadDirect(char *Name, int Wait)
{
	int	err, rc;
	char	*symTo, *symFrom;
	long	Size;
	time_t	ElapstimeStart, ElapstimeFin, iTime;
	long	iTransfer = 0;

	if ((Size = file_size(Name)) == -1) {
		WriteError("No file %s", Name);
		colour(CFG.HiliteF, CFG.HiliteB);
		printf("File not found\n\n");
		Pause();
	}

	/*
	 * Make a symlink to the users tmp dir.
	 */
	symTo = calloc(PATH_MAX, sizeof(char));
	symFrom = calloc(PATH_MAX, sizeof(char));
	sprintf(symFrom, "%s/%s/tmp%s", CFG.bbs_usersdir, exitinfo.Name, strrchr(Name, '/'));
	sprintf(symTo, "%s", Name);

	if (symlink(symTo, symFrom)) {
		WriteError("$Can't create symlink %s %s", symTo, symFrom);
		free(symTo);
		free(symFrom);
		return FALSE;
	}

	/*
	 * If user has no default protocol, make sure he has one.
	 */
	if (!ForceProtocol()) {
		unlink(symFrom);
		free(symTo);
		free(symFrom);
		return FALSE;
	}

	WhosDoingWhat(DOWNLOAD, NULL);
	ReadExitinfo();

	clear();
	/* File(s)    : */
	pout(14, 0, (char *) Language(349)); printf("%s\n", symFrom);
	/* Size       : */
	pout( 3, 0, (char *) Language(350)); printf("%lu\n", Size);
	/* Protocol   : */
	pout( 3, 0, (char *) Language(351)); printf("%s\n", sProtName);

	Syslog('+', "Download direct start %s", Name);

	printf("%s\n\n", sProtAdvice);
	fflush(stdout);
	fflush(stdin);

	/*
	 * Wait a while before download
	 */
	sleep(2);
	ElapstimeStart = time(NULL);

	/*
	 * Transfer the file. Set the Client/Server time at the maximum
	 * time the user has plus 10 minutes. The overall timer 10 seconds
	 * less.
	 */
	alarm_set(((exitinfo.iTimeLeft + 10) * 60) - 10);
	Altime((exitinfo.iTimeLeft + 10) * 60);
	if ((err = execute_str(sProtDn, symFrom, NULL, NULL, NULL, NULL))) {
		/*
		 * Only log the error, we might have sent some files
		 * instead of nothing.
		 */
		perror("");
		colour(CFG.HiliteF, CFG.HiliteB);
		WriteError("Download error %d, prot: %s", err, sProtDn);
	}
	Altime(0);
	alarm_off();
	alarm_on();
	fflush(stdout);
	fflush(stdin);
	ElapstimeFin = time(NULL);

	/*
	 * Get time from Before Download and After Download to get
	 * download time, if the time is zero, it will be one.
	 */
	iTime = ElapstimeFin - ElapstimeStart;
	if (!iTime)
		iTime = 1;

	if ((access(symFrom, R_OK)) != 0) {

		/*
		 * Work out transfer rate in seconds by dividing the
		 * Size of the File by the amount of time it took to download 
		 * the file.
		 */
		iTransfer = Size / iTime;
		Syslog('+', "Download ok, time %ld seconds (%lu cps)", iTime, iTransfer);

		/*
		 * Update the users record. The file is free, so only statistics.
		 */
		ReadExitinfo();
		exitinfo.Downloads++;    /* Increase download counter */
		exitinfo.iTransferTime = iTransfer;
		WriteExitinfo();
		rc = TRUE;
	} else {
		Syslog('+', "Download failed to sent file");
		unlink(symFrom);
		rc = FALSE;
	}
	if (Wait)
		Pause();
	free(symTo);
	free(symFrom);
	return rc;
}



/*
 * Function will list users home directory
 */
void List_Home()
{
	DIR		*dirp;
	char		*FileName, *temp;
	int		iFileCount = 0;
	int		iBytes = 0;
	struct dirent	*dp;
	struct stat	statfile;

	FileName = calloc(PATH_MAX, sizeof(char));
	temp     = calloc(PATH_MAX, sizeof(char));

	iLineCount = 2;
	clear();
	sprintf(temp, "%s/%s/wrk", CFG.bbs_usersdir, exitinfo.Name);

	if ((dirp = opendir(temp)) == NULL) {
		WriteError("$List_Home: Can't open dir: %s", temp);
		/* Can't open directory for listing: */
		printf("\n%s\n\n", (char *) Language(290));
		Pause();
	} else {
		colour(1, 7);
		/* Home directory listing for */
		printf(" %s", (char *) Language(291));
		colour(4, 7);
		printf("%-51s\n", exitinfo.sUserName);

		while ((dp = readdir( dirp )) != NULL ) {
			sprintf(FileName, "%s/%s", temp, dp->d_name);
			/*
			 * Check first letter of file for a ".", do not display hidden files
			 * This includes the current directory and parent directory . & ..
			 */
			if (*(dp->d_name) != '.') {
				iFileCount++;
				if(stat(FileName, &statfile) != 0)
					WriteError("$Can't stat file %s",FileName);
				iBytes += statfile.st_size;

				colour(14,0);
				printf("%-20s", dp->d_name);

				colour(13,0);
				printf("%-12ld", (long)(statfile.st_size));

				colour(10,0);
				printf("%s  ", StrDateDMY(statfile.st_mtime));

				colour(11,0);
				printf("%s", StrTimeHMS(statfile.st_mtime));

				printf("\n");
			}
			if (iLC(1) == 1)
				return;
		}

		colour(11,0);
		/* Total Files: */ /* Bytes */
		printf("\n\n%s%d / %d %s\n", (char *) Language(242), iFileCount, iBytes, (char *) Language(354));

		Pause();
		closedir(dirp);
	}

	free(temp);
	free(FileName);
}



/*
 * Delete files from home directory
 */
void Delete_Home()
{
	char	*temp, *temp1;
	int	i;

	temp  = calloc(PATH_MAX, sizeof(char));
	temp1 = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/%s/wrk/", CFG.bbs_usersdir, exitinfo.Name);

	Enter(1);
	/* Please enter filename to delete: */
	pout(9, 0, (char *) Language(292));
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
	GetstrC(temp1, 80);


	if(strcmp(temp1, "") == 0) {
		free(temp);
		free(temp1);
		return;
	}

	if(temp1[0] == '.') {
		Enter(1);
		/* Sorry you may not delete hidden files ...*/
		pout(12, 0, (char *) Language(293));
	} else {
		strcat(temp, temp1);

		if ((access(temp, R_OK)) == 0) {
			colour(10, 0);
			/* Delete file: */ /* Are you Sure? [Y/n]: */
			printf("\n%s %s, %s", (char *) Language(368), temp1, (char *) Language(369));
			fflush(stdout);
			i = toupper(Getone());

			if (i == Keystroke(369, 0) || i == 13) {
				i = unlink(temp);

				if (i == -1) {
					Enter(1);
					/* Unable to delete file ... */
					pout(12, 0, (char *) Language(294));
				} else {
					Syslog('+', "Delete %s from homedir", temp1);
				}
			} else {
				Enter(2);
				/* Aborting ... */
				pout(8, 0, (char *) Language(116));
			}
		} else {
			Enter(1);
			/*  Invalid filename, please try again ... */
			pout(12, 0, (char *) Language(295));
		}

	}

	free(temp);
	free(temp1);
	printf("\n");
	Pause();
}



/*
 * Function allows user to download from his/her home directory
 * but still does all the necessary checks
 */
int Download_Home()
{
	char	*temp, *File;
	struct	stat statfile;
	int	rc;

	File  = calloc(PATH_MAX, sizeof(char));
	temp  = calloc(PATH_MAX, sizeof(char));

	WhosDoingWhat(DOWNLOAD, NULL);

	colour(14,0);
	/* Please enter filename: */
	printf("\n%s", (char *) Language(245));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(File, 80);

	if(( strcmp(File, "")) == 0) {
		colour(CFG.HiliteF, CFG.HiliteB);
		/* No filename entered, Aborting. */
		printf("\n\n%s\n", (char *) Language(246));
		Pause();
		free(File);
		free(temp);
		return FALSE;
	}

	if( *(File) == '/' || *(File) == ' ') {
		colour(CFG.HiliteF, CFG.HiliteB);
		/* Illegal Filename! */
		printf("\n%s\n\n", (char *) Language(247));
		Pause();
		free(File);
		free(temp);
		return FALSE;
	}

	/*
	 * Get path for users home directory
	 */
	sprintf(temp, "%s/%s/wrk/%s", CFG.bbs_usersdir, exitinfo.Name, File);

	if (stat(temp, &statfile) != 0) {
		Enter(1);
		/* File does not exist, please try again ...*/
  		pout(12, 0, (char *) Language(296));
		Enter(2);
		Pause();
		free(File);
		free(temp);
		return FALSE;
	}

	rc = DownloadDirect(temp, TRUE);

	free(File);
	free(temp);
	return rc;
}



/*
 * Function will upload to users home directory
 */
int Upload_Home()
{
	DIR		*dirp;
	struct dirent	*dp;
	char		*File, *sFileName, *temp, *arc;
	time_t		ElapstimeStart, ElapstimeFin, iTime;
	int		err;
	struct stat	statfile;
	
	WhosDoingWhat(UPLOAD, NULL);
	if (!ForceProtocol())
		return 0;

        File      = calloc(PATH_MAX, sizeof(char));
        sFileName = calloc(PATH_MAX, sizeof(char));
        temp      = calloc(PATH_MAX, sizeof(char));

	if (!uProtBatch) {

		Enter(1);
		/* Please enter file to upload: */
		pout(14, 0, (char *) Language(276));

		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(File, 80);

		if((strcmp(File, "")) == 0) {
			free(File);
			free(sFileName);
			free(temp);
			return 0;
		}

		if(File[0] == '.' || File[0] == '*' || File[0] == ' ') {
			colour(CFG.HiliteF, CFG.HiliteB);
			/* Illegal Filename! */
			printf("\n%s\n\n", (char *) Language(247));
			Pause();
                        free(File);
                        free(sFileName);
                        free(temp);
			return 0;
		}

		Strlen = strlen(File);
		Strlen--;

		if(File[Strlen] == '.' || File[Strlen] == '/' || File[Strlen] == ' ') {
			colour(CFG.HiliteF, CFG.HiliteB);
			/* Illegal Filename! */
			printf("\n%s\n\n", (char *) Language(247));
			Pause();
                        free(File);
                        free(sFileName);
                        free(temp);
			return 0;
		}

	}

	clear();
	colour(CFG.HiliteF, CFG.HiliteB);
	/* Please start your upload now ...*/
	printf("\n\n%s, %s\n\n", sProtAdvice, (char *) Language(283));
	if (uProtBatch)
		Syslog('+', "Upload using %s", sProtName);
	else
		Syslog('+', "Upload \"%s\" using %s", File, sProtName);

	sprintf(temp, "%s/%s/upl", CFG.bbs_usersdir, exitinfo.Name);
	if (chdir(temp)) {
		WriteError("$Can't chdir to %s", temp);
		free(File);
		free(sFileName);
		free(temp);
		return 0;
	}

	fflush(stdout);
	fflush(stdin);
	sleep(2);
	ElapstimeStart = time(NULL);
	
	/*
	 * Get the file(s). Set the Client/Server time to 2 hours.
	 * This is not a nice solution, at least it works and prevents
	 * that the bbs will hang.
	 */
	Altime(7200);
	alarm_set(7190);
	if ((err = execute_str(sProtUp, (char *)"", NULL, NULL, NULL, NULL))) {
		/*
		 * Log any errors
		 */
		colour(CFG.HiliteF, CFG.HiliteB);
		WriteError("$Upload error %d, prot: %s", err, sProtUp);
	}
	Altime(0);
	alarm_off();
	alarm_on();
	printf("\n\n\n");
	fflush(stdout);
	fflush(stdin);
	ElapstimeFin = time(NULL);

	/*
	 * Get time from Before Upload and After Upload to get
	 * upload time, if the time is zero, it will be one.
	 */
	iTime = ElapstimeFin - ElapstimeStart;
	if (!iTime)
		iTime = 1;

	Syslog('b', "Transfer time %ld", iTime);

	if ((dirp = opendir(".")) == NULL) {
		WriteError("$Upload: can't open ./upl");
		Home();
                free(File);
                free(sFileName);
                free(temp);
		return 1;
	}

	Syslog('b', "Start checking uploaded files");
	pout(CFG.UnderlineColourF, CFG.UnderlineColourB, (char *)"\n\nChecking your upload(s)\n\n");

	while ((dp = readdir(dirp)) != NULL) {

		if (*(dp->d_name) != '.') {
			stat(dp->d_name, &statfile);
			Syslog('+', "Uploaded \"%s\", %ld bytes", dp->d_name, statfile.st_size);

			if ((arc = GetFileType(dp->d_name)) == NULL) {
				/*
				 * If the filetype is unknown, it is probably 
				 * a textfile or so. Import it direct.
				 */
				Syslog('b', "Unknown file type");
				ImportHome(dp->d_name);
			} else {
				/*
				 * We figured out the type of the uploaded file.
				 */
				Syslog('b', "File type is %s", arc);

				/*
				 * MS-DOS executables are handled direct.
				 */
				if ((strcmp("EXE", arc) == 0) || (strcmp("COM", arc) == 0)) {
					if (!ScanDirect(dp->d_name)) 
						ImportHome(dp->d_name);
				} else {
					switch(ScanArchive(dp->d_name, arc)) {

					case 0:
						ImportHome(dp->d_name);
						break;

					case 1:
						break;

					case 2:
						break;

					case 3:
						/*
						 * No valid unarchiver found, just import
						 */
						ImportHome(dp->d_name);
						break;
					}
				}
			}
		}
	}
	closedir(dirp);
	Home();

	ReadExitinfo();
	exitinfo.Uploads++;
	WriteExitinfo();	

	Pause();
	free(File);
	free(sFileName);
	free(temp);
	return 1;
}



/*
 * Select filearea, called from menu.
 */
void FileArea_List(char *Option)
{
	FILE	*pAreas;
	int	iAreaCount = 6, Recno = 1;
	int	iOldArea, iAreaNum = 0;
	int	iGotArea = FALSE; /* Flag to check if user typed in area */
	long	offset;
	char 	*temp;

	/*
	 * Save old area, incase he picks a invalid area
	 */
	iOldArea = iAreaNumber;
	if ((pAreas = OpenFareas(FALSE)) == NULL)
		return;

	/*
	 * Count howmany records there are
	 */
	fseek(pAreas, 0, SEEK_END);
	iAreaNum = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

	/*
	 * If there are menu options, select area direct.
	 */
	if (strlen(Option) != 0) {

		if (strcmp(Option, "F+") == 0)
			while(TRUE) {
				iAreaNumber++;
				if (iAreaNumber > iAreaNum)
					iAreaNumber = 1;

				offset = areahdr.hdrsize + ((iAreaNumber - 1) * areahdr.recsize);
				if (fseek(pAreas, offset, 0) != 0) {
					printf("Can't move pointer here");
				}

				fread(&area, areahdr.recsize, 1, pAreas);
				if ((Access(exitinfo.Security, area.LTSec)) && (area.Available) && (strlen(area.Password) == 0))
					break;
			}

		if (strcmp(Option, "F-") == 0)
			while(TRUE) {
				iAreaNumber--;
				if (iAreaNumber < 1)
					iAreaNumber = iAreaNum;

				offset = areahdr.hdrsize + ((iAreaNumber - 1) * areahdr.recsize);
				if (fseek(pAreas, offset, 0) != 0) {
					printf("Can't move pointer here");
				}

				fread(&area, areahdr.recsize, 1, pAreas);
				if ((Access(exitinfo.Security, area.LTSec)) && (area.Available) && (strlen(area.Password) == 0))
					break;
			}
		SetFileArea(iAreaNumber);
		Syslog('+', "File area %lu %s", iAreaNumber, sAreaDesc);
		fclose(pAreas);
		return;
	}

	/*
	 * Interactive mode
	 */
	clear();
	Enter(1);
	/* File Areas */
	pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(298));
	Enter(2);
	temp = calloc(81, sizeof(char));

	fseek(pAreas, areahdr.hdrsize, 0);

	while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {

		if ((Access(exitinfo.Security, area.LTSec)) && (area.Available)) {
			area.Name[31] = '\0';

			colour(15,0);
			printf("%5d", Recno);

			colour(9,0);
			printf(" %c ", 46);

			colour(3,0);
			printf("%-31s", area.Name);

			iAreaCount++;

			if ((iAreaCount % 2) == 0)
				printf("\n");
			else
				printf(" ");
		}

		Recno++; 

		if ((iAreaCount / 2) == exitinfo.iScreenLen) {
			/* More (Y/n/=/Area #): */
			pout(CFG.MoreF, CFG.MoreB, (char *) Language(207)); 
			/*
			 * Ask user for Area or enter to continue
			 */
			colour(CFG.InputColourF, CFG.InputColourB);
			fflush(stdout); 
			GetstrC(temp, 7);

			if (toupper(*(temp)) == Keystroke(207, 1))
				break;

			if ((strcmp(temp, "")) != 0) {
				iGotArea = TRUE;
				break;
			}

			iAreaCount = 2;
		}
	}

	/*
	 * If user type in area above during area listing
	 * don't ask for it again
	 */
	if (!iGotArea) {
		Enter(1);
		/* Select Area: */
		pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(232));
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp, 80);
	} 

	/*
	 * Check if user pressed ENTER
	 */
	if((strcmp(temp, "")) == 0) {
		fclose(pAreas);
		return;
	}

	iAreaNumber = atoi(temp);

	/*
	 * Do a check in case user enters a negative value
	 */
	if (iAreaNumber < 1) 
		iAreaNumber = 1;

	offset = areahdr.hdrsize + ((iAreaNumber - 1) * areahdr.recsize); 
	if(fseek(pAreas, offset, 0) != 0) 
		printf("Can't move pointer there."); 
	else
		fread(&area, areahdr.recsize, 1, pAreas);

	/*
	 * Do a check if area is greater or less number than allowed,
	 * security access level, is oke, and the area is active.
	 */
	if (iAreaNumber > iAreaNum || iAreaNumber < 1 || 
	    (Access(exitinfo.Security, area.LTSec) == FALSE) || 
	    (strlen(area.Name) == 0)) {
		Enter(1);
		/* Invalid area specified - Please try again ...*/
		pout(12, 0, (char *) Language(233));
		Enter(2);
		Pause();
		fclose(pAreas);
		iAreaNumber = iOldArea;
		SetFileArea(iAreaNumber);
		free(temp);
		return;
	}

	SetFileArea(iAreaNumber);
	Syslog('+', "File area %lu %s", iAreaNumber, sAreaDesc);

	/*
	 * Check if file area has a password, if it does ask user for it
	 */ 
	if((strlen(area.Password)) > 2) {
		Enter(2);
		/* Please enter Area Password: */
		pout(15, 0, (char *) Language(299));
		fflush(stdout);
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp, 20);

		if((strcmp(temp, area.Password)) != 0) { 
			Enter(1);
			/* Password is incorrect */
			pout(15, 0, (char *) Language(234));
			Enter(2);
			Syslog('!', "Incorrect File Area # %d password given: %s", iAreaNumber, temp);
			SetFileArea(iOldArea);
		} else {
			Enter(1);
			/* Password is correct */
			pout(15, 0, (char *) Language(235));
			Enter(2);
		}
		Pause();
	} 

	free(temp);
	fclose(pAreas);
}



/*
 * Show filelist from current area, called from the menu.
 */
void Copy_Home()
{
    FILE    *pFile;
    char    *File, *temp1, *temp2;
    int	    err, Found = FALSE;
	
    File  = calloc(81, sizeof(char));
    temp1 = calloc(PATH_MAX, sizeof(char));
    temp2 = calloc(PATH_MAX, sizeof(char));
	
    colour(14,0);
    /* Please enter filename: */
    printf("\n%s", (char *) Language(245));
    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(File, 80);

    if ((strcmp(File, "")) == 0) {
	colour(CFG.HiliteF, CFG.HiliteB);
	/* No filename entered, Aborting. */
	printf("\n\n%s\n", (char *) Language(246));
	Pause();
	free(File);
	free(temp1);
	free(temp2);
	return;
    }

    if (*(File) == '/' || *(File) == ' ') {
	colour(CFG.HiliteF, CFG.HiliteB);
	/* Illegal Filename! */
	printf("\n%s\n\n", (char *) Language(247));
	Pause();
        free(File);
        free(temp1);
        free(temp2);
	return;
    }

    if (Access(exitinfo.Security, area.DLSec) == FALSE) {
	colour(14, 0);
	printf("\n%s\n", (char *) Language(236));
	Pause();
        free(File);
        free(temp1);
        free(temp2);
	return;
    }

    if ((pFile = OpenFileBase(iAreaNumber, FALSE)) == NULL) {
        free(File);
        free(temp1);
        free(temp2);
	return;
    }

    while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {

	if ((strcasecmp(File, fdb.Name) == 0) || (strcasecmp(File, fdb.LName) == 0)) {

	    Found = TRUE;
	    if (((fdb.Size + Quota()) > (CFG.iQuota * 1048576))) {
		colour(CFG.HiliteF, CFG.HiliteB);
		/* You have not enough diskspace free to copy this file */
		printf("%s\n", (char *) Language(279));
		Syslog('+', "Copy homedir, not enough quota");
	    } else {
		sprintf(temp1, "%s/%s", area.Path, fdb.LName); /* Use real longname here */
		sprintf(temp2, "%s/%s/wrk/%s", CFG.bbs_usersdir, exitinfo.Name, File);
		colour(CFG.TextColourF, CFG.TextColourB);
		/* Start copy: */
		printf("%s%s ", (char *) Language(289), File);
		fflush(stdout);

		Syslog('b', "Copy from : %s", temp1);
		Syslog('b', "Copy to   : %s", temp2);

		if ((err = file_cp(temp1, temp2))) {
		    colour(CFG.HiliteF, CFG.HiliteB);
		    /* Failed! */
		    printf("%s\n", (char *) Language(353));
		    WriteError("Copy %s to homedir failed, code %d", File, err);
		} else {
		    /* Ok */
		    printf("%s\n", (char *) Language(200));
		    Syslog('+', "Copied %s from area %d to homedir", File, iAreaNumber);
		}
	    }
	}
    }
    fclose(pFile);

    if (!Found) {
	colour(CFG.HiliteF, CFG.HiliteB);
	/* File does not exist, please try again ... */
	printf("%s\n", (char *) Language(296));
    }

    Pause();
    free(File);
    free(temp1);
    free(temp2);
}



/*
 * Edit the list of tagged files.
 */
void EditTaglist()
{
	FILE	*tf;
	int	i, x, Fg, Count;
	char	*temp;

	if ((tf = fopen("taglist", "r+")) == NULL) {
		colour(CFG.HiliteF, CFG.HiliteB);
		/* No files tagged. */
		printf("\n%s\n\n", (char *) Language(361));
		Pause();
		return;
	}

	temp = calloc(81, sizeof(char));

	while (TRUE) {
		clear();
		fseek(tf, 0, SEEK_SET);
		Count = 0;
		colour(CFG.HiliteF, CFG.HiliteB);
		/* #  Area  Active  File               Size  Cost */
		printf("%s\n", (char *) Language(355));
		colour(10, 0);
		fLine(48);

		while ((fread(&Tag, sizeof(Tag), 1, tf) == 1)) {
			Count++;

			if (Tag.Active)
				Fg = 15;
			else
				Fg = 7;

			colour(Fg, 0);
			printf("%3d ", Count);

			Fg--;
			colour(Fg, 0);
			printf("%5ld  ", Tag.Area);

			Fg--;
			colour(Fg, 0);
			if (Tag.Active)
				/* Yes */
				printf("%-6s  ", (char *) Language(356));
			else
				/* No */
				printf("%-6s  ", (char *) Language(357));

			Fg--;
			colour(Fg, 0);
			printf("%-12s", Tag.SFile);

			Fg--;
			colour(Fg, 0);
			printf(" %8ld", (long)(Tag.Size));

			Fg--;
			colour(Fg, 0);
			printf(" %5d\n", Tag.Cost);
		}
		colour(10, 0);
		fLine(48);

		colour(15, 4);
		/* (T)oggle active, (E)rase all, (ENTER) to continue: */
		printf("\n%s", (char *) Language(358));
		fflush(stdout);
		fflush(stdin);

		i = toupper(Getone());
		colour(CFG.CRColourF, CFG.CRColourB);

		if (i == Keystroke(358, 0)) {
			/* Enter file number, 1.. */
			printf("\n\n%s%d ", (char *) Language(359), Count);
			fflush(stdout);
			fflush(stdin);

			GetstrC(temp, 5);
			x = atoi(temp);

			if ((x > 0) && (x <= Count)) {
				if (fseek(tf, (x - 1) * sizeof(Tag), SEEK_SET) == 0) {
					if (fread(&Tag, sizeof(Tag), 1, tf) == 1) {
						if (Tag.Active)
							Tag.Active = FALSE;
						else
							Tag.Active = TRUE;

						fseek(tf,(x - 1) * sizeof(Tag), SEEK_SET);
						fwrite(&Tag, sizeof(Tag), 1, tf);
					}
				}
			}
		}

		if (i == Keystroke(358, 1)) {
			fclose(tf);
			unlink("taglist");
			free(temp);
			return;
		}

		if ((i == '\r') || (i == '\n')) {
			fclose(tf);
			free(temp);
			return;
		}
	}
}



/*
 * View a file in the current area, menu 103.
 * If a file name is given, display direct,
 * else ask for filename to view.
 */
void ViewFile(char *name)
{
    char    *File, *temp, *arc;
    int	    count, total, rc, found = FALSE;
    FILE    *fp, *pFile;

    Syslog('+', "ViewFile(%s)", printable(name, 0));

    if (Access(exitinfo.Security, area.LTSec) == FALSE) {
	colour(YELLOW, BLACK);
	/* You don't have enough security to list this area */
	printf("\n%s\n", (char *) Language(236));
	Pause();
	return;
    }

    File = calloc(PATH_MAX, sizeof(char));

    if ((name != NULL) && strlen(name)) {
	strcpy(File, name);
    } else {
	Enter(2);
	/* Please enter filename: */
	pout(YELLOW, BLACK, (char *) Language(245));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(File, 80);

	if ((strcmp(File, "")) == 0) {
	    free(File);
	    return;
	}
	
	if (*(File) == '.' || *(File) == '*' || *(File) == ' ' || *(File) == '/') {
	    colour(CFG.HiliteF, CFG.HiliteB);
	    /* Illegal Filename! */
	    printf("\n%s\n\n", (char *) Language(247));
	    Pause();
	    free(File);
	    return;
	}

	Strlen = strlen(File);
	Strlen--;

	if (*(File + Strlen) == '.' || *(File + Strlen) == '/' || *(File + Strlen) == ' ') {
	    colour(CFG.HiliteF, CFG.HiliteB);
	    /* Illegal Filename! */
	    printf("\n%s\n\n", (char *) Language(247));
	    Pause();
	    free(File);
	    return;
	}

	if ((!strcmp(File, "files.bbs")) || (!strcmp(File, "00index")) || (strstr(File, (char *)".html"))) {
	    colour(CFG.HiliteF, CFG.HiliteB);
	    /* Illegal Filename! */
	    printf("\n%s\n\n", (char *) Language(247));
	    Pause();
	    free(File);
	    return;
	}
    }

    /*
     * Now check if this file is present
     */
    if ((pFile = OpenFileBase(iAreaNumber, FALSE)) == NULL) {
	free(File);
	return;
    }

    while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {
	if (((strcasecmp(File, fdb.Name) == 0) || (strcasecmp(File, fdb.LName) == 0)) && (!fdb.Deleted)) {
	    found = TRUE;
	    break;
	}
    }
    fclose(pFile);

    if (!found) {
	colour(YELLOW, BLACK);
	/* File does not exist, please try again ... */
	printf("\n%s\n\n", (char *) Language(296));
	free(File);
	Pause();
	return;
    }

    sprintf(File, "%s/%s", sAreaPath, fdb.LName);
    arc = GetFileType(File);
    Syslog('+', "File to view: %s, type %s", fdb.LName, printable(arc, 0));

    if (arc != NULL) {
	found = FALSE;
	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	
	if ((fp = fopen(temp, "r")) != NULL) {
	    fread(&archiverhdr, sizeof(archiverhdr), 1, fp);
	    while (fread(&archiver, archiverhdr.recsize, 1, fp) == 1) {
		if ((strcmp(arc, archiver.name) == 0) && (archiver.available)) {
		    found = TRUE;
		    break;
		}
	    }
	    fclose(fp);
	}

	if (!found || (strlen(archiver.varc) == 0)) {
	    Syslog('+', "No archiver view for %s available", File);
	    colour(YELLOW, BLACK);
	    /* Archiver not available */
	    printf("\n%s\n\n", Language(442));
	    free(File);
	    free(temp);
	    Pause();
	    return;
	}

	/*
	 * Archiver viewer is available. Make a temp file which we will
	 * display to the user.
	 */
	sprintf(temp, "%s/%s/temptxt", CFG.bbs_usersdir, exitinfo.Name);
	rc = execute_str(archiver.varc, File, NULL, (char *)"/dev/null", temp, (char *)"/dev/null");
	Syslog('+', "Display temp file %s", temp);
	DisplayTextFile(temp);
	unlink(temp);
	free(temp);
    } else {
	/*
	 * Most likely a textfile, first check.
	 */
	total = count = 0;
	if ((fp = fopen(File, "r"))) {
	    while (TRUE) {
		rc = fgetc(fp);
		if (rc == EOF)
		    break;
		total++;
		if (isascii(rc))
		    count++;
	    }
	    fclose(fp);
	}
	if (((count * 10) / total) < 8) {
	    Syslog('+', "This is not a ASCII textfile");
	    printf("\n%s\n\n", Language(17));
	    Pause();
	    free(File);
	    return;
	}
	Syslog('+', "Display text file %s", File);
	DisplayTextFile(File);
    }

    free(File);
}


