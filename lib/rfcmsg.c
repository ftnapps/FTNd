/*****************************************************************************
 *
 * $Id: rfcmsg.c,v 1.14 2007/03/03 14:28:41 mbse Exp $
 * Purpose ...............: RFC msg
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "mbselib.h"



#ifndef BUFSIZ
#define BUFSIZ 512
#endif

#define KWDCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_."



/*
 * Parse RFC message, extract headers and do some sanity checks.
 */
rfcmsg *parsrfc(FILE *fp)
{
    int	    linecont=FALSE,newcont,firstline;
    rfcmsg  *start=NULL, *cur=NULL;
    char    buffer[BUFSIZ], *p;

    while (bgets(buffer, BUFSIZ-1, fp) && strcmp(buffer,"\n")) {
	newcont = (buffer[strlen(buffer)-1] != '\n');
	if (linecont) {
	    cur->val=xstrcat(cur->val,buffer);
	} else {
	    if (isspace(buffer[0])) {
		if (strspn(buffer," \t\n") == strlen(buffer)) {
		    break;
		}
		if (!cur) {
		    cur = (rfcmsg *)malloc(sizeof(rfcmsg));
		    start = cur;
		    cur->next = NULL;
		    cur->key = xstrcpy((char *)"X-Body-Start");
		    cur->val = xstrcpy(buffer);
		    break;
		} else 
		    cur->val = xstrcat(cur->val,buffer);
	    } else {
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
		    cur->key=xstrcpy((char *)"X-UUCP-From");
		    cur->val=xstrcpy(buffer+4);
		} else if ((p=strchr(buffer,':')) && (p > buffer) && /* ':' isn't 1st chr */
			    isspace(*(p+1)) && /* space past ':' */
			    /* at least one non blank char */
			    (strspn(p+2, " \t\n") < strlen(p+2)) && (strspn(buffer,KWDCHARS) == (p-buffer))) {
			*p='\0';
			cur->key = xstrcpy(buffer);
			cur->val = xstrcpy(p+1);
		} else if ((p=strchr(buffer,':')) && (!strncasecmp(buffer, (char *)"X-MS-", 5))) {
		    /*
		     * It looks like M$ invented their own internet standard, these
		     * are header lines without a key. This one will be stored here
		     * and junked in the rfc2ftn function.
		     */
		    cur->key = xstrcpy(buffer);
		    cur->val = xstrcpy((char *)" ");
		} else if ((p=strchr(buffer,':')) && (p > buffer)) {
		    /*
		     * Header line without information, don't add this one but log.
		     */
		    Syslog('!', "Header line \"%s\" without key value", buffer);
		    cur->key = xstrcpy(buffer);
		    cur->val = xstrcpy((char *)" ");
		} else if ((p=strchr(buffer,':')) && (p > buffer) && isspace(*(p+1))) { /* space past ':' */
		    Syslog('!', "Header line \"%s\" without key value (but with a Space)", buffer);
		    cur->key = xstrcpy(buffer);
		    cur->val = xstrcpy((char *)" ");
		} else {
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

