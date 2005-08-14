/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Internet email
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "../lib/mbinet.h"
#include "exitinfo.h"
#include "language.h"
#include "mail.h"
#include "timeout.h"
#include "msgutil.h"
#include "input.h"
#include "email.h"
#include "whoson.h"
#include "term.h"
#include "ttyio.h"


extern unsigned long	LastNum;
extern int		Kludges;
extern FILE		*qf;
extern char		*Message[];
extern int		Line;
extern int		do_mailout;
extern int		LC_Wrote;


/*
 * Internal prototypes
 */
int  HasNoEmail(void);
int  Export_a_Email(unsigned long);
int  EmailPanel(void);
int  Save_Email(int);



int HasNoEmail(void)
{
    if (exitinfo.Email)
	return FALSE;

    Enter(1);
    pout(WHITE, BLACK, (char *)"You have no e-mail access");
    Enter(2);
    sleep(3);
    return TRUE;
}



/* 
 * Show message header screen top for reading messages.
 */
void ShowEmailHdr(void)
{
    static char     Buf1[35], Buf2[35], Buf3[81], temp[81];
    struct tm       *tm;

    Buf1[0] = '\0';
    Buf2[0] = '\0';
    Buf3[0] = '\0';

    clear();
    sprintf(temp, "   %-70s", sMailbox);
    pout(1, 7, temp);

    sprintf(temp, "#%-5lu", Msg.Id);
    pout(4, 7, temp);
    Enter(1);

    /* Date     : */
    pout(14, 0, (char *) Language(206));
    tm = gmtime(&Msg.Written);
    sprintf(temp, "%02d-%02d-%d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon+1, 
		tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
    pout(10, 0, temp);
    colour(12, 0);
    if (Msg.Local)          PUTSTR((char *)" Local");
    if (Msg.Intransit)      PUTSTR((char *)" Transit");
    if (Msg.Private)        PUTSTR((char *)" Priv.");
    if (Msg.Received)       PUTSTR((char *)" Rcvd");
    if (Msg.Sent)           PUTSTR((char *)" Sent");
    if (Msg.KillSent)       PUTSTR((char *)" KillSent");
    if (Msg.ArchiveSent)    PUTSTR((char *)" ArchiveSent");
    if (Msg.Hold)           PUTSTR((char *)" Hold");
    if (Msg.Crash)          PUTSTR((char *)" Crash");
    if (Msg.Immediate)      PUTSTR((char *)" Imm.");
    if (Msg.Direct)         PUTSTR((char *)" Dir");
    if (Msg.Gate)           PUTSTR((char *)" Gate");
    if (Msg.FileRequest)    PUTSTR((char *)" Freq");
    if (Msg.FileAttach)     PUTSTR((char *)" File");
    if (Msg.TruncFile)      PUTSTR((char *)" TruncFile");
    if (Msg.KillFile)       PUTSTR((char *)" KillFile");
    if (Msg.ReceiptRequest) PUTSTR((char *)" RRQ");
    if (Msg.ConfirmRequest) PUTSTR((char *)" CRQ");
    if (Msg.Orphan)         PUTSTR((char *)" Orphan");
    if (Msg.Encrypt)        PUTSTR((char *)" Crypt");
    if (Msg.Compressed)     PUTSTR((char *)" Comp");
    if (Msg.Escaped)        PUTSTR((char *)" 7bit");
    if (Msg.ForcePU)        PUTSTR((char *)" FPU");
    if (Msg.Localmail)      PUTSTR((char *)" Localmail");
    if (Msg.Netmail)        PUTSTR((char *)" Netmail");
    if (Msg.Echomail)       PUTSTR((char *)" Echomail");
    if (Msg.News)           PUTSTR((char *)" News");
    if (Msg.Email)          PUTSTR((char *)" E-mail");
    if (Msg.Nodisplay)      PUTSTR((char *)" Nodisp");
    if (Msg.Locked)         PUTSTR((char *)" LCK");
    if (Msg.Deleted)        PUTSTR((char *)" Del");
    Enter(1);

    /* From    : */
    pout(14,0, (char *) Language(209));
    pout(10, 0, Msg.From);
    Enter(1);

    /* To      : */
    pout(14,0, (char *) Language(208));
    pout(10, 0, Msg.To);
    Enter(1);

    /* Subject : */
    pout(14,0, (char *) Language(210));
    pout(10, 0, Msg.Subject);
    Enter(1);
    
    if (Msg.Reply)
	sprintf(Buf1, "\"+\" %s %lu", (char *)Language(211), Msg.Reply);
    if (Msg.Original)
	sprintf(Buf2, "   \"-\" %s %lu", (char *)Language(212), Msg.Original);
    sprintf(Buf3, "%s%s ", Buf1, Buf2);
    sprintf(temp, "%78s  ", Buf3);
    pout(14, 1, temp);
    Enter(1);
}



/*
 * Export a email to file in the users home directory.
 */
int Export_a_Email(unsigned long Num)
{
    char    *p, temp[21];

    LastNum = Num;
    iLineCount = 7;
    WhosDoingWhat(READ_POST, NULL);
    Syslog('+', "Export email %d in area %s", Num, sMailbox);

    /*
     * The area data is already set, so we can do the next things
     */
    if (EmailBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
	return FALSE;
    }

    if (!Msg_Open(sMailpath)) {
	WriteError("Error open JAM base %s", sMailpath);
	return FALSE;
    }

    if (!Msg_ReadHeader(Num)) {
	Enter(1);
	pout(WHITE, BLACK, (char *)Language(77));
	Msg_Close();
	Enter(2);
	sleep(3);
	return FALSE;
    }

    /*
     * Export the message text to the file in the users home/wrk directory.
     * Create the filename as <areanum>_<msgnum>.msg The message is
     * written in M$DOS <cr/lf> format.
     */
    p = calloc(PATH_MAX, sizeof(char));
    sprintf(p, "%s/%s/wrk/%s_%lu.msg", CFG.bbs_usersdir, exitinfo.Name, sMailbox, Num);
    if ((qf = fopen(p, "w")) != NULL) {
	free(p);
	p = NULL;
	if (Msg_Read(Num, 80)) {
	    if ((p = (char *)MsgText_First()) != NULL) {
		do {
		    if (p[0] == '\001') {
			if (Kludges) {
			    p[0] = 'a';
			    fprintf(qf, "^%s\r\n", p);
			}
		    } else
			fprintf(qf, "%s\r\n", p);
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	}
	fclose(qf);
    } else {
	WriteError("$Can't open %s", p);
	free(p);
    }
    Msg_Close();

    /*
     * Report the result.
     */
    Enter(2);
    pout(CFG.TextColourF, CFG.TextColourB, (char *) Language(46));
    sprintf(temp, "%s_%lu.msg", sMailbox, Num);
    pout(CFG.HiliteF, CFG.HiliteB, temp);
    Enter(2);
    Pause();
    return TRUE;
}



/*
 *  Save the message to disk.
 */
int Save_Email(int IsReply)
{
    int             i;
    char            *p, *temp;
    unsigned long   crc = -1;
    long	    id;
    FILE	    *fp;

    if (Line < 2)
	return TRUE;

    if (!Open_Msgbase(sMailpath, 'w')) {
	return FALSE;
    }

    Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
    Msg.Written = Msg.Arrived;
    Msg.Local = TRUE;
    Msg.Netmail = TRUE;
    temp = calloc(PATH_MAX, sizeof(char));

    /*
     * Add header lines
     */
    sprintf(temp, "\001Date: %s", rfcdate(Msg.Written));
    MsgText_Add2(temp);
    sprintf(temp, "\001From: %s", Msg.From);
    MsgText_Add2(temp);
    sprintf(temp, "\001Subject: %s", Msg.Subject);
    MsgText_Add2(temp);
    sprintf(temp, "\001Sender: %s", Msg.From);
    MsgText_Add2(temp);
    sprintf(temp, "\001To: %s", Msg.To);
    MsgText_Add2(temp);
    MsgText_Add2((char *)"\001MIME-Version: 1.0");
    if (exitinfo.Charset != FTNC_NONE) {
	sprintf(temp, "\001Content-Type: text/plain; charset=%s", getrfcchrs(exitinfo.Charset));
    } else {
	sprintf(temp, "\001Content-Type: text/plain; charset=iso8859-1");
    }
    MsgText_Add2(temp);
    MsgText_Add2((char *)"\001Content-Transfer-Encoding: 8bit");
    sprintf(temp, "\001X-Mailreader: MBSE BBS %s", VERSION);
    MsgText_Add2(temp);
    p = calloc(81, sizeof(char));
    id = sequencer();
    sprintf(p, "<%08lx@%s>", id, CFG.sysdomain);
    sprintf(temp, "\001Message-id: %s", p);
    MsgText_Add2(temp);
    Msg.MsgIdCRC = upd_crc32(temp, crc, strlen(temp));
    free(p);

    if (IsReply) {
	sprintf(temp, "\001In-reply-to: %s", Msg.Replyid);
	MsgText_Add2(temp);
	crc = -1;
	Msg.ReplyCRC = upd_crc32(temp, crc, strlen(temp));
    } else
	Msg.ReplyCRC = 0xffffffff;

    /*
     * Add message text
     */
    for (i = 1; i < Line; i++) {
	MsgText_Add2(Message[i]);
    }

    /*
     * Add signature.
     */
    sprintf(temp, "%s/%s/.signature", CFG.bbs_usersdir, exitinfo.Name);
    if ((fp = fopen(temp, "r"))) {
        Syslog('m', "  Add .signature");
        MsgText_Add2((char *)"");
        while (fgets(temp, 80, fp)) {
	    Striplf(temp);
	    MsgText_Add2(temp);
	}
	fclose(fp);
	MsgText_Add2((char *)"");
    }
    MsgText_Add2(TearLine());

    /*
     * Save if to disk
     */
    Msg_AddMsg();
    Msg_UnLock();

    ReadExitinfo();
    exitinfo.iPosted++;
    WriteExitinfo();

    do_mailout = TRUE;
    LC_Wrote = TRUE;

    Syslog('+', "Email (%ld) to \"%s\", \"%s\", in mailbox", Msg.Id, Msg.To, Msg.Subject);

    Enter(1);
    /* Saving message to disk */
    sprintf(temp, "%s(%ld)", (char *) Language(202), Msg.Id);
    pout(CFG.HiliteF, CFG.HiliteB, temp);
    Enter(2);
    sleep(2);

    /*
     * Add quick mailscan info
     */
    sprintf(temp, "%s/tmp/netmail.jam", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "a")) != NULL) {
	fprintf(fp, "%s/%s/mailbox %lu\n", CFG.bbs_usersdir, exitinfo.Name, Msg.Id);
	fclose(fp);
    }

    free(temp);
    Close_Msgbase(sMailpath);

    return TRUE;
}



int Read_a_Email(unsigned long Num)
{
    char        *p = NULL, *fn;
    lastread    LR;

    LastNum = Num;
    iLineCount = 7;
    WhosDoingWhat(READ_POST, NULL);

    /*
     * The area data is already set, so we can do the next things
     */
    if (EmailBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
	return FALSE;
    }

    if (!Msg_Open(sMailpath)) {
	WriteError("Error open JAM base %s", sMailpath);
	return FALSE;
    }

    if (!Msg_ReadHeader(Num)) {
	Enter(1);
	pout(WHITE, BLACK, (char *)Language(77));
	Enter(2);
	Msg_Close();
	sleep(3);
	return FALSE;
    }
    ShowEmailHdr();

    /*
     * Fill Quote file in case the user wants to reply. Note that line
     * wrapping is set lower then normal message read, to create room
     * for the Quote> strings at the start of each line.
     */
    fn = calloc(PATH_MAX, sizeof(char));
    sprintf(fn, "%s/%s/.quote", CFG.bbs_usersdir, exitinfo.Name);
    if ((qf = fopen(fn, "w")) != NULL) {
	if (Msg_Read(Num, 75)) {
	    if ((p = (char *)MsgText_First()) != NULL)
		do {
		    if (p[0] == '\001') {
			/*
			 * While doing this, store the original Message-id in case
			 * a reply will be made.
			 */
			if (strncasecmp(p, "\001Message-id: ", 13) == 0) {
			    sprintf(Msg.Msgid, "%s", p+13);
			    Syslog('m', "Stored Msgid \"%s\"", Msg.Msgid);
			}
			if (Kludges) {
			    p[0] = 'a';
			    fprintf(qf, "^%s\n", p);
			}
		    } else
			fprintf(qf, "%s\n", p);
		} while ((p = (char *)MsgText_Next()) != NULL);
	}
	fclose(qf);
    } else {
	WriteError("$Can't open %s", p);
    }
    free(fn);

    /*
     * Show message text
     */
    colour(CFG.TextColourF, CFG.TextColourB);
    if (Msg_Read(Num, 79)) {
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if (p[0] == '\001') {
		    if (Kludges) {
			pout(LIGHTGRAY, BLACK, p);
			Enter(1);
			if (CheckLine(CFG.TextColourF, CFG.TextColourB, TRUE, FALSE))
			    break;
		    }
		} else {
		    colour(CFG.TextColourF, CFG.TextColourB);
		    if (strchr(p, '>') != NULL)
			if ((strlen(p) - strlen(strchr(p, '>'))) < 10)
			    colour(CFG.HiliteF, CFG.HiliteB);
		    PUTSTR(p);
		    Enter(1);
		    if (CheckLine(CFG.TextColourF, CFG.TextColourB, TRUE, FALSE))
			break;
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }

    /*
     * Set the Received status on this message.
     */
    if (!Msg.Received) {
	Syslog('m', "Marking message received");
	Msg.Received = TRUE;
	Msg.Read = time(NULL) - (gmt_offset((time_t)0) * 60);
	if (Msg_Lock(30L)) {
	    Msg_WriteHeader(Num);
	    Msg_UnLock();
	}
    }

    /*
     * Update lastread pointer.
     */
    if (Msg_Lock(30L)) {
        LR.UserID = grecno;
        p = xstrcpy(exitinfo.sUserName);
        if (Msg_GetLastRead(&LR) == TRUE) {
            LR.LastReadMsg = Num;
            if (Num > LR.HighReadMsg)
                LR.HighReadMsg = Num;
            if (LR.HighReadMsg > EmailBase.Highest)
                LR.HighReadMsg = EmailBase.Highest;
            LR.UserCRC = StringCRC32(tl(p));
	    if (!Msg_SetLastRead(LR))
                WriteError("Error update lastread");
	} else {
            /*
             * Append new lastread pointer
             */
            LR.UserCRC = StringCRC32(tl(p));
            LR.UserID  = grecno;
            LR.LastReadMsg = Num;
            LR.HighReadMsg = Num;
            if (!Msg_NewLastRead(LR))
                WriteError("Can't append lastread");
        }
        free(p);
        Msg_UnLock();
    }

    Msg_Close();
    return TRUE;
}



/*
 * The panel bar under the message while email reading
 */
int EmailPanel(void)
{
    int	input;

    WhosDoingWhat(READ_POST, NULL);

    /* (A)gain, (N)ext, (L)ast, (R)eply, (E)nter, (D)elete, (Q)uit, e(X)port */
    pout(WHITE, RED, (char *) Language(214));
    if (exitinfo.Security.level >= CFG.sysop_access)
	PUTSTR((char *)", (!)");
    PUTSTR((char *)": ");
    alarm_on();
    input = toupper(Readkey());

    if (input == '!') {
	if (exitinfo.Security.level >= CFG.sysop_access) {
	    if (Kludges)
		Kludges = FALSE;
	    else
		Kludges = TRUE;
	    Read_a_Email(LastNum);
	}
    } else if (input == Keystroke(214, 0)) { /* (A)gain */
	Read_a_Email(LastNum);
    } else if (input == Keystroke(214, 4)) { /* (P)ost */
	Write_Email();
	Read_a_Email(LastNum);
    } else if (input == Keystroke(214, 2)) { /* (L)ast */
	if (LastNum  > EmailBase.Lowest)
	    LastNum--;
	Read_a_Email(LastNum);
    } else if (input == Keystroke(214, 3)) { /* (R)eply */
	Reply_Email(TRUE);
	Read_a_Email(LastNum);
    } else if (input == Keystroke(214, 5)) { /* (Q)uit */
	/* Quit */
	pout(WHITE, BLACK, (char *) Language(189));
	Enter(1);
	return FALSE;
    } else if (input == Keystroke(214, 7)) { /* e(X)port */
	Export_a_Email(LastNum);
	Read_a_Email(LastNum);
    } else if (input == '+') {
	if (Msg.Reply) 
	    LastNum = Msg.Reply;
	Read_a_Email(LastNum);
    } else if (input == '-') {
	if (Msg.Original) 
	    LastNum = Msg.Original;
	Read_a_Email(LastNum);
    } else if (input == Keystroke(214, 6)) { /* (D)elete */
//		Delete_EmailNum(LastNum);
	if (LastNum < EmailBase.Highest) {
	    LastNum++;
	    Read_a_Email(LastNum);
	} else {
	    return FALSE;
	}
    } else {
	/* Next */
	pout(15, 0, (char *) Language(216));
	if (LastNum < EmailBase.Highest)
	    LastNum++;
	else
	    return FALSE;
	Read_a_Email(LastNum);
    }
    return TRUE;
}



/*
 * Read e-mail, called from the menus
 */
void Read_Email(void)
{
    char            *temp;
    unsigned long   Start;
    lastread        LR;

    if (HasNoEmail())
	return;

    Enter(1);
    temp = calloc(128, sizeof(char));
    /* Message area \"%s\" contains %lu messages. */
    sprintf(temp, "\n%s\"%s\" %s%lu %s", (char *) Language(221), sMailbox, (char *) Language(222), 
		EmailBase.Total, (char *) Language(223));
    pout(CFG.TextColourF, CFG.TextColourB, temp);

    /*
     * Check for lastread pointer, suggest lastread number for start.
     */
    Start = EmailBase.Lowest;
    if (Msg_Open(sMailpath)) {
	LR.UserID = grecno;
	if (Msg_GetLastRead(&LR))
	    Start = LR.HighReadMsg + 1;
	else
	    Start = 1;
	Msg_Close();
	/*
	 * If we already have read the last message, the pointer is
	 * higher then HighMsgNum, we set it at HighMsgNum to prevent
	 * errors and read that message again.
	 */
	if (Start > EmailBase.Highest)
	    Start = EmailBase.Highest;
    }

    Enter(1);
    /* Please enter a message between */
    sprintf(temp, "%s(%lu - %lu)", (char *) Language(224), EmailBase.Lowest, EmailBase.Highest);
    pout(WHITE, BLACK, temp);
    Enter(1);
    /* Message number [ */
    sprintf(temp, "%s%lu]: ", (char *) Language(225), Start);
    PUTSTR(temp);

    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(temp, 80);
    if ((strcmp(temp, "")) != 0)
	Start = atoi(temp);
    free(temp);

    if (!Read_a_Email(Start))
	return;

    if (EmailBase.Total == 0)
	return;

    while(EmailPanel()) {}
}



/*
 *  Reply message, in Msg.From and Msg.Subject must be the
 *  name to reply to and the subject.
 */
void Reply_Email(int IsReply)
{
    int     i, j, x;
    char    to[65], from[65], subj[72], msgid[81], replyto[81], replyaddr[81], *tmp, *buf, qin[9], temp[81];
    faddr   *Dest = NULL;

    sprintf(from, "%s", Msg.To);
    sprintf(to, "%s", Msg.From);
    sprintf(replyto, "%s", Msg.ReplyTo);
    sprintf(replyaddr, "%s", Msg.ReplyAddr);

    if (strncasecmp(Msg.Subject, "Re:", 3) && IsReply) {
	sprintf(subj, "Re: %s", Msg.Subject);
    } else {
	sprintf(subj, "%s", Msg.Subject);
    }
    mbse_CleanSubject(subj);
    Syslog('m', "Reply msg to %s, subject %s", to, subj);
    Syslog('m', "Msgid was %s", Msg.Msgid);
    sprintf(msgid, "%s", Msg.Msgid);

    x = 0;
    Line = 1;
    WhosDoingWhat(READ_POST, NULL);
    clear();
    sprintf(temp, "   %-70s", sMailbox);
    pout(BLUE, LIGHTGRAY, temp);
    sprintf(temp, "#%-5lu", EmailBase.Highest + 1);
    pout(RED, LIGHTGRAY, temp);
    Enter(1);

    colour(CFG.HiliteF, CFG.HiliteB);
    sLine();
    Enter(1);

    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	Message[i] = (char *) calloc(MAX_LINE_LENGTH +1, sizeof(char));
    Line = 1;
    Msg_New();

    sprintf(Msg.Replyid, "%s", msgid);
    sprintf(Msg.ReplyTo, "%s", replyto);
    sprintf(Msg.ReplyAddr, "%s", replyaddr);

    /* From     : */
    pout(YELLOW, BLACK, (char *) Language(209));
    if (CFG.EmailMode != E_PRMISP) {
	/*
	 * If not permanent connected to the internet, use fidonet.org style addressing.
	 */
	Dest = fido2faddr(CFG.EmailFidoAka);
	sprintf(Msg.From, "%s@%s (%s)", exitinfo.sUserName, ascinode(Dest, 0x2f), exitinfo.sUserName);
    } else {
	sprintf(Msg.From, "%s@%s (%s)", exitinfo.Name, CFG.sysdomain, exitinfo.sUserName);
    }
    for (i = 0; i < strlen(Msg.From); i++) {
	if (Msg.From[i] == ' ')
	    Msg.From[i] = '_';
	if (Msg.From[i] == '@')
	    break;
    }
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.From);
    Enter(1);
    Syslog('b', "Setting From: %s", Msg.From);

    /* To       : */
    sprintf(Msg.To, "%s", to);
    pout(YELLOW, BLACK, (char *) Language(208));
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.To);
    Enter(1);

    /* Enter to keep Subject. */
    pout(LIGHTRED, BLACK, (char *) Language(219));
    Enter(1);
    /* Subject  : */
    pout(YELLOW, BLACK, (char *) Language(210));
    sprintf(Msg.Subject, "%s", subj);
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.Subject);

    x = strlen(subj);
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    GetstrP(subj, 50, x);

    if (strlen(subj))
	strcpy(Msg.Subject, subj);

    Msg.Private = TRUE;
    Enter(1);

//  Check_Attach();

    /*
     *  Quote original message now, format the original users
     *  initials into qin. If its a name@system.dom the use the
     *  first 8 characters of the name part.
     */
    sprintf(Message[1], "%s wrote to %s:", to, from);
    memset(&qin, 0, sizeof(qin));
    if (strchr(to, '@')) {
	tmp = xstrcpy(strtok(to, "@"));
	tmp[8] = '\0';
	sprintf(qin, "%s", tmp);
	free(tmp);
    } else {
	x = TRUE;
	j = 0;
	for (i = 0; i < strlen(to); i++) {
	    if (x && isalpha(to[i])) {
		qin[j] = to[i];
		j++;
		x = FALSE;
	    }
	    if (to[i] == ' ' || to[i] == '.')
		x = TRUE;
	    if (j == 6)
		break;
	}
    }

    Line = 2;
    tmp = calloc(128, sizeof(char));
    buf = calloc(128, sizeof(char));

    sprintf(tmp, "%s/%s/.quote", CFG.bbs_usersdir, exitinfo.Name);
    if ((qf = fopen(tmp, "r")) != NULL) {
	while ((fgets(buf, 128, qf)) != NULL) {
	    Striplf(buf);
	    sprintf(Message[Line], "%s> %s", (char *)qin, buf);
	    Line++;
	    if (Line == TEXTBUFSIZE)
		break;
	}
	fclose(qf);
    } else
	WriteError("$Can't read %s", tmp);

    free(buf);
    free(tmp);

    if (Edit_Msg())
	Save_Email(IsReply);

    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	free(Message[i]);
}



void Write_Email(void)
{
    faddr   *Dest = NULL;
    int	    i;
    char    *orgbox;

    if (HasNoEmail())
	return;

    orgbox = xstrcpy(sMailbox);
    SetEmailArea((char *)"mailbox");

    WhosDoingWhat(READ_POST, NULL);
    clear();

    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	Message[i] = (char *) calloc(MAX_LINE_LENGTH +1, sizeof(char));
    Line = 1;

    Msg_New();

    Enter(1);
    colour(9, 0);
    /* Posting message in area: */
    pout(LIGHTBLUE, BLACK, (char *) Language(156));
    PUTSTR((char *)"\"mailbox\"");
    Enter(2);

    /* From   : */
    pout(YELLOW, BLACK, (char *) Language(157));
    if (CFG.EmailMode != E_PRMISP) {
	/*
	 * If not permanent connected to the internet, use fidonet.org style addressing.
	 */
	Dest = fido2faddr(CFG.EmailFidoAka);
	sprintf(Msg.From, "%s@%s (%s)", exitinfo.sUserName, ascinode(Dest, 0x2f), exitinfo.sUserName);
    } else
	sprintf(Msg.From, "%s@%s (%s)", exitinfo.Name, CFG.sysdomain, exitinfo.sUserName);
    
    for (i = 0; i < strlen(Msg.From); i++) {
        if (Msg.From[i] == ' ')
            Msg.From[i] = '_';
        if (Msg.From[i] == '@')
            break;
    }
    
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.From);
    Syslog('b', "Setting From: %s", Msg.From);
    Enter(1);

    /* To     : */
    pout(YELLOW, BLACK, (char *) Language(158));
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    alarm_on();
    GetstrU(Msg.To, 63);

    if ((strcmp(Msg.To, "")) == 0) {
	for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	    free(Message[i]);
	SetEmailArea(orgbox);
	free(orgbox);
	return;
    }

    /* Subject  :  */
    pout(YELLOW, BLACK, (char *) Language(161));
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    alarm_on();
    GetstrP(Msg.Subject, 65, 0);
    mbse_CleanSubject(Msg.Subject);

    if ((strcmp(Msg.Subject, "")) == 0) {
	Enter(1);
	/* Abort Message [y/N] ?: */
	pout(CYAN, BLACK, (char *) Language(162));
	alarm_on();

	if (toupper(Readkey()) == Keystroke(162, 0)) {
	    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
		free(Message[i]);
	    SetEmailArea(orgbox);
	    free(orgbox);
	    return;
	}
    }

    Msg.Private = TRUE;

    if (Edit_Msg()) {
	Save_Email(FALSE);
    }

    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	free(Message[i]);

    SetEmailArea(orgbox);
    free(orgbox);
}



void QuickScan_Email(void)
{
    int     FoundMsg  = FALSE;
    long    i;
    char    temp[81];

    iLineCount = 2;
    WhosDoingWhat(READ_POST, NULL);

    if (EmailBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area. */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
	return;
    }

    clear(); 
    /* #    From                  To                       Subject */
    poutCR(YELLOW, BLUE, (char *) Language(220));

    if (Msg_Open(sMailpath)) {
	for (i = EmailBase.Lowest; i <= EmailBase.Highest; i++) {
	    if (Msg_ReadHeader(i)) {
                                
		sprintf(temp, "%-6lu", Msg.Id);
		pout(WHITE, BLACK, temp);
		sprintf(temp, "%s ", padleft(Msg.From, 20, ' '));
		pout(CYAN, BLACK, temp);

		sprintf(temp, "%s ", padleft(Msg.To, 20, ' '));
		pout(GREEN, BLACK, temp);
		sprintf(temp, "%s", padleft(Msg.Subject, 31, ' '));
		pout(MAGENTA, BLACK, temp);
		Enter(1);
		FoundMsg = TRUE;
		if (LC(1))
		    break;
	    }
	}
	Msg_Close();
    }

    if(!FoundMsg) {
	Enter(1);
	/* There are no messages in this area. */
	pout(LIGHTGREEN, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
    }

    iLineCount = 2;
    Pause();
}



void Trash_Email(void)
{
    if (HasNoEmail())
	return;
}



void Choose_Mailbox(char *Option)
{
    char    *temp;

    if (HasNoEmail())
	return;

    if (strlen(Option)) {
	if (!strcmp(Option, "M+")) {
	    if (!strcmp(sMailbox, "mailbox"))
		SetEmailArea((char *)"archive");
	    else if (!strcmp(sMailbox, "archive"))
		SetEmailArea((char *)"trash");
	    else if (!strcmp(sMailbox, "trash"))
		SetEmailArea((char *)"mailbox");
	}
	if (!strcmp(Option, "M-")) {
	    if (!strcmp(sMailbox, "mailbox"))
		SetEmailArea((char *)"trash");
	    else if (!strcmp(sMailbox, "trash"))
		SetEmailArea((char *)"archive");
	    else if (!strcmp(sMailbox, "archive"));
		SetEmailArea((char *)"mailbox");
	}
	Syslog('+', "Emailarea: %s", sMailbox);
	return;
    }

    clear();
    Enter(1);
    /*  Message areas  */
    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(231));
    Enter(2);

    pout(15, 0, (char *)"    1"); pout(9, 0, (char *)" . "); pout(3, 0, (char *) Language(467)); Enter(1);
    pout(15, 0, (char *)"    2"); pout(9, 0, (char *)" . "); pout(3, 0, (char *) Language(468)); Enter(1);
    pout(15, 0, (char *)"    3"); pout(9, 0, (char *)" . "); pout(3, 0, (char *) Language(469)); Enter(1);

    pout(CFG.MoreF, CFG.MoreB, (char *) Language(470));
    colour(CFG.InputColourF, CFG.InputColourB);
    temp = calloc(81, sizeof(char));
    GetstrC(temp, 7);

    switch (atoi(temp)) {
	case 1:	SetEmailArea((char *)"mailbox");
		break;
	case 2: SetEmailArea((char *)"archive");
		break;
	case 3: SetEmailArea((char *)"trash");
		break;
    }

    Syslog('+', "Emailarea: %s", sMailbox);
    free(temp);
}



void SetEmailArea(char *box)
{
	if (!exitinfo.Email)
		return;

	sprintf(sMailpath, "%s/%s/%s", CFG.bbs_usersdir, exitinfo.Name, box);
	sprintf(sMailbox, "%s", box);

	/*
	 * Get information from the message base
	 */
	if (Msg_Open(sMailpath)) {
		EmailBase.Lowest  = Msg_Lowest();
		EmailBase.Highest = Msg_Highest();
		EmailBase.Total   = Msg_Number();
		Msg_Close();
	} else
		WriteError("Error open JAM %s", sMailpath);
}



