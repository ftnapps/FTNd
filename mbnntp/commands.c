/*****************************************************************************
 *
 * $Id: commands.c,v 1.36 2007/10/14 15:29:58 mbse Exp $
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
#include "../lib/users.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/mbsedb.h"
#include "ttyio.h"
#include "mbnntp.h"
#include "rfc2ftn.h"
#include "commands.h"


#ifndef	USE_NEWSGATE

unsigned int	article = 0L;	    /* Current article	    	*/
char		currentgroup[81];   /* Current newsgroup    	*/


extern unsigned int	sentbytes;
extern unsigned int	rcvdbytes;

extern char         *ttystat[];

void send_xlat(char *);


#define	POST_MAXSIZE	10000


/*
 * Safe sending to the client with charset translation.
 */
void send_xlat(char *inp)
{
    char    	temp[1024];

    strncpy(temp, chartran(inp), sizeof(temp) -1);
    PUTSTR(temp);
    PUTSTR((char *)"\r\n");
    FLUSHOUT();
    sentbytes += (strlen(temp) + 2);
}



/*
 * Build a faked RFC msgid, use the CRC32 of the FTN msgid, 
 * the current group and the configured system's fqdn. This
 * gives a unique string specific for the message.
 */
char *make_msgid(char *msgid)
{
    static char	buf[100];

    snprintf(buf, 100, "<%08x$%s@%s>", StringCRC32(msgid), currentgroup, CFG.sysdomain);
    return buf;
}



/*
 * ARTICLE
 * BODY
 * HEADER
 * STAT
 */
void command_abhs(char *buf)
{
    char	    *p, *cmd, *opt, *subj, *charset = NULL;
    unsigned int    art = 0L;
    int		    found, charindex;

    Syslog('+', "%s", buf);
    cmd = strtok(buf, " \0");
    opt = strtok(NULL, " \0");

    IsDoing("Retrieve");

    if (opt == NULL) {
	send_nntp("420 No current article has been selected");
	return;
    }

    if (strlen(currentgroup) == 0) {
	send_nntp("412 No newsgroup has been selected");
	return;
    }

    if (opt[0] == '<') {
	/*
	 * We have to read all headers in the area to retrieve the message using the msgid.
	 */
	found = FALSE;
	Syslog('n', "Search from %lu to %lu for %s", MsgBase.Lowest, MsgBase.Highest, opt);
	for (art = MsgBase.Lowest; art <= MsgBase.Highest; art++) {
	    if (Msg_ReadHeader(art)) {
		if (strcmp(opt, make_msgid(Msg.Msgid)) == 0) {
		    Syslog('n', "Found message %lu", art);
		    found = TRUE;
		    break;
		}
	    }
	}
	if (! found) {
	    send_nntp("430 No such article found");
	    return;
	}
    } else {
	art = atoi(opt);
    }

    if (art == 0L) {
	send_nntp("420 No current article has been selected");
	return;
    }

    if (! Msg_ReadHeader(art)) {
	send_nntp("430 No such article found");
	return;
    }

    if (strcasecmp(cmd, "STAT") == 0) {
	send_nntp("223 %lu %s Article retrieved", art, make_msgid(Msg.Msgid));
	return;
    }

    if (Msg_Read(art, 80)) {

	if (strcasecmp(cmd, "ARTICLE") == 0)
	    send_nntp("220 %ld %s Article retrieved - Head and body follow", art, make_msgid(Msg.Msgid));
	if (strcasecmp(cmd, "HEAD") == 0)
	    send_nntp("221 %ld %s Article retrieved - Head follows", art, make_msgid(Msg.Msgid));
	if (strcasecmp(cmd, "BODY") == 0)
	    send_nntp("222 %ld %s Article retrieved - Body follows", art, make_msgid(Msg.Msgid));

	/*
	 * Find out the charset used in the Fido side of the message
	 */
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if (strncmp(p, (char *)"\001CHRS: ", 7) == 0) {
		    charset = xstrcpy(p + 7);
		} else if (strncmp(p, "\001CHARSET: ", 10) == 0) {
		    charset = xstrcpy(p + 10);
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}

	if (charset == NULL) {
	    /*
	     * Missing CHRS kludge, must be an ancient DOS program that
	     * created this message.
	     */
	    charset = xstrcpy((char *)"CP437");
	}
	charindex = find_ftn_charset(charset);

	if (charindex != FTNC_ERROR) {
	    chartran_init(charmap[charindex].ic_ftn, (char *)"UTF-8", 'n');
	}

	if ((strcasecmp(cmd, "ARTICLE") == 0) || (strcasecmp(cmd, "HEAD") == 0)) {

	    send_nntp("Path: MBNNTP!not-for-mail");
	    send_nntp("From: %s <%s>", chartran(Msg.From), Msg.FromAddress);
	    send_nntp("Newsgroups: %s", currentgroup);
	    asprintf(&subj,"Subject: %s", Msg.Subject);
	    send_xlat(subj);
	    free(subj);
	    send_nntp("Date: %s", rfcdate(Msg.Written + (gmt_offset((time_t)0) * 60)));
	    send_nntp("Message-ID: %s", make_msgid(Msg.Msgid));
	    if (strlen(Msg.Replyid))
		send_nntp("References: %s", make_msgid(Msg.Replyid));

	    /*
	     * Send RFC 2045 Multipurpose Internet Mail Extensions (MIME) header.
	     */
	    send_nntp("MIME-Version: 1.0");
	    send_nntp("Content-Type: text/plain; charset=utf-8");

	    send_nntp("X-FTN-From: %s <%s>", Msg.From, Msg.FromAddress);
	    if (strlen(Msg.To))
		send_nntp("X-FTN-To: %s", Msg.To);
	    if ((p = (char *)MsgText_First()) != NULL) {
		do {
		    if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
			if (p[0] == '\001') {
			    send_nntp("X-FTN-%s", p+1);
			} else {
			    send_nntp("X-FTN-%s", p);
			}
		    }
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	}

	if (charset)
	    free(charset);
	charset = NULL;

	if (strcasecmp(cmd, "ARTICLE") == 0)
	    send_nntp("");

	if ((strcasecmp(cmd, "ARTICLE") == 0) || (strcasecmp(cmd, "BODY") == 0)) {
	    if ((p = (char *)MsgText_First()) != NULL) {
		do {
		    if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
			/*
			 * Kludges, suppress
			 */
		    } else {
			if (strcmp(p, ".") == 0) /* Don't send a single dot */
			    send_nntp("..");
			else
			    send_xlat(p);
		    }
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	}
	send_nntp(".");
	chartran_close();
	return;
    } else {
	send_nntp("503 Could not retrieve message");
	return;
    }
}



/*
 * GROUP ggg
 */
void command_group(char *cmd)
{
    FILE    *fp;
    char    *temp, *opt;

    IsDoing("Group");
    Syslog('+', "%s", cmd);
    opt = strtok(cmd, " \0");
    opt = strtok(NULL, " \0");

    if (opt == NULL) {
	send_nntp("411 No such news group");
	return;
    }

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r"))) {
	fread(&msgshdr, sizeof(msgshdr), 1, fp);
	while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
	    /*
	     * Only echomail areas with a valid newsgroup name to which the user has access.
	     */
	    if (msgs.Active && (msgs.Type == ECHOMAIL) && strlen(msgs.Newsgroup) &&
		    (strcasecmp(opt, msgs.Newsgroup) == 0) && Access(usrconfig.Security, msgs.RDSec)) {
		/*
		 * Returns:
		 *    211 n f l s group selected
		 *        (n = estimated number of articles in group,
		 *         f = first article number in the group,
		 *         l = last article number in the group,
		 *         s = name of the group.)
		 *    411 no such news group
		 */
		if (Msg_Open(msgs.Base)) {
		    Msg_Number();
		    Msg_Highest();
		    Msg_Lowest();
		    send_nntp("211 %lu %lu %lu %s", MsgBase.Total, MsgBase.Lowest, MsgBase.Highest, msgs.Newsgroup);
		    snprintf(currentgroup, 81, "%s", msgs.Newsgroup);
		} else {
		    send_nntp("411 No such news group");
		}
		fclose(fp);
		free(temp);
		return;
	    }
	    fseek(fp, msgshdr.syssize, SEEK_CUR);
	}
	fclose(fp);
    } else {
	WriteError("$Can't open %s", temp);
    }

    free(temp);
    send_nntp("411 No such news group");
}



/*
 * LIST
 * LIST ACTIVE
 * LIST NEWSGROUPS
 * LIST OVERVIEW.FMT
 */
void command_list(char *cmd)
{
    FILE    *fp;
    char    *temp, *opt;
    int	    rw;
  
    IsDoing("List");
    Syslog('+', "%s", cmd);
    opt = strtok(cmd, " \0");
    opt = strtok(NULL, " \0");

    if ((opt == NULL) || (strcasecmp(opt, "ACTIVE") == 0) || (strcasecmp(opt, "NEWSGROUPS") == 0)) {
	send_nntp("215 Information follows");
	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r"))) {
	    fread(&msgshdr, sizeof(msgshdr), 1, fp);
	    while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
		/*
		 * Only list echomail areas. If a user wants news, he should get that from
		 * a real newsserver to prevent problems.
		 */
		if (msgs.Active && (msgs.Type == ECHOMAIL) && strlen(msgs.Newsgroup) && Access(usrconfig.Security, msgs.RDSec)) {
		    if (Access(usrconfig.Security, msgs.WRSec))
			rw = 'y';
		    else
			rw = 'n';
		    if (msgs.MsgKinds == RONLY)
			rw = 'n';
		    if (Msg_Open(msgs.Base)) {
			if (opt && (strcasecmp(opt, "NEWSGROUPS") == 0))
			    send_nntp("%s %s", msgs.Newsgroup, msgs.Name);
			else
			    send_nntp("%s %lu %lu %c", msgs.Newsgroup, Msg_Lowest(), Msg_Highest(), rw);
			Msg_Close();
		    }
		}
		fseek(fp, msgshdr.syssize, SEEK_CUR);
	    }
	    fclose(fp);
	} else {
	    WriteError("$Can't open %s", temp);
	}
	free(temp);
	send_nntp(".");
	return;
    }

    /*
     * Standard list, most clients don't need it, but it's adviced to have.
     */
    if (opt && (strcasecmp(opt, "OVERVIEW.FMT") == 0)) {
	send_nntp("215 Order of fields in overview database");
	send_nntp("Subject:");
	send_nntp("From:");
	send_nntp("Date:");
	send_nntp("Message-ID:");
	send_nntp("References:");
	send_nntp("Bytes:");
	send_nntp("Lines:");
	send_nntp(".");
	return;
    }

    /*
     * No recognized LIST command
     */
    send_nntp("503 Function not available");
}



/*
 * POST
 */
int get_post(char *buf, int max)
{
    int     c, len;

    len = 0;
    memset(buf, 0, sizeof(buf));
    while (TRUE) {
	c = tty_getc(180);
	if (c <= 0) {
	    if (c == -2) {
		/*
		 * Timeout
		 */
		send_nntp("400 Service discontinued, timeout");
	    }
	    Syslog('+', "Receiver status %s", ttystat[- c]);
	    return c;
	}
	if (c != '\n') {
	    buf[len] = c;
	    len++;
	    buf[len] = '\0';
	    if (c == '\r') {
		rcvdbytes += len;
		return len;
	    }
	}
	if (len >= max) {
	    WriteError("Input buffer full");
	    return len;
	}
    }

    return 0;       /* Not reached */
}



/*
 * POST
 */
void command_post(char *cmd)
{
    FILE    *fp = NULL;
    int	    i, rc, maxrc, Done = FALSE, nrofgroups;
    char    buf[1024], *group, *groups[25];

    IsDoing("Post");
    Syslog('+', "%s", cmd);
	    
    if ((fp = tmpfile()) == NULL) {
	WriteError("$Can't create tmpfile");
	send_nntp("503 Out of memory");
	return;
    }

    send_nntp("340 Send article to be posted. End with <CR-LF>.<CR-LF>");

    while (Done == FALSE) {
	rc = get_post(buf, sizeof(buf) -1);
	/*
	 * Strip CR/LF
	 */
	if (buf[strlen(buf)-1] == '\n') {
	    buf[strlen(buf)-1] = '\0';
	    rc--;
	}
	if (buf[strlen(buf)-1] == '\r') {
	    buf[strlen(buf)-1] = '\0';
	    rc--;
	}
	Syslog('n', "%02d \"%s\"", rc, printable(buf, 0));
	if (rc < 0) {
	    WriteError("nntp_get failed, abort");
	    return;
	}
	if ((rc == 1) && (buf[0] == '.')) {
	    Done = TRUE;
	} else {
	    fwrite(&buf, strlen(buf), 1, fp);
	    fputc('\n', fp);
	}
    }
    
    /*
     * Make a list of newsgroups to post in
     */
    rewind(fp);
    nrofgroups = 0;
    while (fgets(buf, sizeof(buf) -1, fp)) {
	if (!strncasecmp(buf, "Newsgroups: ", 12)) {
	    if (buf[strlen(buf)-1] == '\n')
		buf[strlen(buf)-1] = '\0';
	    if (buf[strlen(buf)-1] == '\r')
		buf[strlen(buf)-1] = '\0';
	    strtok(buf, " \0");
	    while ((group = strtok(NULL, ",\0"))) {
		Syslog('f', "group: \"%s\"", printable(group, 0));
		if (SearchMsgsNews(group)) {
		    Syslog('n', "Add group \"%s\" (%s)", msgs.Newsgroup, msgs.Tag);
		    groups[nrofgroups] = xstrcpy(group);
		    nrofgroups++;
		} else {
		    Syslog('+', "Newsgroup \"%s\" doesn't exist", group);
		}
	    }
	}
    }
    
    if (nrofgroups == 0) {
	Syslog('+', "No newsgroups found for POST");
	send_nntp("441 Posting failed");
	fclose(fp);
	return;
    }

    maxrc = 0;
    for (i = 0; i < nrofgroups; i++) {
	Syslog('+', "Posting in newsgroup %s", groups[i]);
	if (SearchMsgsNews(groups[i])) {
	    rc = rfc2ftn(fp);
	    if (rc > maxrc)
		maxrc = rc;
	}
	free(groups[i]);
    }
    fclose(fp);

    if (maxrc)
	send_nntp("441 Posting failed");
    else
	send_nntp("240 Article posted OK");
}



/*
 * XOVER
 */
void command_xover(char *cmd)
{
    char	    *opt, *p, msgid[100], reply[100];
    unsigned int    i, start, end;
    int		    bytecount, linecount;

    IsDoing("Xover");
    opt = strtok(cmd, " \0");
    opt = strtok(NULL, " \0");	

    if (MsgBase.Open == FALSE) {
	send_nntp("411 No news group current selected");
	return;
    }

    if ((opt == NULL) && (article == 0L)) {
	send_nntp("420 No article selected");
	return;
    }

    start = MsgBase.Lowest;
    end   = MsgBase.Highest;

    Syslog('n', "Start %ld, end %ld", start, end);

    if (opt != NULL) {
	if (strchr(opt, '-')) {
	    /*
	     * We have a dash, format 12- or 12-16
	     */
	    p = strtok(opt, "-");
	    start = atoi(p);
	    p = strtok(NULL, "\0");
	    if (p != NULL)
		end = atoi(p);
	} else {
	    /*
	     * Must be a single digit
	     */
	    start = end = atoi(opt);
	}
    }

    Syslog('n', "Start %ld, end %ld", start, end);

    send_nntp("224 Overview information follows");
    for (i = start; i <= end; i++) {
	if (Msg_ReadHeader(i)) {
	    bytecount = linecount = 0;
	    if (Msg_Read(i, 80)) {
		if ((p = (char *)MsgText_First()) != NULL) {
		    do {
			if ((p[0] != '\001') && (strncmp(p, "SEEN-BY:", 8)) && (strncmp(p, "AREA:", 5))) {
			    /*
			     * Only count lines and bytes we should send
			     */
			    linecount++;
			    bytecount += strlen(p);
			}
		    } while ((p = (char *)MsgText_Next()) != NULL);
		}
	    }
	    snprintf(msgid, 100, "%s", make_msgid(Msg.Msgid));
	    reply[0] = 0;
	    if (strlen(Msg.Replyid))
		snprintf(reply, 100, "%s", make_msgid(Msg.Replyid));
	    send_nntp("%lu\t%s\t%s <%s>\t%s\t%s\t%s\t%d\t%d", i, Msg.Subject, Msg.From, Msg.FromAddress, 
		    rfcdate(Msg.Written + (gmt_offset((time_t)0) * 60)), msgid, reply, bytecount, linecount);
	}
    }
    send_nntp(".");
}

#endif
