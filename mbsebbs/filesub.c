/*****************************************************************************
 *
 * Purpose ...............: All the file sub functions. 
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "../lib/mbsedb.h"
#include "filesub.h"
#include "funcs.h"
#include "language.h"
#include "input.h"
#include "misc.h"
#include "timeout.h"
#include "exitinfo.h"
#include "change.h"
#include "term.h"
#include "ttyio.h"


extern pid_t	    	mypid;
int		    	arecno = 1;	/* Area record number			     */
int		    	Hcolor = 9;	/* Color of area line in xxxScan() functions */
extern int	    	rows;
extern unsigned int	mib_uploads;
extern unsigned int	mib_kbupload;



/*
 * Variables for file tagging
 */
int	Tagnr;
_Tag	Tagbuf[100];



/*
 * Reset the tag ringbuffer.
 */
void InitTag()
{
    int	i;

    Tagnr = 0;

    for (i = 0; i < 100; i++) {
	memset(&Tagbuf[i], 0, sizeof(_Tag));
    }
}



/*
 * Add a file in the tag ringbuffer.
 */
void SetTag(_Tag tag)
{
    if (Tagnr < 99)
	Tagnr++;
    else
	Tagnr = 1;

    Tagbuf[Tagnr] = tag;
}



/*
 * Get string, no newline afterwards.
 */
void GetstrD(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0;
    int		    iPos = 0;

    strcpy(sStr, "");

    alarm_on();
    while (ch != 13) {
	ch = Readkey();

	if (((ch == 8) || (ch == KEY_DEL) || (ch == 127)) && (iPos > 0)) {
	    BackErase();
	    sStr[--iPos]='\0';
	}

	if (ch > 31 && ch < 127) {
	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else
		PUTCHAR(7);
	}
    }
}




/*
 * Open the fareas.data file for read or R/W and read the headerrecord.
 * The filepointer is at the start of the first record.
 */
FILE *OpenFareas(int Write)
{
    FILE	*pAreas;
    char	*FileArea;

    FileArea = calloc(PATH_MAX, sizeof(char));
    snprintf(FileArea, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
		
    if (Write)
	pAreas = fopen(FileArea, "r+");
    else
	pAreas = fopen(FileArea, "r");
		
    if (pAreas == NULL) {
	WriteError("$Can't open FileBase %s", FileArea);
	/* FATAL: Unable to open areas database */
	pout(LIGHTRED, BLACK, (char *) Language(243));
	Enter(2);
	sleep(2);
    } else
	fread(&areahdr, sizeof(areahdr), 1, pAreas);

    free(FileArea); 	
    return pAreas;
}



/*
 * Pageheader for filelistings
 */
void Header()
{
    char    temp[81];
	
    pout(RED, LIGHTGRAY, (char *)" Area ");

    snprintf(temp, 81, "%-5d   ", iAreaNumber);
    pout(RED, LIGHTGRAY, temp);

    snprintf(temp, 81, "%-65s", sAreaDesc);
    pout(BLUE, LIGHTGRAY, temp);
    Enter(1);

    colour(WHITE, BLACK);
    PUTSTR(chartran(fLine_str(79)));
    iLineCount++;
}



/*
 * Searchheader for areas during xxxxScan().
 */
void Sheader()
{
    char    temp[81];

    PUTCHAR('\r');
    snprintf(temp, 81, "  %-4d", arecno);
    pout(Hcolor, BLACK, temp);

    pout(LIGHTBLUE, BLACK, (char *)" ... ");

    snprintf(temp, 81, "%-44s", area.Name);
    pout(Hcolor, BLACK, temp);

    if (Hcolor < WHITE)
	Hcolor++;
    else
	Hcolor = LIGHTBLUE;
}



/*
 * Blank current line without newline.
 */
void Blanker(int count)
{
    int i;

    for (i = 0; i < count; i++)
	PUTCHAR('\b');

    for (i = 0; i < count; i++)
	PUTCHAR(' ');

    PUTCHAR('\r');
}



/*
 * Mark one or more files for download by putting them into the "taglist"
 * in the users homedirectory. Check against dupe tags.
 */
void Mark()
{
    char    *temp;
    FILE    *fp;
    int	    i, Found, Count, Size;

    temp = calloc(81, sizeof(char));

    /*
     * First count the already tagged files.
     */
    Count = Size = 0;
    if ((fp = fopen("taglist", "r")) != NULL) {
	while (fread(&Tag, sizeof(Tag), 1, fp) == 1) {
	    if (Tag.Active) {
		Count++;
		Size += (Tag.Size / 1024);
	    }
	}
	fclose(fp);
    }

    /* Marked: */
    snprintf(temp, 81, "%s%d, %dK; ", (char *) Language(360), Count, Size);
    pout(CFG.HiliteF, CFG.HiliteB, temp);

    /* Mark file number of press <Enter> to stop */
    PUTSTR((char *) Language(7));

    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrD(temp, 10);
    Blanker(strlen(Language(7)) + strlen(temp));

    if (strlen(temp) == 0) {
	free(temp);
	return;
    }

    i = atoi(temp);

    if ((i > 0) && (i < 100)) {
	if ((Tagbuf[i].Area) && (strlen(Tagbuf[i].LFile))) {
	    if (Access(exitinfo.Security, area.DLSec)) {
		if ((fp = fopen("taglist", "a+")) != NULL) {

		    fseek(fp, 0, SEEK_SET);
		    Found = FALSE;
		    while (fread(&Tag, sizeof(Tag), 1, fp) == 1)
			if ((Tag.Area == Tagbuf[i].Area) && (strcmp(Tag.LFile, Tagbuf[i].LFile) == 0)) {
			    Found = TRUE;
			    Syslog('b', "Tagbuf[i].File already tagged");
			}

		    if (!Found) {
			memset(&Tag, 0, sizeof(Tag));
			Tag = Tagbuf[i];
			Tag.Active = TRUE;
			fwrite(&Tag, sizeof(Tag), 1, fp);
			Syslog('+', "Tagged file %s from area %d", Tag.LFile, Tag.Area);
		    }

		    fclose(fp);
		}
	    } else {
		/* You do not have enough access to download from this area. */
		pout(LIGHTRED, BLACK, (char *) Language(244));
		sleep(3);
		Blanker(strlen(Language(244)));
	    }
	}
    }

    free(temp);
}



/* 
 * More prompt, returns 1 if user decides not to look any further.
 */
int iLC(int Lines)
{
    int	x, z;

    x = strlen(Language(131));
    iLineCount += Lines;
 
    if ((iLineCount >= rows) && (iLineCount < 1000)) {
	iLineCount = 1;

	while (TRUE) {
	    /* More (Y/n/=) M=Mark */
	    pout(CFG.MoreF, CFG.MoreB, (char *) Language(131));

	    alarm_on();
	    z = toupper(Readkey());
	    Blanker(x);

	    if (z == Keystroke(131, 1)) {
		Enter(1);
		return 1;
	    }

	    if (z == Keystroke(131, 2)) {
		iLineCount = 9000;
		return 0;
	    }

	    if ((z == Keystroke(131, 0)) || (z == '\r') || (z == '\n')) {
		return 0;
	    }

	    if (z == Keystroke(131, 3)) {
		Mark();
	    }
	}
    }
    return 0;
}



/*
 * Show one file, return 1 if user wants to stop, 0 to show next file.
 */
int ShowOneFile()
{
    int	    y, z, fg, bg;
    char    temp[81];

    if (!fdb.Deleted) {

	snprintf(temp, 81, " %02d ", Tagnr);
	pout(LIGHTGRAY, BLACK, temp);

	snprintf(temp, 81, "[%4d] ", fdb.TimesDL);
	pout(LIGHTRED, BLACK, temp);

	snprintf(temp, 81, "%s", fdb.LName);
	pout(CFG.FilenameF, CFG.FilenameB, temp);
	Enter(1);
	if (iLC(1) == 1)
	    return 1;

	snprintf(temp, 81, "    %-10s ",  StrDateDMY(fdb.UploadDate));
	pout(CFG.FiledateF, CFG.FiledateB, temp);

	snprintf(temp, 81, "%10u bytes ", (int)(fdb.Size));
	pout(CFG.FilesizeF, CFG.FilesizeB, temp);

//	snprintf(temp, 81, "%-12s", fdb.Name);
//	pout(CFG.FilenameF, CFG.FilenameB, temp);

//	snprintf(temp, 81, "%10u ", (int)(fdb.Size));
//	pout(CFG.FilesizeF, CFG.FilesizeB, temp);

//	snprintf(temp, 81, "%-10s  ", StrDateDMY(fdb.UploadDate));
//	pout(CFG.FiledateF, CFG.FiledateB, temp);

//	snprintf(temp, 81, "[%4d] ", fdb.TimesDL);
//	pout(LIGHTRED, BLACK, temp);

//	if ((strcmp(fdb.Uploader, "")) == 0)
//	    strcpy(fdb.Uploader, "SysOp");

//	snprintf(temp, 81, "%s%s", (char *) Language(238), fdb.Uploader);
//	pout(CFG.HiliteF, CFG.HiliteB, temp);
	Enter(1);

	if (iLC(1) == 1) 
	    return 1;

	for (z = 0; z < 25; z++) {
	    if ((y = strlen(fdb.Desc[z])) > 1) {
		if ((fdb.Desc[z][0] == '@') && (fdb.Desc[z][1] == 'X')) {
		    /*
		     *  Color formatted description lines.
		     */
		    if (fdb.Desc[z][3] > '9')
			fg = (int)fdb.Desc[z][3] - 55;
		    else
			fg = (int)fdb.Desc[z][3] - 48;
		    bg = (int)fdb.Desc[z][2] - 48;
		    colour(fg, bg);
		    PUTSTR((char *)"    ");
		    PUTSTR(chartran(fdb.Desc[z]+4));
		} else {
		    colour(CFG.FiledescF, CFG.FiledescB);
		    PUTSTR((char *)"    ");
		    PUTSTR(chartran(fdb.Desc[z]));
		}
		Enter(1);

		if (iLC(1) == 1) 
		    return 1;
	    }
	}
    }
    return 0;
}



int CheckBytesAvailable(int CostSize)
{
    char    temp[81];

    if (LIMIT.DownK) {
	if ((exitinfo.DownloadKToday <= 0) || ((CostSize / 1024) > exitinfo.DownloadKToday)) {
	
	    /* You do not have enough bytes to download \" */
	    pout(LIGHTRED, BLACK, (char *) Language(252));
	    Enter(1);
	    Syslog('+', "Not enough bytes to download %ld", CostSize);

	    /* You must upload before you can download. */
	    pout(LIGHTRED, BLACK, (char *) Language(253));
	    Enter(2);

	    /* Kilobytes currently available: */
	    snprintf(temp, 81, "%s%u Kbytes.", (char *) Language(254), exitinfo.DownloadKToday);
	    pout(YELLOW, BLACK, temp);
	    Enter(2);

	    Pause();
	    return FALSE;
	}
    }
	
    return TRUE;
}



/*
 * Change back to users homedir.
 */
void Home()
{
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/%s", CFG.bbs_usersdir, exitinfo.Name);
    chdir(temp);
    free(temp);
}



/*
 * Scan a .COM or .EXE file in users upload directory.
 */
int ScanDirect(char *fn)
{
    int	    Found = FALSE;
    char    *temp, msg[81];

    temp  = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, fn);

				/* Scanning */
    snprintf(msg, 81, "%s %s ", (char *) Language(132), fn);
    pout(CFG.TextColourF, CFG.TextColourB, msg);

    if (VirScanFile(temp)) {
	/* Possible VIRUS found! */
	snprintf(msg, 81, "%s", (char *) Language(199));
	pout(CFG.HiliteF, CFG.HiliteB, msg);
	Found = TRUE;
    } else {
	/* Ok */
	snprintf(msg, 81, "%s", (char *) Language(200));
	PUTSTR(msg);
    }
    Enter(1);
    free(temp);
    return Found;
}



/*
 * Scan archive using users ./tmp directory.
 * Return codes:
 *   0 - All seems well
 *   1 - Error unpacking archive
 *   2 - Possible virus found
 *   3 - Not a known archive format.
 */
int ScanArchive(char *fn, char *ftype)
{
    FILE    *fp;
    char    *temp, *temp2, msg[81], *cwd = NULL, *fid;

    temp = calloc(PATH_MAX, sizeof(char));
    
    /*
     * Scan file for viri
     */
    snprintf(msg, 81, "%s %s ", (char *) Language(132), fn);
    pout(CFG.TextColourF, CFG.TextColourB, msg);
    snprintf(temp, PATH_MAX, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, fn);
    if (VirScanFile(temp)) {
	/* Possible VIRUS found! */
	pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(199));
	free(temp);
	Enter(1);
	return 2;
    } else {
	/* Ok */
	PUTSTR((char *) Language(200));
    }
    Enter(1);

    /*
     * Search the right archiver program.
     */
    snprintf(temp,   PATH_MAX, "%s/etc/archiver.data", getenv("MBSE_ROOT"));
	
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return 3;
    }

    fread(&archiverhdr, sizeof(archiverhdr), 1, fp);

    while (fread(&archiver, archiverhdr.recsize, 1, fp) == 1) {
	if ((strcmp(ftype, archiver.name) == 0) && (archiver.available)) {
	    break;
	}
    }
    fclose(fp);
    if ((strcmp(ftype, archiver.name)) || (!archiver.available)) {
	free(temp);
	return 3;
    }

    cwd = getcwd(cwd, 80);
    snprintf(temp, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);
    if (chdir(temp)) {
	WriteError("$Can't chdir(%s)", temp);
	free(temp);
	return 1;
    }

    /* Unpacking archive */
    snprintf(msg, 81, "%s %s ", (char *) Language(201), fn);
    pout(CFG.TextColourF, CFG.TextColourB, msg);

    if (!strlen(archiver.funarc)) {
	WriteError("No unarc command available");
    } else {
	snprintf(temp, PATH_MAX, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, fn);
	if (execute_str(archiver.funarc, temp, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null")) {
	    WriteError("$Failed %s %s", archiver.funarc, temp);
	    execute_pth((char *)"rm", (char *)"-r -f ./*", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
	    chdir(cwd);
	    free(cwd);
	    /* ERROR */
	    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(217));
	    Enter(1);
	    return 1;
	}
    }

    /* Ok */
    PUTSTR((char *) Language(200));
    Enter(1);

    /*
     * While we are here, see if there is a FILE_ID.DIZ
     */
    fid = xstrcpy((char *)"FILE_ID.DIZ");
    snprintf(temp, PATH_MAX, "%s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);
    if (getfilecase(temp, fid)) {
	snprintf(temp, PATH_MAX, "%s/%s/tmp/%s", CFG.bbs_usersdir, exitinfo.Name, fid);
	temp2 = calloc(PATH_MAX, sizeof(char));
	snprintf(temp2, PATH_MAX, "%s/%s/wrk/FILE_ID.DIZ", CFG.bbs_usersdir, exitinfo.Name);
	if (file_cp(temp, temp2) == 0) {
	    Syslog('b', "Copied %s", temp);
	}
	free(temp2);
    }
    free(fid);

    /*
     * Remove and recreate tmp directory if it was used (or not)
     */
    snprintf(temp, PATH_MAX, "%s/%s", CFG.bbs_usersdir, exitinfo.Name);
    chdir(temp);
    snprintf(temp, PATH_MAX, "-r -f %s/%s/tmp", CFG.bbs_usersdir, exitinfo.Name);
    execute_pth((char *)"rm", temp, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    execute_pth((char *)"mkdir", (char *)"tmp", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    chdir(cwd);
    free(cwd);
    free(temp);
	
    return 0;
}



/*
 * Try to find out the type of uploaded file.
 */
char *GetFileType(char *fn)
{
	unsigned char	buf[8], dbuf[80];
	FILE		*fp;
	int		i;

	if ((fp = fopen(fn, "r")) == NULL) {
		WriteError("$Can't open file %s", fn);
		return NULL;
	}

	if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) {
		WriteError("$Can't read head of file %s", fn);
		return NULL;
	}

	fclose(fp);
	dbuf[0] = '\0';

	for (i = 0; i < sizeof(buf); i++)
		if ((buf[i] >= ' ') && (buf[i] <= 127))
			snprintf((char*)dbuf+strlen((char *)dbuf), 80, "  %c", buf[i]);
		else
			snprintf((char*)dbuf+strlen((char *)dbuf), 80, " %02x", buf[i]);

	/*
	 * Various expected uploads. Not that the standard MS-DOS archivers
	 * must return the exact format, ie "ZIP" for PKZIP. These strings
	 * are tested against the archivers database. Others that aren't
	 * compressed files are not important, they just pop up in your
	 * logfiles.
	 */
 	if (memcmp(buf, "PK\003\004", 4) == 0)		return (char *)"ZIP";
	if (*buf == 0x1a)				return (char *)"ARC";
	if (memcmp(buf+2, "-l", 2) == 0)		return (char *)"LHA";
	if (memcmp(buf, "ZOO", 3) == 0)			return (char *)"ZOO";
	if (memcmp(buf, "`\352", 2) == 0)		return (char *)"ARJ";
	if (memcmp(buf, "Rar!", 4) == 0)		return (char *)"RAR";
	if (memcmp(buf, "HA", 2) == 0)			return (char *)"HA";
	if (memcmp(buf, "MZ", 2) == 0)			return (char *)"EXE";
	if (memcmp(buf, "\000\000\001\263", 4) == 0)	return (char *)"MPEG";
	if (memcmp(buf, "MOVI", 4) == 0)		return (char *)"MOVI";
	if (memcmp(buf, "\007\007\007", 3) == 0)	return (char *)"CPIO";
	if (memcmp(buf, "\351,\001JAM", 6) == 0)	return (char *)"JAM";
	if (memcmp(buf, "SQSH", 4) == 0)		return (char *)"SQSH";
	if (memcmp(buf, "UC2\0x1a", 4) == 0)		return (char *)"UC2";
	if (memcmp(buf, ".snd", 4) == 0)		return (char *)"SND";
	if (memcmp(buf, "MThd", 4) == 0)		return (char *)"MID";
	if (memcmp(buf, "RIFF", 4) == 0)		return (char *)"WAV";
	if (memcmp(buf, "EMOD", 4) == 0)		return (char *)"MOD";
	if (memcmp(buf, "MTM", 3) == 0)			return (char *)"MTM";
	if (memcmp(buf, "#!/bin/", 7) == 0)		return (char *)"UNIX script";
	if (memcmp(buf, "\037\235", 2) == 0)		return (char *)"Compressed data";
	if (memcmp(buf, "\037\213", 2) == 0)		return (char *)"GZIP";
	if (memcmp(buf, "\177ELF", 4) == 0)		return (char *)"ELF";
	if (memcmp(buf, "%!", 2) == 0)			return (char *)"PostScript";
	if (memcmp(buf, "GIF8", 4) == 0)		return (char *)"GIF";
	if (memcmp(buf, "\377\330\377\340", 4) == 0)	return (char *)"JPEG";
	if (memcmp(buf, "\377\330\377\356", 4) == 0)	return (char *)"JPG";
	if (memcmp(buf, "BM", 2) == 0)			return (char *)"Bitmap";
	if (memcmp(buf, "%PDF", 4) == 0)		return (char *)"PDF";
	if (memcmp(buf, "THNL", 4) == 0)		return (char *)"ThumbNail";
	if ((memcmp(buf, "<html>", 6) == 0) ||
	    (memcmp(buf, "<HTML>", 6) == 0))		return (char *)"HTML";
	if (memcmp(buf, "MSCF", 4) == 0)		return (char *)"CAB";
	if (memcmp(buf, "BZ", 2) == 0)			return (char *)"BZIP";

	/*
	 * .COM formats. Should cover about 2/3 of COM files.
	 */
	if ((*buf == 0xe9) || (*buf == 0x8c) ||
	    (*buf == 0xeb) || (*buf == 0xb8))		return (char *)"COM";
	
	return NULL;
}



/*
 * Import file in area. Returns TRUE if successfull.
 */
int ImportFile(char *fn, int Area, int fileid, off_t Size)
{
    char    *temp, *temp1, msg[81];

    temp  = calloc(PATH_MAX, sizeof(char));
    temp1 = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/%s", area.Path, basename(fn));
    snprintf(temp1, PATH_MAX, "%s", fn);

    if ((file_mv(temp1, temp))) {
	WriteError("$Can't move %s to %s", fn, area.Path);
    } else {
	chmod(temp, 0664);
	if (Addfile(basename(fn), Area, fileid)) {

	    ReadExitinfo();

	    /*
	     * If Size is equal to Zero, don't increase file counters else
	     * Increase file counters if any other size
	     */
	    if (Size) {
		exitinfo.Uploads++;
		mib_uploads++;
		exitinfo.UploadK += (Size / 1024);
		mib_kbupload += (Size / 1024);
		exitinfo.UploadKToday += (Size / 1024);
		Syslog('b', "Uploads %d, Kb %d, Kb today %d", exitinfo.Uploads, exitinfo.UploadK, exitinfo.UploadKToday);
		/* You have */  /* extra download KBytes. */
		snprintf(msg, 81, "%s %d %s", (char *) Language(249), (int)(Size / 1024), (char *) Language(250));
		PUTSTR(msg);
		Enter(1);
	
		exitinfo.DownloadKToday += (Size / 1024);
		Syslog('b', "DownloadKToday %d", exitinfo.DownloadKToday);
	    }

	    WriteExitinfo();
	    free(temp);
	    free(temp1);
	    return TRUE;
	}
    }

    free(temp);
    free(temp1);
    return FALSE;
}



/*
 * Add file to the FileDataBase. If fileid is true, then try to
 * get the filedescription from FILE_ID.DIZ if it is in the
 * archive, else the user must supply the description.
 * Returns TRUE is successfull.
 */
int Addfile(char *File, int AreaNum, int fileid)
{
    FILE    *id, *pPrivate;
    int	    iDesc = 1, GotId = FALSE, lines, i, j;
    char    *Filename, *temp1, *idname = NULL, *Desc[26], *lname, temp[PATH_MAX], msg[81]; 
    struct  stat statfile; 
    struct _fdbarea *fdb_area = NULL;

    Filename = calloc(PATH_MAX, sizeof(char));
    temp1    = calloc(PATH_MAX, sizeof(char));  
    lname    = calloc(PATH_MAX, sizeof(char));
	
    snprintf(Filename, PATH_MAX, "%s/%s", area.Path, File);

    if ((fdb_area = mbsedb_OpenFDB(AreaNum, 30))) {
	/*
	 * Do a physical check of file to see if it exists
	 * if it fails it will return a zero which will not
	 * increase his uploads stats
	 */
	if (stat(Filename, &statfile) != 0) {

	    Enter(1);
	    colour(10, 0);
	    /* Upload was unsuccessful for: */
	    snprintf(msg, 81, "%s%s", (char *) Language(284), File);
	    pout(LIGHTGREEN, BLACK, msg);
	    Enter(2);

	    mbsedb_CloseFDB(fdb_area);
	    free(Filename);
	    free(temp1);
	    free(lname);
	    return FALSE;
	}

	memset(&fdb, 0, fdbhdr.recsize);
	strncpy(fdb.LName, File, 80); /* LFN, currently real file */
	strcpy(temp1, File);
	name_mangle(temp1);
	strncpy(fdb.Name, temp1, 12); /* 8.3 name */
	fdb.Size = (int)(statfile.st_size);
	fdb.FileDate = statfile.st_mtime;
	fdb.Crc32 = file_crc(Filename, TRUE);
	strncpy(fdb.Uploader, exitinfo.sUserName, 35);
	fdb.UploadDate = time(NULL);

	/*
	 * Create the symlink is done in the real directory
	 */
	if (getcwd(lname, PATH_MAX-1)) {
	    chdir(area.Path);
	    if (strcmp(fdb.Name, fdb.LName)) {
	    	/*
	    	 * Rename the file first to the 8.3 name, this is the
	    	 * standard way to store files in the filebase.
	    	 */
	    	rename(fdb.LName, fdb.Name);
	    	/*
	    	 * Then make a symlink to the 8.3 name
	    	 */
	    	if (symlink(fdb.Name, fdb.LName)) {
		    WriteError("$Can't create link %s to %s", fdb.Name, fdb.LName);
	    	}
	    }
	    chdir(lname);
	}
	free(lname);

	if (area.PwdUP) {
	    Enter(1);
	    /* Do you want to password protect your upload ? [y/N]: */
	    pout(LIGHTBLUE, BLACK, (char *) Language(285));

	    if (toupper(Readkey()) == Keystroke(285, 0)) {
		Enter(1);
		/* REMEMBER: Passwords are "CaSe SeNsITiVe!" */
		pout(LIGHTGREEN, BLACK, (char *) Language(286));
		Enter(1);
		/* Password: */
		pout(YELLOW, BLACK, (char *) Language(8));
		GetstrC(fdb.Password, 20);
	    }
	}

	if (fileid && strlen(archiver.iunarc)) {
	    /*
	     * During Virus scan, a possible FILE_ID.DIZ is in the users work directory.
	     */
	    idname = xstrcpy(CFG.bbs_usersdir);
	    idname = xstrcat(idname, (char *)"/");
	    idname = xstrcat(idname, exitinfo.Name);
	    idname = xstrcat(idname, (char *)"/wrk/FILE_ID.DIZ");

	    if (file_exist(idname, R_OK) == 0) {
		Syslog('+', "Found %s", idname);
		GotId = TRUE;
	    }
	}

	if (GotId) {
	    lines = 0;
	    if ((id = fopen(idname, "r")) != NULL) {
		/*
		 * Import FILE_ID.DIZ, format to max. 25 * lines, 48 chars width.
		 */
		while (((fgets(temp1, PATH_MAX -1, id)) != NULL) && (lines < 25)) {
		    Striplf(temp1);
		    if (strlen(temp1) > 51) {
			/*
			 * Malformed FILE_ID.DIZ
			 */
			GotId = FALSE;
			for (i = 0; i < 25; i++)
			    fdb.Desc[i][0] = '\0';
			lines = 0;
			Syslog('!', "Trashing illegal formatted FILE_ID.DIZ");
			break;
		    }
		    if (strlen(temp1) > 0) {
			j = 0;
			for (i = 0; i < strlen(temp1); i++) {
			    if (isprint(temp1[i])) {
				fdb.Desc[lines][j] = temp1[i];
				j++;
				if (j > 47)
				    break;
			    }
			}

			/*
			 * Remove trailing spaces
			 */
			while (j && isspace(fdb.Desc[lines][j-1]))
			    j--;
			fdb.Desc[lines][j] = '\0';
			lines++;
		    }
		}
		fclose(id);
	    } else {
		GotId = FALSE;
	    }
	    unlink(idname);

	    if (GotId) {
		/*
		 * Strip empty FILE_ID.DIZ lines at the end
		 */
		while ((strlen(fdb.Desc[lines-1]) == 0) && (lines)) {
		    fdb.Desc[lines-1][0] = '\0';
		    lines--;
		}
		if (lines) {
		    Syslog('+', "Using %d FILE_ID.DIZ lines for description", lines);
		    /* Found FILE_ID.DIZ in */
		    snprintf(msg, 81, "%s %s", (char *) Language(257), File);
		    pout(CFG.TextColourF, CFG.TextColourB, msg);
		    Enter(1);
		} else {
		    Syslog('!', "No FILE_ID.DIZ lines left to use");
		    GotId = FALSE;
		}
	    }
	} 
	
	if (!GotId) {
	    /*
	     * Ask the user for a description.
	     */
	    for (i = 0; i < 26; i++)
		*(Desc + i) = (char *) calloc(49, sizeof(char));

	    Enter(1);
	    /* Please enter description of file */
	    snprintf(msg, 81, "%s %s", (char *) Language(287), File);
	    pout(LIGHTRED, BLACK, msg);
	    Enter(2);

	    while (TRUE) {
		snprintf(msg, 81, "%2d> ", iDesc);
		pout(LIGHTGREEN, BLACK, msg);
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(*(Desc + iDesc), 47);

		if ((strcmp(*(Desc + iDesc), "")) == 0) 
		    break;

		iDesc++;

		if (iDesc >= 26)
		    break;
	    }

	    for (i = 1; i < iDesc; i++)
		strcpy(fdb.Desc[i - 1], Desc[i]);

	    for (i = 0; i < 26; i++)
		free(Desc[i]);
	}


	/*
	 * Log upload before adding to the filebase, after insert the fdb record
	 * is overwritten.
	 */
	snprintf(temp, PATH_MAX, "%s/log/uploads.log", getenv("MBSE_ROOT"));
	if ((pPrivate = fopen(temp, "a+")) == NULL)
	    WriteError("$Can't open %s", temp);
	else {
	    fprintf(pPrivate, "****************************************************");
	    fprintf(pPrivate, "\nUser        : %s", fdb.Uploader);
	    fprintf(pPrivate, "\nFile        : %s (%s)", fdb.LName, fdb.Name);
	    fprintf(pPrivate, "\nSize        : %u", (int)(fdb.Size));
	    fprintf(pPrivate, "\nUpload Date : %s\n\n", StrDateDMY(fdb.UploadDate));
				
	    for (i = 0; i < iDesc - 1; i++)
		fprintf(pPrivate, "%2d: %s\n", i, fdb.Desc[i]);

	    fclose(pPrivate);
	}

	mbsedb_InsertFDB(fdb_area, fdb, area.AddAlpha);
	mbsedb_CloseFDB(fdb_area);

	Enter(1);
	/* Your upload time has been returned to you. Thank you for your upload! */
	pout(LIGHTGREEN, BLACK, (char *) Language(288));
	Enter(1);
    }

    free(Filename);
    free(temp1);
    return TRUE;
}



/*
 * Set file area number, set global area description and path.
 */
void SetFileArea(unsigned int AreaNum)
{
    FILE    *pArea;
    int	    offset;

    memset(&area, 0, sizeof(area));

    if ((pArea = OpenFareas(FALSE)) == NULL)
	return;

    offset = areahdr.hdrsize + ((AreaNum - 1) * areahdr.recsize);
    if (fseek(pArea, offset, 0) != 0) {
	WriteError("$Seek error in fareas.data, area %ld", AreaNum);
	return;
    }

    fread(&area, areahdr.recsize, 1, pArea);
    strcpy(sAreaDesc, area.Name);
    strcpy(sAreaPath, area.Path);
    iAreaNumber = AreaNum;
    fclose(pArea);
}



/*
 * Return size in bytes of all files in the users wrk directory.
 */
unsigned int Quota()
{
    DIR		    *dirp;
    char	    *FileName, *temp;
    unsigned int    Bytes = 0;
    struct dirent   *dp;
    struct stat	    statfile;

    FileName = calloc(PATH_MAX, sizeof(char));
    temp     = calloc(PATH_MAX, sizeof(char));

    snprintf(temp, PATH_MAX, "%s/%s/wrk", CFG.bbs_usersdir, exitinfo.Name);

    if ((dirp = opendir(temp)) == NULL) {
	WriteError("$Can't open dir %s", temp);
    } else {
	while ((dp = readdir(dirp)) != NULL) {
	    snprintf(FileName, PATH_MAX, "%s/%s", temp, dp->d_name);

	    if (*(dp->d_name) != '.')
		if (stat(FileName, &statfile) == 0)
		    Bytes += statfile.st_size;
	}

	closedir(dirp);
    }

    free(FileName);
    free(temp);
    return Bytes;
}



void ImportHome(char *fn)
{
    char    *temp1, *temp2;

    temp1 = calloc(PATH_MAX, sizeof(char));
    temp2 = calloc(PATH_MAX, sizeof(char));
    snprintf(temp1, PATH_MAX, "%s/%s/wrk/%s", CFG.bbs_usersdir, exitinfo.Name, fn);
    snprintf(temp2, PATH_MAX, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, fn);

    Syslog('+', "Move %s to home, result %d", fn, file_mv(temp2, temp1));
    free(temp1);
    free(temp2);
}


