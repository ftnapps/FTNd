/*****************************************************************************
 *
 * $Id: mbseq.c,v 1.9 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: give a hexadecimal sequence to stdout
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "mbseq.h"



int main(int argc, char **argv)
{
	struct passwd	*pw;
	unsigned int	seq;

	InitConfig();

	pw = getpwuid(getuid());

	InitClient(pw->pw_name, (char *)"mbseq", CFG.location, CFG.logfile, 
		CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

	Syslog(' ', " "); 
	Syslog(' ', "MBSEQ v%s", VERSION);

	seq = sequencer();
	Syslog('+', "Sequence string %08lx", seq);
	printf("%08x", seq);
	fflush(stdout);

	Syslog(' ', "MBSEQ finished");
	ExitClient(0);
	return 0;
}



