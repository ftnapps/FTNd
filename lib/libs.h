/*****************************************************************************
 *
 * File ..................: libs.h
 * Purpose ...............: Libraries include list
 * Last modification date : 05-Aug-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/


#ifndef _LIBS_H
#define _LIBS_H

#ifndef _GNU_SOURCE
#define	_GNU_SOURCE 1
#endif
#define	_REGEX_RE_COMP

#define	TRUE 1
#define	FALSE 0


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>   
#include <utime.h>
#include <stdarg.h>
#include <pwd.h>
#include <netdb.h>
#ifdef	SHADOW_PASSWORD
#include <shadow.h>
#endif
#include <sys/ioctl.h>
#ifdef	HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/file.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>
#include <sys/un.h>
#include <sys/time.h>
#include <regex.h>
#include <setjmp.h>
#include <grp.h>
#include <sys/resource.h>
#ifdef	HAVE_ICONV_H
#include <iconv.h>
#endif

#pragma pack(1)

#endif

