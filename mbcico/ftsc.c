/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/nodelist.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
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



extern int		master;
extern int		made_request;
static int		rxftsc(void);
static int		txftsc(void);
static int		recvfiles(void);
static file_list	*tosend;
extern int		Loaded;
extern pid_t		mypid;



int rx_ftsc(void)
{
    int	rc;

    Syslog('+', "Start inbound FTS-0001 session");
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

    Syslog('+', "Start outbound FTS-0001 session with %s", ascfnode(remote->addr,0x1f));
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
	int	c,rc;
	char	*nonhold_mail;
	int	mailsent = FALSE, mailrcvd = FALSE;

//	if (localoptions & NOHOLD) 
		nonhold_mail = (char *)ALL_MAIL;
//	else 
//		nonhold_mail = (char *)NONHOLD_MAIL;
	tosend = create_filelist(remote,nonhold_mail,2);

	Syslog('s', "txftsc SEND_MAIL");
	if ((rc = xmsndfiles(tosend))) 
		return rc;
	mailsent = TRUE;

SM_START(wait_command)

SM_STATE(wait_command)

	Syslog('s', "txftsc WAIT_COMMAND");
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
			Syslog('+', "Lost carrier, FTSC session looks complete");
			SM_SUCCESS;
		} else {
			Syslog('+', "got error waiting for TSYNC: received %d",c);
			SM_ERROR;
		}
	} else switch (c) {
	 	case TSYNC:	SM_PROCEED(recv_mail);
				break;
		case SYN:	SM_PROCEED(recv_req);
				break;
		case ENQ:	SM_PROCEED(send_req);
				break;
		case 'C':
		case NAK:	PUTCHAR(EOT);
				SM_PROCEED(wait_command);
				break;
		case CAN:	SM_SUCCESS;  /* this is not in BT */
				break;
		default:	Syslog('s', "got '%s' waiting command", printablec(c));
				PUTCHAR(SUB);
				SM_PROCEED(wait_command);
				break;
	}

SM_STATE(recv_mail)

	Syslog('s', "txftsc RECV_MAIL");
	if (recvfiles()) {
		SM_ERROR;
	} else {
		mailrcvd = TRUE;
		SM_PROCEED(wait_command);
	}

SM_STATE(send_req)

	Syslog('s', "txftsc SEND_BARK");
	if (sendbark()) {
		SM_ERROR;
	} else {
		SM_SUCCESS;
	}

SM_STATE(recv_req)

	Syslog('s', "txftsc RECV_BARK");
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
	int		c, count = 0, didwazoo = FALSE;
	int		sentmail = FALSE, rcvdmail = FALSE;
	file_list	*request = NULL, *tmpfl;

SM_START(recv_mail)

SM_STATE(recv_mail)

	Syslog('s', "rxftsc RECV_MAIL");
	if (recvfiles()) {
		SM_ERROR;
	} else {
		rcvdmail = TRUE;
		SM_PROCEED(send_mail);
	}

SM_STATE(send_mail)

	Syslog('s', "rxftsc SEND_MAIL count=%d", count);
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
		case NAK:	if (xmsndfiles(tosend)) {
					SM_ERROR;
				} else {
					sentmail = TRUE;
					count = 0;
					SM_PROCEED(send_req);
				}
				break;
		case CAN:	Syslog('+', "Remote refused to pickup mail");
				SM_SUCCESS;
				break;
		case EOT:	PUTCHAR(ACK);
				SM_PROCEED(send_mail);
				break;
		default:	Syslog('s', "Got '%s' waiting NAK", printablec(c));
				SM_PROCEED(send_mail);
				break;
	}

SM_STATE(send_req)

	Syslog('s', "rxftsc SEND_REQ count=%d", count);

	if (didwazoo) {
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
		case ENQ:	if (sendbark()) {
					SM_ERROR;
				} else {
					SM_PROCEED(recv_req);
				}
				break;
		case CAN:	Syslog('+', "Remote refused to accept request");
				SM_PROCEED(recv_req);
				break;
		case 'C':
		case NAK:	PUTCHAR(EOT);
				SM_PROCEED(send_req);
				break;
		case SUB:	SM_PROCEED(send_req);
				break;
		default:	Syslog('s', "got '%s' waiting ENQ", printablec(c));
				SM_PROCEED(send_req);
				break;
	}

SM_STATE(recv_req)

	Syslog('s', "rxftsc RECV_REQ");
	if (recvbark()) {
		if (sentmail && rcvdmail) {
			Syslog('+', "Consider session OK");
			SM_SUCCESS;
		} else {
			SM_ERROR;
		}
	} else {
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
    int	    rc=0;
    char    recvpktname[16];
    char    *fpath;
    FILE    *fp;
    faddr   f, t;
    fa_list **tmpl;

SM_START(recv_packet)
    Loaded = FALSE;

SM_STATE(recv_packet)

    sprintf(recvpktname,"%08lx.pkt",(unsigned long)sequencer());
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

    fpath = xstrcpy(inbound);
    fpath = xstrcat(fpath,(char *)"/");
    fpath = xstrcat(fpath,recvpktname);
    fp = fopen(fpath,"r");
    free(fpath);
    if (fp == NULL) {
	WriteError("$cannot open received packet");
	SM_ERROR;
    }
    switch (getheader(&f , &t, fp, recvpktname)) {
	case 3:	Syslog('+', "remote mistook us for %s",ascfnode(&t,0x1f));
		fclose(fp);
		SM_ERROR;
	case 0:	Syslog('+', "accepting session");
		fclose(fp);
		for (tmpl=&remote;*tmpl;tmpl=&((*tmpl)->next));
		(*tmpl)=(fa_list*)malloc(sizeof(fa_list));
		(*tmpl)->next=NULL;
		(*tmpl)->addr=(faddr*)malloc(sizeof(faddr));
		(*tmpl)->addr->zone=f.zone;
		(*tmpl)->addr->net=f.net;
		(*tmpl)->addr->node=f.node;
		(*tmpl)->addr->point=f.point;
		(*tmpl)->addr->name=NULL;
		(*tmpl)->addr->domain=NULL;
		for (tmpl=&remote;*tmpl;tmpl=&((*tmpl)->next)) {
		    (void)nodelock((*tmpl)->addr);
		    /* try lock all remotes, ignore locking result */
		    if (!Loaded)
			if (noderecord((*tmpl)->addr))
			    Loaded = TRUE;
		}

		history.aka.zone  = remote->addr->zone;
		history.aka.net   = remote->addr->net;
		history.aka.node  = remote->addr->node;
		history.aka.point = remote->addr->point;
		if (remote->addr->domain && strlen(remote->addr->domain))
		    sprintf(history.aka.domain, "%s", remote->addr->domain);
	
		if (((nlent=getnlent(remote->addr))) && (nlent->pflag != NL_DUMMY)) {
		    Syslog('+', "remote is a listed system");
		    if (inbound)
			free(inbound);
		    inbound = xstrcpy(CFG.inbound);
		    strncpy(history.system_name, nlent->name, 35);
		    strncpy(history.location, nlent->location, 35);
		    strncpy(history.sysop, nlent->sysop, 35);
		    UserCity(mypid, nlent->sysop, nlent->location);
		} else {
		    sprintf(history.system_name, "Unknown");
		    sprintf(history.location, "Somewhere");
		}

		if (nlent) 
		    rdoptions(Loaded);

		/*
		 * It appears that if the remote gave no password, the 
		 * getheader function fills in a password itself. Maybe
		 * that's the reason why E.C did not switch to protected
		 * inbound, because of the failing password check. MB.
		 */
		if (f.name) {
		    Syslog('+', "Password correct, protected FTS-0001 session");
		    if (inbound)
			free(inbound);
		    inbound = xstrcpy(CFG.pinbound);
		}

		tosend = create_filelist(remote,(char *)ALL_MAIL,1);
		if (rc == 0) {
		    SM_PROCEED(recv_file);
		} else {
		    SM_SUCCESS;
		}
		
	default: 
		Syslog('+', "received bad packet apparently from",ascfnode(&f,0x1f));
		fclose(fp);
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

