/*****************************************************************************
 *
 * $Id: mbse.h,v 1.16 2007/02/20 20:24:06 mbse Exp $
 * Purpose ...............: Global variables for MBSE BBS
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

#ifndef _MBSE_H
#define _MBSE_H

#define LANG 500		/* Amount of Language Entries		   */



typedef	struct _TagRec {
	int		Area;		/* File Area number		   */
	int		Active;		/* Not deleted from taglist	   */
	int		Cost;		/* Free download		   */
	unsigned int	Size;		/* File Size			   */
	char		SFile[13];	/* Short File Name		   */
	char		LFile[81];	/* Long FIle Name		   */
} _Tag;



/*
 * File Areas
 */
int	iAreaNumber;		/* Current File Area -1			   */
char	sAreaDesc[PATH_MAX];	/* Current File Area Name		   */
char	sAreaPath[PATH_MAX];	/* Current File Area path		   */
FILE	*pTagList;		/* Tagged files for download		   */
_Tag	Tag;			/* Tag record				   */



/*
 * Msg Areas
 */
int	iMsgAreaNumber;		/* Current Message Area number -1	   */
int	iMsgAreaType;		/* Current Message Area Type		   */
char	sMsgAreaDesc[PATH_MAX];	/* Current Message Area Name		   */
char	sMsgAreaBase[PATH_MAX];	/* Current Message Area Base		   */
char	sMailbox[21];		/* Current e-mail mailbox		   */
char	sMailpath[PATH_MAX];	/* Current e-mail path			   */



/* 
 * Protocols
 */
char	sProtName[21];		/* Current Transfer Protocol name	   */
char	sProtUp[51];		/* Upload path & binary			   */
char	sProtDn[51];		/* Download path & binary		   */
char	sProtAdvice[31];	/* Advice for protocol			   */
unsigned uProtInternal;		/* Internal protocol			   */
int	iProtEfficiency;	/* Protocol efficiency			   */



/* 
 * Global variables
 */
char  	*mLanguage[LANG];	/* Define LANG=nnn Language Variables	   */
char	*mKeystroke[LANG];	/* Possible keystrokes			   */
char	*Date1, *Date2;		/* Result from function SwapDate()	   */
char	*pTTY;			/* Current tty name			   */
char  	sUserTimeleft[7];	/* Global Time Left Variable		   */
int	iUserTimeLeft;		/* Global Time Left Variable		   */
char	LastLoginDate[12];	/* Last login date			   */
char	LastLoginTime[9];	/* Last login time			   */
char	LastCaller[36];		/* Last caller on system name		   */
time_t	LastCallerTime;		/* Last caller on system time		   */
char  	FirstName[20];		/* Users First name			   */
char  	LastName[30];		/* Users Last name			   */ 
int	UserAge;		/* Users age				   */
int	grecno;			/* User's Record Number in user file	   */
int	SYSOP;			/* Int to see if user is Sysop		   */
int	iLineCount;		/* Line Counter				   */
int	iExpired;		/* Check if users time ran out		   */
char	sUnixName[9];		/* Unix login name			   */
time_t	Time2Go;		/* Calculated time to force logout	   */
struct	tm *l_date;		/* Structure for Date			   */

time_t	ltime;
time_t	Time_Now;

char	current_language[10];	/* Current language of the user		   */
int	utf8;


#endif
