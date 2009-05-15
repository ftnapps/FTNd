/*****************************************************************************
 *
 * $Id: mail.c,v 1.69 2008/02/12 19:59:45 mbse Exp $
 * Purpose ...............: Message reading and writing.
 * Todo ..................: Implement message groups.
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
 *****************************************************************************
 *
 * JAM(mbp) - Copyright 1993 Joaquim Homrighausen, Andrew Milner,
 *			     Mats Birch, Mats Wallin.
 *			     ALL RIGHTS RESERVED.
 *
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/nodelist.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "mail.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "oneline.h"
#include "exitinfo.h"
#include "fsedit.h"
#include "filesub.h"
#include "msgutil.h"
#include "pop3.h"
#include "email.h"
#include "door.h"
#include "whoson.h"
#include "term.h"
#include "ttyio.h"
#include "openport.h"



/*
 * Global variables
 */
unsigned int	LastNum;		/* Last read message number	    */
int		Kludges = FALSE;	/* Show kludges or not		    */
int		Line = 1;		/* Line counter in editor	    */
char		*Message[TEXTBUFSIZE +1];/* Message compose text buffer	    */
FILE		*qf;			/* Quote file			    */
extern int	do_mailout;
extern int	LC_Wrote;		/* Lastcaller info write message    */
extern int	rows;
extern unsigned int	mib_posted;


/*
 *  Internal prototypes
 */
void    ShowMsgHdr(void);		/* Show message header              */
int	Read_a_Msg(unsigned int Num, int);/* Read a message		    */
int	Export_a_Msg(unsigned int Num);/* Export message to homedir	    */
int	ReadPanel(void);		/* Read panel bar		    */
int	Save_Msg(int, faddr *);		/* Save a message		    */
int	Save_CC(int, char *);		/* Save carbon copy		    */
void	Reply_Msg(int);			/* Reply to message		    */
void	Delete_MsgNum(unsigned int);	/* Delete specified message	    */
int	CheckUser(char *);		/* Check if user exists		    */
int	IsMe(char *);			/* Test if this is my userrecord    */


/****************************************************************************/


/* 
 * More prompt, returns 1 if user decides not to look any further.
 */
int LC(int Lines)
{
    int	z;

    iLineCount += Lines;
 
    if (iLineCount >= rows && iLineCount < 1000) {
	iLineCount = 1;

	pout(CFG.MoreF, CFG.MoreB, (char *) Language(61));
	alarm_on();
	z = toupper(Readkey());

	if (z == Keystroke(61, 1)) {
	    Enter(1);
	    return(1);
	}

	if (z == Keystroke(61, 2))
	    iLineCount = 50000;

	Blanker(strlen(Language(61)));
    }

    return(0);
}



/*
 * Check if posting is allowed
 */
int Post_Allowed(void);
int Post_Allowed(void)
{
    if (msgs.MsgKinds == RONLY) {
	Enter(2);
	/* Posting not allowed, this area is Read Only! */
	pout(LIGHTRED, BLACK, (char *) Language(437));
	Enter(1);
	sleep(3);
	return FALSE;
    }
    if (Access(exitinfo.Security, msgs.WRSec) == FALSE) {
	Enter(2);
	/* No Write access to area */
	pout(LIGHTRED, BLACK, (char *) Language(453));
	Enter(1);
	sleep(3);
	return FALSE;
    }
    return TRUE;
}



/*
 * Check if Alias is allowed, and if so if the user
 * wants to use this in the From: field.
 */
int Alias_Option(void);
int Alias_Option(void)
{
    int	    rc = TRUE;

    if (!msgs.Aliases)
	return FALSE;
    if (strlen(exitinfo.sHandle) == 0)
	return FALSE;
    
    /* Use your alias ( */ 
    pout(CYAN, BLACK, (char *)Language(477));
    pout(CYAN, BLACK, exitinfo.sHandle);
    /* YN|) to post this message [Y/n]: */
    pout(CYAN, BLACK, (char *)Language(478));
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    alarm_on();
    if (toupper(Readkey()) == Keystroke(478, 1)) {
	rc = FALSE;
	PUTCHAR(Keystroke(478, 1));
    } else {
	PUTCHAR(Keystroke(478, 0));
    }
    Enter(2);
    return rc;
}



/*
 * Check if netmail may be send crash or immediate.
 */
int Crash_Option(faddr *);
int Crash_Option(faddr *Dest)
{
    node	    *Nlent;
    int		    rc = 0;
    unsigned short  point;

    if (exitinfo.Security.level < CFG.iCrashLevel)
	return 0;

    point = Dest->point;
    Dest->point = 0;

    if (((Nlent = getnlent(Dest)) != NULL) && (Nlent->addr.zone)) {
	if ((Nlent->can_pots && Nlent->is_cm) || (Nlent->can_ip && Nlent->is_icm)) {
	    /* Crash [y/N]: */
	    pout(CYAN, BLACK, (char *)Language(461));
	    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	    alarm_on();
	    if (toupper(Readkey()) == Keystroke(461, 0)) {
		rc = 1;
		PUTCHAR(Keystroke(461, 0));
	    } else
		PUTCHAR(Keystroke(461, 1));
	} else {
	    /* Warning: node is not CM, send Immediate [y/N]: */
	    pout(CYAN, BLACK, (char *)Language(462));
	    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	    alarm_on();
	    if (toupper(Readkey()) == Keystroke(462, 0)) {
		rc = 2;
		PUTCHAR(Keystroke(462, 0));
	    } else
		PUTCHAR(Keystroke(462, 1));
	}

	if (Nlent->addr.domain)
	    free(Nlent->addr.domain);
	Nlent->addr.domain = NULL;
	if (Nlent->url)
	    free(Nlent->url);
	Nlent->url = NULL;
    } else {
	Syslog('+', "Node %s not found", ascfnode(Dest, 0x1f));
	rc = -1;
    }

    Dest->point = point;
    return rc;
}



/*
 * Ask if message must be private, only allowed in areas which allow
 * both public and private. Private areas are forced to private.
 */
int IsPrivate(void);
int IsPrivate(void)
{
    int	rc = FALSE;

    if (msgs.MsgKinds == BOTH) {
	Enter(1);
        /* Private [y/N]: */
        pout(CYAN, BLACK, (char *) Language(163));
        alarm_on();
        if (toupper(Readkey()) == Keystroke(163, 0)) {
            rc = TRUE;
	    PUTCHAR(Keystroke(163, 0));
	} else {
	    PUTCHAR(Keystroke(163, 1));
	}
    }

    /*
     * Allways set the private flag in Private areas.
     */
    if (msgs.MsgKinds == PRIVATE)
        rc = TRUE;

    return rc;
}



void Check_Attach(void);
void Check_Attach(void)
{
    char	*Attach, *dospath, msg[81];
    struct stat	sb;

    /*
     * This is a dangerous option! Every file on the system to which the
     * bbs has read access and is in the range of paths translatable by
     * Unix to DOS can be attached to the netmail.
     */
    if ((msgs.Type == NETMAIL) && (exitinfo.Security.level >= CFG.iAttachLevel)) {

	Attach = calloc(PATH_MAX, sizeof(char));
	while (TRUE) {
	    Enter(1);
	    /* Attach file [y/N]: */
	    pout(CYAN, BLACK, (char *)Language(463));
	    alarm_on();
	    if (toupper(Readkey()) == Keystroke(463, 0)) {

		PUTCHAR(Keystroke(463, 0));
		Enter(1);
		/* Please enter filename: */
		pout(YELLOW, BLACK, (char *)Language(245));
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		alarm_on();
		snprintf(Attach, PATH_MAX, "%s/", CFG.uxpath);
		PUTSTR(Attach);
		GetstrP(Attach, 71, strlen(Attach));
		if (strcmp(Attach, "") == 0)
		    break;

		if ((stat(Attach, &sb) == 0) && (S_ISREG(sb.st_mode))) {
		    dospath = xstrcpy(Unix2Dos(Attach));
		    if (strncasecmp(Attach, CFG.uxpath, strlen(CFG.uxpath)) == 0) {
			Syslog('+', "FileAttach \"%s\"", Attach);
			if (strlen(CFG.dospath))
			    strcpy(Msg.Subject, dospath);
			else
			    snprintf(Msg.Subject, 101, "%s", Attach);
			Msg.FileAttach = TRUE;
			Enter(1);
			/* File */ /* will be attached */
			snprintf(msg, 81, "%s %s %s", (char *)Language(464), Msg.Subject, Language(465));
			pout(LIGHTCYAN, BLACK, msg);
			Enter(1);
			sleep(2);
			break;
		    } else {
			Enter(1);
			/* File not within */
			snprintf(msg, 81, "%s \"%s\"", Language(466), CFG.uxpath);
			pout(LIGHTGREEN, BLACK, msg);
			Enter(1);
			Pause();
		    }
		} else {
		    Enter(1);
		    /* File does not exist, please try again ... */
		    pout(LIGHTGREEN, BLACK, (char *)Language(296));
		    Enter(1);
		    Pause();
		}
	    } else {
		break;
	    } /* if attach */
	} /* while true */
	free(Attach);
    }
}



/*
 * Comment to sysop
 */
void SysopComment(char *Cmt)
{
    unsigned int    tmp;
    char	    *temp;
    FILE	    *fp;

    tmp = iMsgAreaNumber;

    /*
     *  Make sure that the .quote file is empty.
     */
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/%s/.quote", CFG.bbs_usersdir, exitinfo.Name);
    if ((fp = fopen(temp, "w")) != NULL)
	fclose(fp);
    free(temp);

    SetMsgArea(CFG.iSysopArea -1);
    snprintf(Msg.From, 101, "%s", CFG.sysop_name);
    snprintf(Msg.Subject, 101, "%s", Cmt);
    Reply_Msg(FALSE);

    SetMsgArea(tmp);
}



/*
 * Edit a message. Call the users preffered editor.
 */
int Edit_Msg()
{
    switch (exitinfo.MsgEditor) {
	case X_LINEEDIT:    return Fs_Edit();
	case FSEDIT:	    return Fs_Edit();
	case EXTEDIT:	    return Ext_Edit();
    }
    return 0;
}



/*
 *  Post a message, called from the menu or ReadPanel().
 */
void Post_Msg()
{
    int		    i, x, cc;
    char	    *FidoNode, msg[81];
    faddr	    *Dest = NULL;
    node	    *Nlent = NULL;
    unsigned short  point;

    Line = 1;
    WhosDoingWhat(READ_POST, NULL);
    SetMsgArea(iMsgAreaNumber);

    clear();
    if (!Post_Allowed())
	return;
	
    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	Message[i] = (char *) calloc(MAX_LINE_LENGTH +1, sizeof(char));
    Line = 1;

    Msg_New();

    Enter(1);
    /* Posting message in area: */
    snprintf(msg, 81, "%s\"%s\"", (char *) Language(156), sMsgAreaDesc);
    pout(LIGHTBLUE, BLACK, msg);
    Enter(1);

    if (Alias_Option()) {
	/*
         * Set Handle
         */
        strcpy(Msg.From, exitinfo.sHandle);
        tlcap(Msg.From); // Do we want this???
        /* From     : */
        pout(YELLOW, BLACK, (char *) Language(157));
    } else {
        /*
         * Normal from address, no alias
         */
        /* From     : */
        pout(YELLOW, BLACK, (char *) Language(157));
        if (msgs.Type == NEWS) {
	   if (CFG.EmailMode != E_PRMISP) {
		/*
		 * If no internet mail domain, use fidonet addressing.
		 */
		Dest = fido2faddr(CFG.EmailFidoAka);
		strcpy(Msg.From, exitinfo.sUserName);
		tlcap(Msg.From);
	    } else {
		snprintf(Msg.From, 101, "%s@%s (%s)", exitinfo.Name, CFG.sysdomain, exitinfo.sUserName);
	    }
	} else {
	    strcpy(Msg.From, exitinfo.sUserName);
	    tlcap(Msg.From);
	}
    }

    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.From);
    Syslog('b', "Setting From: %s", Msg.From);

    if ((msgs.Type == NEWS) || (msgs.Type == LIST)) {
        /*
	 * Newsmode or maillist mode, automatic addressing to All.
	 *                                   */
	strcpy(Msg.To, "All");
    } else {
	while (TRUE) {
	    Enter(1);
	    /* To     : */
	    pout(YELLOW, BLACK, (char *) Language(158));
	
	    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	    Getname(Msg.To, 35);
	
	    if ((strcmp(Msg.To, "")) == 0) {
		for(i = 0; i < (TEXTBUFSIZE + 1); i++)
		    free(Message[i]);
		return;
	    }

	    if ((strcasecmp(Msg.To, "sysop")) == 0)
		strcpy(Msg.To, CFG.sysop_name);

	    /*
	     * Localmail and Echomail may be addressed to All
	     */
	    if ((msgs.Type == LOCALMAIL) || (msgs.Type == ECHOMAIL) || (msgs.Type == LIST)) {
		if (strcasecmp(Msg.To, "all") == 0) 
		    x = TRUE;
		else {
		    /*
		     * Local users must exist in the userbase.
		     */
		    if (msgs.Type == LOCALMAIL) {
			/* Verifying user ... */
			pout(CYAN, BLACK, (char *) Language(159));
			x = CheckUser(Msg.To);
		    } else
			x = TRUE;
		}
	    } else if (msgs.Type == NETMAIL) {
		x = FALSE;
		Enter(1);
		pout(YELLOW, BLACK, (char *)"Address  : ");
		FidoNode = calloc(61, sizeof(char));
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		GetstrC(FidoNode, 60);

		if ((Dest = parsefnode(FidoNode)) != NULL) {
		    point = Dest->point;
		    Dest->point = 0;
		    if (((Nlent = getnlent(Dest)) != NULL) && (Nlent->addr.zone)) {
			if (Nlent->url)
			    free(Nlent->url);
			Nlent->url = NULL;
			if (Nlent->addr.domain)
			    free(Nlent->addr.domain);
			Nlent->addr.domain = NULL;

			colour(YELLOW, BLACK);
			if (point)
			    PUTSTR((char *)"Boss     : ");
			else
			    PUTSTR((char *)"Node     : ");
			Dest->point = point;
			snprintf(msg, 81, "%s in %s", Nlent->name, Nlent->location);
			pout(CFG.MsgInputColourF, CFG.MsgInputColourB, msg);
			/* " Is this correct [y/N]: " */
			pout(YELLOW, BLACK, (char *)Language(21));
			colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
			alarm_on();

			if (toupper(Readkey()) == Keystroke(21, 0)) {
			    Enter(1);
			    snprintf(Msg.ToAddress, 101, "%s", ascfnode(Dest, 0x1f));
			    x = TRUE;
			    switch (Crash_Option(Dest)) {
				case 1:	Msg.Crash = TRUE;
					break;
				case 2:	Msg.Immediate = TRUE;
					break;
			    }
			}
		    } else {
			Dest->point = point;
			Enter(1);
			/* Node not known, continue anayway [y/N]: */
			pout(CYAN, BLACK, (char *) Language(241));
			alarm_on();
			if (toupper(Readkey()) == Keystroke(241, 0)) {
			    x = TRUE;
			    Syslog('+', "Node %s not found, forced continue", FidoNode);
			}
		    }
		} else {
		    Syslog('m', "Can't parse address %s", FidoNode);
		}
		free(FidoNode);
	    } else {
		x = FALSE;
	    }

	    if(!x) {
		PUTCHAR('\r');
		/* User not found. Try again, or (Enter) to quit */
		pout(CYAN, BLACK, (char *) Language(160));
	    } else
		break;
	}
    }

    Enter(1);
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
	    for(i = 0; i < (TEXTBUFSIZE + 1); i++)
		free(Message[i]);
	    tidy_faddr(Dest);
	    return;
	}
    }

    /*
     * If not addressed to "all" and the area is Private/Public we
     * ask the user for the private flag.
     */
    if ((strcasecmp(Msg.To, "all")) != 0)
	Msg.Private = IsPrivate();

    Check_Attach();

    if (Edit_Msg()) {
	Enter(1);
	if (msgs.Type == NETMAIL) {
	    /*
	     * Check for Carbon Copy lines, process them if present.
	     */
	    cc = 0;
	    for (i = 1; i <= Line; i++) {
	        if (strncasecmp(Message[i], "cc: ", 4)) {
		    break;
		} else {
		    cc++;
		}
	    }
	    Syslog('b', "CC: detected %d", cc);
	    if (cc) {
		/*
		 * Carbon copies, modify the text to show the presence of CCs.
		 */
		for (i = Line; i; i--) {
		    Syslog('b', "%02d: \"%s\"", i, printable(Message[i], 0));
		    snprintf(Message[i + 1], TEXTBUFSIZE +1, Message[i]);
		}
		Line++;
		snprintf(Message[1], TEXTBUFSIZE +1, " +: Original message to %s", ascfnode(Dest, 0x4f));
		for (i = 1; i <= Line; i++) {
		    Syslog('b', "%02d: \"%s\"", i, printable(Message[i], 0));
		}
		/*
		 * First sent to original destination
		 */
		Save_Msg(FALSE, Dest);
		/*
		 * Now sent copies
		 */
		for (i = 0; i < cc; i++) {
		    Save_CC(FALSE, Message[i+2]);
		}
	    } else {
		Save_Msg(FALSE, Dest);
	    }
	} else {
	    Save_Msg(FALSE, Dest);
	}
	Enter(1);
	sleep(2);
    }
    
    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	free(Message[i]);
    tidy_faddr(Dest);
}



/*
 * Save a Carbon Copy
 * The ccline should have the format "cc: Firstname Lastname z:n/n.p"
 */
int Save_CC(int IsReply, char *ccline)
{
    faddr	    *Dest = NULL;
    int		    i, j, x, rc = FALSE;
    char	    *p, *username, msg[81];
    unsigned short  point;
    node	    *Nlent;
    
    Syslog('b', "Save_CC(%s, %s)",  IsReply ?"TRUE":"FALSE", ccline);

    /*
     * Reformat the line and extract username and node
     */
    i = 4;
    j = strlen(ccline);
    while (ccline[i] == ' ')
	i++;
    while (ccline[j] != ' ')
	j--;
    Syslog('b', "i=%d, j=%d", i, j);
    if (j <= i) {
	Syslog('+', "Could not parse %s", printable(ccline, 0));
	/* Could not parse */
	snprintf(msg, 81, "%s \"%s\"", Language(22), printable(ccline, 0));
	pout(LIGHTRED, BLACK, msg);
	Enter(1);
	Pause();
	return FALSE;
    }

    username = calloc(j - i + 1, sizeof(char));
    strncpy(username, ccline+i, j - i);
    Syslog('b', "Username: \"%s\"", printable(username, 0));
    while (*(p = username + strlen(username) -1) == ' ')
	*p = '\0';
    Syslog('b', "Username: \"%s\"", tlcap(printable(username, 0)));
    
    if (strlen(username) == 0) {
	Syslog('+', "Could not extract username from %s", printable(ccline, 0));
	/* Could not parse */
	snprintf(msg, 81, "%s \"%s\"", Language(22), printable(ccline, 0));
	pout(LIGHTRED, BLACK, msg);
	Enter(1);
	Pause();
	return FALSE;
    }

    if ((Dest = parsefnode(ccline + j)) == NULL) {
	Syslog('+', "Could not extract address from %s", printable(ccline, 0));
	/* Could not parse */
	snprintf(msg, 81, "%s \"%s\"", Language(22), printable(ccline, 0));
	pout(LIGHTRED, BLACK, msg);
	Enter(1);
	Pause();
	return FALSE;
    }

    Dest->name = tlcap(printable(username, 0));
    Syslog('b', "Dest %s", ascfnode(Dest, 0xff));
    Enter(1);
    snprintf(msg, 81, "Confirm CC to %s", ascfnode(Dest, 0xff));
    pout(LIGHTMAGENTA, BLACK, msg);
    Enter(1);

    x = FALSE;
    point = Dest->point;
    Dest->point = 0;
    if (((Nlent = getnlent(Dest)) != NULL) && (Nlent->addr.zone)) {
	colour(YELLOW, BLACK);
	if (point)
	    PUTSTR((char *)"Boss     : ");
	else
	    PUTSTR((char *)"Node     : ");
	Dest->point = point;
	snprintf(msg, 81, "%s in %s", Nlent->name, Nlent->location);
	pout(CFG.MsgInputColourF, CFG.MsgInputColourB, msg);
	/* " Is this correct [y/N]: " */
	pout(YELLOW, BLACK, (char *)Language(21));
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	alarm_on();
    
	if (toupper(Readkey()) == Keystroke(21, 0)) {
	    Enter(1);
	    snprintf(Msg.ToAddress, 101, "%s", ascfnode(Dest, 0x1f));
	    x = TRUE;
	    switch (Crash_Option(Dest)) {
		case 1: Msg.Crash = TRUE;
			break;
		case 2: Msg.Immediate = TRUE;
			break;
	    }
	}
    } else {
	Dest->point = point;
	Enter(1);
	/* Node not known, continue anayway [y/N]: */
	pout(CYAN, BLACK, (char *) Language(241));
	alarm_on();
	if (toupper(Readkey()) == Keystroke(241, 0)) {
	    x = TRUE;
	    Syslog('+', "Node %s not found, forced continue", ascfnode(Dest, 0x0f));
	}
    }

    if (x) {
	Enter(1);
	rc = Save_Msg(IsReply, Dest);
    }
    
    tidy_faddr(Dest);
    free(username);
    return rc;
}



/*
 *  Save the message to disk.
 */
int Save_Msg(int IsReply, faddr *Dest)
{
    int	    i;
    char    *temp;
    FILE    *fp;

    Syslog('b', "Entering Save_Msg() Line=%d, Dest=%s", Line, (Dest == NULL)?"NULL":"valid");

    if (Line < 2)
	return TRUE;

    /* Saving message to disk */
    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(202));

    if (!Open_Msgbase(msgs.Base, 'w')) {
	WriteError("Failed to open msgbase \"%s\"", msgs.Base);
	return FALSE;
    }

    Msg.Written = Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
    Msg.Local = TRUE;
    temp = calloc(PATH_MAX, sizeof(char));

    if (strlen(Msg.ReplyTo) && (msgs.Type == NETMAIL)) {
	/*
	 *  Send message to internet gateway.
	 */
	Syslog('m', "UUCP message to %s", Msg.ReplyAddr);
	snprintf(Msg.To, 101, "UUCP");
	Add_Headkludges(Dest, IsReply);
	snprintf(temp, 101, "To: %s", Msg.ReplyAddr);
	MsgText_Add2(temp);
	MsgText_Add2((char *)"");
    } else {
	Add_Headkludges(Dest, IsReply);
    }

    /*
     * Add message text
     */
    for (i = 1; i <= Line; i++)
	MsgText_Add2(Message[i]);

    Add_Footkludges(TRUE, NULL, FALSE);

    /*
     * Save if to disk
     */
    Msg_AddMsg();
    Msg_UnLock();

    snprintf(temp, 81, " (%d)", Msg.Id);
    PUTSTR(temp);
    Enter(1);
		
    ReadExitinfo();
    exitinfo.iPosted++;
    mib_posted ++;
    WriteExitinfo();

    LC_Wrote = TRUE;

    Syslog('+', "Msg (%ld) to \"%s\", \"%s\", in %d", Msg.Id, Msg.To, Msg.Subject, iMsgAreaNumber + 1);

    msgs.LastPosted = time(NULL);
    msgs.Posted.total++;
    msgs.Posted.tweek++;
    msgs.Posted.tdow[Diw]++;
    msgs.Posted.month[Miy]++;

    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	
    if ((fp = fopen(temp, "r+")) != NULL) {
	fseek(fp, msgshdr.hdrsize + (iMsgAreaNumber * (msgshdr.recsize + msgshdr.syssize)), SEEK_SET);
	fwrite(&msgs, msgshdr.recsize, 1, fp);
	fclose(fp);
    }

    if (strlen(msgs.Group)) {
	snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r+")) != NULL) {
	    fread(&mgrouphdr, sizeof(mgrouphdr), 1, fp);
	    while ((fread(&mgroup, mgrouphdr.recsize, 1, fp)) == 1) {
		if (!strcmp(msgs.Group, mgroup.Name)) {
		    mgroup.LastDate = time(NULL);
		    mgroup.MsgsSent.total++;
		    mgroup.MsgsSent.tweek++;
		    mgroup.MsgsSent.tdow[l_date->tm_wday]++;
		    mgroup.MsgsSent.month[l_date->tm_mon]++;
		    fseek(fp, - mgrouphdr.recsize, SEEK_CUR);
		    fwrite(&mgroup, mgrouphdr.recsize, 1, fp);
		    break;
		}
	    }
	    fclose(fp);
	} else {
	    WriteError("$Save_Msg(): Can't open %s", temp);
	}
    }


    /*
     * Add quick mailscan info
     */
    if (msgs.Type != LOCALMAIL) {
	do_mailout = TRUE;
	snprintf(temp, PATH_MAX, "%s/tmp/%smail.jam", getenv("MBSE_ROOT"), 
		((msgs.Type == ECHOMAIL) || (msgs.Type == LIST))? "echo" : "net");
	if ((fp = fopen(temp, "a")) != NULL) {
	    fprintf(fp, "%s %u\n", msgs.Base, Msg.Id);
	    fclose(fp);
	}
    }
    free(temp);
    Close_Msgbase(msgs.Base);

    SetMsgArea(iMsgAreaNumber);
    return TRUE;
}



/* 
 * Show message header screen top for reading messages.
 */
void ShowMsgHdr(void)
{
    static char	Buf1[35], Buf2[35], Buf3[81];
    char	msg[81];
    struct tm	*tm;
    time_t	now;
    int		color;

    Buf1[0] = '\0';
    Buf2[0] = '\0';
    Buf3[0] = '\0';

    clear();
    snprintf(msg, 81, "   %-70s", sMsgAreaDesc);
    pout(BLUE, LIGHTGRAY, msg);

    snprintf(msg, 81,"#%-5u", Msg.Id);
    pout(RED, LIGHTGRAY, msg);
    Enter(1);

    /* Date     : */
    pout(YELLOW, BLACK, (char *) Language(206));
    colour(GREEN, BLACK);
    /* Use intermediate variable to prevent SIGBUS on Sparc's */
    now = Msg.Written;
    tm = gmtime(&now);
    snprintf(msg, 81, "%02d-%02d-%d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon+1, 
		tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
    PUTSTR(msg);

    colour(LIGHTRED, BLACK);
    if (Msg.Local)	    PUTSTR((char *)" Local");
    if (Msg.Intransit)	    PUTSTR((char *)" Transit");
    if (Msg.Private)	    PUTSTR((char *)" Priv.");
    if (Msg.Received)	    PUTSTR((char *)" Rcvd");
    if (Msg.Sent)	    PUTSTR((char *)" Sent");
    if (Msg.KillSent)	    PUTSTR((char *)" KillSent");
    if (Msg.ArchiveSent)    PUTSTR((char *)" ArchiveSent");
    if (Msg.Hold)	    PUTSTR((char *)" Hold");
    if (Msg.Crash)	    PUTSTR((char *)" Crash");
    if (Msg.Immediate)	    PUTSTR((char *)" Imm.");
    if (Msg.Direct)	    PUTSTR((char *)" Dir");
    if (Msg.Gate)	    PUTSTR((char *)" Gate");
    if (Msg.FileRequest)    PUTSTR((char *)" Freq");
    if (Msg.FileAttach)	    PUTSTR((char *)" File");
    if (Msg.TruncFile)	    PUTSTR((char *)" TruncFile");
    if (Msg.KillFile)	    PUTSTR((char *)" KillFile");
    if (Msg.ReceiptRequest) PUTSTR((char *)" RRQ");
    if (Msg.ConfirmRequest) PUTSTR((char *)" CRQ");
    if (Msg.Orphan)	    PUTSTR((char *)" Orphan");
    if (Msg.Encrypt)	    PUTSTR((char *)" Crypt");
    if (Msg.Compressed)	    PUTSTR((char *)" Comp");
    if (Msg.Escaped)	    PUTSTR((char *)" 7bit");
    if (Msg.ForcePU)	    PUTSTR((char *)" FPU");
    if (Msg.Localmail)	    PUTSTR((char *)" Localmail");
    if (Msg.Netmail)	    PUTSTR((char *)" Netmail");
    if (Msg.Echomail)	    PUTSTR((char *)" Echomail");
    if (Msg.News)	    PUTSTR((char *)" News");
    if (Msg.Email)	    PUTSTR((char *)" E-mail");
    if (Msg.Nodisplay)	    PUTSTR((char *)" Nodisp");
    if (Msg.Locked)	    PUTSTR((char *)" LCK");
    if (Msg.Deleted)	    PUTSTR((char *)" Del");
    Enter(1);

    /* From    : */
    pout(YELLOW, BLACK, (char *) Language(209));
    if (IsMe(Msg.From))
	color = LIGHTGREEN;
    else
        color = GREEN;
    colour(color++, BLACK);
    PUTSTR(chartran(Msg.From));
    if (iMsgAreaType != LOCALMAIL) {
	snprintf(msg, 81, " (%s)", Msg.FromAddress);
	pout(color, BLACK, msg);
    }
    Enter(1);

    /* To      : */
    pout(YELLOW, BLACK, (char *) Language(208));
    if (IsMe(Msg.To))
        color = LIGHTGREEN;
    else
        color = GREEN;
    colour(color++, BLACK);
    PUTSTR(chartran(Msg.To));
    if (iMsgAreaType == NETMAIL) {
	snprintf(msg, 81, " (%s)", Msg.ToAddress);
	pout(color, BLACK, msg);
    }
    Enter(1);

    /* Subject : */
    pout(YELLOW, BLACK, (char *) Language(210));
    colour(GREEN, BLACK);
    PUTSTR(chartran(Msg.Subject));
    Enter(1);

    colour(CFG.HiliteF, CFG.HiliteB);
    colour(YELLOW, BLUE);
    if (Msg.Reply)
	snprintf(Buf1, 35, "\"+\" %s %u", (char *)Language(211), Msg.Reply);
    if (Msg.Original)
	snprintf(Buf2, 35, "   \"-\" %s %u", (char *)Language(212), Msg.Original);
    snprintf(Buf3, 35, "%s%s ", Buf1, Buf2);
    snprintf(msg, 81, "%77s  ", Buf3);
    pout(YELLOW, BLUE, msg);
    Enter(1);
}



/*
 * Export a message to file in the users home directory or to the rules directory.
 */
int Export_a_Msg(unsigned int Num)
{
    char    *p, msg[81];
    int     ShowMsg = TRUE, z, homedir = TRUE;

    LastNum = Num;
    iLineCount = 7;
    WhosDoingWhat(READ_POST, NULL);

    /*
     * The sysop has a choice to export to the rules directory.
     */
    if ((exitinfo.Security.level >= CFG.sysop_access) && strlen(msgs.Tag)) {
	while (TRUE) {
	    Enter(2);
	    /* Export to (H)ome or (R)ules directory: */
	    pout(WHITE, RED, (char *) Language(11));
	    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	    alarm_on();
	    z = toupper(Readkey());
	    if (z == Keystroke(11, 0)) {
		PUTCHAR(Keystroke(478, 0));
		break;
	    }
	    if (z == Keystroke(11, 1)) {
		PUTCHAR(Keystroke(478, 1));
		homedir = FALSE;
		break;
	    }
	}
    }
    
    Syslog('+', "Export msg %d in area #%d (%s) to %s directory", Num, iMsgAreaNumber + 1, sMsgAreaDesc, homedir?"Home":"Rules");

    /*
     * The area data is already set, so we can do the next things
     */
    if (MsgBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(2);
	Syslog('+', "No messages in area");
	sleep(3);
	return FALSE;
    }

    if (!Msg_Open(sMsgAreaBase)) {
	WriteError("Error open JAM base %s", sMsgAreaBase);
	return FALSE;
    }

    if (!Msg_ReadHeader(Num)) {
	Enter(1);
	/* Message doesn't exist */
	pout(WHITE, BLACK, (char *)Language(77));
	Enter(2);
	Msg_Close();
	sleep(3);
	return FALSE;
    }

    if (Msg.Private) {
	ShowMsg = FALSE;
	if ((strcasecmp(exitinfo.sUserName, Msg.From) == 0) || (strcasecmp(exitinfo.sUserName, Msg.To) == 0))
	    ShowMsg = TRUE;
	if (exitinfo.Security.level >= msgs.SYSec.level)
	    ShowMsg = TRUE;
    }

    if (!ShowMsg) {
	Enter(1);
	/* Private message, not owner */
	pout(WHITE, BLACK, (char *) Language(82));
	Enter(2);
	Msg_Close();
	sleep(3);
	return FALSE;
    }

    /*
     * Export the message text to the file in the users home/wrk directory
     * or to the rules directory.
     * Create the filename as <areanum>_<msgnum>.msg if exported to the
     * home directory, if exported to a rule directory the name is the area tag.
     * The message is written in M$DOS <cr/lf> format.
     */
    p = calloc(PATH_MAX, sizeof(char));
    if (homedir)
	snprintf(p, PATH_MAX, "%s/%s/wrk/%d_%u.msg", CFG.bbs_usersdir, exitinfo.Name, iMsgAreaNumber + 1, Num);
    else
	snprintf(p, PATH_MAX, "%s/%s", CFG.rulesdir, msgs.Tag);

    if ((qf = fopen(p, "w")) != NULL) {
	free(p);
	p = NULL;
	if (Msg_Read(Num, 80)) {
	    if ((p = (char *)MsgText_First()) != NULL) {
		do {
		    if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
			if (Kludges && homedir) {
			    if (p[0] == '\001') {
				p[0] = 'a';
				fprintf(qf, "^%s\r\n", p);
			    } else {
				fprintf(qf, "%s\r\n", p);
			    }
			}
		    } else {
			if (homedir || (strncmp(p, "--- ", 4) && strncmp(p, " * Origin", 9))) {
			    /*
			     * In rules directory, don't write tearline and Origin line.
			     */
			    fprintf(qf, "%s\r\n", p);
			}
		    }
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	}
	fclose(qf);
    } else {
	WriteError("$Can't open %s", p);
    }
    Msg_Close();

    /*
     * Report the result.
     */
    Enter(2);
    if (homedir) {
	/* Message exported to your private directory as: */
	pout(CFG.TextColourF, CFG.TextColourB, (char *) Language(46));
	snprintf(msg, 81, "%d_%u.msg", iMsgAreaNumber + 1, Num);
	pout(CFG.HiliteF, CFG.HiliteB, msg);
    } else {
	/* Message exported to rules directory as */
	pout(CFG.TextColourF, CFG.TextColourB, (char *) Language(12));
	pout(CFG.HiliteF, CFG.HiliteB, msgs.Tag);
    }
    Enter(2);
    Pause();
    return TRUE;
}



/*
 * Read a message on screen. Update the lastread pointers,
 * except while scanning and reading new mail at logon.
 */
int Read_a_Msg(unsigned int Num, int UpdateLR)
{
    char	*p = NULL, *fn, *charset = NULL, *charsin = NULL;
    int		ShowMsg = TRUE;
    lastread	LR;
    unsigned int	mycrc;

    LastNum = Num;
    iLineCount = 7;
    WhosDoingWhat(READ_POST, NULL);

    /*
     * The area data is already set, so we can do the next things
     */
    if (MsgBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
	return FALSE;
    }

    if (!Msg_Open(sMsgAreaBase)) {
	WriteError("Error open JAM base %s", sMsgAreaBase);
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
    
    if (Msg.Private) {
	ShowMsg = FALSE;
	if ((strcasecmp(exitinfo.sUserName, Msg.From) == 0) || (strcasecmp(exitinfo.sUserName, Msg.To) == 0))
	    ShowMsg = TRUE;
	if (exitinfo.Security.level >= msgs.SYSec.level)
	    ShowMsg = TRUE;
    } 
    if (!ShowMsg) {
	Enter(1);
	pout(WHITE, BLACK, (char *) Language(82));
	Enter(2);
	Msg_Close();
	sleep(3);
	return FALSE;
    }

    /*
     * Analyse this message to find the character encoding.
     */
    if (Msg_Read(Num, 78)) {
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
		    if (strncmp(p, "\001CHRS: ", 7) == 0) {
			charset = xstrcpy(p + 7);
			break;
		    }
		    if (strncmp(p, "\001CHARSET: ", 10) == 0) {
			charset = xstrcpy(p + 10);
			break;
		    }
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }

    if (charset == NULL) {
	/*
	 * No charset marked in the message, we are going to asume
	 * that an ancient dos program is used with IBMPC characters.
	 * This means CP437.
	 */
	charsin = xstrcpy((char *)"CP437");
    } else {
	charsin = xstrcpy(get_ic_ftn(find_ftn_charset(charset)));
    }
    chartran_init(charsin, get_ic_ftn(exitinfo.Charset), 'b');

    /*
     * Fill Quote file in case the user wants to reply. Note that line
     * wrapping is set lower then normal message read, to create room
     * for the Quote> strings at the start of each line. The text is
     * converted to the users local characterset so the reply can be
     * done with the users prefered characterset.
     */
    fn = calloc(PATH_MAX, sizeof(char));
    snprintf(fn, PATH_MAX, "%s/%s/.quote", CFG.bbs_usersdir, exitinfo.Name);
    if ((qf = fopen(fn, "w")) != NULL) {
	if (Msg_Read(Num, 75)) {
	    if ((p = (char *)MsgText_First()) != NULL) {
		do {
		    if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
			if (Kludges) {
			    if (p[0] == '\001') {
				p[0] = 'a';
				fprintf(qf, "^%s\n", chartran(p));
			    } else {
				fprintf(qf, "%s\n", chartran(p));
			    }
			}
		    } else {
			fprintf(qf, "%s\n", chartran(p));
		    }
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	}
	fclose(qf);
    } else {
	WriteError("$Can't open %s", p);
    }
    free(fn);

    /*
     * Show message header with charset mapping if needed.
     */
    ShowMsgHdr();

    /*
     * Show message text
     */
    colour(CFG.TextColourF, CFG.TextColourB);
    if (Msg_Read(Num, 79)) {
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if ((p[0] == '\001') || (!strncmp(p, "SEEN-BY:", 8)) || (!strncmp(p, "AREA:", 5))) {
		    if (Kludges) {
			colour(LIGHTGRAY, BLACK);
			if (p[0] == '\001')
			    p[0]= 'a';		// Make it printable
			PUTSTR(chartran(p));
			Enter(1);
			if (CheckLine(CFG.TextColourF, CFG.TextColourB, FALSE))
			    break;
		    }
		} else {
		    colour(CFG.TextColourF, CFG.TextColourB);
		    if (strchr(p, '>') != NULL)
			if ((strlen(p) - strlen(strchr(p, '>'))) < 10)
			    colour(CFG.HiliteF, CFG.HiliteB);
		    PUTSTR(chartran(p));
		    Enter(1);
		    if (CheckLine(CFG.TextColourF, CFG.TextColourB, FALSE))
			break;
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }

    if (charset)
	free(charset);
    if (charsin)
	free(charsin);
    chartran_close();

    /*
     * Set the Received status on this message if it's for the user.
     */
    if ((!Msg.Received) && (strlen(Msg.To) > 0) &&
	    ((strcasecmp(Msg.To, exitinfo.sUserName) == 0) || (strcasecmp(exitinfo.sHandle, Msg.To) == 0))) {
	Msg.Received = TRUE;
	Msg.Read = time(NULL) - (gmt_offset((time_t)0) * 60);
	if (Msg_Lock(30L)) {
	    Msg_WriteHeader(Num);
	    Msg_UnLock();
	}
    }

    /*
     * Update lastread pointer if needed. Netmail boards are always updated.
     */
    if (Msg_Lock(30L) && (UpdateLR || msgs.Type == NETMAIL)) {
	p = xstrcpy(exitinfo.sUserName);
	mycrc = StringCRC32(tl(p));
	free(p);
	LR.UserID = grecno;
	LR.UserCRC = mycrc;
	if (Msg_GetLastRead(&LR) == TRUE) {
	    LR.LastReadMsg = Num;
	    if (Num > LR.HighReadMsg)
		LR.HighReadMsg = Num;
	    if (LR.HighReadMsg > MsgBase.Highest)
		LR.HighReadMsg = MsgBase.Highest;
	    LR.UserCRC = mycrc;
	    if (!Msg_SetLastRead(LR))
		WriteError("Error update lastread");
	} else {
	    /*
	     * Append new lastread pointer
	     */
	    LR.UserCRC = mycrc;
	    LR.UserID  = grecno;
	    LR.LastReadMsg = Num;
	    LR.HighReadMsg = Num;
	    if (!Msg_NewLastRead(LR))
		WriteError("Can't append lastread");
	}
	Msg_UnLock();
    }

    Msg_Close();
    return TRUE;
}



/*
 * Read Messages, called from menu
 */
void Read_Msgs()
{
    char	    *temp, *p;
    unsigned int    Start, mycrc;
    lastread	    LR;

    temp = calloc(81, sizeof(char));
    Enter(1);
	/* Message area \"%s\" contains %lu messages. */
    snprintf(temp, 81, "%s\"%s\" %s%u %s", (char *) Language(221), sMsgAreaDesc, 
		(char *) Language(222), MsgBase.Total, (char *) Language(223));
    pout(CFG.TextColourF, CFG.TextColourB, temp);

    /*
     * Check for lastread pointer, suggest lastread number for start.
     */
    Start = MsgBase.Lowest;
    if (Msg_Open(sMsgAreaBase)) {
	p = xstrcpy(exitinfo.sUserName);
	mycrc = StringCRC32(tl(p));
	free(p);
	LR.UserID = grecno;
	LR.UserCRC = mycrc;
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
	if (Start > MsgBase.Highest)
	    Start = MsgBase.Highest;
    }

    Enter(1);
    /* Please enter a message between */
    snprintf(temp, 81, "%s(%u - %u)", (char *) Language(224), MsgBase.Lowest, MsgBase.Highest);
    pout(WHITE, BLACK, temp);
    Enter(1);
    /* Message number [ */
    snprintf(temp, 81, "%s%u]: ", (char *) Language(225), Start);
    PUTSTR(temp);

    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(temp, 80);
    if ((strcmp(temp, "")) != 0)
	Start = atoi(temp);
    free(temp);

    if (!Read_a_Msg(Start, TRUE))
	return;

    if (MsgBase.Total == 0)
	return;

    while(ReadPanel()) {}
}



/*
 * The panel bar under the messages while reading
 */
int ReadPanel()
{
    int input;

    WhosDoingWhat(READ_POST, NULL);

    /*
     * The writer of the message, sysops, and who has sysop rights to the message area
     * are allowed to delete messages.
     */
    if ((msgs.UsrDelete && IsMe(Msg.From)) || (exitinfo.Security.level >= CFG.sysop_access) ||
	    Access(exitinfo.Security, msgs.SYSec)) {
	/* (A)gain, (N)ext, (L)ast, (R)eply, (E)nter, (D)elete, (Q)uit, e(X)port */
	pout(WHITE, RED, (char *) Language(214));
    } else {
	/* (A)gain, (N)ext, (L)ast, (R)eply, (E)nter, (Q)uit, e(X)port */
	pout(WHITE, RED, (char *) Language(215));
    }
    if ((exitinfo.Security.level >= CFG.sysop_access) || Access(exitinfo.Security, msgs.SYSec))
	PUTSTR((char *)", (!)");

    PUTSTR((char *)": ");

    alarm_on();
    input = toupper(Readkey());

    if (input == '!') { /* ! Toggle kludges display */
	if ((exitinfo.Security.level >= CFG.sysop_access) || Access(exitinfo.Security, msgs.SYSec)) {
	    if (Kludges)
		Kludges = FALSE;
	    else
		Kludges = TRUE;
	}
	Read_a_Msg(LastNum, TRUE);
    
    } else if (input == Keystroke(214, 0)) { /* (A)gain */
	Read_a_Msg(LastNum, TRUE);

    } else if (input == Keystroke(214, 4)) { /* (P)ost */
	Post_Msg();
	Read_a_Msg(LastNum, TRUE);

    } else if (input == Keystroke(214, 2)) { /* (L)ast */
	if (LastNum  > MsgBase.Lowest)
	    LastNum--;
	Read_a_Msg(LastNum, TRUE);

    } else if (input == Keystroke(214, 3)) { /* (R)eply */
	Reply_Msg(TRUE);
	Read_a_Msg(LastNum, TRUE);

    } else if (input == Keystroke(214, 5)) { /* (Q)uit */
	/* Quit */
	pout(WHITE, BLACK, (char *) Language(189));
	Enter(1);
	return FALSE;

    } else if (input == Keystroke(214, 7)) { /* e(X)port */
	Export_a_Msg(LastNum);
	Read_a_Msg(LastNum, TRUE);

    } else if (input == '+') {
	if (Msg.Reply) 
	    LastNum = Msg.Reply;
	Read_a_Msg(LastNum, TRUE);

    } else if (input == '-') {
	if (Msg.Original) 
	    LastNum = Msg.Original;
	Read_a_Msg(LastNum, TRUE);

    } else if (input == Keystroke(214, 6)) { /* (D)elete */
	if ((msgs.UsrDelete && IsMe(Msg.From)) || (exitinfo.Security.level >= CFG.sysop_access) ||
		                Access(exitinfo.Security, msgs.SYSec)) {
	    Delete_MsgNum(LastNum);
	    if (LastNum < MsgBase.Highest) {
		LastNum++;
		Read_a_Msg(LastNum, TRUE);
	    } else {
		return FALSE;
	    }
	} else {
	    Read_a_Msg(LastNum, TRUE);
	}

    } else {
	/* Next */
	pout(WHITE, BLACK, (char *) Language(216));
	if (LastNum < MsgBase.Highest)
	    LastNum++;
	else
	    return FALSE;
	Read_a_Msg(LastNum, TRUE);
    }
    return TRUE;
}



/*
 *  Reply message, in Msg.From and Msg.Subject must be the
 *  name to reply to and the subject. IsReply is true if the
 *  message is a real reply, and false for forced messages such
 *  as "message to sysop"
 */
void Reply_Msg(int IsReply)
{
    int	    i, j, x, cc;
    char    to[101], from[101], subj[101], msgid[101], replyto[101], replyaddr[101], *tmp, *buf, qin[6], msg[81];
    faddr   *Dest = NULL;

    if (!Post_Allowed())
	return;

    strncpy(from, Msg.To, 100);
    strncpy(to, Msg.From, 100);
    strncpy(replyto, Msg.ReplyTo, 80);

    /*
     * For some reason there are sometimes spaces at the
     * beginning of the reply address.
     */
    tmp = Msg.ReplyAddr;
    while (*tmp && isspace(*tmp))
	tmp++;
    strncpy(replyaddr, tmp, 100);

    Dest = parsefnode(Msg.FromAddress);
    Syslog('m', "Parsed from address %s", ascfnode(Dest, 0x1f));

    if (strncasecmp(Msg.Subject, "Re:", 3) && strncasecmp(Msg.Subject, "Re^2:", 5) && IsReply) {
	snprintf(subj, 101, "Re: ");
	strncpy(subj+4, Msg.Subject, 97);
    } else {
	strncpy(subj, Msg.Subject, 101);
    }
    Syslog('m', "Reply msg to %s, subject %s", to, subj);
    Syslog('m', "Msgid was %s", Msg.Msgid);
    strncpy(msgid, Msg.Msgid, 100);

    x = 0;
    WhosDoingWhat(READ_POST, NULL);
    clear();
    snprintf(msg, 81, "   %-71s", sMsgAreaDesc);
    pout(BLUE, LIGHTGRAY, msg);
    snprintf(msg, 81, "#%-5u", MsgBase.Highest + 1);
    pout(RED, LIGHTGRAY, msg);
    Enter(1);

    colour(CFG.HiliteF, CFG.HiliteB);
    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');
    PUTSTR(chartran(sLine_str()));
    chartran_close();

    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	Message[i] = (char *) calloc(MAX_LINE_LENGTH +1, sizeof(char));
    Msg_New();

    strncpy(Msg.Replyid, msgid, 101);
    strncpy(Msg.ReplyTo, replyto, 101);
    strncpy(Msg.ReplyAddr, replyaddr, 101);

    /* From     : */
    if (Alias_Option()) {
	/*
	 * Set handle
	 */
	strcpy(Msg.From, exitinfo.sHandle);
	tlcap(Msg.From); // Do we want this???
    } else {
	if (msgs.Type == NEWS) {
	    if (CFG.EmailMode != E_PRMISP) {
		/*
		 * If no inernet mail domain, use fidonet addressing
		 */
		strcpy(Msg.From, exitinfo.sUserName);
		tlcap(Msg.From);
	    } else {
		snprintf(Msg.From, 101, "%s@%s (%s)", exitinfo.Name, CFG.sysdomain, exitinfo.sUserName);
	    }
	} else {
	    strncpy(Msg.From, exitinfo.sUserName, 101);
	    tlcap(Msg.From);
	}
    }
    pout(YELLOW, BLACK, (char *) Language(209));
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.From);
    Enter(1);

    /* To       : */
    strncpy(Msg.To, to, 101);
    pout(YELLOW, BLACK, (char *) Language(208));
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.To);
    Enter(1);

    /* Enter to keep Subject. */
    pout(LIGHTRED, BLACK, (char *) Language(219));
    Enter(1);
    /* Subject  : */
    pout(YELLOW, BLACK, (char *) Language(210));
    strncpy(Msg.Subject, subj, 101);
    pout(CFG.MsgInputColourF, CFG.MsgInputColourB, Msg.Subject);

    x = strlen(subj);
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    GetstrP(subj, 50, x);

    mbse_CleanSubject(subj);
    if (strlen(subj))
	strcpy(Msg.Subject, subj);

    Msg.Private = IsPrivate();
    Enter(1);

    /*
     * If netmail reply and enough security level, allow crashmail.
     */
    if (msgs.Type == NETMAIL) {
	switch (Crash_Option(Dest)) {
	    case 1: Msg.Crash = TRUE;
		    break;
	    case 2: Msg.Immediate = TRUE;
		    break;
	    case -1:printf("\r");
		    /* Node not known, continue anayway [y/N]: */
		    pout(CYAN, BLACK, (char *) Language(241));
		    alarm_on();
		    if (toupper(Readkey()) == Keystroke(241, 0)) {
			Syslog('+', "Node not found, forced continue");
		    } else {
			for (i = 0; i < (TEXTBUFSIZE + 1); i++)
			    free(Message[i]);
			return;
		    }
		    break;
	}
    }

    Check_Attach();

    /*
     *  Quote original message now, format the original users
     *  initials into qin. No quoting if this is a message to Sysop.
     */
    Line = 1;
    if (IsReply) {
	snprintf(Message[1], TEXTBUFSIZE +1, "%s wrote to %s:", to, from);
	memset(&qin, 0, sizeof(qin));
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
	Line = 2;

	tmp = calloc(PATH_MAX, sizeof(char));
	buf = calloc(TEXTBUFSIZE +1, sizeof(char));

	snprintf(tmp, PATH_MAX, "%s/%s/.quote", CFG.bbs_usersdir, exitinfo.Name);
	if ((qf = fopen(tmp, "r")) != NULL) {
	    while ((fgets(buf, TEXTBUFSIZE, qf)) != NULL) {
		Striplf(buf);
		snprintf(Message[Line], TEXTBUFSIZE +1, "%s> %s", (char *)qin, buf);
		Line++;
		if (Line == TEXTBUFSIZE)
		    break;
	    }
	    fclose(qf);
	} else
	    WriteError("$Can't read %s", tmp);

	free(buf);
	free(tmp);
    }

    if (Edit_Msg()) {
	Enter(1);
        if (msgs.Type == NETMAIL) {
	    /*
	     * Check for Carbon Copy lines, process them if present.
	     */
	    cc = 0;
	    for (i = 1; i <= Line; i++) {
		if (strncasecmp(Message[i], "cc: ", 4)) {
		    break;
		} else {
		    cc++;
		}
	    }
	    Syslog('b', "CC: detected %d", cc);
            if (cc) {
		/*
		 * Carbon copies, modify the text to show the presence of CCs.
		 */
		for (i = Line; i; i--) {
		    Syslog('b', "%02d: \"%s\"", i, printable(Message[i], 0));
		    snprintf(Message[i + 1], TEXTBUFSIZE +1, Message[i]);
		}
		Line++;
		snprintf(Message[1], TEXTBUFSIZE +1, " +: Original message to %s", ascfnode(Dest, 0x4f));
		for (i = 1; i <= Line; i++) {
		    Syslog('b', "%02d: \"%s\"", i, printable(Message[i], 0));
		}
		/*
		 * First sent to original destination
		 */
		Save_Msg(IsReply, Dest);
		/*
		 * Now sent copies
		 */
		for (i = 0; i < cc; i++) {
		    Save_CC(IsReply, Message[i+2]);
		}
	    } else {
		Save_Msg(IsReply, Dest);
	    }
	} else {
	    Save_Msg(IsReply, Dest);
	}
	Enter(1);
	sleep(3);
    }

    for (i = 0; i < (TEXTBUFSIZE + 1); i++)
	free(Message[i]);
}



int IsMe(char *Name)
{
    char    *p, *q;
    int	    i, rc = FALSE;

    if (strlen(Name) == 0)
	return FALSE;

    if (strcasecmp(Name, exitinfo.sUserName) == 0)
	rc = TRUE;

    if (strcasecmp(Name, exitinfo.sHandle) == 0)
	rc = TRUE;

    q = xstrcpy(Name);
    if (strstr(q, (char *)"@")) {
	p = strtok(q, "@");
	for (i = 0; i < strlen(p); i++)
	    if (p[i] == '_')
		p[i] = ' ';
	if (strcasecmp(p, exitinfo.sUserName) == 0)
	    rc = TRUE;
	if (strcasecmp(p, exitinfo.sHandle) == 0)
	    rc = TRUE;
	if (strcasecmp(p, exitinfo.Name) == 0)
	    rc = TRUE; 
    }
    free(q);
    return rc ;  	
}



void QuickScan_Msgs()
{
    int	    FoundMsg  = FALSE;
    int	    i;
    char    msg[81];

    iLineCount = 2;
    WhosDoingWhat(READ_POST, NULL);

    if (MsgBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area. */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(3);
	sleep(3);
	return;
    }

    clear(); 
    /* #    From                  To                       Subject */
    poutCR(YELLOW, BLUE, (char *) Language(220));

    if (Msg_Open(sMsgAreaBase)) {
	for (i = MsgBase.Lowest; i <= MsgBase.Highest; i++) {
	    if (Msg_ReadHeader(i) && ((msgs.Type != NETMAIL) || 
				    ((msgs.Type == NETMAIL) && ((IsMe(Msg.From)) || (IsMe(Msg.To)))))) {
				
		snprintf(msg, 81, "%-6u", Msg.Id);
		pout(WHITE, BLACK, msg);
		snprintf(msg, 81, "%s ", padleft(Msg.From, 20, ' '));
		if (IsMe(Msg.From))
		    pout(LIGHTCYAN, BLACK, msg);
		else
		    pout(CYAN, BLACK, msg);

		snprintf(msg, 81, "%s ", padleft(Msg.To, 20, ' '));
		if (IsMe(Msg.To))
		    pout(LIGHTGREEN, BLACK, msg);
		else
		    pout(GREEN, BLACK, msg);
		snprintf(msg, 81, "%s", padleft(Msg.Subject, 31, ' '));
		pout(MAGENTA, BLACK, msg);
		Enter(1);
		FoundMsg = TRUE;
		if (LC(1))
		    break;
	    }
	}
	Msg_Close();
    }

    if (!FoundMsg) {
	Enter(1);
	/* There are no messages in this area. */
	pout(LIGHTGREEN, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
    }

    iLineCount = 2;
    Pause();
}



/*
 *  Called from the menu
 */
void Delete_Msg()
{
    char	    *temp;
    unsigned int    Msgnum = 0L;

    WhosDoingWhat(READ_POST, NULL);

    /*
     * The area data is already set, so we can do the next things
     */
    if (MsgBase.Total == 0) {
	Enter(1);
	/* There are no messages in this area */
	pout(WHITE, BLACK, (char *) Language(205));
	Enter(2);
	sleep(3);
	return;
    }

    temp = calloc(81, sizeof(char));
    Enter(1);
    /* Message area \"%s\" contains %lu messages. */
    snprintf(temp, 81, "%s\"%s\" %s%u %s", (char *) Language(221), sMsgAreaDesc,
	    (char *) Language(222), MsgBase.Total, (char *) Language(223));
    pout(CFG.TextColourF, CFG.TextColourB, temp);

    Enter(1);
    /* Please enter a message between */
    snprintf(temp, 81, "%s(%u - %u): ", (char *) Language(224), MsgBase.Lowest, MsgBase.Highest);
    pout(WHITE, BLACK, temp);

    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(temp, 10);
    if ((strcmp(temp, "")) != 0)
	Msgnum = atoi(temp);
    free(temp);
    
    if (!Msg_Open(sMsgAreaBase)) {
	WriteError("Error open JAM base %s", sMsgAreaBase);
	return;
    }

    if (!Msg_ReadHeader(Msgnum)) {
	Enter(1);
	/* Message doesn't exist */
	pout(WHITE, BLACK, (char *)Language(77));
	Enter(2);
	Msg_Close();
	sleep(3);
	return;
    }
    Msg_Close();
    
    /*
     * Message does exist and a valid number is suplied, check and finally mark the message deleted.
     */
    if ((msgs.UsrDelete && IsMe(Msg.From)) || (exitinfo.Security.level >= CFG.sysop_access) ||
	                Access(exitinfo.Security, msgs.SYSec)) {
	Delete_MsgNum(Msgnum);
    } else {
	Enter(1);
	pout(LIGHTRED, BLACK, (char *)Language(14));
	Enter(2);
    }

    sleep(3);
    return;
} 



/*
 * Check linecounter for reading messages.
 */
int CheckLine(int FG, int BG, int Email)
{
    int	    x, z;

    x = strlen(Language(61));
    iLineCount++;

    if ((iLineCount >= (rows -1)) && (iLineCount < 1000)) {
	iLineCount = 7;

	DoNop();
	pout(CFG.MoreF, CFG.MoreB, (char *) Language(61));

	alarm_on();
	z = tolower(Readkey());

	Blanker(x);
	
	switch(z) {
	    case 'n':	Enter(1);
			return TRUE;
			break;
	    case '=':	iLineCount = 1000;
	}
	if (Email)
	    ShowEmailHdr();
	else
	    ShowMsgHdr();
	colour(FG, BG);
    }
    return FALSE;
}



/*
 * Select message area from the list.
 */
void MsgArea_List(char *Option)
{
    FILE	*pAreas;
    int		iAreaCount = 6, Recno = 0, iOldArea = 0, iAreaNum = 0, loopcount = 0;
    int		iGotArea = FALSE; /* Flag to check if user typed in area */
    int		iCheckNew = FALSE; /* Flag to check for new mail in area */
    int		offset;
    char	*temp, msg[81], *p;
    lastread	LR;
    unsigned int mycrc;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));

    /*
     * Save old area, incase he picks a invalid area
     */
    iOldArea = iMsgAreaNumber;

    if ((pAreas = fopen(temp, "rb")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return;
    }
	
    /* 
     * Count how many records there are
     */
    fread(&msgshdr, sizeof(msgshdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreaNum = (ftell(pAreas) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize);

    /*
     * If there are menu options, parse them first
     *  1. Check for New Messages in Area (only option that will continue to display list)
     *  2. Increment Area, return
     *  3. Decrement Area, return
     *  4. Select area direct (via area number), return
     */
    if (strlen(Option) != 0) {
	if (strcmp(Option, "N") == 0) {
	    iCheckNew = TRUE;
	} else {
	    if (strcmp(Option, "U+") == 0) {
		while(TRUE) {
		    iMsgAreaNumber++;
		    if (iMsgAreaNumber >= iAreaNum) {
			iMsgAreaNumber = 0;
			loopcount++;
			if (loopcount > 1) {
			    /* No more areas with unread messages */
			    pout(LIGHTRED, BLACK, (char *) Language(479));
			    iMsgAreaNumber = iOldArea;
			    Enter(2);
			    Pause();
			    free(temp);
			    fclose(pAreas);
			    return;
			}
		    }
	
		    offset = msgshdr.hdrsize + (iMsgAreaNumber * (msgshdr.recsize + msgshdr.syssize));
		    if (fseek(pAreas, offset, 0) != 0) {
			WriteError("$Can't move pointer in %s", temp);
		    }
									
		    fread(&msgs, msgshdr.recsize, 1, pAreas);
		    if ((Access(exitinfo.Security, msgs.RDSec)) && (msgs.Active) && (strlen(msgs.Password) == 0)) {
			if (Msg_Open(msgs.Base)) {
			    MsgBase.Highest = Msg_Highest();
			    p = xstrcpy(exitinfo.sUserName);
			    mycrc = StringCRC32(tl(p));
			    free(p);
			    LR.UserID = grecno; 
			    LR.UserCRC = mycrc;
			    if (Msg_GetLastRead(&LR) != TRUE) {
				LR.HighReadMsg = 0;
			    }
			    if (MsgBase.Highest > LR.HighReadMsg) {
				Msg_Close();
				break;
			    }
			    Msg_Close();
			}
		    }
		}
	    }

	    if (strcmp(Option, "U-") == 0) {
		while(TRUE) {
		    iMsgAreaNumber--;
		    if (iMsgAreaNumber < 0) {
			iMsgAreaNumber = iAreaNum - 1;
			loopcount++;
			if (loopcount > 1) {
			    /* No more areas with unread messages */
			    pout(LIGHTRED, BLACK, (char *) Language(479));
			    iMsgAreaNumber = iOldArea;
			    Enter(2);
			    Pause();
			    fclose(pAreas);
			    free(temp);
			    return;
			}
		    }
	
		    offset = msgshdr.hdrsize + (iMsgAreaNumber * (msgshdr.recsize + msgshdr.syssize));
		    if (fseek(pAreas, offset, 0) != 0) {
			WriteError("$Can't move pointer in %s", temp);
		    }
									
		    fread(&msgs, msgshdr.recsize, 1, pAreas);
		    if ((Access(exitinfo.Security, msgs.RDSec)) && (msgs.Active) && (strlen(msgs.Password) == 0)) {
			if (Msg_Open(msgs.Base)) {
			    MsgBase.Highest = Msg_Highest();
			    p = xstrcpy(exitinfo.sUserName);
			    mycrc = StringCRC32(tl(p));
			    free(p);
			    LR.UserID = grecno; 
			    LR.UserCRC = mycrc;
			    if (Msg_GetLastRead(&LR) != TRUE ){
				LR.HighReadMsg = 0;
			    }
			    if (MsgBase.Highest > LR.HighReadMsg) {
				Msg_Close();	
				break;
			    }
			    Msg_Close();
			}
		    }
		}
	    }

	    if (strcmp(Option, "M+") == 0) {
		while (TRUE) {
		    iMsgAreaNumber++;
		    if (iMsgAreaNumber >= iAreaNum) {
			iMsgAreaNumber = 0;
		    }
		    offset = msgshdr.hdrsize + (iMsgAreaNumber * (msgshdr.recsize + msgshdr.syssize));
		    if(fseek(pAreas, offset, 0) != 0) {
			WriteError("$Can't move pointer in %s", temp);
		    }
									
		    fread(&msgs, msgshdr.recsize, 1, pAreas);
		    if ((Access(exitinfo.Security, msgs.RDSec)) && (msgs.Active) && (strlen(msgs.Password) == 0)) {
			break;
		    }
		}
	    }
		
	    if (strcmp(Option, "M-") == 0) {
		while (TRUE) {
		    iMsgAreaNumber--;
		    if (iMsgAreaNumber < 0) {
			iMsgAreaNumber = iAreaNum -1;
		    }
		    offset = msgshdr.hdrsize + (iMsgAreaNumber * (msgshdr.recsize + msgshdr.syssize));
		    if (fseek(pAreas, offset, 0) != 0) {
			WriteError("$Can't move pointer in %s", temp);
		    }
					
		    fread(&msgs, msgshdr.recsize, 1, pAreas);
		    if ((Access(exitinfo.Security, msgs.RDSec)) && (msgs.Active) && (strlen(msgs.Password) == 0)) {
			break;
		    }
		}
	    }

	    SetMsgArea(iMsgAreaNumber);
	    Syslog('+', "Msg area %lu %s", iMsgAreaNumber, sMsgAreaDesc);
	    free(temp);
	    fclose(pAreas);
	    return;
	}
    }

    clear();
    Enter(1);
    /*  Message Areas */
    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(231));
    Enter(2);

    fseek(pAreas, msgshdr.hdrsize, 0);
    
    while (fread(&msgs, msgshdr.recsize, 1, pAreas) == 1) {
	/*
	 * Skip the echomail systems
	 */
	fseek(pAreas, msgshdr.syssize, SEEK_CUR);
	if ((Access(exitinfo.Security, msgs.RDSec)) && (msgs.Active)) {
	    msgs.Name[31] = '\0';

	    snprintf(msg, 81, "%5d", Recno + 1);
	    pout(WHITE, BLACK, msg);

	    colour(LIGHTBLUE, BLACK);
	    /* Check for New Mail if N was put on option data */
	    if (iCheckNew) {
		if (Msg_Open(msgs.Base)) {
		    MsgBase.Highest = Msg_Highest();
		    p = xstrcpy(exitinfo.sUserName);
		    mycrc = StringCRC32(tl(p));
		    free(p);
		    LR.UserID = grecno; 
		    LR.UserCRC = mycrc;
		    if (Msg_GetLastRead(&LR) != TRUE) {
			LR.HighReadMsg = 0;
		    }
		    if (MsgBase.Highest > LR.HighReadMsg ) {
			pout(YELLOW, BLACK, (char *)" * ");
		    } else {
			PUTSTR((char *)" . ");
		    }
		    Msg_Close();
		} else {
		    PUTSTR((char *)" . ");
		}
	    } else {
		PUTSTR((char *)" . ");
	    }

	    snprintf(msg, 81, "%-31s", msgs.Name);
	    pout(CYAN, BLACK, msg);

	    iAreaCount++;

	    if ((iAreaCount % 2) == 0)
		Enter(1);
	    else
		PUTCHAR(' ');
	}

	Recno++;

	if ((iAreaCount / 2) == rows) {
	    /* More (Y/n/=/Area #): */
	    pout(CFG.MoreF, CFG.MoreB, (char *) Language(207));
	    /*
	     * Ask user for Area or enter to continue
	     */
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, 7);

	    if (toupper(temp[0]) == Keystroke(207, 1))
		break;

	    if ((strcmp(temp, "")) != 0) {
		iGotArea = TRUE;
		break;
	    }

	    iAreaCount = 2;
	}
    }

    /*
     * If user type in area above during area listing
     * don't ask for it again
     */
    if (!iGotArea) {
	Enter(1);
	/* Select Area: */
	pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(232));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);
    }

    /*
     * Check if user pressed ENTER
     */
    if ((strcmp(temp, "")) == 0) {
	fclose(pAreas);
	free(temp);
	return;
    }
    iMsgAreaNumber = atoi(temp);
    iMsgAreaNumber--;

    /*
     * Do a check in case user presses Zero
     */
    if (iMsgAreaNumber == -1)
	iMsgAreaNumber = 0;

    offset = msgshdr.hdrsize + (iMsgAreaNumber * (msgshdr.recsize + msgshdr.syssize));
    if (fseek(pAreas, offset, 0) != 0) {
	WriteError("$Can't move pointer in mareas.data.");
    } 
    fread(&msgs, msgshdr.recsize, 1, pAreas);

    /*
     * Do a check if area is greater or less number than allowed,
     * security acces (level, flags and age) is oke, and the area
     * is active.
     */
    if (iMsgAreaNumber > iAreaNum || iMsgAreaNumber < 0 || (Access(exitinfo.Security, msgs.RDSec) == FALSE) || (!msgs.Active)) {
	Enter(1);
	/*
	 * Invalid area specified - Please try again ...
	 */
	pout(LIGHTRED, BLACK, (char *) Language(233));
	Enter(2);
	Pause();
	fclose(pAreas);
	iMsgAreaNumber = iOldArea;
	SetMsgArea(iMsgAreaNumber);
	free(temp);
	return;
    }

    SetMsgArea(iMsgAreaNumber);
    Syslog('+', "Msg area %lu %s", iMsgAreaNumber, sMsgAreaDesc);

    /*
     * Check if msg area has a password, if it does ask user for it
     */
    if ((strlen(msgs.Password)) > 2) {
	Enter(2);
	/* Please enter Area Password: */
	pout(WHITE, BLACK, (char *) Language(233));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 20);

	if ((strcmp(temp, msgs.Password)) != 0) {
	    Enter(1);
	    /* Password is incorrect */
	    pout(WHITE, BLACK, (char *) Language(234));
	    Syslog('!', "Incorrect Message Area # %d password given: %s", iMsgAreaNumber, temp);
	    SetMsgArea(iOldArea);
	} else {
	    Enter(1);
	    /* Password is correct */
	    pout(WHITE, BLACK, (char *) Language(235));
	    Enter(2);
	}
	Pause();
    }

    free(temp);
    fclose(pAreas);
}



void Delete_MsgNum(unsigned int MsgNum)
{
    int	Result = FALSE;

    /* Deleting message */
    pout(LIGHTRED, BLACK, (char *) Language(230));

    if (Msg_Open(sMsgAreaBase)) {
	if (Msg_Lock(15L)) {
	    Result = Msg_Delete(MsgNum);
	    Msg_UnLock();
	}
	Msg_Close();
    }

    if (Result)
	Syslog('+', "Deleted msg #%lu in Area #%d (%s)", MsgNum, iMsgAreaNumber, sMsgAreaDesc);
    else
	WriteError("ERROR delete msg #%lu in Area #%d (%s)", MsgNum, iMsgAreaNumber, sMsgAreaDesc);
}



/*
 * This Function checks to see if the user exists in the user database
 * and returns a int TRUE or FALSE
 */
int CheckUser(char *To)
{
    FILE	    *pUsrConfig;
    int		    Found = FALSE;
    char	    *temp;
    int		    offset;
    unsigned int    Crc;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((pUsrConfig = fopen(temp,"rb")) == NULL) {
	WriteError("$Can't open file %s for reading", temp);
	Pause();
	free(temp);
	return FALSE;
    }
    free(temp);
    fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
    Crc = StringCRC32(tl(To));

    while (fread(&usrconfig, usrconfighdr.recsize, 1, pUsrConfig) == 1) {
	if (StringCRC32(tl(usrconfig.sUserName)) == Crc) {
	    Found = TRUE;
	    break;
	}
    }

    if (!Found)
	Syslog('!', "User attempted to mail unknown user: %s", tlcap(To));

    /*
     * Restore users record
     */
    offset = usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize);
    if (fseek(pUsrConfig, offset, 0) != 0)
	WriteError("$Can't move pointer there.");
    else
	fread(&usrconfig, usrconfighdr.recsize, 1, pUsrConfig);
    fclose(pUsrConfig);

    return Found;
}



/*
 * Check for new mail
 */
void CheckMail()
{
    FILE		*pMsgArea, *Tmp;
    int			x, Found = 0, Color, Count = 0, Reading, OldMsgArea;
    char		*temp, *sFileName, msg[81], *p;
    unsigned int	i, Start, mycrc;
    typedef struct	_Mailrec {
	int		Area;
	unsigned int	Msg;
    } _Mail;
    _Mail		Mail;
    lastread		LR;

    OldMsgArea = iMsgAreaNumber;
    iMsgAreaNumber = 0;
    Syslog('+', "Start checking for new mail");

    clear();
    /* Checking your mail box ... */
    language(LIGHTGREEN, BLACK, 150);
    Enter(2);
    Color = LIGHTBLUE;

    /*
     * Open temporary file
     */
    if ((Tmp = tmpfile()) == NULL) {
	WriteError("$unable to open temporary file");
	return;
    }

    /*
     * First check the e-mail mailbox
     */
    temp = calloc(PATH_MAX, sizeof(char));
    if (exitinfo.Email && strlen(exitinfo.Password)) {
	check_popmail(exitinfo.Name, exitinfo.Password);
	colour(Color++, BLACK);
	PUTCHAR('\r');
	PUTSTR((char *)"e-mail  Private e-mail mailbox");
	Count = 0;
	snprintf(temp, PATH_MAX, "%s/%s/mailbox", CFG.bbs_usersdir, exitinfo.Name);
	SetEmailArea((char *)"mailbox");
	if (Msg_Open(temp)) {
	    /*
	     * Check lastread pointer, if none found start
	     * at the begin of the messagebase.
	     */
	    p = xstrcpy(exitinfo.sUserName);
	    mycrc = StringCRC32(tl(p));
	    free(p);
	    LR.UserID = grecno;
	    LR.UserCRC = mycrc;
	    if (Msg_GetLastRead(&LR))
		Start = LR.HighReadMsg + 1;
	    else
		Start = EmailBase.Lowest;

	    for (i = Start; i <= EmailBase.Highest; i++) {
		if (Msg_ReadHeader(i)) {
		    /*
		     * Only check the received status of the email. The mail
		     * may not be direct addressed to this user (aliases database)
		     * but if it is in his mailbox it is always for the user.
		     * FIXME: mail written by the user is shown as new too.
		     */
		    if (!Msg.Received) {
			/*
			 * Store area and message number in temporary file.
			 */
			Mail.Area = -1; /* Means e-mail mailbox */
			Mail.Msg  = Msg.Id + EmailBase.Lowest -1;
			fwrite(&Mail, sizeof(Mail), 1, Tmp);
			Count++;
			Found++;
		    }
		}
	    }
	    Msg_Close();
	}
	if (Count) {
	    Enter(2);
	    /* messages in */
	    snprintf(temp, 81, "%d %s private e-mail mailbox", Count, (char *)Language(213));
	    pout(CFG.TextColourF, CFG.TextColourB, temp);
	    Enter(2);
	    Syslog('m', "  %d messages in private e-mail mailbox", Count);
	}
    }

    /*
     * Open the message base configuration
     */
    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if((pMsgArea = fopen(sFileName, "r+")) == NULL) {
	WriteError("$Can't open: %s", sFileName);
	free(temp);
	free(sFileName);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, pMsgArea);

    /*
     * Check all normal areas one by one
     */
    while (fread(&msgs, msgshdr.recsize, 1, pMsgArea) == 1) {
	fseek(pMsgArea, msgshdr.syssize, SEEK_CUR);
	if ((msgs.Active) && (exitinfo.Security.level >= msgs.RDSec.level)) {
	    SetMsgArea(iMsgAreaNumber);
	    snprintf(temp, 81, "%d", iMsgAreaNumber + 1);
	    if (Color < WHITE)
		Color++;
	    else
		Color = LIGHTBLUE;
	    PUTCHAR('\r');
	    snprintf(msg, 81, "%6s  %-40s", temp, sMsgAreaDesc);
	    pout(Color, BLACK, msg);
	    Count = 0;
	    /*
	     * Refresh timers
	     */
	    Nopper();
	    alarm_on();

	    if (Msg_Open(sMsgAreaBase)) {
		/*
		 * Check lastread pointer, if none found start
		 * at the begin of the messagebase.
		 */
		p = xstrcpy(exitinfo.sUserName);
		mycrc = StringCRC32(tl(p));
		free(p);
		LR.UserID = grecno;
		LR.UserCRC = mycrc;
		if (Msg_GetLastRead(&LR))
		    Start = LR.HighReadMsg + 1;
		else
		    Start = MsgBase.Lowest;

		for (i = Start; i <= MsgBase.Highest; i++) {
		    if (Msg_ReadHeader(i)) {
			if ((!Msg.Received) && (IsMe(Msg.To))) {
			    /*
			     * Store area and message number
			     * in temporary file.
			     */
			    Mail.Area = iMsgAreaNumber;
			    Mail.Msg  = Msg.Id + MsgBase.Lowest -1;
			    fwrite(&Mail, sizeof(Mail), 1, Tmp);
			    Count++;
			    Found++;
			}
		    }
		}
		Msg_Close();
	    }
	    if (Count) {
		Enter(2);
		/* messages in */
		snprintf(msg, 81, "%d %s %s", Count, (char *)Language(213), sMsgAreaDesc);
		pout(CFG.TextColourF, CFG.TextColourB, msg);
		Enter(2);
		Syslog('m', "  %d messages in %s", Count, sMsgAreaDesc);
	    }
	}
	iMsgAreaNumber++;
    }

    fclose(pMsgArea);
    PUTCHAR('\r');
    for (i = 0; i < 48; i++)
	PUTCHAR(' ');
    PUTCHAR('\r');

    if (Found) {
	Enter(1);
	/* You have messages, read your mail now? [Y/n]: */
	snprintf(msg, 81, "%s%d %s", (char *) Language(142), Found, (char *) Language(143));
	pout(YELLOW, BLACK, msg);
	colour(CFG.InputColourF, CFG.InputColourB);
	alarm_on();

	if (toupper(Readkey()) != Keystroke(143,1)) {
	    rewind(Tmp);
	    Reading = TRUE;

	    while ((Reading) && (fread(&Mail, sizeof(Mail), 1, Tmp) == 1)) {
		if (Mail.Area == -1) {
		    /*
		     * Private e-mail
		     */
		    Read_a_Email(Mail.Msg);
		} else {
		    SetMsgArea(Mail.Area);
		    Read_a_Msg(Mail.Msg, FALSE);
		}
		/* (R)eply, (N)ext, (Q)uit */
		pout(CFG.CRColourF, CFG.CRColourB, (char *)Language(218));
		alarm_on();
		x = toupper(Readkey());

		if (x == Keystroke(218, 0)) {
		    Syslog('m', "  Reply!");
		    if (Mail.Area == -1) {
			Reply_Email(TRUE);
		    } else {
			Reply_Msg(TRUE);
		    }
		}
		if (x == Keystroke(218, 2)) {
		    Syslog('m', "  Quit check for new mail");
		    iMsgAreaNumber = OldMsgArea;
		    fclose(Tmp);
		    SetMsgArea(OldMsgArea);
		    Enter(2);
		    free(temp);
		    free(sFileName);
		    return;
		}
	    }
	}
    } else {
	language(LIGHTRED, BLACK, 144);
	Enter(1);
	sleep(3);
    } /* if (Found) */

    iMsgAreaNumber = OldMsgArea;
    fclose(Tmp);
    SetMsgArea(OldMsgArea);
    Enter(2);
    free(temp);
    free(sFileName);
}



/*
 * Status of all mail areas.
 */
void MailStatus()
{
    FILE	    *pMsgArea;
    int		    Count = 0;
    int		    OldMsgArea;
    char	    temp[81], msg[81];
    char	    *sFileName;
    unsigned int    i;

    sFileName = calloc(PATH_MAX, sizeof(char));
    OldMsgArea = iMsgAreaNumber;
    iMsgAreaNumber = 0;
    clear();
    /* Area Type Description                                   Messages Personal */
    snprintf(msg, 81, "%-79s", (char *)Language(226));
    pout(YELLOW, BLUE, msg);
    Enter(1);
    iLineCount = 2;

    if (exitinfo.Email) {
	snprintf(temp, 81, "%s", sMailbox);
	for (i = 0; i < 3; i++) {
	    switch (i) {
		case 0:	SetEmailArea((char *)"mailbox");
			break;
		case 1:	SetEmailArea((char *)"archive");
			break;
		case 2:	SetEmailArea((char *)"trash");
			break;
	    }
	    pout(LIGHTRED, BLACK, (char *)"      Email");
	    snprintf(msg, 81, " %-40s", Language(467 + i));
	    pout(LIGHTCYAN, BLACK, msg);
	    if (EmailBase.Highest)
		snprintf(msg, 81, " %8u", EmailBase.Highest - EmailBase.Lowest + 1);
	    else
		snprintf(msg, 81, "        0");
	    pout(YELLOW, BLACK, msg);
	    if (EmailBase.Highest)
		snprintf(msg, 81, " %8u", EmailBase.Highest - EmailBase.Lowest + 1);
	    else
		snprintf(msg, 81, "        0");
	    pout(LIGHTBLUE, BLACK, msg);
	    Enter(1);
	}
	iLineCount = 5;
	SetEmailArea(temp);
    }

    /*
     * Open the message base configuration
     */
    snprintf(sFileName, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if((pMsgArea = fopen(sFileName, "r+")) == NULL) {
	WriteError("Can't open file: %s", sFileName);
	free(sFileName);
	return;
    }
    fread(&msgshdr, sizeof(msgshdr), 1, pMsgArea);

    /*
     * Check all areas one by one
     */
    while (fread(&msgs, msgshdr.recsize, 1, pMsgArea) == 1) {
	fseek(pMsgArea, msgshdr.syssize, SEEK_CUR);
	if ((msgs.Active) && (exitinfo.Security.level >= msgs.RDSec.level)) {
	    SetMsgArea(iMsgAreaNumber);
	    snprintf(temp, 81, "%d", iMsgAreaNumber + 1);
	    snprintf(msg, 81, "%5s", temp);
	    pout(WHITE, BLACK, msg);
	    colour(LIGHTRED, BLACK);
	    switch(msgs.Type) {
		case LOCALMAIL:	PUTSTR((char *)" Local");
				break;
		case NETMAIL:	PUTSTR((char *)" Net  ");
				break;
		case LIST:
		case ECHOMAIL:	PUTSTR((char *)" Echo ");
				break;
		case NEWS:	PUTSTR((char *)" News ");
				break;
	    }
	    snprintf(msg, 81, " %-40s", sMsgAreaDesc);
	    pout(LIGHTCYAN, BLACK, msg);
	    Count = 0;

	    if (Msg_Open(sMsgAreaBase)) {
		for (i = MsgBase.Lowest; i <= MsgBase.Highest; i++) {
		    if (Msg_ReadHeader(i)) {
			if (IsMe(Msg.To) || IsMe(Msg.From))
			    Count++;
		    }
		}
		Msg_Close();
	    } else
		WriteError("Error open JAM %s", sMsgAreaBase);
	    if (MsgBase.Highest)
		snprintf(msg, 81, " %8u", MsgBase.Highest - MsgBase.Lowest + 1);
	    else
		snprintf(msg, 81, "        0");
	    pout(YELLOW, BLACK, msg);
	    snprintf(msg, 81, " %8d", Count);
	    pout(LIGHTBLUE, BLACK, msg);
	    Enter(1);
	    if (LC(1))
		break;
	}
	iMsgAreaNumber++;
    }

    fclose(pMsgArea);
    SetMsgArea(OldMsgArea);
    free(sFileName);
    Pause();
}



/*
 * Set message area number, set global area description and JAM path
 */
void SetMsgArea(unsigned int AreaNum)
{
    FILE    *pMsgArea;
    int	    offset;
    char    *sFileName;

    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    memset(&msgs, 0, sizeof(msgs));

    if ((pMsgArea = fopen(sFileName, "r")) == NULL) {
	WriteError("$Can't open file: %s", sFileName);
	free(sFileName);
	return;
    }

    fread(&msgshdr, sizeof(msgshdr), 1, pMsgArea);
    offset = msgshdr.hdrsize + (AreaNum * (msgshdr.recsize + msgshdr.syssize));
    if (fseek(pMsgArea, offset, 0) != 0) {
	WriteError("$Can't move pointer in %s",sFileName);
	free(sFileName);
	return;
    }

    fread(&msgs, msgshdr.recsize, 1, pMsgArea);
    strcpy(sMsgAreaDesc, msgs.Name);
    strcpy(sMsgAreaBase, msgs.Base);
    iMsgAreaNumber = AreaNum;
    iMsgAreaType = msgs.Type;

    fclose(pMsgArea);

    /*
     * Get information from the message base
     */

    if (Msg_Open(sMsgAreaBase)) {

	MsgBase.Lowest  = Msg_Lowest();
	MsgBase.Highest = Msg_Highest();
	MsgBase.Total   = Msg_Number();
	Msg_Close();
    } else
	WriteError("Error open JAM %s", sMsgAreaBase);
    free(sFileName);
}



/*
 * External Message Editor
 */
int Ext_Edit()
{
    int     changed;
    int     j, i;
    char    *l, *tmpname;
    FILE    *fd;
    struct stat     st1,st2;

    changed=FALSE;

    tmpname = calloc(PATH_MAX, sizeof(char));

    snprintf(tmpname, PATH_MAX, "%s/%s/data.msg", CFG.bbs_usersdir, exitinfo.Name);
    if ((fd = fopen(tmpname, "w")) == NULL) {
	Syslog('+',"EXT_EDIT: Unable to open %s for writing", tmpname);
    } else {
	fprintf(fd,"AREA='%s'\n",sMsgAreaDesc);
	fprintf(fd,"AREANUM='%d'\n",iMsgAreaNumber+1);
	fprintf(fd,"AREATYPE='%d'\n",iMsgAreaType);
	fprintf(fd,"MSGFROM='%s'\n",Msg.From);
	fprintf(fd,"MSGFROMADDR='%s'\n",Msg.FromAddress);
	fprintf(fd,"MSGTO='%s'\n",Msg.To);
	fprintf(fd,"MSGTOADDR='%s'\n",Msg.ToAddress);
	fprintf(fd,"MSGSUBJECT='%s'\n",Msg.Subject);
	fprintf(fd,"BBSLANGUAGE='%c'\n",exitinfo.iLanguage);
	fprintf(fd,"BBSFSEDKEYS='%d'\n",exitinfo.FSemacs);
	fprintf(fd,"LINES='%d'\n",rows);
	fprintf(fd,"COLUMNS='80'\n");
	fclose(fd);
    }

    snprintf(tmpname, PATH_MAX, "%s/%s/edit.msg", CFG.bbs_usersdir, exitinfo.Name);
    if ((fd = fopen(tmpname, "w")) == NULL) {
	Syslog('+',"EXT_EDIT: Unable to open %s for writing", tmpname);
    } else {
	for (i = 1; i <= Line; i++) {
	    fprintf(fd,"%s\n",Message[i]);
	}
	fclose(fd);
	stat( tmpname, &st1 );
	ExtDoor(CFG.externaleditor,FALSE,TRUE,TRUE,FALSE,TRUE, FALSE, (char *)"Write message");
	if (rawport() != 0) {
	    WriteError("Unable to set raw mode");
	}
	stat( tmpname, &st2 );
    }
    
    if ( st1.st_mtime != st2.st_mtime ) {
	l = calloc(81, sizeof(char));
	if ((fd = fopen(tmpname, "r")) == NULL) {
	    Syslog('+',"EXT_EDIT: Unable to open %s for reading", tmpname);
	} else {
	    i=1;
	    while ( (fgets(l,80,fd) != NULL) && (i < TEXTBUFSIZE ) ) {
		for (j = 0; i < strlen(l); j++) {
		    if (*(l + j) == '\0')
			break;
		    if (*(l + j) == '\n')
			*(l + j) = '\0';
		    if (*(l + j) == '\r')
			*(l + j) = '\0';
		    /*
		     * Make sure that any tear or origin lines are
		     * made invalid.
		     */
		    if (strncmp(l, (char *)"--- ", 4) == 0)
			l[1] = 'v';
		    if (strncmp(l, (char *)" * Origin:", 10) == 0)
			l[1] = '+';
		}
		snprintf(Message[i], TEXTBUFSIZE +1, "%s",l);
		i++;
	    }
	    changed=TRUE;
	    Line=i;
	    fclose(fd);
	}
	free(l);
    }
    free(tmpname);
    return changed;
}


