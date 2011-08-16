/*****************************************************************************
 *
 * $Id: recvbark.c,v 1.7 2004/02/21 17:22:01 mbroek Exp $
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

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "statetbl.h"
#include "recvbark.h"
#include "respfreq.h"
#include "filelist.h"


static int recv_bark(void);
extern int xmsndfiles(file_list*);


int recvbark(void)
{
	if ((session_flags & SESSION_BARK) && !(localoptions & NOFREQS)) {
		return recv_bark();
	} else { /* deny requests */
		PUTCHAR(CAN);
		return STATUS;
	}
}



SM_DECL(recv_bark,(char *)"recvbark")
SM_STATES
	sendenq,
	waitack,
	waitchar,
	scanreq,
	sendack,
	waitnak,
	sendfiles
SM_NAMES
	(char *)"sendenq",
	(char *)"waitack",
	(char *)"waitchar",
	(char *)"scanreq",
	(char *)"sendack",
	(char *)"waitnak",
	(char *)"sendfiles"
SM_EDECL

	int		c, c1, c2;
	short		lcrc, rcrc;
	char		buf[256], *p = NULL;
	int		count = 0,rc = 0;
	file_list	*tosend = NULL;

SM_START(sendenq)

SM_STATE(sendenq)

	count = 0;
	PUTCHAR(ENQ);
	if (STATUS) {
		SM_ERROR;
	} else {
		SM_PROCEED(waitack);
	}

SM_STATE(waitack)

	if (count++ > 10) {
		Syslog('+', "Wait for Bark Request: timeout");
		PUTCHAR(ETB);
		SM_SUCCESS; /* Yes, this is allright. */
	}

	c = GETCHAR(2);
	if (c == TIMEOUT) {
		Syslog('s', "  timeout, send ENQ");
		PUTCHAR(ENQ);
		SM_PROCEED(waitack);
	} else if (c < 0) {
		SM_ERROR;
	} else switch (c) {
		case ACK:	p = buf; 
				SM_PROCEED(waitchar); 
				break;
		case ETB:	SM_SUCCESS; 
				break;
		case ENQ:	PUTCHAR(ETB); 
				SM_PROCEED(sendenq); 
				break;
		case EOT:	PUTCHAR(ACK);
				SM_PROCEED(waitack); 
				break;
		default:	Syslog('s', "Recvbark got '%s' waiting for ACK", printablec(c));
				SM_PROCEED(waitack); 
				break;
	}

SM_STATE(waitchar)

	c=GETCHAR(15);
	if (c == TIMEOUT) {
		Syslog('s', "Recvbark got timeout waiting for char");
		SM_PROCEED(sendenq);
	} else if (c < 0) {
		SM_ERROR;
	} else switch (c) {
		case ACK:	SM_PROCEED(waitchar); 
				break;
		case ETX:	*p = '\0'; 
				SM_PROCEED(scanreq); 
				break;
		case SUB:	SM_PROCEED(sendenq); 
				break;
		default:	if ((p - buf) < sizeof(buf)) 
					*p++= c;
				SM_PROCEED(waitchar); 
				break;
	}

SM_STATE(scanreq)

	lcrc = crc16xmodem(buf, strlen(buf));
	c1 = GETCHAR(15);
	if (c1 == TIMEOUT) {
		SM_PROCEED(sendenq);
	} else if (c1 < 0) {
		SM_ERROR;
	}
	c2 = GETCHAR(15);
	if (c2 == TIMEOUT) {
		SM_PROCEED(sendenq);
	} else if (c2 < 0) {
		SM_ERROR;
	}
	rcrc = (c2 << 8) + (c1 & 0xff);
	if (lcrc != rcrc) {
		Syslog('s', "lcrc 0x%04x != rcrc 0x%04x", lcrc, rcrc);
		PUTCHAR(NAK);
		SM_PROCEED(sendenq);
	}
	SM_PROCEED(sendack);

SM_STATE(sendack)

	count = 0;
	PUTCHAR(ACK);
	tosend = respond_bark(buf);
	SM_PROCEED(waitnak);

SM_STATE(waitnak)

	Syslog('s', "recvbark WAITNAK count=%d, count");

	if (count++ > 5) {
		SM_ERROR;
	}
	c = GETCHAR(3);
	if (c == TIMEOUT) {
		Syslog('s', "  timeout");
		PUTCHAR(ACK);
		SM_PROCEED(waitnak);
	} else if (c < 0) {
		SM_ERROR;
	} else switch (c) {
		case NAK:	session_flags &= ~FTSC_XMODEM_CRC; /* fallthrough */
		case 'C':	session_flags |= FTSC_XMODEM_CRC;
				SM_PROCEED(sendfiles); 
				break;
		case ENQ:	PUTCHAR(ETB); 
				SM_PROCEED(waitack); 
				break;
		case SUB:	SM_PROCEED(sendenq); 
				break;
		default:	Syslog('s', "Recvbark got '%s' waiting for NAK", printablec(c));
				SM_PROCEED(waitack); 
				break;
	}

SM_STATE(sendfiles)

	rc = xmsndfiles(tosend);
	tidy_filelist(tosend, 0);
	if (rc == 0) {
		SM_PROCEED(sendenq);
	} else {
		SM_ERROR;
	}

SM_END
SM_RETURN

