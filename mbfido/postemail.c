/*****************************************************************************
 *
 * File ..................: mbfido/postemail.c
 * Purpose ...............: Post Email message from temp file
 * Last modification date : 06-May-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "../lib/records.h"
#include "../lib/dbuser.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mbinet.h"
#include "postemail.h"


/*
 * Global variables
 */
extern	int	email_in;		/* Total emails processed	    */
extern	int	email_imp;		/* Netmails imported                */
extern	int	email_bad;		/* Bad netmails			    */



/*
 *  Post email message
 *
 *  0 - All seems well.
 *  1 - Something went wrong.
 *  2 - SMTP error.
 *
 */
int postemail(FILE *fp, char *MailFrom, char *MailTo)
{
	char    *temp, *p;
	char    buf[4096];
	int	result = 1;

	temp = calloc(2048, sizeof(char));
	rewind(fp);

	Syslog('+', "SMTP: posting from %s to %s", MailFrom, MailTo);
	if (smtp_connect() == -1) {
		WriteError("SMTP: connection refused");
		return 2;
	}

	sprintf(temp, "MAIL FROM: <%s>\r\n", MailFrom);
	if (smtp_cmd(temp, 250)) {
		WriteError("SMTP: refused FROM <%s>", MailFrom);
		return 2;
	}

	sprintf(temp, "RCPT TO: <%s>\r\n", MailTo);
	if (smtp_cmd(temp, 250)) {
		WriteError("SMTP: refused TO <%s>", MailTo);
		return 2;
	}

	if (smtp_cmd((char *)"DATA\r\n", 354)) {
		WriteError("SMTP refused DATA mode");
		return 2;
	}

	while ((fgets(buf, sizeof(buf)-2, fp)) != NULL) {
		if (strncmp(buf, ".\r\n", 3)) {
			p = buf+strlen(buf)-1;
			if (*p == '\n') {
				*p++ = '\r';
				*p++ = '\n';
				*p = '\0';
			}
			smtp_send(buf);
		} else {
			sprintf(temp, " .\r\n");
			smtp_send(temp);
		}
	}

	email_in++;
	if (smtp_cmd((char *)".\r\n", 250) == 0) {
		Syslog('+', "SMTP: Message accepted");
		result = 0;
		email_imp++;
	} else {
		WriteError("SMTP: refused message");
		email_bad++;
	}

	free(temp);
	smtp_close();

	return result;
}


