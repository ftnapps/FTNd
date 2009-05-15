/*****************************************************************************
 *
 * $Id: mbuser.c,v 1.5 2007/02/11 13:19:37 mbse Exp $
 * Purpose ...............: User Pack Util
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "mbuser.h"


extern	int e_pid;			/* External pid			     */
extern	int do_quiet;			/* Quiet flag			     */
time_t	t_start;			/* Start time			     */
time_t	t_end;				/* End time			     */
int	Days, Level;			/* Kill days and up to level	     */
struct	userhdr	usrhdr;			/* Database header		     */
struct	userrec	usr;			/* Database record		     */
mode_t	oldmask;			/* Old umask value		     */


int main(int argc, char **argv)
{
    int		    i, pack = FALSE;
    char	    *cmd;
    struct passwd   *pw;

    InitConfig();
    mbse_TermInit(1, 80, 24);
    Days = 0;
    Level = 0;

    t_start = time(NULL);

    if (argc < 2)
	Help();

    cmd = xstrcpy((char *)"Command line:");

    for (i = 1; i < argc; i++) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, tl(argv[i]));

	if (strncasecmp(tl(argv[i]), "-q", 2) == 0)
	    do_quiet = TRUE;
	if (strncasecmp(tl(argv[i]), "p", 1) == 0)
	    pack = TRUE;
	if (strncasecmp(tl(argv[i]), "k", 1) == 0) {
	    if (argc  <= (i + 2))
		Help();
	    i++;
	    cmd = xstrcat(cmd, (char *)" ");
	    cmd = xstrcat(cmd, argv[i]);
	    Days = atoi(argv[i]);
	    i++;
	    cmd = xstrcat(cmd, (char *)" ");
	    cmd = xstrcat(cmd, argv[i]);
	    Level = atoi(argv[i]);

	    if ((Days == 0) || (Level == 0))
		Help();
	}
    }

    if ((Days + Level + pack) == 0)
	Help();

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbuser", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', " ");
    Syslog(' ', "MBUSER v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (enoughspace(CFG.freespace) == 0)
	ExitClient(MBERR_DISK_FULL);

    if (lockprogram((char *)"mbuser")) {
	if (!do_quiet)
	    printf("Can't lock mbuser, abort.\n");
	ExitClient(MBERR_NO_PROGLOCK);
    }

    oldmask = umask(027);
    if (!do_quiet)
	mbse_colour(CYAN, BLACK);
    UserPack(Days, Level, pack);
    umask(oldmask);

    ulockprogram((char *)"mbuser");
    t_end = time(NULL);
    Syslog(' ', "MBUSER finished in %s", t_elapsed(t_start, t_end));

    if (!do_quiet)
	mbse_colour(LIGHTGRAY, BLACK);
    ExitClient(MBERR_OK);
    return 0;
}



/*
 * Program header
 */
void ProgName(void)
{
    if (do_quiet)
	return;

    mbse_colour(WHITE, BLACK);
    printf("\nMBUSER: MBSE BBS %s - User maintenance utility\n", VERSION);
    mbse_colour(YELLOW, BLACK);
    printf("        %s\n\n", COPYRIGHT);
    mbse_colour(LIGHTGRAY, BLACK);
}



void Help(void)
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("\nUsage:	mbuser [commands] <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	kill [n] [l]	Kill users not called in \"n\" days below level \"l\"\n");
    printf("	pack		Pack the userbase\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("\n	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-quiet		Quiet mode, (no screen output)\n\n");

    mbse_colour(LIGHTGRAY, BLACK);
    printf("\n");
    ExitClient(MBERR_COMMANDLINE);
}



/*
 * Userpack routine
 */
void UserPack(int days, int level, int pack)
{
    FILE    *fin, *fout;
    char    *fnin, *fnout, *cmd;
    int	    oldsize, curpos;
    int	    updated, delete = 0, rc, highest = 0, record = 0, sysop = FALSE;
    time_t  Last;

    fnin  = calloc(PATH_MAX, sizeof(char));
    fnout = calloc(PATH_MAX, sizeof(char));
    snprintf(fnin,  PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/users.temp", getenv("MBSE_ROOT"));

    /*
     * First copy the users database, all packing will be done
     * on a the copy.
     */
    if ((fin = fopen(fnin, "r")) == NULL) {
	WriteError("$Can't open %s", fnin);
	free(fnin);
	free(fnout);
	return;
    }
    if ((fout = fopen(fnout, "w+")) == NULL) {
	WriteError("$Can't create %s", fnout);
	fclose(fin);
        free(fnin);
        free(fnout);
	return;
    }
    fread(&usrhdr, sizeof(usrhdr), 1, fin);
    oldsize = usrhdr.recsize;
    updated = FALSE;

    /*
     * First count records and blanks at the end. Check if the sysop name
     * in the main configuration exists in the userdatabase.
     */
    while (fread(&usr, oldsize, 1,fin) == 1) {
	delete++;
	if (!usr.Deleted && strlen(usr.sUserName)) {
	    highest = (ftell(fin) / oldsize);
	    if (!strcmp(usr.sUserName, CFG.sysop_name) && !strcmp(usr.Name, CFG.sysop))
		sysop = TRUE;
	}
    }
    if (highest != delete) {
	Syslog('+', "Blank records at the end, truncating userbase");
	updated = TRUE;
    }
    if (!sysop)
	WriteError("No valid Sysop Fidoname and/or Unixname found in userbase, check setup!");

    fseek(fin, usrhdr.hdrsize, SEEK_SET);

    if (oldsize != sizeof(usr)) {
	updated = TRUE;
	Syslog('+', "Userbase recordsize is changed, making update");
    }

    usrhdr.hdrsize = sizeof(usrhdr);
    usrhdr.recsize = sizeof(usr);
    fwrite(&usrhdr, sizeof(usrhdr), 1, fout);

    /*
     * The datarecord is filled with zero's before each read
     * so that if the record format changed, the new fields will
     * be empty by default. The blank records at the end of the
     * database are dropped.
     */
    memset(&usr, 0, sizeof(usr));
    while (fread(&usr, oldsize, 1,fin) == 1) {
	record++;
	fwrite(&usr, sizeof(usr), 1, fout);
	memset(&usr, 0, sizeof(usr));
	if (CFG.slow_util && do_quiet)
	    msleep(1);
	Nopper();
    }
    fclose(fin);
    delete = 0;

    /*
     * Handle packing for days below level
     */
    if (days && level) {
	fseek(fout, sizeof(usrhdr), SEEK_SET);
	curpos = sizeof(usrhdr);

	while (fread(&usr, sizeof(usr), 1, fout)  == 1) {
	    /*
	     * New users don't have the last login date set yet,
	     * use the registration date instead.
	     */
	    if (usr.iTotalCalls == 0)
		Last = usr.tFirstLoginDate;
	    else
		Last = usr.tLastLoginDate;

	    /*
	     * Wow, killing on the second exact!. Don't kill the guest accounts.
	     */
	    if ((((t_start - Last) / 86400) > days) && (usr.Security.level < level) && (!usr.Guest) &&
			    (usr.sUserName[0] != '\0') && (!usr.NeverDelete)) {
		Syslog('+', "Mark user %s", usr.sUserName);
		if (!do_quiet) {
		    printf("Mark user %s\n", usr.sUserName);
		    fflush(stdout);
		}
		delete++;
		updated = TRUE;
		fseek(fout, - sizeof(usr), SEEK_CUR);
		/*
		 * Just mark for deletion
		 */
		usr.Deleted = TRUE;
		fwrite(&usr, sizeof(usr), 1, fout);
	    }
	    if (CFG.slow_util && do_quiet)
		msleep(1);
	}
	Syslog('+', "Marked %d users to delete", delete);
    }

    /*
     * Pack the userbase if told so
     */
    if (pack) {
	Syslog('+', "Packing userbase");
	delete = 0;
	fseek(fout, sizeof(usrhdr), SEEK_SET);
	while (fread(&usr, sizeof(usr), 1, fout) == 1) {
	    if (CFG.slow_util && do_quiet)
		msleep(1);

	    Nopper();
	    if (usr.Deleted) {
		if (!do_quiet) {
		    printf("Delete user %s\n", usr.Name);
		    fflush(stdout);
		}
		if (usr.Name[0] != '\0') {
		    if ((setuid(0) == -1) || (setgid(0) == -1)) {
			WriteError("Cannot setuid(root) or setgid(root)");
			WriteError("Cannot delete unix account %s", usr.Name);
		    } else {
#ifndef __FreeBSD__
			rc = execute_str((char *)"/usr/sbin/userdel ", usr.Name, NULL,
							(char *)"/dev/null",(char *)"/dev/null",(char *)"/dev/null");
#else
			rc = execute_str((char *)"/usr/sbin/pw userdel ", usr.Name, NULL,
							(char *)"/dev/null",(char *)"/dev/null",(char *)"/dev/null");
#endif
#ifdef _VPOPMAIL_PATH
			cmd = xstrcpy((char *)_VPOPMAIL_PATH);
			cmd = xstrcat(cmd, (char *)"/vdeluser ");
			rc = execute_str(cmd, usr.Name, NULL, (char *)"/dev/null",(char *)"/dev/null",(char *)"/dev/null");
			free(cmd);
#endif
			if (chdir(CFG.bbs_usersdir) == 0) {
			    cmd = xstrcpy((char *)"-Rf ");
			    cmd = xstrcat(cmd, usr.Name);
			    rc = execute_pth((char *)"rm", cmd, (char *)"/dev/null",(char *)"/dev/null",(char *)"/dev/null");
			    free(cmd);
			}
		    }
		}

		fseek(fout, - sizeof(usr), SEEK_CUR);
		/*
		 * Blank the deleted records for reuse.
		 */
		memset(&usr, 0, sizeof(usr));
		if (strlen(CFG.externaleditor))
		    usr.MsgEditor = EXTEDIT;
		else
		    usr.MsgEditor = FSEDIT;
		fwrite(&usr, sizeof(usr), 1, fout);
		delete++;
		updated = TRUE;
	    }
	}
	Syslog('+', "Deleted %d records", delete);
    }

    if (updated) {
	/*
	 *  Copy file back to the original file, truncate any
	 *  deleted records at the end.
	 */
	fseek(fout, 0, SEEK_SET);
	if ((fin = fopen(fnin, "w")) == NULL) {
	    WriteError("$Can't open %s", fnin);
	    free(fnin);
	    free(fnout);
	    return;
	}
	fread(&usrhdr, sizeof(usrhdr), 1, fout);
	fwrite(&usrhdr, sizeof(usrhdr), 1, fin);
	record = 0;

	while (fread(&usr, sizeof(usr), 1,fout) == 1) {
	    Nopper();
	    record++;
	    fwrite(&usr, sizeof(usr), 1, fin);
	    if (record >= highest)
		break;
	}
	fclose(fin);
	fclose(fout);
	chmod(fnin, 0660);
	Syslog('+', "Userbase is updated, written %d records", record);
    } else {
	fclose(fout);
    }

    unlink(fnout);
    free(fnin);
    free(fnout);
}



