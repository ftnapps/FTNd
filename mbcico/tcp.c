/*****************************************************************************
 *
 * $Id: tcp.c,v 1.8 2004/02/21 17:22:01 mbroek Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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

/*
	contributed by Stanislav Voronyi <stas@uanet.kharkov.ua>
*/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "statetbl.h"
#include "config.h"
#include "emsi.h"
#include "respfreq.h"
#include "filelist.h"
#include "tcpproto.h"
#include "tcp.h"


extern int made_request;


int rxtcp(void)
{
    int		rc = 0;
    fa_list	*eff_remote, tmpl;
    file_list	*tosend = NULL, **tmpfl;

    Syslog('+', "TCP: inbound session start");

    if (emsi_remote_lcodes & LCODE_NPU) {
	Syslog('+', "Remote requested \"no pickup\", no send");
	eff_remote=NULL;
    } else if (emsi_remote_lcodes & LCODE_PUP) {
	Syslog('+', "Remote requested \"pickup primary\"");
	tmpl.addr=remote->addr;
	tmpl.next=NULL;
	eff_remote=&tmpl;
    } else 
	eff_remote=remote;

    tosend = create_filelist(eff_remote, (char *)ALL_MAIL, 0);

    if ((rc=tcprcvfiles()) == 0) {
	if ((emsi_local_opts & OPT_NRQ) == 0) {
	    for (tmpfl = &tosend; *tmpfl; tmpfl = &((*tmpfl)->next));
		*tmpfl = respond_wazoo();
	}

	if ((tosend != NULL) || ((emsi_remote_lcodes & LCODE_NPU) == 0))
	    rc = tcpsndfiles(tosend);

	if ((rc == 0) && (made_request)) {
	    Syslog('+', "TCP: freq was made, trying to receive files");
	    rc = tcprcvfiles();
	}
    }

    tidy_filelist(tosend,(rc == 0));

    if (rc)
	WriteError("TCP: session failed: rc=%d", rc);
    else
	Syslog('+', "TCP: session completed");
    return rc;
}



int txtcp(void)
{
    int		rc=0;
    file_list	*tosend = NULL, *respond = NULL;
    char	*nonhold_mail;

    Syslog('+', "TCP: outbound session start");

    nonhold_mail = (char *)ALL_MAIL;
    if (emsi_remote_lcodes & LCODE_HAT) {
	Syslog('+', "Remote asked to \"hold all traffic\", no send");
	tosend=NULL;
    } else 
	tosend = create_filelist(remote,nonhold_mail,0);

    if ((tosend != NULL) || ((emsi_remote_lcodes & LCODE_NPU) == 0))
	rc = tcpsndfiles(tosend);
    if (rc == 0)
	if ((rc = tcprcvfiles()) == 0)
	    if ((emsi_local_opts & OPT_NRQ) == 0)
		if ((respond = respond_wazoo()))
		    rc = tcpsndfiles(respond);

    tidy_filelist(tosend,(rc == 0));
    tidy_filelist(respond,0);

    if (rc)
	WriteError("TCP: session failed: rc=%d", rc);
    else
	Syslog('+', "TCP: session completed");
    return rc;
}

