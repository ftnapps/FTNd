/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Libraries include list for mbtask
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10		Internet:	mbse@user.sourceforge.net
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

#ifndef _LIBS_H
#define _LIBS_H

#define TRUE	1
#define FALSE	0
#define	SS_BUFSIZE	1024		/* Streams socket buffersize	*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <netdb.h>
#include <arpa/inet.h>
#ifdef  HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <pwd.h>

#include <stddef.h>
#include <fcntl.h>
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <netinet/in_systm.h>
#endif
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>



/*
 *  Some older systems don;t have this
 */
#ifndef	ICMP_FILTER
#define ICMP_FILTER	1

struct icmp_filter {
        u_int32_t	data;
};

#endif


/* some older glibc versions seem to lack this. */
# ifndef IP_PKTINFO
#  define IP_PKTINFO 8
# endif


/* A macro to extract the pointer to the address of a struct sockaddr (_in or _in6) */
#define SOCKA_A4(a) ((void *)&((struct sockaddr_in *)(a))->sin_addr)


/* Some old libs don't have socklen_t */
#ifndef socklen_t
#define	socklen_t unsigned int
#endif


#pragma pack(1)

#define MAXNAME 35
#define MAXUFLAGS 16

typedef struct _faddr {
        char *name;
        unsigned int point;
        unsigned int node;
        unsigned int net;
        unsigned int zone;
        char *domain;
} faddr;



/*
 * ANSI colors
 */
#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define BROWN           6
#define LIGHTGRAY       7
#define DARKGRAY        8
#define LIGHTBLUE       9
#define LIGHTGREEN      10
#define LIGHTCYAN       11
#define LIGHTRED        12
#define LIGHTMAGENTA    13
#define YELLOW          14
#define WHITE           15


#endif

