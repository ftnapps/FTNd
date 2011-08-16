/*****************************************************************************
 *
 * Purpose .................: Fidonet binkp protocol
 * Binkp protocol copyright : Dima Maloff.
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "bopenfile.h"
#include "respfreq.h"
#include "filelist.h"
#include "opentcp.h"
#include "rdoptions.h"
#include "lutil.h"
#include "binkp.h"
#include "config.h"
#include "md5b.h"
#include "inbound.h"
#include "callstat.h"
#include "mbcico.h"


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
extern struct sockaddr_in   peeraddr4;
extern int	most_debug;
extern int	laststat;
extern int	crashme;
extern int	session_state;

int		gotblock = 0;


extern unsigned int	sentbytes;
extern unsigned int	rcvdbytes;

typedef enum {RxWaitF, RxAccF, RxReceD, RxWriteD, RxEOB, RxDone} RxType;
typedef enum {TxGNF, TxTryR, TxReadS, TxWLA, TxDone} TxType;
typedef enum {Ok, Failure, Continue} TrType;
typedef enum {No, Can, Want, Active} OptionState;
typedef enum {NoState, Sending, IsSent, Got, Skipped, Get} FileState;
typedef enum {InitTransfer, Switch, Receive, Transmit, DeinitTransfer} FtType;
typedef enum {CompNone, CompGZ, CompBZ2, CompPLZ} CompType;

static char *rxstate[] = { (char *)"RxWaitF", (char *)"RxAccF", (char *)"RxReceD", 
			   (char *)"RxWriteD", (char *)"RxEOB", (char *)"RxDone" };
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
static char *opstate[] = { (char *)"No  ", (char *)"Can ", (char *)"Want", (char *)"Act." };
#endif
static char *cpstate[] = { (char *)"No", (char *)"GZ", (char *)"BZ2", (char *)"PLZ" };


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


int compress_init(int type);
int do_compress(int type, char *dst, int *dst_len, char *src, int *src_len, int finish, void *data);
void compress_deinit(int type, void *data);
void compress_abort(int type, void *data);
int decompress_init(int type);
int do_decompress(int type, char *dst, int *dst_len, char *src, int *src_len, void *data);
int decompress_deinit(int type, void *data);
int decompress_abort(int type, void *data);

#define ZBLKSIZE        1024    /* read/write file buffer size */
	

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
    int			PLZwe;			/* Zlib packet compression	    */
    int			PLZthey;		/* Zlib packet compression	    */
    int			cmpblksize;		/* Ideal next blocksize		    */

						/* Receiver buffer		    */
    char		*rxbuf;			/* Receiver buffer		    */
    int			GotFrame;		/* Frame complete flag		    */
    int			rxlen;			/* Frame length			    */
    int			cmd;			/* Frame command flag		    */
    int			blklen;			/* Frame blocklength		    */
    unsigned short	header;			/* Frame header			    */
    int			rc;			/* General return code		    */
    
    int			rsize;			/* Receiver filesize		    */
    int			roffs;			/* Receiver offset		    */
    char		*rname;			/* Receiver filename		    */
    time_t		rtime;			/* Receiver filetime		    */
    FILE		*rxfp;			/* Receiver file		    */
    off_t		rxbytes;		/* Receiver bytecount		    */
    int			rxpos;			/* Receiver position		    */
    int			rxcompressed;		/* Receiver compressed bytes	    */
    char                *ropts;                 /* Receiver M_FILE optional args    */
    int			rmode;			/* Receiver compression mode	    */

    struct timezone	tz;			/* Timezone			    */

    char		*txbuf;			/* Transmitter buffer		    */
    int			txlen;			/* Transmitter file length	    */
    FILE		*txfp;			/* Transmitter file		    */
    int			txpos;			/* Transmitter position		    */
    int			stxpos;			/* Transmitter start position	    */
    int			txcompressed;		/* Transmitter compressed bytes	    */
    int			tmode;			/* Transmitter compression mode	    */
    int			tfsize;			/* Transmitter filesize		    */

    int			local_EOB;		/* Local EOB sent		    */
    int			remote_EOB;		/* Got EOB from remote		    */
    int			messages;		/* Messages sent + rcvd		    */
    unsigned int	nethold;		/* Netmail on hold		    */
    unsigned int	mailhold;		/* Packed mail on hold		    */

    int			batchnr;
    int			msgs_on_queue;		/* Messages on the queue	    */

    int			buggyIrex;		/* Buggy Irex detected		    */

    int			txcpos;			/* Transmitter compressed position  */
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    int			EXTCMDwe;		/* EXTCMD flag			    */
    int			EXTCMDthey;
#endif
#ifdef	HAVE_ZLIB_H
    int			GZwe;			/* GZ compression flag		    */
    int			GZthey;
#endif
#ifdef	HAVE_BZLIB_H
    int			BZ2we;			/* BZ2 compression flag		    */
    int			BZ2they;
#endif
    int			NRwe;			/* NR mode			    */
    int			NRthey;
    int			NDwe;			/* ND mode			    */
    int			NDthey;
    int			NDAwe;			/* NDA mode			    */
    int			NDAthey;
};



void	*z_idata = NULL;			/* Data for zstream                 */
void	*z_odata = NULL;			/* Data for zstream                 */
char	*z_obuf;				/* Compression buffer               */

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
int	binkp_banner(int);			    /* Send system banner	    */
int	binkp_send_comp_opts(int);		    /* Send compression options	    */
void	binkp_set_comp_state(void);		    /* Set compression state	    */
int	binkp_recv_command(char *, unsigned int *, int *);   /* Receive command frame	    */
void	parse_m_nul(char *);			    /* Parse M_NUL message	    */
int	binkp_poll_frame(void);			    /* Poll for a frame		    */
void	binkp_add_message(char *frame);		    /* Add cmd frame to queue	    */
int	binkp_process_messages(void);		    /* Process the queue	    */
char	*unix2binkp(char *);			    /* Binkp -> Unix escape	    */
char	*binkp2unix(char *);			    /* Unix -> Binkp escape	    */
void	fill_binkp_list(binkp_list **, file_list *, off_t); /* Build pending files  */
int	binkp_pendingfiles(void);		    /* Count pending files	    */
void	binkp_clear_filelist(int);		    /* Clear current filelist	    */

static int  orgbinkp(void);			    /* Originate session state	    */
static int  ansbinkp(void);			    /* Answer session state	    */
static int  file_transfer(void);		    /* File transfer state	    */



/************************************************************************************/
/*
 *   General entry point
 */

int binkp(int role)
{
    int		rc = 0;
    
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
    bp.ropts = calloc(512, sizeof(char));
    bp.rxfp = NULL;
    bp.local_EOB = FALSE;
    bp.remote_EOB = FALSE;
    bp.msgs_on_queue = 0;
    bp.cmpblksize = SND_BLKSIZE;
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    bp.EXTCMDwe = bp.EXTCMDthey = No;	    /* Default	*/
#endif
#ifdef	HAVE_ZLIB_H
    if (localoptions & NOPLZ)
	bp.PLZthey = bp.PLZwe = No;
    else
	bp.PLZthey = bp.PLZwe = Can;
    z_obuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
    if (localoptions & NOGZBZ2)
	bp.GZthey = bp.GZwe = No;
    else {
	bp.GZthey = bp.GZwe = Can;
	bp.EXTCMDwe = bp.EXTCMDthey = Can;
    }
#else
    bp.PLZthey = bp.PLZwe = No;
#endif
#ifdef	HAVE_BZLIB_H
    if (localoptions & NOGZBZ2)
	bp.BZ2they = bp.BZ2we = No;
    else {
	bp.BZ2they = bp.BZ2we = Can;
	bp.EXTCMDwe = bp.EXTCMDthey = Can;
    }
#endif
    bp.buggyIrex = FALSE;
    if (localoptions & NONR)
	bp.NRwe = Can;
    else
	bp.NRwe = Want;
    bp.NRthey = Can;
    bp.NDwe = No;
    bp.NDthey = No;
    bp.NDAwe = No;
    bp.NDAthey = No;

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

    if (((Loaded && nodes.NoBinkp11) || bp.buggyIrex) && (bp.Major == 1) && (bp.Minor != 0)) {
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
    if (bp.ropts)
	free(bp.ropts);
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    if ((bp.rmode != CompNone) && z_idata)
	decompress_deinit(bp.rmode, z_idata);
    if ((bp.tmode != CompNone) && z_odata)
	compress_deinit(bp.tmode, z_odata);
    if (z_obuf)
	free(z_obuf);
#endif
    rc = abs(rc);
    if (rc)
	session_state = STATE_BAD;

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
    unsigned int    bufl;
    fa_list	    **tmp, *tmpa;
    faddr	    *fa, ra;
    callstat	    *cst;

SM_START(ConnInit)

SM_STATE(ConnInit)

    SM_PROCEED(WaitConn)

SM_STATE(WaitConn)

    Loaded = FALSE;
    Syslog('+', "Binkp: node %s", ascfnode(remote->addr, 0x1f));
    IsDoing("Connect binkp %s", ascfnode(remote->addr, 0xf));

    rc = binkp_banner(TRUE);
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
			    cst = getstatus(fa);
			    if (cst->trystat)
				laststat = cst->trystat;
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
		snprintf(history.aka.domain, 13, "%s", printable(remote->addr->domain, 0));

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
		if (SendPass) {
		    bp.Secure = TRUE;
		    session_state = STATE_SECURE;
		} else
		    session_state = STATE_UNSECURE;
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

    IsDoing("Binkp to %s", ascfnode(remote->addr, 0xf));
    binkp_set_comp_state();
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
    unsigned int    bufl;
    fa_list	    **tmp, *tmpa;
    faddr	    *fa;
    callstat	    *cst;

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

    if (!CFG.NoMD5 && ((bp.MD_Challenge = MD_getChallenge(NULL, &peeraddr4)) != NULL)) {
	/*
	 * Answering site MUST send CRAM message as very first M_NUL
	 */
	char s[MD5_DIGEST_LEN*2+15]; /* max. length of opt string */
	strcpy(s, "OPT ");
	MD_toString(s + strlen(s), bp.MD_Challenge[0], bp.MD_Challenge+1);
	bp.CRAMflag = TRUE;
	if ((rc = binkp_send_command(MM_NUL, "%s", s))) {
	    SM_ERROR;
	}
    }

    if ((rc = binkp_banner(FALSE))) {
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
			    cst = getstatus(fa);
			    if (cst->trystat)
				laststat = cst->trystat;
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

	        history.aka.zone  = remote->addr->zone;
	        history.aka.net   = remote->addr->net;
	        history.aka.node  = remote->addr->node;
	        history.aka.point = remote->addr->point;
	        snprintf(history.aka.domain, 13, "%s", printable(remote->addr->domain, 0));

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
    if (bp.Secure)
    	session_state = STATE_SECURE;
    else
    	session_state = STATE_UNSECURE;

    Syslog('+', "Binkp: %s%sprotected session", bp.CRAMflag ? "MD5 ":"", bp.Secure ? "":"un");
    inbound_open(remote->addr, bp.Secure, TRUE);
    binkp_send_command(MM_OK, "%ssecure", bp.Secure ? "":"non-");
    SM_PROCEED(Opts)

SM_STATE(Opts)

    IsDoing("Binkp from %s", ascfnode(remote->addr, 0xf));
    if (localoptions & NOGZBZ2) {
#ifdef	HAVE_ZLIB_H
	bp.GZwe = bp.GZthey = No;
	Syslog('b', "Binkp: no GZ compression for this node");
#endif
#ifdef	HAVE_BZLIB_H
	bp.BZ2we = bp.BZ2they = No;
	Syslog('b', "Binkp: no BZ2 compression for this node");
#endif
    }
#ifdef	HAVE_ZLIB_H
    if (localoptions & NOPLZ) {
	bp.PLZwe = bp.PLZthey = No;
	Syslog('b', "Binkp: no PLZ compression for this node");
    }
#endif

    binkp_send_comp_opts(FALSE);
    binkp_set_comp_state();
    
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
 */
int file_transfer(void)
{
    int		rc = 0;
    TrType	Trc = Ok;
    
    for (;;) {
	switch (bp.FtState) {
	    case InitTransfer:	binkp_settimer(BINKP_TIMEOUT);
				bp.RxState = RxWaitF;
				bp.TxState = TxGNF;
				bp.FtState = Switch;
				bp.messages = 0;
				Syslog('b', "Binkp: last session was %d", laststat);
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
				binkp_clear_filelist(bp.rc);
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
#ifdef	__NetBSD__
    struct statvfs  sfs;
#else
    struct statfs   sfs;
#endif
    int		    written;
    off_t	    rxbytes;
    int		    bcmd, rc = 0;
    int		    rc1 = 0, nget = bp.blklen, zavail, nput;
    char	    zbuf[ZBLKSIZE];
    char	    *buf = bp.rxbuf;

    Syslog('b', "Binkp: receiver %s", rxstate[bp.RxState]);

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
			binkp_clear_filelist(0);
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
	
	if (((bp.rmode == CompGZ) || (bp.rmode == CompBZ2)) && z_idata) {
	    decompress_deinit(bp.rmode, z_idata);
	    z_idata = NULL;
	}

	bp.rmode = CompNone;
	if (strlen(bp.rxbuf) < 512) {
	    snprintf(bp.rname, 512, "%s", strtok(bp.rxbuf+1, " \n\r"));
	    bp.rsize = atoi(strtok(NULL, " \n\r"));
	    bp.rtime = atoi(strtok(NULL, " \n\r"));
	    bp.roffs = atoi(strtok(NULL, " \n\r"));
	    snprintf(bp.ropts, 512, "%s", printable(strtok(NULL, " \n\r\0"), 0));
	    if (strcmp((char *)"GZ", bp.ropts) == 0)
		bp.rmode = CompGZ;
	    else if (strcmp((char *)"BZ2", bp.ropts) == 0)
		bp.rmode = CompBZ2;
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

	Syslog('+', "Binkp: receive file \"%s\" date %s size %ld offset %ld comp %s",
		bp.rname, date(bp.rtime), bp.rsize, bp.roffs, cpstate[bp.rmode]);
	if (bp.roffs == -1) {
	    /*
	     * Even without NR mode Taurus sends as if it's in NR mode.
	     */
	    if ((bp.NRwe != Active) && (bp.NRthey != Active)) {
		Syslog('b', "Binkp: detected Taurus bug, start workaround");
	    }
	    bp.roffs = 0;
	    rc =  binkp_send_command(MM_GET, "%s %ld %ld %ld", bp.rname, bp.rsize, bp.rtime, bp.roffs);
	    bp.RxState = RxWaitF;
	    if (rc)
		return Failure;
	    else
		return Ok;
	}
	(void)binkp2unix(bp.rname);
	rxbytes = bp.rxbytes;

	/*
	 * Open file if M_FILE frame has offset 0
	 * Else the file is already open.
	 */
	if (bp.roffs && (bp.roffs == rxbytes)) {
	    Syslog('b', "Binkp: offset == rxbytes, don't open file again");
	    if (fseek(bp.rxfp, bp.roffs, SEEK_SET) == 0)
		Syslog('b', "Binkp: file position set to %ld", bp.roffs);
	    else
		WriteError("$Binkp: can't fseek to %ld", bp.roffs);
	} else {
	    bp.rxfp = bopenfile(binkp2unix(bp.rname), bp.rtime, bp.rsize, &rxbytes);
	}

	bp.rxbytes = rxbytes;
	bp.rxcompressed = 0;

	gettimeofday(&rxtvstart, &bp.tz);
	bp.rxpos = bp.roffs;

	if (enoughspace(CFG.freespace) == 0) {
	    Syslog('+', "Binkp: low diskspace, sending BSY");
	    binkp_send_command(MM_BSY, "Low diskspace, try again later");
	    bp.RxState = RxDone;
	    bp.TxState = TxDone;
	    bp.rc = MBERR_FTRANSFER;
	    return Failure;
	}

#ifdef	__NetBSD__
	if (statvfs(tempinbound, &sfs) == 0) {
#else
	if (statfs(tempinbound, &sfs) == 0) {
#endif
	    if ((bp.rsize / (sfs.f_bsize + 1)) >= sfs.f_bfree) {
		Syslog('!', "Binkp: only %u blocks free (need %u) in %s for this file", sfs.f_bfree, 
			    (unsigned int)(bp.rsize / (sfs.f_bsize + 1)), tempinbound);
		bclosefile(FALSE);
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
	} else if ((bp.rxbytes < bp.rsize) && ((bp.roffs == 0) || (bp.roffs == -1)) && bp.rxbytes) {
	    Syslog('+', "Binkp: partial file present, resync");
	    rc = binkp_send_command(MM_GET, "%s %ld %ld %ld", bp.rname, bp.rsize, bp.rtime, bp.rxbytes);
	    bp.RxState = RxWaitF;
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
	    Syslog('b', "Binkp: rsize=%d, rxbytes=%d, roffs=%d, goto RxReceD", bp.rsize, bp.rxbytes, bp.roffs);
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
	    bclosefile(FALSE);
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

	written = 0;
        if ((bp.rmode == CompBZ2) || (bp.rmode == CompGZ)) {
	    /*
	     * Receive stream compressed data
	     */
	    if (z_idata == NULL) {
		Syslog('b', "Binkp: decompress_init start");
		if (decompress_init(bp.rmode)) {
		    Syslog('+', "Binkp: can't init decompress");
		    bp.RxState = RxDone;
		    return Failure;
		} else
		    Syslog('b', "Binkp: decompress_init success");
	    }
	    while (nget) {
		zavail = ZBLKSIZE;
		nput = nget;
		rc1 = do_decompress(bp.rmode, zbuf, &zavail, buf, &nput, z_idata);
		if (rc1 < 0) {
		    Syslog('+', "Binkp: decompress %s error %d", bp.rname, rc1);
		    bp.RxState = RxDone;
		    return Failure;
		}
		if (zavail != 0 && fwrite(zbuf, zavail, 1, bp.rxfp) < 1) {
		    Syslog('+', "$Binkp: write error");
		    decompress_abort(bp.rmode, z_idata);
		    z_idata = NULL;
		    bp.RxState = RxDone;
		    return Failure;
		}
		buf += nput;
		nget -= nput;
		written += zavail;
		bp.rxcompressed += zavail - nput;
	    }
	    bp.blklen = written;    /* Correct physical to virtual blocklength */
	    if (rc1 == 1) {
		if ((rc1 = decompress_deinit(bp.rmode, z_idata)) < 0)
		    Syslog('+', "Binkp: decompress_deinit retcode %d", rc1);
		else
		    Syslog('b', "Binkp: decompress_deinit done");
		z_idata = NULL;
	    }
	} else {
	    written = fwrite(bp.rxbuf, 1, bp.blklen, bp.rxfp);
	}

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
	    bclosefile(TRUE);
	    bp.rxpos = bp.rxpos - bp.rxbytes;
	    gettimeofday(&rxtvend, &bp.tz);
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
	    if (bp.rxcompressed)
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
    int		rc = 0, eof = FALSE, rc1 = 0;
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    int		sz;
#endif
    char	*extra;
    char	*nonhold_mail;
    fa_list	*eff_remote;
    file_list	*tsl;
    static binkp_list	*tmp;

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
		if ((tsl->remote != NULL) && (bp.batchnr < 20))
		    fill_binkp_list(&bll, tsl, 0L);
	    }

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

		bp.tmode = CompNone;
		extra = (char *)"";

		if ((tmp->compress == CompGZ) || (tmp->compress == CompBZ2)) {
		    bp.tmode = tmp->compress;
		    Syslog('b', "Binkp: compress_init start");
		    if ((rc1 = compress_init(bp.tmode))) {
			Syslog('+', "Binkp: compress_init failed (rc=%d)", rc1);
			tmp->compress = CompNone;
			bp.tmode = CompNone;
		    } else {
			if (bp.tmode == CompBZ2)
			    extra = (char *)" BZ2";
			else if (bp.tmode == CompGZ)
			    extra = (char *)" GZ";
			Syslog('b', "Binkp: compress_init ok, extra=%s", extra);
		    }
		}

		bp.txpos = bp.txcpos = bp.stxpos = tmp->offset;
		bp.txcompressed = 0;
		bp.tfsize = tmp->size;
		Syslog('+', "Binkp: send \"%s\" as \"%s\"", MBSE_SS(tmp->local), MBSE_SS(tmp->remote));
		Syslog('+', "Binkp: size %u bytes, dated %s, comp %s", 
			(unsigned int)tmp->size, date(tmp->date), cpstate[bp.tmode]);
		rc = binkp_send_command(MM_FILE, "%s %u %d %d%s", MBSE_SS(tmp->remote), 
			(unsigned int)tmp->size, (int)tmp->date, (unsigned int)tmp->offset, extra);
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

	bp.TxState = TxTryR;
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
	if ((bp.tmode == CompBZ2) || (bp.tmode == CompGZ)) {
	    int nput = 0;  /* number of compressed bytes */
	    int nget = 0;  /* number of read uncompressed bytes from buffer */
	    int ocnt;      /* number of bytes compressed by one call */
	    int rc2;
	    off_t fleft;

	    sz = bp.tfsize - ftell(bp.txfp);
	    if (bp.cmpblksize < sz)
		sz = bp.cmpblksize;

	    while (TRUE) {
		ocnt = bp.cmpblksize - nput;
		nget = sz;
		if (nget > ZBLKSIZE)
		    nget = ZBLKSIZE;
		fleft = bp.tfsize - ftell(bp.txfp);
		fseek(bp.txfp, bp.txpos, SEEK_SET);
		nget = fread(z_obuf, 1, nget, bp.txfp);
		rc2 = do_compress(bp.tmode, bp.txbuf + nput, &ocnt, z_obuf, &nget, fleft ? 0 : 1, z_odata);
		if (rc2 == -1) {
		    Syslog('+', "Binkp: compression error rc=%d", rc2);
		    return Failure;
		}
		bp.txpos += nget;
		bp.txcpos += ocnt;
		nput += ocnt;
    
		/*
		 * Compressed block is filled for transmission
		 */
		if ((nput == bp.cmpblksize) || (fleft == 0)) {
		    bp.txlen = nput;
		    rc = binkp_send_frame(FALSE, bp.txbuf, bp.txlen);
		    if (rc)
			return Failure;
		    sentbytes += bp.txlen;
		    
		    if (rc2 == 1) {
			/*
			 * Last compressed block is sent, set eof.
			 */
			eof = TRUE;
			bp.txcompressed = bp.tfsize - bp.txcpos;
		    }
		    break;
		}
	    }

	} else {
#endif

	    /*
	     * Send uncompressed block
	     */
	    fseek(bp.txfp, bp.txpos, SEEK_SET);
	    bp.txlen = fread(bp.txbuf, 1, bp.cmpblksize, bp.txfp);
	    eof = feof(bp.txfp);
	    if (ferror(bp.txfp)) {
		WriteError("$Binkp: error reading from file");
		bp.TxState = TxDone;
		cursend->state = Skipped;
		return Failure;
	    }
	    if (bp.txlen) {
		bp.txpos += bp.txlen;
		sentbytes += bp.txlen;
		rc = binkp_send_frame(FALSE, bp.txbuf, bp.txlen);
		if (rc)
		    return Failure;
	    }

#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
	}
#endif

	if ((bp.txlen == 0) || eof) {

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
		if (bp.txcompressed)
		    Syslog('+', "Binkp: %s", compress_stat(bp.stxpos, bp.txcompressed));
#endif
		Syslog('+', "Binkp: OK %s", transfertime(txtvstart, txtvend, bp.stxpos, TRUE));
	    } else {
		Syslog('+', "Binkp: transmitter skipped file after %ld seconds", txtvend.tv_sec - txtvstart.tv_sec);
	    }

	    if ((bp.tmode == CompBZ2) || (bp.tmode == CompGZ)) {
		compress_deinit(bp.tmode, z_odata);
		z_odata = NULL;
		bp.tmode = CompNone;
		Syslog('b', "Binkp: compress_deinit done");
	    }

	    cursend->state = IsSent;
	    cursend = NULL;
	    bp.TxState = TxGNF;
	    return Ok;
	} 
		
	return Ok;

    } else if (bp.TxState == TxWLA) {

	if ((bp.msgs_on_queue == 0) && (binkp_pendingfiles() == 0)) {

	    if ((bp.RxState >= RxEOB) && (bp.Major == 1) && (bp.Minor == 0)) {
		bp.TxState = TxDone;
		if (bp.local_EOB && bp.remote_EOB) {
		    Syslog('b', "Binkp: binkp/1.0 session seems complete");
		    bp.RxState = RxDone;
		}

		binkp_clear_filelist(rc);
		return Ok;
	    }

	    if ((bp.RxState < RxEOB) && (bp.Major == 1) && (bp.Minor == 0)) {
		bp.TxState = TxWLA;
		return Ok;
	    }

	    if ((bp.Major == 1) && (bp.Minor != 0)) {
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
			binkp_clear_filelist(rc);
			return Ok; /* Continue is not good here, troubles with binkd on slow links. */
		    }
		}

		binkp_clear_filelist(rc);
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
    int		    rcz;
    uLongf	    zlen;
    Bytef	    *zbuf;

    if ((len >= BINKP_PLZ_BLOCK) && (bp.PLZwe == Active)) {
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
    /*
     * Only use compression for DATA blocks larger then 20 bytes.
     * Also, don't send PLZ blocks if GZ or BZ2 mode is active.
     * This means if the current file is send uncompressed we still
     * might send some blocks compressed. The receiver needs to deal
     * with this if it showed PLZ, GZ and BZ2 options.
     */
    if ((bp.PLZwe == Active) && (len > 20) && (!cmd) && (bp.tmode != CompGZ) && (bp.tmode != CompBZ2)) {
	zbuf = calloc(BINKP_ZIPBUFLEN, sizeof(char));
	zlen = BINKP_PLZ_BLOCK -1;
	rcz = compress2(zbuf, &zlen, (Bytef *)buf, (uLong)len, 9);
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
		    rc = PUT((char *)zbuf, (int)zlen);
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
	vsnprintf(buf, 1024, fmt, args);
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
int binkp_banner(int originate)
{
    time_t  t;
    int	    rc;

    rc = binkp_send_command(MM_NUL,"SYS %s", CFG.bbs_name);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"ZYZ %s", CFG.sysop_name);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"LOC %s", CFG.location);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"NDL %s", CFG.IP_Flags);
    t = time(NULL);
    if (!rc)
	rc = binkp_send_command(MM_NUL,"TIME %s", rfcdate(t));
    if (!rc) {
	if (nodes.NoBinkp11 || bp.buggyIrex)
	    rc = binkp_send_command(MM_NUL,"VER mbcico/%s/%s-%s %s/%s", VERSION, OsName(), OsCPU(), PRTCLNAME, PRTCLOLD);
	else
	    rc = binkp_send_command(MM_NUL,"VER mbcico/%s/%s-%s %s/%s", VERSION, OsName(), OsCPU(), PRTCLNAME, PRTCLVER);
    }
    if (strlen(CFG.IP_Phone) && !rc)
	rc = binkp_send_command(MM_NUL,"PHN %s", CFG.IP_Phone);
    if (strlen(CFG.comment) && !rc)
	rc = binkp_send_command(MM_NUL,"OPM %s", CFG.comment);

    /*
     * Send compression options
     */
    if (originate) {
	if (!rc) {
	    rc = binkp_send_comp_opts(TRUE);
	}
    }

    return rc;
}



/*
 * Send compression options
 */
int binkp_send_comp_opts(int originate)
{
    int	    rc = 0, nr = FALSE;
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    int	    plz = FALSE, gz = FALSE, bz2 = FALSE;
    char    *p = NULL;
    
#ifdef	HAVE_ZLIB_H
    if ((bp.GZwe == Can) || (bp.GZthey == Can) || (bp.GZthey == Want)) {
	gz = TRUE;
	bp.GZwe = Want;
    }
#endif
#ifdef	HAVE_BZLIB_H
    if ((bp.BZ2we == Can) || (bp.BZ2they == Can) || (bp.BZ2they == Want)) {
	bz2 = TRUE;
	bp.BZ2we = Want;
    }
#endif

#ifdef	HAVE_ZLIB_H
    if ((bp.PLZwe == Can) || (bp.PLZthey == Can) || (bp.PLZthey == Want)) {
	plz = TRUE;
	bp.PLZwe = Want;
    }
#endif

    Syslog('b', "Binkp: binkp_send_comp_opts(%s) NRwe=%s NRthey=%s", 
	    originate ?"TRUE":"FALSE", opstate[bp.NRwe], opstate[bp.NRthey]);
    if (originate) {
	if (bp.NRwe == Want) {
	    Syslog('b', "Binkp: binkp_send_comp_opts(TRUE) NRwe=Want");
	    nr = TRUE;
	}
    } else {
	if ((bp.NRwe == Can) && (bp.NRthey == Want)) {
	    Syslog('b', "Binkp: binkp_send_comp_opts(FALSE) NRwe=Can NRthey=Want");
	    bp.NRwe = Want;
	    nr = TRUE;
	}
    }

    if (plz || gz || bz2 || nr) {
	p = xstrcpy((char *)"OPT");
	if (bz2 || gz) {
	    bp.EXTCMDwe = Want;
	    p = xstrcat(p, (char *)" EXTCMD");
	}
	if (gz)
	    p = xstrcat(p, (char *)" GZ");
	if (bz2)
	    p = xstrcat(p, (char *)" BZ2");
	if (plz)
	    p = xstrcat(p, (char *)" PLZ");
	if (nr)
	    p = xstrcat(p, (char *)" NR");
	rc = binkp_send_command(MM_NUL,"%s", p);
	free(p);
    }
#endif

    return rc;    
}



void binkp_set_comp_state(void)
{
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    Syslog('b', "Binkp: EXTCMD they=%s we=%s", opstate[bp.EXTCMDthey], opstate[bp.EXTCMDwe]);
    if ((bp.EXTCMDthey == Want) && (bp.EXTCMDwe == Want)) {
	Syslog('+', "Binkp: EXTCMD is active");
	bp.EXTCMDthey = bp.EXTCMDwe = Active;
    }
#endif

#ifdef  HAVE_BZLIB_H
    Syslog('b', "Binkp: BZ2    they=%s we=%s", opstate[bp.BZ2they], opstate[bp.BZ2we]);
    if ((bp.BZ2they == Want) && (bp.BZ2we == Want)) {
	Syslog('+', "Binkp: BZ2 compression active");
	bp.BZ2we = bp.BZ2they = Active;
    }
#endif
#ifdef  HAVE_ZLIB_H
    Syslog('b', "Binkp: GZ     they=%s we=%s", opstate[bp.GZthey], opstate[bp.GZwe]);
    if ((bp.GZthey == Want) && (bp.GZwe == Want)) {
	bp.GZwe = bp.GZthey = Active;
	Syslog('+', "Binkp: GZ compression active");
    }
#endif

#ifdef  HAVE_ZLIB_H
    Syslog('b', "Binkp: PLZ    they=%s we=%s", opstate[bp.PLZthey], opstate[bp.PLZwe]);
    if ((bp.PLZthey == Want) && (bp.PLZwe == Want)) {
	bp.PLZwe = bp.PLZthey = Active;
	Syslog('+', "Binkp: PLZ compression active");
    }
#endif

    Syslog('b', "Binkp: NR     they=%s we=%s", opstate[bp.NRthey], opstate[bp.NRwe]);
    if ((bp.NRthey == Want) && (bp.NRwe == Want)) {
	bp.NRwe = bp.NRthey = Active;
	Syslog('+', "Binkp: NR mode active");
    }
}



/*
 *  Receive command frame
 */
int binkp_recv_command(char *buf, unsigned int *len, int *cmd)
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
    Syslog('b', "Binkp: rcvd %s %s", bstate[b0 & 0x7f], printable(buf+1, 0));

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
	/*
	 * Irex 2.24 upto 2.29 claims binkp/1.1 while it is binkp/1.0
	 * Set a flag so we can activate a workaround. This only works
	 * for incoming sessions.
	 */
	if ((p = strstr(msg+4, "Internet Rex 2."))) {
	    q = strtok(p + 15, (char *)" \0");
	    if ((atoi(q) >= 24) && (atoi(q) <= 29)) {
		Syslog('b', "        : Irex bug detected, workaround activated");
		bp.buggyIrex = TRUE;
	    }
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
	    if (strncmp(q, (char *)"CRAM-MD5-", 9) == 0) { /* No SHA-1 support */
		if (CFG.NoMD5) {
		    Syslog('+', "Binkp: Remote supports MD5, but it's turned off here");
		} else {
		    if (bp.MD_Challenge)
			free(bp.MD_Challenge);
		    bp.MD_Challenge = MD_getChallenge(q, NULL);
		}
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
	    } else if (strcmp(q, (char *)"EXTCMD") == 0) {
		Syslog('b', "Binkp: remote wants EXTCMD mode");
		if (bp.EXTCMDthey == Can) {
		    bp.EXTCMDthey = Want;
		    binkp_set_comp_state();
		}
#endif
#ifdef	HAVE_BZLIB_H
	    } else if (strcmp(q, (char *)"BZ2") == 0) {
		Syslog('b', "Binkp: remote wants BZ2 mode");
		if (bp.BZ2they == Can) {
		    bp.BZ2they = Want;
		    binkp_set_comp_state();
		}
#endif
#ifdef	HAVE_ZLIB_H
	    } else if (strcmp(q, (char *)"GZ") == 0) {
		Syslog('b', "Binkp: remote wants GZ mode");
		if (bp.GZthey == Can) {
		    bp.GZthey = Want;
		    binkp_set_comp_state();
		}
#endif
#ifdef	HAVE_ZLIB_H
	    } else if (strcmp(q, (char *)"PLZ") == 0) {
		Syslog('b', "Binkp: remote wants PLZ mode");
		if (bp.PLZthey == Can) {
		    bp.PLZthey = Want;
		    binkp_set_comp_state();
		}
#endif
	    } else if (strcmp(q, (char *)"NR") == 0) {
		Syslog('b', "Binkp: remote requests NR mode");
		if (bp.NRthey == Can) {
		    bp.NRthey = Want;
		    binkp_set_comp_state();
		}
	    } else if (strcmp(q, (char *)"NDA") == 0) {
		Syslog('b', "Binkp: remote wants NDA mode, NOT SUPPORTED HERE YET");
		bp.NDAthey = Want;
	    } else if (strcmp(q, (char *)"ND") == 0) {
		Syslog('b', "Binkp: remote wants ND mode, NOT SUPPORTED HERE YET");
		bp.NDthey = Want;
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
    uLongf	    zlen;
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
		    if ((bp.PLZthey == Active) && (bp.rmode == CompNone)) {
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
		    /*
		     * Got a PLZ compressed block
		     */
		    if ((bp.PLZthey == Active) && (bp.header & BINKP_PLZ_BLOCK) && (bp.rmode == CompNone) && bp.blklen) {
			zbuf = calloc(BINKP_ZIPBUFLEN, sizeof(char));
			zlen = BINKP_PLZ_BLOCK -1;
			rc = uncompress((Bytef *)zbuf, &zlen, (Bytef *)bp.rxbuf, bp.rxlen -1);
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
			gotblock++;
			if (crashme && (gotblock > 10)) {
			    Syslog('b', "Binkp: will crash now");
			    die(SIGHUP);
			}
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
    char	*lname, *ropts;
    time_t	ltime;
    int		lsize, loffs;

    Syslog('b', "Binkp: Process The Messages Queue Start");

    lname = calloc(512, sizeof(char));
    ropts = calloc(512, sizeof(char));

    for (tmpq = tql; tmpq; tmpq = tmpq->next) {
	Syslog('+', "Binkp: %s \"%s\"", bstate[tmpq->cmd], printable(tmpq->data, 0));
	if (tmpq->cmd == MM_GET) {
	    snprintf(lname, 512, "%s", strtok(tmpq->data, " \n\r"));
	    lsize = atoi(strtok(NULL, " \n\r"));
	    ltime = atoi(strtok(NULL, " \n\r"));
	    loffs = atoi(strtok(NULL, " \n\r"));
	    snprintf(ropts, 512, "%s", printable(strtok(NULL, " \n\r\0"), 0));
	    Syslog('b', "Binkp: m_file options \"%s\"", ropts);
	    if (strcmp((char *)"NZ", ropts) == 0) {
#ifdef	HAVE_ZLIB_H
		bp.GZwe = bp.GZthey = No;
#endif
#ifdef	HAVE_BZLIB_H
		bp.BZ2we = bp.BZ2they = No;
#endif
		Syslog('+', "Binkp: received NZ on M_GET command, compression turned off");
	    }
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
	    snprintf(lname, 512, "%s", strtok(tmpq->data, " \n\r"));
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
	    snprintf(lname, 512, "%s", strtok(tmpq->data, " \n\r"));
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
 * Translate string to binkp escaped string, unsafe characters are escaped.
 */
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
		snprintf(q, 4, "\\%2x", p[0]);
	    } else {
		snprintf(q, 5, "\\x%2x", p[0]);
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
 * Translate escaped binkp string to normal string.
 */
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
    FILE	*fp;
    struct stat tstat;
    int		comp;
    char	*unp;

    for (tmpl = bkll; *tmpl; tmpl = &((*tmpl)->next));
    *tmpl = (binkp_list *)malloc(sizeof(binkp_list));

    (*tmpl)->next   = NULL;
    (*tmpl)->state  = NoState;
    if ((fp = fopen(fal->local, "r")) == NULL) {
	if ((errno == ENOENT) || (errno == EINVAL)) {
	    Syslog('+', "Binkp: file %s doesn't exist, removing", MBSE_SS(fal->local));
	    (*tmpl)->state  = Got;
	    execute_disposition(fal);
	}
    } else {
	fclose(fp);
	stat(fal->local, &tstat);
	if (strstr(fal->remote, (char *)".pkt"))
	    bp.nethold += tstat.st_size;
	else
	    bp.mailhold += tstat.st_size;
    }
    (*tmpl)->get      = FALSE;
    (*tmpl)->local    = xstrcpy(fal->local);
    (*tmpl)->remote   = xstrcpy(unix2binkp(fal->remote));
    (*tmpl)->offset   = offs;
    (*tmpl)->size     = tstat.st_size;
    (*tmpl)->date     = tstat.st_mtime;
    (*tmpl)->compress = CompNone;

    /*
     * Search compression method, but only if GZ or BZ2 compression is active.
     */
    comp = FALSE;
#ifdef	HAVE_ZLIB_H
    if ((bp.GZwe == Active) && (bp.GZthey == Active))
	comp = TRUE;
#endif
#ifdef	HAVE_BZLIB_H
    if ((bp.BZ2we == Active) && (bp.BZ2they == Active))
	comp = TRUE;
#endif
    if (!comp)
	return;
    
    /*
     * Don't compress small files
     */
    if (tstat.st_size <= 1024)
	return;

    /*
     * Check file type
     */
    unp = unpacker(fal->local);
    if (unp && strcmp((char *)"TAR", unp)) {
	Syslog('b', "Binkp: %s send as-is", fal->local);
	return;
    }

#ifdef	HAVE_BZLIB_H
    /*
     * Use BZ2 for files > 200K
     */
    if ((bp.BZ2we == Active) && (bp.BZ2they == Active) && (tstat.st_size > 202400)) {
	(*tmpl)->compress = CompBZ2;
	Syslog('b', "Binkp: %s compressor BZ2", fal->local);
	return;
    }
#endif
#ifdef	HAVE_ZLIB_H
    if ((bp.GZwe == Active) && (bp.GZthey == Active)) {
	(*tmpl)->compress = CompGZ;
	Syslog('b', "Binkp: %s compressor GZ", fal->local);
	return;
    }
#endif
}



/*
 * Clear current filelist
 */
void binkp_clear_filelist(int rc)
{
    binkp_list	*tmp;
    file_list	*fal;

    if (tosend != NULL) {
	Syslog('b', "Binkp: binkp_clear_filelist(%d)", rc);

	for (tmp = bll; bll; bll = tmp) {
	    tmp = bll->next;
	    if (bll->local)
		free(bll->local);
	    if (bll->remote)
		free(bll->remote);
	    free(bll);
	}

	/* WARNING: Added 16-07-2004 to see if this is safe to clean /flo files.
	 *
	 *  Remove sent fake files like .spl and .flo
	 */
	for (fal = tosend; fal; fal = fal->next) {
	    if ((fal->remote == NULL) && (rc == 0))
		execute_disposition(fal);
	}
	tidy_filelist(tosend, TRUE);
	tosend = NULL;
	respond = NULL;
    }
}



/*****************************************************************************
 *
 *  Compression support for GZ and BZ2 modes. Routines from the original
 *  binkd package, slightly adopted for mbcico.
 *
 *  Original written by val khokhlov, FIDONet 2:550/180
 *
 */


int compress_init(int type)
{
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    int	    lvl;
#endif

    switch (type) {
#ifdef HAVE_BZLIB_H
	case CompBZ2: {
	    z_odata = calloc(1, sizeof(bz_stream));
	    if (z_odata == NULL) {
		Syslog('+', "Binkp: compress_init: not enough memory (%lu needed)", sizeof(bz_stream));
		return BZ_MEM_ERROR;
	    }
	    lvl = 1; /* default is small (100K) buffer */
	    return BZ2_bzCompressInit((bz_stream *)z_odata, lvl, 0, 0);
	}
#endif
#ifdef HAVE_ZLIB_H
	case CompGZ: {
	    z_odata = calloc(1, sizeof(z_stream));
	    if (z_odata == NULL) {
		Syslog('+', "Binkp: compress_init: not enough memory (%lu needed)", sizeof(z_stream));
		return Z_MEM_ERROR;
	    }
	    lvl = 9; /* Maximum compression */
	    if (lvl <= 0) lvl = Z_DEFAULT_COMPRESSION;
		return deflateInit((z_stream *)z_odata, lvl);
	}
#endif
	default:
	    Syslog('+', "Binkp: unknown compression method: %d; data lost", type);
    }
    return -1;
}



int do_compress(int type, char *dst, int *dst_len, char *src, int *src_len, int finish, void *data)
{
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    int rc;
#endif

    switch (type) {
#ifdef HAVE_BZLIB_H
	case CompBZ2: {
	    bz_stream *zstrm = (bz_stream *)data;
	    zstrm->next_in = (char *)src;
	    zstrm->avail_in = (unsigned int)*src_len;
	    zstrm->next_out = (char *)dst;
	    zstrm->avail_out = (unsigned int)*dst_len;
	    rc = BZ2_bzCompress(zstrm, finish ? BZ_FINISH : 0);
	    *src_len -= (int)zstrm->avail_in;
	    *dst_len -= (int)zstrm->avail_out;
	    if (rc == BZ_RUN_OK || rc == BZ_FLUSH_OK || rc == BZ_FINISH_OK) 
		rc = 0;
	    if (rc == BZ_STREAM_END) 
		rc = 1;
	    return rc;
	}
#endif
#ifdef HAVE_ZLIB_H
	case CompGZ: {
	    z_stream *zstrm = (z_stream *)data;
	    zstrm->next_in = (Bytef *)src;
	    zstrm->avail_in = (uLong)*src_len;
	    zstrm->next_out = (Bytef *)dst;
	    zstrm->avail_out = (uLong)*dst_len;
	    rc = deflate(zstrm, finish ? Z_FINISH : 0);
	    *src_len -= (int)zstrm->avail_in;
	    *dst_len -= (int)zstrm->avail_out;
	    if (rc == Z_STREAM_END) 
		rc = 1;
	    return rc;
	}
#endif
	default:
	    Syslog('+', "Binkp: unknown compression method: %d; data lost", type);
    }
    return -1;
}



void compress_deinit(int type, void *data)
{
    switch (type) {
#ifdef HAVE_BZLIB_H
	case CompBZ2: {
	    int rc = BZ2_bzCompressEnd((bz_stream *)data);
	    if (rc < 0) 
		Syslog('+', "Binkp: BZ2_bzCompressEnd error: %d", rc);
	    break;
	}
#endif
#ifdef HAVE_ZLIB_H
	case CompGZ: {
	    int rc = deflateEnd((z_stream *)data);
	    if (rc < 0) 
		Syslog('+', "Binkp: deflateEnd error: %d", rc);
	    break;
	}
#endif
	default:
	    Syslog('+', "Binkp: unknown compression method: %d", type);
    }
    free(data);
    return;
}



void compress_abort(int type, void *data)
{
    char    buf[1024];
    int	    i, j;

    if (data) {
	Syslog('b', "Binkp: purge compress buffers");
	do {
	    i = sizeof(buf);
	    j = 0;
	} while (do_compress(type, buf, &i, NULL, &j, 1, data) == 0 && i > 0);
	compress_deinit(type, data);
    }
}



int decompress_init(int type)
{
    switch (type) {
#ifdef HAVE_BZLIB_H
	case CompBZ2: {
	    z_idata = calloc(1, sizeof(bz_stream));
	    if (z_idata == NULL) {
		Syslog('+', "Binkp: decompress_init: not enough memory (%lu needed)", sizeof(bz_stream));
		return BZ_MEM_ERROR;
	    }
	    return BZ2_bzDecompressInit((bz_stream *)z_idata, 0, 0);
	}
#endif
#ifdef HAVE_ZLIB_H
	case CompGZ: {
	    z_idata = calloc(1, sizeof(z_stream));
	    if (z_idata == NULL) {
		Syslog('+', "Binkp: decompress_init: not enough memory (%lu needed)", sizeof(z_stream));
		return Z_MEM_ERROR;
	    }
	    return inflateInit((z_stream *)z_idata);
	}
#endif
	default:
	    Syslog('+', "Binkp: unknown compression method: %d; data lost", type);
    }
    return -1;
}



int do_decompress(int type, char *dst, int *dst_len, char *src, int *src_len, void *data) 
{
#if defined(HAVE_ZLIB_H) || defined(HAVE_BZLIB_H)
    int	    rc;
#endif

    switch (type) {
#ifdef HAVE_BZLIB_H
	case CompBZ2: {
	    bz_stream *zstrm = (bz_stream *)data;
	    zstrm->next_in = (char *)src;
	    zstrm->avail_in = (unsigned int)*src_len;
	    zstrm->next_out = (char *)dst;
	    zstrm->avail_out = (unsigned int)*dst_len;
	    rc = BZ2_bzDecompress(zstrm);
	    *src_len -= (int)zstrm->avail_in;
	    *dst_len -= (int)zstrm->avail_out;
	    if (rc == BZ_RUN_OK || rc == BZ_FLUSH_OK) 
		rc = 0;
	    if (rc == BZ_STREAM_END) 
		rc = 1;
	    return rc;
	}
#endif
#ifdef HAVE_ZLIB_H
	case CompGZ: {
	    z_stream *zstrm = (z_stream *)data;
	    zstrm->next_in = (Bytef *)src;
	    zstrm->avail_in = (uLong)*src_len;
	    zstrm->next_out = (Bytef *)dst;
	    zstrm->avail_out = (uLong)*dst_len;
	    rc = inflate(zstrm, 0);
	    *src_len -= (int)zstrm->avail_in;
	    *dst_len -= (int)zstrm->avail_out;
	    if (rc == Z_STREAM_END) 
		rc = 1;
	    return rc;
	}
#endif
	default:
	    Syslog('+', "Binkp: unknown compression method: %d; data lost", type);
    }
    return 0;
}



int decompress_deinit(int type, void *data)
{
    int	    rc = -1;

    switch (type) {
#ifdef HAVE_BZLIB_H
	case CompBZ2: {
	    rc = BZ2_bzDecompressEnd((bz_stream *)data);
	    break;
	}
#endif
#ifdef HAVE_ZLIB_H
	case CompGZ: {
	    rc = inflateEnd((z_stream *)data);
	    break;
	}
#endif
	default:
	    Syslog('+', "Binkp: unknown compression method: %d", type);
	    break;
    }
    free(data);
    return rc;
}



int decompress_abort(int type, void *data) 
{
    char    buf[1024];
    int	    i, j;

    if (data) {
	Syslog('b', "Binkp: purge decompress buffers");
	do {
	    i = sizeof(buf);
	    j = 0;
	} while (do_decompress(type, buf, &i, NULL, &j, data) == 0 && i > 0);
	return decompress_deinit(type, data);
    }
    return 0;
}



/*
 * Abort, called from die in case there were errors.
 * This will reconstruct file data that is still in
 * compressed state in memory so that resync works.
 */
void binkp_abort(void)
{
    Syslog('b', "Binkp: abort");
    if ((bp.rmode != CompNone) && z_idata)
	decompress_abort(bp.rmode, z_idata);
    if ((bp.tmode != CompNone) && z_odata)
	compress_abort(bp.tmode, z_odata);
}

