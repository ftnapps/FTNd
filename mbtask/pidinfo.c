/*****************************************************************************
 *
 * $Id: pidinfo.c,v 1.1 2005/10/09 13:37:11 mbse Exp $
 * Purpose ...............: Pid utilities
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:	2:280/2802
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


/*
 * Find the program name for a given pid. The progname buffer
 * has te be allocated with size byte.
 */
int pid2prog(pid_t pid, char *progname, size_t size)
{
#if defined(__OpenBSD__)
#define ARG_SIZE 60
    static char **s, buf[ARG_SIZE];
    size_t      siz = 100;
    char        **p;
    int         mib[4];
#elif defined(__NetBSD__)
#define ARG_SIZE 60
    static char **s;
    size_t      siz = 100;
    int         mib[4];
#else
    char        temp[PATH_MAX];
    FILE        *fp;
#endif

#if defined(__OpenBSD__)
    /*
     * Systems that use sysctl to get process information
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC_ARGS;
    mib[2] = pid;
    mib[3] = KERN_PROC_ARGV;
    if ((s = realloc(s, siz)) == NULL) {
	WriteError("pid2prog(): no memory");
	return -1;
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
	WriteError("$pid2prog() sysctl call failed");
	return -1;
    }
    buf[0] = '\0';
    for (p = s; *p != NULL; p++) {
	if (p != s)
	    strlcat(buf, " ", sizeof(buf));
	strlcat(buf, *p, sizeof(buf));
    }
    strncpy(progname, buf, size);
    return 0;

#elif defined(__NetBSD__)
    /*
     * Systems that use sysctl to get process information
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC_ARGS;
    mib[2] = pid;
    mib[3] = KERN_PROC_ARGV;
    if ((s = realloc(s, siz)) == NULL) {
	WriteError("pid2prog(): no memory");
	return -1;
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
	WriteError("$pid2prog() sysctl call failed");
	return -1;
    }
//    parent = xstrcpy((char *)s);
    strncpy(progname, (char *)s, size);
    return 0;

#else
    /*
     *  Systems with /proc filesystem like Linux, FreeBSD
     */
    snprintf(temp, PATH_MAX, "/proc/%d/cmdline", pid);
    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't read %s", temp);
	return -1;
    }
    fgets(temp, PATH_MAX-1, fp);
    fclose(fp);
    strncpy(progname, temp, size);
    return 0;

#endif
}


