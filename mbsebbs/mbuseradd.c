/*****************************************************************************
 *
 * File ..................: mbuseradd.c
 * Purpose ...............: setuid root version of useradd
 * Last modification date : 28-Jun-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek	FIDO:		2:280/2802
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

#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <linux/limits.h>

#include "mbuseradd.h"




int execute(char *cmd, char *file, char *in, char *out, char *err)
{
	char	buf[PATH_MAX];
	char	*vector[16];
	int	i;
	int	pid, status, rc, sverr;

	sprintf(buf, "%s %s", cmd, file);

	i=0;
	vector[i++] = strtok(buf, " \t\n");
	while ((vector[i++] = strtok(NULL," \t\n")) && (i < 16));
	vector[15] = NULL;
	fflush(stdout);
	fflush(stderr);

	if ((pid = fork()) == 0) {
		if (in) {
			close(0);
			if (open(in, O_RDONLY) != 0) {
				perror("");
				fprintf(stderr, "mbuseradd: Reopen of stdin to %s failed\n", in);
				exit(-1);
			}
		}
		if (out) {
			close(1);
			if (open(out, O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
				perror("");
				fprintf(stderr, "mbuseradd: Reopen of stdout to %s failed\n", out);
				exit(-1);
			}
		}
		if (err) {
			close(2);
			if (open(err, O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
				perror("");
				fprintf(stderr, "mbuseradd: Reopen of stderr to %s failed\n", err);
				exit(-1);
			}
		}
		rc = execv(vector[0],vector);
		fprintf(stderr, "mbuseradd: Exec \"%s\" returned %d\n", vector[0], rc);
		exit(-1);
	}

	do {
		rc = wait(&status);
		sverr = errno;
	} while (((rc > 0) && (rc != pid)) || ((rc == -1) && (sverr == EINTR)));

	if (rc == -1) {
		fprintf(stderr, "mbuseradd: Wait returned %d, status %d,%d\n", rc, status >> 8, status & 0xff);
		return -1;
	}

	return status;
}



void makedir(char *path, mode_t mode, uid_t owner, gid_t group)
{
	if (mkdir(path, mode) != 0) {
		perror("");
		fprintf(stderr, "mbuseradd: Can't create %s\n", path);
		exit(2);
	}
	if ((chown(path, owner, group)) == -1) {
		perror("");
		fprintf(stderr, "mbuseradd: Unable to change ownership of %s\n", path);
		exit(2);
	}
}



/*
 * Function will create the users name in the passwd file
 * Note that this function must run setuid root!
 */
int main(int argc, char *argv[])
{
	char		*PassEnt, *temp, *shell;
	int		i;
	struct passwd	*pwent, *pwuser;
	FILE		*fp;

	if (setuid(0) == -1 || setgid(1) == -1) {
		perror("");
		fprintf(stderr, "mbuseradd: Unable to setuid(root) or setgid(root)\n");
		fprintf(stderr, "Make sure that this program is set to -rwsr-sr-x\n");
		fprintf(stderr, "Owner must be root and group root\n");
		exit(1);
	}

	if (argc != 5)
		Help();

	for (i = 1; i < 5; i++) {
		if (strlen(argv[i]) > 80) {
			fprintf(stderr, "mbuseradd: Argument %d is too long\n", i);
			exit(1);
		}
	}

	PassEnt = calloc(PATH_MAX, sizeof(char));
	temp    = calloc(PATH_MAX, sizeof(char));
	shell   = calloc(PATH_MAX, sizeof(char));

	/*
	 * Build command to add user entry to the /etc/passwd and /etc/shadow
	 * files. We use the systems own useradd program.
	 */
	if ((access("/usr/bin/useradd", R_OK)) == 0)
		strcpy(temp, "/usr/bin/useradd");
	else if ((access("/bin/useradd", R_OK)) == 0)
		strcpy(temp, "/bin/useradd");
	else if ((access("/usr/sbin/useradd", R_OK)) == 0)
		strcpy(temp, "/usr/sbin/useradd");
	else if ((access("/sbin/useradd", R_OK)) == 0)
		strcpy(temp, "/sbin/useradd");
	else {
		fprintf(stderr, "mbuseradd: Can't find useradd\n");
		exit(1);
	}

	sprintf(shell, "%s/bin/mbsebbs", getenv("MBSE_ROOT"));

	sprintf(PassEnt, "%s -c \"%s\" -d %s/%s -g %s -s %s %s",
		temp, argv[3], argv[4], argv[2], argv[1], shell, argv[2]);
	fflush(stdout);
	fflush(stdin);

	if (system(PassEnt) != 0) {
		perror("mbuseradd: Failed to create unix account\n");
		exit(1);
	} 

	/*
	 * Now create directories and files for this user.
	 */
	if ((pwent = getpwnam((char *)"mbse")) == NULL) {
		perror("mbuseradd: Can't get password entry for \"mbse\"\n");
		exit(2);
	}

	/*
	 *  Check bbs users base home directory
	 */
	if ((access(argv[4], R_OK)) != 0)
		makedir(argv[4], 0770, pwent->pw_uid, pwent->pw_gid);

	/*
	 * Now create users home directory. Check for an existing directory,
	 * some systems have already created a home directory. If one is found
	 * it is removed to create a fresh one.
	 */
	sprintf(temp, "%s/%s", argv[4], argv[2]);
	if ((access(temp, R_OK)) == 0) {
		if ((access("/bin/rm", X_OK)) == 0)
			strcpy(shell, "/bin/rm");
		else if ((access("/usr/bin/rm", X_OK)) == 0)
			strcpy(shell, "/usr/bin/rm");
		else {
			fprintf(stderr, "mbuseradd: Can't find rm\n");
			exit(2);
		}
		sprintf(PassEnt, " -Rf %s", temp);
		fflush(stdout);
		fflush(stdin);
		i = execute(shell, PassEnt, (char *)"/dev/tty", (char *)"/dev/tty", (char *)"/dev/tty");

		if (i != 0) {
			fprintf(stderr, "mbuseradd: Unable remove old home directory\n");
			exit(2);
		}
	}

	/*
	 *  Create users home directory.
	 */
	pwuser = getpwnam(argv[2]);
	makedir(temp, 0770, pwuser->pw_uid, pwent->pw_gid);

	/*
	 *  Create Maildir and subdirs for Qmail.
	 */
	sprintf(temp, "%s/%s/Maildir", argv[4], argv[2]);
	makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
	sprintf(temp, "%s/%s/Maildir/cur", argv[4], argv[2]);
	makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
	sprintf(temp, "%s/%s/Maildir/new", argv[4], argv[2]);
	makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
	sprintf(temp, "%s/%s/Maildir/tmp", argv[4], argv[2]);
	makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);

	free(shell);
	free(PassEnt);
	free(temp);

	exit(0);
}



void Help()
{
	fprintf(stderr, "\nmbuseradd commandline:\n\n");
	fprintf(stderr, "mbuseradd [gid] [name] [comment] [usersdir]\n");
	exit(1);
}


