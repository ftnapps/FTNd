/*****************************************************************************
 *
 * File ..................: bbs/pop3.c
 * Purpose ...............: POP3 client
 * Last modification date : 13-May-2001
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
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mbinet.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "msgutil.h"
#include "pop3.h"



void error_popmail(char *);
void error_popmail(char *umsg)
{
	char	*p;

	pop3_send((char *)"QUIT\r\n");
	p = pop3_receive();
	pop3_close();
	colour(12, 0);
	printf("%s\r\n", umsg);
	fflush(stdout);
}



void retr_msg(int);
void retr_msg(int msgnum)
{
	char    	*p, *q, temp[128];
	int     	Header;
	unsigned long	crc = -1;

	sprintf(temp, "RETR %d\r\n", msgnum);
	if (pop3_cmd(temp) == 0) {
		Msg_New();
		Header = TRUE;
		sprintf(temp, "%s/%s/mailbox", CFG.bbs_usersdir, exitinfo.Name);
		Open_Msgbase(temp, 'w');
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
						sprintf(Msg.To, "%s", p+4);
					}
					if (strncmp(p, "From: ", 6) == 0) {
						if (strlen(p) > 106)
							p[106] = '\0';
						sprintf(Msg.From, "%s", p+6);
					}
					if (strncmp(p, "Subject: ", 9) == 0) {
						if (strlen(p) > 109)
							p[109] = '\0';
						sprintf(Msg.Subject, "%s", p+9);
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
						sprintf(temp, "\001%s", p);
						MsgText_Add2(temp);
					}
				} else {
					MsgText_Add2(p);
				}
			}
		}
		Msg_AddMsg();
		Msg_UnLock();
		Msg_Close();
		sprintf(temp, "DELE %d\r\n", msgnum);
		pop3_cmd(temp);
	} else {
		WriteError("POP3: Can't retrieve message %d", msgnum);
	}
}



void check_popmail(char *user, char *pass)
{
	char	*p, *q, temp[128];
	int	tmsgs = 0, size, msgnum, color = 9;
	FILE	*tp;

	/*
	 *  If nothing is retrieved from the POP3 mailbox, the user sees nothing.
	 */
	Syslog('+', "POP3: connect user %s", user);
	if (pop3_connect() == -1) {
		WriteError("Can't connect POP3 server");
		return;
	}

	sprintf(temp, "USER %s\r\n", user);
	if (pop3_cmd(temp)) {
		error_popmail((char *)"You have no email box");
		return;
	}

	sprintf(temp, "PASS %s\r\n", pass);
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
					colour(color, 0);
					printf("\rFetching message %02d/%02d, total %d bytes", msgnum, tmsgs, size);
					fflush(stdout);
					if (color < 15)
						color++;
					else
						color = 9;
					retr_msg(msgnum);
				}
				fclose(tp);
			}
		}
		fflush(stdout);
	}

	pop3_cmd((char *)"QUIT\r\n");
	pop3_close();

	if (tmsgs) {
		colour(13, 0);
		printf("\r                                                \r");
		fflush(stdout);
	}
}



