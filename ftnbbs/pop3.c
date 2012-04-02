/*****************************************************************************
 *
 * $Id: pop3.c,v 1.15 2005/10/11 20:49:48 mbse Exp $
 * Purpose ...............: POP3 client
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/mbinet.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "msgutil.h"
#include "pop3.h"
#include "term.h"
#include "ttyio.h"


void error_popmail(char *);
void error_popmail(char *umsg)
{
    pop3_close();
    poutCR(LIGHTRED, BLACK, umsg);
}



void retr_msg(int);
void retr_msg(int msgnum)
{
    char	    *p, *q, temp[PATH_MAX], *base;
    int		    Header;
    unsigned int    crc = -1;

    snprintf(temp, 81, "RETR %d\r\n", msgnum);
    if (pop3_cmd(temp) == 0) {
	Msg_New();
	Header = TRUE;
	snprintf(temp, PATH_MAX, "%s/%s/mailbox", CFG.bbs_usersdir, exitinfo.Name);
	base = xstrcpy(temp);
	Open_Msgbase(base, 'w');
	Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
	Msg.Private = TRUE;
	while (TRUE) {
	    p = pop3_receive();
	    if ((p[0] == '.') && (strlen(p) == 1)) {
		break;
	    } else {
		if (Header) {
		    /*
		     *  Check the primary message header lines.
		     */
		    if (strncmp(p, "To: ", 4) == 0) {
			if (strlen(p) > 104)
			    p[104] = '\0';
			snprintf(Msg.To, 101, "%s", p+4);
		    }
		    if (strncmp(p, "From: ", 6) == 0) {
		        if (strlen(p) > 106)
			    p[106] = '\0';
			snprintf(Msg.From, 101, "%s", p+6);
		    }
		    if (strncmp(p, "Subject: ", 9) == 0) {
			if (strlen(p) > 109)
			    p[109] = '\0';
			snprintf(Msg.Subject, 101, "%s", p+9);
			mbse_CleanSubject(Msg.Subject);
		    }
		    if (strncmp(p, "Date: ", 6) == 0)
			Msg.Written = parsedate(p+6, NULL) - (gmt_offset((time_t)0) * 60);
		    if (strncmp(p, "Message-Id: ", 12) == 0) {
			q = xstrcpy(p+12);
			Msg.MsgIdCRC = upd_crc32(q, crc, strlen(q));
			free(q);
		    }
		    Msg.ReplyCRC = 0xffffffff;
		    if (strlen(p) == 0) {
			Header = FALSE;
		    } else {
			snprintf(temp, PATH_MAX, "\001%s", p);
			MsgText_Add2(temp);
		    }
		} else {
		    MsgText_Add2(p);
		}
	    }
	}
	Msg_AddMsg();
	Msg_UnLock();
	Close_Msgbase(base);
	free(base);
	snprintf(temp, 81, "DELE %d\r\n", msgnum);
	pop3_cmd(temp);
    } else {
	WriteError("POP3: Can't retrieve message %d", msgnum);
    }
}



void check_popmail(char *user, char *pass)
{
    char	*p, *q, temp[128];
    int	tmsgs = 0, size, msgnum, color = LIGHTBLUE;
    FILE	*tp;

    /*
     *  If nothing is retrieved from the POP3 mailbox, the user sees nothing.
     */
    if (CFG.UsePopDomain)
	Syslog('+', "POP3: connect user %s@%s", user, CFG.sysdomain);
    else
	Syslog('+', "POP3: connect user %s", user);
    if (pop3_connect() == -1) {
	WriteError("Can't connect POP3 server");
	return;
    }

    if (CFG.UsePopDomain)
	snprintf(temp, 128, "USER %s@%s\r\n", user, CFG.sysdomain);
    else
	snprintf(temp, 128, "USER %s\r\n", user);
    
    if (pop3_cmd(temp)) {
	error_popmail((char *)"You have no email box");
	return;
    }

    snprintf(temp, 128, "PASS %s\r\n", pass);
    if (pop3_cmd(temp)) {
	error_popmail((char *)"Wrong email password, reset your password");
	return;
    }

    Syslog('+', "POP3: logged in");

    pop3_send((char *)"STAT\r\n");
    p = pop3_receive();
    if (strncmp(p, "+OK", 3) == 0) {
	q = strtok(p, " ");
	q = strtok(NULL, " ");
	tmsgs = atoi(q);
	q = strtok(NULL, " \r\n\0");
	size = atoi(q);
	Syslog('+', "POP3: %d messages, %d bytes", tmsgs, size);
	if (tmsgs && ((tp = tmpfile()) != NULL)) {
	    if (pop3_cmd((char *)"LIST\r\n") == 0) {
		while (TRUE) {
		    p = pop3_receive();
		    if (p[0] == '.') {
			break;
		    } else {
			q = strtok(p, " ");
			msgnum = atoi(q);
			fwrite(&msgnum, sizeof(msgnum), 1, tp);
		    }
		}
		rewind(tp);
		while (fread(&msgnum, sizeof(msgnum), 1, tp) == 1) {
		    /*
		     *  Show progress
		     */
		    PUTCHAR('\r');
		    snprintf(temp, 128, "Fetching message %02d/%02d, total %d bytes", msgnum, tmsgs, size);
		    pout(color, BLACK, temp);
		    if (color < WHITE)
			color++;
		    else
			color = LIGHTBLUE;
		    retr_msg(msgnum);
		}
		fclose(tp);
	    }
	}
    }

    pop3_cmd((char *)"QUIT\r\n");
    pop3_close();

    if (tmsgs) {
	PUTCHAR('\r');
	colour(LIGHTMAGENTA, BLACK);
	pout(LIGHTMAGENTA, BLACK, (char *)"                                                ");
	PUTCHAR('\r');
    }
}



