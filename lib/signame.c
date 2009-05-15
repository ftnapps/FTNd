/*****************************************************************************
 *
 * $Id: signame.c,v 1.9 2007/08/22 21:09:24 mbse Exp $
 * Purpose ...............: Signal names
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "mbselib.h"


/*
 *  Signal handler signal names.
 */

#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)

char	SigName[32][16] = {	"NOSIGNAL",
		"SIGHUP",	"SIGINT",	"SIGQUIT",	"SIGILL",
		"SIGTRAP",	"SIGIOT",	"SIGBUS",	"SIGFPE",
		"SIGKILL",	"SIGUSR1",	"SIGSEGV",	"SIGUSR2",
		"SIGPIPE",	"SIGALRM",	"SIGTERM",	"SIGSTKFLT",
		"SIGCHLD",	"SIGCONT",	"SIGSTOP",	"SIGTSTP",
		"SIGTTIN",	"SIGTTOU",	"SIGURG",	"SIGXCPU",
		"SIGXFSZ",	"SIGVTALRM",	"SIGPROF",	"SIGWINCH",
		"SIGIO",	"SIGPWR",	"SIGUNUSED"};

#elif defined(__PPC__) || defined(__ppc__)

char    SigName[32][16] = {     "NOSIGNAL",
                "SIGHUP",       "SIGINT",       "SIGQUIT",      "SIGILL",
                "SIGTRAP",      "SIGIOT",       "SIGBUS",       "SIGFPE",
                "SIGKILL",      "SIGUSR1",      "SIGSEGV",      "SIGUSR2",
                "SIGPIPE",      "SIGALRM",      "SIGTERM",      "SIGSTKFLT",
                "SIGCHLD",      "SIGCONT",      "SIGSTOP",      "SIGTSTP",
                "SIGTTIN",      "SIGTTOU",      "SIGURG",       "SIGXCPU",
                "SIGXFSZ",      "SIGVTALRM",    "SIGPROF",      "SIGWINCH",
                "SIGIO",        "SIGPWR",       "SIGUNUSED"};

#elif defined(__sparc__)

char	SigName[32][16] = {	"NOSIGNAL",
		"SIGHUP",	"SIGINT",	"SIGQUIT",	"SIGILL",
		"SIGTRAP",	"SIGIOT",	"SIGEMT",	"SIGFPE",
		"SIGKILL",	"SIGBUS",	"SIGSEGV",	"SIGSYS",
		"SIGPIPE",	"SIGALRM",	"SIGTERM",	"SIGURG",
		"SIGSTOP",	"SIGTSTP",	"SIGCONT",	"SIGCHLD",
		"SIGTTIN",	"SIGTTOU",	"SIGIO",	"SIGXCPU",
		"SIGXFSZ",	"SIGVTALRM",	"SIGPROF",	"SIGWINCH",
		"SIGLOST",	"SIGUSR1",	"SIGUSR2"};

#elif defined(__alpha__) || defined(__hppa__)

char	SigName[32][16] = {	"NOSIGNAL",
		"SIGHUP",       "SIGINT",       "SIGQUIT",      "SIGILL",
		"SIGTRAP",      "SIGABRT",      "SIGEMT",       "SIGFPE",
		"SIGKILL",      "SIGBUS",       "SIGSEGV",      "SIGSYS",
		"SIGPIPE",      "SIGALRM",      "SIGTERM",      "SIGURG",
		"SIGSTOP",      "SIGTSTP",      "SIGCONT",      "SIGCHLD",
		"SIGTTIN",      "SIGTTOU",      "SIGIO",        "SIGXCPU",
		"SIGXFSZ",      "SIGVTALRM",    "SIGPROF",      "SIGWINCH",
		"SIGINFO",      "SIGUSR1",      "SIGUSR2"};

#else
#error "Can't make signal names on this platform"
#endif

