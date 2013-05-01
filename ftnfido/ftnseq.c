/*****************************************************************************
 *
 * ftnseq.c
 * Purpose ...............: give a hexadecimal sequence to stdout
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/users.h"
#include "../lib/ftnddb.h"
#include "ftndq.h"



int main(int argc, char **argv)
{
	struct passwd	*pw;
	unsigned int	seq;

	InitConfig();

	pw = getpwuid(getuid());

	InitClient(pw->pw_name, (char *)"ftnseq", CFG.location, CFG.logfile, 
		CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

	Syslog(' ', " "); 
	Syslog(' ', "FTNSEQ v%s", VERSION);

	seq = sequencer();
	Syslog('+', "Sequence string %08lx", seq);
	printf("%08x", seq);
	fflush(stdout);

	Syslog(' ', "FTNSEQ finished");
	ExitClient(0);
	return 0;
}



