/*****************************************************************************
 *
 * File ..................: tosser/echoout.c
 * Purpose ...............: Forward echomail packets
 * Last modification date : 23-Aug-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "../lib/dbmsgs.h"
#include "addpkt.h"
#include "echoout.h"



/*
 * External declarations
 */
extern	char	*toname;		/* To user			    */
extern	char	*fromname;		/* From user			    */
extern 	char	*subj;			/* Message subject		    */




void EchoOut(faddr *fa, fidoaddr aka, FILE *fp, int flags, int cost, time_t date)
{
	char		*buf;
	FILE		*qp;
	faddr		*From, *To;

	if ((qp = OpenPkt(msgs.Aka, aka, (char *)"qqq")) == NULL)
		return;

	From = fido2faddr(msgs.Aka);
	To = fido2faddr(aka);
	if (AddMsgHdr(qp, From, To, flags, cost, date, toname, fromname, subj)) {
		tidy_faddr(From);
		tidy_faddr(To);
		return;
	}

	rewind(fp);
	buf = calloc(2048, sizeof(char));

	while ((fgets(buf, 2048, fp)) != NULL) {
		Striplf(buf);
		fprintf(qp, "%s\r", buf);
	}

	free(buf);
	putc(0, qp);
	fsync(fileno(qp));
	fclose(qp);
}



