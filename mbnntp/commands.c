/*****************************************************************************
 *
 * $Id$
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
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "ttyio.h"
#include "mbnntp.h"
#include "commands.h"



unsigned long	article = 0L;	    /* Current article	    */
char		currentgroup[81];   /* Current newsgroup    */

extern unsigned long	sentbytes;



/*
 * Safe sending to the client with charset translation.
 */
void send_xlat(char *inp)
{
    char    *xl, temp[1024];
    int     i;
	
    memset(&temp, 0, sizeof(temp));
    
    for (i = 0; i < strlen(inp); i++) {
	if (inp[i] & 0x80) {
	    if ((xl = charset_map_c(inp[i], FALSE))) {
		while (*xl) {
		    temp[i] = *xl++;
		    if (*xl)
			i++;
		}
	    }
	} else {
	    temp[i] = inp[i];
	}
    }

    Syslog('n', "> \"%s\"", printable(temp, 0));
    PUTSTR(temp);
    PUTSTR((char *)"\r\n");
    FLUSHOUT();
    sentbytes += (strlen(temp) + 2);
}



char *make_msgid(unsigned long nr, unsigned long crc)
{
    static char	buf[100];

    sprintf(buf, "<%lu$%8lx@%s>", nr, crc, CFG.sysdomain);
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
    char	    *p, *cmd, *opt, dig[128];
    unsigned long   art = 0L;
    int		    i;

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
	Syslog('n', "\"%s\"", printable(opt, 0));
	strcpy(dig, opt+1);
	Syslog('n', "\"%s\"", printable(dig, 0));
	for (i = 0; i < strlen(dig); i++) {
	    if (dig[i] == '$') {
		dig[i] = '\0';
		break;
	    }
	}
	Syslog('n', "\"%s\"", printable(dig, 0));
	art = atoi(dig);
    } else {
	art = atoi(opt);
    }

    Syslog('n', "Article %lu", art);
    
    if (art == 0L) {
	send_nntp("420 No current article has been selected");
	return;
    }

    if (! Msg_ReadHeader(art)) {
	send_nntp("430 No such article found");
	return;
    }

    if (strcasecmp(cmd, "STAT") == 0) {
	send_nntp("223 %lu %s Article retrieved", art, make_msgid(art, StringCRC32(Msg.Msgid)));
	return;
    }

    /*
     * Setup a default translation
     */
    charset_set_in_out((char *)"x-ibmpc", (char *)"iso-8859-1");

    if (Msg_Read(art, 75)) {

	if (strcasecmp(cmd, "ARTICLE") == 0)
	    send_nntp("220 %ld %s Article retrieved - Head and body follow", art, make_msgid(art, StringCRC32(Msg.Msgid)));
	if (strcasecmp(cmd, "HEAD") == 0)
	    send_nntp("221 %ld %s Article retrieved - Head follows", art, make_msgid(art, StringCRC32(Msg.Msgid)));
	if (strcasecmp(cmd, "BODY") == 0)
	    send_nntp("222 %ld %s Article retrieved - Body follows", art, make_msgid(art, StringCRC32(Msg.Msgid)));

	if ((strcasecmp(cmd, "ARTICLE") == 0) || (strcasecmp(cmd, "HEAD") == 0)) {

	    send_nntp("Path: MBNNTP!not-for-mail");
	    send_nntp("From: %s <%s>", Msg.From, Msg.FromAddress);
	    send_nntp("Newsgroups: %s", currentgroup);
	    send_nntp("Subject: %s", Msg.Subject);
	    send_nntp("Date: %s", rfcdate(Msg.Written));
	    send_nntp("Message-ID: %s", make_msgid(art, StringCRC32(Msg.Msgid)));
	    if (strlen(Msg.Replyid))
		send_nntp("References: %s", make_msgid(Msg.Reply, StringCRC32(Msg.Replyid)));
	    send_nntp("X-JAM-From: %s <%s>", Msg.From, Msg.FromAddress);
	    if (strlen(Msg.To))
		send_nntp("X-JAM-To: %s", Msg.To);
	    if ((p = (char *)MsgText_First()) != NULL) {
		do {
		    if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
			if (p[0] == '\001') {
			    send_nntp("X-JAM-%s", p+1);
			} else {
			    send_nntp("X-JAM-%s", p);
			}
		    }
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	    
//	    send_nntp("X-JAM-Attributes:");
	    send_nntp("MIME-Version: 1.0");
	    send_nntp("Content-Type: text/plain; charset=iso-8859-1; format=fixed");
	    send_nntp("Content-Transfer-Encoding: 8bit");
	}

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
    sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r"))) {
	fread(&msgshdr, sizeof(msgshdr), 1, fp);
	while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
	    if (msgs.Active && ((msgs.Type == ECHOMAIL) || (msgs.Type == NEWS)) && strlen(msgs.Newsgroup) &&
		    (strcasecmp(opt, msgs.Newsgroup) == 0) && Access(usrconfig.Security, msgs.RDSec)) {
		Syslog('n', "Found the group");
		if (Msg_Open(msgs.Base)) {
		    Msg_Number();
		    Msg_Highest();
		    Msg_Lowest();
		    send_nntp("211 %lu %lu %lu %s", MsgBase.Total, MsgBase.Lowest, MsgBase.Highest, msgs.Newsgroup);
		    sprintf(currentgroup, "%s", msgs.Newsgroup);
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
	sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r"))) {
	    fread(&msgshdr, sizeof(msgshdr), 1, fp);
	    while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
		if (msgs.Active && ((msgs.Type == ECHOMAIL) || (msgs.Type == NEWS)) && strlen(msgs.Newsgroup) && 
			Access(usrconfig.Security, msgs.RDSec)) {
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

    if (opt && (strcasecmp(opt, "OVERVIEW.FMT") == 0)) {
	send_nntp("215 Order of fields in overview database");
	send_nntp("Subject:");
	send_nntp("From:");
	send_nntp("Date:");
	send_nntp("Message-ID:");
	send_nntp("References:");
	send_nntp("Bytes:");
	send_nntp("Lines:");
//	send_nntp("Xref:full");
	send_nntp(".");
	return;
    }

    /*
     * No recognized LIST command
     */
    send_nntp("503 Function not available");

    msleep(1);	    /* For the linker only */
    colour(0, 0);
}



/*
 * XOVER
 */
void command_xover(char *cmd)
{
    char	    *opt, *p, msgid[100], reply[100];
    unsigned long   i, start, end;
    int		    refs, bytecount, linecount;

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
	    bytecount = linecount = refs = 0;
	    if (Msg.Original)
		refs++;
	    if (Msg.Reply)
		refs++;
	    if (Msg_Read(i, 80)) {
		if ((p = (char *)MsgText_First()) != NULL) {
		    do {
			if (p[0] != '\001') {
			    linecount++;
			    bytecount += strlen(p);
			}
		    } while ((p = (char *)MsgText_Next()) != NULL);
		}
	    }
	    sprintf(msgid, "%s", make_msgid(i, StringCRC32(Msg.Msgid)));
	    reply[0] = 0;
	    if (strlen(Msg.Replyid))
		sprintf(reply, "%s", make_msgid(Msg.Reply, StringCRC32(Msg.Replyid)));
	    send_nntp("%lu\t%s\t%s <%s>\t%s\t%s\t%s\t%d\t%d", i, Msg.Subject, Msg.From, Msg.FromAddress, 
		    rfcdate(Msg.Written), msgid, reply, bytecount, linecount);
	}
    }
    send_nntp(".");
}


