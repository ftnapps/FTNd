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
#include "binkp.h"
#include "config.h"
#include "md5b.h"
#include "inbound.h"


/*
 * Safe characters for binkp filenames, the rest will be escaped.
 */
#define BNKCHARS    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@&=+%$-_.!()#|"


static char rbuf[MAX_BLKSIZE + 1];

static char *bstate[] = {
    (char *)"NUL", (char *)"ADR", (char *)"PWD", (char *)"FILE", (char *)"OK",
    (char *)"EOB", (char *)"GOT", (char *)"ERR", (char *)"BSY", (char *)"GET",
    (char *)"SKIP"
};



/*
 * Local prototypes
 */
void		binkp_init(void);
void		binkp_deinit(void);
char		*unix2binkp(char *);
char		*binkp2unix(char *);
int		binkp_expired(void);
void		b_banner(void);
void		b_nul(char *);
void		fill_binkp_list(binkp_list **, file_list *, off_t);
void		debug_binkp_list(binkp_list **);
void		binkp_send_data(char *, int);
void		binkp_send_control(int id, ...);
int		binkp_recv_frame(char *, int *, int *);
void		binkp_settimer(int);
int		resync(off_t);
static int	orgbinkp(void);
static int	ansbinkp(void);
static int	binkp_batch(file_list *);


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
static int	CRAMflag = FALSE;		/* CRAM option flag		    */
static int	Secure = FALSE;			/* Secure session		    */
unsigned long	nethold, mailhold;		/* Trafic for the remote	    */
int		transferred = FALSE;		/* Anything transferred in batch    */
int		batchnr = 0, crc_errors = 0;
unsigned char	*MD_challenge = NULL;		/* Received CRAM challenge data	    */
int		ext_rand = 0;

struct binkprec {
    int			role;			/* 1=orig, 0=answer		    */
    int			RxState;		/* Receiver state		    */
    int			TxState;		/* Transmitter state		    */
    int			DidSendGET;		/* Did we send a GET command	    */
    long		rsize;			/* Receiver filesize		    */
    long		roffs;			/* Receiver offset		    */
    char		*rname;			/* Receiver filename		    */
    time_t		rtime;			/* Receiver filetime		    */
    unsigned long	rcrc;			/* Receiver crc			    */
    long		lsize;			/* Local filesize		    */
    char		*lname;			/* Local filename		    */
    time_t		ltime;			/* Local filetime		    */
    unsigned long	tcrc;			/* Transmitter crc		    */
    long		gsize;			/* GET filesize			    */
    long		goffset;		/* GET offset			    */
    char		*gname;			/* GET filename			    */
    time_t		gtime;			/* GET filetime			    */
    int			MBflag;			/* MB option flag                   */
    int			CRCflag;		/* CRC option flag		    */
    int			Major;			/* Remote major protocol version    */
    int			Minor;			/* Remote minor protocol version    */
};

struct binkprec	bp;				/* Global structure		    */



void binkp_init(void)
{
    bp.rname = calloc(512, sizeof(char));
    bp.lname = calloc(512, sizeof(char));
    bp.gname = calloc(512, sizeof(char));
    bp.MBflag = WeCan;
    if (CFG.NoCRC32)
	bp.CRCflag = No;
    else
	bp.CRCflag = WeCan;
    bp.Major = 1;
    bp.Minor = 0;
    bp.DidSendGET = FALSE;
}


void binkp_deinit(void)
{
    if (bp.rname)
	free(bp.rname);
    if (bp.lname)
	free(bp.lname);
    if (bp.gname)
	free(bp.gname);
}


int binkp(int role)
{
    int		rc = MBERR_OK;
    fa_list	*eff_remote;
    file_list	*tosend = NULL, *request = NULL, *respond = NULL, *tmpfl;
    char	*nonhold_mail;

    binkp_init();

    most_debug = TRUE;
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
	Syslog('!', "Binkp: session failed");
	binkp_deinit();
	return rc;
    }

//    if (localoptions & NOFREQS)
//	session_flags &= ~SESSION_WAZOO;
//    else
	session_flags |= SESSION_WAZOO;

    Syslog('b', "Binkp: WAZOO requests: %s", (session_flags & SESSION_WAZOO) ? "True":"False");

    nonhold_mail = (char *)ALL_MAIL;
    eff_remote = remote;
    /*
     * If remote doesn't have the 8.3 flag set, allow long filenames.
     */
    if (!nodes.FNC)
	remote_flags &= ~SESSION_FNC;
	
    tosend = create_filelist(eff_remote, nonhold_mail, 0);

    if (request != NULL) {
	Syslog('b', "Binkp: inserting request list");
	tmpfl = tosend;
	tosend = request;
	for (; request->next; request = request->next);
	request->next = tmpfl;

	request = NULL;
    }

    rc = binkp_batch(tosend);
    tidy_filelist(tosend, (rc == 0));
    tosend = NULL;

    if ((rc == 0) && transferred && (bp.MBflag == Active)) {
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

    Syslog('+', "Binkp: end transfer rc=%d", rc);
    closetcp();

    Syslog('b', "2nd batch or not, MB flag is %s", opstate[bp.MBflag]);

    if (bp.MBflag != Active) {
	/*
	 *  In singe batch mode we process filerequests after the batch.
	 *  The results will be put on hold for the calling node.
	 *  This method is also known as "dual session mode".
	 */
	respond = respond_wazoo();
	for (tmpfl = respond; tmpfl; tmpfl = tmpfl->next) {
	    if (strncmp(tmpfl->local, "/tmp", 4)) {
		attach(*remote->addr, tmpfl->local, LEAVE, 'h');
		Syslog('+', "Binkp: put on hold: %s", MBSE_SS(tmpfl->local));
	    } else {
		file_mv(tmpfl->local, pktname(remote->addr, 'h'));
		Syslog('+', "Binkp: new netmail: %s", pktname(remote->addr, 'h'));
	    }
	}
    }

    tidy_filelist(request, (rc == 0));
    tidy_filelist(tosend, (rc == 0));
    tidy_filelist(respond, 0);

    binkp_deinit();
    rc = abs(rc);
    return rc;
}



/*
 * Translate string to binkp escaped string, unsafe characters are escaped.
 */
char *unix2binkp(char *fn)
{
    static char	buf[PATH_MAX];
    char	*p, *q;

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
 * Translate escaped binkp string to normal string.
 */
char *binkp2unix(char *fn)
{
    static char buf[PATH_MAX];
    char	*p, *q, hex[3];
    int		c;

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
 *  Transmit data frame
 */
void binkp_send_data(char *buf, int len)
{
    unsigned short	header = 0;

    Syslog('B', "Binkp: send_data len=%d", len);

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
    va_list	args;
    char	*fmt, *s;
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

    Syslog('b', "Binkp: send_ctl %s \"%s\"", bstate[id], buf);
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
 * This function is called two times if a partial file exists from openfile.
 *  1. A partial file is detected, send a GET to the remote, set DidSendGET flag.
 *  2. DidSendGET is set, return 0 and let openfile open the file in append mode.
 */
int resync(off_t off)
{
    Syslog('b', "Binkp: resync(%d) DidSendGET=%s", off, bp.DidSendGET ?"TRUE":"FALSE");
    if (!bp.DidSendGET) {
	if (bp.CRCflag == Active) {
	    binkp_send_control(MM_GET, "%s %ld %ld %ld %lx", bp.rname, bp.rsize, bp.rtime, off, bp.rcrc);
	} else {
	    binkp_send_control(MM_GET, "%s %ld %ld %ld", bp.rname, bp.rsize, bp.rtime, off);
	}
	bp.DidSendGET = TRUE;
	Syslog('+', "Binkp: already %lu bytes received, requested restart with offset", (unsigned long)off);
	return -1;  /* Signal openfile not to open the file */
    }
    bp.DidSendGET = FALSE;
    return 0;	    /* Signal openfile to open the file in append mode	*/
}



/*
 *  Receive control frame
 */
int binkp_recv_frame(char *buf, int *len, int *cmd)
{
    int	b0, b1;

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



void binkp_settimer(int interval)
{
    Timer = time((time_t*)NULL) + interval;
}



int binkp_expired(void)
{
    time_t	now;

    now = time(NULL);
    if (now >= Timer)
	Syslog('+', "Binkp: timeout");
    return (now >= Timer);
}



void b_banner(void)
{
    time_t  t;

    binkp_send_control(MM_NUL,"SYS %s", CFG.bbs_name);
    binkp_send_control(MM_NUL,"ZYZ %s", CFG.sysop_name);
    binkp_send_control(MM_NUL,"LOC %s", CFG.location);
    binkp_send_control(MM_NUL,"NDL %s", CFG.Flags);
    t = time(NULL);
    binkp_send_control(MM_NUL,"TIME %s", rfcdate(t));
    binkp_send_control(MM_NUL,"VER mbcico/%s/%s-%s %s/%s", VERSION, OsName(), OsCPU(), PRTCLNAME, PRTCLVER);
    if (strlen(CFG.Phone))
	binkp_send_control(MM_NUL,"PHN %s", CFG.Phone);
    if (strlen(CFG.comment))
	binkp_send_control(MM_NUL,"OPM %s", CFG.comment);
}



void b_nul(char *msg)
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
    } else if (strncmp(msg, "NDL ", 4) == 0)
	Syslog('+', "Flags   : %s", msg+4);
    else if (strncmp(msg, "TIME ", 5) == 0)
	Syslog('+', "Time    : %s", msg+5);
    else if (strncmp(msg, "VER ", 4) == 0) {
	Syslog('+', "Uses    : %s", msg+4);
	if ((p = strstr(msg+4, PRTCLNAME "/")) && (q = strstr(p, "."))) {
	    bp.Major = atoi(p + 6);
	    bp.Minor = atoi(q + 1);
	    Syslog('b', "Remote protocol version %d.%d", bp.Major, bp.Minor);
	    /*
	     * Disable MB if protocol > 1.0 and MB was not yet active.
	     */
	    if ((bp.MBflag != Active) && (((bp.Major * 10) + bp.Minor) > 10)) {
		Syslog('b', "MBflag %s => No", opstate[bp.MBflag]);
		bp.MBflag = No;
	    }
	}
    }
    else if (strncmp(msg, "PHN ", 4) == 0)
	Syslog('+', "Phone   : %s", msg+4);
    else if (strncmp(msg, "OPM ", 4) == 0)
	Syslog('+', "Remark  : %s", msg+4);
    else if (strncmp(msg, "TRF ", 4) == 0)
	Syslog('+', "Binkp: remote has %s mail/files for us", msg+4);
    else if (strncmp(msg, "OPT ", 4) == 0) {
	Syslog('+', "Options : %s", msg+4);
	if (strstr(msg, (char *)"MB") != NULL) {
	    Syslog('b', "Remote requests MB, current state = %s", opstate[bp.MBflag]);
	    if ((bp.MBflag == WeCan) && (bp.Major == 1) && (bp.Minor == 0)) {	/* Answering session and do binkp/1.0   */
		bp.MBflag = TheyWant;
		Syslog('b', "MBflag WeCan => TheyWant");
		binkp_send_control(MM_NUL,"OPT MB");
		Syslog('b', "MBflag TheyWant => Active");
		bp.MBflag = Active;
	    } else if ((bp.MBflag == WeWant) && (bp.Major == 1) && (bp.Minor == 0)) {  /* Originating session and do binkp/1.0 */
		bp.MBflag = Active;
		Syslog('b', "MBflag WeWant => Active");
	    } else {
		Syslog('b', "MBflag is %s and received MB option", opstate[bp.MBflag]);
	    }
	}
	if (strstr(msg, (char *)"CRAM-MD5-") != NULL) {	/* No SHA-1 support */
	    if (CFG.NoMD5) {
		Syslog('+', "Binkp: Remote supports MD5, but it's turned off here");
	    } else {
		if (MD_challenge)
		    free(MD_challenge);
		MD_challenge = MD_getChallenge(msg, NULL);
	    }
	}
	if (strstr(msg, (char *)"CRC") != NULL) {
	    if (bp.CRCflag == WeCan) {
		bp.CRCflag = TheyWant;
		Syslog('b', "CRCflag WeCan => TheyWant");
	    } else if (bp.CRCflag == WeWant) {
		bp.CRCflag = Active;
		Syslog('b', "CRCflag WeWant => Active");
	    } else {
		Syslog('b', "CRCflag is %s and received CRC option", opstate[bp.CRCflag]);
	    }
	}
    } else
	Syslog('+', "Binkp: M_NUL \"%s\"", msg);
}



/*
 * Originate a binkp session
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
    int	    i, rc, bufl, cmd, dupe, SendPass = FALSE;
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
     * Build options we want
     */
    p = xstrcpy((char *)"OPT");
    if ((noderecord(remote->addr)) && nodes.CRC32 && (bp.CRCflag == WeCan)) {
	p = xstrcat(p, (char *)" CRC");
	bp.CRCflag = WeWant;
	Syslog('b', "CRCflag WeCan => WeWant");
    }
    if (strcmp(p, (char *)"OPT"))
	binkp_send_control(MM_NUL, p);
    free(p);
    b_banner();

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
    SM_PROCEED(WaitAddr)

SM_STATE(WaitAddr)
	
    for (;;) {
	if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error receiving remote info");
	    SM_ERROR;
	}

	if (cmd) {
	    if (rbuf[0] == MM_ADR) {
		p = xstrcpy(&rbuf[1]);
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
			Syslog('!', "Binkp: bad remote address: \"%s\"", printable(q, 0));
		        binkp_send_control(MM_ERR, "Bad address");
		    }
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

	        SM_PROCEED(SendPasswd)
		    
	    } else if (rbuf[0] == MM_BSY) {
		Syslog('!', "Binkp: M_BSY \"%s\"", printable(&rbuf[1], 0));
	        SM_ERROR;
		
	    } else if (rbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(&rbuf[1], 0));
		SM_ERROR;

	    } else if (rbuf[0] == MM_NUL) {
		b_nul(&rbuf[1]);

	    } else {
		binkp_send_control(MM_ERR, "Unexpected frame");
		SM_ERROR;
	    }
	}
    }

SM_STATE(SendPasswd)

    if (Loaded && strlen(nodes.Spasswd)) {
	pwd = xstrcpy(nodes.Spasswd);
	SendPass = TRUE;
    } else {
	pwd = xstrcpy((char *)"-");
    }

    if (MD_challenge) {
	char	*tp = NULL;
	tp = MD_buildDigest(pwd, MD_challenge);
	if (!tp) {
	    Syslog('!', "Unable to build MD5 digest");
	    SM_ERROR;
	}
	CRAMflag = TRUE;
	binkp_send_control(MM_PWD, "%s", tp);
	free(tp);
    } else {
	binkp_send_control(MM_PWD, "%s", pwd);
    }

    free(pwd);
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
	binkp_send_control(MM_ERR, "No AKAs in common or all AKAs busy");
	SM_ERROR;
    }

SM_STATE(IfSecure)

    SM_PROCEED(WaitOk)

SM_STATE(WaitOk)

    for (;;) {
	if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error waiting for remote acknowledge");
	    SM_ERROR;
	}

	if (cmd) {
	    if (rbuf[0] == MM_OK) {
		Syslog('b', "Binkp: M_OK \"%s\"", printable(&rbuf[1], 0));
	        if (SendPass)
		   Secure = TRUE;
		Syslog('+', "Binkp: %s%sprotected session", CRAMflag ? "MD5 ":"", Secure ? "":"un");
		SM_PROCEED(Opts)

	    } else if (rbuf[0] == MM_BSY) {
		Syslog('!', "Binkp: M_BSY \"%s\"", printable(&rbuf[1], 0));
	        SM_ERROR;

	    } else if (rbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(&rbuf[1], 0));
	        SM_ERROR;

	    } else if (rbuf[0] == MM_NUL) {
		b_nul(&rbuf[1]);

	    } else {
		binkp_send_control(MM_ERR, "Unexpected frame");
	        SM_ERROR;
	    }
	}
    }

SM_STATE(Opts)

    /*
     *  Try to initiate the MB option if the remote is binkp/1.0
     */
    if ((bp.MBflag == WeCan) && (bp.Major == 1) && (bp.Minor == 0)) {
	bp.MBflag = WeWant;
	Syslog('b', "MBflag WeCan => WeWant");
	binkp_send_control(MM_NUL, "OPT MB");
    }
    SM_SUCCESS;

SM_END
SM_RETURN
	


/*
 * Answer a binkp session
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
	binkp_send_control(MM_BSY, "This system is closed, try again later");
	SM_ERROR;
    }

    if (!CFG.NoMD5 && ((MD_challenge = MD_getChallenge(NULL, &peeraddr)) != NULL)) {
	/*
	 * Answering site MUST send CRAM message as very first M_NUL
	 */
	char s[MD5_DIGEST_LEN*2+15]; /* max. length of opt string */
	strcpy(s, "OPT ");
	MD_toString(s+4, MD_challenge[0], MD_challenge+1);
	CRAMflag = TRUE;
	binkp_send_control(MM_NUL, "%s", s);
    }
    b_banner();
    p = xstrcpy((char *)"");

    for (i = 0; i < 40; i++)
	if ((CFG.aka[i].zone) && (CFG.akavalid[i])) {
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, aka2str(CFG.aka[i]));
	}

    binkp_send_control(MM_ADR, "%s", p);
    free(p);
    SM_PROCEED(WaitAddr)

SM_STATE(WaitAddr)

    for (;;) {
	if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error waiting for remote info");
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
			Syslog('!', "Binkp: bad remote address: \"%s\"", printable(q, 0));
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
		    if (((nlent = getnlent(tmpa->addr))) && (nlent->pflag != NL_DUMMY)) {
			Syslog('+', "Binkp: remote is a listed system");
			UserCity(mypid, nlent->sysop, nlent->location);
			break;
		    }
		}
		if (nlent)
		    rdoptions(Loaded);

//		if (bp.MBflag == TheyWant) {
//		    Syslog('b', "Binkp: remote supports MB");
//		    binkp_send_control(MM_NUL,"OPT MB");
//		    bp.MBflag = Active;
//		}
		if (bp.CRCflag == TheyWant) {
		    if (Loaded && nodes.CRC32 && !CFG.NoCRC32) {
			binkp_send_control(MM_NUL,"OPT CRC");
			Syslog('+', "Binkp: using file transfers with CRC32 checking");
			bp.CRCflag = Active;
		    } else {
			Syslog('b', "Binkp: CRC32 support is diabled here");
			bp.CRCflag = No;
		    }
		}

		history.aka.zone  = remote->addr->zone;
		history.aka.net   = remote->addr->net;
		history.aka.node  = remote->addr->node;
		history.aka.point = remote->addr->point;
		sprintf(history.aka.domain, "%s", remote->addr->domain);

		SM_PROCEED(IsPasswd)

	    } else if (rbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(&rbuf[1], 0));
		SM_ERROR;

	    } else if (rbuf[0] == MM_NUL) {
		b_nul(&rbuf[1]);
	    } else if (rbuf[0] <= MM_MAX) {
		binkp_send_control(MM_ERR, "Unexpected frame");
		SM_ERROR;
	    }
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
	if ((rc = binkp_recv_frame(rbuf, &bufl, &cmd))) {
	    Syslog('!', "Binkp: error waiting for password");
	    SM_ERROR;
	}

        if (cmd) {
	    if (rbuf[0] == MM_PWD) {
		SM_PROCEED(PwdAck)

	    } else if (rbuf[0] == MM_ERR) {
		Syslog('!', "Binkp: M_ERR \"%s\"", printable(&rbuf[1], 0));
                SM_ERROR;
	    } else if (rbuf[0] == MM_NUL) {
                b_nul(&rbuf[1]);
	    } else if (rbuf[0] <= MM_MAX) {
		binkp_send_control(MM_ERR, "Unexpected frame");
		SM_ERROR;
	    }
	}
    }

SM_STATE(PwdAck)

    if (we_have_pwd) {
	pw = xstrcpy(nodes.Spasswd);
    } else {
	pw = xstrcpy((char *)"-");
    }

    if ((strncmp(&rbuf[1], "CRAM-", 5) == 0) && CRAMflag) {
	char	*sp;
	sp = MD_buildDigest(pw, MD_challenge);
	if (sp != NULL) {
	    if (strcmp(&rbuf[1], sp)) {
		Syslog('+', "Binkp: bad MD5 crypted password");
		binkp_send_control(MM_ERR, "Bad password");
		free(sp);
		sp = NULL;
		free(pw);
		SM_ERROR;
	    } else {
		free(sp);
		sp = NULL;
		if (we_have_pwd)
		    Secure = TRUE;
	    }
	} else {
	    free(pw);
	    Syslog('!', "Binkp: could not build MD5 digest");
	    binkp_send_control(MM_ERR, "*** Internal error ***");
	    SM_ERROR;
	}
    } else if ((strcmp(&rbuf[1], pw) == 0)) {
	if (we_have_pwd)
	    Secure = TRUE;
    } else {
	free(pw);
	Syslog('?', "Binkp: password error: expected \"%s\", got \"%s\"", nodes.Spasswd, &rbuf[1]);
	binkp_send_control(MM_ERR, "Bad password");
	SM_ERROR;
    }

    free(pw);
    Syslog('+', "Binkp: %s%sprotected session", CRAMflag ? "MD5 ":"", Secure ? "":"un");
    inbound_open(remote->addr, Secure);
    binkp_send_control(MM_OK, "%ssecure", Secure ? "":"non-");
    SM_PROCEED(Opts)

SM_STATE(Opts)

    SM_SUCCESS;

SM_END
SM_RETURN



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
    (*tmpl)->remote = xstrcpy(unix2binkp(fal->remote));
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



void debug_binkp_list(binkp_list **bll)
{
    binkp_list	*tmpl;

    Syslog('B', "Current filelist:");

    for (tmpl = *bll; tmpl; tmpl = tmpl->next)
	Syslog('B', "%s %s %s %ld", MBSE_SS(tmpl->local), MBSE_SS(tmpl->remote), lbstat[tmpl->state], tmpl->offset);
}



int binkp_batch(file_list *to_send)
{
    int		    rc = 0, NotDone, rxlen = 0, txlen = 0, rxerror = FALSE;
    static char	    *txbuf, *rxbuf;
    FILE	    *txfp = NULL, *rxfp = NULL;
    long	    txpos = 0, rxpos = 0, stxpos = 0, written;
    int		    sverr, cmd = FALSE, GotFrame = FALSE, blklen = 0, c, Found = FALSE;
    unsigned short  header = 0;
    unsigned long   rxcrc = 0;
    off_t	    rxbytes;
    binkp_list	    *bll = NULL, *tmp, *tmpg, *cursend = NULL;
    file_list	    *tsl;
    struct timeval  rxtvstart, rxtvend;
    struct timeval  txtvstart, txtvend;
    struct timezone tz;
    struct statfs   sfs;

    rxtvstart.tv_sec = rxtvstart.tv_usec = 0;
    rxtvend.tv_sec   = rxtvend.tv_usec   = 0;
    txtvstart.tv_sec = txtvstart.tv_usec = 0;
    txtvend.tv_sec   = txtvend.tv_usec   = 0;
    tz.tz_minuteswest = tz.tz_dsttime = 0;
    bp.rcrc = 0;
    bp.tcrc = 0;

    batchnr++;
    Syslog('+', "Binkp: starting batch %d", batchnr);
    IsDoing("Binkp %s %s", (bp.role == 1)?"out":"inb", ascfnode(remote->addr, 0xf));
    txbuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
    rxbuf = calloc(MAX_BLKSIZE + 3, sizeof(unsigned char));
//    TfState = Switch;
    bp.RxState = RxWaitFile;
    bp.TxState = TxGetNextFile;
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

    while ((bp.RxState != RxDone) || (bp.TxState != TxDone)) {

	Nopper();
	if (binkp_expired()) {
	    Syslog('!', "Binkp: Transfer timeout");
	    Syslog('b', "Binkp: TxState=%s, RxState=%s, rxlen=%d", txstate[bp.TxState], rxstate[bp.RxState], rxlen);
	    bp.RxState = RxDone;
	    bp.TxState = TxDone;
	    binkp_send_control(MM_ERR, "Transfer timeout");
	    rc = MBERR_FTRANSFER;
	    break;
	}

	/*
	 * Receiver binkp frame
	 */
	for (;;) {
	    if (GotFrame) {
		Syslog('b', "Binkp: WARNING: frame not processed");
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
		    bp.TxState = TxDone;
		    bp.RxState = RxDone;
		    rc = (MBERR_TTYIO + (-c));
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
	switch (bp.TxState) {
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

		    if (bp.CRCflag == Active)
			bp.tcrc = file_crc(tmp->local, FALSE);
		    else
			bp.tcrc = 0;

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
		    if ((bp.CRCflag == Active) && bp.tcrc) {
			Syslog('+', "Binkp: size %lu bytes, dated %s, crc %lx", (unsigned long)tmp->size, date(tmp->date), bp.tcrc);
			binkp_send_control(MM_FILE, "%s %lu %ld %ld %lx", MBSE_SS(tmp->remote),
			    (unsigned long)tmp->size, (long)tmp->date, (unsigned long)tmp->offset, bp.tcrc);
		    } else {
			Syslog('+', "Binkp: size %lu bytes, dated %s", (unsigned long)tmp->size, date(tmp->date));
			binkp_send_control(MM_FILE, "%s %lu %ld %ld", MBSE_SS(tmp->remote), 
			    (unsigned long)tmp->size, (long)tmp->date, (unsigned long)tmp->offset);
		    }
		    gettimeofday(&txtvstart, &tz);
		    tmp->state = Sending;
		    cursend = tmp;
		    bp.TxState = TxTryRead;
		    transferred = TRUE;
		    break;
		} /* if state == NoState */
	    } /* for */

	    if (tmp == NULL) {
		/*
		 * No more files to send
		 */
		bp.TxState = TxWaitLastAck;
		Syslog('b', "Binkp: transmitter to WaitLastAck");
	    }
	    break;

	case TxTryRead:
	    /*
	     * Check if there is room in the output buffer
	     */
	    if ((WAITPUTGET(-1) & 2) != 0)
		bp.TxState = TxReadSend;
	    break;

	case TxReadSend:
	    fseek(txfp, txpos, SEEK_SET);
	    txlen = fread(txbuf, 1, SND_BLKSIZE, txfp);

	    if (txlen == 0) {

		if (ferror(txfp)) {
		    WriteError("$Binkp: error reading from file");
		    bp.TxState = TxGetNextFile;
		    cursend->state = Skipped;
		    debug_binkp_list(&bll);
		    break;
		}

		/*
		 * Send empty dataframe, most binkp mailers need it to detect EOF.
		 */
//		binkp_send_data(txbuf, 0);

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
		    Syslog('+', "Binkp: transmitter skipped file after %ld seconds", txtvend.tv_sec - txtvstart.tv_sec);
		}

		cursend->state = IsSent;
		bp.TxState = TxGetNextFile;
		break;
	    } else {
		txpos += txlen;
		sentbytes += txlen;
		binkp_send_data(txbuf, txlen);
	    }

	    bp.TxState = TxTryRead;
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
		bp.TxState = TxDone;
		binkp_send_control(MM_EOB, "");
		Syslog('b', "Binkp: sending EOB");
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
				bp.RxState = RxDone;
				bp.TxState = TxDone;
				rc = MBERR_FTRANSFER;
				break;

		case MM_BSY:	Syslog('+', "Binkp: got BSY: %s", rxbuf+1);
				bp.RxState = RxDone;
				bp.TxState = TxDone;
				rc = MBERR_FTRANSFER;
				break;

		case MM_SKIP:   Syslog('+', "Binkp: got SKIP: %s", rxbuf+1);
				break;

		case MM_GET:    Syslog('+', "Binkp: got GET: %s", rxbuf+1);
				sscanf(rxbuf+1, "%s %ld %ld %ld", bp.gname, &bp.gsize, &bp.gtime, &bp.goffset);
				for (tmpg = bll; tmpg; tmpg = tmpg->next) {
				    if (strcasecmp(tmpg->remote, bp.gname) == 0) {
					tmpg->state = NoState;
					tmpg->offset = bp.goffset;
					if (bp.goffset) {
					    Syslog('+', "Remote wants %s for resync at offset %ld", bp.gname, bp.goffset);
					} else {
					    Syslog('+', "Remote wants %s again", bp.gname);
					}
					bp.TxState = TxGetNextFile;
				    }
				}
				break;

		case MM_GOT:    Syslog('+', "Binkp: got GOT: %s", rxbuf+1);
				sscanf(rxbuf+1, "%s %ld %ld", bp.lname, &bp.lsize, &bp.ltime);
				Found = FALSE;
				for (tmp = bll; tmp; tmp = tmp->next)
				    if ((strcmp(bp.lname, tmp->remote) == 0) && (bp.lsize == tmp->size) && 
					    (bp.ltime == tmp->date)) {
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

		case MM_EOB:    Syslog('+', "Binkp: got EOB");
				bp.RxState = RxEndOfBatch;
				break;

		case MM_FILE:   Syslog('b', "Binkp: got FILE: %s", rxbuf+1);
				if ((bp.RxState == RxWaitFile) || (bp.RxState == RxEndOfBatch)) {
				    bp.RxState = RxAcceptFile;
				    if (strlen(rxbuf) < 512) {
					/*
					 * Check against buffer overflow
					 */
					bp.rcrc = 0;
					if (bp.CRCflag == Active) {
					    sscanf(rxbuf+1, "%s %ld %ld %ld %lx", bp.rname, &bp.rsize, &bp.rtime, &bp.roffs, &bp.rcrc);
					} else {
					    sscanf(rxbuf+1, "%s %ld %ld %ld", bp.rname, &bp.rsize, &bp.rtime, &bp.roffs);
					}
				    } else {
					Syslog('+', "Binkp: got corrupted FILE frame, size %d bytes", strlen(rxbuf));
				    }
				} else {
				    Syslog('+', "Binkp: got unexpected FILE frame %s", rxbuf+1);
				}
				break;

		default:        Syslog('+', "Binkp: Unexpected frame %d \"%s\"", rxbuf[0], printable(rxbuf+1, 0));
		}
	    } else {
		if (blklen) {
		    if (bp.RxState == RxReceData) {
			written = fwrite(rxbuf, 1, blklen, rxfp);
			if (bp.CRCflag == Active)
			    rxcrc = upd_crc32(rxbuf, rxcrc, blklen);
			if (!written && blklen) {
			    Syslog('+', "Binkp: file write error");
			    bp.RxState = RxDone;
			}
			rxpos += written;
			if (rxpos == bp.rsize) {
			    bp.RxState = RxWaitFile;
			    if ((bp.CRCflag == Active) && bp.rcrc) {
				rxcrc = rxcrc ^ 0xffffffff;
				if (bp.rcrc == rxcrc) {
				    binkp_send_control(MM_GOT, "%s %ld %ld %lx", bp.rname, bp.rsize, bp.rtime, bp.rcrc);
				    closefile();
				} else {
				    rxerror = TRUE;
				    crc_errors++;
				    binkp_send_control(MM_SKIP, "%s %ld %ld %lx", bp.rname, bp.rsize, bp.rtime, bp.rcrc);
				    Syslog('+', "Binkp: file CRC error nr %d, sending SKIP frame", crc_errors);
				    if (crc_errors >= 3) {
					WriteError("Binkp: file CRC error nr %d, aborting session", crc_errors);
					binkp_send_control(MM_ERR, "Too much CRC errors, aborting session");
					bp.RxState = RxDone;
					rc = MBERR_FTRANSFER;
				    }
				    closefile();
				}
			    } else {
				/*
				 * ACK without CRC check
				 */
				binkp_send_control(MM_GOT, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
				closefile();
			    }
			    rxpos = rxpos - rxbytes;
			    gettimeofday(&rxtvend, &tz);
			    Syslog('+', "Binkp: %s %s", rxerror?"ERROR":"OK", transfertime(rxtvstart, rxtvend, rxpos, FALSE));
			    rcvdbytes += rxpos;
			    bp.RxState = RxWaitFile;
			    transferred = TRUE;
			}
		    } else {
			if (!bp.DidSendGET) {
			    /*
			     * Do not log after a GET command, there will be data packets
			     * in the pipeline that must be ignored.
			     */
			    Syslog('+', "Binkp: unexpected DATA frame %d", rxbuf[0]);
			}
		    }
		}
	    }
	    GotFrame = FALSE;
	    rxlen = 0;
	    header = 0;
	    blklen = 0;
	}

	/*
	 * Receiver state machine
	 */
	switch (bp.RxState) {
	case RxWaitFile:
	    break;

	case RxAcceptFile:
	    if (bp.CRCflag == Active)
		Syslog('+', "Binkp: receive file \"%s\" date %s size %ld offset %ld crc %lx", 
			bp.rname, date(bp.rtime), bp.rsize, bp.roffs, bp.rcrc);
	    else
		Syslog('+', "Binkp: receive file \"%s\" date %s size %ld offset %ld", 
			bp.rname, date(bp.rtime), bp.rsize, bp.roffs);
	    (void)binkp2unix(bp.rname);
	    rxfp = openfile(binkp2unix(bp.rname), bp.rtime, bp.rsize, &rxbytes, resync);

	    if (bp.DidSendGET) {
		/*
		 * The file was partly received, via the openfile the resync function
		 * has send a GET command to start this file with a offset. This means
		 * we will get a new FILE command to open this file with a offset.
		 */
		bp.RxState = RxWaitFile;
		break;
	    }

	    gettimeofday(&rxtvstart, &tz);
	    rxpos = bp.roffs;

	    /*
	     * FIXME: if we have a rxpos, we are appending a partial received file.
	     * We now need to know the CRC of the already received part!
	     * Note, file is open in a+ mode, so we can read the already received
	     * part and calculate the crc. For now, don't use CRC32 mode.
	     */
	    rxcrc = 0xffffffff;
	    rxerror = FALSE;

	    if (!diskfree(CFG.freespace)) {
		Syslog('+', "Binkp: low diskspace, sending BSY");
		binkp_send_control(MM_BSY, "Low diskspace, try again later");
		bp.RxState = RxDone;
		bp.TxState = TxDone;
		rc = MBERR_FTRANSFER;
		break;
	    }

	    if (statfs(tempinbound, &sfs) == 0) {
		Syslog('b', "blocksize %lu free blocks %lu", sfs.f_bsize, sfs.f_bfree);
		Syslog('b', "need %lu blocks", (unsigned long)(bp.rsize / (sfs.f_bsize + 1)));
		if ((bp.rsize / (sfs.f_bsize + 1)) >= sfs.f_bfree) {
		    Syslog('!', "Only %lu blocks free (need %lu) in %s", sfs.f_bfree, 
			    (unsigned long)(bp.rsize / (sfs.f_bsize + 1)), tempinbound);
		    closefile();
		    rxfp = NULL; /* Force SKIP command	*/
		}
	    }

	    if (bp.rsize == rxbytes) {
		/*
		 * We already got this file, send GOT so it will
		 * be deleted at the remote.
		 */
		Syslog('+', "Binkp: already got %s, sending GOT", bp.rname);
		if ((bp.CRCflag == Active) && bp.rcrc)
		    binkp_send_control(MM_GOT, "%s %ld %ld %lx", bp.rname, bp.rsize, bp.rtime, bp.rcrc);
		else
		    binkp_send_control(MM_GOT, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
		bp.RxState = RxWaitFile;
		rxfp = NULL;
	    } else if (!rxfp) {
		/*
		 * Some error, request to skip it
		 */
		Syslog('+', "Binkp: error file %s, sending SKIP", bp.rname);
		if ((bp.CRCflag == Active) && bp.rcrc)
		    binkp_send_control(MM_SKIP, "%s %ld %ld %lx", bp.rname, bp.rsize, bp.rtime, bp.rcrc);
		else
		    binkp_send_control(MM_SKIP, "%s %ld %ld", bp.rname, bp.rsize, bp.rtime);
		bp.RxState = RxWaitFile;
	    } else {
		Syslog('b', "rsize=%d, rxbytes=%d, roffs=%d", bp.rsize, rxbytes, bp.roffs);
		bp.RxState = RxReceData;
	    }
	    break;

	case RxReceData:
	    break;

	case RxEndOfBatch:
	    if (bp.TxState == TxDone)
		bp.RxState = RxDone;
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

    free(txbuf);
    free(rxbuf);

    /*
     * If there was an error, try to close a possible incomplete file in
     * the temp inbound so we can resume the next time we have a session
     * with this node.
     */
    if (rc)
	closefile();

    Syslog('+', "Binkp: batch %d completed rc=%d", batchnr, rc);
    return rc;
}


