/*****************************************************************************
 *
 * postemail.c
 * Purpose ...............: Post Email message from temp file
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
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
#include "../lib/users.h"
#include "../lib/ftnddb.h"
#include "../lib/ftninet.h"
#include "../lib/msg.h"
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
    char	*temp, *p, buf[4096];
    int		result = 1;
    faddr	*fa;
    parsedaddr	rfcaddr;
    
    rewind(fp);
    Syslog('+', "SMTP: posting email from \"%s\" to \"%s\"", MailFrom, MailTo);

    /*
     * If a user forgets the To: line at the start of the FTN
     * netmail, we end up here with a UUCP user with a local
     * address as the destination.
     * We can't deliver this and create a loop if we pass this
     * message to SMTP.
     */
    if ((fa = parsefaddr(MailTo))) {
	if (is_local(fa)) {
	    WriteError("Destination is a local FTN address");
	    email_bad++;
	    tidy_faddr(fa);
	    return 2;
	}
	tidy_faddr(fa);
    }

    if (smtp_connect() == -1) {
	WriteError("SMTP: connection refused");
	email_bad++;
	return 2;
    }

    temp = calloc(MAX_LINE_LENGTH +1, sizeof(char));

    rfcaddr = parserfcaddr(MailFrom);
    snprintf(temp, MAX_LINE_LENGTH, "MAIL FROM:<%s@%s>\r\n", FTND_SS(rfcaddr.remainder), FTND_SS(rfcaddr.target));
    Syslog('m', "%s", printable(temp, 0));
    if (smtp_cmd(temp, 250)) {
	WriteError("SMTP: refused FROM <%s@%s>", FTND_SS(rfcaddr.remainder), FTND_SS(rfcaddr.target));
	email_bad++;
	free(temp);
	return 2;
    }
    tidyrfcaddr(rfcaddr);

    rfcaddr = parserfcaddr(MailTo);
    snprintf(temp, MAX_LINE_LENGTH, "RCPT TO:<%s@%s>\r\n", FTND_SS(rfcaddr.remainder), FTND_SS(rfcaddr.target));
    Syslog('m', "%s", printable(temp, 0));
    if (smtp_cmd(temp, 250)) {
	WriteError("SMTP: refused TO <%s@%s>", FTND_SS(rfcaddr.remainder), FTND_SS(rfcaddr.target));
	email_bad++;
	free(temp);
	return 2;
    }
    tidyrfcaddr(rfcaddr);

    if (smtp_cmd((char *)"DATA\r\n", 354)) {
	WriteError("SMTP refused DATA mode");
	email_bad++;
	free(temp);
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
	    snprintf(temp, MAX_LINE_LENGTH, " .\r\n");
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


