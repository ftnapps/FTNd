/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Global structure
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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


#ifndef _STRUCTS_H
#define _STRUCTS_H


#define Max_passlen     14      /* Define maximum passwd length            */


/*****************************************************************************
 *
 *  Global typedefs.
 *
 */

typedef enum {YES, NO, ASK, ONLY} ASKTYPE;
typedef enum {LOCALMAIL, NETMAIL, ECHOMAIL, NEWS, LIST} MSGTYPE;
typedef enum {BOTH, PRIVATE, PUBLIC, RONLY, FTNMOD, USEMOD} MSGKINDSTYPE;
typedef enum {IGNORE, CREATE, KILL} ORPHANTYPE;
typedef enum {SEND, RECV, BOTHDIR} NODETYPE;
typedef enum {POTS, ISDN, NETWORK, LOCAL} LINETYPE;
typedef enum {BROWSING, DOWNLOAD, UPLOAD, READ_POST, DOOR, SYSOPCHAT,
	      FILELIST, TIMEBANK, SAFE, WHOSON, OLR} DOESTYPE;
typedef enum {I_AVT0, I_ANSI, I_VT52, I_VT100, I_TTY} ITERM;
typedef enum {I_DZA, I_ZAP, I_ZMO, I_SLK, I_KER} IPROT;
typedef enum {E_NOISP, E_TMPISP, E_PRMISP} EMODE;
typedef enum {AREAMGR, FILEMGR, EMAIL} SERVICE;
typedef enum {FEEDINN, FEEDRNEWS, FEEDUUCP} NEWSFEED;
typedef enum {S_DIRECT, S_DIR, S_FTP} SESSIONTYPE;


/***********************************************************************
 *
 *  Routing definitions
 */
#define R_NOROUTE   0           /* No route descision made          */
#define R_ROUTE     1           /* Route to destination             */
#define	R_DIRECT    2		/* Direct route			    */
#define	R_REDIRECT  3		/* Redirect to new address	    */
#define	R_BOUNCE    4		/* Bounce back to sender	    */
#define	R_CC	    5		/* Make a CC			    */
#define	R_LOCAL	    6		/* Local destination		    */
#define	R_UNLISTED  7		/* Unlisted destination		    */


/***********************************************************************
 *
 *  Nodelist definitions.
 *
 */

#define MAXUFLAGS 16


/*
 *  Nodelist index file to nodelists. (node.files)
 */
typedef struct	_nlfil {
	char		filename[13];		/* Nodelist filename	*/
	char		domain[13];		/* Domain name		*/
	unsigned short	number;			/* File number		*/
} nlfil;



/*
 *  Nodelist index file for node lookup. (node.index)
 */
typedef struct	_nlidx {
	unsigned short	zone;			/* Zone number		*/
	unsigned short	net;			/* Net number		*/
	unsigned short	node;			/* Node number		*/
	unsigned short	point;			/* Point number		*/
	unsigned short	region;			/* Region of node	*/
	unsigned short	upnet;			/* Uplink net		*/
	unsigned short	upnode;			/* Uplink node		*/
	unsigned char	type;			/* Node type		*/
	unsigned char	pflag;			/* Node status		*/
	unsigned short	fileno;			/* Nodelist number	*/
	long		offset;			/* Offset in nodelist	*/
} nlidx;



/*
 *  Nodelist usernames index file. (node.users)
 */
typedef struct	_nlusr {
	char		user[36];		/* User name		*/
	long		record;			/* Record in index	*/
} nlusr;



/*
 *  type values
 */
#define NL_NONE		0
#define NL_ZONE		1
#define	NL_REGION	2
#define	NL_HOST		3
#define	NL_HUB		4
#define	NL_NODE		5
#define	NL_POINT	6



/*
 *  pflag values, all bits zero, node may be dialed analogue FTS-0001.
 *  the rest are special cases.
 */
#define NL_DOWN		0x01			/* Node is Down		*/
#define NL_HOLD		0x02			/* Node is Hold		*/
#define	NL_PVT		0x04			/* Private node		*/
#define NL_DUMMY	0x08			/* Dummy entry		*/
#define	NL_ISDN		0x10			/* ISDN Only node	*/
#define	NL_TCPIP	0x20			/* TCP/IP Only node	*/


/************************************************************************
 *
 *  Other BBS structures
 *
 */


#ifndef _SECURITYSTRUCT
#define _SECURITYSTRUCT

/*
 * Security structure
 */
typedef struct _security {
	unsigned int	level;			/* Security level	   */
	unsigned long	flags;			/* Access flags		   */
	unsigned long	notflags;		/* No Access flags	   */
} securityrec;

#endif


/* 
 * Fidonet 5d address structure 
 */
typedef struct _fidoaddr {
	unsigned short	zone;			/* Zone number		   */
	unsigned short	net;			/* Net number		   */
	unsigned short	node;			/* Node number		   */
	unsigned short	point;			/* Point number		   */
	char		domain[13];		/* Domain name (no dots)   */
} fidoaddr;



/*
 * Connected system structure
 */
typedef	struct _sysconnect {
	fidoaddr	aka;			/* Address of system	   */
	unsigned short	sendto;			/* If we send to system	   */
	unsigned short	receivefrom;		/* If we receive from      */
	unsigned 	pause		: 1;	/* If system is paused	   */
	unsigned 	cutoff		: 1;	/* Cutoff by moderator	   */
	unsigned	spare3		: 1;
	unsigned	spare4		: 1;
	unsigned	spare5		: 1;
	unsigned	spare6		: 1;
	unsigned	spare7		: 1;
	unsigned	spare8		: 1;
	unsigned	spare9		: 1;	/* Forces enough space	   */
} sysconnect;


	int		Diw;			/* Day in week index	   */
	int		Miy;			/* Month in year index	   */


/*
 * Statistic counters structure
 */
typedef struct _statcnt {
	unsigned long	tdow[7];		/* Days of current week	   */
	unsigned long	ldow[7];		/* Days of previous week   */
	unsigned long	tweek;			/* Week total counters	   */
	unsigned long	lweek;			/* Last week counters	   */
	unsigned long	month[12];		/* Monthly total counters  */
	unsigned long	total;			/* The ever growing total  */
} statcnt;



/*
 * Find replace match structure (phone translation etc).
 */
typedef struct _dual {
	char		match[21];		/* String to match	   */
	char		repl[21];		/* To replace with	   */
} dual;



/****************************************************************************
 *
 *  Datafile records structure in $MBSE_ROOT/etc
 *
 */


/*
 * Task Manager configuration (task.data)
 */
struct	taskrec {
	float		maxload;		/* Maximum system load	    */

	char		isp_connect[81];	/* ISP connect command	    */
	char		isp_hangup[81];		/* ISP hangup command	    */
	char		isp_ping1[41];		/* ISP ping host 1	    */
	char		isp_ping2[41];		/* ISP ping host 2	    */

	char		zmh_start[6];		/* Zone Mail Hour start	    */
	char		zmh_end[6];		/* Zone Mail Hour end	    */

	char		cmd_mailout[81];	/* mailout command	    */
	char		cmd_mailin[81];		/* mailin command	    */
	char		cmd_newnews[81];	/* newnews command	    */
	char		cmd_mbindex1[81];	/* mbindex command 1	    */
	char		cmd_mbindex2[81];	/* mbindex command 2	    */
	char		cmd_mbindex3[81];	/* mbindex command 3	    */
	char		cmd_msglink[81];	/* msglink command	    */
	char		cmd_reqindex[81];	/* reqindex command	    */

	int		xmax_pots;
	int		xmax_isdn;
	int		max_tcp;		/* maximum TCP/IP calls	    */

	unsigned	xipblocks	: 1;
	unsigned	debug		: 1;	/* debugging on/off	    */
};



/*
 * Special mail services (service.data)
 */
struct	servicehdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of records	    */
	time_t		lastupd;		/* Last updated at	    */
};

struct	servicerec {
	char		Service[16];		/* Service name		    */
	int		Action;			/* Service action	    */
	unsigned	Active		: 1;	/* Service is active	    */
	unsigned	Deleted		: 1;	/* Service is deleted	    */
};



/*
 * Domain translation (domain.data)
 */
struct	domhdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
	time_t		lastupd;		/* Last updated at	   */
};

struct	domrec {
        char            ftndom[61];             /* Fidonet domain          */
        char            intdom[61];             /* Internet domain         */
	unsigned	Active		: 1;	/* Domain is active	   */
	unsigned	Deleted		: 1;	/* Domain is deleted	   */
};



/*
 * System Control Structures (sysinfo.data)
 */
struct	sysrec {
	unsigned long	SystemCalls;		/* Total # of system calls */
	unsigned long	Pots;			/* POTS calls		   */
	unsigned long	ISDN;			/* ISDN calls		   */
	unsigned long	Network;		/* Network (internet) calls*/
	unsigned long	Local;			/* Local calls		   */
	unsigned long	xADSL;			/*			   */
	time_t		StartDate;		/* Start Date of BBS	   */
	char		LastCaller[37];		/* Last Caller to BBS	   */
	time_t		LastTime;		/* Time of last caller	   */
};



/*
 * Protocol Control Structure (protocol.data)
 */
struct	prothdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of records	    */
};

struct	prot {
	char		ProtKey[2];		/* Protocol Key             */
	char		ProtName[21];		/* Protocol Name            */
	char		ProtUp[51];		/* Upload Path & Binary     */
	char		ProtDn[51];		/* Download Path & Bianry   */
	unsigned	Available	: 1;	/* Available/Not Available  */
	unsigned	Batch		: 1;	/* Batching protocol	    */
	unsigned	Bidir		: 1;	/* Bi Directional	    */
	unsigned	Deleted		: 1;	/* Protocol is deleted	    */
	unsigned	Internal	: 1;	/* Internal protocol	    */
	char		Advice[31];		/* Small advice to user	    */
	int		Efficiency;		/* Protocol efficiency in % */
	securityrec	Level;			/* Sec. level to select	    */
};



/*
 * Oneliners Control Structure (oneline.data)
 */
struct	onelinehdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of record	    */
};

struct	oneline	{
	char		Oneline[81];		/* Oneliner text            */
	char		UserName[36];		/* User who wrote oneliner  */
	char		DateOfEntry[12];	/* Date of oneliner entry   */
	unsigned	Available	: 1;	/* Available Status         */
};



/*
 * File Areas Control Structure (fareas.data)
 */
struct	fileareashdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of records	    */
};

struct	fileareas {
	char		Name[45];		/* Filearea Name            */
	char		Path[81];		/* Filearea Path            */
	securityrec	DLSec;			/* Download Security        */
	securityrec	UPSec;			/* Upload Security          */
	securityrec	LTSec;			/* List Security            */
	int		Age;			/* Age to access area	    */
	unsigned	New		: 1;	/* New Files Check          */
	unsigned	Dupes		: 1;	/* Check for Duplicates     */
	unsigned	Free		: 1;	/* All files are Free       */
	unsigned	DirectDL	: 1;	/* Direct Download          */
	unsigned	PwdUP		: 1;	/* Password Uploads         */
	unsigned	FileFind	: 1;	/* FileFind Scan	    */
	unsigned	AddAlpha	: 1;	/* Add New files sorted	    */
	unsigned	Available	: 1;	/* Area is available	    */
	unsigned	CDrom		: 1;	/* Area is on CDrom	    */
	unsigned	FileReq		: 1;	/* Allow File Requests	    */
	char		BbsGroup[13];		/* BBS Group 		    */
	char		Password[21];		/* Area Password            */
	unsigned	DLdays;			/* Move not DL for days     */
	unsigned	FDdays;			/* Move if FD older than    */
	unsigned	MoveArea;		/* Move to Area             */
	int		Cost;			/* File Cost		    */
	char		FilesBbs[65];		/* Path to files.bbs if CD  */
	char		NewGroup[13];		/* Newfiles scan group	    */
	char		Archiver[6];		/* Archiver for area	    */
	unsigned	Upload;			/* Upload area		    */
};



/*
 * Index file for fast search of file requests (request.index)
 */
struct	FILEIndex {
	char		Name[13];		/* Short DOS name	   */
	char		LName[81];		/* Long filename	   */
	long		AreaNum;		/* File area number	   */
	long		Record;			/* Record in database	   */
};



/*
 * File Record Control Structure (fdb#.data)
 */
struct	FILERecord {
	char		Name[13];		/* DOS style filename	    */
	char		LName[81];		/* Long filename	    */
	char		xTicArea[9];		/* Tic area file came in    */
	unsigned long	TicAreaCRC;		/* CRC of TIC area name	    */
	off_t		Size;			/* File Size                */
	unsigned long	Crc32;			/* File CRC-32		    */
	char		Uploader[36];		/* Uploader name            */
	time_t		UploadDate;		/* Date/Time uploaded	    */
	time_t		FileDate;		/* Real file date	    */
	time_t		LastDL;			/* Last Download date	    */
	unsigned long	TimesDL;		/* Times file was dl'ed     */
	unsigned long	TimesFTP;		/* Times file was FTP'ed    */
	unsigned long	TimesReq;		/* Times file was frequed   */
	char		Password[16];		/* File password            */
	char		Desc[25][49];		/* file description         */
	int		Cost;			/* File cost		    */
	unsigned	Free         : 1;	/* Free File		    */
	unsigned	Deleted      : 1;	/* Deleted		    */
	unsigned	Missing      : 1;	/* Missing		    */
	unsigned	NoKill	     : 1;	/* Cannot be deleted        */
	unsigned	Announced    : 1;	/* File is announced	    */
	unsigned	Double	     : 1;	/* Double record	    */
};



/*
 * BBS List Control Structure (bbslist.data)
 */
struct	bbslisthdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of records	    */
};

struct	bbslist {
	char		UserName[36];		/* User Name                */
	char		DateOfEntry[12];	/* Entry date               */
	char		Verified[12];		/* Last Verify date	    */
	unsigned	Available	: 1;	/* Available Status	    */
	char		BBSName[41];		/* BBS Name                 */
	int		Lines;			/* Nr of phone lines	    */
	char		Phone[5][21];		/* BBS phone number         */
	char		Speeds[5][41];		/* Speeds for each line	    */
	fidoaddr	FidoAka[5];		/* Fidonet Aka's	    */
	char		Software[20];		/* BBS Software             */
	char		Sysop[36];		/* Name of Sysop            */
	int		Storage;		/* Storage amount in megs   */
	char		Desc[2][81];	 	/* Description              */
	char		IPaddress[51];		/* IP or domain name	    */
	char		Open[21];		/* Online time		    */
};



/* 
 * Last Callers Control Structure (lastcall.data)
 */
struct	lastcallershdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of records	    */
};

struct  lastcallers {  
	char		UserName[36];		/* User Name                */
	char		Handle[36];		/* User Handle              */
	char		Name[9];		/* Unix Name		    */
	char		TimeOn[6];		/* Time user called bbs     */
	int		CallTime;		/* Time this call	    */
	char		Device[10];		/* Device user used         */
	int		Calls;			/* Total calls to bbs       */
	unsigned int	SecLevel;		/* Users security level	    */
	char		Speed[21];		/* Caller speed		    */
	unsigned	Hidden     : 1;		/* Hidden or Not at time    */
	unsigned	Download   : 1;		/* If downloaded	    */
	unsigned	Upload	   : 1;		/* If uploaded		    */
	unsigned	Read	   : 1;		/* If read messages	    */
	unsigned	Wrote	   : 1;		/* If wrote a message	    */
	unsigned	Chat	   : 1;		/* If did chat		    */
	unsigned	Olr	   : 1;		/* If used Offline Reader   */
	unsigned	Door	   : 1;		/* If used a Door	    */
	char		Location[28];		/* User Location            */
};



/* 
 * System Control Structure (config.data)
 */
struct	sysconfig {
						/* Registration Info	    */
	char		sysop_name[36];		/* Sysop Name               */
	char		bbs_name[36];		/* BBS Name                 */
	char		sysop[9];		/* Unix Sysop name	    */
	char		location[36];		/* System location	    */
	char		bbsid[9];		/* QWK/Bluewave BBS ID	    */
	char		bbsid2[3];		/* Omen filename	    */
	char		sysdomain[36];		/* System Domain name	    */
	char		comment[56];		/* Do what you like here    */
	char		origin[51];		/* Default origin line	    */

						/* FileNames		    */
	char		error_log[15];		/* Name of Error Log	    */
	char		default_menu[15];	/* Default Menu		    */
	char		current_language[15];	/* Default Language	    */
	char		chat_log[15];		/* Chat Logfile		    */
	char		welcome_logo[15];	/* Welcome Logofile	    */

						/* Paths		    */
	char		rnewspath[65];		/* Path to rnews	    */
	char		bbs_menus[65];		/* Default Menus	    */
	char		bbs_txtfiles[65];	/* Default Textfiles	    */
	char		nntpnode[65];		/* NNTP server		    */
	char		msgs_path[65];		/* Path to *.msg area	    */
	char		alists_path[65];	/* Area lists storage	    */
	char		req_magic[65];		/* Request magic directory  */
	char		bbs_usersdir[65];	/* Users Home Dir Base	    */
	char		nodelists[65];		/* Nodelists		    */
	char		inbound[65];		/* Inbound directory	    */
	char		pinbound[65];		/* Protected inbound	    */
	char		outbound[65];		/* Outbound		    */
	char		externaleditor[65];	/* External mail editor	    */
	char		dospath[65];		/* DOS path		    */
	char		uxpath[65];		/* Unix path		    */

						/* Allfiles/Newfiles	    */
	char		ftp_base[65];		/* FTP root		    */
	int		newdays;		/* New files since	    */
	securityrec	security;		/* Max level list	    */

	unsigned	addr4d		  : 1;	/* Use 4d addressing	    */
	unsigned	leavecase	  : 1;	/* Leave outbound case	    */

						/* BBS Globals		    */
	int		xmax_login;
	unsigned	NewAreas	  : 1;	/* Notify if new msg areas  */
	unsigned	xelite_mode       : 1;
	unsigned	slow_util	  : 1;	/* Run utils slowly	    */
	unsigned	exclude_sysop     : 1;	/* Exclude Sysop from lists */
	unsigned	xUseSysDomain	  : 1;
	unsigned	xChkMail          : 1;
	unsigned	iConnectString	  : 1;  /* Display Connect String   */
	unsigned	iAskFileProtocols : 1;	/* Ask user FileProtocols   */
                                                /* before every d/l or u/l  */
	unsigned	sysop_access;		/* Sysop Access Security    */
	int		password_length;        /* Minimum Password Length  */
	long		bbs_loglevel;		/* Logging level for BBS    */
	int		iPasswd_Char;		/* Password Character       */
  	int		iQuota;			/* User homedir quota in MB */
	int		idleout;                /* Idleout Value            */
	int		CityLen;		/* Minimum city length	    */
	short		OLR_NewFileLimit;	/* Limit Newfilesscan days  */
	unsigned	iCRLoginCount;          /* Count login Enters       */

						/* New Users		    */
	securityrec	newuser_access;		/* New Users Access level   */
	int		OLR_MaxMsgs;		/* OLR Max nr Msgs download */
	unsigned	iCapUserName    : 1;	/* Capitalize Username      */
	unsigned	iAnsi	        : 1;	/* Ask Ansi                 */
	unsigned	iSex            : 1;	/* Ask Sex                  */
	unsigned	iDataPhone      : 1;	/* Ask Data Phone           */
	unsigned	iVoicePhone     : 1;	/* Ask Voice Phone          */
	unsigned	iHandle         : 1;	/* Ask Alias/Handle         */
	unsigned	iDOB            : 1;	/* Ask Date of Birth        */
	unsigned	iTelephoneScan  : 1;	/* Telephone Scan           */
	unsigned	iLocation       : 1;	/* Ask Location             */
	unsigned	iCapLocation    : 1;	/* Capitalize Location      */
	unsigned	iHotkeys        : 1;	/* Ask Hot-Keys             */
	unsigned	GiveEmail	: 1;	/* Give user email	    */
	unsigned	AskAddress      : 1;	/* Ask Home Address	    */
	unsigned	iOneName        : 1;	/* Allow one user name      */
	unsigned	iCrashLevel;		/* User level for crash mail*/
	unsigned	iAttachLevel;		/* User level for fileattach*/

						/* Colors		    */
	int		TextColourF;            /* Text Colour Foreground   */
	int		TextColourB;            /* Text Colour Background   */
	int		UnderlineColourF;       /* Underline Text Colour    */
	int		UnderlineColourB;       /* Underline Colour         */
	int		InputColourF;           /* Input Text Colour        */
	int		InputColourB;           /* Input Text Colour        */
	int		CRColourF;              /* CR Text Colour           */
	int		CRColourB;              /* CR Text Colour           */
	int		MoreF;                  /* More Prompt Text Colour  */
	int		MoreB;                  /* More Prompt Text Colour  */
	int		HiliteF;                /* Hilite Text Colour       */
	int		HiliteB;                /* Hilite Text Colour 	    */
	int		FilenameF;              /* Filename Colour          */
	int		FilenameB;              /* Filename Colour          */
	int		FilesizeF;              /* Filesize Colour          */
	int		FilesizeB;              /* Filesize Colour          */
	int		FiledateF;              /* Filedate Colour          */
	int		FiledateB;              /* Filedate Colour          */
	int		FiledescF;              /* Filedesc Colour          */
	int		FiledescB;              /* Filedesc Colour          */
	int		MsgInputColourF;        /* MsgInput Filename Colour */
	int		MsgInputColourB;        /* MsgInput Filename Colour */

	char		xNuScreen[50];          /* Obsolete Next User Door  */
	char		xNuQuote[81];

						/* Safe Cracker Door	    */
	int		iSafeFirstDigit;        /* Safe Door First Digit    */
	int		iSafeSecondDigit;       /* Safe Door Second Digit   */
	int		iSafeThirdDigit;        /* Safe Door Third Digit    */
	int		iSafeMaxTrys;           /* Max trys per day         */
	int		iSafeMaxNumber;         /* Maximum Safe Number      */
	unsigned	iSafeNumGen  : 1;       /* Use number generator     */
	char		sSafePrize[81];         /* Safe Prize               */
	char		sSafeWelcome[81];       /* Safe welcome file        */
	char		sSafeOpened[81];        /* Opended safe file        */

						/* Sysop Paging		    */
	int		iPageLength;		/* Page Length in Seconds   */
	int		iMaxPageTimes;		/* Max Pages per call       */
	unsigned	iAskReason     : 1;	/* Ask Reason               */
	int		iSysopArea;		/* Msg Area if Sysop not in */
	unsigned	iExternalChat  : 1;	/* Use External Chat        */
	char		sExternalChat[50];	/* External Chat Program    */
	unsigned	iAutoLog       : 1;	/* Log Chats ?              */
	char		sChatDevice[20];	/* Chat Device              */
	unsigned	iChatPromptChk;		/* Check for chat at prompt */
	unsigned	iStopChatTime;		/* Stop time during chat    */
	char		cStartTime[7][6];	/* Starting Times	    */
	char		cStopTime[7][6];	/* Stop Times		    */
	char		sCallScript[51];	/* Sysop External Call scr. */

						/* Mail Options		    */
	char		xquotestr[11];		/* Quote String		    */

	int		xMaxTimeBalance;	/* Obsolete Time Bank Door  */
	int		xMaxTimeWithdraw;
	int		xMaxTimeDeposit;
	int		xMaxByteBalance;
	int		xMaxByteWithdraw;
	int		xMaxByteDeposit;
	unsigned	xNewBytes	: 1;
	char		xTimeRatio[7];
	char		xByteRatio[7];

	long		new_groups;		/* Maximum newfiles groups  */
	int		new_split;		/* Split reports at KB.	    */
	int		new_force;		/* Force split at KB.	    */
	char		startname[9];		/* BBS startup name	    */
	char		extra4[239];

						/* TIC Processing	    */
	unsigned	ct_KeepDate	: 1;	/* Keep Filedate	    */
	unsigned	ct_KeepMgr	: 1;	/* Keep Mgr netmails	    */
	unsigned	xct_ResFuture	: 1;	/* Reset Future filedates   */
	unsigned	ct_LocalRep	: 1;	/* Respond to local requests*/
	unsigned	xct_ReplExt	: 1;	/* Replace Extension	    */
	unsigned	ct_PlusAll	: 1;	/* Filemgr: allow +%*	    */
	unsigned	ct_Notify	: 1;	/* Filemgr: Notify on/off   */
	unsigned	ct_Passwd	: 1;	/* Filemgr: Passwd change   */
	unsigned	ct_Message	: 1;	/* Filemgr: Msg file on/off */
	unsigned	ct_TIC		: 1;	/* Filemgr: TIC files on/off*/
	unsigned	ct_Pause	: 1;	/* Filemgr: Allow Pause	    */
	char		logfile[15];		/* System Logfile	    */
	int		OLR_MaxReq;		/* Max nr of Freq's	    */
	int		tic_days;		/* Keep on hold for n days  */
	char		hatchpasswd[21];	/* Internal Hatch Passwd    */
	unsigned long	drspace;		/* Minimum free drivespace  */
	char		xmgrname[5][21];	/* Areamgr names	    */
	long		tic_systems;		/* Systems in database	    */
	long		tic_groups;		/* Groups in database	    */
	long		tic_dupes;		/* TIC dupes dabase size    */
	char		badtic[65];		/* Bad TIC's path	    */
	char		ticout[65];		/* TIC queue		    */

						/* Mail Tosser		    */
	char		pktdate[65];		/* pktdate by Tobias Ernst  */
	int		maxpktsize;		/* Maximum packet size	    */
	int		maxarcsize;		/* Maximum archive size	    */
	int		toss_old;		/* Reject older then days   */
	char		xtoss_log[11];
	long		util_loglevel;		/* Logging level for utils  */
	char		badboard[65];		/* Bad Mail board	    */
	char		dupboard[65];		/* Dupe Mail board	    */
	char		popnode[65];		/* Node with pop3 boxes     */
	char		smtpnode[65];		/* SMTP node		    */
	int		toss_days;		/* Keep on hold		    */
	int		toss_dupes;		/* Dupes in database	    */
	int		defmsgs;		/* Default purge messages   */
	int		defdays;		/* Default purge days	    */
	int		freespace;		/* Free diskspace in MBytes */
	long		toss_systems;		/* Systems in database	    */
	long		toss_groups;		/* Groups in database       */
	char		xareamgr[5][21];	/* Areamgr names	    */

						/* Flags		    */
	char		fname[32][17];		/* Name of the access flags */
	fidoaddr	aka[40];		/* Fidonet AKA's	    */
	unsigned short	akavalid[40];		/* Fidonet AKA valid/not    */

	long		cico_loglevel;		/* Mailer loglevel	    */
	long		timeoutreset;		/* Reset timeout	    */
	long		timeoutconnect;		/* Connect timeout	    */
	long		dialdelay;		/* Delay between calls	    */
	unsigned	NoFreqs		: 1;	/* Don't allow requests	    */
	unsigned	NoCall		: 1;	/* Don't call		    */
	unsigned	xNoHold		: 1;
	unsigned	xNoPUA		: 1;
	unsigned	NoEMSI		: 1;	/* Don't do EMSI	    */
	unsigned	NoWazoo		: 1;	/* Don't do Yooho/2U2	    */
	unsigned	NoZmodem	: 1;	/* Don't do Zmodem	    */
	unsigned	NoZedzap	: 1;	/* Don't do Zedzap	    */

	unsigned	xNoJanus	: 1;
	unsigned	NoHydra		: 1;	/* Don't do Hydra	    */
	unsigned	xNoIBN		: 1;
	unsigned	xNoITN		: 1;
	unsigned	xNoIFC		: 1;

	char		Phone[21];		/* Default phonenumber	    */
	unsigned long	Speed;			/* Default linespeed	    */
	char		Flags[31];		/* Default EMSI flags	    */
	int		Req_Files;		/* Maximum files request    */
	int		Req_MBytes;		/* Maximum MBytes request   */
	char		extra5[96];
	dual		phonetrans[40];		/* Phone translation table  */

                                                /* FTP Daemon               */
	int             ftp_limit;              /* Connections limit        */
	int             ftp_loginfails;         /* Maximum login fails      */
	unsigned        ftp_compress    : 1;    /* Allow compress           */
	unsigned        ftp_tar         : 1;    /* Allow tar                */
	unsigned        ftp_upl_mkdir   : 1;    /* Allow mkdir              */
	unsigned        ftp_log_cmds	: 1;	/* Log user commands	    */
	unsigned        ftp_anonymousok	: 1;	/* Allow anonymous logins   */
	unsigned        ftp_mbseok	: 1;	/* Allow mbse user login    */
	unsigned        ftp_x7          : 1;
	unsigned        ftp_x8          : 1;
	unsigned        ftp_x9          : 1;
	char            ftp_readme_login[21];   /* Readme file for login    */
	char		ftp_readme_cwd[21];	/* Readme file for cwd	    */
	char		ftp_msg_login[21];	/* Message file for login   */
	char		ftp_msg_cwd[21];	/* Message file for cwd	    */
	char		ftp_msg_shutmsg[41];	/* Shutdown message	    */
	char		ftp_upl_path[81];	/* Upload path		    */
	char		ftp_banner[81];		/* Banner file		    */
	char		ftp_email[41];		/* Email address	    */
	char		ftp_pth_filter[41];	/* Path filter expression   */
	char		ftp_pth_message[81];	/* Message to display	    */

						/* HTML creation	    */
	char		www_root[81];		/* HTML doc root	    */
	char		www_link2ftp[21];	/* Link name to ftp_base    */
	char		www_url[41];		/* Webserver URL	    */
	char		www_charset[21];	/* Default characher set    */
	char		xwww_tbgcolor[21];
	char		xwww_hbgcolor[21];
	char		www_author[41];		/* Author name in pages	    */
	char		www_convert[81];	/* Graphic Convert command  */
	char		xwww_icon_home[21];
	char		xwww_name_home[21];
	char		xwww_icon_back[21];
	char		xwww_name_back[21];
	char		xwww_icon_prev[21];
	char		xwww_name_prev[21];
	char		xwww_icon_next[21];
	char		xwww_name_next[21];
	int		www_files_page;		/* Files per webpage	    */

	fidoaddr	EmailFidoAka;		/* Email aka in fidomode    */
	fidoaddr	UUCPgate;		/* UUCP gateway in fidomode */
	int		EmailMode;		/* Email mode to use	    */
	unsigned	modereader	: 1;	/* NNTP Mode Reader	    */
	unsigned	allowcontrol	: 1;	/* Allow control messages   */
	unsigned	dontregate	: 1;	/* Don't regate gated msgs  */
	char		nntpuser[16];		/* NNTP username	    */
	char		nntppass[16];		/* NNTP password	    */
	long		nntpdupes;		/* NNTP dupes database size */
	int		newsfeed;		/* Newsfeed mode	    */
	int		maxarticles;		/* Default max articles	    */
	char		bbs_macros[65];		/* Default macros path	    */
	char		out_queue[65];		/* Outbound queue path	    */

	char		mgrlog[15];		/* Area/File-mgr logfile    */
	char            aname[32][17];          /* Name of areas flags	    */

	unsigned	ca_PlusAll	: 1;	/* Areamgr: allow +%*       */
	unsigned	ca_Notify	: 1;	/* Areamgr: Notify on/off   */
	unsigned	ca_Passwd	: 1;	/* Areamgr: Passwd change   */
	unsigned	ca_Pause	: 1;	/* Areamgr: Allow Pause     */
	unsigned	ca_Check	: 1;	/* Flag for upgrade check   */

	char		rulesdir[65];		/* Area rules directory	    */
};



/*
 * Limits Control Structure (limits.data)
 */
struct	limitshdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
};

struct	limits {
	unsigned long	Security;		/* Security Level          */
	long		Time;			/* Amount of time per call */
	unsigned long	DownK;			/* Download KB per call    */
	unsigned int	DownF;			/* Download files per call */
	char		Description[41];	/* Description for level   */
	unsigned	Available	: 1;	/* Is this limit available */
	unsigned	Deleted		: 1;	/* Is this limit deleted   */
};                             



/*
 * Menu File Control Structure (*.mnu)
 */
struct	menufile {
	char		MenuKey[2];		/* Menu Key                 */
	int		MenuType;		/* Menu Type                */
	char		OptionalData[81];	/* Optional Date            */
	char		Display[81];		/* Menu display line	    */
	securityrec	MenuSecurity;		/* Menu Security Level      */
	int		Age;			/* Minimum Age to use menu  */
	unsigned int	xMaxSecurity;
	char		Password[15];		/* Menu Password            */
	char		TypeDesc[30];		/* Menu Type Description    */
	unsigned	AutoExec	: 1;	/* Auto Exec Menu Type      */
	unsigned	NoDoorsys	: 1;	/* Suppress door.sys	    */
	unsigned	Y2Kdoorsys	: 1;	/* Write Y2K style door.sys */
	unsigned	Comport		: 1;	/* Vmodem comport mode	    */
	unsigned	NoSuid		: 1;	/* Execute door nosuid	    */
	unsigned	NoPrompt	: 1;	/* No prompt after door	    */
	long		xCredit;
	int		HiForeGnd;		/* High ForeGround color    */
	int		HiBackGnd;		/* High ForeGround color    */
	int		ForeGnd;		/* Normal ForeGround color  */
	int		BackGnd;		/* Normal BackGround color  */
};



/*
 * News dupes database. Stores newsgroupname and CRC32 of article msgid.
 */
struct	newsdupes {
	char		NewsGroup[65];		/* Name of the group	    */
	unsigned long	Crc;			/* CRC32 of msgid	    */
};



/* 
 * Message Areas Structure (mareas.data)
 * This is also used for echomail, netmail and news
 */
struct	msgareashdr {
	long		hdrsize;		/* Size of header	    */
	long		recsize;		/* Size of records	    */
	long		syssize;		/* Size for systems	    */
	time_t		lastupd;		/* Last date stats updated  */
};

struct msgareas {
	char		Name[41];		/* Message area Name        */
	char		Tag[51];		/* Area tag		    */
	char		Base[65];		/* JAM base		    */
	char		QWKname[21];		/* QWK area name	    */
	int		Type;			/* Msg Area Types           */
						/* Local, Net, Echo, News,  */
						/* Listserv		    */
	int		MsgKinds;		/* Type of Messages         */
						/* Public,Private,ReadOnly  */
	int		DaysOld;		/* Days to keep messages    */
	int		MaxMsgs;		/* Maximum number of msgs   */
	int		UsrDelete;		/* Allow users to delete    */
	securityrec	RDSec;        		/* Read Security            */
	securityrec	WRSec;			/* Write Security           */
	securityrec	SYSec;			/* Sysop Security           */
	int		Age;			/* Age to access this area  */
	char		Password[20];		/* Area Password            */
	char		Group[13];		/* Group Area               */
	fidoaddr	Aka;			/* Fidonet address	    */
	char		Origin[65];		/* Origin Line              */
	unsigned	Aliases		: 1;	/* Allow aliases	    */
	unsigned	NetReply;		/* Area for Netmail reply   */
	unsigned	Active		: 1;	/* Area is active	    */
	unsigned	OLR_Forced	: 1;	/* OLR Area always on	    */
	unsigned	xFileAtt	: 1;	/* Allow file attach	    */
	unsigned	xModerated	: 1;	/* Moderated newsgroup	    */
	unsigned	Quotes		: 1;	/* Add random quotes	    */
	unsigned	Mandatory	: 1;	/* Mandatory for nodes	    */
	unsigned	UnSecure	: 1;	/* UnSecure tossing	    */
	unsigned	xUseFidoDomain	: 1;
	unsigned	OLR_Default	: 1;	/* OLR Deafault turned on   */
	unsigned	xPrivate	: 1;	/* Pvt bits allowed	    */
	unsigned	xCheckSB	: 1;
	unsigned	xPassThru	: 1;
	unsigned	xNotiFied	: 1;
	unsigned	xUplDisc	: 1;
	statcnt		Received;		/* Received messages	    */
	statcnt		Posted;			/* Posted messages	    */
	time_t		LastRcvd;		/* Last time msg received   */
	time_t		LastPosted;		/* Last time msg posted	    */
	char		Newsgroup[81];		/* Newsgroup/Mailinglist    */
	char		Distribution[17];	/* Ng distribution	    */
	char		xModerator[65];
	int		Rfccode;		/* RFC characterset	    */
	int		Ftncode;		/* FTN characterset	    */
	int		MaxArticles;		/* Max. newsarticles to get */
	securityrec	LinkSec;		/* Link security flags	    */
};



/*
 * Structure for Language file (language.data)
 */
struct	languagehdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
};

struct language {
	char		Name[30];		/* Name of Language        */
	char		LangKey[2];		/* Language Key            */
	char		MenuPath[81];		/* Path of menu directory  */
	char		TextPath[81];		/* Path of text files      */
	unsigned	Available	: 1;	/* Availability of Language*/
	unsigned	Deleted		: 1;	/* Language is deleted	   */
	char		Filename[81];		/* Path of language file   */
	securityrec	Security;		/* Security level	   */
	char		MacroPath[81];		/* Path to the macro files */
};



/* 
 * Structure for Language Data File (english.lang)
 */
struct langdata {
	char		sString[85];		/* Language text	   */
	char		sKey[30];		/* Keystroke characters	   */
};
 


/*
 * Structure for Safe Cracker Door Data File (safe.data)
 */
struct	crackerhdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
};

struct	cracker {
	char		Date[12];		/* Date used		   */
	char		Name[36];		/* User name		   */
	int		Trys;			/* Trys today		   */
	unsigned	Opened : 1;		/* If user succeeded	   */
};



/*
 * Fidonet Networks (fidonet.data)
 */
struct _fidonethdr {
	long		hdrsize;		/* Size of header record   */
	long		recsize;		/* Size of records	   */
};

typedef	struct	_seclist {
	char		nodelist[9];		/* Secondary nodelist name */
	unsigned short	zone;			/* Adress for this list	   */
	unsigned short	net;
	unsigned short	node;
} seclistrec;

struct _fidonet {
	char		domain[13];		/* Network domain name	   */
	char		nodelist[9];		/* Nodelist name	   */
	seclistrec	seclist[6];		/* 6 secondary nodelists   */
	unsigned short	zone[6];		/* Maximum 6 zones	   */
	char		comment[41];		/* Record comment	   */
	unsigned	available : 1;		/* Network available	   */
	unsigned	deleted	  : 1;		/* Network is deleted	   */
};



/*
 * Archiver programs (archiver.data)
 */
struct _archiverhdr {
	long		hdrsize;		/* Size of header record   */
	long		recsize;		/* Size of records	   */
};

struct _archiver {
	char		comment[41];		/* Archiver comment	   */
	char		name[6];		/* Archiver name	   */
	unsigned	available	: 1;	/* Archiver available	   */
	unsigned	deleted  	: 1;	/* Archiver is deleted     */
	char		farc[65];		/* Archiver for files	   */
	char		marc[65];		/* Archiver for mail	   */
	char		barc[65];		/* Archiver for banners	   */
	char		tarc[65];		/* Archiver test	   */
	char		funarc[65];		/* Unarc files		   */
	char		munarc[65];		/* Unarc mail		   */
	char		iunarc[65];		/* Unarc FILE_ID.DIZ	   */
};



/*
 * Virus scanners (virscan.data)
 */
struct	_virscanhdr {
	long		hdrsize;		/* Size of header record   */
	long		recsize;		/* Size of records	   */
};

struct	_virscan {
	char		comment[41];		/* Comment		   */
	char		scanner[65];		/* Scanner command	   */
	unsigned	available	: 1;	/* Scanner available	   */
	unsigned	deleted  	: 1;	/* Scanner is deleted	   */
	char		options[65];		/* Scanner options	   */
	int		error;			/* Error level for OK	   */
};



/*
 * TTY information
 */
struct	_ttyinfohdr {
	long		hdrsize;		/* Size of header record   */
	long		recsize;		/* Size of records	   */
};

struct	_ttyinfo {
	char		comment[41];		/* Comment for tty	   */
	char		tty[7];			/* TTY device name	   */
	char		phone[26];		/* Phone or dns name	   */
	char		speed[21];		/* Max speed for this tty  */
	char		flags[31];		/* Fidonet capabilty flags */
	int		type;			/* Pots/ISDN/Netw/Local	   */
	unsigned	available	: 1;	/* Available flag	   */
	unsigned	authlog		: 1;	/* Is speed logged	   */
	unsigned	honor_zmh	: 1;	/* Honor ZMH on this line  */
	unsigned	deleted		: 1;	/* Is deleted		   */
	unsigned	callout		: 1;	/* Callout allowed	   */
	char		modem[31];		/* Modem type		   */
	char		name[36];		/* EMSI line name	   */
	long		portspeed;		/* Locked portspeed	   */
};



/*
 * Modem definitions.
 */
struct	_modemhdr {
	long		hdrsize;		/* Size of header record   */
	long		recsize;		/* Size of records	   */
};

struct	_modem {
	char		modem[31];		/* Modem type		   */
	char		init[3][61];		/* Init strings		   */
	char		ok[11];			/* OK string		   */
	char		hangup[41];		/* Hangup command	   */
	char		info[41];		/* After hangup get info   */
	char		dial[41];		/* Dial command		   */
	char		connect[20][31];	/* Connect strings	   */
	char		error[10][21];		/* Error strings	   */
	char		reset[61];		/* Reset string		   */
	int		costoffset;		/* Offset add to connect   */
	char		speed[16];		/* EMSI speed string	   */
	unsigned	available	: 1;	/* Is modem available	   */
	unsigned	deleted		: 1;	/* Is modem deleted	   */
	unsigned	stripdash	: 1;	/* Strip dashes from dial  */
};



/*
 * Structure for TIC areas (tic.data)
 */
struct	_tichdr {
	long		hdrsize;		/* Size of header 	   */
	long		recsize;		/* Size of records	   */
	long		syssize;		/* Size for systems	   */
	time_t		lastupd;		/* Last statistic update   */
};

struct	_tic {
	char		Name[21];		/* Area name		   */
	char		Comment[56];		/* Area comment		   */
	long		FileArea;		/* The BBS filearea	   */
	char		Message[15];		/* Message file		   */
	char		Group[13];		/* FDN group		   */
	int		KeepLatest;		/* Keep latest n files	   */
	long		xOld[6];
	time_t		AreaStart;		/* Startdate		   */
	fidoaddr	Aka;			/* Fidonet address	   */
	char		Convert[6];		/* Archiver to convert	   */
	time_t		LastAction;		/* Last Action in this area*/
	char		Banner[15];		/* Banner file		   */
	long		xUnitCost;
	long		xUnitSize;
	long		xAddPerc;
	unsigned	Replace		: 1;	/* Allow Replace	   */
	unsigned	DupCheck	: 1;	/* Dupe Check		   */
	unsigned	Secure		: 1;	/* Check for secure system */
	unsigned	Touch		: 1;	/* Touch filedate	   */
	unsigned	VirScan 	: 1;	/* Run Virus scanners	   */
	unsigned	Announce	: 1;	/* Announce files	   */
	unsigned	UpdMagic	: 1;	/* Update Magic database   */
	unsigned	FileId		: 1;	/* Check FILE_ID.DIZ	   */
	unsigned	ConvertAll	: 1;	/* Convert allways	   */
	unsigned	SendOrg		: 1;	/* Send Original to downl's*/
	unsigned	Mandat		: 1;	/* Mandatory area	   */
	unsigned	Notified	: 1;	/* Notified if disconn.	   */
	unsigned	UplDiscon	: 1;	/* Uplink disconnected	   */
	unsigned	Active		: 1;	/* If this area is active  */
	unsigned	Deleted		: 1;	/* If this area is deleted */
	statcnt		Files;			/* Total processed files   */
	statcnt		KBytes;			/* Total processed KBytes  */
	securityrec	LinkSec;		/* Link security flags	   */
};



/*
 * Nodes, up- and downlinks. (nodes.data)
 */
struct	_nodeshdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
	long		filegrp;		/* Size for file groups	   */
	long		mailgrp;		/* Size for mail groups	   */
	time_t		lastupd;		/* Last statistic update   */
};

struct	_nodes {
	char		Sysop[36];		/* Sysop name		    */
	fidoaddr	Aka[20];		/* Aka's for this system    */
	char		Fpasswd[16];		/* Files password	    */
	char		Epasswd[16];		/* Mail password	    */
	char		Apasswd[16];		/* Areamgr password	    */
	char		UplFmgrPgm[9];		/* Uplink FileMgr program   */
	char		UplFmgrPass[16];	/* Uplink FileMgr password  */
	char		UplAmgrPgm[9];		/* Uplink AreaMgr program   */
	char		UplAmgrPass[16];	/* Uplink AreaMgr password  */

	unsigned	Direct		: 1;	/* Netmail Direct	    */
	unsigned	Message		: 1;	/* Send Message w. files    */
	unsigned	Tic		: 1;	/* Send TIC files	    */
	unsigned	Notify		: 1;	/* Send Notify messages	    */
	unsigned	FileFwd		: 1;	/* Accept File Forward	    */
	unsigned	MailFwd		: 1;	/* Accept Mail Forward	    */
	unsigned	AdvTic		: 1;	/* Advanced Tic files	    */
	unsigned	Billing		: 1;	/* Cost sharing on/off	    */

	unsigned	BillDirect	: 1;	/* Send bill direct	    */
	unsigned	Crash		: 1;	/* Netmail crash	    */
	unsigned	Hold		: 1;	/* Netmail hold		    */
	unsigned	AddPlus		: 1;	/* Add + for uplink msgs    */
	unsigned	MailPwdCheck	: 1;	/* Mail password check	    */
	unsigned	Deleted		: 1;	/* Node is deleted	    */
	unsigned	NoEMSI		: 1;	/* No EMSI handshake	    */
	unsigned	NoWaZOO		: 1;	/* No YooHoo/2U2 handshake  */

	unsigned	NoFreqs		: 1;	/* Don't allow requests	    */
	unsigned	NoCall		: 1;	/* Don't call this node	    */
	unsigned	TIC_AdvSB	: 1;	/* Advanced tic SB lines    */
	unsigned	TIC_To		: 1;	/* Add To line to ticfile   */
	unsigned	NoZmodem	: 1;	/* Don't use Zmodem	    */
	unsigned	NoZedzap	: 1;	/* Don't use Zedzap	    */
	unsigned	xNoJanus	: 1;	/* Don't use Janus	    */
	unsigned	NoHydra		: 1;	/* Don't use Hydra	    */

	unsigned	xNoIBN		: 1;
	unsigned	PackNetmail	: 1;	/* Pack netmail		    */
	unsigned	ARCmailCompat	: 1;	/* ARCmail Compatibility    */
	unsigned	ARCmailAlpha	: 1;	/* Allow a..z ARCmail name  */
	unsigned	FNC		: 1;	/* Node needs 8.3 filenames */
	unsigned	xNoITN		: 1;
	unsigned	xNoIFC		: 1;

	char		xExtra[94];
	time_t		StartDate;		/* Node start date	    */
	time_t		LastDate;		/* Last action date	    */
	long		Credit;			/* Node's credit	    */
	long		Debet;			/* Node's debet		    */
	long		AddPerc;		/* Add Percentage	    */
	long		WarnLevel;		/* Warning level	    */
	long		StopLevel;		/* Stop level		    */
	fidoaddr	RouteVia;		/* Routing address	    */
	int		Language;		/* Language for netmail	    */
	statcnt		FilesSent;		/* Files sent to node	    */
	statcnt		FilesRcvd;		/* Files received from node */
	statcnt		F_KbSent;		/* File KB. sent	    */
	statcnt		F_KbRcvd;		/* File KB. received	    */
	statcnt		MailSent;		/* Messages sent to node    */
	statcnt		MailRcvd;		/* Messages received	    */
	char		dial[41];		/* Dial command override    */
	char		phone[2][21];		/* Phone numbers override   */

	char		Spasswd[16];		/* Session password	    */
	int		Session_out;		/* Outbound session type    */
	int		Session_in;		/* Inbound session type	    */

						/* Directory in/outbound    */
	char		Dir_out_path[65];	/* Outbound files	    */
	char		Dir_out_clock[65];	/* Outbound filelock check  */
	char		Dir_out_mlock[65];	/* Outbound filelock create */
	char		Dir_in_path[65];	/* Inbound files	    */
	char		Dir_in_clock[65];	/* Inbound filelock check   */
	char		Dir_in_mlock[65];	/* Inbound filelock create  */
	unsigned	Dir_out_chklck	: 1;	/* Outbound check lock	    */
	unsigned	Dir_out_waitclr	: 1;	/* Outbound wait for clear  */
	unsigned	Dir_out_mklck	: 1;	/* Outbound create lock	    */
	unsigned	Dir_in_chklck	: 1;	/* Inbound check lock	    */
	unsigned	Dir_in_waitclr	: 1;	/* Inbound wait for clear   */
	unsigned	Dir_in_mklck	: 1;	/* Inbound create lock	    */

						/* FTP transfers		    */
	char		FTP_site[65];		/* Site name or IP address  */
	char		FTP_user[17];		/* Username		    */
	char		FTP_pass[17];		/* Password		    */
	char		FTP_starthour[6];	/* Start hour		    */
	char		FTP_endhour[6];		/* End hour		    */
	char		FTP_out_path[65];	/* Outbound files path	    */
	char		FTP_out_clock[65];	/* Outbound filelock check  */
	char		FTP_out_mlock[65];	/* Outbound filelock create */
	char		FTP_in_path[65];	/* Inbound files path	    */
	char		FTP_in_clock[65];	/* Inbound filelock check   */
	char		FTP_in_mlock[65];	/* Inbound filelock create  */
	unsigned	FTP_lock1byte	: 1;	/* Locksize 1 or 0 bytes    */
	unsigned	FTP_unique	: 1;	/* Unique storage	    */
	unsigned	FTP_uppercase	: 1;	/* Force uppercase	    */
	unsigned	FTP_lowercase	: 1;	/* Force lowercase	    */
	unsigned	FTP_passive	: 1;	/* Passive mode		    */
	unsigned	FTP_out_chklck	: 1;	/* Outbound check lockfile  */
	unsigned	FTP_out_waitclr	: 1;	/* Outbound wait for clear  */
	unsigned	FTP_out_mklck	: 1;	/* Outbound create lock	    */
	unsigned	FTP_in_chklck	: 1;	/* Inbound check lockfile   */
	unsigned	FTP_in_waitclr	: 1;	/* Inbound wait for clear   */
	unsigned	FTP_in_mklck	: 1;	/* Inbound create lock	    */

	char		OutBox[65];		/* Node's personal outbound */
	char		Nl_flags[65];		/* Override nodelist flags  */
	char		Nl_hostname[41];	/* Override hostname	    */

						/* Contact information	    */
	char		Ct_phone[17];		/* Node's private phone	    */
	char		Ct_fax[17];		/* Node's fax		    */
	char		Ct_cellphone[21];	/* Node's cellphone	    */
	char		Ct_email[31];		/* Node's email		    */
	char		Ct_remark[65];		/* Remark		    */

	securityrec	Security;		/* Security flags	    */
};



/*
 * Groups for file areas. (fgroups.data)
 */
struct	_fgrouphdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
	time_t		lastupd;		/* Last statistics update  */
};

struct	_fgroup {
	char		Name[13];		/* Group Name		   */
	char		Comment[56];		/* Group Comment	   */
	unsigned	Active		: 1;	/* Group Active		   */
	unsigned	Deleted		: 1;	/* Is group deleted	   */
	unsigned	DivideCost	: 1;	/* Divide cost over links  */
	fidoaddr	UseAka;			/* Aka to use		   */
	fidoaddr	UpLink;			/* Uplink address	   */
	long		UnitCost;		/* Cost per unit	   */
	long		UnitSize;		/* Size per unit	   */
	long		AddProm;		/* Promillage to add	   */
	time_t		StartDate;		/* Start Date		   */
	time_t		LastDate;		/* Last active date	   */
	char		AreaFile[13];		/* Areas filename	   */
	statcnt		Files;			/* Files processed	   */
	statcnt		KBytes;			/* KBytes msgs or files	   */
						/* Auto add area options   */
	long		StartArea;		/* Lowest filearea nr.	   */
	char		Banner[15];		/* Banner to add	   */
	char		Convert[6];		/* Archiver to convert	   */
	unsigned	FileGate	: 1;	/* List is in filegate fmt */
	unsigned	AutoChange	: 1;	/* Auto add/del areas      */
	unsigned	UserChange	: 1;	/* User add/del areas      */
	unsigned	Replace		: 1;	/* Allow replace	   */
	unsigned	DupCheck	: 1;	/* Dupe Check		   */
	unsigned	Secure		: 1;	/* Check for secure system */
	unsigned	Touch		: 1;	/* Touch filedates	   */
	unsigned	VirScan		: 1;	/* Run Virus scanners	   */
	unsigned	Announce	: 1;	/* Announce files	   */
	unsigned	UpdMagic	: 1;	/* Update Magic database   */
	unsigned	FileId		: 1;	/* Check FILE_ID.DIZ	   */
	unsigned	ConvertAll	: 1;	/* Convert always	   */
	unsigned	SendOrg		: 1;	/* Send original file	   */
	unsigned	xRes6		: 1;
	unsigned	xRes7		: 1;
	unsigned	xRes8		: 1;
	char		BasePath[65];		/* File area base path     */
	securityrec	DLSec;			/* Download Security	   */
	securityrec	UPSec;			/* Upload Security	   */
	securityrec	LTSec;			/* List Security	   */
	char		BbsGroup[13];		/* BBS Group		   */
	char		AnnGroup[13];		/* BBS Announce Group	   */
	unsigned	Upload;			/* Upload area		   */
	securityrec	LinkSec;		/* Default link security   */
};



/*
 * Groups for message areas. (mgroups.data)
 */
struct	_mgrouphdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
	time_t		lastupd;		/* Last statistics update  */
};

struct	_mgroup {
	char		Name[13];		/* Group Name		   */
	char		Comment[56];		/* Group Comment	   */
	unsigned	Active		: 1;	/* Group Active		   */
	unsigned	Deleted		: 1;	/* Group is deleted	   */
	fidoaddr	UseAka;			/* Aka to use		   */
	fidoaddr	UpLink;			/* Uplink address	   */
	long		xOld[6];
	time_t		StartDate;		/* Start Date		   */
	time_t		LastDate;		/* Last active date	   */
	char		AreaFile[13];		/* Areas filename	   */
	statcnt		MsgsRcvd;		/* Received messages	   */
	statcnt		MsgsSent;		/* Sent messages	   */
						/* Auto create options	   */
	char		BasePath[65];		/* Base path to JAM areas  */
	securityrec	RDSec;			/* Read security	   */
	securityrec	WRSec;			/* Write security	   */
	securityrec	SYSec;			/* Sysop secirity	   */
	unsigned	NetReply;		/* Netmail reply area	   */
	unsigned	UsrDelete	: 1;	/* Allow users to delete   */
	unsigned	Aliases		: 1;	/* Allow aliases	   */
	unsigned	Quotes		: 1;	/* Add random quotes	   */
	unsigned	AutoChange	: 1;	/* Auto add/del from list  */
	unsigned	UserChange	: 1;	/* User add/del from list  */
	unsigned	xRes6		: 1;
	unsigned	xRes7		: 1;
	unsigned	xRes8		: 1;
	unsigned	StartArea;		/* Start at area number    */
	securityrec	LinkSec;		/* Default link security   */
};



/*
 *  Groups for newfiles announce. (ngroups.data)
 */
struct	_ngrouphdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
};

struct	_ngroup {
	char		Name[13];		/* Group Name		   */
	char		Comment[56];		/* Group Comment	   */
	unsigned	Active		: 1;	/* Group Active		   */
	unsigned	Deleted		: 1;	/* Group is deleted	   */
};



/*
 * Hatch manager (hatch.data)
 */
struct	_hatchhdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
	time_t		lastupd;		/* Last stats update	   */
};

struct	_hatch {
	char		Spec[79];		/* File spec to hatch	   */
	char		Name[21];		/* File Echo name	   */
	char		Replace[15];		/* File to replace	   */
	char		Magic[15];		/* Magic to update	   */
	char		Desc[256];		/* Description for file	   */
	unsigned	DupeCheck	: 1;	/* Check for dupes	   */
	unsigned	Active		: 1;	/* Record active	   */
	unsigned	Deleted		: 1;	/* Record is deleted	   */
	unsigned short	Days[7];		/* Days in the week	   */
	unsigned short	Month[32];		/* Days in the month	   */
	statcnt		Hatched;		/* Hatched statistics	   */
};



/*
 * Magic manager (magic.data)
 */
typedef enum {
	MG_EXEC,				/* Execute command	   */
	MG_COPY,				/* Copy file		   */
	MG_UNPACK,				/* Unpack file	 	   */
	MG_KEEPNUM,				/* Keep nr of files	   */
	MG_MOVE,				/* Move to other area	   */
	MG_UPDALIAS,				/* Update alias		   */
	MG_ADOPT,				/* Adopt file		   */
	MG_DELETE				/* Delete file		   */
} MAGICTYPE;

struct	_magichdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
};

struct	_magic {
	char		Mask[15];		/* Filemask for magic	   */
	unsigned int	Attrib;			/* Record type		   */
	unsigned	Active		: 1;	/* Record active	   */
	unsigned	Compile		: 1;	/* Compile Flag 	   */
	unsigned	Deleted		: 1;	/* Deleted record	   */
	char		From[21];		/* From area		   */
	char		Path[65];		/* Destination path	   */
	char		Cmd[65];		/* Command to execute	   */
	int		KeepNum;		/* Keep number of files	   */
	char		ToArea[21];		/* Destination area	   */
};



/*
 * Billing database
 */
struct	_bill {
	fidoaddr	Node;			/* Fido address		   */
	char		FileName[15];		/* File Name		   */
	char		FileEcho[21];		/* File Echo		   */
	char		Group[13];		/* Group		   */
	off_t		Size;			/* File Size		   */
	long		Cost;			/* File Cost		   */
};



/*
 * Newfile reports (newfiles.data)
 */
struct	_newfileshdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
	long		grpsize;		/* Size of groups	   */
};

struct	_newfiles {
	char		Comment[56];		/* Comment		   */
	char		Area[51];		/* Message area		   */
	char		Origin[51];		/* Origin line, or random  */
	char		From[36];		/* From field		   */
	char		Too[36];		/* To field		   */
	char		Subject[61];		/* Subject field	   */
	int		Language;		/* Language		   */
	char		Template[15];		/* Template filename	   */
	fidoaddr	UseAka;			/* Aka to use		   */
	unsigned	Active		: 1;	/* Active		   */
	unsigned	HiAscii		: 1;	/* Hi-Ascii allowed	   */
	unsigned	Deleted		: 1;	/* Report is deleted	   */
};



/*
 * Scanmanager (scanmgr.data)
 */
struct	_scanmgrhdr {
	long		hdrsize;		/* Size of header	   */
	long		recsize;		/* Size of records	   */
};

struct	_scanmgr {
	char		Comment[56];		/* Comment		   */
	char		Origin[51];		/* Origin line		   */
	fidoaddr	Aka;			/* Fido address		   */
	char		ScanBoard[51];		/* Board to scan	   */
	char		ReplBoard[51];		/* Reply board		   */
	int		Language;		/* Language to use	   */
	char		template[15];		/* Template filename	   */
	unsigned	Active		: 1;	/* Record active	   */
	unsigned	NetReply	: 1;	/* Netmail reply	   */
	unsigned	Deleted		: 1;	/* Area is deleted	   */
	unsigned	HiAscii		: 1;	/* High Ascii allowed	   */
};



/*
 * Record structure for file handling
 */
struct	_filerecord {
	char		Echo[21];		/* File echo		   */
	char		Comment[56];		/* Comment		   */
	char		Group[13];		/* Group		   */
	char		Name[13];		/* File Name		   */
	char		LName[81];		/* Long FileName	   */
	off_t		Size;			/* File Size		   */
	unsigned long	SizeKb;			/* File Size in Kb	   */
	time_t		Fdate;			/* File Date		   */
	char		Origin[24];		/* Origin system	   */
	char		From[24];		/* From system		   */
	char		Crc[9];			/* CRC 32		   */
	char		Replace[81];		/* Replace file		   */
	char		Magic[21];		/* Magic name		   */
	char		Desc[256];		/* Short description	   */
	char		LDesc[25][49];		/* Long description	   */
	int		TotLdesc;		/* Total long desc lines   */
	long		Cost;			/* File cost		   */
	unsigned	Announce	: 1;	/* Announce this file	   */
};



/*
 *  Mailer history file (mailhist.data)
 *  The first record conatains only the date (online) from which date this
 *  file is valid. The offline date is the date this file is created or
 *  packed. From the second record and on the records are valid data records.
 */
struct _history {
	fidoaddr	aka;			/* Node number		   */
	char		system_name[36];	/* System name		   */
	char		sysop[36];		/* Sysop name		   */
	char		location[36];		/* System location	   */
	char		tty[7];			/* Tty of connection	   */
	time_t		online;			/* Starttime of session	   */
	time_t		offline;		/* Endtime of session	   */
	unsigned long	sent_bytes;		/* Bytes sent		   */
	unsigned long	rcvd_bytes;		/* Bytes received	   */
	int		cost;			/* Session cost		   */
	unsigned	inbound		: 1;	/* Inbound session	   */
};



/*
 * Routing file, will override standard routing.
 * The mask is some kind of free formatted string like:
 *   1:All
 *   2:2801/16
 *   2:2801/All
 * If sname is used, the message To name is also tested for, this way
 * extra things can be done for netmail to a specific person.
 */
struct _routehdr {
	long            hdrsize;                /* Size of header	    */
	long            recsize;                /* Size of records	    */
};


struct _route {
	char		mask[25];		/* Mask to check	    */
	char		sname[37];		/* Opt. name to test	    */
	int		routetype;		/* What to do with it	    */
	fidoaddr	dest;			/* Destination address	    */
	char		dname[37];		/* Destination name	    */
	unsigned	Active	    : 1;	/* Is record active	    */
	unsigned	Deleted	    : 1;	/* Is record deleted	    */
};


#endif

