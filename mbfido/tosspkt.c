/*****************************************************************************
 *
 * Purpose ...............: Toss a single *.pkt file
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "../lib/users.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/mbsedb.h"
#include "tosspkt.h"
#include "postnetmail.h"
#include "postecho.h"
#include "rollover.h"
#include "createm.h"



/*
 * External declarations
 */
extern	int	do_quiet;
extern  int     do_unsec;
extern  int     check_dupe;
extern  time_t  t_start;



/*
 * Global variables
 */
int		net_in = 0;		/* Netmails received		    */
int		net_imp = 0;		/* Netmails imported                */
int		net_out = 0;		/* Netmails forwarded		    */
int		net_bad = 0;		/* Bad netmails (tracking errors    */
int		net_msgs = 0;		/* *.msg files processed	    */
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


static int at_zero = 0;


/*
 *  Internal prototypes
 */
char	*aread(char *, int, FILE *);
int     importmsg(faddr *, faddr *, faddr *, char *, char *, time_t, int, int, FILE *, unsigned int);



char *aread(char *s, int count, FILE *fp)
{
    int	i,c,next;

    if (feof(fp)) 
	return(NULL);
    if (s == NULL) 
	return NULL;
    if (at_zero) {
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
 * Import 1 message, forward if needed.
 * pkt_from, from, to, subj, orig, mdate, flags, cost, file
 *
 *  0 - All Seems Well.
 *  1 - Can't access messagebase.
 *  2 - Cannot open mareas.data
 *  3 - Echomail without Origin line.
 *  4 - Echomail from unknown node or disconnected node.
 *  5 - Locking error.
 *  6 - Unknown echomail area.
 *
 */
int importmsg(faddr *p_from, faddr *f, faddr *t, char *orig, char *subj, time_t mdate, 
	int flags, int cost, FILE *fp, unsigned int tzone)
{
    char	*buf, *marea = NULL;
    int		echomail = FALSE, rc = 0, bad = 0, FirstLine, size = 0;
    sysconnect	Link;

    if (CFG.slow_util && do_quiet)
	msleep(1);

    memset(&Link, 0, sizeof(Link));

    /*
     * Increase uplink's statistic counter.
     */
    Link.aka.zone  = p_from->zone;
    Link.aka.net   = p_from->net;
    Link.aka.node  = p_from->node;
    Link.aka.point = p_from->point;
    if (SearchNode(Link.aka)) {
	StatAdd(&nodes.MailRcvd, 1);
	UpdateNode();
	SearchNode(Link.aka);
    }

    buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
    marea = NULL;

    /*
     * First read the message for kludges we need.
     */
    rewind(fp);

    FirstLine = TRUE;
    while ((fgets(buf, MAX_LINE_LENGTH, fp)) != NULL) {

	size += strlen(buf);
	Striplf(buf);

	/*
	 * Check if message is echomail and if the areas exists.
	 */
	if (FirstLine && (!strncmp(buf, "AREA:", 5))) {

	    marea = xstrcpy(tu(buf + 5));

	    if (orig == NULL) {
		Syslog('!', "Echomail without Origin line");
		echo_bad++;
		echo_in++;
		bad = 3;
		free(buf);
		free(marea);
		return 3;
	    }

	    if (!SearchMsgs(marea)) {
		UpdateNode();
		Syslog('m', "Unknown echo area %s", marea);
		if (!create_msgarea(marea, p_from)) {
		    WriteError("Create echomail area %s failed", marea);
		    bad = 6;
		}
		if (bad == 0) {	
		    SearchNode(Link.aka);
		    if (!SearchMsgs(marea)) {
			WriteError("Unknown echo area %s", marea);
			echo_bad++;
			echo_in++;
			bad = 4;
			free(marea);
			free(buf);
			return 4;
		    }
		}	    
	    }
	    echomail = TRUE;
	    free(marea);
	}
	if (*buf != '\001')
	    FirstLine = FALSE;
    } /* end of checking kludges */

    if (size >= 32768) {
	if (echomail) {
	    Syslog('!', "WARNING: Echo area %s from \"%s\" subj \"%s\" size %d", msgs.Tag, f->name, subj, size);
	} else {
	    Syslog('!', "WARNING: Net from \"%s\" subj \"%s\" size %d", f->name, subj, size);
	}
    }

    if (echomail) {
	if (bad) {
	    /*
	     * Bad, load the badmail record
	     */
	    if ((strlen(CFG.badboard) == 0) && !SearchBadBoard()) {
		Syslog('+', "No badmail area, killing message");
		free(buf);
		return 6;
	    }
	    UpdateMsgs();
	}
	/*
	 * At this point, the destination zone is not yet set.
	 */
	if (!t->zone)
	    t->zone = tzone;
	rc = postecho(p_from, f, t, orig, subj, mdate, flags, cost, fp, TRUE, bad);
    } else
	rc = postnetmail(fp, f, t, orig, subj, mdate, flags, TRUE, p_from->zone, tzone);

    free(buf);
    return rc;
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
		mbse_colour(LIGHTGREEN, BLACK);
		printf("Tossing packet %s\n", fn);
	}

	if ((pkt = fopen(fn, "r")) == 0) {
		WriteError("$Cannot open %s", fn);
		return 1;
	}

	memset(&from, 0, sizeof(faddr));
	memset(&to,   0, sizeof(faddr));

	if (((rc = getheader(&from, &to, pkt, fn, FALSE)) != 0)) {
		WriteError("%s, aborting",
				(rc == 1)?"wrong header type":
				(rc == 2)?"bad packet header":
				(rc == 3)?"packet is not for us":
				(rc == 4)?"bad password":
				"bad packet");
		fclose(pkt);
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
    char	    buf[MAX_LINE_LENGTH +1], *orig = NULL, *p, *l, *r, *subj = NULL;
    int		    tmp, rc, maxrc = 0, result, flags, cost;
    static faddr    f, t;
    faddr	    *o;
    time_t	    mdate = 0L;
    FILE	    *fp;
    unsigned char   buffer[0x0e];

    Nopper();

    result = fread(&buffer, 1, sizeof(buffer), pkt);
    if (result == 0) {
	Syslog('m', "Zero bytes message, assume end of pkt");
	return 0;
    }

    switch (tmp = (buffer[0x01] << 8) + buffer[0x00]) {
	case 0:	if (result == 2)
		    return 0;
		else {
		    Syslog('!', "Junk after logical end of packet, skipped");
		    return 5;

		}
	case 2:	break;

	default:Syslog('!', "bad message type: 0x%04x",tmp);
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

    if ((fp = tmpfile()) == NULL) {
	WriteError("$unable to open temporary file");
	return 4;
    }

    /*
     * Read the text from the .pkt file
     */
    while (aread(buf,sizeof(buf)-1,pkt)) {

	fputs(buf, fp);

	/*
	 * Extract info from Origin line if found.
	 */
	if (!strncmp(buf," * Origin:",10)) {
	    p=buf+10;
	    while (*p == ' ') 
		p++;
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

    rc = importmsg(p_from, &f, &t, orig, subj, mdate, flags, cost, fp, p_to->zone);
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

    if (subj)
	free(subj);
    subj = NULL;

    if (orig)
	free(orig);
    orig = NULL;

    if (feof(pkt) || ferror(pkt)) {
	WriteError("Unexpected end of packet");
	return 5;
    }
    return 1;
}


