/*****************************************************************************
 *
 * $Id: dietifna.c,v 1.9 2007/08/26 12:21:16 mbse Exp $
 * Purpose ...............: Fidonet mailer
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
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "emsi.h"
#include "dietifna.h"
#include "respfreq.h"
#include "filelist.h"
#include "xmrecv.h"
#include "xmsend.h"


extern int made_request;
static int sendfiles(file_list*);
static int xmrcvfiles(void);



int rxdietifna(void)
{
	int		rc;
	file_list	*tosend = NULL, **tmpfl;

	Syslog('+', "Start DietIFNA session");
	session_flags |= SESSION_IFNA;
	session_flags &= ~SESSION_BARK;
	tosend = create_filelist(remote, (char *)ALL_MAIL, 0);

	if ((rc = xmrcvfiles()) == 0) {
		if ((emsi_local_opts & OPT_NRQ) == 0) {
			for (tmpfl = &tosend; *tmpfl; tmpfl = &((*tmpfl)->next));
			*tmpfl = respond_wazoo();
		}
		rc = sendfiles(tosend);
		/* we are not sending file requests in slave session */
	}

	tidy_filelist(tosend, (rc == 0));
	if (rc)
		WriteError("DietIFNA session failed: rc=%d", rc);
	else
		Syslog('+', "DietIFNA session completed");
	return rc;
}



int txdietifna(void)
{
	int		rc;
	file_list	*tosend = NULL, *respond = NULL;
	char		*nonhold_mail;

	Syslog('+', "Start DietIFNA session");
	session_flags |= SESSION_IFNA;
	session_flags &= ~SESSION_BARK;
	nonhold_mail = (char *)ALL_MAIL;
	tosend = create_filelist(remote, nonhold_mail, 2);

	if ((rc = sendfiles(tosend)) == 0)
		if ((rc = xmrcvfiles()) == 0)
			if ((emsi_local_opts & OPT_NRQ) == 0)
				if ((respond = respond_wazoo()))
					rc = sendfiles(respond);
	/* but we are trying to respond other's file requests in master  */
	/* session, though they are not allowed by the DietIFNA protocol */
	
	tidy_filelist(tosend, (rc == 0));
	tidy_filelist(respond, 0);
	if (rc)
		WriteError("DietIFNA session failed: rc=%d", rc);
	else
		Syslog('+', "DietIFNA session completed");
	return rc;
}



int xmrcvfiles(void)
{
	int	rc;

	while ((rc = xmrecv(NULL)) == 0);
	if (rc == 1) 
		return 0;
	else 
		return rc;
}



int sendfiles(file_list *tosend)
{
	int	c, count = 0;

	while (((c = GETCHAR(15)) >= 0) && (c != NAK) && (c != 'C') && 
	       (count++ < 9)) 
		Syslog('s', "got '%s' waiting for C/NAK", printablec(c));

	if (c == NAK)
		session_flags &= ~FTSC_XMODEM_CRC;
	else if (c == 'C')
		session_flags |= FTSC_XMODEM_CRC;
	else if (c < 0) 
		return c;
	else 
		return 1;

	return xmsndfiles(tosend);
}


