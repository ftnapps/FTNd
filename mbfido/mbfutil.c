/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - utilities
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "mbfutil.h"
#include "mbfile.h"



extern int	do_quiet;		/* Suppress screen output	    */
extern int	do_force;		/* Force file overwrite		    */
extern	int	e_pid;			/* Pid of external process	    */
extern time_t	t_start;		/* Start time			    */
extern time_t	t_end;			/* End time			    */
int		marker = 0;		/* Marker counter		    */



void ProgName(void)
{
	if (do_quiet)
		return;

	mbse_colour(WHITE, BLACK);
	printf("\nMBFILE: MBSE BBS %s File maintenance utility\n", VERSION);
	mbse_colour(YELLOW, BLACK);
	printf("        %s\n", COPYRIGHT);
}



void die(int onsig)
{
    /*
     * First check if a child is running, if so, kill it.
     */
    if (e_pid) {
	if ((kill(e_pid, SIGTERM)) == 0)
	    Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
	else {
	    if ((kill(e_pid, SIGKILL)) == 0)
		Syslog('+', "SIGKILL to pid %d succeded", e_pid);
	    else
		WriteError("$Failed to kill pid %d", e_pid);
	}

	/*
	 * In case the child had the tty in raw mode...
	 */
	if (!do_quiet)
	    execute_pth((char *)"stty", (char *)"sane", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    }

    signal(onsig, SIG_IGN);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    ulockprogram((char *)"mbfile");
    t_end = time(NULL);
    Syslog(' ', "MBFILE finished in %s", t_elapsed(t_start, t_end));

    if (!do_quiet) {
	mbse_colour(LIGHTGRAY, BLACK);
	fflush(stdout);
    }
    ExitClient(onsig);
}



void Help(void)
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("Usage:	mbfile [command] <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	a  adopt <area> <file> [desc]	Adopt file to area\n");
    printf("	c  check [area]			Check filebase\n");
    printf("	d  delete <area> \"<filemask>\"	Mark file(s) in area for deletion\n");
    printf("	im import <area>		Import files in current dir to area\n");
    printf("	in index			Create filerequest index\n");
    printf("	k  kill				Kill/move old files\n");
    printf("	l  list	[area]			List file areas or one area\n");
    printf("	m  move <from> <to> <file>	Move file from to area\n");
    printf("	p  pack				Pack filebase\n");
    printf("	r  rearc <area> \"<filemask>\"	Rearc file(s) in area to area archive type\n");
    printf("	s  sort <area>			Sort files in a file area\n");
    printf("	t  toberep			Show toberep database\n");
    printf("	u  undelete <area> \"<filemask>\"	Mark file(s) in area for undeletion\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("\n	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-a -announce			Suppress announce added files\n");
    printf("	-f -force			Force file overwrite\n");
    printf("	-q -quiet			Quiet mode\n");
    printf("	-v -virus			Suppress virus scanning, use with care\n");
    die(MBERR_COMMANDLINE);
}



void Marker(void)
{
	/*
	 * Keep the connection with the server alive
	 */
	Nopper();

	/*
	 * Release system resources when running in the background
	 */
	if (CFG.slow_util && do_quiet)
		msleep(1);

	if (do_quiet)
		return;

	switch (marker) {
		case 0:	printf(">---");
			break;

		case 1:	printf(">>--");
			break;

		case 2:	printf(">>>-");
			break;

		case 3:	printf(">>>>");
			break;

		case 4:	printf("<>>>");
			break;

		case 5:	printf("<<>>");
			break;

		case 6:	printf("<<<>");
			break;

		case 7:	printf("<<<<");
			break;

		case 8: printf("-<<<");
			break;

		case 9: printf("--<<");
			break;

		case 10:printf("---<");
			break;

		case 11:printf("----");
			break;
	}
	printf("\b\b\b\b");
	fflush(stdout);

	if (marker < 11)
		marker++;
	else
		marker = 0;
}



void DeleteVirusWork()
{
        char    *buf, *temp;

        buf  = calloc(PATH_MAX, sizeof(char));
        temp = calloc(PATH_MAX, sizeof(char));
        getcwd(buf, PATH_MAX);
        sprintf(temp, "%s/tmp", getenv("MBSE_ROOT"));

        if (chdir(temp) == 0) {
                Syslog('f', "DeleteVirusWork %s/arc", temp);
                execute_pth((char *)"rm", (char *)"-r -f arc", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
		execute_pth((char *)"mkdir", (char *)"arc", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
        } else
                WriteError("$Can't chdir to %s", temp);

        chdir(buf);
        free(temp);
        free(buf);
}



int UnpackFile(char *File)
{
    char    *temp, *pwd, *unarc, *cmd;

    Syslog('f', "UnpackFile(%s)", File);

    if ((unarc = unpacker(File)) == NULL) {
	Syslog('f', "Unknown archive format %s", File);
	return FALSE;
    }

    temp = calloc(PATH_MAX, sizeof(char));
    pwd  = calloc(PATH_MAX, sizeof(char));
    getcwd(pwd, PATH_MAX);

    /*
     * Check if there is a temp directory to unpack the archive.
     */
    sprintf(temp, "%s/tmp/arc", getenv("MBSE_ROOT"));
    if ((access(temp, R_OK)) != 0) {
	if (mkdir(temp, 0777)) {
	    WriteError("$Can't create %s", temp);
	    if (!do_quiet)
		printf("\nCan't create %s\n", temp);
	    die(MBERR_GENERAL);
	}
    }

    /*
     * Check for stale FILE_ID.DIZ files
     */
    sprintf(temp, "%s/tmp/arc/FILE_ID.DIZ", getenv("MBSE_ROOT"));
    if (!unlink(temp))
	Syslog('+', "Removed stale %s", temp);
    sprintf(temp, "%s/tmp/arc/file_id.diz", getenv("MBSE_ROOT"));
    if (!unlink(temp))
	Syslog('+', "Removed stale %s", temp);

    if (!getarchiver(unarc)) {
	WriteError("No archiver available for %s", File);
	if (!do_quiet)
	    printf("\nNo archiver available for %s\n", File);
	return FALSE;
    }

    cmd = xstrcpy(archiver.funarc);
    if ((cmd == NULL) || (cmd == "")) {
	WriteError("No unarc command available");
	if (!do_quiet)
	    printf("\nNo unarc command available\n");
	return FALSE;
    }

    sprintf(temp, "%s/tmp/arc", getenv("MBSE_ROOT"));
    if (chdir(temp) != 0) {
	WriteError("$Can't change to %s", temp);
	die(MBERR_GENERAL);
    }

    if (execute_str(cmd, File, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
	chdir(pwd);
	free(temp);
	free(pwd);
	free(cmd);
	return TRUE;
    } else {
	chdir(pwd);
	WriteError("Unpack error, file may be corrupt");
	DeleteVirusWork();
    }
    return FALSE;
}



/*
 * Add file to the BBS. The file is in the current directory. 
 * The f_db record already has all needed information.
 */
int AddFile(struct FILE_record f_db, int Area, char *DestPath, char *FromPath, char *LinkPath)
{
    int	    rc;
    struct _fdbarea *fdb_area = NULL;

    Syslog('f', "AddFile Area    : %d", Area);
    Syslog('f', "AddFile DestPath: %s", MBSE_SS(DestPath));
    Syslog('f', "AddFile FromPath: %s", MBSE_SS(FromPath));
    Syslog('f', "AddFile LinkPath: %s", MBSE_SS(LinkPath));

    /*
     * Copy file to the final destination and make a hard link with the
     * 8.3 filename to the long filename.
     */
    mkdirs(DestPath, 0775);

    if ((file_exist(DestPath, F_OK) == 0) || (file_exist(LinkPath, F_OK) == 0)) {
	if (do_force) {
	    Syslog('+', "File %s (%s) already exists in area %d, forced overwrite", f_db.Name, f_db.LName, Area);
	} else {
	    WriteError("File %s (%s) already exists in area %d", f_db.Name, f_db.LName, Area);
	    if (!do_quiet)
		printf("\nFile %s (%s) already exists in area %d\n", f_db.Name, f_db.LName, Area);
	    return FALSE;
	}
    }

    if ((rc = file_cp(FromPath, DestPath))) {
	WriteError("Can't copy file to %s, %s", DestPath, strerror(rc));
	if (!do_quiet)
	    printf("\nCan't copy file to %s, %s\n", DestPath, strerror(rc));
	return FALSE;
    }
    chmod(DestPath, 0644);
    if (LinkPath) {
	if ((rc = symlink(DestPath, LinkPath))) {
	    WriteError("Can't create symbolic link %s", LinkPath);
	    if (!do_quiet)
		printf("\nCan't create symbolic link %s, %s\n", LinkPath, strerror(rc));
	    unlink(DestPath);
	    return FALSE;
	}
    }

    if ((fdb_area = mbsedb_OpenFDB(Area, 30))) {
	rc = mbsedb_InsertFDB(fdb_area, f_db, TRUE);
	mbsedb_CloseFDB(fdb_area);
	return rc;
    } else {
	return FALSE;
    }
}



int CheckFDB(int Area, char *Path)
{
    char    *temp;
    FILE    *fp;
    int	    rc = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/var/fdb/file%d.data", getenv("MBSE_ROOT"), Area);

    /*
     * Open the file database, create new one if it doesn't excist.
     */
    if ((fp = fopen(temp, "r+")) == NULL) {
	Syslog('!', "Creating new %s", temp);
	if ((fp = fopen(temp, "a+")) == NULL) {
	    WriteError("$Can't create %s", temp);
	    rc = TRUE;
	} else {
	    fdbhdr.hdrsize = sizeof(fdbhdr);
	    fdbhdr.recsize = sizeof(fdb);
	    fwrite(&fdbhdr, sizeof(fdbhdr), 1, fp);
	    fclose(fp);
	}
    } else {
	fread(&fdbhdr, sizeof(fdbhdr), 1, fp);
	fclose(fp);
    }

    /*
     * Set the right attributes
     */
    chmod(temp, 0660);

    /*
     * Now check the download directory
     */
    if (access(Path, W_OK) == -1) {
	sprintf(temp, "%s/foobar", Path);
	if (mkdirs(temp, 0775))
	    Syslog('+', "Created directory %s", Path);
    }

    free(temp);

    return rc;
}



/*
 * Load Area Record
 */
int LoadAreaRec(int Area)
{
    FILE    *pAreas;
    char    *sAreas;

    sAreas = calloc(PATH_MAX, sizeof(char));

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
        WriteError("$Can't open %s", sAreas);
        if (!do_quiet)
            printf("\nCan't open %s\n", sAreas);
        return FALSE;
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    if (fseek(pAreas, ((Area - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET)) {
        WriteError("$Can't seek record %d in %s", Area, sAreas);
        if (!do_quiet)
            printf("\nCan't seek record %d in %s\n", Area, sAreas);
        fclose(pAreas);
        free(sAreas);
        return FALSE;
    }

    if (fread(&area, areahdr.recsize, 1, pAreas) != 1) {
        WriteError("$Can't read record %d in %s", Area, sAreas);
        if (!do_quiet)
            printf("\nCan't read record %d in %s\n", Area, sAreas);
        fclose(pAreas);
        free(sAreas);
        return FALSE;
    }

    fclose(pAreas);
    free(sAreas);
    return TRUE;
}



int is_real_8_3(char *File)
{
    int	    i;

    if (! is_8_3(File))
	return FALSE;
    for (i = 0; i < strlen(File); i++)
	if (isalpha(File[i]) && islower(File[i]))
	    return FALSE;
    return TRUE;
}

