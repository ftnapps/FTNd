/*****************************************************************************
 *
 * $Id: expipe.c,v 1.7 2004/02/21 14:24:04 mbroek Exp $
 * Purpose ...............: MBSE BBS Execute pipe
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
#include "mbselib.h"



static struct _fppid {
    FILE    *fp;
    int	    pid;
} fppid[] = {
    {NULL, 0}, {NULL, 0}, {NULL, 0}
};

#define maxfppid 2



FILE *expipe(char *cmd, char *from, char *to)
{
    char    buf[256], *buflimit, *vector[16], *p, *q, *f=from, *t=to;
    FILE    *fp;
    int	    i, rc, pid, slot, pipedes[2];

    buflimit = buf + sizeof(buf) -1 - (f&&t&&(strlen(f)>strlen(t))?strlen(f):t?strlen(t):0);

    for (slot = 0; slot <= maxfppid; slot++) {
	if (fppid[slot].fp == NULL) 
	    break;
    }
    if (slot > maxfppid) {
	WriteError("Attempt to pipe more than %d processes", maxfppid + 1);
	return NULL;
    }

    for (p = cmd, q = buf; (*p); p++) {
	if (q > buflimit) {
	    WriteError("Attempt to pipe too long command");
	    return NULL;
	}
	switch (*p) {
	    case '$':	switch (*(++p)) {
			    case 'f':
			    case 'F':	if ((f)) 
					    while (*f) 
						*(q++) = *(f++);
					f=from; 
					break;
			    case 't':
			    case 'T': 	if ((t)) 
					    while (*t) 
						*(q++) = *(t++);
					t=to; 
					break;
			    default: 	*(q++)='$'; 
					*(q++)=*p; 
					break;
			}
			break;
	    case '\\':	*(q++) = *(++p); 
			break;
	    default: 	*(q++) = *p; 
			break;
	}
    }

    *q = '\0';
    Syslog('+', "Expipe: %s",buf);
    i = 0;
    vector[i++] = strtok(buf," \t\n");
    while ((vector[i++] = strtok(NULL," \t\n")) && (i<16));
    vector[15] = NULL;
    fflush(stdout);
    fflush(stderr);
    if (pipe(pipedes) != 0) {
	WriteError("$Pipe failed for command \"%s\"", MBSE_SS(vector[0]));
	return NULL;
    }

    Syslog('e', "pipe() returned read=%d, write=%d", pipedes[0], pipedes[1]);
    if ((pid = fork()) == 0) {
	close(pipedes[1]);
	close(0);
	if (dup(pipedes[0]) != 0) {
	    WriteError("$Reopen of stdin for command %s failed", MBSE_SS(vector[0]));
	    exit(MBERR_EXEC_FAILED);
	}
	rc = execv(vector[0],vector);
	WriteError("$Exec \"%s\" returned %d", MBSE_SS(vector[0]), rc);
	exit(MBERR_EXEC_FAILED);
    }

    close(pipedes[0]);

    if ((fp = fdopen(pipedes[1],"w")) == NULL) {
	WriteError("$fdopen failed for pipe to command \"%s\"", MBSE_SS(vector[0]));
    }

    fppid[slot].fp = fp;
    fppid[slot].pid = pid;
    return fp;
}



int exclose(FILE *fp)
{
    int	status, rc, pid, slot, sverr;

    for (slot = 0; slot <= maxfppid; slot++) {
	if (fppid[slot].fp == fp) 
	    break;
    }
    if (slot > maxfppid) {
	WriteError("Attempt to close unopened pipe");
	return -1;
    }
    pid = fppid[slot].pid;
    fppid[slot].fp = NULL;
    fppid[slot].pid = 0;

    Syslog('e', "Closing pipe to the child process %d",pid);
    if ((rc = fclose(fp)) != 0) {
	WriteError("$Error closing pipe to transport (rc=%d)", rc);
	if ((rc = kill(pid,SIGKILL)) != 0)
	    WriteError("$kill for pid %d returned %d",pid,rc);
    }
    Syslog('e', "Waiting for process %d to finish",pid);
    do {
	rc = wait(&status);
	sverr = errno;
	if (status)
	    Syslog('e', "$Wait returned %d, status %d,%d", rc, status >> 8, status & 0xff);
    } while (((rc > 0) && (rc != pid)) || ((rc == -1) && (sverr == EINTR)));

    switch (rc) {
	case -1:WriteError("$Wait returned %d, status %d,%d", rc, status >> 8, status & 0xff);
		return MBERR_EXEC_FAILED;
	case 0:	return 0;
	default:
		if (WIFEXITED(status)) {
		    rc = WEXITSTATUS(status);
		    if (rc) {
			WriteError("Expipe: returned error %d", rc);
			return (rc + MBERR_EXTERNAL);
		    }
		}
		if (WIFSIGNALED(status)) {
		    rc = WTERMSIG(status);
		    WriteError("Wait stopped on signal %d", rc);
		    return rc;
		}
		if (rc)
		    WriteError("Wait stopped unknown, rc=%d", rc);
		return rc;
    }
    return 0;
}

