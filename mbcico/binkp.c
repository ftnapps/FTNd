/*****************************************************************************
 *
 * $Id$
 * Purpose .................: Fidonet binkd protocol
 * Binkp protocol copyright : Dima Maloff.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/dbnode.h"
#include "../lib/clcomm.h"
#include "ttyio.h"
#include "session.h"
#include "statetbl.h"
#include "config.h"
#include "emsi.h"
#include "openfile.h"
#include "respfreq.h"
#include "filelist.h"
#include "opentcp.h"
#include "rdoptions.h"
#include "lutil.h"
#include "binkp.h"
#include "config.h"


static char	rbuf[2048];

void		binkp_send_data(char *, int);
void		binkp_send_control(int id, ...);
int		binkp_recv_frame(char *, int *, int *);
void		binkp_settimer(int);
int		resync(off_t);


static int	orgbinkp(void);
static int	ansbinkp(void);
static int	binkp_batch(file_list *);

extern char	*ttystat[];
extern int	Loaded;

extern unsigned long	sentbytes;
extern unsigned long	rcvdbytes;

typedef enum {RxWaitFile, RxAcceptFile, RxReceData, RxWriteData, RxEndOfBatch, RxDone} RxType;
typedef enum {TxGetNextFile, TxTryRead, TxReadSend, TxWaitLastAck, TxDone} TxType;
typedef enum {InitTransfer, Switch, Receive, Transmit} TransferType;
typedef enum {Ok, Failure, Continue} TrType;

static int	RxState;
static int	TxState;
static int	TfState;
static time_t	Timer;
static int	NRflag = FALSE;
static int	MBflag = FALSE;
static int	NDflag = FALSE;
static int	CRYPTflag = FALSE;
static int	CRAMflag = FALSE;
unsigned long	nethold, mailhold;
int		transferred = FALSE;
int		batchnr = 0;



int binkp(int role)
{
	int		rc = 0;
	fa_list		*eff_remote;
	file_list	*tosend = NULL, *request = NULL, *respond = NULL, *tmpfl;
	char		*nonhold_mail;

	if (role == 1) {
		Syslog('+', "BINKP start outbound session");
		IsDoing("Binkp %s outb", ascfnode(remote->addr, 0x0f));
		if (orgbinkp()) {
			rc = 5;
		}
	} else {
		Syslog('+', "BINKP start inbound session");
		IsDoing("Answer binkp");
		if (ansbinkp()) {
			rc = 5;
		}
	}
	
	if (rc) {
		Syslog('!', "BINKP session failed");
		return rc;
	}

	nonhold_mail = (char *)ALL_MAIL;
	eff_remote = remote;
	/*
	 * If remote doesn't have the 8.3 flag set, allow long filenames.
	 */
	if (!nodes.FNC)
		remote_flags &= ~SESSION_FNC;
	
	tosend = create_filelist(eff_remote, nonhold_mail, 0);
	request = create_freqlist(remote);

	if (request != NULL) {
		Syslog('b', "Inserting request list");
		tmpfl = tosend;
		tosend = request;
		for (; request->next; request = request->next);
		request->next = tmpfl;

		request = NULL;
	}

	rc = binkp_batch(tosend);
	tidy_filelist(tosend, (rc == 0));
	tosend = NULL;

	if ((rc == 0) && transferred && MBflag) {
		/*
		 * Running Multiple Batch, only if last batch actually
		 * did transfer some data.
		 */
		respond = respond_wazoo();
		/*
		 * Just create the tosend list again, there may be something
		 * ready again for this node.
		 */
		tosend = create_filelist(eff_remote, nonhold_mail, 0);
		for (tmpfl = tosend; tmpfl->next; tmpfl = tmpfl->next);
		tmpfl->next = respond;
		rc = binkp_batch(tosend);
		tmpfl->next = NULL;
	}

	Syslog('+', "BINKP end transfer rc=%d", rc);
	closetcp();

	if (!MBflag) {
		/*
		 *  In singe batch mode we process filerequests after the batch.
		 *  The results will be put on hold for the calling node.
		 */
		respond = respond_wazoo();
		for (tmpfl = respond; tmpfl; tmpfl = tmpfl->next) {
			if (strncmp(tmpfl->local, "/tmp", 4)) {
				attach(*remote->addr, tmpfl->local, LEAVE, 'h');
				Syslog('+', "Put on hold: %s", MBSE_SS(tmpfl->local));
			} else {
				file_mv(tmpfl->local, pktname(remote->addr, 'h'));
				Syslog('+', "New netmail: %s", pktname(remote->addr, 'h'));
			}
		}
	}

	tidy_filelist(request, (rc == 0));
	tidy_filelist(tosend, (rc == 0));
	tidy_filelist(respond, 0);

	rc = abs(rc);
	return rc;
}



int resync(off_t off)
{
	return 0;
}



/*
 *  Transmit data frame
 */
void binkp_send_data(char *buf, int len)
{
	unsigned short	header = 0;

	Syslog('B', "send_data(%d)", len);
	header = ((BINKP_DATA_BLOCK + len) & 0xffff);

	PUTCHAR((header >> 8) & 0x00ff);
	PUTCHAR(header & 0x00ff);
	if (len)
		PUT(buf, len);
	FLUSHOUT();
	binkp_settimer(BINKP_TIMEOUT);
}



/*
 *  Transmit control frame
 */
void binkp_send_control(int id,...)
{
	va_list		args;
	char		*fmt, *s;
	binkp_frame	frame;
	static char	buf[1024];
	int		sz;

	va_start(args, id);
	fmt = va_arg(args, char*);

	if (fmt) {
		vsprintf(buf, fmt, args);
		sz = ((1 + strlen(buf)) & 0x7fff);
	} else {
		buf[0]='\0';
		sz = 1;
	}

	frame.header = ((BINKP_CONTROL_BLOCK + sz) & 0xffff);
	frame.id = (char)id;
	frame.data = buf;

	s = (unsigned char *)malloc(sz + 2 + 1);
	s[sz + 2] = '\0';
	s[0] = ((frame.header >> 8)&0xff);
	s[1] = (frame.header & 0xff);
	s[2] = frame.id;
	if (frame.data[0])
		strncpy(s + 3, frame.data, sz-1);
	
	PUT(s, sz+2);
	FLUSHOUT();

	free(s);
	va_end(args);
	binkp_settimer(BINKP_TIMEOUT);
}



/*
 *  Receive control frame
 */
int binkp_recv_frame(char *buf, int *len, int *cmd)
{
	int	b0, b1;

	*len = *cmd = 0;

	b0 = GETCHAR(180);
	if (tty_status)
		goto to;
	if (b0 & 0x80)
		*cmd = 1;

	b1 = GETCHAR(1);
	if (tty_status)
		goto to;

	*len = (b0 & 0x7f) << 8;
	*len += b1;

	GET(buf, *len, 120);
	buf[*len] = '\0';
	if (tty_status)
		goto to;

to:
	if (tty_status)
		WriteError("TCP receive error: %d %s", tty_status, ttystat[tty_status]);
	return tty_status;
}



void binkp_settimer(int interval)
{
	Timer = time((time_t*)NULL) + interval;
}



int binkp_expired(void);
int binkp_expired(void)
{
	time_t	now;

	now = time(NULL);
	if (now >= Timer)
		Syslog('+', "Binkp: timeout");
	return (now >= Timer);
}



void b_banner(int);
void b_banner(int originate)
{
	time_t  t;

	binkp_send_control(MM_NUL,"SYS %s", CFG.bbs_name);
	binkp_send_control(MM_NUL,"ZYZ %s", CFG.sysop_name);
	binkp_send_control(MM_NUL,"LOC %s", CFG.location);
	binkp_send_control(MM_NUL,"NDL %s", CFG.Flags);
	t = time(NULL);
	binkp_send_control(MM_NUL,"TIME %s", rfcdate(t));
	binkp_send_control(MM_NUL,"VER mbcico/%s binkp/1.0", VERSION);
	if (strlen(CFG.Phone))
		binkp_send_control(MM_NUL,"PHN %s", CFG.Phone);
	if (strlen(CFG.comment))
		binkp_send_control(MM_NUL,"OPM %s", CFG.comment);
}



void b_nul(char *);
void b_nul(char *msg)
{
	if (strncmp(msg, "SYS ", 4) == 0) {
		Syslog('+', "System  : %s", msg+4);
		msg[40] = '\0';
		sprintf(history.system_name, "%s", msg+4);
	} else if (strncmp(msg, "ZYZ ", 4) == 0) {
		Syslog('+', "Sysop   : %s", msg+4);
		msg[40] = '\0';
		sprintf(history.sysop, "%s", msg+4);
	} else if (strncmp(msg, "LOC ", 4) == 0) {
		Syslog('+', "Location: %s", msg+4);
		msg[40] = '\0';
		sprintf(history.location, "%s", msg+4);
	} else if (strncmp(msg, "NDL ", 4) == 0)
		Syslog('+', "Flags   : %s", msg+4);
	else if (strncmp(msg, "TIME ", 5) == 0)
		Syslog('+', "Time    : %s", msg+5);
	else if (strncmp(msg, "VER ", 4) == 0)
		Syslog('+', "Uses    : %s", msg+4);
	else if (strncmp(msg, "PHN ", 4) == 0)
		Syslog('+', "Phone   : %s", msg+4);
	else if (strncmp(msg, "OPM ", 4) == 0)
		Syslog('+', "Remark  : %s", msg+4);
	else if (strncmp(msg, "TRF ", 4) == 0)
		Syslog('+', "Binkp: remote has %s mail/files for us", msg+4);
	else if (strncmp(msg, "OPT ", 4) == 0) {
		Syslog('+', "Options : %s", msg+4);
		if (strstr(msg, (char *)"NR") != NULL)
			NRflag = TRUE;
		if (strstr(msg, (char *)"MB") != NULL)
			MBflag = TRUE;
		if (strstr(msg, (char *)"ND") != NULL)
			NDflag = TRUE;
		if (strstr(msg, (char *)"CRYPT") != NULL)
			CRYPTflag = TRUE;
		if (strstr(msg, (char *)"CRAM-") != NULL) {
			CRAMflag = TRUE;
		}
	} else
		Syslog('+', "M_NUL \"%s\"", msg);
}



/*
 * Originate a binkp session
 */
SM_DECL(orgbinkp, (char *)"orgbinkp")
SM_STATES
	waitconn,
	sendpass,
	waitaddr,
	authremote,
	waitok
SM_NAMES
	(char *)"waitconn",
	(char *)"sendpass",
	(char *)"waitaddr",
	(char *)"authremote",
	(char *)"waitok"
SM_EDECL
	faddr	*primary;
	char	*p, *q;
	int	i, rc, bufl, cmd;
	fa_list	**tmp, *tmpa;
	int	SendPass = FALSE;
	faddr	*fa, ra;

SM_START(waitconn)

SM_STATE(waitconn)

	Loaded = FALSE;
	Syslog('+', "Start binkp session with %s", ascfnode(remote->addr, 0x1f));
	b_banner(TRUE);
//	binkp_send_control(MM_NUL,"OPT NR");
	binkp_send_control(MM_NUL,"OPT MB");

	/*
	 * Build a list of aka's to send, the primary aka first.
	 */
	ra.zone  = remote->addr->zone;
	ra.net   = remote->addr->net;
	ra.node  = remote->addr->node;
	ra.point = remote->addr->point;

	primary = bestaka_s(remote->addr);
	p = xstrcpy(ascfnode(primary, 0x1f));

	/*
	 * Add all other aka's exept primary aka.
	 */
	for (i = 0; i < 40; i++)
		if ((CFG.aka[i].zone) && (CFG.akavalid[i]) &&
		    ((CFG.aka[i].zone != primary->zone) || (CFG.aka[i].net  != primary->net)  ||
		     (CFG.aka[i].node != primary->node) || (CFG.aka[i].point!= primary->point))) {
			p = xstrcat(p, (char *)" ");
			p = xstrcat(p, aka2str(CFG.aka[i]));
		}

	binkp_send_control(MM_ADR, "%s", p);
	free(p);
	tidy_faddr(primary);
	SM_PROCEED(sendpass)

SM_STATE(sendpass)

	if (strlen(nodes.Epasswd)) {
		SendPass = TRUE;
		binkp_send_control(MM_PWD, "%s", nodes.Epasswd);
	} else { 
		binkp_send_control(MM_PWD, "-");
	}

	SM_PROCEED(waitaddr)

SM_STATE(waitaddr)
	
	for (;;) {
		if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
			Syslog('!', "Error receiving remote info");
			SM_ERROR;
		}

		if (cmd) {
			if (rbuf[0] == MM_ADR) {
				p = xstrcpy(&rbuf[1]);
				tidy_falist(&remote);
				remote = NULL;
				tmp = &remote;

				for (q = strtok(p, " "); q; q = strtok(NULL, " "))
					if ((fa = parsefnode(q))) {
						*tmp = (fa_list*)malloc(sizeof(fa_list));
						(*tmp)->next = NULL;
						(*tmp)->addr = fa;
						tmp = &((*tmp)->next);
					} else {
						Syslog('!', "Bad remote address: \"%s\"", printable(q, 0));
						binkp_send_control(MM_ERR, "Bad address");
					}

				for (tmpa = remote; tmpa; tmpa = tmpa->next) {
					Syslog('+', "Address : %s", ascfnode(tmpa->addr, 0x1f));
					if (nodelock(tmpa->addr)) {
						binkp_send_control(MM_BSY, "Address %s locked", ascfnode(tmpa->addr, 0x1f));
						SM_ERROR;
					}

					/*
					 * With the loaded flag we prevent removing the noderecord
					 * when the remote presents us an address we don't know about.
					 */
					if (!Loaded) {
						if (noderecord(tmpa->addr))
							Loaded = TRUE;
					}
				}

				history.aka.zone  = remote->addr->zone;
				history.aka.net   = remote->addr->net;
				history.aka.node  = remote->addr->node;
				history.aka.point = remote->addr->point;
				sprintf(history.aka.domain, "%s", remote->addr->domain);

				SM_PROCEED(authremote)

			} else if (rbuf[0] == MM_BSY) {
				Syslog('!', "Remote is busy");
				SM_ERROR;

			} else if (rbuf[0] == MM_ERR) {
				Syslog('!', "Remote error: %s", &rbuf[1]);
				SM_ERROR;

			} else if (rbuf[0] == MM_NUL) {
				b_nul(&rbuf[1]);
			}
		}
	}

SM_STATE(authremote)

	rc = 0;
	for (tmpa = remote; tmpa; tmpa = tmpa->next) {
		if ((tmpa->addr->zone  == ra.zone) &&
		    (tmpa->addr->net   == ra.net) &&
		    (tmpa->addr->node  == ra.node) &&
		    (tmpa->addr->point == ra.point)) {
			rc = 1;		
		}
	}

	if (rc) {
		SM_PROCEED(waitok)
	} else {
		Syslog('!', "Error: the wrong node is reached");
		binkp_send_control(MM_ERR, "No AKAs in common or all AKAs busy");
		SM_ERROR;
	}

SM_STATE(waitok)

	for (;;) {
		if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
			Syslog('!', "Error waiting for remote acknowledge");
			SM_ERROR;
		}

		if (cmd) {
			if (rbuf[0] == MM_OK) {
			    if (SendPass)
				Syslog('+', "Password protected BINKP session");
			    else
				Syslog('+', "Unprotected BINKP session");
			    SM_SUCCESS;

			} else if (rbuf[0] == MM_BSY) {
				Syslog('!', "Remote is busy");
				SM_ERROR;

			} else if (rbuf[0] == MM_ERR) {
				Syslog('!', "Remote error: %s", &rbuf[1]);
				SM_ERROR;

			} else if (rbuf[0] == MM_NUL) {
				b_nul(&rbuf[1]);
			}
		}
	}

SM_END
SM_RETURN
	


/*
 * Answer a binkp session
 */
SM_DECL(ansbinkp, (char *)"ansbinkp")
SM_STATES
	waitconn,
	waitaddr,
	waitpwd,
	pwdack
SM_NAMES
	(char *)"waitconn",
	(char *)"waitaddr",
	(char *)"waitpwd",
	(char *)"pwdack"

SM_EDECL
	char    *p, *q;
	int     i, rc, bufl, cmd;
	fa_list **tmp, *tmpa;
	faddr   *fa;

SM_START(waitconn)

SM_STATE(waitconn)

	Loaded = FALSE;
	b_banner(FALSE);
	p = xstrcpy((char *)"");

	for (i = 0; i < 40; i++)
		if ((CFG.aka[i].zone) && (CFG.akavalid[i])) {
			p = xstrcat(p, (char *)" ");
			p = xstrcat(p, aka2str(CFG.aka[i]));
		}

	binkp_send_control(MM_ADR, "%s", p);
	free(p);
	SM_PROCEED(waitaddr)

SM_STATE(waitaddr)

        for (;;) {
		if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
			Syslog('!', "Error waiting for remote info");
			SM_ERROR;
		}

		if (cmd) {
			if (rbuf[0] == MM_ADR) {
				p = xstrcpy(&rbuf[1]);
				tidy_falist(&remote);
				remote = NULL;
				tmp = &remote;

				for (q = strtok(p, " "); q; q = strtok(NULL, " "))
					if ((fa = parsefnode(q))) {
						*tmp = (fa_list*)malloc(sizeof(fa_list));
						(*tmp)->next = NULL;
						(*tmp)->addr = fa;
						tmp = &((*tmp)->next);
					} else {
						Syslog('!', "Bad remote address: \"%s\"", printable(q, 0));
						binkp_send_control(MM_ERR, "Bad address");
					}

				for (tmpa = remote; tmpa; tmpa = tmpa->next) {
					Syslog('+', "Address : %s", ascfnode(tmpa->addr, 0x1f));
					if (nodelock(tmpa->addr)) {
						binkp_send_control(MM_BSY, "Address %s locked", ascfnode(tmpa->addr, 0x1f));
						SM_ERROR;
					}

					/*
					 * With the loaded flag we prevent removing the noderecord
					 * when the remote presents us an address we don't know about.
					 */
					if (!Loaded) {
						if (noderecord(tmpa->addr))
							Loaded = TRUE;
					}
				}

				for (tmpa = remote; tmpa; tmpa = tmpa->next) {
					if (((nlent = getnlent(tmpa->addr))) &&
					    (nlent->pflag != NL_DUMMY)) {
						Syslog('+', "Remote is a listed system");
						if (inbound)
							free(inbound);
						inbound = xstrcpy(CFG.inbound);
						break;
					}
				}
				if (nlent)
					rdoptions(Loaded);

				if (MBflag) {
					Syslog('b', "Remote supports MB");
					binkp_send_control(MM_NUL,"OPT MB");
				}

				history.aka.zone  = remote->addr->zone;
				history.aka.net   = remote->addr->net;
				history.aka.node  = remote->addr->node;
				history.aka.point = remote->addr->point;
				sprintf(history.aka.domain, "%s", remote->addr->domain);

				SM_PROCEED(waitpwd)

			} else if (rbuf[0] == MM_ERR) {
				Syslog('!', "Remote error: %s", &rbuf[1]);
				SM_ERROR;

			} else if (rbuf[0] == MM_NUL) {
				b_nul(&rbuf[1]);
			}
		}
	}

SM_STATE(waitpwd)

	for (;;) {
		if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
			Syslog('!', "Error waiting for password");
			SM_ERROR;
		}

                if (cmd) {
                        if (rbuf[0] == MM_PWD) {
				SM_PROCEED(pwdack)

                        } else if (rbuf[0] == MM_ERR) {
                                Syslog('!', "Remote error: %s", &rbuf[1]);
                                SM_ERROR;

                        } else if (rbuf[0] == MM_NUL) {
                                b_nul(&rbuf[1]);
                        }
		}
	}

SM_STATE(pwdack)
        if ((strcmp(&rbuf[1], "-") == 0) && !Loaded) {
		Syslog('+', "Node not in setup, unprotected BINKP session");
		binkp_send_control(MM_OK, "");
		SM_SUCCESS;
	} else if ((strcmp(&rbuf[1], "-") == 0) && Loaded && !strlen(nodes.Epasswd)) {
		Syslog('+', "Node in setup but no session password, unprotected BINKP session");
		binkp_send_control(MM_OK, "");
		SM_SUCCESS;
	} else if ((strcmp(&rbuf[1], nodes.Epasswd) == 0) && Loaded) {
		Syslog('+', "Password OK, protected BINKP session");
		if (inbound)
			free(inbound);
		inbound = xstrcpy(CFG.pinbound);
		binkp_send_control(MM_OK, "");
		SM_SUCCESS;
	} else {
		Syslog('?', "Password error: expected \"%s\", got \"%s\"", nodes.Epasswd, &rbuf[1]);
		binkp_send_control(MM_ERR, "*** Password error, check setup ***");
		SM_ERROR;
	}

SM_END
SM_RETURN



void fill_binkp_list(binkp_list **, file_list *, off_t);
void fill_binkp_list(binkp_list **bll, file_list *fal, off_t offs)
{
	binkp_list	**tmpl;
	struct stat	tstat;

	if (stat(fal->local, &tstat) != 0) {
		Syslog('!', "$Can't add %s to sendlist", fal->local);
		exit;
	}
	if (strstr(fal->remote, (char *)".pkt"))
		nethold += tstat.st_size;
	else
		mailhold += tstat.st_size;
	
	for (tmpl = bll; *tmpl; tmpl = &((*tmpl)->next));
	*tmpl = (binkp_list *)malloc(sizeof(binkp_list));

	(*tmpl)->next   = NULL;
	(*tmpl)->state  = NoState;
	(*tmpl)->get    = FALSE;
	(*tmpl)->local  = xstrcpy(fal->local);
	(*tmpl)->remote = xstrcpy(fal->remote);
	(*tmpl)->offset = offs;
	(*tmpl)->size   = tstat.st_size;
	(*tmpl)->date   = tstat.st_mtime;
}



char *lbstat[]={(char *)"None",
		(char *)"Sending",
		(char *)"IsSent",
		(char *)"Got",
		(char *)"Skipped",
		(char *)"Get"};


void debug_binkp_list(binkp_list **);
void debug_binkp_list(binkp_list **bll)
{
	binkp_list	*tmpl;

	Syslog('B', "Current filelist:");

	for (tmpl = *bll; tmpl; tmpl = tmpl->next)
		Syslog('B', "%s %s %s %ld", MBSE_SS(tmpl->local), MBSE_SS(tmpl->remote), lbstat[tmpl->state], tmpl->offset);
}



int binkp_batch(file_list *to_send)
{
	int		rc = 0, NotDone;
	static char	*txbuf, *rxbuf;
	FILE		*txfp = NULL;
	FILE		*rxfp = NULL;
	int		rxlen = 0, txlen = 0;
	long		txpos = 0, rxpos = 0;
	long		stxpos = 0;
	int		sverr, cmd = FALSE, GotFrame = FALSE;
	int		blklen = 0, c, Found = FALSE;
	unsigned short	header = 0;
	char		*rname, *lname, *gname;
	long		rsize, roffs, lsize, gsize, goffset;
	time_t		rtime, ltime, gtime;
	off_t		rxbytes;
	long		written;
	binkp_list	*bll = NULL, *tmp, *tmpg, *cursend = NULL;
	file_list	*tsl;
	struct timeval	rxtvstart, rxtvend;
	struct timeval	txtvstart, txtvend;
	struct timezone	tz;

	rxtvstart.tv_sec = rxtvstart.tv_usec = 0;
	rxtvend.tv_sec   = rxtvend.tv_usec   = 0;
	txtvstart.tv_sec = txtvstart.tv_usec = 0;
	txtvend.tv_sec   = txtvend.tv_usec   = 0;
	tz.tz_minuteswest = tz.tz_dsttime = 0;

	batchnr++;
	Syslog('+', "Binkp: starting batch %d", batchnr);
	txbuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
	rxbuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
	rname = calloc(512, sizeof(char));
	lname = calloc(512, sizeof(char));
	gname = calloc(512, sizeof(char));
	TfState = Switch;
	RxState = RxWaitFile;
	TxState = TxGetNextFile;
	binkp_settimer(BINKP_TIMEOUT);
	nethold = mailhold = 0L;
	transferred = FALSE;

	/*
	 *  Build a new filelist from the existing filelist.
	 *  This one is special for binkp behaviour.
	 */
	for (tsl = to_send; tsl; tsl = tsl->next) {
		if (tsl->remote != NULL)
			fill_binkp_list(&bll, tsl, 0L);
	}
	debug_binkp_list(&bll);

	Syslog('+', "Binkp: mail %ld, files %ld bytes", nethold, mailhold);
	binkp_send_control(MM_NUL, "TRF %ld %ld", nethold, mailhold);

	while ((RxState != RxDone) || (TxState != TxDone)) {

		Nopper();
		if (binkp_expired()) {
			Syslog('!', "Binkp: Transfer timeout");
			Syslog('b', "Binkp: TxState=%d, RxState=%d, rxlen=%d", TxState, RxState, rxlen);
			RxState = RxDone;
			TxState = TxDone;
			binkp_send_control(MM_ERR, "Transfer timeout");
			rc = -2;
			break;
		}

		/*
		 * Receiver binkp frame
		 */
		for (;;) {
			if (GotFrame) {
				Syslog('b', "WARNING: frame not processed");
				break;
			} else {
				c = GETCHAR(0);
				if (c < 0) {
					c = -c;
					if (c == STAT_TIMEOUT) {
						usleep(1);
						break;
					}
					Syslog('?', "Binkp: receiver status %s", ttystat[c]);
					TxState = TxDone;
					RxState = RxDone;
					rc = -c;
					break;
				} else {
					switch (rxlen) {
						case 0: header = c << 8;
							break;
						case 1: header += c;
							break;
						default:rxbuf[rxlen-2] = c;
					}
					if (rxlen == 1) {
						cmd = header & 0x8000;
						blklen = header & 0x7fff;
					}
					if ((rxlen == (blklen + 1) && (rxlen >= 1))) {
						GotFrame = TRUE;
						binkp_settimer(BINKP_TIMEOUT);
						rxbuf[rxlen-1] = '\0';
						break;
					}
					rxlen++;
				}
			}
		}

		/*
		 * Transmitter state machine
		 */
		switch (TxState) {
		case TxGetNextFile:
			for (tmp = bll; tmp; tmp = tmp->next) {
				if (tmp->state == NoState) {
					/*
					 * There is something to send
					 */
					struct flock	txflock;

					txflock.l_type   = F_RDLCK;
					txflock.l_whence = 0;
					txflock.l_start  = 0L;
					txflock.l_len    = 0L;

					txfp = fopen(tmp->local, "r");
					if (txfp == NULL) {
						sverr = errno;
						if ((sverr == ENOENT) || (sverr == EINVAL)) {
							Syslog('+', "Binkp: file %s doesn't exist, removing", MBSE_SS(tmp->local));
							tmp->state = Got;
						} else {
							WriteError("$Binkp: can't open %s, skipping", MBSE_SS(tmp->local));
							tmp->state = Skipped;
						}
						break;
					}

					if (fcntl(fileno(txfp), F_SETLK, &txflock) != 0) {
						WriteError("$Binkp: can't lock file %s, skipping", MBSE_SS(tmp->local));
						fclose(txfp);
						tmp->state = Skipped;
						break;
					}

					txpos = stxpos = tmp->offset;
					Syslog('+', "Binkp: send \"%s\" as \"%s\"", MBSE_SS(tmp->local), MBSE_SS(tmp->remote));
					Syslog('+', "Binkp: size %lu bytes, dated %s", (unsigned long)tmp->size, date(tmp->date));
					binkp_send_control(MM_FILE, "%s %lu %ld %ld", MBSE_SS(tmp->remote), 
						(unsigned long)tmp->size, (long)tmp->date, (unsigned long)tmp->offset);
					gettimeofday(&txtvstart, &tz);
					tmp->state = Sending;
					cursend = tmp;
					TxState = TxTryRead;
					transferred = TRUE;
					break;
				} /* if state == NoState */
			} /* for */

			if (tmp == NULL) {
				/*
				 * No more files to send
				 */
				TxState = TxWaitLastAck;
				Syslog('b', "Binkp: transmitter to WaitLastAck");
			}
			break;

		case TxTryRead:
			/*
			 * Check if there is room in the output buffer
			 */
			if ((WAITPUTGET(-1) & 2) != 0)
				TxState = TxReadSend;
			break;

		case TxReadSend:
			fseek(txfp, txpos, SEEK_SET);
			txlen = fread(txbuf, 1, SND_BLKSIZE, txfp);

			if (txlen == 0) {

				if (ferror(txfp)) {
					WriteError("$Binkp: error reading from file");
					TxState = TxGetNextFile;
					cursend->state = Skipped;
					debug_binkp_list(&bll);
					break;
				}

				/*
				 * Send empty dataframe, most binkp mailers need it to detect EOF.
				 */
				binkp_send_data(txbuf, 0);

				/*
				 * calculate time needed and bytes transferred
				 */
				gettimeofday(&txtvend, &tz);

				/*
				 * Close transmitter file
				 */
				fclose(txfp);

				if (txpos >= 0) {
					stxpos = txpos - stxpos;
					Syslog('+', "Binkp: OK %s", transfertime(txtvstart, txtvend, stxpos, TRUE));
				} else {
					Syslog('+', "Binkp: transmitter skipped file after %ld seconds", 
						txtvend.tv_sec - txtvstart.tv_sec);
				}

				cursend->state = IsSent;
				TxState = TxGetNextFile;
				break;
			} else {
				txpos += txlen;
				sentbytes += txlen;
				binkp_send_data(txbuf, txlen);
			}

			TxState = TxTryRead;
			break;

		case TxWaitLastAck:
			debug_binkp_list(&bll);
			NotDone = FALSE;
			for (tmp = bll; tmp; tmp = tmp->next)
				if ((tmp->state != Got) && (tmp->state != Skipped)) {
					NotDone = TRUE;
					break;
				}
			if (tmp == NULL) {
				TxState = TxDone;
				binkp_send_control(MM_EOB, "");
				Syslog('+', "Binkp: sending EOB");
			}
			break;

		case TxDone:
			break;
		}

		/*
		 * Process received frame
		 */
		if (GotFrame) {
			if (cmd) {
				switch (rxbuf[0]) {
				case MM_ERR:    Syslog('+', "Binkp: got ERR: %s", rxbuf+1);
						RxState = RxDone;
						TxState = TxDone;
						break;

				case MM_BSY:	Syslog('+', "Binkp: got BSY: %s", rxbuf+1);
						RxState = RxDone;
						TxState = TxDone;
						break;

				case MM_SKIP:   Syslog('+', "Got SKIP frame");
						Syslogp('+', printable(rxbuf+1, 0));
						break;

				case MM_GET:    Syslog('+', "Got GET frame");
						sscanf(rxbuf+1, "%s %ld %ld %ld", gname, &gsize, &gtime, &goffset);
						for (tmpg = bll; tmpg; tmpg = tmpg->next) {
						    if (strcasecmp(tmpg->remote, gname) == 0) {
							tmpg->state = NoState;
							tmpg->offset = goffset;
							Syslog('+', "Remote wants %s again, offset %ld", gname, goffset);
							TxState = TxGetNextFile;
						    }
						}
						break;

				case MM_GOT:    sscanf(rxbuf+1, "%s %ld %ld", lname, &lsize, &ltime);
						Found = FALSE;
						for (tmp = bll; tmp; tmp = tmp->next)
							if ((strcmp(lname, tmp->remote) == 0) && 
							    (lsize == tmp->size) && (ltime == tmp->date)) {
								Syslog('+', "Binkp: remote GOT \"%s\"", tmp->remote);
								tmp->state = Got;
								Found = TRUE;
							}
						if (!Found) {
							Syslog('!', "Binkp: unexpected GOT \"%s\"", rxbuf+1);
						}
						break;

				case MM_NUL:    b_nul(rxbuf+1);
						break;

				case MM_EOB:    Syslog('+', "Binkp: received EOB");
						RxState = RxEndOfBatch;
						break;

				case MM_FILE:   if ((RxState == RxWaitFile) || (RxState == RxEndOfBatch)) {
							RxState = RxAcceptFile;
							sscanf(rxbuf+1, "%s %ld %ld %ld", rname, &rsize, &rtime, &roffs);
						} else {
							Syslog('+', "Binkp: got unexpected FILE frame %s", rxbuf+1);
						}
						break;

				default:        Syslog('+', "Binkp: Unexpected frame %d", rxbuf[0]);
				}
			} else {
				if (blklen) {
					if (RxState == RxReceData) {
						written = fwrite(rxbuf, 1, blklen, rxfp);
						if (!written && blklen) {
							Syslog('+', "Binkp: file write error");
							RxState = RxDone;
						}
						rxpos += written;
						if (rxpos == rsize) {
							binkp_send_control(MM_GOT, "%s %ld %ld", rname, rsize, rtime);
							closefile(TRUE);
							rxpos = rxpos - rxbytes;
							gettimeofday(&rxtvend, &tz);
							Syslog('+', "Binkp: OK %s", transfertime(rxtvstart, rxtvend, rxpos, FALSE));
							rcvdbytes += rxpos;
							RxState = RxWaitFile;
							transferred = TRUE;
						}
					} else {
						Syslog('+', "Binkp: unexpected DATA frame %d", rxbuf[0]);
					}
				}
			}
			GotFrame = FALSE;
			rxlen = 0;
			header = 0;
			blklen = 0;
		}

		switch (RxState) {
		case RxWaitFile:
			break;

		case RxAcceptFile:
			Syslog('+', "Binkp: receive file \"%s\" date %s size %ld offset %ld", rname, date(rtime), rsize, roffs);
			rxfp = openfile(rname, rtime, rsize, &rxbytes, resync);
			gettimeofday(&rxtvstart, &tz);
			rxpos = 0;

			if (!diskfree(CFG.freespace)) {
				Syslog('+', "Binkp: low diskspace, sending BSY");
				binkp_send_control(MM_BSY, "Low diskspace, try again later");
				RxState = RxDone;
				TxState = TxDone;
				break;
			}

			if (rsize == rxbytes) {
				/*
				 * We already got this file, send GOT so it will
				 * be deleted at the remote.
				 */
				Syslog('+', "Binkp: already got %s, sending GOT", rname);
				binkp_send_control(MM_GOT, "%s %ld %ld", rname, rsize, rtime);
				RxState = RxWaitFile;
				rxfp = NULL;
			} else if (!rxfp) {
				/*
				 * Some error, request to skip it
				 */
				Syslog('+', "Binkp: error file %s, sending SKIP", rname);
				binkp_send_control(MM_SKIP, "%s %ld %ld", rname, rsize, rtime);
				RxState = RxWaitFile;
			} else {
				RxState = RxReceData;
			}
			break;

		case RxReceData:
			break;

		case RxEndOfBatch:
			if (TxState == TxDone)
				RxState = RxDone;
			break;

		case RxDone:
			break;
		}
	}

	debug_binkp_list(&bll);

	/*
	 *  Process all send files.
	 */
	for (tsl = to_send; tsl; tsl = tsl->next) {
		if (tsl->remote == NULL) {
			execute_disposition(tsl);
		} else {
			for (tmp = bll; tmp; tmp = tmp->next) {
				if ((strcmp(tmp->local, tsl->local) == 0) &&
				    ((tmp->state == Got) || (tmp->state == Skipped))) {
					execute_disposition(tsl);
				}
			}
		}
	}

	for (tmp = bll; bll; bll = tmp) {
		tmp = bll->next;
		if (bll->local)
			free(bll->local);
		if (bll->remote)
			free(bll->remote);
		free(bll);
	}

	free(txbuf);
	free(rxbuf);
	free(rname);
	free(lname);
	free(gname);
	Syslog('+', "Binkp: batch %d completed rc=%d", batchnr, rc);
	return rc;
}


