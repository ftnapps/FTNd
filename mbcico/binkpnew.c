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


static char rbuf[MAX_BLKSIZE + 1];

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

typedef enum {RxWaitFile, RxAcceptFile, RxReceData, RxWriteData, RxEndOfBatch, RxDone} RxType;
typedef enum {TxGetNextFile, TxTryRead, TxReadSend, TxWaitLastAck, TxDone} TxType;
typedef enum {InitTransfer, Switch, Receive, Transmit} TransferType;
typedef enum {Ok, Failure, Continue} TrType;
typedef enum {No, WeCan, WeWant, TheyWant, Active} OptionState;

static char *rxstate[] = { (char *)"RxWaitFile", (char *)"RxAccpetFile", (char *)"RxReceData", 
			   (char *)"RxWriteData", (char *)"RxEndOfBatch", (char *)"RxDone" };
static char *txstate[] = { (char *)"TxGetNextFile", (char *)"TryRread", (char *)"ReadSent", 
			   (char *)"WaitLastAck", (char *)"TxDone" };
//static char *tfstate[] = { (char *)"InitTransfer", (char *)"Switch", (char *)"Receive", (char *)"Transmit" };
//static char *trstate[] = { (char *)"Ok", (char *)"Failure", (char *)"Continue" };
static char *opstate[] = { (char *)"No", (char *)"WeCan", (char *)"WeWant", (char *)"TheyWant", (char *)"Active" };


//static int	TfState;
static time_t	Timer;
int		transferred = FALSE;		/* Anything transferred in batch    */
int		ext_rand = 0;



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

};

struct binkprec	bp;				/* Global structure		    */


/*
 * Prototypes
 */
int	binkp_send_frame(int, char *, int);	    /* Send cmd/data frame	    */
int	binkp_send_command(int, ...);		    /* Send command frame	    */
void	binkp_settimer(int);			    /* Set timeout timer	    */
int	binkp_expired(void);			    /* Timer expired?		    */
int	binkp_banner(void);			    /* Send system banner	    */
int	binkp_recv_command(char *, int *, int *);   /* Receive command frame	    */
void	parse_m_nul(char *);			    /* Parse M_NUL message	    */

static int  orgbinkp(void);			    /* Originate session state	    */



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

    if (role == 1) {
	if (orgbinkp()) {
	    rc = MBERR_SESSION_ERROR;
	}
    } else {
//	if (ansbinkp()) {
//	    rc = MBERR_SESSION_ERROR;
//	}
    }

    if (rc) {
	Syslog('!', "Binkp: session failed");
	goto binkpend;
    }

binkpend:   
    /*
     * Deinit
     */
    Syslog('b', "Binkp: deinit start");
    if (bp.MD_Challenge)
	free(bp.MD_Challenge);
    if (bp.rxbuf)
	free(bp.rxbuf);
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
		Syslog('!', "Binkp: M_BSY \"%s\"", printable(&rbuf[1], 0));
		SM_ERROR;
	    } else if (bp.rxbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(&rbuf[1], 0));
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



/************************************************************************************/
/*
 *   File Transfer State
 */



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



#endif
