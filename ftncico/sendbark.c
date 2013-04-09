/*****************************************************************************
 *
 * sendbark.c
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
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
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "statetbl.h"
#include "sendbark.h"
#include "xmrecv.h"



static int send_bark(void);
static char *nm,*pw,*dt;


int sendbark(void)
{
	char	*fn;
	FILE	*fp;
	char	buf[256], *p;
	int	rc = 0;

	fn = reqname(remote->addr);
	if ((fp = fopen(fn,"r")) == NULL) {
		Syslog('s', "no request file for this node");
		PUTCHAR(ETB);
		return 0;
	}

	while (fgets(buf,sizeof(buf)-1,fp)) {
		nm = buf;
		pw = strchr(buf, '!');
		dt = strchr(buf, '+');

		if (pw) 
			*pw++= '\0';
		if (dt) 
			*dt++= '\0';

		if (nm) {
			while (isspace(*nm)) 
				nm++;
			for (p = nm; (*p != '!') && (*p != '+') && (!isspace(*p)); p++);
			*p = '\0';
		}

		if (pw) {
			while (isspace(*pw)) 
				pw++;
			for (p = pw; (*p != '!') && (*p != '+') && (!isspace(*p)); p++);
			*p = '\0';
		} else
			pw = (char *)"";

		if (dt) {
			while (isspace(*nm)) 
				nm++;
			for (p = nm; (*p != '!') && (*p != '+') && (*p != '-') && (!isspace(*p)); p++);
			*p = '\0';
		} else
			dt = (char *)"0";

		if (*nm == ';') 
			continue;

		Syslog('+', "Sending bark request for \"%s\", password \"%s\", update \"%s\"",FTND_SS(nm),FTND_SS(pw),FTND_SS(dt));
		if ((rc = send_bark())) 
			break;
	}
	if (rc == 0) 
		PUTCHAR(ETB);
	fclose(fp);
	if (rc == 0) 
		unlink(fn);

	return rc;
}



SM_DECL(send_bark,(char *)"sendbark")
SM_STATES
    Send,
    waitack,
    getfile
SM_NAMES
    (char *)"send",
    (char *)"waitack",
    (char *)"getfile"
SM_EDECL

    char	    buf[256];
    unsigned short  crc;
    int		    c, count = 0;

    snprintf(buf,256,"%s %s %s",nm,dt,pw);
    crc = crc16xmodem(buf, strlen(buf));
    Syslog('s', "sending bark packet \"%s\", crc = 0x%04x", buf, crc);

SM_START(Send)

SM_STATE(Send)

    if (count++ > 5) {
	Syslog('+', "Bark request failed");
	SM_ERROR;
    }

    PUTCHAR(ACK);
    PUT(buf, strlen(buf));
    PUTCHAR(ETX);
    PUTCHAR(crc & 0xff);
    PUTCHAR((crc >> 8) & 0xff);
    if (STATUS) {
	SM_ERROR;
    } else {
	SM_PROCEED(waitack);
    }

SM_STATE(waitack)

    c = GETCHAR(10);
    if (c == TIMEOUT) {
	Syslog('s', "sendbark got timeout waiting for ACK");
	SM_PROCEED(Send);
    } else if (c < 0) {
	SM_PROCEED(Send);
    } else if (c == ACK) {
	SM_PROCEED(getfile);
    } else {
	Syslog('s', "sendbark got %s waiting for ACK", printablec(c));
	SM_PROCEED(Send);
    }

SM_STATE(getfile)

    switch (xmrecv(NULL)) {
	case 0:	    SM_PROCEED(getfile); 
		    break;
	case 1:	    SM_SUCCESS; 
		    break;
	default:    SM_ERROR; 
		    break;
    }

SM_END
SM_RETURN


