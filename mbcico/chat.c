/*****************************************************************************
 *
 * Purpose ...............: Fidonet mailer - modem chat
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "config.h"
#include "chat.h"
#include "ttyio.h"


char *tranphone(char *Phone)
{
    static char	trp[21];
    char	buf[21], *p;
    int		ln, i, j;

    if (Phone == NULL) 
	return NULL;
    strncpy(trp,Phone,sizeof(trp)-1);

    for (i = 0; i < 40; i++) 
	if (strlen(CFG.phonetrans[i].match) || strlen(CFG.phonetrans[i].repl)) {
	    memset(&buf, 0, sizeof(buf));
	    strncpy(buf,CFG.phonetrans[i].match,strlen(CFG.phonetrans[i].match));
	    ln=strlen(buf);
	    p = xstrcpy(CFG.phonetrans[i].repl);

	    if (strncmp(Phone,buf,ln) == 0) {
		strcpy(trp,p);
		strncat(trp,Phone+ln,sizeof(trp)-strlen(p)-1);
		free(p);
		break;
	    } else {
		free(p);
	    }
	}

    if (modem.stripdash) {
	j = 0;
	for (i = 0; i < strlen(trp); i++) {
	    if (trp[i] != '-') {
		trp[j] = trp[i];
		j++;
	    }
	}
	trp[j] = '\0';
    }
    return trp;
}



int send_str(char *, char *);
int send_str(char *str, char *Phone)
{
    char	*p, *q;
    static char	logs[81];

    p = str;
    memset(&logs, 0, sizeof(logs));
    q = logs;
	
    while (*p) {
	if (*p == '\\') {
	    switch (*++p) {
		case '\0':  p--; break;
		case '\\':  PUTCHAR('\\');   *q++ = '\\';              break;
		case 'r':   PUTCHAR('\r');   *q++ = '\\'; *q++ = 'r';  break;
		case 'n':   PUTCHAR('\n');   *q++ = '\\'; *q++ = 'n';  break;
		case 't':   PUTCHAR('\t');   *q++ = '\\'; *q++ = 't';  break;
		case 'b':   PUTCHAR('\b');   *q++ = '\\'; *q++ = 'b';  break;
		case 's':   PUTCHAR(' ');    *q++ = ' ';               break;
		case ' ':   PUTCHAR(' ');    *q++ = ' ';               break;
		case 'd':   sleep(1);        *q++ = '\\'; *q++ = 'd';  break;
		case 'p':   msleep(250); *q++ = '\\'; *q++ = 'p';  break;
		case 'D':   if (Phone) {
				PUTSTR(Phone);
				snprintf(q, 21, "%s", Phone);
			    }
			    break;
		case 'T':   if (Phone) {
				PUTSTR(tranphone(Phone));
				snprintf(q, 21, "%s", tranphone(Phone));
			    }
			    break;
		default:    PUTCHAR(*p);  *q++ = *p; break;
	    }
	    while (*q)
		q++;
	} else {
	    PUTCHAR(*p);
	    *q++ = *p;
	}
	p++;
    }
    Syslog('+', "chat: snd \"%s\"", logs);

    if (STATUS) {
	WriteError("$chat: send_str error %d", STATUS);
	return 1;
    } else
	return 0;
}



static int expired = FALSE;

void almhdl(int);
void almhdl(int sig)
{
    expired = TRUE;
    Syslog('c' ,"chat: timeout");
    return;
}



int expect_str(int, int, char *);
int expect_str(int timeout, int aftermode, char *Phone)
{
    int		    matched = FALSE, smatch = FALSE, ioerror = FALSE, i, rc;
    char	    inbuf[256];
    unsigned char   ch = '\0';
    int		    eol = FALSE;

    expired = FALSE;
    signal(SIGALRM, almhdl);
    alarm(timeout);

    while (!matched && !expired && !ioerror && !feof(stdin)) {

	eol = FALSE;
	i = 0;
	memset(&inbuf, 0, sizeof(inbuf));

	while (!ioerror && !feof(stdin) && !eol && (i < 255)) {

	    if ((rc = read(0, &ch, 1)) != 1) {
		ioerror = TRUE;
	    } else {
		switch(ch) {
		    case '\n': 	break;
		    case '\r':	inbuf[i++] = '\r';
				eol = TRUE;
				break;
		    default:	inbuf[i++] = ch;
		}
	    }
	    if (expired)
		Syslog('c', "chat: got TIMEOUT");
	}

	inbuf[i] = '\0';
	if (aftermode && strcmp(inbuf, (char *)"OK\r") && strcmp(inbuf, (char *)"\r"))
	    Syslog('+', "chat: rcv \"%s\"", printable(inbuf, 0));
	else
	    Syslog('c', "chat: rcv \"%s\"", printable(inbuf, 0));

	for (i = 0; (i < 10) && !matched; i++)
	    if (strlen(modem.error[i]))
		if (strncmp(modem.error[i], inbuf, strlen(modem.error[i])) == 0) {
		    matched = TRUE;
		    Syslog('+', "chat: got \"%s\", aborting", printable(inbuf, 0));
		}

	if (Phone != NULL)
	    for (i = 0; (i < 20) && !matched; i++)
		if (strlen(modem.connect[i]))
		    if (strncmp(modem.connect[i], inbuf, strlen(modem.connect[i])) == 0) {
			matched = TRUE;
			smatch  = TRUE;
			Syslog('+', "chat: got \"%s\", continue", printable(inbuf, 0));
		    }

	if (!matched)
	    if (strlen(modem.ok))
		if (strncmp(modem.ok, inbuf, strlen(modem.ok)) == 0) {
		    matched = TRUE;
		    smatch  = TRUE;
		    Syslog('+', "chat: got \"%s\", continue", printable(inbuf, 0));
		}

	if (expired)
	    Syslog('+', "chat: got timeout, aborting");
	else
	    if (ferror(stdin))
		Syslog('+', "chat: got error, aborting");

	if (feof(stdin))
	    WriteError("$chat: got EOF, aborting");
    }
    alarm(0);
    signal(SIGALRM, SIG_DFL);

    rc = !(matched && smatch);
    return rc;
}



/*
 * Chat with modem. If phone is not NULL, then expect also tests the modem
 * connect strings, else only the error strings and ok string is tested.
 * The phone number must be full international notation unless the \D
 * macro is in the dial command.
 */
int chat(char *Send, int timeout, int aftermode, char *Phone)
{
    int	rc;

    if ((rc = send_str(Send, Phone)) == 0)
	rc = expect_str(timeout, aftermode, Phone);

    return rc;
}


