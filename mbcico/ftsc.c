/*****************************************************************************
 *
 * $Id: ftsc.c,v 1.22 2007/04/30 19:04:17 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "../lib/nodelist.h"
#include "session.h"
#include "ttyio.h"
#include "statetbl.h"
#include "config.h"
#include "ftsc.h"
#include "rdoptions.h"
#include "recvbark.h"
#include "filelist.h"
#include "sendbark.h"
#include "respfreq.h"
#include "xmrecv.h"
#include "xmsend.h"
#include "inbound.h"


extern int		master;
extern int		made_request;
static int		rxftsc(void);
static int		txftsc(void);
static int		recvfiles(void);
static file_list	*tosend;
extern int		Loaded;
extern pid_t		mypid;
extern char		*tempinbound;
extern int		session_state;


int rx_ftsc(void)
{
    int	rc;

    IsDoing("FTS-0001 inbound");

    session_flags |= SESSION_BARK;
    if ((rc = rxftsc())) {
	WriteError("Session failed: rc=%d",rc);
	PUTCHAR(CAN);
	PUTCHAR(CAN);
	PUTCHAR(CAN);
    } else 
	Syslog('+', "FTS-0001 session completed");

    tidy_filelist(tosend, (rc == 0));
    tosend = NULL;
    if (rc)
	return MBERR_FTSC;
    else
	return 0;
}




int tx_ftsc(void)
{
    int	rc;

    IsDoing("FTS-0001 to %s", ascfnode(remote->addr, 0x0f));

    if ((rc = txftsc())) {
	WriteError("Session failed: rc=%d",rc);
	PUTCHAR(CAN);
	PUTCHAR(CAN);
	PUTCHAR(CAN);
    } else 
	Syslog('+', "FTS-0001 session completed");

    tidy_filelist(tosend, (rc == 0));
    tosend = NULL;
    if (rc)
	return MBERR_FTSC;
    else
	return 0;
}



SM_DECL(txftsc,(char *)"txftsc")
SM_STATES
    wait_command,
    recv_mail,
    send_req,
    recv_req
SM_NAMES
    (char *)"wait_command",
    (char *)"recv_mail",
    (char *)"send_req",
    (char *)"recv_req"
SM_EDECL
    int	    c,rc, mailsent = FALSE, mailrcvd = FALSE;
    char    *nonhold_mail;

    nonhold_mail = (char *)ALL_MAIL;
    tosend = create_filelist(remote,nonhold_mail,2);

    if ((rc = xmsndfiles(tosend))) 
	return rc;
    mailsent = TRUE;

SM_START(wait_command)

SM_STATE(wait_command)

    c = GETCHAR(30);
    if (c == TIMEOUT) {
	Syslog('+', "timeout waiting for remote action, try receive");
	SM_PROCEED(recv_mail);
    } else if (c < 0) {
	if (mailrcvd && mailsent) {
	    /*
	     * Some systems hangup after sending mail, so if we did
	     * send and receive mail we consider the session OK.
	     */
	    Syslog('+', "Lost carrier, FTS-0001 session looks complete");
		SM_SUCCESS;
	} else {
	    Syslog('+', "got error waiting for TSYNC: received %d",c);
	    SM_ERROR;
	}
    } else switch (c) {
	case TSYNC: SM_PROCEED(recv_mail);
		    break;
	case SYN:   SM_PROCEED(recv_req);
		    break;
	case ENQ:   SM_PROCEED(send_req);
		    break;
	case 'C':
	case NAK:   PUTCHAR(EOT);
		    SM_PROCEED(wait_command);
		    break;
	case CAN:   SM_SUCCESS;  /* this is not in BT */
		    break;
	default:    Syslog('s', "got '%s' waiting command", printablec(c));
		    PUTCHAR(SUB);
		    SM_PROCEED(wait_command);
		    break;
    }

SM_STATE(recv_mail)

    if (recvfiles()) {
	SM_ERROR;
    } else {
	mailrcvd = TRUE;
	SM_PROCEED(wait_command);
    }

SM_STATE(send_req)

    if (sendbark()) {
	SM_ERROR;
    } else {
	SM_SUCCESS;
    }

SM_STATE(recv_req)

    if (recvbark()) {
    	SM_ERROR;
    } else {
	SM_PROCEED(wait_command);
    }

SM_END
SM_RETURN



SM_DECL(rxftsc,(char *)"rxftsc")
SM_STATES
    recv_mail,
    send_mail,
    send_req,
    recv_req
SM_NAMES
    (char *)"recv_mail",
    (char *)"send_mail",
    (char *)"send_req",
    (char *)"recv_req"
SM_EDECL
    int		c, count = 0, didwazoo = FALSE, sentmail = FALSE, rcvdmail = FALSE;
    file_list	*request = NULL, *tmpfl;

SM_START(recv_mail)

SM_STATE(recv_mail)

    if (recvfiles()) {
	SM_ERROR;
    } else {
	rcvdmail = TRUE;
	SM_PROCEED(send_mail);
    }

SM_STATE(send_mail)

    Syslog('x', "rxftsc SEND_MAIL count=%d", count);
    if (count++ > 45) {
	SM_ERROR;
    }

    /*
     * If we got a wazoo request, add files now.
     */
    request = respond_wazoo();
    if (request != NULL) {
	didwazoo = TRUE;
	tmpfl = tosend;
	tosend = request;
	for (; request->next; request = request->next);
	    request->next = tmpfl;

	request = NULL;
    }

    if (tosend == NULL) {
	count = 0;
	SM_PROCEED(send_req);
    }

    PUTCHAR(TSYNC);
    c = GETCHAR(1);
    Syslog('x', "Got char 0x%02x", c);
    if (c == TIMEOUT) {
	Syslog('x', "  timeout");
	SM_PROCEED(send_mail);
    } else if (c < 0) {
	Syslog('+', "got error waiting for NAK: received %d",c);
	SM_ERROR;
    } else switch (c) {
	case 'C':
	case NAK:   if (xmsndfiles(tosend)) {
			SM_ERROR;
		    } else {
			sentmail = TRUE;
			count = 0;
			SM_PROCEED(send_req);
		    }
		    break;
	case CAN:   Syslog('+', "Remote refused to pickup mail");
		    SM_SUCCESS;
		    break;
	case EOT:   PUTCHAR(ACK);
		    SM_PROCEED(send_mail);
		    break;
	default:    Syslog('s', "Got '%s' waiting NAK", printablec(c));
		    SM_PROCEED(send_mail);
		    break;
    }

SM_STATE(send_req)

    Syslog('x', "rxftsc SEND_REQ count=%d", count);

    if (didwazoo) {
	session_state = STATE_UNSECURE;
	SM_SUCCESS;
    }

    if (count > 15) {
	SM_ERROR;
    }

    if (!made_request) {
	SM_PROCEED(recv_req);
    }

    PUTCHAR(SYN);
    c = GETCHAR(5);
    Syslog('x', "Got char 0x%02x", c);
    count++;
    if (c == TIMEOUT) {
	Syslog('x', "  timeout");
	SM_PROCEED(send_req);
    } else if (c < 0) {
	Syslog('+', "got error waiting for ENQ: received %d",c);
	SM_ERROR;
    } else switch (c) {
	case ENQ:   if (sendbark()) {
			SM_ERROR;
		    } else {
			SM_PROCEED(recv_req);
		    }
		    break;
	case CAN:   Syslog('+', "Remote refused to accept request");
		    SM_PROCEED(recv_req);
		    break;
	case 'C':
	case NAK:   PUTCHAR(EOT);
		    SM_PROCEED(send_req);
		    break;
	case SUB:   SM_PROCEED(send_req);
		    break;
	default:    Syslog('s', "got '%s' waiting ENQ", printablec(c));
		    SM_PROCEED(send_req);
		    break;
    }

SM_STATE(recv_req)

    if (recvbark()) {
	if (sentmail && rcvdmail) {
	    Syslog('+', "Consider session OK");
	    session_state = STATE_SECURE;
	    SM_SUCCESS;
	} else {
	    SM_ERROR;
	}
    } else {
	session_state = STATE_SECURE;
	SM_SUCCESS;
    }

SM_END
SM_RETURN



SM_DECL(recvfiles,(char *)"recvfiles")
SM_STATES
    recv_packet,
    scan_packet,
    recv_file
SM_NAMES
    (char *)"recv_packet",
    (char *)"scan_packet",
    (char *)"recv_file"
SM_EDECL
    int	    rc = 0, ghc = 0;
    char    recvpktname[16];
    char    *fpath, *tpath;
    FILE    *fp;
    faddr   f, t;
    fa_list **tmpl;

SM_START(recv_packet)
    Loaded = FALSE;

SM_STATE(recv_packet)

    snprintf(recvpktname,16, "%08x.pkt", sequencer());
    if ((rc = xmrecv(recvpktname)) == 1) {
	SM_SUCCESS;
    } else if (rc == 0) {
	if (master) {
	    SM_PROCEED(recv_file);
	} else {
	    SM_PROCEED(scan_packet);
	}
    } else {
	SM_ERROR;
    }

SM_STATE(scan_packet)

    /*
     * We cannot use the temp inbound per node yet, FTS-0001 does it's
     * handshake by sending us a .pkt file, we store this in the old
     * style ../tmp/ dir in the unprotected inbound.
     */
    fpath = xstrcpy(CFG.inbound);
    fpath = xstrcat(fpath,(char *)"/tmp/");
    fpath = xstrcat(fpath,recvpktname);
    mkdirs(fpath, 0700);
    fp = fopen(fpath,"r");
    if (fp == NULL) {
	WriteError("$cannot open received packet");
	free(fpath);
	SM_ERROR;
    }
    switch ((ghc = getheader(&f , &t, fp, recvpktname, TRUE))) {
	case 3:	Syslog('+', "remote mistook us for %s",ascfnode(&t,0x1f));
		fclose(fp);
		Syslog('s', "Unlink %s rc=%d", fpath, unlink(fpath));
		free(fpath);
		SM_ERROR;
	case 0:	
	case 5:	fclose(fp);
		if (nodelock(&f, mypid)) {
		    /* 
		     * Lock failed, FTSC-0001 allowes only one aka, so abort session.
		     */
		    Syslog('+', "address : %s is locked, abort", ascfnode(&f, 0x1f));
		    Syslog('s', "Unlink %s rc=%d", fpath, unlink(fpath));
		    free(fpath);
		    SM_ERROR;
		}

		Syslog('+', "accepting session");
		for (tmpl = &remote; *tmpl; tmpl = &((*tmpl)->next));
		(*tmpl)=(fa_list*)malloc(sizeof(fa_list));
		(*tmpl)->next=NULL;
		(*tmpl)->addr=(faddr*)malloc(sizeof(faddr));
		(*tmpl)->addr->zone=f.zone;
		(*tmpl)->addr->net=f.net;
		(*tmpl)->addr->node=f.node;
		(*tmpl)->addr->point=f.point;
		(*tmpl)->addr->name=NULL;
		(*tmpl)->addr->domain=NULL;
		Syslog('+', "address : %s",ascfnode((*tmpl)->addr,0x1f));

		/*
		 * With the loaded flag we prevent removing the noderecord
		 * when the remote presents us an address we don't know about.
		 */
		if (!Loaded) {
		    if (noderecord((*tmpl)->addr))
			Loaded = TRUE;
		}

		history.aka.zone  = remote->addr->zone;
		history.aka.net   = remote->addr->net;
		history.aka.node  = remote->addr->node;
		history.aka.point = remote->addr->point;
		if (remote->addr->domain && strlen(remote->addr->domain))
		    snprintf(history.aka.domain, 13, "%s", printable(remote->addr->domain, 0));
	
		if (((nlent=getnlent(remote->addr))) && (nlent->pflag != NL_DUMMY)) {
		    Syslog('+', "remote is a listed system");
		    strncpy(history.system_name, nlent->name, 35);
		    strncpy(history.location, nlent->location, 35);
		    strncpy(history.sysop, nlent->sysop, 35);
		    UserCity(mypid, nlent->sysop, nlent->location);
		} else {
		    snprintf(history.system_name, 36, "Unknown");
		    snprintf(history.location, 36, "Somewhere");
		}

		if (nlent) 
		    rdoptions(Loaded);

		if (ghc == 0) {
		    Syslog('+', "Password correct, protected FTS-0001 session");
		    inbound_open(remote->addr, TRUE, FALSE);
		    session_state = STATE_SECURE;
		} else {
		    Syslog('+', "Unsecure FTS-0001 session");
		    inbound_open(remote->addr, FALSE, FALSE);
		    session_state = STATE_UNSECURE;
		}
		/*
		 * Move the packet to the temp inbound so the we can later
		 * move it to the final inbound.
		 */
		tpath = xstrcpy(tempinbound);
		tpath = xstrcat(tpath,(char *)"/");
		tpath = xstrcat(tpath,recvpktname);

		Syslog('s', "Move %s to %s rc=%d", fpath, tpath, file_mv(fpath, tpath));

		free(fpath);
		free(tpath);

		tosend = create_filelist(remote,(char *)ALL_MAIL,1);
		if (rc == 0) {
		    SM_PROCEED(recv_file);
		} else {
		    SM_SUCCESS;
		}
		
	default: 
		Syslog('+', "received bad packet apparently from",ascfnode(&f,0x1f));
		fclose(fp);
		Syslog('s', "Unlink %s rc=%d", fpath, unlink(fpath));
		free(fpath);
		SM_ERROR;
    }

SM_STATE(recv_file)

    switch (xmrecv(NULL)) {
	case 0:	    SM_PROCEED(recv_file); 
		    break;
	case 1:	    SM_SUCCESS; 
		    break;
	default:    SM_ERROR; 
		    break;
    }

SM_END
SM_RETURN

