/*****************************************************************************
 *
 * $Id$
 * Purpose .................: Fidonet binkp protocol
 * Binkp protocol copyright : Dima Maloff.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/nodelist.h"
#include "../lib/mbsedb.h"
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
#include "md5b.h"
#include "inbound.h"


/*
 * Safe characters for binkp filenames, the rest will be escaped.
 */
#define BNKCHARS    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@&=+%$-_.!()#|"



static char *bstate[] = {
    (char *)"M_NUL", (char *)"M_ADR", (char *)"M_PWD", (char *)"M_FILE", (char *)"M_OK",
    (char *)"M_EOB", (char *)"M_GOT", (char *)"M_ERR", (char *)"M_BSY", (char *)"M_GET",
    (char *)"M_SKIP"
};



extern char	*tempinbound;
extern char	*ttystat[];
extern int	Loaded;
extern pid_t	mypid;
extern struct sockaddr_in   peeraddr;
extern int	most_debug;


extern unsigned long	sentbytes;
extern unsigned long	rcvdbytes;

typedef enum {RxWaitF, RxAccF, RxReceD, RxWriteD, RxEOB, RxDone} RxType;
typedef enum {TxGNF, TxTryR, TxReadS, TxWLA, TxDone} TxType;
typedef enum {Ok, Failure, Continue} TrType;
typedef enum {No, WeCan, WeWant, TheyWant, Active} OptionState;
typedef enum {NoState, Sending, IsSent, Got, Skipped, Get} FileState;
typedef enum {InitTransfer, Switch, Receive, Transmit, DeinitTransfer} FtType;


static char *rxstate[] = { (char *)"RxWaitF", (char *)"RxAccF", (char *)"RxReceD", 
			   (char *)"RxWriteD", (char *)"RxEOB", (char *)"RxDone" };
static char *txstate[] = { (char *)"TxGNF", (char *)"TxTryR", (char *)"TxReadS", 
			   (char *)"TxWLA", (char *)"TxDone" };
static char *trstate[] = { (char *)"Ok", (char *)"Failure", (char *)"Continue" };
static char *opstate[] = { (char *)"No", (char *)"WeCan", (char *)"WeWant", (char *)"TheyWant", (char *)"Active" };
static char *lbstate[] = { (char *)"None", (char *)"Sending", (char *)"IsSent", (char *)"Got", (char *)"Skipped", (char *)"Get"};
static char *ftstate[] = { (char *)"InitTransfer", (char *)"Switch", (char *)"Receive", 
			   (char *)"Transmit", (char *)"DeinitTransfer" };


static time_t	Timer;
int		ext_rand = 0;


the_queue	    *tql = NULL;		/* The Queue List		    */
file_list	    *tosend = NULL;		/* Files to send		    */
file_list	    *respond = NULL;		/* Requests honored		    */
binkp_list	    *bll = NULL;		/* Files to send with status	    */
static binkp_list   *cursend = NULL;		/* Current file being transmitted   */
struct timeval      txtvstart;			/* Transmitter start time           */
struct timeval      txtvend;			/* Transmitter end time             */
struct timeval      rxtvstart;			/* Receiver start time              */
struct timeval      rxtvend;			/* Receiver end time                */


struct binkprec {
    int			Role;			/* 1=orig, 0=answer		    */
    int			RxState;		/* Receiver state		    */
    int			TxState;		/* Transmitter state		    */
    int			FtState;		/* File transfer state		    */
						/* Option flags			    */
    int			CRAMflag;		/* CRAM authentication		    */
						/* Session state		    */
    int			Secure;			/* Secure session		    */
    int			Major;			/* Major protocol version	    */
    int			Minor;			/* Minor protocol version	    */
    unsigned char	*MD_Challenge;		/* Received challenge data	    */
    int			PLZflag;		/* Zlib packet compression	    */
    int			cmpblksize;		/* Ideal next blocksize		    */

						/* Receiver buffer		    */
    char		*rxbuf;			/* Receiver buffer		    */
    int			GotFrame;		/* Frame complete flag		    */
    int			rxlen;			/* Frame length			    */
    int			cmd;			/* Frame command flag		    */
    int			blklen;			/* Frame blocklength		    */
    unsigned short	header;			/* Frame header			    */
    int			rc;			/* General return code		    */

    long		rsize;			/* Receiver filesize		    */
    long		roffs;			/* Receiver offset		    */
    char		*rname;			/* Receiver filename		    */
    time_t		rtime;			/* Receiver filetime		    */
    FILE		*rxfp;			/* Receiver file		    */
    off_t		rxbytes;		/* Receiver bytecount		    */
    int			rxpos;			/* Receiver position		    */
    int			rxcompressed;		/* Receiver compressed bytes	    */
    struct timezone	tz;			/* Timezone			    */
    int			DidSendGET;		/* Receiver send GET status	    */

    char		*txbuf;			/* Transmitter buffer		    */
    int			txlen;			/* Transmitter file length	    */
    FILE		*txfp;			/* Transmitter file		    */
    int			txpos;			/* Transmitter position		    */
    int			stxpos;			/* Transmitter start position	    */
    int			txcompressed;		/* Transmitter compressed bytes	    */

    int			local_EOB;		/* Local EOB sent		    */
    int			remote_EOB;		/* Got EOB from remote		    */
    int			messages;		/* Messages sent + rcvd		    */
    unsigned long	nethold;		/* Netmail on hold		    */
    unsigned long	mailhold;		/* Packed mail on hold		    */

    int			batchnr;
    int			msgs_on_queue;		/* Messages on the queue	    */
};


struct binkprec	bp;				/* Global structure		    */


/*
 * Prototypes
 */
TrType  binkp_receiver(void);                       /* Receiver routine             */
TrType  binkp_transmitter(void);                    /* Transmitter routine          */
int	binkp_send_frame(int, char *, int);	    /* Send cmd/data frame	    */
int	binkp_send_command(int, ...);		    /* Send command frame	    */
void	binkp_settimer(int);			    /* Set timeout timer	    */
int	binkp_expired(void);			    /* Timer expired?		    */
int	binkp_banner(void);			    /* Send system banner	    */
int	binkp_recv_command(char *, unsigned long *, int *);   /* Receive command frame	    */
void	parse_m_nul(char *);			    /* Parse M_NUL message	    */
int	binkp_poll_frame(void);			    /* Poll for a frame		    */
void	binkp_add_message(char *frame);		    /* Add cmd frame to queue	    */
int	binkp_process_messages(void);		    /* Process the queue	    */
int	binkp_resync(off_t);			    /* File resync		    */
char	*unix2binkp(char *);			    /* Binkp -> Unix escape	    */
char	*binkp2unix(char *);			    /* Unix -> Binkp escape	    */
void	fill_binkp_list(binkp_list **, file_list *, off_t); /* Build pending files  */
void	debug_binkp_list(binkp_list **);	    /* Debug pending files list	    */
int	binkp_pendingfiles(void);		    /* Count pending files	    */
void	binkp_clear_filelist(void);		    /* Clear current filelist	    */

static int  orgbinkp(void);			    /* Originate session state	    */
static int  ansbinkp(void);			    /* Answer session state	    */
static int  file_transfer(void);		    /* File transfer state	    */



/************************************************************************************/
/*
 *   General entry point
 */

int binkp(int role)
{
    int	    rc = 0;

#ifdef USE_NEWBINKP
    most_debug = TRUE;
#endif

    Syslog('+', "Binkp: start session");

    memset(&bp, 0, sizeof(bp));
    bp.Role = role;
    bp.CRAMflag = FALSE;
    bp.Secure = FALSE;
    bp.Major = 1;
    bp.Minor = 0;
    bp.MD_Challenge = NULL;
    bp.rxbuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
    bp.txbuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
    bp.rname = calloc(512, sizeof(char));
    bp.rxfp = NULL;
    bp.DidSendGET = FALSE;
    bp.local_EOB = FALSE;
    bp.remote_EOB = FALSE;
    bp.msgs_on_queue = 0;
    bp.cmpblksize = SND_BLKSIZE;
#ifdef	HAVE_ZLIB_H
    bp.PLZflag = WeCan;
#else
    bp.PLZflag = No;
#endif

    if (role == 1) {
	if (orgbinkp()) {
	    rc = MBERR_SESSION_ERROR;
	}
    } else {
	if (ansbinkp()) {
	    rc = MBERR_SESSION_ERROR;
	}
    }

    if (rc) {
	Syslog('!', "Binkp: session handshake failed");
	goto binkpend;
    }

    if (Loaded && nodes.NoBinkp11 && (bp.Major == 1) && (bp.Minor != 0)) {
	Syslog('+', "Binkp: forcing downgrade to binkp/1.0 protocol");
        bp.Major = 1;
        bp.Minor = 0;
    }

    if (localoptions & NOFREQS)
	session_flags &= ~SESSION_WAZOO;
    else
	session_flags |= SESSION_WAZOO;

    bp.FtState = InitTransfer;
    rc = file_transfer();

binkpend:   
    /*
     * Deinit
     */
    if (bp.rname)
	free(bp.rname);
    if (bp.MD_Challenge)
	free(bp.MD_Challenge);
    if (bp.rxbuf)
	free(bp.rxbuf);
    if (bp.txbuf)
	free(bp.txbuf);
    rc = abs(rc);

    Syslog('+', "Binkp: session finished, rc=%d", rc);
    return rc;
}


/************************************************************************************/
/*
 *   Originate Session Setup
 */

SM_DECL(orgbinkp, (char *)"orgbinkp")
SM_STATES
    ConnInit,
    WaitConn,
    SendPasswd,
    WaitAddr,
    AuthRemote,
    IfSecure,
    WaitOk,
    Opts
SM_NAMES
    (char *)"ConnInit",
    (char *)"WaitConn",
    (char *)"SendPasswd",
    (char *)"WaitAddr",
    (char *)"AuthRemote",
    (char *)"IfSecure",
    (char *)"WaitOk",
    (char *)"Opts"
SM_EDECL
    faddr	    *primary;
    char	    *p, *q, *pwd;
    int		    i, rc = 0, cmd, dupe, SendPass = FALSE, akas = 0;
    unsigned long   bufl;
    fa_list	    **tmp, *tmpa;
    faddr	    *fa, ra;

SM_START(ConnInit)

SM_STATE(ConnInit)

    SM_PROCEED(WaitConn)

SM_STATE(WaitConn)

    Loaded = FALSE;
    Syslog('+', "Binkp: node %s", ascfnode(remote->addr, 0x1f));
    IsDoing("Connect binkp %s", ascfnode(remote->addr, 0xf));

    /*
     * Build options we want
     */
    p = xstrcpy((char *)"OPT");

#ifdef HAVE_ZLIB_H
    if (bp.PLZflag == WeCan) {
	p = xstrcat(p, (char *)" PLZ");
	bp.PLZflag = WeWant;
	Syslog('b', "PLZflag WeCan => WeWant");
    }
#endif
    
    if (strcmp(p, (char *)"OPT"))
	rc = binkp_send_command(MM_NUL, p);
    free(p);
    if (rc) {
	SM_ERROR;
    }

    rc = binkp_banner();
    if (rc) {
	SM_ERROR;
    }

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
    for (i = 0; i < 40; i++) {
	if ((CFG.aka[i].zone) && (CFG.akavalid[i]) &&
	    ((CFG.aka[i].zone != primary->zone) || (CFG.aka[i].net  != primary->net)  ||
	    (CFG.aka[i].node != primary->node) || (CFG.aka[i].point!= primary->point))) {
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, aka2str(CFG.aka[i]));
	}
    }

    rc = binkp_send_command(MM_ADR, "%s", p);
    free(p);
    tidy_faddr(primary);
    if (rc) {
	SM_ERROR;
    }

    SM_PROCEED(WaitAddr)

SM_STATE(WaitAddr)

    for (;;) {
	if ((rc = binkp_recv_command(bp.rxbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error receiving remote info");
	    SM_ERROR;
	}
	if (cmd) {
	    if (bp.rxbuf[0] == MM_ADR) {
		p = xstrcpy(bp.rxbuf + 1);
		tidy_falist(&remote);
		remote = NULL;
		tmp = &remote;
		for (q = strtok(p, " "); q; q = strtok(NULL, " ")) {
		    if ((fa = parsefnode(q))) {
			dupe = FALSE;
			for (tmpa = remote; tmpa; tmpa = tmpa->next) {
			    if ((tmpa->addr->zone == fa->zone) && (tmpa->addr->net == fa->net) &&
				(tmpa->addr->node == fa->node) && (tmpa->addr->point == fa->point) &&
				(strcmp(tmpa->addr->domain, fa->domain) == 0)) {
				dupe = TRUE;
				Syslog('b', "Binkp: double address %s", ascfnode(tmpa->addr, 0x1f));
				break;
			    }
			}
			if (!dupe) {
			    *tmp = (fa_list*)malloc(sizeof(fa_list));
			    (*tmp)->next = NULL;
			    (*tmp)->addr = fa;
			    tmp = &((*tmp)->next);
			}
		    } else {
			Syslog('!', "Binkp: unparsable remote address: \"%s\"", printable(q, 0));
			binkp_send_command(MM_ERR, "Unparsable address \"%s\"", printable(q, 0));
			SM_ERROR;
		    }
		}
		free(p);

		tmp = &remote;
		while (*tmp) {
		    if (nodelock((*tmp)->addr, mypid)) {
			Syslog('+', "address : %s is locked, removed from aka list",ascfnode((*tmp)->addr,0x1f));
			tmpa=*tmp;
			*tmp=(*tmp)->next;
			free(tmpa);
		    } else {
			/*
			 * With the loaded flag we prevent removing the noderecord
			 * when the remote presents us an address we don't know about.
			 */
			akas++;
			Syslog('+', "address : %s",ascfnode((*tmp)->addr,0x1f));
			if (!Loaded) {
			    if (noderecord((*tmp)->addr))
				Loaded = TRUE;
			}
			tmp = &((*tmp)->next);
		    }
		}

		if (akas == 0) {
		    binkp_send_command(MM_BSY, "All aka's busy");
		    Syslog('+', "Binkp: abort, all aka's are busy");
		    SM_ERROR;
		}

		history.aka.zone  = remote->addr->zone;
		history.aka.net   = remote->addr->net;
		history.aka.node  = remote->addr->node;
		history.aka.point = remote->addr->point;
		sprintf(history.aka.domain, "%s", remote->addr->domain);

		SM_PROCEED(SendPasswd)
	    } else if (bp.rxbuf[0] == MM_BSY) {
		Syslog('!', "Binkp: M_BSY \"%s\"", printable(bp.rxbuf +1, 0));
		SM_ERROR;
	    } else if (bp.rxbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
		SM_ERROR;
	    } else if (bp.rxbuf[0] == MM_NUL) {
		parse_m_nul(bp.rxbuf +1);
	    } else {
		binkp_send_command(MM_ERR, "Unexpected frame");
		SM_ERROR;
	    }
	} else {
	    binkp_send_command(MM_ERR, "Unexpected frame");
	    SM_ERROR;
	}
    }

SM_STATE(SendPasswd)

    if (Loaded && strlen(nodes.Spasswd)) {
	pwd = xstrcpy(nodes.Spasswd);
	SendPass = TRUE;
    } else {
	pwd = xstrcpy((char *)"-");
    }

    if (bp.MD_Challenge) {
	char    *tp = NULL;
	tp = MD_buildDigest(pwd, bp.MD_Challenge);
	if (!tp) {
	    Syslog('!', "Unable to build MD5 digest");
	    binkp_send_command(MM_ERR, "CRAM authentication failed, internal error");
	    SM_ERROR;
	}
	bp.CRAMflag = TRUE;
	rc = binkp_send_command(MM_PWD, "%s", tp);
	free(tp);
    } else {
	rc = binkp_send_command(MM_PWD, "%s", pwd);
    }

    free(pwd);
    if (rc) {
	SM_ERROR;
    }
    SM_PROCEED(AuthRemote)

SM_STATE(AuthRemote)

    rc = 0;
    for (tmpa = remote; tmpa; tmpa = tmpa->next) {
	if ((tmpa->addr->zone  == ra.zone) && (tmpa->addr->net   == ra.net) &&
	    (tmpa->addr->node  == ra.node) && (tmpa->addr->point == ra.point)) {
	    rc = 1;         
	}
    }

    if (rc) {
	SM_PROCEED(IfSecure)
    } else {
	Syslog('!', "Binkp: error, the wrong node is reached");
	binkp_send_command(MM_ERR, "No AKAs in common or all AKAs busy");
	SM_ERROR;
    }

SM_STATE(IfSecure)

    SM_PROCEED(WaitOk)

SM_STATE(WaitOk)

    for (;;) {
	if ((rc = binkp_recv_command(bp.rxbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error waiting for remote acknowledge");
	    SM_ERROR;
	}

	if (cmd) {
	    if (bp.rxbuf[0] == MM_OK) {
		Syslog('b', "Binkp: M_OK \"%s\"", printable(bp.rxbuf +1, 0));
		if (SendPass)
		    bp.Secure = TRUE;
		Syslog('+', "Binkp: %s%sprotected session", bp.CRAMflag ? "MD5 ":"", bp.Secure ? "":"un");
		SM_PROCEED(Opts)
	    
	    } else if (bp.rxbuf[0] == MM_BSY) {
		Syslog('!', "Binkp: M_BSY \"%s\"", printable(bp.rxbuf +1, 0));
		SM_ERROR;

	    } else if (bp.rxbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
		SM_ERROR;

	    } else if (bp.rxbuf[0] == MM_NUL) {
		parse_m_nul(bp.rxbuf +1);

	    } else {
		binkp_send_command(MM_ERR, "Unexpected frame");
		SM_ERROR;
	    }
	} else {
	    binkp_send_command(MM_ERR, "Unexpected frame");
	    SM_ERROR;
	}
    }

SM_STATE(Opts)

    /*
     *  Try to initiate the MB option if the remote is binkp/1.0
     */
//    if ((bp.MBflag == WeCan) && (bp.Major == 1) && (bp.Minor == 0)) {
//	bp.MBflag = WeWant;
//	Syslog('b', "MBflag WeCan => WeWant");
//	binkp_send_command(MM_NUL, "OPT MB");
//    }
    SM_SUCCESS;

SM_END
SM_RETURN



/************************************************************************************/
/*
 *   Answer Session Setup
 */

SM_DECL(ansbinkp, (char *)"ansbinkp")
SM_STATES
    ConnInit,
    WaitConn,
    WaitAddr,
    IsPasswd,
    WaitPwd,
    PwdAck,
    Opts
SM_NAMES
    (char *)"ConnInit",
    (char *)"WaitConn",
    (char *)"WaitAddr",
    (char *)"IsPasswd",
    (char *)"WaitPwd",
    (char *)"PwdAck",
    (char *)"Opts"
SM_EDECL
    char	    *p, *q, *pw;
    int		    i, rc, cmd, dupe, we_have_pwd = FALSE, akas = 0;
    unsigned long   bufl;
    fa_list	    **tmp, *tmpa;
    faddr	    *fa;

SM_START(ConnInit)

SM_STATE(ConnInit)

    SM_PROCEED(WaitConn)

SM_STATE(WaitConn)

    Loaded = FALSE;

    if (strncmp(SockR("SBBS:0;"), "100:2,1", 7) == 0) {
	Syslog('+', "Binkp: system is closed, sending M_BSY");
	binkp_send_command(MM_BSY, "This system is closed, try again later");
	SM_ERROR;
    }

    if (!CFG.NoMD5 && ((bp.MD_Challenge = MD_getChallenge(NULL, &peeraddr)) != NULL)) {
	/*
	 * Answering site MUST send CRAM message as very first M_NUL
	 */
	char s[MD5_DIGEST_LEN*2+15]; /* max. length of opt string */
	strcpy(s, "OPT ");
#ifdef HAVE_ZLIB_H
	if (bp.PLZflag == WeCan) {
	    strcpy(s + strlen(s), "PLZ ");
	    bp.PLZflag = WeWant;
	    Syslog('b', "PLZflag WeCan => WeWant");
	}
#endif
	MD_toString(s + strlen(s), bp.MD_Challenge[0], bp.MD_Challenge+1);
	bp.CRAMflag = TRUE;
	if ((rc = binkp_send_command(MM_NUL, "%s", s))) {
	    SM_ERROR;
	}
    }

    if ((rc = binkp_banner())) {
	SM_ERROR;
    }

    p = xstrcpy((char *)"");
    for (i = 0; i < 40; i++) {
	if ((CFG.aka[i].zone) && (CFG.akavalid[i])) {
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, aka2str(CFG.aka[i]));
	}
    }

    rc = binkp_send_command(MM_ADR, "%s", p);
    free(p);
    if (rc) {
	SM_ERROR;
    }

    SM_PROCEED(WaitAddr)

SM_STATE(WaitAddr)

    for (;;) {
	if ((rc = binkp_recv_command(bp.rxbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error waiting for remote info");
	    SM_ERROR;
	}

	if (cmd) {
	    if (bp.rxbuf[0] == MM_ADR) {
		p = xstrcpy(bp.rxbuf +1);
		tidy_falist(&remote);
		remote = NULL;
		tmp = &remote;

		for (q = strtok(p, " "); q; q = strtok(NULL, " ")) {
		    if ((fa = parsefnode(q))) {
			dupe = FALSE;
			for (tmpa = remote; tmpa; tmpa = tmpa->next) {
			    if ((tmpa->addr->zone == fa->zone) && (tmpa->addr->net == fa->net) &&
				(tmpa->addr->node == fa->node) && (tmpa->addr->point == fa->point) &&
				(strcmp(tmpa->addr->domain, fa->domain) == 0)) {
				dupe = TRUE;
				Syslog('b', "Binkp: double address %s", ascfnode(tmpa->addr, 0x1f));
				break;
			    }
			}
			if (!dupe) {
			    *tmp = (fa_list*)malloc(sizeof(fa_list));
			    (*tmp)->next = NULL;
			    (*tmp)->addr = fa;
			    tmp = &((*tmp)->next);
			}
		    } else {
			Syslog('!', "Binkp: unparsable remote address: \"%s\"", printable(q, 0));
			binkp_send_command(MM_ERR, "Unparsable address \"%s\"", printable(q, 0));
			SM_ERROR;
		    }
		}

		tmp = &remote;
		while (*tmp) {
		    if (nodelock((*tmp)->addr, mypid)) {
			Syslog('+', "address : %s is locked, removed from aka list",ascfnode((*tmp)->addr,0x1f));
			tmpa=*tmp;
			*tmp=(*tmp)->next;
			free(tmpa);
		    } else {
			/*
			 * With the loaded flag we prevent removing the noderecord
			 * when the remote presents us an address we don't know about.
			 */
			akas++;
			Syslog('+', "address : %s",ascfnode((*tmp)->addr,0x1f));
			if (!Loaded) {
			    if (noderecord((*tmp)->addr))
				Loaded = TRUE;
			}
			tmp = &((*tmp)->next);
		    }
		}

                if (akas == 0) {
		    binkp_send_command(MM_BSY, "All aka's busy");
		    Syslog('+', "Binkp: abort, all aka's are busy");
		    SM_ERROR;
		}

		for (tmpa = remote; tmpa; tmpa = tmpa->next) {
		    if (((nlent = getnlent(tmpa->addr))) && (nlent->pflag != NL_DUMMY)) {
		        Syslog('+', "Binkp: remote is a listed system");
		        UserCity(mypid, nlent->sysop, nlent->location);
		        break;
		    }
		}
	        if (nlent)
		    rdoptions(Loaded);

		//if (bp.MBflag == TheyWant) {
		//                  Syslog('b', "Binkp: remote supports MB");
	        //                  binkp_send_control(MM_NUL,"OPT MB");
	        //                  bp.MBflag = Active;
	        //}
	        history.aka.zone  = remote->addr->zone;
	        history.aka.net   = remote->addr->net;
	        history.aka.node  = remote->addr->node;
	        history.aka.point = remote->addr->point;
	        sprintf(history.aka.domain, "%s", remote->addr->domain);

	        SM_PROCEED(IsPasswd)
    
	    } else if (bp.rxbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
		SM_ERROR;

	    } else if (bp.rxbuf[0] == MM_NUL) {
		parse_m_nul(bp.rxbuf +1);

	    } else if (bp.rxbuf[0] <= MM_MAX) {
		binkp_send_command(MM_ERR, "Unexpected frame");
		SM_ERROR;

	    }
	} else {
	    binkp_send_command(MM_ERR, "Unexpected frame");
	    SM_ERROR;
	}
    }

SM_STATE(IsPasswd)

    if (Loaded && strlen(nodes.Spasswd)) {
	we_have_pwd = TRUE;
    }
    
    Syslog('b', "We %s have a password", we_have_pwd ?"do":"don't");
    SM_PROCEED(WaitPwd)

SM_STATE(WaitPwd)

    for (;;) {
	if ((rc = binkp_recv_command(bp.rxbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error waiting for password");
	    SM_ERROR;
	}

	if (cmd) {
	    if (bp.rxbuf[0] == MM_PWD) {
		SM_PROCEED(PwdAck)
	    } else if (bp.rxbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
		SM_ERROR;
	    } else if (bp.rxbuf[0] == MM_NUL) {
		parse_m_nul(bp.rxbuf +1);
	    } else if (bp.rxbuf[0] <= MM_MAX) {
		binkp_send_command(MM_ERR, "Unexpected frame");
		SM_ERROR;
	    }
	} else {
	    binkp_send_command(MM_ERR, "Unexpected frame");
	    SM_ERROR;
	}
    }

SM_STATE(PwdAck)

    if (we_have_pwd) {
	pw = xstrcpy(nodes.Spasswd);
    } else {
	pw = xstrcpy((char *)"-");
    }

    if ((strncmp(bp.rxbuf +1, "CRAM-", 5) == 0) && bp.CRAMflag) {
	char    *sp;
	sp = MD_buildDigest(pw, bp.MD_Challenge);
	if (sp != NULL) {
	    if (strcmp(bp.rxbuf +1, sp)) {
		Syslog('+', "Binkp: bad MD5 crypted password");
		binkp_send_command(MM_ERR, "Bad password");
		free(sp);
		sp = NULL;
		free(pw);
		SM_ERROR;
	    } else {
		free(sp);
		sp = NULL;
		if (we_have_pwd)
		    bp.Secure = TRUE;
	    }
	} else {
	    free(pw);
	    Syslog('!', "Binkp: could not build MD5 digest");
	    binkp_send_command(MM_ERR, "*** Internal error ***");
	    SM_ERROR;
	}
    } else if ((strcmp(bp.rxbuf +1, pw) == 0)) {
	if (we_have_pwd)
	    bp.Secure = TRUE;
    } else {
	free(pw);
	Syslog('?', "Binkp: password error: expected \"%s\", got \"%s\"", nodes.Spasswd, bp.rxbuf +1);
	binkp_send_command(MM_ERR, "Bad password");
	SM_ERROR;
    }
    free(pw);
    Syslog('+', "Binkp: %s%sprotected session", bp.CRAMflag ? "MD5 ":"", bp.Secure ? "":"un");
    inbound_open(remote->addr, bp.Secure);
    binkp_send_command(MM_OK, "%ssecure", bp.Secure ? "":"non-");
    SM_PROCEED(Opts)

SM_STATE(Opts)

    SM_SUCCESS;

SM_END
SM_RETURN



/************************************************************************************/
/*
 *   File Transfer State
 */

/*
 * We do not use the normal state machine because that produces a lot
 * of debug logging that will drive up the CPU usage.
 *         FIXME: Remove these messages!!
 */
int file_transfer(void)
{
    int		rc = 0;
    TrType	Trc = Ok;
    
    for (;;) {
	Syslog('B', "Binkp: FileTransfer state %s", ftstate[bp.FtState]);
	switch (bp.FtState) {
	    case InitTransfer:	binkp_settimer(BINKP_TIMEOUT);
				bp.RxState = RxWaitF;
				bp.TxState = TxGNF;
				bp.FtState = Switch;
				bp.messages = 0;
				break;

	    case Switch:	if ((bp.RxState == RxDone) && (bp.TxState == TxDone)) {
				    bp.FtState = DeinitTransfer;
				    break;
				}

				rc = binkp_poll_frame();
				if (rc == -1) {
				    Syslog('b', "Binkp: receiver error detected");
				    bp.rc = rc = MBERR_FTRANSFER;
				    bp.FtState = DeinitTransfer;
				    break;
				} else if (rc == 1) {
				    bp.FtState = Receive;
				    break;
				}

				/*
				 * Check if there is room in the output buffer
				 */
				if (WAITPUTGET(-1) & 2) {
				    bp.FtState = Transmit;
				    break;
				}

				if (binkp_expired()) {
				    Syslog('+', "Binkp: transfer timeout");
				    bp.rc = 1;
				    bp.FtState = DeinitTransfer;
				    break;
				}

				/*
				 * Nothing done, release
				 */
				Syslog('b', "Binkp: NOTHING DONE");
				msleep(1);
				break;

	    case Receive:	Trc = binkp_receiver();
				if (Trc == Ok) {
				    if (bp.local_EOB && bp.remote_EOB)
					bp.FtState = Transmit;
				    else
					bp.FtState = Switch;
				} else if (Trc == Failure) {
				    Syslog('+', "Binkp: receiver failure");
				    bp.rc = 1;
				    bp.FtState = DeinitTransfer;
				}
				break;

	    case Transmit:	Trc = binkp_transmitter();
				if (Trc == Ok) {
				    bp.FtState = Switch;
				} else if (Trc == Failure) {
				    Syslog('+', "Binkp: transmitter failure");
				    bp.rc = 1;
				    bp.FtState = DeinitTransfer;
				}
				break;

	    case DeinitTransfer:/*
				 * In case of a transfer error the filelist is not yet cleared
				 */
				binkp_clear_filelist();
				if (bp.rc)
				    return MBERR_FTRANSFER;
				else
				    return 0;
				break;
	}
    }
}



/************************************************************************************/
/*
 *  Receiver routine
 */

TrType binkp_receiver(void)
{
    int		    bcmd, rc = 0;
    struct statfs   sfs;
    long	    written;
    off_t	    rxbytes;

    Syslog('B', "Binkp: receiver state %s", rxstate[bp.RxState]);

    if (bp.RxState == RxWaitF) {

	if (! bp.GotFrame)
	    return Ok;

	bp.GotFrame = FALSE;
	bp.rxlen = 0;
	bp.header = 0;

        if (! bp.cmd) {
            Syslog('b', "Binkp: got %d bytes DATA frame in %s state, ignored", bp.blklen, rxstate[bp.RxState]);
	    bp.blklen = 0;
            return Ok;
        }
	bp.blklen = 0;

        bcmd = bp.rxbuf[0];
        if (bcmd == MM_ERR) {
            Syslog('+', "Binkp: got M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
            bp.RxState = RxDone;
            return Failure;
        } else if ((bcmd == MM_GET) || (bcmd == MM_GOT) || (bcmd == MM_SKIP)) {
            binkp_add_message(bp.rxbuf);
            return Ok;
        } else if (bcmd == MM_NUL) {
            parse_m_nul(bp.rxbuf +1);
            return Ok;
        } else if (bcmd == MM_EOB) {
	    if ((bp.Major == 1) && (bp.Minor != 0)) {
//                Syslog('B', "Binkp: 1.1 check local_EOB=%s remote_EOB=%s messages=%d",
//		    bp.local_EOB?"True":"False", bp.remote_EOB?"True":"False", bp.messages);
		if (bp.local_EOB && bp.remote_EOB) {
		    Syslog('b', "Binkp: receiver detects both sides in EOB state");
		    if ((bp.messages < 3) || binkp_pendingfiles()) {
			Syslog('b', "Binkp: receiver detected end of session or pendingfiles, stay in RxWaitF");
			return Ok;
		    } else {
			bp.batchnr++;
			bp.local_EOB = FALSE;
			bp.remote_EOB = FALSE;
			bp.messages = 0;
			bp.TxState = TxGNF;
			bp.RxState = RxWaitF;
			Syslog('+', "Binkp: receiver starts batch %d", bp.batchnr + 1);
			binkp_clear_filelist();
			return Ok;
		    }
		} else {
		    return Ok;
		}
	    } else {
		Syslog('b', "Binkp/1.0 mode and got M_EOB, goto RxEOB");
		bp.RxState = RxEOB;
		return Ok;
	    }
        } else if (bcmd == MM_FILE) {
            bp.RxState = RxAccF;
            return Continue;
        } else if (bcmd <= MM_MAX) {
            Syslog('+', "Binkp: ERROR got unexpected %s frame in %s state", bstate[bcmd], rxstate[bp.RxState]);
            bp.RxState = RxDone;
            return Failure;
        } else {
            Syslog('+', "Binkp: got unexpected unknown frame, ignore");
            return Ok;
        }
    } else if (bp.RxState == RxAccF) {
	if (strlen(bp.rxbuf) < 512) {
	    sprintf(bp.rname, "%s", strtok(bp.rxbuf+1, " \n\r"));
	    bp.rsize = atoi(strtok(NULL, " \n\r"));
	    bp.rtime = atoi(strtok(NULL, " \n\r"));
	    bp.roffs = atoi(strtok(NULL, " \n\r"));
	} else {
	    /*
	     * Corrupted command, in case this was serious, send the M_GOT back so it's
	     * deleted at the remote.
	     */
	    Syslog('+', "Binkp: got corrupted FILE frame, size %d bytes", strlen(bp.rxbuf));
	    bp.RxState = RxWaitF;
	    rc = binkp_send_command(MM_GOT, bp.rxbuf +1);
	    if (rc)
		return Failure;
	    else
		return Ok;
	}
	Syslog('+', "Binkp: receive file \"%s\" date %s size %ld offset %ld", bp.rname, date(bp.rtime), bp.rsize, bp.roffs);
	(void)binkp2unix(bp.rname);
	rxbytes = bp.rxbytes;
	bp.rxfp = openfile(binkp2unix(bp.rname), bp.rtime, bp.rsize, &rxbytes, binkp_resync);
	bp.rxbytes = rxbytes;
	bp.rxcompressed = 0;

	if (bp.DidSendGET) {
	    Syslog('b', "Binkp: DidSendGET is set");
	    /*
	     * The file was partly received, via the openfile the resync function
	     * has send a GET command to start this file with a offset. This means
	     * we will get a new FILE command to open this file with a offset.
	     */
	    bp.RxState = RxWaitF;
	    return Ok;
	}

	gettimeofday(&rxtvstart, &bp.tz);
	bp.rxpos = bp.roffs;

	if (!diskfree(CFG.freespace)) {
	    Syslog('+', "Binkp: low diskspace, sending BSY");
	    binkp_send_command(MM_BSY, "Low diskspace, try again later");
	    bp.RxState = RxDone;
	    bp.TxState = TxDone;
	    bp.rc = MBERR_FTRANSFER;
	    return Failure;
	}

	if (statfs(tempinbound, &sfs) == 0) {
	    if ((bp.rsize / (sfs.f_bsize + 1)) >= sfs.f_bfree) {
		Syslog('!', "Binkp: only %lu blocks free (need %lu) in %s for this file", sfs.f_bfree, 
			    (unsigned long)(bp.rsize / (sfs.f_bsize + 1)), tempinbound);
		closefile();
		bp.rxfp = NULL; /* Force SKIP command       */
	    }
	}

	if (bp.rsize == bp.rxbytes) {
	    /*
	     * We already got this file, send GOT so it will
	     * be deleted at the remote.
	     */
	    Syslog('+', "Binkp: already got %s, sending GOT", bp.rname);
	    rc = binkp_send_command(MM_GOT, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
	    bp.RxState = RxWaitF;
	    bp.rxfp = NULL;
	    if (rc)
		return Failure;
	    else
		return Ok;
	} else if (!bp.rxfp) {
	    /*
	     * Some error, request to skip it
	     */
	    Syslog('+', "Binkp: error file %s, sending SKIP", bp.rname);
	    rc = binkp_send_command(MM_SKIP, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
	    bp.RxState = RxWaitF;
	    if (rc)
		return Failure;
	    else
		return Ok;
	} else {
	    Syslog('b', "rsize=%d, rxbytes=%d, roffs=%d", bp.rsize, bp.rxbytes, bp.roffs);
	    bp.RxState = RxReceD;
	    return Ok;
	}

    } else if (bp.RxState == RxReceD) {

	if (! bp.GotFrame)
	    return Ok;

	if (! bp.cmd) {
	    bp.RxState = RxWriteD;
	    return Continue;
	}

	bp.GotFrame = FALSE;
	bp.rxlen = 0;
	bp.header = 0;
	bp.blklen = 0;

	bcmd = bp.rxbuf[0];
        if (bcmd == MM_ERR) {
	    Syslog('+', "Binkp: got M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
	    bp.RxState = RxDone;
	    return Failure;
	} else if ((bcmd == MM_GET) || (bcmd == MM_GOT) || (bcmd == MM_SKIP)) {
	    binkp_add_message(bp.rxbuf);
	    return Ok;
	} else if (bcmd == MM_NUL) {
	    parse_m_nul(bp.rxbuf +1);
	    return Ok;
	} else if (bcmd == MM_FILE) {
	    Syslog('+', "Binkp: partial received file, saving");
	    closefile();
	    bp.rxfp = NULL;
	    bp.RxState = RxAccF;
	    return Continue;
	} else if (bcmd <= MM_MAX) {
	    Syslog('+', "Binkp: ERROR got unexpected %s frame in %s state", bstate[bcmd], rxstate[bp.RxState]);
	    bp.RxState = RxDone;
	    return Failure;
	} else {
	    Syslog('+', "Binkp: got unexpected unknown frame, ignore");
	    return Ok;
	}
    } else if (bp.RxState == RxWriteD) {

	if (bp.rxfp == NULL) {
	    /*
	     * After sending M_GET for restart at offset, the file is closed
	     * but some data frames may be underway, so ignore these.
	     */
	    Syslog('b', "Binkp: file is closed, ignore data");
	    bp.GotFrame = FALSE;
	    bp.rxlen = 0;
	    bp.header = 0;
	    bp.RxState = RxReceD;
	    return Ok;
	}

	written = fwrite(bp.rxbuf, 1, bp.blklen, bp.rxfp);

	bp.GotFrame = FALSE;
	bp.rxlen = 0;
	bp.header = 0;

	if (bp.blklen && (bp.blklen != written)) {
	    Syslog('+', "Binkp: ERROR file write error");
	    bp.RxState = RxDone;
	    bp.blklen = 0;
	    return Failure;
	}
	bp.blklen = 0;
	bp.rxpos += written;

	if (bp.rxpos == bp.rsize) {
	    rc = binkp_send_command(MM_GOT, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
	    closefile();
	    bp.rxpos = bp.rxpos - bp.rxbytes;
	    gettimeofday(&rxtvend, &bp.tz);
#ifdef HAVE_ZLIB_H
	    if (bp.rxcompressed && (bp.PLZflag == Active))
		Syslog('+', "Binkp: %s", compress_stat(bp.rxpos, bp.rxcompressed));
#endif
	    Syslog('+', "Binkp: OK %s", transfertime(rxtvstart, rxtvend, bp.rxpos, FALSE));
	    rcvdbytes += bp.rxpos;
	    bp.RxState = RxWaitF;
	    if (rc)
		return Failure;
	    else
		return Ok;
	}
	bp.RxState = RxReceD;
	return Ok;
    } else if (bp.RxState == RxEOB) {

	if (! bp.GotFrame)
	    return Ok;

	bp.GotFrame = FALSE;
	bp.rxlen = 0;
	bp.header = 0;
	bp.blklen = 0;

	if (bp.cmd) {
	    bcmd = bp.rxbuf[0];
	    if (bcmd == MM_ERR) {
		Syslog('+', "Binkp: got M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
		bp.RxState = RxDone;
		return Failure;
	    } else if ((bcmd == MM_GET) || (bcmd == MM_GOT) || (bcmd == MM_SKIP)) {
		binkp_add_message(bp.rxbuf);
		return Ok;
	    } else if (bcmd == MM_NUL) {
		parse_m_nul(bp.rxbuf +1);
		return Ok;
	    } else if (bcmd <= MM_MAX) {
		Syslog('+', "Binkp: ERROR got unexpected %s frame in %s state", bstate[bcmd], rxstate[bp.RxState]);
		bp.RxState = RxDone;
		return Failure;
	    } else {
		Syslog('+', "Binkp: got unexpected unknown frame, ignore");
		return Ok;
	    }
	} else {
	    Syslog('+', "Binkp: ERROR got unexpected data frame in %s state", rxstate[bp.RxState]);
	    bp.RxState = RxDone;
	    return Failure;
	}

    } else if (bp.RxState == RxDone) {
	return Ok;
    }

    /*
     * Cannot be here
     */
    bp.RxState = RxDone;
    return Failure;
}


/************************************************************************************/
/*
 *  Transmitter routine
 */

TrType binkp_transmitter(void)
{
    int		rc = 0, eof = FALSE;
    char	*nonhold_mail;
    fa_list	*eff_remote;
    file_list	*tsl;
    static binkp_list	*tmp;

    Syslog('B', "Binkp: transmitter state %s", txstate[bp.TxState]);

    if (bp.TxState == TxGNF) {
	/*
	 * If we do not have a filelist yet, create one.
	 */
	if (tosend == NULL) {
	    Syslog('b', "Creating filelist");
	    nonhold_mail = (char *)ALL_MAIL;
	    bp.nethold = bp.mailhold = 0L;
	    cursend = NULL;

	    /*
	     * If remote doesn't have the 8.3 flag set, allow long names
	     */
	    if (!nodes.FNC)
		remote_flags &= ~SESSION_FNC;

	    eff_remote = remote;
	    tosend = create_filelist(eff_remote, nonhold_mail, 0);
	    if ((respond = respond_wazoo()) != NULL) {
		for (tsl = tosend; tsl->next; tsl = tsl ->next);
		tsl->next = respond;
		Syslog('b', "Binkp: added requested files");
	    }

	    /*
	     *  Build a new filelist from the existing filelist.
	     *  This one is special for binkp behaviour.
	     */
	    for (tsl = tosend; tsl; tsl = tsl->next) {
		if (tsl->remote != NULL)
		    fill_binkp_list(&bll, tsl, 0L);
	    }
	    debug_binkp_list(&bll);

	    if ((bp.nethold || bp.mailhold) || (bp.batchnr == 0)) {
		Syslog('+', "Binkp: mail %ld, files %ld bytes", bp.nethold, bp.mailhold);
		if ((rc = binkp_send_command(MM_NUL, "TRF %ld %ld", bp.nethold, bp.mailhold))) {
		    bp.TxState = TxDone;
		    return Failure;
		}
	    }
	}
	
	for (tmp = bll; tmp; tmp = tmp->next) {
	    if (tmp->state == NoState) {
		/*
		 * There is something to send
		 */
		struct flock        txflock;
		txflock.l_type   = F_RDLCK;
		txflock.l_whence = 0;
		txflock.l_start  = 0L;
		txflock.l_len    = 0L;

		bp.txfp = fopen(tmp->local, "r");
		if (bp.txfp == NULL) {
		    if ((errno == ENOENT) || (errno == EINVAL)) {
			Syslog('+', "Binkp: file %s doesn't exist, removing", MBSE_SS(tmp->local));
			tmp->state = Got;
		    } else {
			WriteError("$Binkp: can't open %s, skipping", MBSE_SS(tmp->local));
			tmp->state = Skipped;
		    }
		    break;
		}

		if (fcntl(fileno(bp.txfp), F_SETLK, &txflock) != 0) {
		    WriteError("$Binkp: can't lock file %s, skipping", MBSE_SS(tmp->local));
		    fclose(bp.txfp);
		    bp.txfp = NULL;
		    tmp->state = Skipped;
		    break;
		}

		bp.txpos = bp.stxpos = tmp->offset;
		bp.txcompressed = 0;
		Syslog('+', "Binkp: send \"%s\" as \"%s\"", MBSE_SS(tmp->local), MBSE_SS(tmp->remote));
		Syslog('+', "Binkp: size %lu bytes, dated %s", (unsigned long)tmp->size, date(tmp->date));
		rc = binkp_send_command(MM_FILE, "%s %lu %ld %ld", MBSE_SS(tmp->remote), 
			(unsigned long)tmp->size, (long)tmp->date, (unsigned long)tmp->offset);
		if (rc) {
		    bp.TxState = TxDone;
		    return Failure;
		}
		gettimeofday(&txtvstart, &bp.tz);
		tmp->state = Sending;
		cursend = tmp;
		bp.TxState = TxTryR;
		return Continue;
	    } /* if state == NoState */
	} /* for */

	if (tmp == NULL) {
	    /*
	     * No more files
	     */
	    rc = binkp_send_command(MM_EOB, "");
	    bp.TxState = TxWLA;
	    if (rc)
		return Failure;
	    else
		return Continue;
	}
	return Ok;

    } else if (bp.TxState == TxTryR) {

	if (bp.msgs_on_queue == 0) {
	    bp.TxState = TxReadS;
	    return Continue;
	} else {
	    if (binkp_process_messages()) {
		bp.TxState = TxDone;
		return Failure;
	    } else 
		return Continue;
	}

    } else if (bp.TxState == TxReadS) {
	fseek(bp.txfp, bp.txpos, SEEK_SET);
	bp.txlen = fread(bp.txbuf, 1, bp.cmpblksize, bp.txfp);
	eof = feof(bp.txfp);

	if ((bp.txlen == 0) || eof) {
	    /*
	     * Nothing read/error or we are at eof
	     */
	    if (ferror(bp.txfp)) {
		WriteError("$Binkp: error reading from file");
		bp.TxState = TxDone;
		cursend->state = Skipped;
		debug_binkp_list(&bll);
		return Failure;
	    }

	    /*
	     * We are at eof, send last datablock
	     */
	    if (eof && bp.txlen) {
		bp.txpos += bp.txlen;
		sentbytes += bp.txlen;
		rc = binkp_send_frame(FALSE, bp.txbuf, bp.txlen);
		if (rc)
		    return Failure;
	    }

	    /*
	     * calculate time needed and bytes transferred
	     */
	    gettimeofday(&txtvend, &bp.tz);

	    /*
	     * Close transmitter file
	     */
	    fclose(bp.txfp);
	    bp.txfp = NULL;

	    if (bp.txpos >= 0) {
		bp.stxpos = bp.txpos - bp.stxpos;
#ifdef HAVE_ZLIB_H
		if (bp.txcompressed && (bp.PLZflag == Active))
		    Syslog('+', "Binkp: %s", compress_stat(bp.stxpos, bp.txcompressed));
#endif
		Syslog('+', "Binkp: OK %s", transfertime(txtvstart, txtvend, bp.stxpos, TRUE));
	    } else {
		Syslog('+', "Binkp: transmitter skipped file after %ld seconds", txtvend.tv_sec - txtvstart.tv_sec);
	    }

	    cursend->state = IsSent;
	    cursend = NULL;
	    bp.TxState = TxGNF;
	    return Ok;
	} else {
	    /*
	     * regular datablock
	     */
	    bp.txpos += bp.txlen;
	    sentbytes += bp.txlen;
	    rc = binkp_send_frame(FALSE, bp.txbuf, bp.txlen);
	    bp.TxState = TxTryR;
	    if (rc)
		return Failure;
	    else
		return Ok;
	}

    } else if (bp.TxState == TxWLA) {

	if ((bp.msgs_on_queue == 0) && (binkp_pendingfiles() == 0)) {

	    if ((bp.RxState >= RxEOB) && (bp.Major == 1) && (bp.Minor == 0)) {
		bp.TxState = TxDone;
		if (bp.local_EOB && bp.remote_EOB) {
		    Syslog('b', "Binkp: binkp/1.0 session seems complete");
		    bp.RxState = RxDone;
		}

		binkp_clear_filelist();
		return Ok;
	    }

	    if ((bp.RxState < RxEOB) && (bp.Major == 1) && (bp.Minor == 0)) {
		bp.TxState = TxWLA;
		return Ok;
	    }

	    if ((bp.Major == 1) && (bp.Minor != 0)) {
//		Syslog('B', "Binkp: 1.1 check local_EOB=%s remote_EOB=%s messages=%d",
//			bp.local_EOB?"True":"False", bp.remote_EOB?"True":"False", bp.messages);

		if (bp.local_EOB && bp.remote_EOB) {
		    /*
		     * We did send EOB and got a EOB
		     */
		    if (bp.messages < 3) {
			/*
			 * Nothing sent anymore, finish
			 */
			Syslog('b', "Binkp: binkp/1.1 session seems complete");
			bp.RxState = RxDone;
			bp.TxState = TxDone;
		    } else {
			/*
			 * Start new batch
			 */
			bp.batchnr++;
			bp.local_EOB = FALSE;
			bp.remote_EOB = FALSE;
			bp.messages = 0;
			bp.TxState = TxGNF;
			bp.RxState = RxWaitF;
			Syslog('+', "Binkp: transmitter starts batch %d", bp.batchnr + 1);
			binkp_clear_filelist();
			return Ok; /* Continue is not good here, troubles with binkd on slow links. */
		    }
		}

		binkp_clear_filelist();
		return Ok;
	    }
	}

	if (bp.msgs_on_queue) {
	    if (binkp_process_messages()) {
		return Failure;
	    } else {
		return Continue;
	    }
	}
	return Ok;

    } else if (bp.TxState == TxDone) {
	return Ok;
    }

    /*
     * Cannot be here
     */
    bp.TxState = TxDone;
    return Failure;
}


/************************************************************************************/
/*
 *   Functions
 */


/*
 * Send a binkp frame
 */
int binkp_send_frame(int cmd, char *buf, int len)
{
    unsigned short  header = 0;
    int		    rc, id;
#ifdef HAVE_ZLIB_H
    int		    rcz, last;
    unsigned long   zlen;
    char	    *zbuf;
#endif

#ifdef HAVE_ZLIB_H
    if ((len >= BINKP_PLZ_BLOCK) && (bp.PLZflag == Active)) {
	WriteError("Can't send block of %d bytes in PLZ mode", len);
	return 1;
    }
#endif

    if (cmd) {
	header = ((BINKP_CONTROL_BLOCK + len) & 0xffff);
	bp.messages++;
	id = buf[0];
	if (id == MM_EOB) {
	    bp.local_EOB = TRUE;
	}
	if (len == 1)
	    Syslog('b', "Binkp: send %s", bstate[id]);
	else
	    Syslog('b', "Binkp: send %s %s", bstate[id], printable(buf +1, len -1));
    } else {
	header = ((BINKP_DATA_BLOCK + len) & 0xffff);
	Syslog('b', "Binkp: send data (%d)", len);
    }

#ifdef HAVE_ZLIB_H
    last = bp.cmpblksize;
    /*
     * Only use compression for DATA blocks larger then 20 bytes.
     */
    if ((bp.PLZflag == Active) && (len > 20) && (!cmd)) {
	zbuf = calloc(BINKP_ZIPBUFLEN, sizeof(char));
	zlen = BINKP_PLZ_BLOCK -1;
	rcz = compress2(zbuf, &zlen, buf, len, 9);
	if (rcz == Z_OK) {
	    Syslog('b', "Binkp: compressed OK, srclen=%d, destlen=%d, will send compressed=%s", 
		    len, zlen, (zlen < len) ?"yes":"no");
	    if (zlen < len) {
		bp.txcompressed += (len - zlen);
		/*
		 * Calculate the perfect blocksize for the next block
		 * using the current compression ratio. This gives
		 * a dynamic optimal blocksize. The average maximum
		 * blocksize on the line will be 4096 bytes.
		 */
		bp.cmpblksize = ((len * 4) / zlen) * 512;
		if (bp.cmpblksize < SND_BLKSIZE)
		    bp.cmpblksize = SND_BLKSIZE;
		if (bp.cmpblksize > (BINKP_PLZ_BLOCK -1))
		    bp.cmpblksize = (BINKP_PLZ_BLOCK -1);

		/*
		 * Rebuild header for compressed block
		 */
		header = ((BINKP_DATA_BLOCK + BINKP_PLZ_BLOCK + zlen) & 0xffff);
		rc = PUTCHAR((header >> 8) & 0x00ff);
		if (!rc)
		    rc = PUTCHAR(header & 0x00ff);
		if (zlen && !rc)
		    rc = PUT(zbuf, zlen);
	    } else {
		rc = PUTCHAR((header >> 8) & 0x00ff);
		if (!rc)
		    rc = PUTCHAR(header & 0x00ff);
		if (len && !rc)
		    rc = PUT(buf, len);
		if (!cmd)
		    bp.cmpblksize = SND_BLKSIZE;
	    }
	} else {
	    Syslog('+', "Binkp: compress error %d", rcz);
	    rc = PUTCHAR((header >> 8) & 0x00ff);
	    if (!rc)
		rc = PUTCHAR(header & 0x00ff);
	    if (len && !rc)
		rc = PUT(buf, len);
	    if (!cmd)
		bp.cmpblksize = SND_BLKSIZE;
	}
	free(zbuf);
    } else {
	rc = PUTCHAR((header >> 8) & 0x00ff);
	if (!rc)
	    rc = PUTCHAR(header & 0x00ff);
	if (len && !rc)
	    rc = PUT(buf, len);
	if (!cmd)
	    bp.cmpblksize = SND_BLKSIZE;
    }
    if (!cmd && (last != bp.cmpblksize))
	Syslog('b', "Binkp: adjusting next blocksize to %d bytes", bp.cmpblksize);
#else
    rc = PUTCHAR((header >> 8) & 0x00ff);
    if (!rc)
	rc = PUTCHAR(header & 0x00ff);
    if (len && !rc)
	rc = PUT(buf, len);
    bp.cmpblksize = SND_BLKSIZE;
#endif

    FLUSHOUT();
    binkp_settimer(BINKP_TIMEOUT);
    Nopper();
    return rc;
}



/*
 * Send a command message
 */
int binkp_send_command(int id, ...)
{
    va_list     args;
    char        *fmt;
    static char buf[1024];
    int         sz, rc;

    va_start(args, id);
    fmt = va_arg(args, char*);

    if (fmt) {
	vsprintf(buf, fmt, args);
	sz = (strlen(buf) & 0x7fff);
    } else {
	buf[0]='\0';
	sz = 0;
    }

    memmove(buf+1, buf, sz);
    buf[0] = id & 0xff;
    sz++;

    rc = binkp_send_frame(TRUE, buf, sz);
    va_end(args);
    return rc;
}



/*
 * Set binkp master timer
 */
void binkp_settimer(int interval)
{
    Timer = time((time_t*)NULL) + interval;
}



/*
 * Test if master timer is expired
 */
int binkp_expired(void)
{
    time_t      now;

    now = time(NULL);
    if (now >= Timer)
	Syslog('+', "Binkp: session timeout");
    return (now >= Timer);
}



/*
 * Send system info to remote
 */
int binkp_banner(void)
{
    time_t  t;
    int	    rc;

    rc = binkp_send_command(MM_NUL,"SYS %s", CFG.bbs_name);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"ZYZ %s", CFG.sysop_name);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"LOC %s", CFG.location);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"NDL %s", CFG.Flags);
    t = time(NULL);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"TIME %s", rfcdate(t));
    if (!rc) {
	if (nodes.NoBinkp11)
	    rc = binkp_send_command(MM_NUL,"VER mbcico/%s/%s-%s %s/%s", VERSION, OsName(), OsCPU(), PRTCLNAME, PRTCLOLD);
	else
	    rc = binkp_send_command(MM_NUL,"VER mbcico/%s/%s-%s %s/%s", VERSION, OsName(), OsCPU(), PRTCLNAME, PRTCLVER);
    }
    if (strlen(CFG.Phone) && !rc)
	rc = binkp_send_command(MM_NUL,"PHN %s", CFG.Phone);
    if (strlen(CFG.comment) && !rc)
	rc = binkp_send_command(MM_NUL,"OPM %s", CFG.comment);

    return rc;
}



/*
 *  Receive command frame
 */
int binkp_recv_command(char *buf, unsigned long *len, int *cmd)
{
    int	    b0, b1;

    *len = *cmd = 0;

    b0 = GETCHAR(BINKP_TIMEOUT);
    if (tty_status) {
	Syslog('-', "Binkp: tty_status with b0");
	goto to;
    }
    if (b0 & 0x80)
	*cmd = 1;

    b1 = GETCHAR(BINKP_TIMEOUT / 2);
    if (tty_status) {
	Syslog('-', "Binkp: tty_status with b1");
	goto to;
    }

    *len = (b0 & 0x7f) << 8;
    *len += b1;

    GET(buf, *len, BINKP_TIMEOUT / 2);
    buf[*len] = '\0';
    if (tty_status) {
	Syslog('-', "Binkp: tty_status with block len=%d", *len);
	goto to;
    }
    binkp_settimer(BINKP_TIMEOUT);
    Nopper();

to:
    if (tty_status)
	WriteError("Binkp: TCP rcv error, tty status: %s", ttystat[tty_status]);
    return tty_status;
}



/*
 * Parse a received M_NUL message
 */
void parse_m_nul(char *msg)
{
    char    *p, *q;

    if (strncmp(msg, "SYS ", 4) == 0) {
	Syslog('+', "System  : %s", msg+4);
	strncpy(history.system_name, msg+4, 35);
    
    } else if (strncmp(msg, "ZYZ ", 4) == 0) {
	Syslog('+', "Sysop   : %s", msg+4);
	strncpy(history.sysop, msg+4, 35);
    
    } else if (strncmp(msg, "LOC ", 4) == 0) {
	Syslog('+', "Location: %s", msg+4);
	strncpy(history.location, msg+4, 35);
    
    } else if (strncmp(msg, "NDL ", 4) == 0) {
	Syslog('+', "Flags   : %s", msg+4);
    
    } else if (strncmp(msg, "TIME ", 5) == 0) {
	Syslog('+', "Time    : %s", msg+5);
    
    } else if (strncmp(msg, "VER ", 4) == 0) {
	Syslog('+', "Uses    : %s", msg+4);
	if ((p = strstr(msg+4, PRTCLNAME "/")) && (q = strstr(p, "."))) {
	    bp.Major = atoi(p + 6);
	    bp.Minor = atoi(q + 1);
	}
    } else if (strncmp(msg, "PHN ", 4) == 0) {
	Syslog('+', "Phone   : %s", msg+4);
    
    } else if (strncmp(msg, "OPM ", 4) == 0) {
	Syslog('+', "Remark  : %s", msg+4);
    
    } else if (strncmp(msg, "TRF ", 4) == 0) {
	Syslog('+', "Binkp: remote has %s mail/files for us", msg+4);
    
    } else if (strncmp(msg, "OPT ", 4) == 0) {
	Syslog('+', "Options : %s", msg+4);
	p = msg;
	q = strtok(p, " \n\r\0");
	while ((q = strtok(NULL, " \r\n\0"))) {
	    Syslog('b', "Binkp: parsing opt \"%s\"", printable(q, 0));
	    if (strncmp(q, (char *)"CRAM-MD5-", 9) == 0) { /* No SHA-1 support */
		if (CFG.NoMD5) {
		    Syslog('+', "Binkp: Remote supports MD5, but it's turned off here");
		} else {
		    if (bp.MD_Challenge)
			free(bp.MD_Challenge);
		    bp.MD_Challenge = MD_getChallenge(q, NULL);
		}
#ifdef HAVE_ZLIB_H
	    } else if (strncmp(q, (char *)"PLZ", 3) == 0) {
		Syslog('b', "Binkp: got PLZ, current state %s", opstate[bp.PLZflag]);
		if (bp.PLZflag == WeCan) {
		    bp.PLZflag = TheyWant;
		    Syslog('b', "PLZflag WeCan => TheyWant");
		    binkp_send_command(MM_NUL,"OPT PLZ");
		    Syslog('b', "PLZflag TheyWant => Active");
		    bp.PLZflag = Active;
		    Syslog('+', "        : zlib compression active");
		} else if (bp.PLZflag == WeWant) {
		    bp.PLZflag = Active;
		    Syslog('b', "PLZflag WeWant => Active");
		    Syslog('+', "        : zlib compression active");
		} else {
		    Syslog('b', "PLZflag is %s and received PLZ option", opstate[bp.PLZflag]);
		}
#endif
	    } else {
		Syslog('b', "Binkp: opt not supported");
	    }
	}

    } else {
	Syslog('+', "Binkp: M_NUL \"%s\"", msg);
    }
}



/*
 * Poll for a frame, returns:
 *  -1 = Error
 *   0 = Nothing yet
 *   1 = Got a frame
 *   2 = Frame not processed
 *   3 = Uncompress error
 */
int binkp_poll_frame(void)
{
    int		    c, rc = 0, bcmd;
#ifdef HAVE_ZLIB_H
    unsigned long   zlen;
    char	    *zbuf;
#endif

    for (;;) {
	if (bp.GotFrame) {
	    Syslog('b', "Binkp: WARNING: frame not processed");
	    rc = 2;
	    break;
	} else {
	    c = GETCHAR(0);
	    if (c < 0) {
		c = -c;
		if (c == STAT_TIMEOUT) {
		    msleep(1);
		    rc = 0;
		    break;
		}
		Syslog('?', "Binkp: receiver status %s", ttystat[c]);
		bp.rc = (MBERR_TTYIO + (-c));
		rc = -1;
		break;
	    } else {
		switch (bp.rxlen) {
		    case 0: bp.header = c << 8;
			    rc = 0;
			    break;
		    case 1: bp.header += c;
			    rc = 0;
			    break;
		    default:bp.rxbuf[bp.rxlen-2] = c;
		}
		if (bp.rxlen == 1) {
		    bp.cmd = bp.header & BINKP_CONTROL_BLOCK;
#ifdef HAVE_ZLIB_H
		    if (bp.PLZflag == Active) {
			bp.blklen = bp.header & 0x3fff;
		    } else {
			bp.blklen = bp.header & 0x7fff;
		    }
#else
		    bp.blklen = bp.header & 0x7fff;
#endif
		}
		if ((bp.rxlen == (bp.blklen + 1) && (bp.rxlen >= 1))) {
		    bp.GotFrame = TRUE;
#ifdef HAVE_ZLIB_H
		    if ((bp.PLZflag == Active) && (bp.header & BINKP_PLZ_BLOCK)) {
			Syslog('b', "Binkp: got a compressed block %d bytes", bp.blklen);
			zbuf = calloc(BINKP_ZIPBUFLEN, sizeof(char));
			zlen = BINKP_PLZ_BLOCK -1;
			rc = uncompress(zbuf, &zlen, bp.rxbuf, bp.rxlen -1);
			if (rc == Z_OK) {
			    bp.rxcompressed += (zlen - (bp.rxlen -1));
			    memmove(bp.rxbuf, zbuf, zlen);
			    bp.rxlen = zlen +1;
			    bp.blklen = zlen;
			} else {
			    free(zbuf);
			    Syslog('!', "Binkp: uncompress error %d", rc);
			    return 3;
			}
			free(zbuf);
		    }
#endif
		    bp.rxbuf[bp.rxlen-1] = '\0';
		    if (bp.cmd) {
			bp.messages++;
			bcmd = bp.rxbuf[0];
			Syslog('b', "Binkp: rcvd %s %s", bstate[bcmd], printable(bp.rxbuf+1, 0));
			if (bcmd == MM_EOB) {
			    bp.remote_EOB = TRUE;
			}
		    } else {
			Syslog('b', "Binkp: rcvd data (%d)", bp.rxlen -1);
		    }
		    binkp_settimer(BINKP_TIMEOUT);
		    Nopper();
		    rc = 1;
		    break;
		}
		bp.rxlen++;
	    }
	}
    }

    return rc;
}



/*
 * Add received command frame to the queue, will be processed by the transmitter.
 */
void binkp_add_message(char *frame)
{
    the_queue	**tmpl;
    int		bcmd;
    
    bcmd = frame[0];
    Syslog('b', "Binkp: add message %s %s", bstate[bcmd], printable(frame +1, 0));

    for (tmpl = &tql; *tmpl; tmpl = &((*tmpl)->next));
    *tmpl = (the_queue *)malloc(sizeof(the_queue));

    (*tmpl)->next = NULL;
    (*tmpl)->cmd  = frame[0];
    (*tmpl)->data = xstrcpy(frame +1);

    bp.msgs_on_queue++;
}



/*
 * Process all messages on the queue, the oldest are on top, after
 * processing release memory and reset the messages queue.
 */
int binkp_process_messages(void)
{
    the_queue	*tmpq, *oldq;
    binkp_list	*tmp;
    file_list	*tsl;
    int		Found;
    char	*lname;
    time_t	ltime;
    long	lsize, loffs;

    Syslog('b', "Binkp: Process The Messages Queue Start");

    lname = calloc(512, sizeof(char));

    for (tmpq = tql; tmpq; tmpq = tmpq->next) {
	Syslog('+', "Binkp: %s \"%s\"", bstate[tmpq->cmd], printable(tmpq->data, 0));
	if (tmpq->cmd == MM_GET) {
	    sprintf(lname, "%s", strtok(tmpq->data, " \n\r"));
	    lsize = atoi(strtok(NULL, " \n\r"));
	    ltime = atoi(strtok(NULL, " \n\r"));
	    loffs = atoi(strtok(NULL, " \n\r"));
	    Found = FALSE;
	    for (tmp = bll; tmp; tmp = tmp->next) {
		if ((strcmp(lname, tmp->remote) == 0) && (lsize == tmp->size) && (ltime == tmp->date)) {
		    if (lsize == loffs) {
			/*
			 * Requested offset is filesize, close file.
			 */
			if (cursend && (strcmp(lname, cursend->remote) == 0) && 
				(lsize == cursend->size) && (ltime == cursend->date)) {
			    Syslog('b', "Got M_GET with offset == filesize for current file, close");

			    /*
			     * calculate time needed and bytes transferred
			     */
			    gettimeofday(&txtvend, &bp.tz);

			    /*
			     * Close transmitter file
			     */
			    fclose(bp.txfp);
			    bp.txfp = NULL;

			    if (bp.txpos >= 0) {
				bp.stxpos = bp.txpos - bp.stxpos;
				Syslog('+', "Binkp: OK %s", transfertime(txtvstart, txtvend, bp.stxpos, TRUE));
			    } else {
				Syslog('+', "Binkp: transmitter skipped file after %ld seconds", 
					txtvend.tv_sec - txtvstart.tv_sec);
			    }

			    cursend->state = Got; // 12-03-2004 changed from IsSent, bug from LdG with FT.
			    cursend = NULL;
			    bp.TxState = TxGNF;

			    for (tsl = tosend; tsl; tsl = tsl->next) { // Added 12-03
				if (tsl->remote == NULL) {
				    execute_disposition(tsl);
				} else {
				    if (strcmp(cursend->local, tsl->local) == 0) {
					execute_disposition(tsl);
				    }
				}
			    }
			} else {
			    Syslog('+', "Binkp: requested offset = filesize, but is not the current file");
			    if (tmp->state == IsSent) { // Added 12-03
				Syslog('b', "Binkp: file is sent, treat as m_got");
				tmp->state = Got;
				for (tsl = tosend; tsl; tsl = tsl->next) {
				    if (tsl->remote == NULL) {
					execute_disposition(tsl);
				    } else {
					if (strcmp(tmp->local, tsl->local) == 0) {
					    execute_disposition(tsl);
					}
				    }
				}
			    }
			}
		    } else if (loffs < lsize) {
			tmp->state = NoState;
			tmp->offset = loffs;
			if (loffs) {
			    Syslog('+', "Binkp: Remote wants %s for resync at offset %ld", tmp->remote, tmp->offset);
			} else {
			    Syslog('+', "Binkp: Remote wants %s again from start", tmp->remote);
			}
			bp.TxState = TxGNF;
		    } else {
			Syslog('+', "Binkp: requested offset > filesize, ignore");
		    }
		    Found = TRUE;
		    break;
		}
		if (!Found) {
		    Syslog('+', "Binkp: unexpected M_GET \"%s\"", tmpq->data); /* Ignore frame */
		}
	    }
	} else if (tmpq->cmd == MM_GOT) {
	    sprintf(lname, "%s", strtok(tmpq->data, " \n\r"));
	    lsize = atoi(strtok(NULL, " \n\r"));
	    ltime = atoi(strtok(NULL, " \n\r"));
	    Found = FALSE;
	    for (tmp = bll; tmp; tmp = tmp->next) {
		if ((strcmp(lname, tmp->remote) == 0) && (lsize == tmp->size) && (ltime == tmp->date)) {
		    Found = TRUE;
		    if (tmp->state == Sending) {
			Syslog('+', "Binkp: remote refused %s", tmp->remote);
			fclose(bp.txfp);
			bp.txfp = NULL;
			bp.TxState = TxGNF;
			cursend = NULL;
		    } else {
			Syslog('+', "Binkp: remote GOT \"%s\"", tmp->remote);
		    }
		    tmp->state = Got;
		    for (tsl = tosend; tsl; tsl = tsl->next) {
			if (tsl->remote == NULL) {
			    execute_disposition(tsl);
			} else {
			    if (strcmp(tmp->local, tsl->local) == 0) {
				execute_disposition(tsl);
			    }
			}
		    }
		    break;
		}
	    }
	    if (!Found) {
		Syslog('+', "Binkp: unexpected M_GOT \"%s\"", tmpq->data); /* Ignore frame */
	    }
	} else if (tmpq->cmd == MM_SKIP) {
	    sprintf(lname, "%s", strtok(tmpq->data, " \n\r"));
	    lsize = atoi(strtok(NULL, " \n\r"));
	    ltime = atoi(strtok(NULL, " \n\r"));
	    Found = FALSE;
	    for (tmp = bll; tmp; tmp = tmp->next) {
		if ((strcmp(lname, tmp->remote) == 0) && (lsize == tmp->size) && (ltime == tmp->date)) {
		    Found = TRUE;
		    if (tmp->state == Sending) {
			Syslog('+', "Binkp: remote skipped %s, may be accepted later", tmp->remote);
			fclose(bp.txfp);
			bp.txfp = NULL;
			cursend = NULL;
			bp.TxState = TxGNF;
		    } else {
			Syslog('+', "Binkp: remote refused %s", tmp->remote);
		    }
		    tmp->state = Skipped;
		    break;
		}
	    }
	    if (!Found) {
		Syslog('+', "Binkp: unexpected M_SKIP \"%s\"", tmpq->data); /* Ignore frame */
	    }
	} else {
	    /* Illegal message on the queue */
	}
    }

    /*
     * Tidy the messages queue and reset.
     */
    for (tmpq = tql; tmpq; tmpq = oldq) {
	oldq = tmpq->next;
	if (tmpq->data)
	    free(tmpq->data);
	free(tmpq);
    }
    tql = NULL;
    free(lname);
    bp.msgs_on_queue = 0;

    debug_binkp_list(&bll);
    Syslog('b', "Binkp: Process The Messages Queue End");
    return 0;
}



/*
 * Count number of pending files
 */
int binkp_pendingfiles(void)
{
    binkp_list  *tmpl;
    int         count = 0;

    for (tmpl = bll; tmpl; tmpl = tmpl->next) {
	if ((tmpl->state != Got) && (tmpl->state != Skipped))
	    count++;
    }

    if (count)
	Syslog('b', "Binkp: %d pending files on queue", count);
    return count;
}



/*
 * This function is called two times if a partial file exists from openfile.
 *  1. A partial file is detected, send a GET to the remote, set DidSendGET flag.
 *  2. DidSendGET is set, return 0 and let openfile open the file in append mode.
 */
int binkp_resync(off_t off)
{
    Syslog('b', "Binkp: resync(%d) DidSendGET=%s", off, bp.DidSendGET ?"TRUE":"FALSE");
    if (!bp.DidSendGET) {
	binkp_send_command(MM_GET, "%s %ld %ld %ld", bp.rname, bp.rsize, bp.rtime, off);
	bp.DidSendGET = TRUE;
	Syslog('+', "Binkp: already %lu bytes received, requested restart with offset", (unsigned long)off);
	return -1;  /* Signal openfile not to open the file */
    }
    bp.DidSendGET = FALSE;
    return 0;       /* Signal openfile to open the file in append mode  */
}



/*
 *  * Translate string to binkp escaped string, unsafe characters are escaped.
 *   */
char *unix2binkp(char *fn)
{
    static char buf[PATH_MAX];
    char        *p, *q;

    memset(&buf, 0, sizeof(buf));
    p = fn;
    q = buf;
    
    while (*p) {
	if (strspn(p, (char *)BNKCHARS)) {
	    *q++ = *p; 
	    *q = '\0';
	} else {
	    if (nodes.WrongEscape) {
		sprintf(q, "\\%2x", p[0]);
	    } else {
		sprintf(q, "\\x%2x", p[0]);
	    }
	}
	while (*q)
	    q++;
	p++;
    }
    *q = '\0';

    return buf;
}



/*
 *  * Translate escaped binkp string to normal string.
 *   */
char *binkp2unix(char *fn)
{
    static char buf[PATH_MAX];
    char        *p, *q, hex[3];
    int         c;

    memset(&buf, 0, sizeof(buf));
    p = fn;
    q = buf;

    while (*p) {
	if (p[0] == '\\') {
	    p++;
	    if (*p == '\\') {
		/*
		 * A backslash is transmitted
		 */
		*q++ = '\\';
		*q = '\0';
	    } else {
		/*
		 * If remote sends \x0a method instead of \0a, eat the x character.
		 * Remotes should send the x character, But some (Argus and Irex) don't.
		 */
		if ((*p == 'x') || (*p == 'X'))
		    p++;
		/*
		 * Decode hex characters
		 */
		hex[0] = *p++;
		hex[1] = *p;
		hex[2] = '\0';
		sscanf(hex, "%2x", &c);
		*q++ = c;
		*q = '\0';
	    }
	} else {
	    *q++ = *p;
	    *q = '\0';
	}
	p++;
    }
    *q = '\0';

    return buf;
}



/*
 * Fill internal binkp filelist
 */
void fill_binkp_list(binkp_list **bkll, file_list *fal, off_t offs)
{
    binkp_list  **tmpl;
    struct stat tstat;

    if (stat(fal->local, &tstat) != 0) {
	Syslog('+', "$Binkp: can't add %s to sendlist", fal->local);
	return;
    }
    if (strstr(fal->remote, (char *)".pkt"))
	bp.nethold += tstat.st_size;
    else
	bp.mailhold += tstat.st_size;
			        
    for (tmpl = bkll; *tmpl; tmpl = &((*tmpl)->next));
    *tmpl = (binkp_list *)malloc(sizeof(binkp_list));

    (*tmpl)->next   = NULL;
    (*tmpl)->state  = NoState;
    (*tmpl)->get    = FALSE;
    (*tmpl)->local  = xstrcpy(fal->local);
    (*tmpl)->remote = xstrcpy(unix2binkp(fal->remote));
    (*tmpl)->offset = offs;
    (*tmpl)->size   = tstat.st_size;
    (*tmpl)->date   = tstat.st_mtime;
}




void debug_binkp_list(binkp_list **bkll)
{
    binkp_list  *tmpl;

    Syslog('B', "Current filelist:");

    for (tmpl = *bkll; tmpl; tmpl = tmpl->next)
	Syslog('B', "%s %s %s %ld", MBSE_SS(tmpl->local), MBSE_SS(tmpl->remote), lbstate[tmpl->state], tmpl->offset);
}



/*
 * Clear current filelist
 */
void binkp_clear_filelist(void)
{
    binkp_list	*tmp;

    if (tosend != NULL) {
	Syslog('b', "Binkp: clear current filelist");

	for (tmp = bll; bll; bll = tmp) {
	    tmp = bll->next;
	    if (bll->local)
		free(bll->local);
	    if (bll->remote)
		free(bll->remote);
	    free(bll);
	}

	tidy_filelist(tosend, TRUE);
	tosend = NULL;
	respond = NULL;
    }
}


