/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Post echomail message.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbmsgs.h"
#include "../lib/dbdupe.h"
#include "../lib/dbuser.h"
#include "../lib/dbftn.h"
#include "ftn2rfc.h"
#include "postecho.h"
#include "storeecho.h"
#include "addpkt.h"
#include "rollover.h"
#include "qualify.h"



/*
 * External declarations
 */
extern	int	do_quiet;		/* No tty output	*/
extern	int	do_unsec;		/* Toss unsecure	*/
extern	int	check_dupe;		/* Check dupes		*/
extern	time_t	t_start;		/* Reference time	*/
extern	int	echo_in;		/* Echomail processed	*/
extern	int	echo_out;		/* Echomail forwarded	*/
extern	int	echo_bad;		/* Bad echomail		*/


#define	MAXPATH	73
#define	MAXSEEN	70


int  EchoOut(fidoaddr, char *, char *, char *, FILE *, int, int, time_t);




/*
 *  Add echomail mesage to the queue.
 */
int EchoOut(fidoaddr aka, char *toname, char *fromname, char *subj, FILE *fp, int flags, int cost, time_t date)
{
    char    *buf, ext[4];
    FILE    *qp;
    faddr   *From, *To;
    int	    rc;

    /*
     * Pack flavor for echomail packets.
     */
    memset(&ext, 0, sizeof(ext));
    if (nodes.PackNetmail)
	sprintf(ext, (char *)"qqq");
    else if (nodes.Crash)
	sprintf(ext, (char *)"ccc");
    else if (nodes.Hold)
	sprintf(ext, (char *)"hhh");
    else
	sprintf(ext, (char *)"nnn");

    if ((qp = OpenPkt(msgs.Aka, aka, (char *)ext)) == NULL) {
	WriteError("EchoOut(): OpenPkt failed");
	return 1;
    }

    From = fido2faddr(msgs.Aka);
    To = fido2faddr(aka);
    rc = AddMsgHdr(qp, From, To, flags, cost, date, toname, fromname, subj);
    tidy_faddr(To);
    tidy_faddr(From);
    if (rc) {
	WriteError("EchoOut(): AddMsgHdr failed");
	return 1;
    }

    rewind(fp);
    buf = calloc(2049, sizeof(char));

    while ((fgets(buf, 2048, fp)) != NULL) {
	Striplf(buf);
	fprintf(qp, "%s\r", buf);
    }

    free(buf);
    putc(0, qp);
    fsync(fileno(qp));
    fclose(qp);
    return 0;
}



/*
 * Post echomail message, forward if needed.
 * pkt_from, from, to, subj, orig, mdate, flags, cost, file
 * The msgs record must be in memory.
 *
 *  1 - Cannot open message base.
 *  4 - Rejected echomail message.
 *
 * For echomail, the crc32 is calculated over the ^AREA kludge, subject, 
 * message date, origin line, message id.
 */
int postecho(faddr *p_from, faddr *f, faddr *t, char *orig, char *subj, time_t mdate, int flags, int cost, FILE *fp, int tonews)
{
    char	    *buf, *msgid = NULL, *reply = NULL, *p, *q, sbe[16];
    int		    First = TRUE, rc = 0, i, kludges = TRUE, dupe = FALSE, bad = TRUE, seenlen, oldnet;
    faddr	    *Faddr;
    unsigned long   crc;
    sysconnect	    Link;
    fa_list	    *sbl = NULL, *ptl = NULL, *tmpl;
    qualify	    *qal = NULL, *tmpq;
    FILE	    *nfp, *qp;
    time_t	    ddate;

    memset(&Link, 0, sizeof(Link));
    crc = 0xffffffff;
    echo_in++;

    /*
     *  p_from is set for tossed echomail, it is NULL for local posted echomail and gated news.
     */
    if (p_from) {
	while (GetMsgSystem(&Link, First)) {
	    First = FALSE;
	    if ((p_from->zone == Link.aka.zone) && (p_from->net  == Link.aka.net) && (p_from->node == Link.aka.node)) {
		bad = FALSE;
		break;
	    }
	}
	if (bad && (msgs.UnSecure || do_unsec)) {
	    bad = FALSE;
	    memset(&Link, 0, sizeof(Link));
	    Syslog('!', "Warning, unsecure echomail from %s accepted in area %s", ascfnode(p_from, 0x1f), msgs.Tag);
	}
	if (bad) {
	    Syslog('+', "Node %s not connected to area %s", ascfnode(p_from, 0x1f), msgs.Tag);
	    echo_bad++;
	    return 4;
	}
	if (Link.cutoff && !bad) {
	    Syslog('+', "Echomail from %s in %s refused, cutoff", ascfnode(p_from, 0x1f), msgs.Tag);
	    bad = TRUE;
	    echo_bad++;
	    return 4;
	}
	if (!Link.receivefrom && !bad) {
	    Syslog('+', "Echomail from %s in %s refused, read only", ascfnode(p_from, 0x1f), msgs.Tag);
	    bad = TRUE;
	    echo_bad++;
	    return 4;
	}
    } else {
	/*
	 *  Fake the zone entry to be our own zone, this prevents
	 *  zonegate behaviour. It's also not a bad message yet.
	 */
	Link.aka.zone = msgs.Aka.zone;
	bad = FALSE;
    }

    if (CFG.toss_old && ((t_start - mdate) / 86400) > CFG.toss_old) {
	Syslog('+', "Rejecting msg: too old, %s", rfcdate(mdate));
	bad = TRUE;
	echo_bad++;
	return 4;
    }

    /*
     * Read the message for kludges we need.
     */
    buf = calloc(2049, sizeof(char));
    First = TRUE;
    rewind(fp);
    while ((fgets(buf, 2048, fp)) != NULL) {

	Striplf(buf);

	if (First && (!strncmp(buf, "AREA:", 5))) {
	    crc = upd_crc32(buf, crc, strlen(buf));
	    First = FALSE;
	}
	if (!strncmp(buf, "\001MSGID: ", 8)) {
	    msgid = xstrcpy(buf + 8);
	}
	if (!strncmp(buf, "\001REPLY: ", 8))
	    reply = xstrcpy(buf + 8);
	if (!strncmp(buf, "SEEN-BY:", 8)) {
	    p = xstrcpy(buf + 9);
	    fill_list(&sbl, p, NULL);
	    free(p);
	}
	if (!strncmp(buf, "\001PATH:", 6)) {
	    p = xstrcpy(buf + 7);
	    fill_path(&ptl, p);
	    free(p);
	}
    } /* end of checking kludges */


    /*
     * Further dupe checking.
     */
    crc = upd_crc32(subj, crc, strlen(subj));
    if (orig == NULL)
	Syslog('!', "No origin line found");
    else
	crc = upd_crc32(orig, crc, strlen(orig));

    /*
     * Some tossers don't bother the seconds in the message, also some
     * rescanning software changes the seconds of a message. Do the
     * timestamp check without the seconds.
     */
    ddate = mdate - (mdate % 60);
    crc = upd_crc32((char *)&ddate, crc, sizeof(ddate));

    if (msgid != NULL) {
	crc = upd_crc32(msgid, crc, strlen(msgid));
    } else {
	if (check_dupe) {
	    /*
	     *  If a MSGID is missing it is possible that dupes from some offline
	     *  readers slip through because these readers use the same date for
	     *  each message. In this case the message text is included in the
	     *  dupecheck. Redy Rodriguez.
	     */
	    rewind(fp);
	    while ((fgets(buf, 2048, fp)) != NULL) {
		Striplf(buf);
		if (strncmp(buf, "---", 3) == 0)
		    break;
		if ((strncmp(buf, "\001", 1) != 0 ) && (strncmp(buf,"AREA:",5) != 0 )) 
		    crc = upd_crc32(buf, crc , strlen(buf));        
	    }     
	}   
    }
    if (check_dupe)
	dupe = CheckDupe(crc, D_ECHOMAIL, CFG.toss_dupes);
    else
	dupe = FALSE;


    if (!dupe && !msgs.UnSecure && !do_unsec) {
	/*
	 * Check if the message is for us. Don't check point address,
	 * echomail messages don't have point destination set.
	 */
	if ((msgs.Aka.zone != t->zone) || (msgs.Aka.net != t->net) || (msgs.Aka.node != t->node)) {
	    bad = TRUE;
	    /*
	     * If we are a hub or host and have all our echomail
	     * connected to the hub/host aka, echomail from points
	     * under a nodenumber aka isn't accepted. The match
	     * must be further tested.
	     */
	    if ((msgs.Aka.zone == t->zone) && (msgs.Aka.net == t->net)) {
		for (i = 0; i < 40; i++) {
		    if ((CFG.akavalid[i]) && (CFG.aka[i].zone == t->zone) && 
			(CFG.aka[i].net  == t->net) && (CFG.aka[i].node == t->node))
			bad = FALSE; /* Undo the result */
		}
	    }
	}
	if (bad) {
	    echo_bad++;
	    WriteError("Msg in %s not for us (%s) but for %s", msgs.Tag, aka2str(msgs.Aka), ascfnode(t,0x1f));
	    free(buf);
	    if (msgid)
		free(msgid);
	    if (reply)
		free(reply);
	    return 4;
	}
    }


    /*
     *  The echomail message is accepted for post/forward/gate
     */
    if (!dupe) {

	if (msgs.Aka.zone != Link.aka.zone) {
	    /*
	     * If it is a zonegated echomailmessage the SEEN-BY lines
	     * are stripped off including that of the other zone's
	     * gate. Add the gate's aka to the SEEN-BY
	     */
	    Syslog('m', "Gated echomail, clean SB");
	    tidy_falist(&sbl);
	    sprintf(sbe, "%u/%u", Link.aka.net, Link.aka.node);
	    Syslog('m', "Add gate SB %s", sbe);
	    fill_list(&sbl, sbe, NULL);
	}

	/*
	 * Add more aka's to SEENBY if in the same zone as our system.
	 * When ready filter dupe's, there is at least one.
	 */
	for (i = 0; i < 40; i++) {
	    if (CFG.akavalid[i] && (msgs.Aka.zone == CFG.aka[i].zone) && (CFG.aka[i].point == 0) &&
		!((msgs.Aka.net == CFG.aka[i].net) && (msgs.Aka.node == CFG.aka[i].node))) {
		sprintf(sbe, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
		fill_list(&sbl, sbe, NULL);
	    }
	}
	uniq_list(&sbl);
    }


    /*
     * Add our system to the path for later export.
     */
    sprintf(sbe, "%u/%u", msgs.Aka.net, msgs.Aka.node);
    fill_path(&ptl, sbe);
    uniq_list(&ptl);	/* remove possible duplicate own aka */

    /*
     * Build a list of qualified systems to receive this message.
     * Complete the SEEN-BY lines.
     */
    First = TRUE;
    while (GetMsgSystem(&Link, First)) {
	First = FALSE;
	if ((Link.aka.zone) && (Link.sendto) && (!Link.pause) && (!Link.cutoff)) {
	    Faddr = fido2faddr(Link.aka);
	    if (p_from == NULL) {
		fill_qualify(&qal, Link.aka, FALSE, in_list(Faddr, &sbl, FALSE));
	    } else {
		fill_qualify(&qal, Link.aka, ((p_from->zone  == Link.aka.zone) && 
		      (p_from->net   == Link.aka.net) && (p_from->node  == Link.aka.node) && 
		      (p_from->point == Link.aka.point)), in_list(Faddr, &sbl, FALSE));
	    }
	    tidy_faddr(Faddr);
	}
    }


    /*
     *  Add SEEN-BY for nodes qualified to receive this message.
     *  When ready, filter the dupes and sort the SEEN-BY entries.
     */
    for (tmpq = qal; tmpq; tmpq = tmpq->next) {
	if (tmpq->send) {
	    sprintf(sbe, "%u/%u", tmpq->aka.net, tmpq->aka.node);
	    fill_list(&sbl, sbe, NULL);
	}
    }
    uniq_list(&sbl);
    sort_list(&sbl);


    /*
     *  Create a new tmpfile with a copy of the message
     *  without original PATH and SEENBY lines, add the
     *  new PATH and SEENBY lines.
     */
    rewind(fp);
    if ((nfp = tmpfile()) == NULL)
	WriteError("$Unable to open tmpfile");
    while ((fgets(buf, 2048, fp)) != NULL) {
	Striplf(buf);
	fprintf(nfp, "%s", buf);
	/*
	 * Don't write SEEN-BY and PATH lines
	 */
	if (strncmp(buf, " * Origin:", 10) == 0)
	    break;
	fprintf(nfp, "\n");
    }


    /*
     * Now add new SEEN-BY and PATH lines
     */
    seenlen = MAXSEEN + 1;
    /*
     * Ensure that it will not match for the first entry.
     */
    oldnet = sbl->addr->net - 1;
    for (tmpl = sbl; tmpl; tmpl = tmpl->next) {
	if (tmpl->addr->net == oldnet)
	    sprintf(sbe, " %u", tmpl->addr->node);
	else
	    sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	oldnet = tmpl->addr->net;
	seenlen += strlen(sbe);
	if (seenlen > MAXSEEN) {
	    seenlen = 0;
	    fprintf(nfp, "\nSEEN-BY:");
	    sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	    seenlen = strlen(sbe);
	}
	fprintf(nfp, "%s", sbe);
    }

    seenlen = MAXPATH + 1;
    /* 
     * Ensure it will not match for the first entry
     */
    oldnet = ptl->addr->net - 1;
    for (tmpl = ptl; tmpl; tmpl = tmpl->next) {
	if (tmpl->addr->net == oldnet)
	    sprintf(sbe, " %u", tmpl->addr->node);
	else
	    sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	oldnet = tmpl->addr->net;
	seenlen += strlen(sbe);
	if (seenlen > MAXPATH) {
	    seenlen = 0;
	    fprintf(nfp, "\n\001PATH:");
	    sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	    seenlen = strlen(sbe);
	}
	fprintf(nfp, "%s", sbe);
    }
    fprintf(nfp, "\n");
    fflush(nfp);
    rewind(nfp);


    /*
     * Import this echomail, even if it's bad or a dupe.
     */
    if ((rc = storeecho(f, t, mdate, flags, subj, msgid, reply, bad, dupe, nfp)) || bad || dupe) {
	/*
	 *  Store failed or it was bad or a dupe. Only log failed store.
	 */
	if (rc)
	    WriteError("Store echomail in JAM base failed");
	tidy_falist(&sbl);
	tidy_falist(&ptl);
	tidy_qualify(&qal);
	free(buf);
	if (msgid)
	    free(msgid);
	if (reply)
	    free(reply);
	fclose(nfp);
	return rc;
    }

    /*
     * Forward to other links
     */
    for (tmpq = qal; tmpq; tmpq = tmpq->next) {
	if (tmpq->send) {
	    if (SearchNode(tmpq->aka)) {
		StatAdd(&nodes.MailSent, 1L);
		UpdateNode();
		SearchNode(tmpq->aka);
		echo_out++;
		if (EchoOut(tmpq->aka, t->name, f->name, subj, nfp, flags, cost, mdate))
		    WriteError("Forward echomail to %s failed", aka2str(tmpq->aka));
	    } else {
		WriteError("Forward echomail to %s failed, noderecord not found", aka2str(tmpq->aka));
	    }
	}
    }

    /*
     *  Gate to newsserver
     */
    if (strlen(msgs.Newsgroup) && tonews) {
	rewind(nfp);
	qp = tmpfile();
        while ((fgets(buf, 2048, nfp)) != NULL) {
	    Striplf(buf);
	    if (kludges && (buf[0] != '\001') && strncmp(buf, "AREA:", 5)) {
		kludges = FALSE;
		q = xstrcpy(Msg.From);
		for (i = 0; i < strlen(q); i++)
		    if (q[i] == ' ')
			q[i] = '_';
		fprintf(qp, "From: %s@%s\n", q, ascinode(f, 0x1f));
		fprintf(qp, "Subject: %s\n", Msg.Subject);
		fprintf(qp, "To: %s\n", Msg.To);
		free(q);
		fprintf(qp, "\n");
	    }
	    fprintf(qp, "%s\n", buf);
	}
	rewind(qp);
	ftn2rfc(f, t, subj, orig, mdate, flags, qp);
	fclose(qp);
    }
    fclose(nfp);

    /*
     * Free memory used by SEEN-BY, ^APATH and Qualified lines.
     */
    tidy_falist(&sbl);
    tidy_falist(&ptl);
    tidy_qualify(&qal);

    if (rc < 0) 
	rc =-rc;
    free(buf);
    if (msgid)
	free(msgid);
    if (reply)
	free(reply);
    return rc;
}



