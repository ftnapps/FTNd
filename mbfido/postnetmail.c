/*****************************************************************************
 *
 * $Id: postnetmail.c,v 1.21 2005/08/28 14:10:06 mbse Exp $
 * Purpose ...............: Post Netmail message from temp file
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "tracker.h"
#include "addpkt.h"
#include "storenet.h"
#include "ftn2rfc.h"
#include "areamgr.h"
#include "filemgr.h"
#include "ping.h"
#include "bounce.h"
#include "postemail.h"



/*
 * Global variables
 */
extern	int	net_in;			/* Total netmails processed	*/
extern	int	net_out;		/* Netmails exported		*/
extern	int	net_imp;		/* Netmails imported            */
extern	int	net_bad;		/* Bad netmails			*/



/*
 *  Post netmail message for temp file. The tempfile is an FTN style message.
 *
 *  0 - All seems well.
 *  1 - Can't access messagebase.
 *  2 - Can't find netmail board.
 *
 */
int postnetmail(FILE *fp, faddr *f, faddr *t, char *orig, char *subject, time_t mdate, 
	int flags, int DoPing, unsigned int fzone, unsigned int tzone)
{
    char    	*p, *msgid = NULL, *reply = NULL, *flagstr = NULL;
    char    	name[37], *buf, *l, *r, *q, System[37], ext[4];
    int		result = 1, email = FALSE, fmpt = 0, topt = 0;
    faddr	*ta, *ra;
    fidoaddr	na, routeto, Orig;
    FILE	*sfp, *net;
    time_t	now;
    struct tm	*tm;

    Syslog('m', "Post netmail from: %s", ascfnode(f, 0xff));
    Syslog('m', "Post netmail to  : %s", ascfnode(t, 0xff));
    Syslog('m', "Post netmail subj: %s", MBSE_SS(subject));
    net_in++;

    /*
     *  Extract MSGID and REPLY kludges from this netmail.
     */
    buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
    rewind(fp);
    while ((fgets(buf, MAX_LINE_LENGTH, fp)) != NULL) {
	Striplf(buf);
	Syslogp('M', printable(buf, 0));
	if (!strncmp(buf, "\001MSGID: ", 8)) {
	    msgid = xstrcpy(buf + 8);
	    /*
	     *  Extra test to see if the mail comes from a pointaddress.
	     */
	    l = strtok(buf," \n");
	    l = strtok(NULL," \n");
	    if ((ta = parsefnode(l))) {
		if (ta->net == f->net && ta->node == f->node && !fmpt && ta->point) {
		    Syslog('m', "Setting pointinfo (%d) from MSGID", ta->point);
		    fmpt = f->point = ta->point;
		}
		if ((ta->net == f->net) && (ta->node == f->node) && (f->zone == 0)) {
		    /*
		     * Missing zone info, maybe later we will see a INTL kludge or so, but for
		     * now, just in case we fix it. And we need that for some Aka collecting
		     * sysop who doesn't know how to configure his system right.
		     */
		    Syslog('m', "No from zone set, setting zone %d from MSGID", ta->zone);
		    f->zone = ta->zone;
		    /*
		     * 99.9 % chance that the destination zone is also missing.
		     */
		    if (t->zone == 0) {
			t->zone = ta->zone;
			Syslog('m', "No dest zone set, setting zone %d from MSGID", ta->zone);
		    }
		}
		tidy_faddr(ta);
	    }
	    if (msgid)
		free(msgid);
	    msgid = NULL;
	}
	if (!strncmp(buf, "\001FMPT", 5)) {
	    p = strtok(buf, " \n");
	    p = strtok(NULL, " \n");
	    fmpt = atoi(p);
	}
	if (!strncmp(buf, "\001TOPT", 5)) {
	    p = strtok(buf, " \n");
	    p = strtok(NULL, " \n");
	    topt = atoi(p);
	}
	if (!strncmp(buf, "\001REPLY: ", 8))
	    reply = xstrcpy(buf + 8);

	/*
	 * Check DOMAIN and INTL kludges
	 */
	if (!strncmp(buf, "\001DOMAIN", 7)) {
	    l = strtok(buf," \n");
	    l = strtok(NULL," \n");
	    p = strtok(NULL," \n");
	    r = strtok(NULL," \n");
	    q = strtok(NULL," \n");
	    if ((ta = parsefnode(p))) {
		t->point = ta->point;
		t->node = ta->node;
		t->net = ta->net;
		t->zone = ta->zone;
		tidy_faddr(ta);
	    }
	    t->domain = xstrcpy(l);
	    if ((ta = parsefnode(q))) {
		f->point = ta->point;
		f->node = ta->node;
		f->net = ta->net;
		f->zone = ta->zone;
		tidy_faddr(ta);
	    }
	    f->domain = xstrcpy(r);
	} else {
	    if (!strncmp(buf, "\001INTL", 5)) {
		l = strtok(buf," \n");
		l = strtok(NULL," \n");
		r = strtok(NULL," \n");
		if ((ta = parsefnode(l))) {
		    t->point = ta->point;
		    t->node = ta->node;
		    t->net = ta->net;
		    t->zone = ta->zone;
		    if (ta->domain) {
			if (t->domain)
			    free(t->domain);
			t->domain = ta->domain;
			ta->domain = NULL;
		    }
		    tidy_faddr(ta);
		}
		if ((ta = parsefnode(r))) {
		    f->point = ta->point;
		    f->node = ta->node;
		    f->net = ta->net;
		    f->zone = ta->zone;
		    if (ta->domain) {
			if (f->domain)
			    free(f->domain);
			f->domain = ta->domain;
			ta->domain = NULL;
		    }
		    tidy_faddr(ta);
		}
	    }
	}

	/*
	 * Check FLAGS kludge
	 */
	if (!strncmp(buf, "\001FLAGS ", 7)) {
	    flagstr = xstrcpy(buf + 7);
	    Syslog('m', "^aFLAGS %s", flagstr);
	}
	if (!strncmp(buf, "\001FLAGS: ", 8)) {
	    flagstr = xstrcpy(buf + 8);
	    Syslog('m', "^aFLAGS: %s", flagstr);
	}

	/*
	 * Check for X-FTN- kludges, this could be gated email.
	 * This should be impossible.
	 */
	if (!strncmp(buf, "\001X-FTN-", 7)) {
	    email = TRUE;
	    Syslog('?', "Warning: detected ^aX-FTN- kludge in netmail");
	}
    }
    free(buf);

    /*
     *  Only set point info if there was any info.
     *  GoldED doesn't set FMPT and TOPT kludges.
     */
    if (fmpt)
	f->point = fmpt;
    if (topt)
	t->point = topt;

    /*
     * If zone info is still missing, set the defaults which came from the
     * original mail packet addressing.
     */
    if (fzone && (f->zone == 0))
	f->zone = fzone;
    if (tzone && (t->zone == 0))
	t->zone = tzone;

    l = xstrcpy(ascfnode(f, 0xff));
    r = xstrcpy(ascfnode(t, 0xff));
    Syslog('+', "Netmail from \"%s\" to \"%s\"", l, r);
    free(l);
    free(r);

    memset(&na, 0, sizeof(na));
    na.zone  = t->zone;
    na.net   = t->net;
    na.node  = t->node;
    na.point = t->point;
    if (SearchFidonet(na.zone))
	snprintf(na.domain, 13, "%s", fidonet.domain);

    switch(TrackMail(na, &routeto)) {
	case R_LOCAL:
	    /*
	     *  Check the To: field.
	     */
	    if (strchr(t->name, '@') != NULL) {
		snprintf(name, 36, "%s", strtok(t->name, "@"));
		snprintf(System, 36, "%s", strtok(NULL, "\000"));
		email = TRUE;
	    } else {
		snprintf(name, 36, "%s", t->name);
		snprintf(System, 36, "%s", CFG.sysdomain);
	    }

	    if (email) {
		/*
		 * Send this netmail via ftn2rfc -> postemail.
		 */
		return result = ftn2rfc(f, t, subject, orig, mdate, flags, fp);
	    }

	    /*
	     * If message to "sysop" or "postmaster" replace it
	     * with the sysops real name.
	     */
	    if ((strncasecmp(name, "sysop", 5) == 0) || 
		(strcasecmp(name, "postmaster") == 0) ||
		(strcasecmp(name, "coordinator") == 0)) {
		Syslog('+', "  Readdress from %s to %s", name, CFG.sysop_name);
		snprintf(name, 36, "%s", CFG.sysop_name);
	    }

	    /*
	     * If the message is a service message, check the
	     * services database to see what action is needed.
	     * First make sure that the right noderecord is loaded.
	     */
	    (void)noderecord(f);
	    p = calloc(PATH_MAX, sizeof(char));
	    snprintf(p, PATH_MAX, "%s/etc/service.data", getenv("MBSE_ROOT"));
	    if ((sfp = fopen(p, "r")) == NULL) {
		WriteError("$Can't open %s", p);
	    } else {
		fread(&servhdr, sizeof(servhdr), 1, sfp);
		while (fread(&servrec, servhdr.recsize, 1, sfp) == 1) {
		    if ((strncasecmp(servrec.Service, name, strlen(servrec.Service)) == 0) && servrec.Active) {
			switch (servrec.Action) {
			    case AREAMGR:   result = AreaMgr(f, t, msgid, subject, mdate, flags, fp);
					    break;
			    case FILEMGR:   result = FileMgr(f, t, msgid, subject, mdate, flags, fp);
					    break;
			    case EMAIL:     result = ftn2rfc(f, t, subject, orig, mdate, flags, fp);
					    if (result) {
						if (result == 2)
						    Bounce(f, t, fp, (char *)"Could not post email");
						else
						    Bounce(f, t, fp, (char *)"Could not convert to email");
					    }
					    break;
			}
			Syslog('m', "Handled service %s, rc=%d", servrec.Service, result);
			fclose(sfp);
			return result;
		    }
		}
		fclose(sfp);
	    }
	    free(p);

	    /*
	     * Ping function
	     */
	    if (!strcasecmp(name, (char *)"ping") && DoPing) {
		return Ping(f, t, fp, FALSE);
	    }

	    /*
	     * Check userlist real names, handles, unix names.
	     * Import if one fits.
	     */
	    if (SearchUser(name)) {
		return storenet(f, t, mdate, flags, subject, msgid, reply, fp, flagstr);
	    }

	    Syslog('+', "  \"%s\" is not a known BBS user", name);
	    /*
	     *  Unknown, readdress it to the sysop.
	     */
	    net_bad++;
	    Syslog('+', "  Readdress from %s to %s", name, CFG.sysop_name);
	    snprintf(name, 36, "%s", CFG.sysop_name);
	    if (SearchUser(name)) {
		return storenet(f, t, mdate, flags, subject, msgid, reply, fp, flagstr);
	    } else {
		WriteError("Readdress import failed, sysop doesn't exist. CHECK YOUR SETUP");
		return 0;
	    }
	    break;

	case R_DIRECT:
	case R_ROUTE:
	    Syslog('+', "Route netmail via %s", aka2str(routeto));
            if (!strcasecmp(t->name, (char *)"ping") && DoPing) {
                Syslog('+', "In transit \"Ping\" message detected");
                Ping(f, t, fp, TRUE);
                (void)noderecord(f);
            }

	    /*
	     * Forward this message. Will not work for unknown
	     * direct links.
	     */
	    if (SearchNode(routeto)) {
		memset(&Orig, 0, sizeof(Orig));
		ra = fido2faddr(routeto);
		ta = bestaka_s(ra);
		Orig.zone  = ta->zone;
		Orig.net   = ta->net;
		Orig.node  = ta->node;
		Orig.point = ta->point;
		tidy_faddr(ra);
		tidy_faddr(ta);

		memset(&ext, 0, sizeof(ext));
		if (nodes.PackNetmail)
		    snprintf(ext, 4, (char *)"qqq");
		else if (nodes.Crash)
		    snprintf(ext, 4, (char *)"ccc");
		else if (nodes.Hold)
		    snprintf(ext, 4, (char *)"hhh");
		else 
		    snprintf(ext, 4, (char *)"nnn");

		if ((net = OpenPkt(Orig , routeto, (char *)ext)) == NULL) {
		    net_bad++;
		    WriteError("Can't create netmail");
		    return 0;
		}
	    } else {
		/*
		 * If it's not a direct link, create a outbound
		 * .pkt anyway, better then that this mail is 
		 * lost. It gets the normal status, it might
		 * get delivered during ZMH this way.
		 */
		Syslog('!', "Warning: not a direct link, check setup");
		memset(&Orig, 0, sizeof(Orig));
		ra = fido2faddr(routeto);
		ta = bestaka_s(ra);
		Orig.zone  = ta->zone;
		Orig.net   = ta->net;
		Orig.node  = ta->node;
		Orig.point = ta->point;
		tidy_faddr(ra);
		tidy_faddr(ta);

		if ((net = OpenPkt(Orig , routeto, (char *)"nnn")) == NULL) {
		    net_bad++;
		    WriteError("Can't create netmail");
		    return 0;
		}
	    }

	    /*
	     * Now start forward.
	     */
	    Syslog('m', "Net from  %s", ascfnode(f, 0xff));
	    Syslog('m', "Net to    %s", ascfnode(t, 0xff));
	    Syslog('m', "Net flags %08x", flags);
	    Syslog('m', "Net subj  %s", subject);

	    if (AddMsgHdr(net, f, t, flags, 0, mdate, t->name, f->name, subject)) {
		WriteError("Can't write message header");
		net_bad++;
		return 0;
	    }
	    rewind(fp);

	    /*
	     * Copy all text including kludges, when
	     * finished, insert our ^aVia line.
	     */
	    buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
	    while ((fgets(buf, MAX_LINE_LENGTH, fp)) != NULL)
		fprintf(net, "%s\r", buf);

	    now = time(NULL);
	    tm = gmtime(&now);
	    ta = bestaka_s(t);
	    fprintf(net, "\001Via %s @%d%02d%02d.%02d%02d%02d.00.UTC mbfido %s\r", 
		ascfnode(ta, 0x1f), tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, VERSION);
	    tidy_faddr(ta);

	    putc(0, net);
	    fclose(net);
	    free(buf);
	    net_out++;
	    Syslog('m', "Forward done.");
	    return 0;

	default:
	    /*
	     * If we came this far, there's definitly something wrong
	     * with this netmail.
	     */
	    WriteError("No ROUTE for this netmail");
	    net_bad++;
	    flags |= M_ORPHAN;
	    return storenet(f, t, mdate, flags, subject, msgid, reply, fp, flagstr);
	    break;
    }

    /* Never reached */
    return result;
}


