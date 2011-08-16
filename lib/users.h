/*****************************************************************************
 *
 * $Id: users.h,v 1.10 2007/02/25 20:28:07 mbse Exp $
 * Purpose ...............: MBSE BBS Users Database structure
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


#ifndef _USERRECS_H
#define _USERRECS_H


#define Max_passlen     14      /* Define maximum passwd length            */


typedef enum {X_LINEEDIT, FSEDIT, EXTEDIT} MSGEDITOR;


/************************************************************************
 *
 *  Other BBS structures
 *
 */


#ifndef	_SECURITYSTRUCT
#define	_SECURITYSTRUCT

/*
 * Security structure
 */
typedef struct _security {
	unsigned int	level;			/* Security level	   */
	unsigned int	flags;			/* Access flags		   */
	unsigned int	notflags;		/* No Access flags	   */
} securityrec;

#endif


/****************************************************************************
 *
 *  Datafile records structure in $MBSE_ROOT/etc
 *
 */



/*
 * Users Control Structures (users.data)
 */
struct	userhdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
};

struct	userrec {
	char		sUserName[36];		/* User First and Last Name */
	char		Name[9];		/* Unix name		    */
	unsigned int	xPassword;
	char		sVoicePhone[20];	/* Voice Number             */
	char		sDataPhone[20];		/* Data/Business Number     */
	char		sLocation[28];		/* Users Location           */
	char		address[3][41];		/* Users address	    */
	char		sDateOfBirth[12];	/* Date of Birth            */
	int32_t		tFirstLoginDate;	/* Date of First Login      */
	int32_t		tLastLoginDate;		/* Date of Last Login       */
	securityrec	Security;		/* User Security Level      */
	char		sComment[81];		/* User Comment             */
	char		sExpiryDate[12];	/* User Expiry Date         */
	securityrec	ExpirySec;		/* Expiry Security Level    */
	char		sSex[8];		/* Users Sex                */

	unsigned	Hidden		: 1;	/* Hide User from Lists     */
	unsigned	HotKeys		: 1;	/* Hot-Keys ON/OFF          */
	unsigned	xGraphMode	: 1;
	unsigned	Deleted		: 1;	/* Deleted Status           */
	unsigned	NeverDelete	: 1;	/* Never Delete User        */
	unsigned	xChat		: 1;
	unsigned	LockedOut	: 1;	/* User is locked out	    */
	unsigned	DoNotDisturb	: 1;	/* DoNot disturb	    */
	unsigned	Cls		: 1;	/* CLS on/off		    */
	unsigned	More		: 1;	/* More prompt		    */
	unsigned	xFsMsged	: 1;
	unsigned	MailScan	: 1;	/* New Mail scan	    */
	unsigned	Guest		: 1;	/* Is guest account	    */
	unsigned	OL_ExtInfo	: 1;	/* OLR extended msg info    */
	int		iTotalCalls; 		/* Total number of calls    */
	int		iTimeLeft;              /* Time left today          */
	int		iConnectTime;           /* Connect time this call   */
	int		iTimeUsed;              /* Time used today          */
	int		xScreenLen;
	int32_t		tLastPwdChange;         /* Date last password chg   */
	unsigned	xHangUps;
	int		Credit;			/* Users credit		    */
	int		Paged;			/* Times paged today	    */
	int		MsgEditor;		/* Message Editor to use    */
	int		LastPktNum;		/* Todays Last packet number*/
	char		Archiver[6];		/* Archiver to use	    */

	int		iLastFileArea;          /* Number of last file area */
	int		iLastFileGroup;		/* Number of last file group*/
	char		sProtocol[21];          /* Users default protocol   */
	unsigned int	Downloads;		/* Total number of d/l's    */
	unsigned int	Uploads;		/* Total number of uploads  */
	unsigned int	UploadK;		/* Upload KiloBytes         */
	unsigned int	DownloadK;		/* Download KiloBytes       */
	int		DownloadKToday;		/* KB Downloaded today      */
	int		UploadKToday;		/* KB Uploaded today        */
	int		iTransferTime;          /* Last file transfer time  */
	int		iLastMsgArea;           /* Number of last msg area  */
	int		iLastMsgGroup;		/* Number of last msg group */
	int		iPosted;                /* Number of msgs posted    */
	int		iLanguage;              /* Current Language         */
	char		sHandle[36];            /* Users Handle             */
	int		iStatus;                /* WhosDoingWhat status	    */
	int		DownloadsToday;		/* Downloads today	    */
	int		CrtDef;			/* IEMSI Terminal emulation */
	int		Protocol;		/* IEMSI protocol	    */
	unsigned	IEMSI		: 1;	/* Is this a IEMSI session  */
	unsigned	ieMNU		: 1;	/* Can do ASCII download    */
	unsigned	ieTAB		: 1;	/* Can handle TAB character */
	unsigned	ieASCII8	: 1;	/* Can handle 8-bit IBM-PC  */
	unsigned	ieNEWS		: 1;	/* Show bulletins	    */
	unsigned	ieFILE		: 1;	/* Check for new files	    */
	unsigned	Email		: 1;	/* Has private email box    */
	unsigned	FSemacs		: 1;	/* FSedit uses emacs keys   */
	char		Password[Max_passlen+1];/* Plain password	    */
	int		Charset;		/* Character set	    */
};


struct  userhdr         usrconfighdr;           /* Users database          */
struct  userrec         usrconfig;
struct  userrec         exitinfo;               /* Users online data       */


#endif
