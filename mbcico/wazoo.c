/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "ttyio.h"
#include "session.h"
#include "statetbl.h"
#include "config.h"
#include "emsi.h"
#include "respfreq.h"
#include "filelist.h"
#include "wazoo.h"
#include "zmodem.h"


extern int made_request;


int rxwazoo(void)
{
	int		rc = 0;
	fa_list		*eff_remote, tmpl;
	file_list	*tosend = NULL, **tmpfl;

	Syslog('+', "Start WaZOO session");

	if (emsi_remote_lcodes & LCODE_NPU) {
		Syslog('+', "Remote requested \"no pickup\", no send");
		eff_remote=NULL;
	} else if (emsi_remote_lcodes & LCODE_PUP) {
		Syslog('+', "Remote requested \"pickup primary\"");
		tmpl.addr = remote->addr;
		tmpl.next = NULL;
		eff_remote = &tmpl;
	} else eff_remote=remote;

	tosend = create_filelist(eff_remote,(char *)ALL_MAIL,0);

	if ((rc = zmrcvfiles()) == 0) {
		if ((emsi_local_opts & OPT_NRQ) == 0) {
			for (tmpfl = &tosend; *tmpfl; tmpfl = &((*tmpfl)->next));
			*tmpfl = respond_wazoo();
		}

		if ((tosend != NULL) || ((emsi_remote_lcodes & LCODE_NPU) == 0))
			rc = zmsndfiles(tosend);

		if ((rc == 0) && (made_request)) {
			Syslog('+', "Freq was made, trying to receive files");
			rc = zmrcvfiles();
		}
	}

	tidy_filelist(tosend, (rc == 0));

	if (rc)
		WriteError("WaZOO session failed: rc=%d", rc);
	else
		Syslog('+', "WaZOO session completed");
	return rc;
}



int txwazoo(void)
{
	int		rc = 0;
	file_list	*tosend = NULL, *respond = NULL;
	char		*nonhold_mail;

	Syslog('+', "Start WaZOO session");
//	if (localoptions & NOHOLD) 
		nonhold_mail = (char *)ALL_MAIL;
//	else 
//		nonhold_mail = (char *)NONHOLD_MAIL;
	if (emsi_remote_lcodes & LCODE_HAT) {
		Syslog('+', "Remote asked to \"hold all traffic\", no send");
		tosend = NULL;
	} else tosend = create_filelist(remote, nonhold_mail, 0);

	if ((tosend != NULL) || ((emsi_remote_lcodes & LCODE_NPU) == 0))
		rc = zmsndfiles(tosend);
	if (rc == 0)
		if ((rc = zmrcvfiles()) == 0)
			if ((emsi_local_opts & OPT_NRQ) == 0)
				if ((respond = respond_wazoo()))
					rc = zmsndfiles(respond);

	tidy_filelist(tosend,(rc == 0));
	tidy_filelist(respond,0);
	if (rc)
		WriteError("WaZOO session failed: rc=%d", rc);
	else
		Syslog('+', "WaZOO session completed");
	return rc;
}

