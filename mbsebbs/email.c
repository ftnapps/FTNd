/*****************************************************************************
 *
 * File ..................: bbs/email.c
 * Purpose ...............: Internet email
 * Last modification date : 17-Sep-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mbinet.h"
#include "exitinfo.h"
#include "language.h"
#include "mail.h"
#include "timeout.h"
#include "msgutil.h"
#include "funcs4.h"
#include "email.h"


extern unsigned long	LastNum;
extern int		Kludges;
extern FILE		*qf;
extern char		*Message[];
extern int		Line;
extern int		do_mailout;



/*
 * Internal prototypes
 */
int  HasNoEmail(void);
int  Export_a_Email(unsigned long);
int  EmailPanel(void);
void Reply_Email(int);
int  Save_Email(int);



int HasNoEmail(void)
{
	if (exitinfo.Email)
		return FALSE;

	colour(15, 0);
	printf("\nYou have no e-mail access\n\n");
	sleep(3);
	return TRUE;
}



/* 
 * Show message header screen top for reading messages.
 */
void ShowEmailHdr(void)
{
	static char     Buf1[35], Buf2[35], Buf3[81];
	struct tm       *tm;

	Buf1[0] = '\0';
	Buf2[0] = '\0';
	Buf3[0] = '\0';

	clear();
	colour(1,7);
	printf("   %-70s", sMailbox);

	colour(4,7);
	printf("#%-5lu\n", Msg.Id);

	/* Date     : */
	pout(14, 0, (char *) Language(206));
	colour(10, 0);
	tm = gmtime(&Msg.Written);
	printf("%02d-%02d-%d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon+1, 
		tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	colour(12, 0);
	if (Msg.Local)          printf(" Local");
	if (Msg.Intransit)      printf(" Transit");
	if (Msg.Private)        printf(" Priv.");
	if (Msg.Received)       printf(" Rcvd");
	if (Msg.Sent)           printf(" Sent");
	if (Msg.KillSent)       printf(" KillSent");
	if (Msg.ArchiveSent)    printf(" ArchiveSent");
	if (Msg.Hold)           printf(" Hold");
	if (Msg.Crash)          printf(" Crash");
	if (Msg.Immediate)      printf(" Imm.");
	if (Msg.Direct)         printf(" Dir");
	if (Msg.Gate)           printf(" Gate");
	if (Msg.FileRequest)    printf(" Freq");
	if (Msg.FileAttach)     printf(" File");
	if (Msg.TruncFile)      printf(" TruncFile");
	if (Msg.KillFile)       printf(" KillFile");
	if (Msg.ReceiptRequest) printf(" RRQ");
	if (Msg.ConfirmRequest) printf(" CRQ");
	if (Msg.Orphan)         printf(" Orphan");
	if (Msg.Encrypt)        printf(" Crypt");
	if (Msg.Compressed)     printf(" Comp");
	if (Msg.Escaped)        printf(" 7bit");
	if (Msg.ForcePU)        printf(" FPU");
	if (Msg.Localmail)      printf(" Localmail");
	if (Msg.Netmail)        printf(" Netmail");
	if (Msg.Echomail)       printf(" Echomail");
	if (Msg.News)           printf(" News");
	if (Msg.Email)          printf(" E-mail");
	if (Msg.Nodisplay)      printf(" Nodisp");
	if (Msg.Locked)         printf(" LCK");
	if (Msg.Deleted)        printf(" Del");
	printf("\n");

        /* From    : */
        pout(14,0, (char *) Language(209));
        colour(10, 0);
	printf("%s\n", Msg.From);

	/* To      : */
	pout(14,0, (char *) Language(208));
	colour(10, 0);
	printf("%s\n", Msg.To);

	/* Subject : */
	pout(14,0, (char *) Language(210));
	colour(10, 0);
	printf("%s\n", Msg.Subject);

	colour(CFG.HiliteF, CFG.HiliteB);
	colour(14, 1);
	if (Msg.Reply)
		sprintf(Buf1, "\"+\" %s %lu", (char *)Language(211), Msg.Reply);
	if (Msg.Original)
		sprintf(Buf2, "   \"-\" %s %lu", (char *)Language(212), Msg.Original);
	sprintf(Buf3, "%s%s ", Buf1, Buf2);
	colour(14, 1);
	printf("%78s  \n", Buf3);
}




/*
 * Export a email to file in the users home directory.
 */
int Export_a_Email(unsigned long Num)
{
	char	*p;

	LastNum = Num;
	iLineCount = 7;
	WhosDoingWhat(READ_POST);
	Syslog('+', "Export email %d in area %s", Num, sMailbox);

	/*
	 * The area data is already set, so we can do the next things
	 */
	if (EmailBase.Total == 0) {
		colour(15, 0);
		/* There are no messages in this area */
		printf("\n%s\n\n", (char *) Language(205));
		sleep(3);
		return FALSE;
	}

	if (!Msg_Open(sMailpath)) {
		WriteError("Error open JAM base %s", sMailpath);
		return FALSE;
	}

	if (!Msg_ReadHeader(Num)) {
		perror("");
		colour(15, 0);
		printf("\n%s\n\n", (char *)Language(77));
		Msg_Close();
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
			if ((p = (char *)MsgText_First()) != NULL)
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
		fclose(qf);
	} else {
		WriteError("$Can't open %s", p);
		free(p);
	}
	Msg_Close();

	/*
	 * Report the result.
	 */
	colour(CFG.TextColourF, CFG.TextColourB);
	printf("\n\n%s", (char *) Language(46));
	colour(CFG.HiliteF, CFG.HiliteB);
	printf("%s_%lu.msg\n\n", sMailbox, Num);
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
	unsigned long	crc = -1;
	long		id;
	FILE		*fp;

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
	MsgText_Add2((char *)"\001Content-Type: text/plain");
	MsgText_Add2((char *)"\001Content-Transfer-Encoding: 8bit");
	sprintf(temp, "\001X-Mailreader: MBSE BBS %s", VERSION);
	MsgText_Add2(temp);
	p = calloc(81, sizeof(char));
	id = sequencer();
	sprintf(p, "<%08lx@%s>", id, CFG.sysdomain);
	sprintf(temp, "\001Message-id: %s", p);
	MsgText_Add2(temp);
//	sprintf(temp, "\001MSGID: %s %08lx", aka2str(CFG.EmailFidoAka), id);
//	MsgText_Add2(temp);
	Msg.MsgIdCRC = upd_crc32(temp, crc, strlen(temp));
	free(p);
//	sprintf(temp, "\001PID: MBSE-BBS %s", VERSION);
//	MsgText_Add2(temp);

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

	sprintf(temp, "--- MBSE BBS v%s (Linux)", VERSION);
	MsgText_Add2(temp);

	/*
	 * Save if to disk
	 */
	Msg_AddMsg();
	Msg_UnLock();

	ReadExitinfo();
	exitinfo.iPosted++;
	WriteExitinfo();

	do_mailout = TRUE;

	Syslog('+', "Email (%ld) to \"%s\", \"%s\", in mailbox", Msg.Id, Msg.To, Msg.Subject);

	colour(CFG.HiliteF, CFG.HiliteB);
	/* Saving message to disk */
	printf("\n%s(%ld)\n\n", (char *) Language(202), Msg.Id);
	fflush(stdout);
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
	Msg_Close();

        return TRUE;
}



int Read_a_Email(unsigned long Num)
{
	char            *p = NULL, *fn;
	lastread        LR;

	LastNum = Num;
	iLineCount = 7;
	WhosDoingWhat(READ_POST);

	/*
	 * The area data is already set, so we can do the next things
	 */
	if (EmailBase.Total == 0) {
		colour(15, 0);
		/* There are no messages in this area */
		printf("\n%s\n\n", (char *) Language(205));
		sleep(3);
		return FALSE;
	}

	if (!Msg_Open(sMailpath)) {
		WriteError("Error open JAM base %s", sMailpath);
		return FALSE;
	}

	if (!Msg_ReadHeader(Num)) {
		perror("");
		colour(15, 0);
		printf("\n%s\n\n", (char *)Language(77));
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
	if (Msg_Read(Num, 78)) {
		if ((p = (char *)MsgText_First()) != NULL) {
			do {
				if (p[0] == '\001') {
					if (Kludges) {
						colour(7, 0);
						printf("%s\n", p);
						if (CheckLine(CFG.TextColourF, CFG.TextColourB, TRUE))
							break;
					}
				} else {
					colour(CFG.TextColourF, CFG.TextColourB);
					if (strchr(p, '>') != NULL)
						if ((strlen(p) - strlen(strchr(p, '>'))) < 10)
							colour(CFG.HiliteF, CFG.HiliteB);
					printf("%s\n", p);
					if (CheckLine(CFG.TextColourF, CFG.TextColourB, TRUE))
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

	WhosDoingWhat(READ_POST);

	colour(15, 4);
	/* (A)gain, (N)ext, (L)ast, (R)eply, (E)nter, (D)elete, (Q)uit, e(X)port */
	printf("%s", (char *) Language(214));
	if (exitinfo.Security.level >= CFG.sysop_access)
		printf(", (!)");
	printf(": ");
	fflush(stdout);
	alarm_on();
	input = toupper(Getone());

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
		printf("%s\n", (char *) Language(189));
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

	colour(CFG.TextColourF, CFG.TextColourB);
	/* Message area \"%s\" contains %lu messages. */
	printf("\n%s\"%s\" %s%lu %s", (char *) Language(221), sMailbox, (char *) Language(222), EmailBase.Total, (char *) Language(223));

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

	colour(15, 0);
	/* Please enter a message between */
	printf("\n%s(%lu - %lu)", (char *) Language(224), EmailBase.Lowest, EmailBase.Highest);
	/* Message number [ */
	printf("\n%s%lu]: ", (char *) Language(225), Start);

	temp = calloc(128, sizeof(char));
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
	char    to[65];
	char    from[65];
	char    subj[72];
	char    msgid[81];
	char    replyto[81];
	char    replyaddr[81];
	char    *tmp, *buf;
	char    qin[9];
	faddr	*Dest = NULL;

	sprintf(from, "%s", Msg.To);
	sprintf(to, "%s", Msg.From);
	sprintf(replyto, "%s", Msg.ReplyTo);
	sprintf(replyaddr, "%s", Msg.ReplyAddr);

	if (strncasecmp(Msg.Subject, "Re:", 3) && IsReply) {
		sprintf(subj, "Re: %s", Msg.Subject);
	} else {
		sprintf(subj, "%s", Msg.Subject);
	}
	Syslog('m', "Reply msg to %s, subject %s", to, subj);
	Syslog('m', "Msgid was %s", Msg.Msgid);
	sprintf(msgid, "%s", Msg.Msgid);

	x = 0;
	Line = 1;
	WhosDoingWhat(READ_POST);
	clear();
	colour(1,7);
	printf("   %-71s", sMailbox);
	colour(4,7);
	printf("#%-5lu", EmailBase.Highest + 1);

	colour(CFG.HiliteF, CFG.HiliteB);
	sLine();

	for (i = 0; i < (TEXTBUFSIZE + 1); i++)
		Message[i] = (char *) calloc(81, sizeof(char));
	Line = 1;
	Msg_New();

	sprintf(Msg.Replyid, "%s", msgid);
	sprintf(Msg.ReplyTo, "%s", replyto);
	sprintf(Msg.ReplyAddr, "%s", replyaddr);

	/* From     : */
	pout(14, 0, (char *) Language(209));
	if (CFG.EmailMode != E_PRMISP) {
		/*
		 * If not permanent connected to the internet, use fidonet.org style addressing.
		 */
		Dest = fido2faddr(CFG.EmailFidoAka);
		sprintf(Msg.From, "%s@%s (%s)", exitinfo.sUserName, ascinode(Dest, 0x2f), exitinfo.sUserName);
	} else
		sprintf(Msg.From, "%s@%s (%s)", exitinfo.sUserName, CFG.sysdomain, exitinfo.sUserName);
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
	pout(14, 0, (char *) Language(208));
	pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.To);
	Enter(1);

	/* Enter to keep Subject. */
	pout(12, 0, (char *) Language(219));
	Enter(1);
	/* Subject  : */
	pout(14, 0, (char *) Language(210));
	sprintf(Msg.Subject, "%s", subj);
	pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.Subject);

	x = strlen(subj);
	fflush(stdout);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	GetstrP(subj, 50, x);
	fflush(stdout);

	if (strlen(subj))
		strcpy(Msg.Subject, subj);
	tlf(Msg.Subject);

	Msg.Private = TRUE;
	Enter(1);

//	Check_Attach();

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
			if (x) {
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
	faddr	*Dest = NULL;
	int	i;
	char	*orgbox;

	if (HasNoEmail())
		return;

	orgbox = xstrcpy(sMailbox);
	SetEmailArea((char *)"mailbox");

	WhosDoingWhat(READ_POST);
	clear();

	for (i = 0; i < (TEXTBUFSIZE + 1); i++)
		Message[i] = (char *) calloc(81, sizeof(char));
	Line = 1;

        Msg_New();

	colour(9, 0);
	/* Posting message in area: */
	printf("\n%s\"%s\"\n", (char *) Language(156), "mailbox");

	Enter(1);
	/* From   : */
	pout(14, 0, (char *) Language(157));
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
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	printf("%s", Msg.From);
	Syslog('b', "Setting From: %s", Msg.From);

	Enter(1);
	/* To     : */
	pout(14, 0, (char *) Language(158));

	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	GetstrU(Msg.To, 63);

	if ((strcmp(Msg.To, "")) == 0) {
		for(i = 0; i < (TEXTBUFSIZE + 1); i++)
			free(Message[i]);
		SetEmailArea(orgbox);
		return;
	}

	/* Subject  :  */
	pout(14, 0, (char *) Language(161));
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	fflush(stdout);
	alarm_on();
	GetstrP(Msg.Subject, 65, 0);
	tlf(Msg.Subject);

	if((strcmp(Msg.Subject, "")) == 0) {
		Enter(1);
		/* Abort Message [y/N] ?: */
		pout(3, 0, (char *) Language(162));
		fflush(stdout);
		alarm_on();

		if (toupper(Getone()) == Keystroke(162, 0)) {
			for(i = 0; i < (TEXTBUFSIZE + 1); i++)
				free(Message[i]);
			SetEmailArea(orgbox);
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
}



void QuickScan_Email(void)
{
	int     FoundMsg  = FALSE;
	long    i;

	iLineCount = 2;
	WhosDoingWhat(READ_POST);

	if (EmailBase.Total == 0) {
		Enter(1);
		/* There are no messages in this area. */
		pout(15, 0, (char *) Language(205));
		Enter(3);
		sleep(3);
		return;
	}

	clear(); 
	/* #    From                  To                       Subject */
	poutCR(14, 1, (char *) Language(220));

	if (Msg_Open(sMailpath)) {
		for (i = EmailBase.Lowest; i <= EmailBase.Highest; i++) {
			if (Msg_ReadHeader(i)) {
                                
				colour(15, 0);
				printf("%-6lu", Msg.Id);
				colour(3, 0);
				printf("%s ", padleft(Msg.From, 20, ' '));

				colour(2, 0);
				printf("%s ", padleft(Msg.To, 20, ' '));
				colour(5, 0);
				printf("%s", padleft(Msg.Subject, 31, ' '));
				printf("\n");
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
		pout(10, 0, (char *) Language(205));
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
	char	*temp;

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

	pout(15, 0, (char *)"    1"); pout(9, 0, (char *)" . "); pout(3, 0, (char *) Language(467)); printf("\n");
	pout(15, 0, (char *)"    2"); pout(9, 0, (char *)" . "); pout(3, 0, (char *) Language(468)); printf("\n");
	pout(15, 0, (char *)"    3"); pout(9, 0, (char *)" . "); pout(3, 0, (char *) Language(469)); printf("\n");

	pout(CFG.MoreF, CFG.MoreB, (char *) Language(470));
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
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



