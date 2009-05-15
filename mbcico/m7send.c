/*****************************************************************************
 *
 * $Id: m7send.c,v 1.6 2004/02/21 17:22:01 mbroek Exp $
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
#include "m7send.h"

static int m7_send(void);

static char *fn;

int m7send(char *fname)
{
	fn=fname;
	return m7_send();
}



SM_DECL(m7_send,(char *)"m7send")
SM_STATES
	waitnak,
	sendack,	
	sendchar,
	waitack,
	sendsub,
	waitcheck,
	ackcheck
SM_NAMES
	(char *)"waitnak",
	(char *)"sendack",
	(char *)"sendchar",
	(char *)"waitack",
	(char *)"sendsub",
	(char *)"waitcheck",
	(char *)"ackcheck"
SM_EDECL

	char	buf[12],*p;
	int	i,c,count=0;
	char	cs=SUB;

	memset(buf,' ',sizeof(buf));
	for (i=0,p=fn; (i<8) && (*p) && (*p != '.'); i++,p++)
		buf[i] = toupper(*p);
	if (*p == '.') 
		p++;
	for (; (i<11) && (*p); i++,p++)
		buf[i] = toupper(*p);
	for (i=0; i<11; i++)
		cs += buf[i];
	buf[11]='\0';
	Syslog('x', "modem7 filename \"%s\", checksum %02x", buf,(unsigned char)cs);

SM_START(sendack)

SM_STATE(waitnak)

	if (count++ > 20) {
		Syslog('+', "too many tries sending modem7 name");
		SM_ERROR;
	}

	c=GETCHAR(5);
	if (c == TIMEOUT) {
		Syslog('x', "m7 got timeout waiting NAK");
		SM_PROCEED(waitnak);
	} else if (c < 0) {
		SM_ERROR;
	} else if (c == NAK) {
		SM_PROCEED(sendack);
	} else {
		Syslog('x', "m7 got '%s' waiting NAK", printablec(c));
		PUTCHAR('u');
		SM_PROCEED(waitnak);
	}

SM_STATE(sendack)

	i = 0;
	PUTCHAR(ACK);
	if (STATUS) {
		SM_ERROR;
	} else {
		SM_PROCEED(sendchar);
	}

SM_STATE(sendchar)

	if (i > 11) {
		SM_PROCEED(sendsub);
	}

	PUTCHAR(buf[i++]);
	if (STATUS) {
		SM_ERROR;
	} else {
		SM_PROCEED(waitack);
	}

SM_STATE(waitack)

	c = GETCHAR(1);
	if (c == TIMEOUT) {
		Syslog('x', "m7 got timeout waiting ACK for char %d",i);
		PUTCHAR('u');
		SM_PROCEED(waitnak);
	} else if (c < 0) {
		SM_ERROR;
	} else if (c == ACK) {
		SM_PROCEED(sendchar);
	} else {
		Syslog('x', "m7 got '%s' waiting ACK for char %d", printablec(c),i);
		PUTCHAR('u');
		SM_PROCEED(waitnak);
	}

SM_STATE(sendsub)

	PUTCHAR(SUB);
	SM_PROCEED(waitcheck);

SM_STATE(waitcheck)

	c = GETCHAR(1);
	if (c == TIMEOUT) {
		Syslog('x', "m7 got timeout waiting check");
		PUTCHAR('u');
		SM_PROCEED(waitnak);
	} else if (c < 0) {
		SM_ERROR;
	} else if (c == cs) {
		SM_PROCEED(ackcheck);
	} else {
		Syslog('x', "m7 got %02x waiting check %02x", (unsigned char)c,(unsigned char)cs);
		PUTCHAR('u');
		SM_PROCEED(waitnak);
	}

SM_STATE(ackcheck)

	PUTCHAR(ACK);
	if (STATUS) {
		SM_ERROR;
	} else {
		SM_SUCCESS;
	}

SM_END
SM_RETURN


