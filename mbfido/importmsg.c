/*****************************************************************************
 *
 * File ..................: tosser/importmsg.c
 * Purpose ...............: Import a message
 * Last modification date : 05-Jul-2001
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
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbmsgs.h"
#include "../lib/dbdupe.h"
#include "../lib/dbuser.h"
#include "../lib/dbftn.h"
#include "echoout.h"
#include "mkrfcmsg.h"
#include "importmsg.h"
#include "postnetmail.h"
#include "rollover.h"



/*
 * External declarations
 */
extern	int	do_quiet;
extern	int	do_unsec;
extern	int	check_dupe;
extern	int	autocrea;
extern	time_t	t_start;
extern	int	most_debug;


/*
 * Global variables
 */
extern	int	echo_in;		/* Echomail received		    */
extern	int	echo_imp;		/* Echomail imported		    */
extern	int	echo_out;		/* Echomail forwarded		    */
extern	int	echo_bad;		/* Bad echomail			    */
extern	int	echo_dupe;		/* Dupe echomail		    */
extern	char	*subj;			/* Message subject		    */
char		*msgid = NULL;		/* Message id string		    */

#define	MAXPATH	73
#define	MAXSEEN	70


void tidy_qualify(qualify **);
void fill_qualify(qualify **, fidoaddr, int, int);
void dlog_qualify(qualify **, char *);


void tidy_qualify(qualify **qal)
{
	qualify	*tmp, *old;

	for (tmp = *qal; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*qal = NULL;
}



void fill_qualify(qualify **qal, fidoaddr aka, int orig, int insb)
{
	qualify		*tmp;

	tmp = (qualify *)malloc(sizeof(qualify));
	tmp->next     = *qal;
	tmp->aka      = aka;
	tmp->inseenby = insb;
	tmp->send     = ((!insb) && (!orig));
	tmp->orig     = orig;
	*qal = tmp;
}



void dlog_qualify(qualify **qal, char *msg)
{
	qualify	*tmpl;

	for (tmpl = *qal; tmpl; tmpl = tmpl->next) {
		Syslog('m', "%s InSB=%s Snd=%s Org=%s", 
			aka2str(tmpl->aka), tmpl->inseenby ? "True":"False",
			tmpl->send ? "True":"False",
			tmpl->orig ? "True":"False");
	}
}



/*
 * Import 1 message, forward if needed.
 * pkt_from, from, to, subj, orig, mdate, flags, cost, file
 *
 *  1 - Cannot open message base.
 *  2 - Cannot open mareas.data
 *  3 - Echomail without Origin line.
 *  4 - Echomail from unknown node, disconnected node.
 *  5 - Locking error.
 *
 * For echomail, the crc32 is calculated over the ^AREA kludge, subject, 
 * message date, origin line, message id.
 */
int importmsg(faddr *p_from, faddr *f, faddr *t, char *orig, time_t mdate, int flags, int cost, FILE *fp, off_t orig_off)
{
	char		*buf, *marea = NULL, *reply = NULL;
	char		*p, *q, *l, *r;
	int		echomail = FALSE, First = FALSE, email = FALSE;
	int		rc = 0, i, topt = 0, fmpt = 0, kludges = TRUE;
	faddr		*ta, *Faddr;
	int		result, dupe = FALSE, bad = FALSE;
	unsigned long	crc, crc2;
	char		sbe[16];
	sysconnect	Link;
	fa_list		*sbl = NULL, *ptl = NULL, *tmpl;
	qualify		*qal = NULL, *tmpq;
	FILE		*nfp, *qp;
	int		Known = FALSE, seenlen, oldnet;
	int		FirstLine;

	if (CFG.slow_util && do_quiet)
		usleep(1);

	memset(&Link, 0, sizeof(Link));
	msgid = NULL;

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
		Known = TRUE;
	}

	crc = 0xffffffff;
	buf = calloc(2048, sizeof(char));
	marea = NULL;

	/*
	 * First read the message for kludges we need.
	 */
	rewind(fp);

	FirstLine = TRUE;
	while ((fgets(buf, 2048, fp)) != NULL) {

		Striplf(buf);

		/*
		 * Check all sort of relevant kludges.
		 */
		if (FirstLine && (!strncmp(buf, "AREA:", 5))) {

			echo_in++;
			marea = xstrcpy(tu(buf + 5));

			if (orig == NULL) {
				Syslog('!', "Echomail without Origin line");
				echo_bad++;
				bad = TRUE;
				free(buf);
				free(marea);
				return 3;
			}

			if (!SearchMsgs(marea)) {
				WriteError("Unknown echo area %s", marea);
				if (autocrea) {
					autocreate(marea, p_from);
					if (!SearchMsgs(marea)) {
						WriteError("Autocreate of area %s failed.", area);
						echo_bad++;
						bad = TRUE;
						free(marea);
						free(buf);
						return 4;
					}       
				} else {
					echo_bad++;
					bad = TRUE;
					free(buf);
					free(marea);
					return 4;
				}
			}
			crc = upd_crc32(buf, crc, strlen(buf));
			echomail = TRUE;
			free(marea);

			/*
			 * Check if node is allowed to enter echomail in this area.
			 */
			if (!bad) {
				bad = TRUE;
				First = TRUE;
				while (GetMsgSystem(&Link, First)) {
					First = FALSE;
					if ((p_from->zone == Link.aka.zone) &&
					    (p_from->net  == Link.aka.net) &&
					    (p_from->node == Link.aka.node)) {
						bad = FALSE;
						break;
					}
				}
				if (bad && (msgs.UnSecure || do_unsec)) {
					bad = FALSE;
					memset(&Link, 0, sizeof(Link));
				}
				if (bad) {
					Syslog('+', "Node %s not connected to area %s", ascfnode(p_from, 0x1f), msgs.Tag);
					free(buf);
					echo_bad++;
					return 4;
				} 

				if (Link.cutoff && !bad) {
					Syslog('+', "Echomail from %s in %s refused, cutoff", ascfnode(p_from, 0x1f), msgs.Tag);
					free(buf);
					bad = TRUE;
					echo_bad++;
					return 4;
				}
				if (!Link.receivefrom && !bad) {
					Syslog('+', "Echomail from %s in %s refused, read only", ascfnode(p_from, 0x1f), msgs.Tag);
					bad = TRUE;
					free(buf);
					echo_bad++;
					return 4;
				}

				if (CFG.toss_old) {
					if (((t_start - mdate) / 86400) > CFG.toss_old) {
						Syslog('+', "Rejecting msg: too old, %s", rfcdate(mdate));
						bad = TRUE;
						free(buf);
						echo_bad++;
						return 4;
					}
				}
			}

                        StatAdd(&msgs.Received, 1);
                        time(&msgs.LastRcvd);
                        StatAdd(&mgroup.MsgsRcvd, 1);
                        time(&mgroup.LastDate);
                        UpdateMsgs();
		}

		if (*buf != '\001')
			FirstLine = FALSE;

		if (!echomail) {

			/*
			 * Check for X-FTN- kludges, this could be gated email.
			 * This should be impossible.
			 */
			if (!strncmp(buf, "\001X-FTN-", 7)) {
				email = TRUE;
				Syslog('?', "Warning: detected ^aX-FTN- kludge in netmail");
			}

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
			 * Now check FMPT and TOPT kludges
			 */
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

			if (!strncmp(buf, "\001MSGID: ", 8)) {
				msgid = xstrcpy(buf + 8);
				/*
				 *  Extra test to see if the mail comes from a pointaddress.
				 */
				l = strtok(buf," \n");
				l = strtok(NULL," \n");
				if ((ta = parsefnode(l))) {
					if (ta->zone == f->zone && ta->net == f->net && ta->node == f->node && !fmpt && ta->point) {
						Syslog('m', "Setting pointinfo (%d) from MSGID", ta->point);
						fmpt = f->point = ta->point;
					}
					tidy_faddr(ta);
				}
			}
		} /* if netmail */

		if (!strncmp(buf, "\001REPLYTO: ", 10))
			reply = xstrcpy(buf + 10);
		if (!strncmp(buf, "SEEN-BY:", 8)) {
			if (Link.aka.zone == msgs.Aka.zone) {
				p = xstrcpy(buf + 9);
				fill_list(&sbl, p, NULL, FALSE);
				free(p);
			} else
				Syslog('m', "Strip zone SB lines");
		}
		if (!strncmp(buf, "\001PATH:", 6)) {
			p = xstrcpy(buf + 7);
			fill_path(&ptl, p);
			free(p);
		}

	} /* end of checking kludges */

	/*
	 * Handle netmail
	 */
	if (!echomail) {
		/*
		 *  Only set point info if there was any info.
		 *  GoldED doesn't set FMPT and TOPT kludges.
		 */
		if (fmpt)
			f->point = fmpt;
		if (topt)
			t->point = topt;
		rc = postnetmail(fp, f, t, orig, subj, mdate, flags, TRUE);
		free(buf);
		return rc;
	} /* if !echomail */

	/*
	 * Handle echomail
	 */
	if (echomail && (!bad)) {
		/*
		 * First, further dupe checking.
		 */
		crc = upd_crc32(subj, crc, strlen(subj));

		if (orig == NULL)
			Syslog('!', "No origin line found");
		else
			crc = upd_crc32(orig, crc, strlen(orig));

		crc = upd_crc32((char *)&mdate, crc, sizeof(mdate));

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
		if (dupe)
			echo_dupe++;
	}

	if ((echomail) && (!bad) && (!dupe) && (!msgs.UnSecure) && (!do_unsec)) {

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
					if ((CFG.akavalid[i]) &&
					    (CFG.aka[i].zone == t->zone) &&
					    (CFG.aka[i].net  == t->net) &&
					    (CFG.aka[i].node == t->node))
						bad = FALSE; /* Undo the result */
				}
			}
		}
		if (bad) {
			echo_bad++;
			WriteError("Msg in %s not for us (%s) but for %s", msgs.Tag, aka2str(msgs.Aka), ascfnode(t,0x1f));
		}
	}

	if ((echomail) && (!bad) && (!dupe)) {

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
			fill_list(&sbl, sbe, NULL, FALSE);
		}

		/*
		 * Add more aka's to SEENBY if in the same zone as our system.
		 * When ready filter dupe's, there is at least one.
		 */
		for (i = 0; i < 40; i++) {
			if (CFG.akavalid[i] && (msgs.Aka.zone == CFG.aka[i].zone) &&
			    !((msgs.Aka.net == CFG.aka[i].net) && (msgs.Aka.node == CFG.aka[i].node))) {
				sprintf(sbe, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
				fill_list(&sbl, sbe, NULL, FALSE);
			}
		}
		uniq_list(&sbl);
	}

	/*
	 * Add our system to the path for later export.
	 */
	sprintf(sbe, "%u/%u", msgs.Aka.net, msgs.Aka.node);
	fill_path(&ptl, sbe);

	if (echomail) {
		/*
		 * Build a list of qualified systems to receive this message.
		 * Complete the SEEN-BY lines.
		 */
		First = TRUE;
		while (GetMsgSystem(&Link, First)) {
			First = FALSE;
			if ((Link.aka.zone) && (Link.sendto) && (!Link.pause) && (!Link.cutoff)) {
				Faddr = fido2faddr(Link.aka);
				fill_qualify(&qal, Link.aka, ((p_from->zone  == Link.aka.zone) && 
				      (p_from->net   == Link.aka.net) && (p_from->node  == Link.aka.node) && 
				      (p_from->point == Link.aka.point)), in_list(Faddr, &sbl, FALSE));
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
				fill_list(&sbl, sbe, NULL, FALSE);
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
		if ((nfp = tmpfile()) == NULL) {
			WriteError("$Unable to open tmpfile");
		}
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
		 * Ensure that it will not match for 
		 * the first entry.
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
		 * Import this message.
		 */
		if (bad) {
			if (strlen(CFG.badboard) == 0) {
				Syslog('+', "Killing bad message");
				tidy_falist(&sbl);
				tidy_falist(&ptl);
				tidy_qualify(&qal);
				free(buf);
				return 0;
			} else {
				if ((result = Msg_Open(CFG.badboard)))
					Syslog('+', "Tossing in bad board");
			}
		} else if (dupe) {
			if (strlen(CFG.dupboard) == 0) {
				Syslog('+', "Killing dupe message");
				tidy_falist(&sbl);
				tidy_falist(&ptl);
				tidy_qualify(&qal);
				free(buf);
				return 0;
			} else {
				if ((result = Msg_Open(CFG.dupboard)))
					Syslog('+', "Tossing in dupe board");
			}
		} else {
			result = Msg_Open(msgs.Base);
		}
		if (!result) {
			WriteError("Can't open JAMmb %s", msgs.Base);
			tidy_falist(&sbl);
			tidy_falist(&ptl);
			tidy_qualify(&qal);
			free(buf);
			return 1;
		}

		if (Msg_Lock(30L)) {
			if ((!dupe) && (!bad))
				echo_imp++;

			if (!do_quiet) {
				colour(3, 0);
				printf("\r%6u => %-40s\r", echo_in, msgs.Name);
				fflush(stdout);
			}

			Msg_New();

			/*
			 * Fill subfields
			 */
			strcpy(Msg.From, f->name);
			strcpy(Msg.To, t->name);
			strcpy(Msg.FromAddress, ascfnode(f,0x1f));
			strcpy(Msg.Subject, subj);
			Msg.Written = mdate;
			Msg.Arrived = time(NULL);
			Msg.Echomail = TRUE;

			/*
			 * These are the only usefull flags in echomail
			 */
			if ((flags & M_PVT) && ((msgs.MsgKinds == BOTH) || (msgs.MsgKinds == PRIVATE)))
				Msg.Private = TRUE;
			if (flags & M_FILE)
				Msg.FileAttach = TRUE;

			/*
			 * Set MSGID and REPLYID crc.
			 */
			if (msgid != NULL) {
				crc2 = -1;
				Msg.MsgIdCRC = upd_crc32(msgid, crc2, strlen(msgid));
			}
			if (reply != NULL) {
				crc2 = -1;
				Msg.ReplyCRC = upd_crc32(reply, crc2, strlen(reply));
			}

			/*
			 * Start write the message
			 * If not a bad or dupe message, eat the first
			 * line (AREA:tag).
			 */
			rewind(nfp);
			if (!dupe && !bad)
				fgets(buf , 256, nfp);
			Msg_Write(nfp);
			Msg_AddMsg();
			Msg_UnLock();
			Msg_Close();
		} else {
			Syslog('+', "Can't lock msgbase %s", msgs.Base);
			Msg_UnLock();
			Msg_Close();
			tidy_falist(&sbl);
			tidy_falist(&ptl);
			tidy_qualify(&qal);
			free(buf);
			return 5;
		}

		/*
		 * Forward to other links
		 */
		if ((!dupe) && (!bad)) {
			/*
			 * Now start exporting this echomail.
			 */
			for (tmpq = qal; tmpq; tmpq = tmpq->next) {
				if (tmpq->send) {
					if (SearchNode(tmpq->aka)) {
						StatAdd(&nodes.MailSent, 1L);
						UpdateNode();
						SearchNode(tmpq->aka);
					}
					echo_out++;
					EchoOut(p_from, tmpq->aka, nfp, flags, cost, mdate);
				}
			}

			/*
			 *  Gate to newsserver
			 */
			if (strlen(msgs.Newsgroup)) {
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
						fprintf(qp, "\n");
					}
					fprintf(qp, "%s\n", buf);
				}
				rewind(qp);
				most_debug = TRUE;
				mkrfcmsg(f, t, subj, orig, mdate, flags, qp, orig_off, FALSE);
				most_debug = FALSE;
				fclose(qp);
			}
		}
		fclose(nfp);
	}

	/*
	 * Free memory used by SEEN-BY, ^APATH and Qualified lines.
	 */
	tidy_falist(&sbl);
	tidy_falist(&ptl);
	tidy_qualify(&qal);

	if (rc < 0) 
		rc =-rc;
	free(buf);
	if (reply)
		free(reply);
	return rc;
}



void autocreate(char *marea, faddr *p_from)
{
	FILE            *pMsgs;
	char            temp[250];
	int             i;
	struct  _sysconnect syscon;

	if (!SearchMsgs((char *)"DEFAULT")){
		WriteError("Can't find DEFAULT area, can't autocreate:");
		autocrea = FALSE;
		return;
	}
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((pMsgs = fopen(temp, "r+")) == NULL) {
		WriteError("$Database error: Can't create %s", temp);
		return;
	}
	strncat(msgs.Name,marea,40-strlen(msgs.Name));
	strncpy(msgs.Tag,marea,50);
	strncpy(msgs.QWKname,marea,20);
	strncat(msgs.Base,marea,64-strlen(msgs.Base));
	fseek(pMsgs, 0, SEEK_END);
	Syslog('+', "Autocreate area %s", marea);

	memset(&syscon, 0, sizeof(syscon));
	syscon.aka.zone    = p_from->zone;
	syscon.aka.node    = p_from->node;
	syscon.aka.net     = p_from->net;
	if (SearchFidonet(p_from->zone)) 
		strcpy(syscon.aka.domain,fidonet.domain);
	else {
		WriteError("New area %s from node of unknown zone %d not created.", marea,p_from->zone);
		fclose(pMsgs);
		return;
	}       
	syscon.sendto      = TRUE;
	syscon.receivefrom = TRUE;
	if (msgs.Aka.zone == 0) {
		for (i = 0; i < 40; i++) {
			if (CFG.akavalid[i]) { 
				msgs.Aka.zone=CFG.aka[i].zone;
				msgs.Aka.net=CFG.aka[i].net;
				msgs.Aka.node=CFG.aka[i].node;
				msgs.Aka.point=CFG.aka[i].point;
				strcpy(msgs.Aka.domain,CFG.aka[i].domain);
				i=40;
			}
		}               
		for (i = 0; i < 40; i++) {
			if (CFG.akavalid[i] && (strcmp(CFG.aka[i].domain,msgs.Aka.domain)==0)) {
				msgs.Aka.zone=CFG.aka[i].zone;
				msgs.Aka.net=CFG.aka[i].net;
				msgs.Aka.node=CFG.aka[i].node;
				msgs.Aka.point=CFG.aka[i].point;
				strcpy(msgs.Aka.domain,CFG.aka[i].domain);
				i=40;
			}
		}
		for (i = 0; i < 40; i++) {
			if ((CFG.akavalid[i]) && (CFG.aka[i].zone == p_from->zone)) {
				msgs.Aka.zone=CFG.aka[i].zone;
				msgs.Aka.net=CFG.aka[i].net;
				msgs.Aka.node=CFG.aka[i].node;
				msgs.Aka.point=CFG.aka[i].point;
				strcpy(msgs.Aka.domain,CFG.aka[i].domain);
				i=40;
			}
		}
		for (i = 0; i < 40; i++) {
			if ((CFG.akavalid[i]) && (CFG.aka[i].zone == p_from->zone) && (CFG.aka[i].net  == p_from->net)) {
				msgs.Aka.zone=CFG.aka[i].zone;
				msgs.Aka.net=CFG.aka[i].net;
				msgs.Aka.node=CFG.aka[i].node;
				msgs.Aka.point=CFG.aka[i].point;
				strcpy(msgs.Aka.domain,CFG.aka[i].domain);
				i=40;
			}
		}
		for (i = 0; i < 40; i++) {
			if ((CFG.akavalid[i]) && (CFG.aka[i].zone == p_from->zone) &&
			    (CFG.aka[i].net  == p_from->net) && (CFG.aka[i].node == p_from->node)) {
				msgs.Aka.zone=CFG.aka[i].zone;
				msgs.Aka.net=CFG.aka[i].net;
				msgs.Aka.node=CFG.aka[i].node;
				msgs.Aka.point=CFG.aka[i].point;
				strcpy(msgs.Aka.domain,CFG.aka[i].domain);
				i=40;
			}
		}
	}
	fwrite(&msgs, msgshdr.recsize, 1, pMsgs);
	fwrite(&syscon, sizeof(syscon), 1, pMsgs);
	memset(&syscon, 0, sizeof(syscon));
	for (i = 1 ; i < CFG.toss_systems; i++ ) 
		fwrite(&syscon, sizeof(syscon), 1, pMsgs);
	fclose(pMsgs);
	return;
}


