/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: RFC msg
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

#include "libs.h"
#include "structs.h"
#include "records.h"
#include "common.h"
#include "clcomm.h"



#ifndef BUFSIZ
#define BUFSIZ 512
#endif

#define KWDCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_."



rfcmsg *parsrfc(FILE *fp)
{
	int	linecont=FALSE,newcont,firstline;
	rfcmsg	*start=NULL, *cur=NULL;
	char	buffer[BUFSIZ];
	char	*p;

	while (bgets(buffer, BUFSIZ-1, fp) && strcmp(buffer,"\n")) {
		newcont = (buffer[strlen(buffer)-1] != '\n');
		Syslog('M', "Line read: \"%s\" - %s continued", buffer,newcont?"to be":"not to be");
		if (linecont) {
			Syslog('M', "this is a continuation of a long line");
			cur->val=xstrcat(cur->val,buffer);
		} else {
			if (isspace(buffer[0])) {
				if (strspn(buffer," \t\n") == strlen(buffer)) {
					Syslog('M', "breaking with blank-only line");
					break;
				}
				Syslog('M', "this is a continuation line");
				if (!cur) {
					Syslog('M', "Wrong first line: \"%s\"",buffer);
					cur = (rfcmsg *)malloc(sizeof(rfcmsg));
					start = cur;
					cur->next = NULL;
					cur->key = xstrcpy((char *)"X-Body-Start");
					cur->val = xstrcpy(buffer);
					break;
				} else 
					cur->val = xstrcat(cur->val,buffer);
			} else {
				Syslog('M', "this is a header line");
				if (cur) {
					firstline=FALSE;
					(cur->next) = (rfcmsg *)malloc(sizeof(rfcmsg));
					cur = cur->next;
				} else {
					firstline = TRUE;
					cur = (rfcmsg *)malloc(sizeof(rfcmsg));
					start = cur;
				}
				cur->next = NULL;
				cur->key = NULL;
				cur->val = NULL;
				if (firstline && !strncmp(buffer,"From ",5)) {
					Syslog('M', "This is a uucpfrom line");
					cur->key=xstrcpy((char *)"X-UUCP-From");
					cur->val=xstrcpy(buffer+4);
				} else if ( !strncasecmp(buffer,"Cc:",3)) {
					Syslog('M', "Cc: line");
					if (strchr(buffer+3,'@')) {
					    cur->key = xstrcpy((char *)"Cc");
					    cur->val = xstrcpy(buffer+3);
					} else {
						Syslog('M', "FTN Cc: line: \"%s\"", buffer);
						cur->key = xstrcpy((char *)"X-Body-Start");
						cur->val = xstrcpy(buffer);
						break;
					}
				} else if ((p=strchr(buffer,':')) && (p > buffer) && /* ':' isn't 1st chr */
					 isspace(*(p+1)) && /* space past ':' */
					/* at least one non blank char */
					 (strspn(p+2, " \t\n") < strlen(p+2)) && (strspn(buffer,KWDCHARS) == (p-buffer))) {
					*p='\0';
					Syslog('M', "This is a regular header");
					cur->key = xstrcpy(buffer);
					cur->val = xstrcpy(p+1);
				} else {
					Syslog('M', "Non-header line: \"%s\"",buffer);
					cur->key = xstrcpy((char *)"X-Body-Start");
					cur->val = xstrcpy(buffer);
					break;
				}
			}
		}
		linecont = newcont;
	}
	return(start);
}



void tidyrfc(rfcmsg *msg)
{
	rfcmsg *nxt;

	for (; msg; msg=nxt) {
		nxt = msg->next;
		if (msg->key) 
			free(msg->key);
		if (msg->val) 
			free(msg->val);
		free(msg);
	}
	return;
}



void dumpmsg(rfcmsg *msg, FILE *fp)
{
	char *p;

	p = hdr((char *)"X-Body-Start",msg);
	for (; msg; msg=msg->next) 
		if (strcasecmp(msg->key, "X-Body-Start")) {
			if (!strcasecmp(msg->key, "X-UUCP-From"))
				fputs("From", fp);
			else {
				fputs(msg->key,fp);
				fputs(":",fp);
			}
			fputs(msg->val,fp);
		}
	fputs("\n",fp);
	if (p) 
		fputs(p,fp);
	return;
}


