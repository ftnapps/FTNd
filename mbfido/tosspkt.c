/*****************************************************************************
 *
 * File ..................: tosser/tosspkt.c
 * Purpose ...............: Toss a single *.pkt file
 * Last modification date : 03-Jun-2001
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
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "importmsg.h"
#include "tosspkt.h"



/*
 * External declarations
 */
extern	int	do_quiet;



/*
 * Global variables
 */
int		net_in = 0;		/* Netmails received		    */
int		net_imp = 0;		/* Netmails imported                */
int		net_out = 0;		/* Netmails forwarded		    */
int		net_bad = 0;		/* Bad netmails (tracking errors    */
int		echo_in = 0;		/* Echomail received		    */
int		echo_imp = 0;		/* Echomail imported		    */
int		echo_out = 0;		/* Echomail forwarded		    */
int		echo_bad = 0;		/* Bad echomail			    */
int		echo_dupe = 0;		/* Dupe echomail		    */
int		news_in = 0;		/* News received		    */
int		news_imp = 0;		/* News imported		    */
int		news_out = 0;		/* News posted			    */
int		news_bad = 0;		/* Bad news			    */
int		news_dupe = 0;		/* Dupe articles		    */
int		email_in = 0;		/* Email received		    */
int		email_imp = 0;		/* Email imported		    */
int		email_out = 0;		/* Email forwarded		    */
int		email_bad = 0;		/* Bad email			    */
char		*toname = NULL;		/* To user			    */
char		*fromname = NULL;	/* From user			    */
char		*subj = NULL;		/* Message subject		    */
extern char	*msgid;


static int at_zero = 0;

char *aread(char *, int, FILE *);
char *aread(char *s, int count, FILE *fp)
{
	int i,c,next;

	if (feof(fp)) 
		return(NULL);
	if (s == NULL) 
		return NULL;
	if (at_zero)
	{
		at_zero=0;
		return NULL;
	}

	for (i = 0,next = 1; (i < count-1) && next;)
		switch (c=getc(fp)) {
		case '\n':	break;

		case '\r':	s[i]='\n';
				i++;
				next=0;
				break;

		case 0x8d:	s[i]=' ';
				i++;
				break;

		case '\0':	at_zero=1;
				next=0;
				break;

		default:	s[i]=c;
				i++;
				break;
		}
	s[i]='\0';
	return s;
}



/*
 * Toss one packet.
 *
 *  0  - 
 *  1  - Cannot open packet
 *  2  - Bad packet header
 *  3  - Packet is not for us
 *  4  - Bad password
 */
int TossPkt(char *fn)
{
	int		rc, count = 0; 
	static int	maxrc = 0;
	static faddr	from, to;
	FILE		*pkt;

	if (!do_quiet) {
		colour(10, 0);
		printf("Tossing packet %s\n", fn);
	}

	if ((pkt = fopen(fn, "r")) == 0) {
		WriteError("$Cannot open %s", fn);
		return 1;
	}

	memset(&from, 0, sizeof(faddr));
	memset(&to,   0, sizeof(faddr));

	if (((rc = getheader(&from, &to, pkt, fn)) != 0)) {
		WriteError("%s, aborting",
				(rc == 1)?"wrong header type":
				(rc == 2)?"bad packet header":
				(rc == 3)?"packet is not for us":
				(rc == 4)?"bad password":
				"bad packet");
		return(rc);
	}

	while ((rc = getmessage(pkt, &from, &to)) == 1) {
		count++;
	}
	Syslog('+', "Messages : %d", count);

	maxrc = rc;
	if (!do_quiet)
		printf("\r                                                    \r");

	if (rc)
		Syslog('+', "End, rc=%d", maxrc);

	fclose(pkt);
	return maxrc;
}



/*
 * Process one message from message packet.
 *
 *  0   - no more messages
 *  1   - more messages
 *  2   - bad file
 *  3   - bad message header
 *  4   - unable to open temp file
 *  5   - unexpected end of packet
 *  >10 - import error
 */
int getmessage(FILE *pkt, faddr *p_from, faddr *p_to)
{
	char		buf[2048];
	char		*orig = NULL;
	char		*p, *l, *r;
	int		tmp, rc, maxrc = 0;
	static faddr	f, t;
	faddr		*o;
	int		result, flags, cost;
	time_t		mdate = 0L;
	FILE		*fp;
	unsigned char	buffer[0x0e];
	off_t		orig_off;

	subj = NULL;
	toname = NULL;
	fromname = NULL;
	result = fread(&buffer, 1, sizeof(buffer), pkt);
	if (result == 0) {
		Syslog('m', "Zero bytes message, assume end of pkt");
		return 0;
	}

	switch(tmp = (buffer[0x01] << 8) + buffer[0x00]) {
	case 0:	
		if (result == 2)
			return 0;
		else {
			Syslog('!', "Junk after logical end of packet, skipped");
			return 5;

		}
	case 2:	
		break;

	default:
		Syslog('!', "bad message type: 0x%04x",tmp);
		return 2;
	}

	if (result != 14) {
		Syslog('!', "Unexpected end of packet");
		return 5;
	}

	memset(&f, 0, sizeof(f));
	memset(&t, 0, sizeof(t));
	f.node = (buffer[0x03] << 8) + buffer[0x02];
	t.node = (buffer[0x05] << 8) + buffer[0x04];
	f.net  = (buffer[0x07] << 8) + buffer[0x06];
	t.net  = (buffer[0x09] << 8) + buffer[0x08];
	flags  = (buffer[0x0b] << 8) + buffer[0x0a];
	cost   = (buffer[0x0d] << 8) + buffer[0x0c];

	/*
	 * Read the DateTime, toUserName, fromUserName and subject fields
	 * from the packed message. The stringlength is +1 for the right
	 * check. This is different then in ifmail's original code.
	 */
	if (aread(buf, sizeof(buf)-1, pkt)) {
		if (strlen(buf) > 20)
			Syslog('!', "date too long (%d) \"%s\"", strlen(buf), printable(buf, 0));
		mdate = parsefdate(buf, NULL);
		if (aread(buf, sizeof(buf)-1, pkt)) {
			Syslog('!', "date not null-terminated: \"%s\"",buf);
			return 3;
		}
	}

	if (aread(buf, sizeof(buf)-1, pkt)) {
		if (strlen(buf) > 36)
			Syslog('!', "to name too long (%d) \"%s\"", strlen(buf), printable(buf, 0));
		t.name = xstrcpy(buf);
		toname = xstrcpy(buf);
		if (aread(buf, sizeof(buf)-1, pkt)) {
			if (*(p=t.name+strlen(t.name)-1) == '\n')
				 *p = '\0';
			Syslog('!', "to name not null-terminated: \"%s\"",buf);
			return 3;
		}
	}

	if (aread(buf, sizeof(buf)-1, pkt)) {
		if (strlen(buf) > 36)
			Syslog('!', "from name too long (%d) \"%s\"", strlen(buf), printable(buf, 0));
		f.name = xstrcpy(buf);
		fromname = xstrcpy(buf);
		if (aread(buf, sizeof(buf)-1, pkt)) {
			if (*(p=f.name+strlen(f.name)-1) == '\n') 
				*p = '\0';
			Syslog('!', "from name not null-terminated: \"%s\"",buf);
			return 3;
		}
	}
	
	if (aread(buf, sizeof(buf)-1, pkt)) {
		if (strlen(buf) > 72)
			Syslog('!', "subject too long (%d) \"%s\"", strlen(buf), printable(buf, 0));
		subj = xstrcpy(buf);
		if (aread(buf, sizeof(buf)-1, pkt)) {
			if (*(p=subj+strlen(subj)-1) == '\n') 
				*p = '\0';
			subj = xstrcat(subj,(char *)"\\n");
			subj = xstrcat(subj,buf);
			Syslog('!', "subj not null-terminated: \"%s\"",buf);
			return 3;
		}
	}

	if (feof(pkt) || ferror(pkt)) {
		Syslog('!', "Could not read message header, aborting");
		return 3;
	}

	f.zone = p_from->zone;
	t.zone = p_to->zone;
	
	if ((fp = tmpfile()) == NULL) {
		WriteError("$unable to open temporary file");
		return 4;
	}
	orig_off = 0L;

	/*
	 * Read the text from the .pkt file
	 */
	while (aread(buf,sizeof(buf)-1,pkt)) {

		fputs(buf, fp);

		/*
		 * Extract info from Origin line if found.
		 */
		if (!strncmp(buf," * Origin:",10)) {
			orig_off = ftell(fp);
			p=buf+10;
			while (*p == ' ') p++;
			if ((l=strrchr(p,'(')) && (r=strrchr(p,')')) && (l < r)) {
				*l = '\0';
				*r = '\0';
				l++;
				if ((o = parsefnode(l))) {
					f.point = o->point;
					f.node = o->node;
					f.net = o->net;
					f.zone = o->zone;
					if (o->domain) 
						f.domain=o->domain;
					o->domain=NULL;
					tidy_faddr(o);
				}
			} else
				if (*(l=p+strlen(p)-1) == '\n')
					 *l='\0';
			for (l=p+strlen(p)-1;*l == ' ';l--) 
				*l='\0'; 
			orig = xstrcpy(p);
		}
	}

	rc = importmsg(p_from, &f,&t,orig,mdate,flags,cost,fp,orig_off);
	if (rc)
		rc+=10;
	if (rc > maxrc) 
		maxrc = rc;

	fclose(fp);

	if(f.name) 
		free(f.name); 
	f.name=NULL;

	if(t.name) 
		free(t.name); 
	t.name=NULL;

	if(f.domain) 
		free(f.domain); 
	f.domain=NULL;

	if(t.domain) 
		free(t.domain); 
	t.domain=NULL;

	if (fromname)
		free(fromname);
	fromname = NULL;

	if (toname)
		free(toname);
	toname = NULL;

	if (subj)
		free(subj);
	subj = NULL;

	if (orig)
		free(orig);
	orig = NULL;

	if (msgid)
		free(msgid);
	msgid = NULL;

	if (feof(pkt) || ferror(pkt)) {
		WriteError("Unexpected end of packet");
		return 5;
	}
	return 1;
}


