/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Nodelist diff processor
 * Original ideas ........: Eugene G. Crosser.
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "mbdiff.h"



#ifndef BLKSIZ
#define BLKSIZ 512
#endif

extern unsigned short crc16xmodemtab[];
#define updcrc(cp, crc) ( crc16xmodemtab[((crc >> 8) & 255) ^ cp] ^ (crc << 8))

extern int	show_log;
extern int	e_pid;
extern int	do_quiet;		/* Supress screen output	    */
extern int	show_log;		/* Show logging			    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */



void ProgName(void)
{
	if (do_quiet)
		return;

	colour(15, 0);
	printf("\nMBDIFF: MBSE BBS %s Nodelist diff processor\n", VERSION);
	colour(14, 0);
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
		system("stty sane");
	}

	signal(onsig, SIG_IGN);

	if (onsig) {
		if (onsig <= NSIG)
			WriteError("$Terminated on signal %d (%s)", onsig, SigName[onsig]);
		else
			WriteError("Terminated with error %d", onsig);
	}

	t_end = time(NULL);
	Syslog(' ', "MBDIFF finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet) {
		colour(7, 0);
		printf("\n");
	}
	ExitClient(onsig);
}



int main(int argc, char **argv)
{
	int	i, Match;
	char	*cmd, *nl = NULL, *nd = NULL, *nn;
	int	rc;
	char	*p, *q, *arc;
	struct	passwd *pw;
	char	*wrk, *onl, *ond;
	DIR	*dp;
	struct	dirent *de;

#ifdef MEMWATCH
        mwInit();
#endif
	InitConfig();
	TermInit(1);
	t_start = time(NULL);
	umask(002);

	/*
	 * Catch all signals we can, and ignore the rest.
	 */
	for (i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) ||
		    (i == SIGILL) || (i == SIGSEGV) || (i == SIGKILL))
			signal(i, (void (*))die);
		else
			signal(i, SIG_IGN);
	}

	if(argc < 3)
		Help();

	cmd = xstrcpy((char *)"Cmd: mbdiff");

	for (i = 1; i < argc; i++) {

		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);

		if (i == 1)
			if ((nl = argv[i]) == NULL)
				Help();
		if (i == 2)
			if ((nd = argv[i]) == NULL)
				Help();
		if (!strncasecmp(argv[i], "-q", 2))
			do_quiet = TRUE;

	}

	ProgName();
	pw = getpwuid(getuid());
	InitClient(pw->pw_name, (char *)"mbdiff", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);

	Syslog(' ', " ");
	Syslog(' ', "MBDIFF v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet) {
		colour(12, 0);
		printf("\n");
	}

	if (!diskfree(CFG.freespace))
		die(101);

	/*
	 *  Extract work directory from the first commandline parameter
	 *  and set that directory as default.
	 */
	show_log = TRUE;
	wrk = xstrcpy(nl);
	if (strrchr(wrk, '/') == NULL) {
		WriteError("No path in nodelist name");
		free(wrk);
		die(100);
	}
	if (strrchr(wrk, '.') != NULL) {
		WriteError("Filename extension given for nodelist");
		free(wrk);
		die(100);
	}
	if (strrchr(nd, '/') == NULL) {
		WriteError("No path in nodediff name");
		free(wrk);
		die(100);
	}
	show_log = FALSE;

	while (wrk[strlen(wrk) -1] != '/')
		wrk[strlen(wrk) -1] = '\0';
	wrk[strlen(wrk) -1] = '\0';

	show_log = TRUE;
	if (access(wrk, R_OK|W_OK)) {
		WriteError("$No R/W access in %s", wrk);
		free(wrk);
		die(100);
	}

	if (chdir(wrk)) {
		WriteError("$Can't chdir to %s", wrk);
		free(wrk);
		die(100);
	}
	show_log = FALSE;

	onl = xstrcpy(strrchr(nl, '/') + 1);
	onl = xstrcat(onl, (char *)".???");

	if ((dp = opendir(wrk)) == 0) {
		show_log = TRUE;
		free(wrk);
		WriteError("$Error opening directory %s", wrk);
		die(100);
	}

	Match = FALSE;
	while ((de = readdir(dp))) {
		if (strlen(de->d_name) == strlen(onl)) {
			Match = TRUE;
			for (i = 0; i < strlen(onl); i++) {
				if ((onl[i] != '?') && (onl[i] != de->d_name[i]))
					Match = FALSE;
			}
			if (Match) {
				free(onl);
				onl = xstrcpy(de->d_name);
				break;
			}
		}
	}
	closedir(dp);
	if (!Match) {
		show_log = TRUE;
		free(wrk);
		free(onl);
		WriteError("Old nodelist not found");
		die(100);
	}

	/*
	 *  Now try to get the diff file into the workdir.
	 */
	if ((arc = unpacker(nd)) == NULL) {
		show_log = TRUE;
		free(onl);
		free(wrk);
		WriteError("Can't get filetype for %s", nd);
		die(100);
	}

	ond = xstrcpy(strrchr(nd, '/') + 1);

	if (strncmp(arc, "ASC", 3)) {
		if (!getarchiver(arc)) {
			show_log = TRUE;
			free(onl);
			free(wrk);
			free(ond);
			WriteError("Can't find unarchiver %s", arc);
			die(100);
		}

		/*
		 * We may both use the unarchive command for files and mail,
		 * unarchiving isn't recursive anyway.
		 */
		if (strlen(archiver.funarc))
			cmd = xstrcpy(archiver.funarc);
		else
			cmd = xstrcpy(archiver.munarc);

		if ((cmd == NULL) || (cmd == "")) {
			show_log = TRUE;
			free(cmd);
			free(onl);
			free(wrk);
			free(ond);
			WriteError("No unarc command available for %s", arc);
			die(100);
		}

		if (execute(cmd, nd, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null")) {
			show_log = TRUE;
			free(cmd);
			free(onl);
			free(wrk);
			free(ond);
			WriteError("Unpack error");
			die(100);
		}
		free(cmd);

		Match = FALSE;
		if ((dp = opendir(wrk)) != NULL) {
			while ((de = readdir(dp))) {
				if (strlen(ond) == strlen(de->d_name)) {
					Match = TRUE;
					for (i = 0; i < (strlen(ond) -3); i++)
						if (toupper(ond[i]) != toupper(de->d_name[i]))
							Match = FALSE;
					if (Match) {
						free(ond);
						ond = xstrcpy(de->d_name);
						break;
					}
				}
			}
			closedir(dp);
		}
		if (!Match) {
			show_log = TRUE;
			free(ond);
			free(onl);
			free(wrk);
			WriteError("Could not find extracted file");
			die(100);
		}
	} else {
		if (file_cp(nd, ond)) {
			show_log = TRUE;
			free(ond);
			free(onl);
			free(wrk);
			WriteError("$Copy %s failed", nd);
			die(100);
		}
		Syslog('s', "Copied %s", nd);
	}

	if (((p = strrchr(onl, '.'))) && ((q = strrchr(ond, '.'))) &&
	    (strlen(p) == strlen(q))) {
		nn = xstrcpy(onl);
		p = strrchr(nn, '.') + 1;
		q++;
		strcpy(p, q);
	} else 
		nn = xstrcpy((char *)"newnodelist");

	if (strcmp(onl, nn) == 0) {
		show_log = TRUE;
		WriteError("Attempt to update nodelist to the same version");
		unlink(ond);
		free(ond);
		free(onl);
		free(wrk);
		free(nn);
		die(100);
	}

	Syslog('+', "Apply %s with %s to %s", onl, ond, nn);
	if (!do_quiet) {
		colour(3, 0);
		printf("Apply %s with %s to %s\n", onl, ond, nn);
	}
	rc = apply(onl, ond, nn);

	unlink(ond);
	if (rc) {
		unlink(nn);
		free(nn);
		free(ond);
		free(onl);
		free(wrk);
		die(rc + 100);
	} else {
		unlink(onl);
		cmd = xstrcpy(archiver.farc);

		if ((cmd == NULL) || (!strlen(cmd))) {
			free(cmd);
			Syslog('+', "No archive command for %s, fallback to ZIP", arc);
			if (!getarchiver((char *)"ZIP")) {
				WriteError("No ZIP command available");
				free(ond);
				free(onl);
				free(wrk);
				free(nn);
				die(100);
			} else {
				cmd = xstrcpy(archiver.farc);
			}
		} else {
			free(cmd);
			cmd = xstrcpy(archiver.farc);
		}

		if ((cmd == NULL) || (!strlen(cmd))) {
			WriteError("No archiver command available");
		} else {
			free(onl);
			onl = xstrcpy(nn);
			onl[strlen(onl) -3] = tolower(archiver.name[0]);
			tl(onl);
			p = xstrcpy(onl);
			p = xstrcat(p, (char *)" ");
			p = xstrcat(p, nn);
			if (execute(cmd, p, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null"))
				WriteError("Create %s failed", onl);
			else {
				CreateSema((char *)"mailin");
			}
			free(p);
			free(cmd);
		}

		free(onl);
		free(ond);
		free(wrk);
		free(nn);
		die(0);
	}
	return 0;
}



void Help(void)
{
	do_quiet = FALSE;
	ProgName();

	colour(11, 0);
	printf("\nUsage:	mbdiff [nodelist] [nodediff] <options>\n\n");
	colour(3, 0);
	printf("	The nodelist must be the full path and filename\n");
	printf("	without the dot and daynumber digits to the working\n");
	printf("	directory of that nodelist.\n");
	printf("	The nodediff must be the full path and filename\n");
	printf("	to the (compressed) nodediff file in the download\n");
	printf("	directory.\n");
	colour(9, 0);
	printf("\n	Options are:\n\n");
	colour(3, 0);
	printf("	-quiet		Quiet mode\n");
	colour(7, 0);
	printf("\n");
	die(99);
}



int apply(char *nl, char *nd, char *nn)
{
	FILE		*fo, *fd, *fn;
	unsigned char	cmdbuf[BLKSIZ];
	unsigned char	lnbuf[BLKSIZ];
	int		i, count;
	int		ac = 0, cc = 0, dc = 0;
	int		rc = 0;
	int		firstline = 1;
	unsigned short	theircrc = 0, mycrc = 0;
	unsigned char	*p;

	if ((fo = fopen(nl, "r")) == NULL) {
		WriteError("$Can't open %s", nl);
		return 2;
	}

	if ((fd = fopen(nd, "r")) == NULL) {
		WriteError("$Can't open %s", nd);
		fclose(fo);
		return 2;
	}

	if ((fn = fopen(nn, "w")) == NULL) {
		WriteError("$Can't open %s", nn);
		fclose(fo);
		fclose(fd);
		return 2;
	}

	if ((fgets(cmdbuf, sizeof(cmdbuf)-1, fd) == NULL) ||
	    (fgets(lnbuf, sizeof(cmdbuf)-1, fo) == NULL) ||
	    (strcmp(cmdbuf, lnbuf) != 0)) {
		rc = 6;
	} else {
		rewind(fo);
		rewind(fd);

		while ((rc == 0) && fgets(cmdbuf, sizeof(cmdbuf)-1, fd))
			switch (cmdbuf[0]) {
				case ';':
					Striplf(cmdbuf);
					break;
				case 'A':
					count = atoi(cmdbuf+1);
					ac += count;
					Striplf(cmdbuf);
					for (i = 0;(i < count) && (rc == 0); i++)
						if (fgets(lnbuf, sizeof(lnbuf)-1, fd)) {
							if (firstline) {
								firstline = 0;
								if ((p = strrchr(lnbuf, ':'))) {
									theircrc = atoi(p+1);
								}
							} else {
								for (p = lnbuf; *p; p++)
									mycrc = updcrc(*p, mycrc);
							}
							fputs(lnbuf, fn);
						} else
							rc = 3;
					break;
				case 'D':
					count = atoi(cmdbuf + 1);
					dc += count;
					Striplf(cmdbuf);
					for (i = 0;(i < count) && (rc == 0); i++)
						if (fgets(lnbuf, sizeof(lnbuf)-1, fo) == NULL) 
							rc = 3;
					break;
				case 'C':
					count = atoi(cmdbuf+1);
					cc += count;
					Striplf(cmdbuf);
					for (i = 0; (i < count) && (rc == 0); i++)
						if (fgets(lnbuf, sizeof(lnbuf) - 1, fo)) {
							for (p = lnbuf; *p; p++)
								mycrc = updcrc(*p, mycrc);
							fputs(lnbuf, fn);
						} else
							rc = 3;
					break;
				default:
					rc = 5;
					break;
			}
	}

	fclose(fo);
	fclose(fd);
	fclose(fn);

	if ((rc != 0) && !do_quiet) {
		show_log = TRUE;
		colour(12, 0);
	}

	if ((rc == 0) && (mycrc != theircrc)) 
		rc = 4;

	if (rc == 3)
		WriteError("Could not read some of the files");
	else if (rc == 4)
		WriteError("CRC is %hu, should be %hu", mycrc, theircrc);
	else if (rc == 5)
		WriteError("Unknown input line: \"%s\"", cmdbuf);
	else if (rc == 6)
		WriteError("Diff does not match old list");
	else {
		Syslog('+', "Copied %d, added %d, deleted %d, difference %d", cc, ac, dc, ac-dc);
		if (!do_quiet)
			printf("Created new nodelist\n");
	}

	return rc;
}


