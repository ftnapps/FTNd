/*****************************************************************************
 *
 * $Id: rfc2ftn.c,v 1.19 2008/08/31 21:10:51 mbse Exp $
 * Purpose ...............: Convert RFC to FTN
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
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbinet.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "mkftnhdr.h"
#include "hash.h"
#include "msgflags.h"
#include "rfc2ftn.h"
#include "commands.h"

#ifndef	USE_NEWSGATE

#define MAXHDRSIZE 2048
#define	MAXSEEN 70
#define	MAXPATH 73


/*
 *  Global variables
 */
static int	ftnorigin;
static int	removemime;
static int	removemsgid;
static int	removeref;
static int	removeinreply;
static int	removereplyto;
static int	removereturnto;



/*
 *  External variables
 */
extern	char	*replyaddr;
extern	int	do_mailout;
extern	int	grecno;
extern	char	currentgroup[];

/*
 *  Internal functions
 */
int	needputrfc(rfcmsg *, int);



int charwrite(char *, FILE *);
int charwrite(char *s, FILE *fp)
{
    if ((strlen(s) >= 3) && (strncmp(s,"---",3) == 0) && (s[3] != '-')) {
	putc('-',fp);
	putc(' ',fp);
    }

    fwrite(s, strlen(s), 1, fp);
    return 0;
}



/* 
 * write (multiline) header to kluge: change \n to ' ' and end line with \n
 */
int kludgewrite(char *, FILE *);
int kludgewrite(char *s, FILE *fp)
{
        while (*s) {
		if (*s == '\r')
			putc('\n', fp);
		else {
                	if (*s != '\n')
                        	putc(*s, fp);
                	else if (*(s+1))
                        	putc(' ',fp);
		}
                s++;
        }
        putc('\n',fp);
        return 0;
}


int findorigmsg(char *msgid, char *o)
{
    unsigned int    i, start, end;
    char	    *gen2;

    if (msgid == NULL) {
	return 0;
    }

    if (!Msg_Open(msgs.Base)) {
	return 0;
    }
    Msg_Number();
    Msg_Highest();
    Msg_Lowest();

    if (MsgBase.Open == FALSE) {
	Syslog('-', "Base closed");
	return 0;
    }

    strcpy(currentgroup,msgs.Newsgroup);
    start = MsgBase.Lowest;
    end   = MsgBase.Highest;

    gen2 = calloc(strlen(msgid)+1,sizeof(char));
    strcpy(gen2, strchr(msgid,'<'));
    for (i = start; i <= end; i++) {
	if (Msg_ReadHeader(i)) {
	    if (strncmp(gen2,make_msgid(Msg.Msgid),strlen(gen2)-1) == 0) {
		Syslog('m',"Found msgid: %s",make_msgid(Msg.Msgid));
//		realloc(o,(strlen(Msg.Msgid)+1)* sizeof(char));
		strcpy(o,Msg.Msgid);
		free(gen2);
		return 1;
	    }
        }
    }
    free(gen2);
    return 0;
}



/*
 *  Input a RFC news message.
 */
int rfc2ftn(FILE *fp)
{
    char            sbe[16], *p, *q, *temp, *origin, newsubj[4 * (MAXSUBJ+1)], *oldsubj;
    int             i, rc, newsmode, seenlen, oldnet;
    rfcmsg          *msg = NULL, *tmsg, *tmp;
    ftnmsg          *fmsg = NULL;
    FILE            *ofp, *qfp;
    fa_list         *sbl = NULL, *ptl = NULL, *tmpl;
    faddr           *ta, *fta;
    unsigned int    svmsgid, svreply;
    int             sot_kludge = FALSE, eot_kludge = FALSE, tinyorigin = FALSE;
    int             needsplit, hdrsize, datasize, splitpart, forbidsplit, rfcheaders;
    time_t          Now;
    struct tm	    *l_date;
    char	    *charset = NULL;

    temp = calloc(4097, sizeof(char));
    Syslog('m', "Entering rfc2ftn");
    rewind(fp);
    msg = parsrfc(fp);

    newsmode = hdr((char *)"Newsgroups", msg) ?TRUE:FALSE;
    if (newsmode == FALSE) {
	WriteError("Not a news article");
	return 1;
    }

    if ((fmsg = mkftnhdr(msg, newsmode, NULL)) == NULL) {
	WriteError("Unable to create FTN headers from RFC ones, aborting");
	tidyrfc(msg);
	return 1;
    }

    fmsg->area = xstrcpy(msgs.Tag);
    svmsgid = fmsg->msgid_n;
    svreply = fmsg->reply_n;
    if ((p = hdr((char *)"Message-ID",msg))) {
	ftnmsgid(p, &fmsg->msgid_a, &fmsg->msgid_n, fmsg->area);
	hash_update_s(&fmsg->msgid_n, fmsg->area);
    }

    if ((p = hdr((char *)"References",msg))) {
	p = strrchr(p,' ');
	ftnmsgid(p,&fmsg->reply_a, &fmsg->reply_n,fmsg->area);

//Griffin
	fmsg->reply_s=calloc(256,sizeof(char));
	findorigmsg(p,fmsg->reply_s);

	fmsg->to->name=calloc(strlen(Msg.From)+1,sizeof(char));
	strcpy(fmsg->to->name,Msg.From);
	Syslog('m', "fmsg to-name %s",fmsg->to->name);
	Syslog('m', "reply_s %s",fmsg->reply_s);

	if (!chkftnmsgid(p)) {
	    hash_update_s(&fmsg->reply_n, fmsg->area);
	}
    } else if ((p = hdr((char *)"In-Reply-To",msg))) {
	ftnmsgid(p,&fmsg->reply_a, &fmsg->reply_n,fmsg->area);
	if (!chkftnmsgid(p)) {
	    hash_update_s(&fmsg->reply_n, fmsg->area);
	}
    }

    chkftnmsgid(hdr((char *)"Message-ID",msg)); // ??
    removemime       = FALSE;
    removemsgid      = FALSE;
    removeref        = FALSE;
    removeinreply    = FALSE;
    removereplyto    = TRUE;
    removereturnto   = TRUE;
    ftnorigin = fmsg->ftnorigin;

    q = hdr((char *)"Content-Transfer-Encoding",msg);
    if (q) 
	while (*q && isspace(*q)) 
	    q++;
    if (!(q)) 
	q = (char *)"8bit"; 
    if ((p = hdr((char *)"Content-Type",msg))) {
	while (*p && isspace(*p)) 
	    p++;

	/*
	 * Check for mime to remove.
	 */
	if ((strncasecmp(p, "text/plain", 10) == 0) && ((q == NULL) || 
		    (strncasecmp(q,"7bit",4) == 0) || (strncasecmp(q,"8bit",4) == 0))) {
	    removemime = TRUE; /* no need in MIME headers */
	}

	q = strtok(p, " \n\0");
	q = strtok(NULL, "; \n\0");
	if (q) {
	    while (*q && isspace(*q))
		q++;
	    Syslog('m', "charset part: %s", printable(q, 0));
	    if (q && (strncasecmp(q, "charset=", 8) == 0)) {
		/*
		 * google.com quotes the charset name
		 */
		if (strchr(q, '"')) {
		    charset = xstrcpy(q + 9);
		    charset[strlen(charset)-1] = '\0';
		    Syslog('m', "Unquoted charset name");
		} else {
		    charset = xstrcpy(q + 8);
		}
		Syslog('m', "Charset \"%s\"", printable(charset, 0));
	    }
	}
    }

    if (charset == NULL) {
	charset = xstrcpy((char *)"ISO-8859-1");
	Syslog('m', "No charset, setting default to iso-8859-1");
    }

    chartran_init(charset, get_ic_ftn(msgs.Charset), 'm');

    if ((p = hdr((char *)"Message-ID",msg))) {
	if (!removemsgid)
	    removemsgid = chkftnmsgid(p);
    }

    if ((!removeref) && (p = hdr((char *)"References",msg))) {
	p = xstrcpy(p);
	q = strtok(p," \t\n");
	if ((q) && (strtok(NULL," \t\n") == NULL))
	    removeref = chkftnmsgid(q);       
	free(p);
    }

    if ((p = hdr((char *)"Reply-To",msg))) {
	removereplyto = FALSE;
	if ((q = hdr((char *)"From",msg))) {
	    char    *r;
	    r = xstrcpy(p); 
	    p = r;
	    while(*p && isspace(*p)) 
		p++;
	    if (p[strlen(p)-1] == '\n')
		p[strlen(p)-1]='\0';
	    if (strcasestr(q,p))
		removereplyto = TRUE;
	}
    }

    if ((p = hdr((char *)"Return-Receipt-To",msg))) {
	removereturnto = FALSE;
	if ((q = hdr((char *)"From",msg))) {
	    char    *r;

	    r = xstrcpy(p); 
	    p = r;
	    while (*p && isspace(*p)) 
		p++;
	    if (p[strlen(p)-1] == '\n') 
		p[strlen(p)-1]='\0';
	    if (strcasestr(q,p)) 
		removereturnto = TRUE;
	}
    }

    Syslog('m', "removemime=%s removemsgid=%s removeref=%s removeinreply=%s removereplyto=%s removereturnto=%s",
		removemime ?"TRUE ":"FALSE", removemsgid ?"TRUE ":"FALSE", removeref ?"TRUE ":"FALSE",
		removeinreply ?"TRUE ":"FALSE", removereplyto ?"TRUE ":"FALSE", removereturnto ?"TRUE ":"FALSE");

    p = ascfnode(fmsg->from,0x1f);
    i = 79-11-3-strlen(p);
    if (ftnorigin && fmsg->origin && (strlen(fmsg->origin) > i)) {
        /* This is a kludge...  I don't like it too much.  But well,
           if this is a message of FTN origin, the original origin (:)
           line MUST have been short enough to fit in 79 chars...
           So we give it a try.  Probably it would be better to keep
           the information about the address format from the origin
           line in a special X-FTN-... header, but this seems even
           less elegant.  Any _good_ ideas, anyone? */

        /* OK, I am keeping this, though if should never be used
           al long as X-FTN-Origin is used now */

	p = ascfnode(fmsg->from,0x0f);
	Syslog('m', "checkorigin 3");
	i = 79-11-3-strlen(p);
	tinyorigin = TRUE;
    }
    if (tinyorigin)
	Syslog('m', "tinyorigin = %s", tinyorigin ? "True":"False");

    if ((fmsg->origin) && (strlen(fmsg->origin) > i))
	fmsg->origin[i]='\0';
    forbidsplit = (ftnorigin || ((p = hdr((char *)"X-FTN-Split",msg))  && (strcasecmp(p," already\n") == 0)));
    needsplit = 0;
    splitpart = 0;
    hdrsize = 20;
    hdrsize += (fmsg->subj)?strlen(fmsg->subj):0;
    if (fmsg->from)
	hdrsize += (fmsg->from->name)?strlen(fmsg->from->name):0;
    if (fmsg->to)
	hdrsize += (fmsg->to->name)?strlen(fmsg->to->name):0;
    do {
	Syslog('m', "split loop, splitpart = %d", splitpart);
	datasize = 0;

	if (splitpart) {
	    snprintf(newsubj,4 * (MAXSUBJ+1),"[part %d] ",splitpart+1);
	    strncat(newsubj,fmsg->subj,MAXSUBJ-strlen(newsubj));
	} else {
	    strncpy(newsubj,fmsg->subj,MAXSUBJ);
	}
	newsubj[MAXSUBJ]='\0';

	if (splitpart) {
	    hash_update_n(&fmsg->msgid_n,splitpart);
	}
	oldsubj = fmsg->subj;
	fmsg->subj = newsubj;

	/*
	 * Create a new temp message in FTN style format
	 */
	if ((ofp = tmpfile()) == NULL) {
	    WriteError("$Can't open second tmpfile");
	    tidyrfc(msg);
	    return 1;
	}

	if ((fmsg->msgid_a == NULL) && (fmsg->msgid_n == 0)) {
	    Syslog('n', "No Messageid from poster, creating new MSGID");
	    fprintf(ofp, "\001MSGID: %s %08x\n", aka2str(msgs.Aka), sequencer());
	} else {
	    fprintf(ofp, "\001MSGID: %s %08x\n", MBSE_SS(fmsg->msgid_a),fmsg->msgid_n);
	}
	if (fmsg->reply_s) 
	    fprintf(ofp, "\1REPLY: %s\n", fmsg->reply_s);
	else if (fmsg->reply_a)
	    fprintf(ofp, "\1REPLY: %s %08x\n", fmsg->reply_a, fmsg->reply_n);
	Now = time(NULL) - (gmt_offset((time_t)0) * 60);
	fprintf(ofp, "\001TZUTC: %s\n", gmtoffset(Now));
	fprintf(ofp, "\001CHRS: %s\n", getftnchrs(msgs.Charset));
	
	fmsg->subj = oldsubj;
	if ((p = hdr((char *)"X-FTN-REPLYADDR",msg))) {
	    hdrsize += 10+strlen(p);
	    fprintf(ofp,"\1REPLYADDR:");
	    kludgewrite(p,ofp);
	} else if (replyaddr) {
	    hdrsize += 10+strlen(replyaddr);
	    fprintf(ofp,"\1REPLYADDR: ");
	    kludgewrite(replyaddr,ofp);
	}
	if ((p = hdr((char *)"X-FTN-REPLYTO",msg))) {
	    hdrsize += 8+strlen(p);
	    fprintf(ofp,"\1REPLYTO:");
	    kludgewrite(p,ofp);
	} else if (replyaddr) {
	    hdrsize += 15;
	    if (newsmode)
		fprintf(ofp,"\1REPLYTO: %s UUCP\n", aka2str(msgs.Aka));
	    else {
		fta = bestaka_s(fmsg->to);
		fprintf(ofp,"\1REPLYTO: %s UUCP\n", ascfnode(fta, 0x1f));
		tidy_faddr(fta);
	    }
	} else if ((p = hdr((char *)"Reply-To",msg))) {
	    if ((ta = parsefaddr(p))) {
		if ((q = hdr((char *)"From",msg))) {
		    if (!strcasestr(q,p)) {
			fprintf(ofp,"\1REPLYTO: %s %s\n", ascfnode(ta,0x1f), ta->name);
		    }
		}
		tidy_faddr(ta);
	    }
	}
	if ((p=strip_flags(hdr((char *)"X-FTN-FLAGS",msg)))) {
	    hdrsize += 15;
	    fprintf(ofp,"\1FLAGS:%s\n",p);
	    free(p);
	}
	if (!hdr((char *)"X-FTN-PID", msg)) { 
	    p = hdr((char *)"User-Agent", msg);
	    if (p == NULL) 
		p = hdr((char *)"X-Newsreader", msg);
	    if (p == NULL) 
		p = hdr((char *)"X-Mailer", msg);
	    if (p) {
		hdrsize += 4 + strlen(p);
		fprintf(ofp, "\1PID:");
		kludgewrite(p, ofp);
	    } else {
		fprintf(ofp, "\001PID: MBSE-NNTPD %s (%s-%s)\n", VERSION, OsName(), OsCPU());
	    }
	}

	if (!(hdr((char *)"X-FTN-Tearline", msg)) && !(hdr((char *)"X-FTN-TID", msg))) {
	    snprintf(temp, 4096, " MBSE-NNTPD %s (%s-%s)", VERSION, OsName(), OsCPU());
	    hdrsize += 4 + strlen(temp);
	    fprintf(ofp, "\1TID:");
	    kludgewrite(temp, ofp);
	}

	if ((splitpart == 0) || (hdrsize < MAXHDRSIZE)) {
	    for (tmp = msg; tmp; tmp = tmp->next) {
	 	if ((!strncmp(tmp->key,"X-Fsc-",6)) || (!strncmp(tmp->key,"X-FTN-",6) &&
			strcasecmp(tmp->key,"X-FTN-Tearline") &&
			strcasecmp(tmp->key,"X-FTN-Origin") &&
			strcasecmp(tmp->key,"X-FTN-Sender") &&
			strcasecmp(tmp->key,"X-FTN-Split") &&
			strcasecmp(tmp->key,"X-FTN-FLAGS") &&
			strcasecmp(tmp->key,"X-FTN-AREA") &&
			strcasecmp(tmp->key,"X-FTN-MSGID") &&
			strcasecmp(tmp->key,"X-FTN-REPLY") &&
			strcasecmp(tmp->key,"X-FTN-SEEN-BY") &&
			strcasecmp(tmp->key,"X-FTN-PATH") &&
			strcasecmp(tmp->key,"X-FTN-REPLYADDR") &&
			strcasecmp(tmp->key,"X-FTN-REPLYTO") &&
			strcasecmp(tmp->key,"X-FTN-To") &&
			strcasecmp(tmp->key,"X-FTN-From") &&
			strcasecmp(tmp->key,"X-FTN-CHARSET") &&
			strcasecmp(tmp->key,"X-FTN-CHRS") &&
			strcasecmp(tmp->key,"X-FTN-CODEPAGE") &&
			strcasecmp(tmp->key,"X-FTN-ORIGCHRS") &&
			strcasecmp(tmp->key,"X-FTN-SOT") &&
			strcasecmp(tmp->key,"X-FTN-EOT") &&
			strcasecmp(tmp->key,"X-FTN-Via"))) {
		    if ((strcasecmp(tmp->key,"X-FTN-KLUDGE") == 0)) {
			if (!strcasecmp(tmp->val," SOT:\n"))
			    sot_kludge = TRUE;
			else if (!strcasecmp(tmp->val," EOT:\n"))
			    eot_kludge = TRUE;
			else {
			    hdrsize += strlen(tmp->val);
			    fprintf(ofp,"\1");
			    /* we should have restored the original string here... */
			    kludgewrite((tmp->val)+1,ofp);
			}
		    } else {
			hdrsize += strlen(tmp->key)+strlen(tmp->val);
			fprintf(ofp,"\1%s:",tmp->key+6);
			kludgewrite(tmp->val,ofp);
		    }
		}
	    }

	    /* ZConnect are X-ZC-*: in usenet, \1ZC-*: in FTN */
	    for (tmp=msg;tmp;tmp=tmp->next)
		if ((!strncmp(tmp->key,"X-ZC-",5))) {
		    hdrsize += strlen(tmp->key)+strlen(tmp->val);
		    fprintf(ofp,"\1%s:",tmp->key+2);
		    kludgewrite(tmp->val,ofp);
		}

	    /* mondo.org gateway uses ".MSGID: ..." in usenet */
	    for (tmp=msg;tmp;tmp=tmp->next)
		if ((!strncmp(tmp->key,".",1)) && (strcasecmp(tmp->key,".MSGID"))) {
		    hdrsize += strlen(tmp->key)+strlen(tmp->val);
		    fprintf(ofp,"\1%s:",tmp->key+1);
		    kludgewrite(tmp->val,ofp);
		}

	    for (tmp = msg; tmp; tmp = tmp->next) {
		if ((needputrfc(tmp, newsmode) == 1)) {
		    if (strcasestr((char *)"X-Origin-Newsgroups",tmp->key)) {
			hdrsize += 10+strlen(tmp->val);
			fprintf(ofp,"\1RFC-Newsgroups:");
		    } else {
			hdrsize += strlen(tmp->key)+strlen(tmp->val);
			fprintf(ofp,"\1RFC-%s:",tmp->key);
		    }
		    kludgewrite(tmp->val, ofp);
		}
	    }

	    rfcheaders=0;
	    for (tmp=msg;tmp;tmp=tmp->next) {
		if ((needputrfc(tmp, newsmode) > 1)) {
		    rfcheaders++;
		    if (strcasestr((char *)"X-Origin-Newsgroups",tmp->key)) {
			hdrsize += 10+strlen(tmp->val);
			fprintf(ofp,"Newsgroups:");
		    } else {
			hdrsize += strlen(tmp->key)+strlen(tmp->val);
			fprintf(ofp,"%s:",tmp->key);
		    }
		    charwrite(tmp->val, ofp);
		}
	    }

	    if (rfcheaders) 
		charwrite((char *)"\n",ofp);
	    if ((hdr((char *)"X-FTN-SOT",msg)) || (sot_kludge))
		fprintf(ofp,"\1SOT:\n");
	}
	if (replyaddr) {
	    replyaddr = NULL;
	}

	if (needsplit) {
	    fprintf(ofp," * Continuation %d of a split message *\n\n", splitpart);
	    needsplit = FALSE;
	} else if ((p=hdr((char *)"X-Body-Start",msg))) {
	    datasize += strlen(p);
	    charwrite(p, ofp);
	}
	while (!(needsplit=(!forbidsplit) && (((splitpart && (datasize > (CFG.new_split * 1024))) ||
		      (!splitpart && ((datasize+hdrsize) > (CFG.new_split * 1024)))))) && (bgets(temp,4096-1,fp))) {
	    datasize += strlen(temp);
	    charwrite(temp, ofp);
	}

	if (needsplit) {
	    fprintf(ofp,"\n * Message split, to be continued *\n");
	    splitpart++;
	}
	if ((p=hdr((char *)"X-FTN-EOT",msg)) || (eot_kludge))
	    fprintf(ofp,"\1EOT:\n");

	if ((p=hdr((char *)"X-FTN-Tearline",msg))) {
	    fprintf(ofp,"---");
	    if (strcasecmp(p," (none)\n") == 0)
		charwrite((char *)"\n",ofp);
	    else
		charwrite(p,ofp);
	} else
	    fprintf(ofp,"\n%s\n", TearLine());

	if ((p = hdr((char *)"X-FTN-Origin",msg))) {
	    if (*(q=p+strlen(p)-1) == '\n') 
		*q='\0';
	    origin = xstrcpy((char *)" * Origin: ");
	    origin = xstrcat(origin, p);
	} else {
	    origin = xstrcpy((char *)" * Origin: ");
	    if (fmsg->origin)
		origin = xstrcat(origin, fmsg->origin);
	    else
		origin = xstrcat(origin, CFG.origin);
	    origin = xstrcat(origin, (char *)" (");
	    origin = xstrcat(origin, ascfnode(fmsg->from,tinyorigin?0x0f:0x1f));
	    origin = xstrcat(origin, (char *)")");
	}
	fprintf(ofp, "%s", origin);

	if (newsmode) {
	    /*
	     * Setup SEEN-BY lines, first SEEN-BY from RFC message, then all matching AKA's
	     */
	    for (tmsg = msg; tmsg; tmsg = tmsg->next)
		if (strcasecmp(tmsg->key, "X-FTN-SEEN-BY") == 0)
		    fill_list(&sbl, tmsg->val, NULL);
	    for (i = 0; i < 40; i++) {
		if (CFG.akavalid[i] && (CFG.aka[i].point == 0) && (msgs.Aka.zone == CFG.aka[i].zone) &&
				    !((msgs.Aka.net == CFG.aka[i].net) && (msgs.Aka.node == CFG.aka[i].node))) {
		    snprintf(sbe, 16, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
		    fill_list(&sbl, sbe, NULL);
		}
	    }
	    if (msgs.Aka.point == 0) {
		snprintf(sbe, 16, "%u/%u", msgs.Aka.net, msgs.Aka.node);
		fill_list(&sbl, sbe, NULL);
	    }

	    /*
	     *  Only add SEEN-BY lines if there are any
	     */
	    if (sbl != NULL) {
		uniq_list(&sbl);
		sort_list(&sbl);
		seenlen = MAXSEEN + 1;
		memset(&sbe, 0, sizeof(sbe));
		/* ensure it will not match for the first entry */
		oldnet = sbl->addr->net-1;
		for (tmpl = sbl; tmpl; tmpl = tmpl->next) {
		    if (tmpl->addr->net == oldnet)
			snprintf(sbe,16," %u",tmpl->addr->node);
		    else
			snprintf(sbe,16," %u/%u",tmpl->addr->net, tmpl->addr->node);
		    oldnet = tmpl->addr->net;
		    seenlen += strlen(sbe);
		    if (seenlen > MAXSEEN) {
			seenlen = 0;
			fprintf(ofp,"\nSEEN-BY:");
			snprintf(sbe,16," %u/%u",tmpl->addr->net, tmpl->addr->node);
			seenlen = strlen(sbe);
		    }
		    fprintf(ofp,"%s",sbe);
		}
		tidy_falist(&sbl);
	    }

	    /*
	     *  Setup PATH lines
	     */
	    for (tmp = msg; tmp; tmp = tmp->next)
		if (!strcasecmp(tmp->key,"X-FTN-PATH"))
		    fill_path(&ptl,tmp->val);
		if (msgs.Aka.point == 0) {
		    snprintf(sbe,16,"%u/%u",msgs.Aka.net, msgs.Aka.node);
		    fill_path(&ptl,sbe);
		}

	    /*
	     *  Only add PATH line if there is something
	     */
	    if (ptl != NULL) {
		uniq_list(&ptl);
		seenlen = MAXPATH+1;
		/* ensure it will not match for the first entry */
		oldnet = ptl->addr->net-1;
		for (tmpl = ptl; tmpl; tmpl = tmpl->next) {
		    if (tmpl->addr->net == oldnet)
			snprintf(sbe,16," %u",tmpl->addr->node);
		    else
			snprintf(sbe,16," %u/%u",tmpl->addr->net, tmpl->addr->node);
		    oldnet = tmpl->addr->net;
		    seenlen += strlen(sbe);
		    if (seenlen > MAXPATH) {
			seenlen = 0;
			fprintf(ofp,"\n\1PATH:");
			snprintf(sbe,16," %u/%u",tmpl->addr->net, tmpl->addr->node);
			seenlen = strlen(sbe);
		    }
		    fprintf(ofp,"%s",sbe);
		}
		tidy_falist(&ptl);
	    }
	} /* if (newsmode) */

	/*
	 *  Add newline and message is ready.
	 */
	fprintf(ofp,"\n");
	fflush(ofp);
	rewind(ofp);

	Syslog('m', "========== Fido start");
	while (fgets(temp, 4096, ofp) != NULL) {
	    /*
	     *  Only log kludges, skip the body
	     */
	    if ((temp[0] == '\001') || !strncmp(temp, "AREA:", 5) || !strncmp(temp, "SEEN-BY", 7)) {
		Striplf(temp);
		Syslogp('m', printable(temp, 0));
	    }
	}
	Syslog('m', "========== Fido end");

	if (!Msg_Open(msgs.Base)) {
	    WriteError("Failed to open msgbase \"%s\"", msgs.Base);
	} else {
	    if (!Msg_Lock(30L)) {
		WriteError("Can't lock %s", msgs.Base);
	    } else {
		Msg_New();
		strcpy(Msg.From, fmsg->from->name);
		strcpy(Msg.To, fmsg->to->name);
		strcpy(Msg.FromAddress, ascfnode(fmsg->from,0x1f));
		strcpy(Msg.Subject, fmsg->subj);
		Msg.Written = Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
		Msg.Local = TRUE;
		rewind(ofp);
		while (fgets(temp, 4096, ofp) != NULL) {
		    Striplf(temp);
		    MsgText_Add2(temp);
		}

		Msg_AddMsg();
		Msg_UnLock();
		Syslog('+', "Msg (%ld) to \"%s\", \"%s\"", Msg.Id, Msg.To, Msg.Subject);
		do_mailout = TRUE;

		/*
		 * Create fast scan index
		 */
		snprintf(temp, PATH_MAX, "%s/tmp/echomail.jam", getenv("MBSE_ROOT"));
		if ((qfp = fopen(temp, "a")) != NULL) {
		    fprintf(qfp, "%s %u\n", msgs.Base, Msg.Id);
		    fclose(qfp);
		}

		/*
		 * Link messages
		 */
		rc = Msg_Link(msgs.Base, TRUE, CFG.slow_util);
		if (rc != -1)
		    Syslog('+', "Linked %d message%s", rc, (rc != 1) ? "s":"");
		else
		    Syslog('+', "Could not link messages");

		/*
		 * Update statistical counters
		 */
		Now = time(NULL);
		l_date = localtime(&Now);
		msgs.LastPosted = time(NULL);
		msgs.Posted.total++;
		msgs.Posted.tweek++;
		msgs.Posted.tdow[l_date->tm_wday]++;
		msgs.Posted.month[l_date->tm_mon]++;
		mgroup.LastDate = time(NULL);
		mgroup.MsgsSent.total++;
		mgroup.MsgsSent.tweek++;
		mgroup.MsgsSent.tdow[l_date->tm_wday]++;
		mgroup.MsgsSent.month[l_date->tm_mon]++;
		UpdateMsgs();

		snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
		if ((qfp = fopen(temp, "r+"))) {
		    fread(&usrconfighdr, sizeof(usrconfighdr), 1, qfp);
		    fseek(qfp, usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize), SEEK_SET);
		    if (fread(&usrconfig, usrconfighdr.recsize, 1, qfp) == 1) {
			usrconfig.iPosted++;
			fseek(qfp, usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize), SEEK_SET);
			fwrite(&usrconfig, usrconfighdr.recsize, 1, qfp);
		    }
		    fclose(qfp);
		}
	    }
	    Msg_Close();
	}
	
	free(origin);
        fclose(ofp);
    } while (needsplit);
    free(temp);
    if (charset)
	free(charset);
    chartran_close();
    tidyrfc(msg);
    tidy_ftnmsg(fmsg);
    UpdateMsgs();

    return 0;
}



/*
 *  Test which kludges to add;
 *  <1  junk
 *  1   make kludge
 *  >1  pass
 */
int needputrfc(rfcmsg *msg, int newsmode)
{
	faddr	*ta;

	if ((msg->key == NULL) || (strlen(msg->key) == 0)) return 0;

	if (!strcasecmp(msg->key,"X-UUCP-From")) return -1;
	if (!strcasecmp(msg->key,"X-Body-Start")) return -1;
	if (!strncasecmp(msg->key,".",1)) return 0;
	if (!strcasecmp(msg->key,"Path")) return 0;
	if (!strcasecmp(msg->key,"Newsgroups")) {
 		if ((hdr((char *)"X-Origin-Newsgroups",msg)))
			return 0;
		else if (strstr(msg->val,","))
			return 1;
		else 
			return 0;
	}

	if (!strcasecmp(msg->key,"X-Origin-Newsgroups")) {
		if (strstr(msg->val,","))
			return 1;
		else 
			return 0;
	}

	if (!strcasecmp(msg->key,"Control")) {
		if (CFG.allowcontrol) {
			if (strstr(msg->val,"cancel")) 
				return 1;
			else 
				return 0;
		} else
			return 0;
	}
	if (!strcasecmp(msg->key,"Return-Path")) return 1;
	if (!strcasecmp(msg->key,"Xref")) return 0;
	if (!strcasecmp(msg->key,"Approved")) return 1;
	if (!strcasecmp(msg->key,"Return-Receipt-To")) return removereturnto? 0:1;
	if (!strcasecmp(msg->key,"Notice-Requested-Upon-Delivery-To")) return 0;
	if (!strcasecmp(msg->key,"Received")) return newsmode?0:2;
	if (!strcasecmp(msg->key,"From")) {
		if ((ta = parsefaddr(msg->val))) {
			tidy_faddr(ta);
			return 0;
		} else {
			return 1; /* 28-Jul-2001 MB (was 2) */ 
		}
	}
	if (!strcasecmp(msg->key,"To")) {
		if ((ta=parsefaddr(msg->val))) {
			tidy_faddr(ta);
			return 0;
		} else
			return 2;
	}
	if (!strcasecmp(msg->key,"Cc")) return 2;
	if (!strcasecmp(msg->key,"Bcc")) return 0;
	if (!strcasecmp(msg->key,"Reply-To")) {
		if ((ta = parsefaddr(msg->val))) {
			tidy_faddr(ta);
			return -1;
		} else 
			return removereplyto ?0:4;
	}
	if (!strcasecmp(msg->key,"Lines")) return 0;
	if (!strcasecmp(msg->key,"Date")) return 0;
	if (!strcasecmp(msg->key,"Subject")) {
		if ((msg->val) && (strlen(msg->val) > MAXSUBJ)) 
			return 2;
		else 
			return 0;
	}
	if (!strcasecmp(msg->key,"Organization")) return 1;
	if (!strcasecmp(msg->key,"Organisation")) return 1;
	if (!strcasecmp(msg->key,"Comment-To")) return 0;
	if (!strcasecmp(msg->key,"Apparently-To")) return 0;
	if (!strcasecmp(msg->key,"Keywords")) return 2;
	if (!strcasecmp(msg->key,"Summary")) return 2;
	if (!strcasecmp(msg->key,"MIME-Version")) return removemime ?0:1;
	if (!strcasecmp(msg->key,"Content-Type")) return removemime ?0:1;
	if (!strcasecmp(msg->key,"Content-Length")) return removemime ?0:1;
	if (!strcasecmp(msg->key,"Content-Transfer-Encoding")) return removemime ?0:1;
	if (!strcasecmp(msg->key,"Content-Name")) return 2;
	if (!strcasecmp(msg->key,"Content-Description")) return 2;
	if (!strcasecmp(msg->key,"Message-ID")) return removemsgid ?0:1;
	if (!strcasecmp(msg->key,"References")) return removeref ?0:1;
	if (!strcasecmp(msg->key,"Supersedes")) return 1;
	if (!strcasecmp(msg->key,"Distribution")) return ftnorigin ?0:0;
	if (!strcasecmp(msg->key,"User-Agent")) return 0;
	if (!strncasecmp(msg->key,"NNTP-",5)) return 0;
	if (!strncasecmp(msg->key,"Resent-",7)) return 0;
	if (!strncasecmp(msg->key,"X-MS-",5)) return -1;
	if (!strcasecmp(msg->key,"Precedence")) return 0;
	if (!strcasecmp(msg->key,"Cache-Post-Path")) return 0;
	if (!strcasecmp(msg->key,"Injection-Info")) return 0;
	if (!strcasecmp(msg->key,"Complaints-To")) return 0;
	/* Default X- headers */
	if (!strncasecmp(msg->key,"X-",2)) return 0;
	return 1;
}

#endif
