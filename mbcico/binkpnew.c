/*****************************************************************************
 *
 * $Id$
 * Purpose .................: Fidonet binkp protocol
 * Binkp protocol copyright : Dima Maloff.
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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

#ifdef USE_NEWBINKP

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/nodelist.h"
#include "../lib/dbnode.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
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
#include "binkpnew.h"
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

static char *rxstate[] = { (char *)"RxWaitF", (char *)"RxAccF", (char *)"RxReceD", 
			   (char *)"RxWriteD", (char *)"RxEOB", (char *)"RxDone" };
static char *txstate[] = { (char *)"TxGNF", (char *)"TxTryR", (char *)"TxReadS", 
			   (char *)"TxWLA", (char *)"TxDone" };
static char *trstate[] = { (char *)"Ok", (char *)"Failure", (char *)"Continue" };
static char *opstate[] = { (char *)"No", (char *)"WeCan", (char *)"WeWant", (char *)"TheyWant", (char *)"Active" };
char *lbstat[]={(char *)"None", (char *)"Sending", (char *)"IsSent", (char *)"Got", (char *)"Skipped", (char *)"Get"};

static time_t	Timer;
int		transferred = FALSE;		/* Anything transferred in batch    */
int		ext_rand = 0;


the_queue	*tql = NULL;			/* The Queue List		    */
file_list	*tosend = NULL;			/* Files to send		    */
binkp_list	*bll = NULL;			/* Files to send with status	    */



struct binkprec {
    int			Role;			/* 1=orig, 0=answer		    */
    int			RxState;		/* Receiver state		    */
    int			TxState;		/* Transmitter state		    */
						/* Option flags			    */
    int			CRAMflag;		/* CRAM authentication		    */
						/* Session state		    */
    int			Secure;			/* Secure session		    */
    int			Major;			/* Major protocol version	    */
    int			Minor;			/* Minor protocol version	    */
    unsigned char	*MD_Challenge;		/* Received challenge data	    */
    
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
    struct timeval	rxtvstart;		/* Receiver start time		    */
    struct timeval	rxtvend;		/* Receiver end time		    */
    struct timezone	tz;			/* Timezone			    */
    int			DidSendGET;		/* Receiver send GET status	    */

    char		*txbuf;			/* Transmitter buffer		    */
    int			txlen;			/* Transmitter file length	    */
    FILE		*txfp;			/* Transmitter file		    */
    int			txpos;			/* Transmitter position		    */
    int			stxpos;			/* Transmitter start position	    */
    struct timeval	txtvstart;		/* Transmitter start time	    */
    struct timeval	txtvend;		/* Transmitter end time		    */

    int			local_EOB;		/* Local EOB sent		    */
    int			remote_EOB;		/* Got EOB from remote		    */
    unsigned long	nethold;		/* Netmail on hold		    */
    unsigned long	mailhold;		/* Packed mail on hold		    */
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
int	binkp_recv_command(char *, int *, int *);   /* Receive command frame	    */
void	parse_m_nul(char *);			    /* Parse M_NUL message	    */
int	binkp_poll_frame(void);			    /* Poll for a frame		    */
void	binkp_addqueue(char *frame);		    /* Add cmd frame to queue	    */
int	binkp_countqueue(void);			    /* Count commands on the queue  */
int	binkp_processthequeue(void);		    /* Process the queue	    */
int	binkp_resync(off_t);			    /* File resync		    */
char	*unix2binkp(char *);			    /* Binkp -> Unix escape	    */
char	*binkp2unix(char *);			    /* Unix -> Binkp escape	    */
void	fill_binkp_list(binkp_list **, file_list *, off_t);
void	debug_binkp_list(binkp_list **);
int	binkp_pendingfiles(void);

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

    most_debug = TRUE;

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

    if (role == 1) {
	if (orgbinkp()) {
	    rc = MBERR_SESSION_ERROR;
	}
    } else {
	if (ansbinkp()) {
	    rc = MBERR_SESSION_ERROR;
	}
    }

    rc = file_transfer();

    if (rc) {
	Syslog('!', "Binkp: session failed");
	goto binkpend;
    }

binkpend:   
    /*
     * Deinit
     */
    Syslog('b', "Binkp: deinit start");
    if (bp.rname)
	free(bp.rname);
    if (bp.MD_Challenge)
	free(bp.MD_Challenge);
    if (bp.rxbuf)
	free(bp.rxbuf);
    if (bp.txbuf)
	free(bp.txbuf);
    Syslog('b', "Binkp: deinit end");
    rc = abs(rc);
    Syslog('b', "Binkp: rc=%d", rc);
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
    faddr   *primary;
    char    *p, *q, *pwd;
    int     i, rc = 0, bufl, cmd, dupe, SendPass = FALSE;
    fa_list **tmp, *tmpa;
    faddr   *fa, ra;

SM_START(ConnInit)

SM_STATE(ConnInit)

    SM_PROCEED(WaitConn)

SM_STATE(WaitConn)

    Loaded = FALSE;
    Syslog('+', "Binkp: node %s", ascfnode(remote->addr, 0x1f));
    IsDoing("Connect binkp %s", ascfnode(remote->addr, 0xf));

    /*
     * Build options we want (Add PLZ etc).
     */
    p = xstrcpy((char *)"OPT");
    // if ((noderecord(remote->addr)) && nodes.CRC32 && (bp.CRCflag == WeCan)) {
    // p = xstrcat(p, (char *)" CRC");
    // bp.CRCflag = WeWant;
    // Syslog('b', "CRCflag WeCan => WeWant");
    // }
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
		for (tmpa = remote; tmpa; tmpa = tmpa->next) {
		    Syslog('+', "Address : %s", ascfnode(tmpa->addr, 0x1f));
		    if (nodelock(tmpa->addr, mypid)) {
			binkp_send_command(MM_BSY, "Address %s locked", ascfnode(tmpa->addr, 0x1f));
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
    WaitConn,
    WaitAddr,
    IsPasswd,
    WaitPwd,
    PwdAck,
    Opts
SM_NAMES
    (char *)"WaitConn",
    (char *)"WaitAddr",
    (char *)"IsPasswd",
    (char *)"WaitPwd",
    (char *)"PwdAck",
    (char *)"Opts"
SM_EDECL
    char    *p, *q, *pw;
    int     i, rc, bufl, cmd, dupe, we_have_pwd = FALSE;
    fa_list **tmp, *tmpa;
    faddr   *fa;

SM_START(WaitConn)

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
	MD_toString(s+4, bp.MD_Challenge[0], bp.MD_Challenge+1);
	bp.CRAMflag = TRUE;
	rc = binkp_send_command(MM_NUL, "%s", s);
	if (rc) {
	    SM_ERROR;
	}
    }

    rc = binkp_banner();
    if (rc) {
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
		
		for (tmpa = remote; tmpa; tmpa = tmpa->next) {
		    Syslog('+', "Address : %s", ascfnode(tmpa->addr, 0x1f));
		    if (nodelock(tmpa->addr, mypid)) {
			binkp_send_command(MM_BSY, "Address %s locked", ascfnode(tmpa->addr, 0x1f));
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

SM_DECL(file_transfer, (char *)"file_transfer")
SM_STATES
    InitTransfer,
    Switch,
    Receive,
    Transmit
SM_NAMES
    (char *)"InitTransfer",
    (char *)"Switch",
    (char *)"Receive",
    (char *)"Transmit"
SM_EDECL
    int	    rc;
    TrType  Trc;

SM_START(InitTransfer)

SM_STATE(InitTransfer)

    binkp_settimer(BINKP_TIMEOUT);
    bp.RxState = RxWaitF;
    bp.TxState = TxGNF;
    SM_PROCEED(Switch)

SM_STATE(Switch)

    for (;;) {
	if ((bp.RxState == RxDone) && (bp.TxState == TxDone)) {
	    Syslog('+', "Binkp: file transfer complete rc=%d", bp.rc);
	    if (bp.rc) {
		SM_ERROR;
	    } else {
		SM_SUCCESS;
	    }
	}

	/*
	 * Check for received data
	 */
	rc = binkp_poll_frame();
	if (rc == -1) {
	    SM_ERROR;
	} else if (rc == 1) {
	    SM_PROCEED(Receive)
	}

	/*
	 * Check if there is room in the output buffer
	 */
	if ((WAITPUTGET(-1) & 2) != 0) {
	    SM_PROCEED(Transmit)
	}

	if (binkp_expired()) {
	    Syslog('+', "Binkp: transfer timeout");
	    bp.rc = 1;
	    SM_ERROR;
	}

	/*
	 * Nothing done, release
	 */
	usleep(1);
    }

SM_STATE(Receive)

    for (;;) {
	Trc = binkp_receiver();
	Syslog('b', "Binkp: receiver rc=%s", trstate[Trc]);
	if (Trc == Ok) {
	    binkp_settimer(BINKP_TIMEOUT / 2);
	    SM_PROCEED(Switch)
	} else if (Trc == Failure) {
	    /* Close all opened files */
	    SM_ERROR;
	}
    }

SM_STATE(Transmit)

    for (;;) {
        Trc = binkp_transmitter();
	Syslog('b', "Binkp: transmitter rc=%s", trstate[Trc]);
	if (Trc == Ok) {
	    binkp_settimer(BINKP_TIMEOUT / 2);
	    SM_PROCEED(Switch)
	} else if (Trc == Failure) {
	    Syslog('b', "Clear current filelist");
	    tidy_filelist(tosend, FALSE);
	    tosend = NULL;
	    /* Close all opened files */
	    SM_ERROR;
	}
    }

SM_END
SM_RETURN


/************************************************************************************/
/*
 *  Receiver routine
 */

TrType binkp_receiver(void)
{
    int		    bcmd, rc = 0;
    struct statfs   sfs;
    long	    written;
    
    Syslog('B', "Binkp: receiver state %s", rxstate[bp.RxState]);

    if (bp.RxState == RxWaitF) {
        if (! bp.GotFrame) {
            return Ok;
        }
	
	bp.GotFrame = FALSE;
	bp.rxlen = 0;
	bp.header = 0;
	bp.blklen = 0;

        if (! bp.cmd) {
            Syslog('b', "Binkp: got DATA frame in %s state, ignored", rxstate[bp.RxState]);
            return Ok;
        }

        bcmd = bp.rxbuf[0];
        if (bcmd == MM_ERR) {
            Syslog('+', "Binkp: got M_ERR \"%s\"", printable(bp.rxbuf +1, 0));
            bp.RxState = RxDone;
            return Failure;
        } else if ((bcmd == MM_GET) || (bcmd == MM_GOT) || (bcmd == MM_SKIP)) {
            binkp_addqueue(bp.rxbuf);
            return Ok;
        } else if (bcmd == MM_NUL) {
            parse_m_nul(bp.rxbuf +1);
            return Ok;
        } else if (bcmd == MM_EOB) {
            Syslog('+', "Binkp: got M_EOB");
            bp.RxState = RxEOB;
	    bp.remote_EOB = TRUE;
            return Ok;
        } else if (bcmd == MM_FILE) {
            bp.RxState = RxAccF;
            return Continue;
        } else if (bcmd <= MM_MAX) {
            Syslog('+', "Binkp: got unexpected %s frame in %s state", bstate[bcmd], rxstate[bp.RxState]);
            bp.RxState = RxDone;
            return Failure;
        } else {
            Syslog('+', "Binkp: got unexpected unknown frame, ignore");
            return Ok;
        }
    } else if (bp.RxState == RxAccF) {
        Syslog('b', "Binkp: \"%s\"", printable(bp.rxbuf +1, 0));
	if (strlen(bp.rxbuf) < 512) {
	    sscanf(bp.rxbuf+1, "%s %ld %ld %ld", bp.rname, &bp.rsize, &bp.rtime, &bp.roffs);
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
	bp.rxfp = openfile(binkp2unix(bp.rname), bp.rtime, bp.rsize, &bp.rxbytes, binkp_resync);
	
	if (bp.DidSendGET) {
	    Syslog('b', "Binkp: DidSendGET is set");
	    /*
	     * The file was partly received, via the openfile the resync function
	     * has send a GET command to start this file with a offset. This means
	     * we will get a new FILE command to open this file with a offset.
	     */
	    bp.RxState = RxReceD;
	    return Ok;
	}

	gettimeofday(&bp.rxtvstart, &bp.tz);
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
	    Syslog('b', "blocksize %lu free blocks %lu", sfs.f_bsize, sfs.f_bfree);
	    Syslog('b', "need %lu blocks", (unsigned long)(bp.rsize / (sfs.f_bsize + 1)));
	    if ((bp.rsize / (sfs.f_bsize + 1)) >= sfs.f_bfree) {
		Syslog('!', "Only %lu blocks free (need %lu) in %s", sfs.f_bfree, 
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
	if (! bp.GotFrame) {
	    return Ok;
	}

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
	    binkp_addqueue(bp.rxbuf);
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
	    Syslog('+', "Binkp: got unexpected %s frame in %s state", bstate[bcmd], rxstate[bp.RxState]);
	    bp.RxState = RxDone;
	    return Failure;
	} else {
	    Syslog('+', "Binkp: got unexpected unknown frame, ignore");
	    return Ok;
	}
    } else if (bp.RxState == RxWriteD) {
	written = fwrite(bp.rxbuf, 1, bp.blklen, bp.rxfp);
	Syslog('b', "Binkp: write %d bytes of %d bytes", written, bp.blklen);

	bp.GotFrame = FALSE;
	bp.rxlen = 0;
	bp.header = 0;
	bp.blklen = 0;

	if (!written && bp.blklen) {
	    Syslog('+', "Binkp: file write error");
	    bp.RxState = RxDone;
	    return Failure;
	}
	bp.rxpos += written;
	if (bp.rxpos == bp.rsize) {
	    Syslog('b', "We are at EOF");
	    rc = binkp_send_command(MM_GOT, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
	    closefile();
	    bp.rxpos = bp.rxpos - bp.rxbytes;
	    gettimeofday(&bp.rxtvend, &bp.tz);
	    Syslog('+', "Binkp: OK %s", transfertime(bp.rxtvstart, bp.rxtvend, bp.rxpos, FALSE));
	    rcvdbytes += bp.rxpos;
	    bp.RxState = RxWaitF;
	    transferred = TRUE;
	    if (rc)
		return Failure;
	    else
		return Ok;
	}
	bp.RxState = RxReceD;
	return Ok;
    } else if (bp.RxState == RxEOB) {
	if (! bp.GotFrame) {
	    return Ok;
	}

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
		binkp_addqueue(bp.rxbuf);
		return Ok;
	    } else if (bcmd == MM_NUL) {
		parse_m_nul(bp.rxbuf +1);
		return Ok;
	    } else if (bcmd <= MM_MAX) {
		Syslog('+', "Binkp: got unexpected %s frame in %s state", bstate[bcmd], rxstate[bp.RxState]);
		bp.RxState = RxDone;
		return Failure;
	    } else {
		Syslog('+', "Binkp: got unexpected unknown frame, ignore");
		return Ok;
	    }
	} else {
	    Syslog('+', "Binkp: got unexpected data frame in %s state", rxstate[bp.RxState]);
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
    int		rc = 0;
    char	*nonhold_mail;
    fa_list	*eff_remote;
    file_list	*tsl;
    static binkp_list	*tmp, *cursend;


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

	    /*
	     *  Build a new filelist from the existing filelist.
	     *  This one is special for binkp behaviour.
	     */
	    for (tsl = tosend; tsl; tsl = tsl->next) {
		if (tsl->remote != NULL)
		    fill_binkp_list(&bll, tsl, 0L);
	    }
	    debug_binkp_list(&bll);

	    Syslog('+', "Binkp: mail %ld, files %ld bytes", bp.nethold, bp.mailhold);
	    binkp_send_command(MM_NUL, "TRF %ld %ld", bp.nethold, bp.mailhold);
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
		Syslog('+', "Binkp: send \"%s\" as \"%s\"", MBSE_SS(tmp->local), MBSE_SS(tmp->remote));
		Syslog('+', "Binkp: size %lu bytes, dated %s", (unsigned long)tmp->size, date(tmp->date));
		binkp_send_command(MM_FILE, "%s %lu %ld %ld", MBSE_SS(tmp->remote), 
			(unsigned long)tmp->size, (long)tmp->date, (unsigned long)tmp->offset);
		gettimeofday(&bp.txtvstart, &bp.tz);
		tmp->state = Sending;
		cursend = tmp;
		bp.TxState = TxTryR;
		transferred = TRUE;
		return Continue;
	    } /* if state == NoState */
	} /* for */

	if (tmp == NULL) {
	    /*
	     * No more files
	     */
	
	    Syslog('b', "Binkp: sending M_EOB");
	    rc = binkp_send_command(MM_EOB, "");
	    bp.TxState = TxWLA;
	    bp.local_EOB = TRUE;
	    if (rc)
		return Failure;
	    else
		return Continue;
	}
	return Ok;

    } else if (bp.TxState == TxTryR) {

	if (binkp_countqueue() == 0) {
	    Syslog('b', "The queue is empty");
	    bp.TxState = TxReadS;
	    return Continue;
	} else {
	    if (binkp_processthequeue()) {
		bp.TxState = TxDone;
		return Failure;
	    } else 
		return Continue;
	}

    } else if (bp.TxState == TxReadS) {
	fseek(bp.txfp, bp.txpos, SEEK_SET);
	bp.txlen = fread(bp.txbuf, 1, SND_BLKSIZE, bp.txfp);

	if (bp.txlen == 0) {

	    if (ferror(bp.txfp)) {
		WriteError("$Binkp: error reading from file");
		bp.TxState = TxDone;
		cursend->state = Skipped;
		debug_binkp_list(&bll);
		return Failure;
	    }

	    /*
	     * calculate time needed and bytes transferred
	     */
	    gettimeofday(&bp.txtvend, &bp.tz);

	    /*
	     * Close transmitter file
	     */
	    fclose(bp.txfp);
	    bp.txfp = NULL;

	    if (bp.txpos >= 0) {
		bp.stxpos = bp.txpos - bp.stxpos;
		Syslog('+', "Binkp: OK %s", transfertime(bp.txtvstart, bp.txtvend, bp.stxpos, TRUE));
	    } else {
		Syslog('+', "Binkp: transmitter skipped file after %ld seconds", bp.txtvend.tv_sec - bp.txtvstart.tv_sec);
	    }

	    cursend->state = IsSent;
	    bp.TxState = TxGNF;
	    return Ok;
	} else {
	    bp.txpos += bp.txlen;
	    sentbytes += bp.txlen;
	    binkp_send_frame(FALSE, bp.txbuf, bp.txlen);
	    bp.TxState = TxTryR;
	    return Ok;
	}

    } else if (bp.TxState == TxWLA) {

	if ((binkp_countqueue() == 0) && (binkp_pendingfiles() == 0) && (bp.RxState >= RxEOB)) {
	    Syslog('b', "The queue is empty and RxState >= RxEOB");
	    bp.TxState = TxDone;
	    if (bp.local_EOB && bp.remote_EOB)
		bp.RxState = RxDone;    /* Not in FSP-1018 rev.1 */
	    if (tosend != NULL) {
		Syslog('b', "Clear current filelist");

		debug_binkp_list(&bll);

		/*
		 *  Process all send files.
		 */
		for (tsl = tosend; tsl; tsl = tsl->next) {
		    if (tsl->remote == NULL) {
			execute_disposition(tsl);
		    } else {
			for (tmp = bll; tmp; tmp = tmp->next) {
			    if ((strcmp(tmp->local, tsl->local) == 0) && (tmp->state == Got)) {
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

		tidy_filelist(tosend, TRUE);
		tosend = NULL;
	    }
	    return Ok;
	}

	if ((binkp_countqueue() == 0) && (binkp_pendingfiles() == 0) && (bp.RxState < RxEOB)) {
	    Syslog('b', "The queue is empty and RxState < RxEOB");
	    bp.TxState = TxWLA;
	    return Ok;
	}

	if (binkp_countqueue()) {
	    if (binkp_processthequeue()) {
		return Failure;
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
    int		    rc;

    if (cmd)
	header = ((BINKP_CONTROL_BLOCK + len) & 0xffff);
    else
	header = ((BINKP_DATA_BLOCK + len) & 0xffff);

    rc = PUTCHAR((header >> 8) & 0x00ff);
    if (!rc)
	rc = PUTCHAR(header & 0x00ff);
    if (len && !rc)
	rc = PUT(buf, len);
    FLUSHOUT();
    binkp_settimer(BINKP_TIMEOUT);

    Syslog('b', "Binkp: send %s frame, len=%d rc=%d", cmd?"CMD":"DATA", len, rc);
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

    Syslog('b', "Binkp: send %s %s", bstate[id], buf);
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
	Syslog('+', "Binkp: timeout");
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
    if (!rc)
	rc = binkp_send_command(MM_NUL,"VER mbcico/%s/%s-%s %s/%s", VERSION, OsName(), OsCPU(), PRTCLNAME, PRTCLVER);
    if (strlen(CFG.Phone) && !rc)
	rc = binkp_send_command(MM_NUL,"PHN %s", CFG.Phone);
    if (strlen(CFG.comment) && !rc)
	rc = binkp_send_command(MM_NUL,"OPM %s", CFG.comment);

    return rc;
}



/*
 *  Receive command frame
 */
int binkp_recv_command(char *buf, int *len, int *cmd)
{
    int b0, b1;

    *len = *cmd = 0;

    b0 = GETCHAR(BINKP_TIMEOUT);
    if (tty_status)
	goto to;
    if (b0 & 0x80)
	*cmd = 1;

    b1 = GETCHAR(1);
    if (tty_status)
	goto to;

    *len = (b0 & 0x7f) << 8;
    *len += b1;

    GET(buf, *len, BINKP_TIMEOUT / 2);
    buf[*len] = '\0';
    if (tty_status)
	goto to;

to:
    if (tty_status)
	WriteError("Binkp: TCP receive error: %d %s", tty_status, ttystat[tty_status]);
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
	    Syslog('b', "Remote protocol version %d.%d", bp.Major, bp.Minor);
	    /*
	     * Disable MB if protocol > 1.0 and MB was not yet active.
	     */
//	    if ((bp.MBflag != Active) && (((bp.Major * 10) + bp.Minor) > 10)) {
//		Syslog('b', "MBflag %s => No", opstate[bp.MBflag]);
//		bp.MBflag = No;
//	    }
	}
    } else if (strncmp(msg, "PHN ", 4) == 0) {
	Syslog('+', "Phone   : %s", msg+4);
    
    } else if (strncmp(msg, "OPM ", 4) == 0) {
	Syslog('+', "Remark  : %s", msg+4);
    
    } else if (strncmp(msg, "TRF ", 4) == 0) {
	Syslog('+', "Binkp: remote has %s mail/files for us", msg+4);
    
    } else if (strncmp(msg, "OPT ", 4) == 0) {
	Syslog('+', "Options : %s", msg+4);

//	if (strstr(msg, (char *)"MB") != NULL) {
//	    Syslog('b', "Remote requests MB, current state = %s", opstate[bp.MBflag]);
//	    if ((bp.MBflag == WeCan) && (bp.Major == 1) && (bp.Minor == 0)) {   /* Answering session and do binkp/1.0   */
//		bp.MBflag = TheyWant;
//		Syslog('b', "MBflag WeCan => TheyWant");
//		binkp_send_control(MM_NUL,"OPT MB");
//		Syslog('b', "MBflag TheyWant => Active");
//		bp.MBflag = Active;
//	    } else if ((bp.MBflag == WeWant) && (bp.Major == 1) && (bp.Minor == 0)) {  /* Originating session and do binkp/1.0 */
//		bp.MBflag = Active;
//		Syslog('b', "MBflag WeWant => Active");
//	    } else {
//		Syslog('b', "MBflag is %s and received MB option", opstate[bp.MBflag]);
//
//	    }
//	}

	if (strstr(msg, (char *)"CRAM-MD5-") != NULL) { /* No SHA-1 support */
	    if (CFG.NoMD5) {
		Syslog('+', "Binkp: Remote supports MD5, but it's turned off here");
	    } else {
		if (bp.MD_Challenge)
		    free(bp.MD_Challenge);
		bp.MD_Challenge = MD_getChallenge(msg, NULL);
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
 */
int binkp_poll_frame(void)
{
    int	    c, rc = 0;

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
		    usleep(1);
		    rc = 0;
		    break;
		}
		Syslog('?', "Binkp: receiver status %s", ttystat[c]);
		bp.TxState = TxDone;
		bp.RxState = RxDone;
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
		    bp.cmd = bp.header & 0x8000;
		    bp.blklen = bp.header & 0x7fff;
		}
		if ((bp.rxlen == (bp.blklen + 1) && (bp.rxlen >= 1))) {
		    bp.GotFrame = TRUE;
		    bp.rxbuf[bp.rxlen-1] = '\0';
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
 * Add received command frame to the queue
 */
void binkp_addqueue(char *frame)
{
    the_queue	**tmpl;

    Syslog('b', "Binkp: add \"%s\" to queue", printable(frame, 0));

    for (tmpl = &tql; *tmpl; tmpl = &((*tmpl)->next));
    *tmpl = (the_queue *)malloc(sizeof(the_queue));

    (*tmpl)->next = NULL;
    (*tmpl)->cmd  = frame[0];
    (*tmpl)->data = xstrcpy(frame +1);
    
}



/*
 * Get nr of items on the queue
 */
int binkp_countqueue(void)
{
    the_queue	*tmp;
    int		count = 0;

    Syslog('b', "Binkp: count the queue");

    Syslog('b', "check %s", (tql == NULL) ?"NULL":"Not NULL");

    for (tmp = tql; tmp; tmp = tmp->next) {
	count++;
	Syslog('b', "Binkp: %02d %s \"%s\"", count, bstate[tmp->cmd], printable(tmp->data, 0));
    }


    Syslog('b', "Binkp: %d items on queue", count);
    return count;
}



int binkp_pendingfiles(void)
{
    binkp_list  *tmpl;
    int         count = 0;

    for (tmpl = bll; tmpl; tmpl = tmpl->next) {
	Syslog('B', "%s %s %s %ld", MBSE_SS(tmpl->local), MBSE_SS(tmpl->remote), lbstat[tmpl->state], tmpl->offset);
	if ((tmpl->state != Got) && (tmpl->state != Skipped))
	    count++;
    }

    Syslog('b', "Binkp: %d pending files on queue", count);
    return count;
}



/*
 * Process commands on the queue
 */
int binkp_processthequeue(void)
{
    the_queue	*tmpq;
    binkp_list	*tmp;
    int		Found;
    char	*lname;
    time_t	ltime;
    long	lsize;

    Syslog('b', "Binkp: Process The Queue");

    lname = calloc(512, sizeof(char));

    for (tmpq = tql; tmpq; tmpq = tmpq->next) {
	Syslog('b', "Binkp: %s \"%s\"", bstate[tmpq->cmd], printable(tmpq->data, 0));
	if (tmpq->cmd == MM_GOT) {
	    sscanf(tmpq->data, "%s %ld %ld", lname, &lsize, &ltime);
	    Found = FALSE;
	    for (tmp = bll; tmp; tmp = tmp->next) {
		if ((strcmp(lname, tmp->remote) == 0) && (lsize == tmp->size) && (ltime == tmp->date)) {
		    Syslog('+', "Binkp: remote GOT \"%s\"", tmp->remote);
		    tmp->state = Got;
		    Found = TRUE;
		}
		if (!Found) {
		    Syslog('!', "Binkp: unexpected GOT \"%s\"", tmpq->data);
		}
	    }
	}
    }
    tql = NULL;
    free(lname);

    return 0;
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
	Syslog('!', "$Can't add %s to sendlist", fal->local);
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
	Syslog('B', "%s %s %s %ld", MBSE_SS(tmpl->local), MBSE_SS(tmpl->remote), lbstat[tmpl->state], tmpl->offset);
}



#endif
