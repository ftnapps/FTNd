/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: give a hexadecimal sequence to stdout
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "mbseq.h"



int main(int argc, char **argv)
{
	struct passwd	*pw;
	unsigned long	seq;

#ifdef MEMWATCH
	mwInit();
#endif

	InitConfig();

	pw = getpwuid(getuid());

	InitClient(pw->pw_name, (char *)"mbseq", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log, CFG.mgrlog);

	Syslog(' ', " "); 
	Syslog(' ', "MBSEQ v%s", VERSION);

	seq = sequencer();
	Syslog('+', "Sequence string %08lx", seq);
	printf("%08lx", seq);
	fflush(stdout);

	Syslog(' ', "MBSEQ finished");
	ExitClient(0);
	return 0;
}



