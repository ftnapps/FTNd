/*****************************************************************************
 *
 * $Id: m7recv.c,v 1.6 2004/02/21 17:22:01 mbroek Exp $
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
#include "statetbl.h"
#include "ttyio.h"
#include "m7recv.h"



static int m7_recv(void);
static char* fn;
static int last;

int m7recv(char *fname)
{
	int rc;

	fn = fname;
	last = 0;
	rc = m7_recv();
	if (rc) 
		return -1;
	else if (last) 
		return 1;
	else 
		return 0;
}



SM_DECL(m7_recv,(char *)"m7recv")
SM_STATES
	sendnak,
	waitack,
	waitchar,
	sendack,
	sendcheck,
	waitckok
SM_NAMES
	(char *)"sendnak",
	(char *)"waitack",
	(char *)"waitchar",
	(char *)"sendack",
	(char *)"sendcheck",
	(char *)"waitckok"
SM_EDECL

	int	count = 0;
	int	c, i = 0;
	char	*p = fn;
	char	cs = SUB;

SM_START(waitchar)

SM_STATE(sendnak)

	if (count++ > 20) {
		Syslog('+', "Too many tries getting modem7 name");
		SM_ERROR;
	}
	p = fn;
	cs = SUB;
	i = 0;
	PUTCHAR(NAK);
	if (STATUS) {
		SM_ERROR;
	} else {
		SM_PROCEED(waitack);
	}

SM_STATE(waitack)

	c = GETCHAR(5);
	if (c == TIMEOUT) {
		Syslog('x', "m7 got timeout waiting for ACK");
		SM_PROCEED(sendnak);
	} else if (c < 0) {
		SM_ERROR;
	} else {
		switch (c) {
		case ACK:	SM_PROCEED(waitchar); 
				break;
		case EOT:	last=1; 
				SM_SUCCESS; 
				break;
		default:	Syslog('x', "m7 got '%s' waiting for ACK", printablec(c));
				break;
		}
	}

SM_STATE(waitchar)

	c = GETCHAR(1);
	if (c == TIMEOUT) {
		Syslog('x', "m7 got timeout waiting for char",c);
		SM_PROCEED(sendnak);
	} else if (c < 0) {
		SM_ERROR;
	} else {
		switch (c) {
		case EOT:	last=1; 
				SM_SUCCESS; 
				break;
		case SUB:	*p='\0'; 
				SM_PROCEED(sendcheck); 
				break;
		case 'u':	SM_PROCEED(sendnak); 
				break;
		default:	cs += c;
				if (i < 15) {
					if (c != ' ') {
						if (i == 8) 
							*p++='.';
						*p++ = tolower(c);
					}
					i++;
				}
				SM_PROCEED(sendack); 
				break;
		}
	}

SM_STATE(sendack)

	PUTCHAR(ACK);
	SM_PROCEED(waitchar);

SM_STATE(sendcheck)

	PUTCHAR(cs);
	SM_PROCEED(waitckok);

SM_STATE(waitckok)

	c = GETCHAR(1);
	if (c == TIMEOUT) {
		Syslog('x', "m7 got timeout waiting for ack ACK");
		SM_PROCEED(sendnak);
	} else if (c < 0) {
		SM_ERROR;
	} else if (c == ACK) {
		SM_SUCCESS;
	} else {
		SM_PROCEED(sendnak);
	}

SM_END
SM_RETURN


