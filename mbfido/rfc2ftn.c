/*****************************************************************************
 *
 * File ..................: mbfido/rfc2ftn.c
 * Purpose ...............: Convert RFC to FTN
 * Last modification date : 14-Aug-2001
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
#include "../lib/mbinet.h"
#include "../lib/dbdupe.h"
#include "../lib/dbnode.h"
#include "../lib/dbmsgs.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "mkftnhdr.h"
#include "hash.h"
#include "rollover.h"
#include "pack.h"
#include "postnetmail.h"
#include "postecho.h"
#include "rfc2ftn.h"


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
static int	removesupersedes;
static int	removeapproved;
static int	removereplyto;
static int	removereturnto;
static int	dirtyoutcode = CHRS_NOTSET;



/*
 *  External variables
 */
extern	int	do_quiet;
extern	int	do_learn;
extern	int	most_debug;
extern	int	news_in;
extern	int	email_in;
extern	char	*replyaddr;



/*
 *  Internal functions
 */
int	needputrfc(rfcmsg *);



int charwrite(char *, FILE *);
int charwrite(char *s, FILE *fp)
{
	if ((strlen(s) >= 3) && (strncmp(s,"---",3) == 0) && (s[3] != '-')) {
		putc('-',fp);
		putc(' ',fp);
	}
	while (*s) {
		putc(*s, fp);
		s++;
	}
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



/*
 *  Input a RFC message.
 */
int rfc2ftn(FILE *fp, faddr *recipient)
{
        char            sbe[16], *p, *q, *temp, *origin;
        int             i, rc, incode, outcode, pgpsigned, newsmode;
	int		seenlen, oldnet;
        rfcmsg          *msg = NULL, *tmsg, *tmp;
        ftnmsg          *fmsg = NULL;
        FILE            *ofp;
        fa_list         *sbl = NULL, *ptl = NULL, *tmpl;
        faddr           *ta;
        unsigned long   svmsgid, svreply;
        int             sot_kludge = FALSE, eot_kludge = FALSE, qp_or_base64 = FALSE, tinyorigin = FALSE;
        int             needsplit, hdrsize, datasize, splitpart, forbidsplit, rfcheaders;
        char            newsubj[4 * (MAXSUBJ+1)], *oldsubj, *acup_a = NULL;
        unsigned long   acup_n = 0;
        int             html_message = FALSE;
        time_t          Now;

	temp = calloc(4096, sizeof(char));
	Syslog('m', "Entering rfc2ftn");
	if (recipient)
		Syslog('m', "Recipient: %s", ascfnode(recipient, 0xff));
	rewind(fp);
	Syslog('m', "========== RFC Start");
	while ((fgets(temp, 4095, fp)) != NULL) {
		Syslogp('m', printable(temp, 0));
	}
	Syslog('m', "========== RFC end");
	rewind(fp);
	msg = parsrfc(fp);
	incode = outcode = CHRS_NOTSET;
	pgpsigned = FALSE;

	newsmode = hdr((char *)"Newsgroups", msg) ?TRUE:FALSE;
	Syslog('m', "RFC message is %s", newsmode ? "news article":"e-mail message");

	if (newsmode)
		news_in++;
	else
		email_in++;

	p = hdr((char *)"Content-Type",msg);
	if (p)
		incode = readcharset(p);
	if (incode == CHRS_NOTSET) {
		p = hdr((char *)"X-FTN-CHRS",msg);
		if (p == NULL) 
			p = hdr((char *)"X-FTN-CHARSET", msg);
		if (p == NULL) 
			p = hdr((char *)"X-FTN-CODEPAGE", msg);
		if (p) 
			incode = readchrs(p);
	}

	if ((p = hdr((char *)"Content-Type",msg)) && ((strcasestr(p,(char *)"multipart/signed")) || 
	      (strcasestr(p,(char *)"application/pgp")))) {
		pgpsigned = TRUE; 
		outcode = incode; 
	} else if ((p = hdr((char *)"X-FTN-ORIGCHRS", msg))) 
		outcode = readchrs(p);
	else if (dirtyoutcode != CHRS_NOTSET)
		outcode = dirtyoutcode;
	else 
		outcode = getoutcode(incode);

	if (!CFG.allowcontrol) {
		if (hdr((char *)"Control",msg)) {
			Syslog('n', "skipping news message");
			tidyrfc(msg);
			return 1;
		}
	}

	if ((fmsg = mkftnhdr(msg, incode, outcode, newsmode, recipient)) == NULL) {
		WriteError("Unable to create FTN headers from RFC ones, aborting");
		tidyrfc(msg);
		return 1;
	}

	if (newsmode)
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
		if (!chkftnmsgid(p)) {
			hash_update_s(&fmsg->reply_n, fmsg->area);
		}
	} else if ((p = hdr((char *)"In-Reply-To",msg))) {
		ftnmsgid(p,&fmsg->reply_a, &fmsg->reply_n,fmsg->area);
		if (!chkftnmsgid(p)) {
			hash_update_s(&fmsg->reply_n, fmsg->area);
		}
	}

	if (incode == CHRS_NOTSET && newsmode)
		incode = msgs.Rfccode;
	if (outcode == CHRS_NOTSET) {
		if (newsmode)
			outcode = msgs.Ftncode;
		else
			outcode = CHRS_DEFAULT_FTN;
	}
	if ((incode == CHRS_NOTSET) && (hdr((char *)"Message-ID",msg))) {
		if (chkftnmsgid(hdr((char *)"Message-ID",msg)))
			incode = CHRS_DEFAULT_FTN;
		else
			incode = CHRS_DEFAULT_RFC;
	}
        removemime       = FALSE;
        removemsgid      = FALSE;
        removeref        = FALSE;
        removeinreply    = FALSE;
        removesupersedes = FALSE;
        removeapproved   = FALSE;
        removereplyto    = TRUE;
        removereturnto   = TRUE;
	ftnorigin = fmsg->ftnorigin;
	if ((hdr((char *)"X-PGP-Signed",msg)))
		pgpsigned = TRUE;
	if (pgpsigned)
		Syslog('n', "pgpsigned = %s", pgpsigned ? "True":"False");

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
		 * turn the quoted-printable decode mode on; remember FTN is virtually 8-bit clean
		 */
		if ((strncasecmp(p, "text/plain", 10) == 0) && (strncasecmp(q, "quoted-printable", 16) == 0))
			qp_or_base64 = 1;
		/*
		 * turn the base64 decode mode on
		 */
		else if ((strncasecmp(p, "text/plain", 10) == 0) && (strncasecmp(q, "base64", 6) == 0))
			qp_or_base64 = 2;

		/* 
		 * text/html support from FSC-HTML 001 proposal of Odinn Sorensen (2:236/77)
		 */ 
		if (strncasecmp(p, "text/html", 9) == 0)
			html_message = TRUE;
		for (tmp = msg; tmp; tmp = tmp->next)
			if (((strcasecmp(tmp->key,"X-FTN-KLUDGE") == 0) && (strcasecmp(tmp->val,"FSCHTML") == 0)) ||
			     (strcasecmp(tmp->key,"X-FTN-HTML") == 0))
				html_message = FALSE;

		if ((readcharset(p) != CHRS_NOTSET ) && ((q == NULL) || (strncasecmp(q,"7bit",4) == 0) ||
		    ((!pgpsigned) && (qp_or_base64==1)) || ((!pgpsigned) && (qp_or_base64==2)) || (strncasecmp(q,"8bit",4) == 0)))
			removemime=1; /* no need in MIME headers */
		/*
		 * some old MUA puts "text" instead of "text/plain; charset=..."
		 */
		else if ((strcasecmp(p,"text\n") == 0))
			removemime = TRUE;
	}
	if (removemime || qp_or_base64 || html_message)
		Syslog('n', "removemime=%s, qp_or_base64 = %d, html_message=%s", removemime ? "True":"False", qp_or_base64,
			html_message ? "True":"False");

	if ((p = hdr((char *)"Message-ID",msg))) {
		if (!removemsgid)
			removemsgid = chkftnmsgid(p);
	}
	Syslog('n', "removemsgid = %s", removemsgid ? "True":"False");

	if ((!removeref) && (p = hdr((char *)"References",msg))) {
		p = xstrcpy(p);
		q = strtok(p," \t\n");
		if ((q) && (strtok(NULL," \t\n") == NULL))
			removeref = chkftnmsgid(q);       
		free(p);
	}
	if (removeref)
		Syslog('n', "removeref = %s", removeref ? "True":"False");

	if ((p = hdr((char *)"Supersedes",msg)))
		removesupersedes = chkftnmsgid(p);
	if (removesupersedes)
		Syslog('n', "removesupersedes = %s", removesupersedes ? "True":"False");

        if ((p = hdr((char *)"Approved",msg))) {
		while (*p && isspace(*p)) 
			p++;
		if ((q = strchr(p,'\n'))) 
			*q='\0';
		if (newsmode && strlen(msgs.Moderator) && (strcasestr(msgs.Moderator,p)))
			removeapproved = TRUE;
		if (q) 
			*q='\n';
	}
	if (removeapproved)
		Syslog('n', "removeapproved = %s", removeapproved ? "True":"False");

	if ((p = hdr((char *)"Reply-To",msg))) {
		removereplyto = FALSE;
		if ((q = hdr((char *)"From",msg))) {
			char	*r;
			r = xstrcpy(p); 
			p = r;
			while(*p && isspace(*p)) 
				p++;
			if (p[strlen(p)-1] == '\n')
				p[strlen(p)-1]='\0';
			if (strcasestr(q,p))
				removereplyto = TRUE;
//			free(r);
		}
	}
	Syslog('n', "removereplyto = %s", removereplyto ? "True":"False");

	if ((p = hdr((char *)"Return-Receipt-To",msg))) {
		removereturnto = FALSE;
		if ((q = hdr((char *)"From",msg))) {
			char	*r;

			r = xstrcpy(p); 
			p = r;
			while (*p && isspace(*p)) 
				p++;
			if (p[strlen(p)-1] == '\n') 
				p[strlen(p)-1]='\0';
			if (strcasestr(q,p)) 
				removereturnto = TRUE;
//			free(r);
		}
	}
	if (!removereturnto)
		Syslog('n', "removereturnto = %s", removereturnto ? "True":"False");

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
		Syslog('n', "checkorigin 3");
		i = 79-11-3-strlen(p);
		tinyorigin = TRUE;
	}
	if (tinyorigin)
		Syslog('n', "tinyorigin = %s", tinyorigin ? "True":"False");

	if ((fmsg->origin) && (strlen(fmsg->origin) > i))
		fmsg->origin[i]='\0';
	forbidsplit = (ftnorigin || (hdr((char *)"X-FTN-Split",msg)));
	needsplit = 0;
	splitpart = 0;
	hdrsize = 20;
	hdrsize += (fmsg->subj)?strlen(fmsg->subj):0;
	if (fmsg->from)
		hdrsize += (fmsg->from->name)?strlen(fmsg->from->name):0;
	if (fmsg->to)
		hdrsize += (fmsg->to->name)?strlen(fmsg->to->name):0;
	do {
		Syslog('n', "split loop, splitpart = %d", splitpart);
		datasize = 0;

		if (splitpart) {
			sprintf(newsubj,"[part %d] ",splitpart+1);
			strncat(newsubj,fmsg->subj,MAXSUBJ-strlen(newsubj));
		} else {
			strncpy(newsubj,fmsg->subj,MAXSUBJ);
		}
		strcpy(newsubj, hdrnconv(newsubj, incode, outcode, MAXSUBJ));
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

		if (newsmode) {
			fprintf(ofp, "AREA:%s\n", msgs.Tag);
		} else {
			if (fmsg->to->point != 0)
				fprintf(ofp, "\001TOPT %d\n", fmsg->to->point);
			if (fmsg->from->point != 0)
				fprintf(ofp, "\001FMPT %d\n", fmsg->from->point);
			fprintf(ofp, "\001INTL %d:%d/%d %d:%d/%d\n", fmsg->to->zone, fmsg->to->net, fmsg->to->node,
				fmsg->from->zone, fmsg->from->net, fmsg->from->node);
		}

		fprintf(ofp, "\001MSGID: %s %08lx\n", MBSE_SS(fmsg->msgid_a),fmsg->msgid_n);
		if (fmsg->reply_s) 
			fprintf(ofp, "\1REPLY: %s\n", fmsg->reply_s);
		else if (fmsg->reply_a)
			fprintf(ofp, "\1REPLY: %s %08lx\n", fmsg->reply_a, fmsg->reply_n);
		Now = time(NULL) - (gmt_offset((time_t)0) * 60);
		fprintf(ofp, "\001TZUTC: %s\n", gmtoffset(Now));
		fmsg->subj = oldsubj;
		if ((p = hdr((char *)"X-FTN-REPLYADDR",msg))) {
			Syslog('n', "replyaddr 1 %s", p);
			hdrsize += 10+strlen(p);
			fprintf(ofp,"\1REPLYADDR:");
			kludgewrite(p,ofp);
		} else if (replyaddr) {
			Syslog('n', "replyaddr 2");
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
			else
				fprintf(ofp,"\1REPLYTO: %s UUCP\n", ascfnode(bestaka_s(fmsg->to), 0x1f));
		} else if ((p = hdr((char *)"Reply-To",msg))) {
			if ((ta = parsefaddr(p))) {
				if ((q = hdr((char *)"From",msg))) {
					if (!strcasestr(q,p)) {
						fprintf(ofp,"\1REPLYTO: %s %s\n", ascfnode(ta,0x1f), ta->name);
					}
					tidy_faddr(ta);
				}
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
				fprintf(ofp, "\001PID: MBSE-FIDO %s\n", VERSION);
			}
		}

		hdrsize += 8 + strlen(getchrs(outcode));
		fprintf(ofp, "\001CHRS: %s\n", getchrs(outcode));
		if (html_message) {
			hdrsize += 9;
			fprintf(ofp, "\1HTML: 5\n");
		}

		if (CFG.allowcontrol && (!hdr((char *)"X-FTN-ACUPDATE",msg)) && (p=hdr((char *)"Control",msg))) {
			if (strstr(p,"cancel")) {
				ftnmsgid(p,&acup_a,&acup_n,fmsg->area);
				if (acup_a) {
					hash_update_s(&acup_n,fmsg->area);
					hdrsize += 26 + strlen(acup_a);
					fprintf(ofp,"\1ACUPDATE: DELETE %s %08lx\n", acup_a,acup_n);
				}
			}
		}
		if ((!hdr((char *)"X-FTN-ACUPDATE",msg)) && (p=hdr((char *)"Supersedes",msg))) {
			ftnmsgid(p,&acup_a,&acup_n,fmsg->area);
			if (acup_a) {
				hash_update_s(&acup_n,fmsg->area);
				hdrsize += 26 + strlen(acup_a);
				fprintf(ofp,"\1ACUPDATE: MODIFY %s %08lx\n", acup_a,acup_n);
			}
		}
#ifdef FSC_0070
		/* FSC-0070 */
		if((p = hdr((char *)"Message-ID", msg)) && !(hdr((char *)"X-FTN-RFCID", msg))) {
			q = strdup(p);
			fprintf(ofp,"\1RFCID:");
			if ((l = strrchr(q, '<')) && (r = strchr(q, '>')) && (l < r)) {
				*l++ = ' ';
				while(*l && isspace(*l))
					l++;
				l--; /* leading ' ' */
				*r-- = '\0';
				while(*r && isspace(*r))
					*r-- = '\0';
			} else
				l = q;
			kludgewrite(l, ofp);
			hdrsize += 6 + strlen(l);
			free(q);
		}
#endif /* FSC_0070 */

		if (!(hdr((char *)"X-FTN-Tearline", msg)) && !(hdr((char *)"X-FTN-TID", msg))) {
			sprintf(temp, " MBSE-FIDO %s", VERSION);
			hdrsize += 4 + strlen(temp);
			fprintf(ofp, "\1TID:");
			kludgewrite(temp, ofp);
		}
		if ((splitpart == 0) || (hdrsize < MAXHDRSIZE)) {
			for (tmp = msg; tmp; tmp = tmp->next) {
	 			if ((!strncmp(tmp->key,"X-Fsc-",6)) ||
				    (!strncmp(tmp->key,"X-FTN-",6) &&
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

			/*
			 *  Add the Received: header from this system to the mesage.
			 */
			if (!newsmode) {
				Now = time(NULL);
				fprintf(ofp, "\1RFC-Received: by %s (mbfido) via RFC2FTN; %s\n", CFG.sysdomain, rfcdate(Now));
				hdrsize += 72+strlen(CFG.sysdomain);
			}

			for (tmp = msg; tmp; tmp = tmp->next) {
				if ((needputrfc(tmp) == 1)) {
					if (strcasestr((char *)"X-Origin-Newsgroups",tmp->key)) {
						hdrsize += 10+strlen(tmp->val);
						fprintf(ofp,"\1RFC-Newsgroups:");
					} else {
						hdrsize += strlen(tmp->key)+strlen(tmp->val);
						fprintf(ofp,"\1RFC-%s:",tmp->key);
					}
					kludgewrite(hdrconv(tmp->val, incode, outcode),ofp);
				}
			}

			rfcheaders=0;
			for (tmp=msg;tmp;tmp=tmp->next) {
				if ((needputrfc(tmp) > 1)) {
					rfcheaders++;
					if (strcasestr((char *)"X-Origin-Newsgroups",tmp->key)) {
						hdrsize += 10+strlen(tmp->val);
						fprintf(ofp,"Newsgroups:");
					} else {
						hdrsize += strlen(tmp->key)+strlen(tmp->val);
						fprintf(ofp,"%s:",tmp->key);
					}
					charwrite(hdrconv(tmp->val, incode, outcode),ofp);
				}
			}
			if (rfcheaders) 
				charwrite((char *)"\n",ofp);
			if ((hdr((char *)"X-FTN-SOT",msg)) || (sot_kludge))
				fprintf(ofp,"\1SOT:\n");
			if ((splitpart == 0) && (hdr((char *)"X-PGP-Signed",msg)))
				fprintf(ofp,PGP_SIGNED_BEGIN"\n");
		}
		if (replyaddr) {
//			free(replyaddr); /* Gives SIGSEGV */
			replyaddr = NULL;
		}
		if (needsplit) {
			fprintf(ofp," * Continuation %d of a split message *\n\n", splitpart);
			needsplit = FALSE;
		} else if ((p=hdr((char *)"X-Body-Start",msg))) {
			datasize += strlen(p);
			if (qp_or_base64==1)
				charwrite(strkconv(qp_decode(p), incode, outcode), ofp);
			else if (qp_or_base64==2)
				charwrite(strkconv(b64_decode(p), incode, outcode), ofp);
			else
				charwrite(strkconv(p, incode, outcode), ofp);
		}
		while (!(needsplit=(!forbidsplit) && (((splitpart && (datasize > (CFG.new_split * 1024))) ||
		      (!splitpart && ((datasize+hdrsize) > (CFG.new_split * 1024)))))) && (bgets(temp,4096-1,fp))) {
			datasize += strlen(temp);
			if (qp_or_base64==1)
				charwrite(strkconv(qp_decode(temp), incode, outcode), ofp);
			else if (qp_or_base64==2)
				charwrite(strkconv(b64_decode(temp), incode, outcode), ofp);
			else
				charwrite(strkconv(temp, incode, outcode), ofp);
		}
		if (needsplit) {
			fprintf(ofp,"\n * Message split, to be continued *\n");
			splitpart++;
		} else if ((p=hdr((char *)"X-PGP-Signed",msg))) {
			fprintf(ofp,PGP_SIG_BEGIN"\n");
			if ((q=hdr((char *)"X-PGP-Version",msg))) {
				fprintf(ofp,"Version:");
				charwrite(q,ofp);
			}
			if ((q=hdr((char *)"X-PGP-Charset",msg))) {
				fprintf(ofp,"Charset:");
				charwrite(q,ofp);
			}
			if ((q=hdr((char *)"X-PGP-Comment",msg))) {
				fprintf(ofp,"Comment:");
				charwrite(q,ofp);
			}
			fprintf(ofp,"\n");
			p=xstrcpy(p);
			q=strtok(p," \t\n");
			fprintf(ofp,"%s\n",q);
			while ((q=(strtok(NULL," \t\n"))))
				fprintf(ofp,"%s\n",q);
			fprintf(ofp,PGP_SIG_END"\n");
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
			fprintf(ofp,"--- MBSE BBSv.%s\n",VERSION);

		if ((p = hdr((char *)"X-FTN-Origin",msg))) {
			if (*(q=p+strlen(p)-1) == '\n') 
				*q='\0';
			origin = xstrcpy((char *)" * Origin: ");
			origin = xstrcat(origin, hdrconv(p, incode, outcode));
		} else {
			origin = xstrcpy((char *)" * Origin: ");
			if (fmsg->origin)
				origin = xstrcat(origin, hdrconv(fmsg->origin, incode, outcode));
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
					sprintf(sbe, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
					fill_list(&sbl, sbe, NULL);
				}
			}
			if (msgs.Aka.point == 0) {
				sprintf(sbe, "%u/%u", msgs.Aka.net, msgs.Aka.node);
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
						sprintf(sbe," %u",tmpl->addr->node);
					else
						sprintf(sbe," %u/%u",tmpl->addr->net, tmpl->addr->node);
					oldnet = tmpl->addr->net;
					seenlen += strlen(sbe);
					if (seenlen > MAXSEEN) {
						seenlen = 0;
						fprintf(ofp,"\nSEEN-BY:");
						sprintf(sbe," %u/%u",tmpl->addr->net, tmpl->addr->node);
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
				sprintf(sbe,"%u/%u",msgs.Aka.net, msgs.Aka.node);
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
						sprintf(sbe," %u",tmpl->addr->node);
					else
						sprintf(sbe," %u/%u",tmpl->addr->net, tmpl->addr->node);
					oldnet = tmpl->addr->net;
					seenlen += strlen(sbe);
					if (seenlen > MAXPATH) {
						seenlen = 0;
						fprintf(ofp,"\n\1PATH:");
						sprintf(sbe," %u/%u",tmpl->addr->net, tmpl->addr->node);
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

		Syslog('n', "========== Fido start");
		while (fgets(temp, 4096, ofp) != NULL) {
			/*
			 *  Only log kludges, skip the body
			 */
//			if ((temp[0] == '\001') || !strncmp(temp, "AREA:", 5) || !strncmp(temp, "SEEN-BY", 7)) {
//				Striplf(temp);
				Syslogp('n', printable(temp, 0));
//			}
		}
		Syslog('n', "========== Fido end");

		if (newsmode)
			rc = postecho(NULL, fmsg->from, fmsg->to, origin, fmsg->subj, fmsg->date, fmsg->flags, 0, ofp, FALSE);
		else
			rc = postnetmail(ofp, fmsg->from, fmsg->to, origin, fmsg->subj, fmsg->date, fmsg->flags, FALSE);

		free(origin);
                fclose(ofp);
        } while (needsplit);
	free(temp);
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
int needputrfc(rfcmsg *msg)
{
	faddr	*ta;

//	Syslog('M', "needputrfc(%s)", printable(msg->key,0));
	if ((msg->key == NULL) || (strlen(msg->key) == 0)) return 0;

	if (!strcasecmp(msg->key,"X-UUCP-From")) return -1;
	if (!strcasecmp(msg->key,"X-Body-Start")) return -1;
	if (!strncasecmp(msg->key,".",1)) return 0;
	if (!strncasecmp(msg->key,"X-FTN-",6)) return 0;
	if (!strncasecmp(msg->key,"X-Fsc-",6)) return 0;
	if (!strncasecmp(msg->key,"X-ZC-",5)) return 0;
	if (!strcasecmp(msg->key,"X-Gateway")) return 0;
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
	if (!strcasecmp(msg->key,"Approved")) return removeapproved ? -1:2;
	if (!strcasecmp(msg->key,"X-URL")) return 0;
	if (!strcasecmp(msg->key,"Return-Receipt-To")) return removereturnto? 0:1;
	if (!strcasecmp(msg->key,"Notice-Requested-Upon-Delivery-To")) return 0;
	if (!strcasecmp(msg->key,"Received")) return ftnorigin ?0:1;
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
	if (!strcasecmp(msg->key,"X-Comment-To")) return 0;
	if (!strcasecmp(msg->key,"X-Apparently-To")) return 0;
	if (!strcasecmp(msg->key,"Apparently-To")) return 0;
	if (!strcasecmp(msg->key,"X-Fidonet-Comment-To")) return 0;
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
	if (!strcasecmp(msg->key,"Supersedes")) return removesupersedes ?0:1;
	if (!strcasecmp(msg->key,"Distribution")) return ftnorigin ?0:0;
	if (!strcasecmp(msg->key,"X-Newsreader")) return 0;
	if (!strcasecmp(msg->key,"X-Mailer")) return 0;
	if (!strcasecmp(msg->key,"User-Agent")) return 0;
	if (!strncasecmp(msg->key,"NNTP-",5)) return 0;
	if (!strncasecmp(msg->key,"X-Trace",7)) return 0;
	if (!strncasecmp(msg->key,"X-Complaints",12)) return 0;
	if (!strncasecmp(msg->key,"X-MSMail",9)) return 0;
	if (!strncasecmp(msg->key,"X-MimeOLE",9)) return 0;
	if (!strncasecmp(msg->key,"X-MIME-Autoconverted",20)) return 0;
	if (!strcasecmp(msg->key,"X-Origin-Date")) return 0;
	if (!strncasecmp(msg->key,"X-PGP-",6)) return 0;
	if (!strncasecmp(msg->key,"Resent-",7)) return 0;
	if (!strcasecmp(msg->key,"X-Mailing-List")) return 0;
	if (!strcasecmp(msg->key,"X-Loop")) return 0;
	if (!strcasecmp(msg->key,"Precedence")) return 0;
	/*if (!strcasecmp(msg->key,"")) return ;*/
	return 1;
}


