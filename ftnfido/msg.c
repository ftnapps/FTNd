/*****************************************************************************
 *
 * $Id: msg.c,v 1.16 2008/11/26 22:12:28 mbse Exp $
 * Purpose ...............: Read *.msg messages
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "msgflags.h"
#include "msg.h"


extern int	net_msgs;
extern int	net_in;
extern int	net_imp;
extern int	do_scan;


int toss_onemsg(char *);


int toss_msgs(void)
{
    DIR			*dp;
    struct dirent	*de;
    int			files = 0;

    if (strlen(CFG.msgs_path) == 0) {
	WriteError("No path defined for *.msg mail, check setup 1.4 screen 2, item 10");
	return -1;
    }

    if ((dp = opendir(CFG.msgs_path)) == NULL) {
	WriteError("$Can't opendir %s", CFG.msgs_path);
	return -1;
    }

    Syslog('m', "Process *.msg in %s", CFG.msgs_path);
    IsDoing("Get *.msgs");

    while ((de = readdir(dp))) {
	if ((de->d_name[0] != '.') && (strstr(de->d_name, ".msg"))) {
	    if (IsSema((char *)"upsalarm")) {
		Syslog('+', "Detected upsalarm semafore, aborting toss");
		break;
	    }

	    Syslog('m', "Process %s", de->d_name);
	    toss_onemsg(de->d_name);
	    files++;
	}
    }
    closedir(dp);

    if (files) {
	Syslog('+',"Processed %d msg messages", files);
    }

    return files;
}



/*
 * Toss one message and post into a JAM messagebase. Returns:
 *  0 = Ok
 *  1 = Can't open *.msg
 *  2 = Can't read message
 *  3 = Missing zone info
 *  4 = No ftn network or netmailboard in setup
 *  5 = Can't open JAM area
 *  6 = Empty local netmail, dropped.
 */
int toss_onemsg(char *msgname)
{
    int		    rc = 0, islocal, empty = TRUE;
    char	    *temp, *dospath, *flagstr = NULL, *l, *r, *msgid = NULL; 
    char	    fromUserName[37], toUserName[37], subject[73], DateTime[21];
    FILE	    *fp, *np;
    faddr	    *ta;
    unsigned char   buf[0xbe];
    unsigned short  destNode = 0, destNet = 0, destZone = 0, destPoint = 0;
    unsigned short  origNode = 0, origNet = 0, origZone = 0, origPoint = 0;
    unsigned short  Attribute = 0;
    struct stat	    sb;

    net_msgs++;
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/%s", CFG.msgs_path, msgname);
    
    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return 1;
    }
    
    /*
     * First read message header information we need.
     */
    memset(&fromUserName, 0, sizeof(fromUserName));
    memset(&toUserName, 0, sizeof(toUserName));
    memset(&subject, 0, sizeof(subject));
    memset(&DateTime, 0, sizeof(DateTime));

    if (fread(&buf, 1, 0xbe, fp) != 0xbe) {
	WriteError("$Could not read header (%s)", temp);
	fclose(fp);
	free(temp);
	return 2;
    }
    
    strncpy(fromUserName, (char *)buf, 36);
    strncpy(toUserName, (char *)buf+0x24, 36);
    strncpy(subject, (char *)buf+0x48, 72);
    strncpy(DateTime, (char *)buf+0x90, 20);

    Syslog('m', "From \"%s\"", printable(fromUserName, 0));
    Syslog('m', "To   \"%s\"", printable(toUserName, 0));
    Syslog('m', "Subj \"%s\"", printable(subject, 0));
    Syslog('m', "Date \"%s\"", printable(DateTime, 0));
 
    destNode  = (buf[0xa7] << 8) + buf[0xa6];
    origNode  = (buf[0xa9] << 8) + buf[0xa8];
    origNet   = (buf[0xad] << 8) + buf[0xac];
    destNet   = (buf[0xaf] << 8) + buf[0xae];
    destZone  = (buf[0xb1] << 8) + buf[0xb0];
    origZone  = (buf[0xb3] << 8) + buf[0xb2];
    destPoint = (buf[0xb5] << 8) + buf[0xb4];
    origPoint = (buf[0xb7] << 8) + buf[0xb6];
    Attribute = (buf[0xbb] << 8) + buf[0xba];
 
    Syslog('m', "From %d:%d/%d.%d to %d:%d/%d.%d", origZone, origNet, origNode, origPoint, destZone, destNet, destNode, destPoint);

    while (Fgets(temp, PATH_MAX-1, fp)) {
	Syslog('m', "Line \"%s\"", printable(temp, 0));
	if (!strncmp(temp, "\001MSGID: ", 8)) {
	    msgid = xstrcpy(temp + 8);
	    /*
	     *  Extra test to see if the mail comes from a pointaddress.
	     */
	    l = strtok(temp," \0");
	    l = strtok(NULL," \0");
	    if ((ta = parsefnode(l))) {
		if (ta->net == origNet && ta->node == origNode && !origPoint && ta->point) {
		    Syslog('m', "Setting pointinfo (%d) from MSGID", ta->point);
		    origPoint = ta->point;
		}
		if ((ta->net == origNet) && (ta->node == origNode) && (origZone == 0) && ta->zone) {
		    /*
		     * Missing zone info, maybe later we will see a INTL kludge or so, but for
		     * now, just in case we fix it. And we need that for some Aka collecting
		     * sysop who doesn't know how to configure his system right.
		     */
		    Syslog('m', "No from zone set, setting zone %d from MSGID", ta->zone);
		    origZone = ta->zone;
		    /*
		     * 99.9 % chance that the destination zone is also missing.
		     */
		    if (destZone == 0) {
			destZone = ta->zone;
			Syslog('m', "No dest zone set, setting zone %d from MSGID", ta->zone);
		    }
		}
		tidy_faddr(ta);
	    }
	    if (msgid)
		free(msgid);
	    msgid = NULL;
	}
	if (!strncmp(temp, "\001FMPT", 5)) {
	    l = strtok(temp, " \0");
	    l = strtok(NULL, " \0");
	    origPoint = atoi(l);
	}
	if (!strncmp(temp, "\001TOPT", 5)) {
	    l = strtok(temp, " \0");
	    l = strtok(NULL, " \0");
	    destPoint = atoi(l);
	}
	if (!strncmp(temp, "\001INTL", 5)) {
	    Syslog('m', "Setting addresses from INTL kludge");
	    l = strtok(temp," \0");
	    l = strtok(NULL," \0");
	    r = strtok(NULL," \0");
	    if ((ta = parsefnode(l))) {
		destPoint = ta->point;
		destNode  = ta->node;
		destNet   = ta->net;
		destZone  = ta->zone;
		tidy_faddr(ta);
	    }
	    if ((ta = parsefnode(r))) {
		origPoint = ta->point;
		origNode  = ta->node;
		origNet   = ta->net;
		origZone  = ta->zone;
		tidy_faddr(ta);
	    }
	}
	/*
	 * Check FLAGS kludge
	 */
	if (!strncmp((char *)buf, "\001FLAGS ", 7)) {
	    flagstr = xstrcpy((char *)buf + 7);
	    Syslog('m', "^aFLAGS %s", flagstr);
	}
	if (!strncmp((char *)buf, "\001FLAGS: ", 8)) {
	    flagstr = xstrcpy((char *)buf + 8);
	    Syslog('m', "^aFLAGS: %s", flagstr);
	}
	if (buf[0] != '\0') {
	    if ((buf[0] != '\001') && (strcmp((char *)buf, (char *)"--- ")))
		empty = FALSE;
	}
    }
    Syslog('m', "Mail is %sempty", empty ? "":"not ");

    Syslog('m', "From %d:%d/%d.%d to %d:%d/%d.%d", origZone, origNet, origNode, origPoint, destZone, destNet, destNode, destPoint);
    
    if ((origZone == 0) || (destZone == 0)) {
	WriteError("No zone info in %s/%s", CFG.msgs_path, msgname);
	fclose(fp);
	free(temp);
	return 3;
    }

    if (SearchFidonet(origZone) == FALSE) {
	WriteError("Can't find network for zone %d", origZone);
	fclose(fp);
	free(temp);
	return 4;
    }

    if (SearchNetBoard(origZone, origNet) == FALSE) {
	WriteError("Can't find netmail board for zone:net %d:%d", origZone, origNet);
	fclose(fp);
	free(temp);
	return 4;
    }

    if (Msg_Open(msgs.Base) && Msg_Lock(30L)) {
	Msg_New();
	strcpy(Msg.From, fromUserName);
	strcpy(Msg.To, toUserName);
	strcpy(Msg.Subject, subject);
	if (Attribute & M_FILE) {
	    if ((stat(subject, &sb) == 0) && (S_ISREG(sb.st_mode))) {
		dospath = xstrcpy(Unix2Dos(subject));
		Syslog('+', "Fileattach %s in message %s", subject, msgname);
		if (strlen(CFG.dospath))
		    strcpy(Msg.Subject, dospath);
		Msg.FileAttach = TRUE;
		free(dospath);
	    } else {
		WriteError("Fileattach %s in message %s not found", subject, msgname);
	    }
	}
	Msg.Written = parsefdate(DateTime, NULL);
	Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);

	Msg.KillSent       = ((Attribute & M_KILLSENT)) ? 1:0;
	Msg.Hold           = ((Attribute & M_HOLD)) ? 1:0;
	Msg.Crash          = ((Attribute & M_CRASH)    || flag_on((char *)"CRA", flagstr));
	Msg.ReceiptRequest = ((Attribute & M_RRQ)      || flag_on((char *)"RRQ", flagstr));
	Msg.Orphan         = ((Attribute & M_ORPHAN));
	Msg.Intransit      = ((Attribute & M_TRANSIT));
	Msg.FileRequest    = ((Attribute & M_REQ)      || flag_on((char *)"FRQ", flagstr));
	Msg.ConfirmRequest = ((Attribute & M_AUDIT)    || flag_on((char *)"CFM", flagstr));
	Msg.Immediate      =                              flag_on((char *)"IMM", flagstr);
	Msg.Direct         =                              flag_on((char *)"DIR", flagstr);
	Msg.Gate           =                              flag_on((char *)"ZON", flagstr);

	if ((origZone == destZone) && (origNet == destNet) && (origNode == destNode) && (origPoint == destPoint)) {
	    /*
	     * Message is local, make the message appear as a received netmail
	     */
	    if (empty) {
		Syslog('+', "Empty local netmail for %s dropped", toUserName);
		Msg_UnLock();
		Msg_Close();
		fclose(fp);
		free(temp);
		return 6;
	    }
	    islocal = TRUE;
	    if ((strncasecmp(toUserName, "sysop", 5) == 0) ||
		(strncasecmp(toUserName, "postmaster", 10) == 0) ||
		(strncasecmp(toUserName, "coordinator", 11) == 0)) {
		Syslog('+', "  Readdress from %s to %s", toUserName, CFG.sysop_name);
		snprintf(toUserName, 37, "%s", CFG.sysop_name);
		strcpy(Msg.To, toUserName);
	    }
	    net_imp++;
	} else {
	    Msg.Local = TRUE;
	    islocal = FALSE;
	}
	Syslog('m', "Netmail is %s", islocal ? "Local":"for export");
	Msg.Private = TRUE;
	Msg.Netmail = TRUE;

	if (origPoint)
	    snprintf(Msg.FromAddress, 101, "%d:%d/%d.%d@%s", origZone, origNet, origNode, origPoint, fidonet.domain);
	else
	    snprintf(Msg.FromAddress, 101, "%d:%d/%d@%s", origZone, origNet, origNode, fidonet.domain);
	if (SearchFidonet(destZone)) {
	    if (destPoint)
		snprintf(Msg.ToAddress, 101, "%d:%d/%d.%d@%s", destZone, destNet, destNode, destPoint, fidonet.domain);
	    else
		snprintf(Msg.ToAddress, 101, "%d:%d/%d@%s", destZone, destNet, destNode, fidonet.domain);
	} else {
	    if (destPoint)
		snprintf(Msg.ToAddress, 101, "%d:%d/%d.%d", destZone, destNet, destNode, destPoint);
	    else
		snprintf(Msg.ToAddress, 101, "%d:%d/%d", destZone, destNet, destNode);
	}
	
	/*
	 * Add original message text
	 */
	fseek(fp, 0xbe, SEEK_SET);
	while (fgets(temp, PATH_MAX-1, fp)) {
	    Striplf(temp);
	    if (temp[strlen(temp)-1] == '\r')
		temp[strlen(temp)-1] = '\0';
	    MsgText_Add2(temp);
	}
	
	Msg_AddMsg();
	Msg_UnLock();
	Syslog('+', "%s => %s (%ld) to \"%s\", \"%s\"", msgname, msgs.Name, Msg.Id, Msg.To, Msg.Subject);

	msgs.LastPosted = time(NULL);
	msgs.Posted.total++;
	msgs.Posted.tweek++;
	msgs.Posted.tdow[Diw]++;
	msgs.Posted.month[Miy]++;
	UpdateMsgs();

	if (!islocal) {
	    do_scan = TRUE;
	    snprintf(temp, PATH_MAX, "%s/tmp/netmail.jam", getenv("MBSE_ROOT"));
	    if ((np = fopen(temp, "a")) != NULL) {
		fprintf(np, "%s %u\n", msgs.Base, Msg.Id);
		fclose(np);
	    }
	}

	Msg_Close();

    } else {
	WriteError("Can't open JAM %s", msgs.Base);
	rc = 5;
    }

    fclose(fp);

    if (rc == 0) {
	net_in++;
	snprintf(temp, PATH_MAX, "%s/%s", CFG.msgs_path, msgname);
	if (unlink(temp) != 0)
	    WriteError("Can't remove %s", temp);
    }

    free(temp);
    return rc;
}


