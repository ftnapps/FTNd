/*****************************************************************************
 *
 * File ..................: mbfido/scannews.c
 * Purpose ...............: Scan for new News
 * Last modification date : 01-Jul-2001
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
#include "echoout.h"
#include "rollover.h"
#include "pack.h"
#include "scannews.h"


#define MAXHDRSIZE 2048
#define	MAXSEEN 70
#define	MAXPATH 73


/*
 *  Global variables
 */
POverview	xoverview = NULL;
int		marker = 0;
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
extern	int	news_imp;
extern	int	news_dupe;
extern	int	echo_out;
extern	int	echo_in;
extern	char	*replyaddr;

extern  char    *toname;                /* To user                          */
extern  char    *fromname;              /* From user                        */
extern  char    *subj;                  /* Message subject                  */


/*
 *  Internal functions
 */
int		do_one_group(List **, char *, char *);
int		get_xover(char *, long, long, List **);
int		get_xoverview(void);
void		tidy_artlist(List **);
void		fill_artlist(List **, char *, long, int);
void		Marker(void);
int		get_article(char *, char *);
int		needputrfc(rfcmsg *);


void tidy_artlist(List **fdp)
{
	List	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add article to the list
 */
void fill_artlist(List **fdp, char *id, long nr, int dupe)
{
	List	**tmp;

	Syslog('N', "Fill %s %ld %s", id, nr, dupe ? "Dupe":"New msg");

	for (tmp = fdp; *tmp; tmp = &((*tmp)->next));
	*tmp = (List *)malloc(sizeof(List));
	(*tmp)->next = NULL;
	sprintf((*tmp)->msgid, "%s", id);
	(*tmp)->nr = nr;
	(*tmp)->isdupe = dupe;
}



/*
 * write an arbitrary line to message body,
 * if a line starts with three dashes, insert a dash and a blank
 */
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



void Marker(void)
{
        if (do_quiet)
                return;

        switch (marker) {
                case 0: printf(">---");
                        break;

                case 1: printf(">>--");
                        break;

                case 2: printf(">>>-");
                        break;

                case 3: printf(">>>>");
                        break;

                case 4: printf("<>>>");
                        break;

                case 5: printf("<<>>");
                        break;

                case 6: printf("<<<>");
                        break;

                case 7: printf("<<<<");
                        break;

                case 8: printf("-<<<");
                        break;

                case 9: printf("--<<");
                        break;

                case 10:printf("---<");
                        break;

                case 11:printf("----");
                        break;
        }
        printf("\b\b\b\b");
        fflush(stdout);

        if (marker < 11)
                marker++;
        else
                marker = 0;
}



/*
 *  Scan for new news available at the nntp server.
 */
void ScanNews(void)
{
	List			*art = NULL;
	POverview		tmp, old;
	FILE			*pAreas;
	char			*sAreas;
	struct msgareashdr	Msgshdr;
	struct msgareas		Msgs;

	IsDoing((char *)"Scan News");
	if (nntp_connect() == -1) {
		WriteError("Can't connect to newsserver");
		return;
	}
	if (get_xoverview())
		return;

	if (!do_quiet) {
		colour(10, 0);
		printf("Scan for new news articles\n");
	}

	sAreas = calloc(PATH_MAX, sizeof(char));
	sprintf(sAreas, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if(( pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("$Can't open Messages Areas File.");
		return;
	}
	fread(&Msgshdr, sizeof(Msgshdr), 1, pAreas);

	while (fread(&Msgs, Msgshdr.recsize, 1, pAreas) == 1) {
		fseek(pAreas, Msgshdr.syssize, SEEK_CUR);
		if ((Msgs.Active) && strlen(Msgs.Newsgroup)) {
			if (IsSema((char *)"upsalarm")) {
				Syslog('+', "Detected upsalarm semafore, aborting newsscan");
				break;
			}
			Syslog('n', "Scan newsgroup: %s", Msgs.Newsgroup);
			if (!do_quiet) {
				colour(3, 0);
				printf("\r%-40s", Msgs.Newsgroup);
				fflush(stdout);
			}
			Nopper();
			if (do_one_group(&art, Msgs.Newsgroup, Msgs.Tag) == RETVAL_ERROR)
				break;
		}
	}
	fclose(pAreas);
	free(sAreas);

	for (tmp = xoverview; tmp; tmp = old) {
		old = tmp->next;
		if (tmp->header)
			free(tmp->header);
		if (tmp->field)
			free(tmp->field);
		free(tmp);
	}
	packmail();

	if (!do_quiet)
		printf("\r                                                    \r");
}



int do_one_group(List **art, char *grpname, char *ftntag)
{
	List	*tmp;
	char	temp[128], *resp;
	int	retval;
	long	total, start, end;

	Syslog('N', "do_one_group(%s, %s)", grpname, ftntag);
	IsDoing((char *)"Scan %s", grpname);
	sprintf(temp, "GROUP %s\r\n", grpname);
	nntp_send(temp);
	resp = nntp_receive();
	retval = atoi(strtok(resp, " "));
	if (retval != 211) {
		if (retval == 411)
			WriteError("No such newsgroup: %s", grpname);
		else
			WriteError("Unknown response %d to GROUP command", retval);
		return RETVAL_ERROR;
	}

	total = atol(strtok(NULL, " "));
	start = atol(strtok(NULL, " "));
	end   = atol(strtok(NULL, " '\0'"));
	if (!total) {
		Syslog('N', "No articles");
		return RETVAL_OK;
	}

	retval = get_xover(grpname, start, end, art);
	if (retval != RETVAL_OK) {
		tidy_artlist(art);
		return retval;
	}

	if (!do_learn) {
		for (tmp = *art; tmp; tmp = tmp->next) {
			if (!tmp->isdupe) {
				/*
				 *  If the message isn't a dupe, it must be new for us.
				 */
				most_debug = TRUE;
				get_article(tmp->msgid, ftntag);
				most_debug = FALSE;
			}
		}
	}

	tidy_artlist(art);
	return RETVAL_OK;
}



int get_article(char *msgid, char *ftntag)
{
	char	cmd[81], *resp;
	int	retval, done = FALSE;
	FILE	*fp = NULL;

	Syslog('n', "Get article %s, %s", msgid, ftntag);
	if (!SearchMsgs(ftntag)) {
		WriteError("Search message area %s failed", ftntag);
		return RETVAL_ERROR;
	}
	IsDoing("Article %d", (news_in + 1));
	sprintf(cmd, "ARTICLE %s\r\n", msgid);
	nntp_send(cmd);
	resp = nntp_receive();
	retval = atoi(strtok(resp, " "));
	switch (retval) {
		case 412:	WriteError("No newsgroup selected");
				return RETVAL_UNEXPECTEDANS;
		case 420:	WriteError("No current article has been selected");
				return RETVAL_UNEXPECTEDANS;
		case 423:	WriteError("No such article in this group");
				return RETVAL_UNEXPECTEDANS;
		case 430:	WriteError("No such article found");
				return RETVAL_UNEXPECTEDANS;
		case 220:	if ((fp = tmpfile()) == NULL) {
					WriteError("$Can't open tmpfile");
					return RETVAL_UNEXPECTEDANS;
				}
				while (done == FALSE) {
					resp = nntp_receive();
					if ((strlen(resp) == 1) && (strcmp(resp, ".") == 0)) {
						done = TRUE;
					} else {
						fwrite(resp, strlen(resp), 1, fp);
						fputc('\n', fp);
					}
				}
				break;
	}

	news_in++;
	IsDoing("Article %d", (news_in));
	retval = do_article(fp);
	fclose(fp);
	return retval;
}



int do_article(FILE *fp)
{
        char            sbe[16], *p, *q, *temp;
        int             i, incode, outcode, pgpsigned;
        int             First = TRUE, seenlen, oldnet;
        sysconnect      Link;
        rfcmsg          *msg = NULL, *tmsg, *tmp;
        ftnmsg          *fmsg = NULL;
        FILE            *ofp;
        fa_list         *sbl = NULL, *ptl = NULL, *tmpl;
        faddr           *ta, *From;
        unsigned long   svmsgid, svreply;
        int             sot_kludge = FALSE, eot_kludge = FALSE, qp_or_base64 = FALSE, tinyorigin = FALSE;
        int             needsplit, hdrsize, datasize, splitpart, forbidsplit, rfcheaders;
        char            newsubj[4 * (MAXSUBJ+1)], *oldsubj, *acup_a = NULL;
        unsigned long   acup_n = 0, crc2;
        int             html_message = FALSE;
        time_t          Now;

	rewind(fp);
	msg = parsrfc(fp);
	incode = outcode = CHRS_NOTSET;
	pgpsigned = FALSE;

	p = hdr((char *)"Content-Type",msg);
	if (p)
		incode=readcharset(p);
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
	for (tmsg = msg; tmsg; tmsg = tmsg->next)
		if (strcasecmp(tmsg->key, "X-FTN-SEEN-BY") == 0)
			fill_list(&sbl, tmsg->val, NULL, TRUE);

	if (!CFG.allowcontrol) {
		if (hdr((char *)"Control",msg)) {
			Syslog('n', "skipping news message");
			tidy_falist(&sbl);
			tidyrfc(msg);
			return RETVAL_OK;
		}
	}

	if ((fmsg = mkftnhdr(msg, incode, outcode, TRUE)) == NULL) {
		WriteError("Unable to create FTN headers from RFC ones, aborting");
		tidy_falist(&sbl);
		tidyrfc(msg);
		return RETVAL_ERROR;
	}

	/*
	 * Setup PATH line
	 */
	sprintf(sbe, "%u/%u", msgs.Aka.net, msgs.Aka.node);
	fill_path(&ptl, sbe);
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

	if (incode == CHRS_NOTSET)
		incode = msgs.Rfccode;
	if (outcode == CHRS_NOTSET)
		outcode = msgs.Ftncode;
	if ((incode == CHRS_NOTSET) && (hdr((char *)"Message-ID",msg))) {
		if (chkftnmsgid(hdr((char *)"Message-ID",msg)))
			incode = CHRS_DEFAULT_FTN;
		else
			incode = CHRS_DEFAULT_RFC;
	}
	temp = calloc(4096, sizeof(char));
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
		if (strlen(msgs.Moderator) && (strcasestr(msgs.Moderator,p)))
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
			free(r);
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
			tidy_falist(&sbl);
			tidy_falist(&ptl);
			tidyrfc(msg);
			return RETVAL_ERROR;
		}
		fprintf(ofp, "AREA:%s\n", msgs.Tag);
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
			fprintf(ofp,"\1REPLYTO: %s UUCP\n", aka2str(msgs.Aka));
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
			rfcheaders=0;
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
			fprintf(ofp," * Origin:");
			charwrite(hdrconv(p, incode, outcode),ofp);
		} else {
			fprintf(ofp," * Origin: "); /* strlen=11 */
			if (fmsg->origin)
				charwrite(hdrconv(fmsg->origin, incode, outcode), ofp);
			else
				charwrite(CFG.origin, ofp);
			fprintf(ofp," (%s)",
			ascfnode(fmsg->from,tinyorigin?0x0f:0x1f));
		}
		/*
		 * Setup SEEN-BY lines
		 */
		for (i = 0; i < 40; i++) {
			if (CFG.akavalid[i] && (msgs.Aka.zone == CFG.aka[i].zone) &&
			    !((msgs.Aka.net == CFG.aka[i].net) && (msgs.Aka.node == CFG.aka[i].node))) {
				sprintf(sbe, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
				fill_list(&sbl, sbe, NULL, FALSE);
			}
		}
		sprintf(sbe, "%u/%u", msgs.Aka.net, msgs.Aka.node);
		First = TRUE;
		/*
		 *  Count downlinks, if there are none then no more SEEN-BY entries will be added.
		 */
		i = 0;
		while (GetMsgSystem(&Link, First)) {
			First = FALSE;
			if ((Link.aka.zone) && (Link.sendto) && (!Link.pause) && (!Link.cutoff)) {
				sprintf(sbe, "%u/%u", Link.aka.net, Link.aka.node);
				fill_list(&sbl, sbe, NULL, FALSE);
				i++;
			}
		}
		uniq_list(&sbl);
		sort_list(&sbl);
		seenlen=MAXSEEN+1;
		if (i) {
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
		} else {
			fprintf(ofp,"\nSEEN-BY: %s",sbe);
		}

		for (tmp = msg; tmp; tmp = tmp->next) {
			if (!strcasecmp(tmp->key,"X-FTN-PATH")) {
				fill_path(&ptl,tmp->val);
			}
		}
		sprintf(sbe,"%u/%u",msgs.Aka.net, msgs.Aka.node);
		fill_path(&ptl,sbe);
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
		fprintf(ofp,"\n");

		tidy_falist(&ptl);
		fflush(ofp);
		rewind(ofp);
		Syslog('n', "========== Fido start");
		while (fgets(temp, 4096, ofp) != NULL) {
			/*
			 *  Only log kludges, skip the body
			 */
			if ((temp[0] == '\001') || !strncmp(temp, "AREA:", 5) || !strncmp(temp, "SEEN-BY", 7)) {
				Striplf(temp);
				Syslogp('n', printable(temp, 0));
			}
		}
		Syslog('n', "========== Fido end");

		rewind(ofp);
		Msg_New();

		if ((fmsg->to != NULL) && (fmsg->to->name != NULL))
			strcpy(Msg.To, fmsg->to->name);
		else
			sprintf(Msg.To, "All");
		Syslog('n', "Msg.To: %s", printable(Msg.To, 0));
		toname = xstrcpy(Msg.To);

		if ((fmsg->from != NULL) && (fmsg->from->name != NULL)) {
			strcpy(Msg.From, fmsg->from->name);
			Syslog('n', "Msg.From: %s", printable(Msg.From, 0));
			fromname = xstrcpy(Msg.From);
		} else {
			Syslog('n', "Warning: no Msg.From name found");
		}

		strcpy(Msg.Subject, fmsg->subj);
		subj = xstrcpy(Msg.Subject);
		Msg.Echomail = TRUE;
		Msg.Written = fmsg->date;
		Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
		sprintf(Msg.FromAddress, "%s", aka2str(msgs.Aka));
		/*
		 * These are the only usefull flags in echomail
		 */
		if ((fmsg->flags & M_PVT) && ((msgs.MsgKinds == BOTH) || (msgs.MsgKinds == PRIVATE)))
			Msg.Private = TRUE;
		if (fmsg->flags & M_FILE)
			Msg.FileAttach = TRUE;

		/*
		 * Set MSGID and REPLYID crc.
		 */
		if (fmsg->msgid_a != NULL) {
			crc2 = -1;
			Msg.MsgIdCRC = upd_crc32(fmsg->msgid_a, crc2, strlen(fmsg->msgid_a));
		}
		if (fmsg->reply_a != NULL) {
			crc2 = -1;
			Msg.ReplyCRC = upd_crc32(fmsg->reply_a, crc2, strlen(fmsg->reply_a));
		}

		if (Msg_Open(msgs.Base)) {
			if (Msg_Lock(30L)) {
				rewind(ofp);
				fgets(temp, 2048, ofp); /* "Eat" the first line AREA:... */
				Msg_Write(ofp);
				Msg_AddMsg();
				Msg_UnLock();
				echo_in++;
				Syslog('+', "Newsgate %s => %s msg %ld", msgs.Newsgroup, msgs.Tag, Msg.Id);
			}
			Msg_Close();
			StatAdd(&msgs.Received, 1);
			time(&msgs.LastRcvd);
			StatAdd(&mgroup.MsgsRcvd, 1);
			time(&mgroup.LastDate);
		}

		/*
		 * Now start exporting this echomail.
		 */
		First = TRUE;
		while (GetMsgSystem(&Link, First)) {
			First = FALSE;
			if ((Link.aka.zone) && (Link.sendto) && (!Link.pause) && (!Link.cutoff)) {
				if (SearchNode(Link.aka)) {
					StatAdd(&nodes.MailSent, 1L);
					UpdateNode();
					SearchNode(Link.aka);
				}
				echo_out++;
				Syslog('n', "Export to %s", aka2str(Link.aka));
				From = fido2faddr(msgs.Aka);
				EchoOut(From, Link.aka, ofp, fmsg->flags, 0, fmsg->date);
				tidy_faddr(From);
			}
		}

		free(fromname);
		free(toname);
		free(subj);
                fclose(ofp);
        } while (needsplit);
	tidy_falist(&sbl);
	tidy_falist(&ptl);
	free(temp);
	news_imp++;
	tidyrfc(msg);
	tidy_ftnmsg(fmsg);
	UpdateMsgs();

	return RETVAL_OK;
}



int get_xover(char *grpname, long startnr, long endnr, List **art)
{
	char		cmd[81], *ptr, *ptr2, *resp, *p;
	int		retval, dupe, done = FALSE;
	long		nr;
	unsigned long	crc;
	POverview	pov;

	sprintf(cmd, "XOVER %ld-%ld\r\n", startnr, endnr);
	if ((retval = nntp_cmd(cmd, 224))) {
		switch (retval) {
			case 412:
				WriteError("No newsgroup selected");
				return RETVAL_NOXOVER;
			case 502:
				WriteError("Permission denied");
				return RETVAL_NOXOVER;
			case 420:
				Syslog('n', "No articles in group %s", grpname);
				return RETVAL_OK;
		}
	}

	while (done == FALSE) {
		resp = nntp_receive();
		if ((strlen(resp) == 1) && (strcmp(resp, ".") == 0)) {
			done = TRUE;
		} else {
			Marker();
			pov = xoverview;
			ptr = resp;
			ptr2 = ptr;

			/*
			 * First item is the message number.
			 */
			while (*ptr2 != '\0' && *ptr2 != '\t')
				ptr2++;
			if (*ptr2 != '\0')
				*(ptr2) = '\0';
			nr = atol(ptr);
			ptr = ptr2;
			ptr++;

			/*
			 * Search the message-id
			 */
			while (*ptr != '\0' && pov != NULL && strcmp(pov->header, "Message-ID:") != 0) {
				/*
				 * goto the next field, past the tab.
				 */
				pov = pov->next;

				while (*ptr != '\t' && *ptr != '\0')
					ptr++;
				if (*ptr != '\0')
					ptr++;
			}
			if (*ptr != '\0' && pov != NULL) {
				/*
				 * Found it, now find start of msgid
				 */
				while (*ptr != '\0' && *ptr != '<')
					ptr++;
				if(ptr != '\0') {
					ptr2 = ptr;
					while(*ptr2 != '\0' && *ptr2 != '>')
						ptr2++;
					if (*ptr2 != '\0') {
						*(ptr2+1) = '\0';
						p = xstrcpy(ptr);
						p = xstrcat(p, grpname);
						crc = str_crc32(p);
						dupe = CheckDupe(crc, D_NEWS, CFG.nntpdupes);
						fill_artlist(art, ptr, nr, dupe);
						free(p);
						if (CFG.slow_util && do_quiet)
							usleep(1);
					}
				}
			}
		}
	}

	return RETVAL_OK;
}



int get_xoverview(void)
{
	int		retval, len, full, done = FALSE;
	char		*resp;
	POverview	tmp, curptr = NULL;

	Syslog('n', "Getting overview format list");
	if ((retval = nntp_cmd((char *)"LIST overview.fmt\r\n", 215)) == 0) {
		while (done == FALSE) {
			resp = nntp_receive();
			if ((strcmp(resp, ".") == 0) && (strlen(resp) == 1)) {
				done = TRUE;
			} else {
				len = strlen(resp);
				/*
				 * Check for the full flag, which means the field name
				 * is in the xover string.
				 */
				full = (strstr(resp, ":full") == NULL) ? FALSE : TRUE;
				/*
				 * Now get rid of everything back to :
				 */
				while (resp[len] != ':')
					resp[len--] = '\0';
				len++;

				tmp = malloc(sizeof(Overview));
				tmp->header = calloc(len + 1, sizeof(char));
				strncpy(tmp->header, resp, len);
				tmp->header[len] = '\0';
				tmp->next = NULL;
				tmp->field = NULL;
				tmp->fieldlen = 0;
				tmp->full = full;

				if (curptr == NULL) {
					/* at head of list */
					curptr = tmp;
					xoverview = tmp;
				} else {
					/* add to linked list */
					curptr->next = tmp;
					curptr = tmp;
				}
			}
		}

		if ((tmp = xoverview) != NULL) {
			Syslog('N', "--Xoverview.fmt list");
			while (tmp != NULL) {
				if (tmp->header != NULL) {
					Syslog('N', "item = %s -- full = %s", tmp->header, tmp->full ? "True":"False");
				}
				tmp = tmp->next;
			}
		}
	} else {
		return 1;
	}
	return 0;
}



int needputrfc(rfcmsg *msg)
{
	faddr	*ta;

	/* 0-junk, 1-kludge, 2-pass */

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
	if (!strcasecmp(msg->key,"Received")) return 0;
	if (!strcasecmp(msg->key,"From")) {
		if ((ta = parsefaddr(msg->val))) {
			tidy_faddr(ta);
			return 0;
		} else {
			return 2;
		}
	}
	if (!strcasecmp(msg->key,"To")) {
		return 0;
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


