/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Main user login procedure.  Checks for limits, 
 *                          new ratio's cats all the welcome screens, and 
 *                          does a lot of checking in general.
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
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "timeout.h"
#include "user.h"
#include "pwcheck.h"
#include "funcs.h"
#include "funcs4.h"
#include "misc.h"
#include "bye.h"
#include "file.h"
#include "mail.h"
#include "change.h"
#include "menu.h"
#include "exitinfo.h"
#include "language.h"
#include "offline.h"
#include "statetbl.h"
#include "email.h"
#include "newuser.h"


extern int	sock;
extern pid_t	mypid;
char		*StartTime;


/* Non global function prototypes */

char *AskLogin(void);
static int rxemsi(void);


char	*Passwd = NULL;
int	iMaxLogin = 1;
char	*ieUserName = NULL;
char	*ieHandle = NULL;
char	*ieLocation = NULL;
char	*ieVoicePhone = NULL;
char	*ieDataPhone = NULL;
long	iePassword = 0;
char	*ieBirthDate = NULL;
char	*ieTerminal = NULL;
int	ieRows = 0;
int	ieCols = 0;
int	ieNulls = 0;
int	ieDZA = FALSE;
int	ieZAP = FALSE;
int	ieZMO = FALSE;
int	ieSLK = FALSE;
int	ieKER = FALSE;
int	ieCHT = FALSE;
int	ieMNU = FALSE;
int	ieTAB = FALSE;
int	ieASCII8 = FALSE;
int	ieNEWS = FALSE;
int	ieMAIL = FALSE;
int	ieFILE = FALSE;
int	ieHOT  = FALSE;
int	ieCLR  = FALSE;
int	ieHUSH = FALSE;
int	ieMORE = FALSE;
int	ieFSED = FALSE;
int	ieXPRS = FALSE;
char	*ieSoftware = NULL;
int	ieLogin = FALSE;



/*
 * Ask for BBS Login name (Firstname Lastname) and try to
 * establish an IEMSI session if the client supports it.
 */
char *AskLogin(void);
char *AskLogin(void)
{
	char	temp[36];
	int	Count = 0, rc = 0;
	static	char GetName[36];

	do {
		/*
		 * if after the first time IEMSI failed rc will not be
		 * zero anymore so we don't check for IEMSI logon anymore.
		 */
		if (rc == 0) {
			rc = rxemsi();
			if (rc)
				Syslog('+', "rxemsi rc=%d", rc);
		}
		if (rc) {
			/*
			 * IEMSI Aborted or other errors, so prompt user.
			 * Note that if the user didn't had an IEMSI client,
			 * rc is still 0, the name is entered in the rxemsi
			 * function.
			 */
			/* Please enter your First and Last name: */
			language(7, 0, 0);
			fflush(stdout);
			alarm_on();
			Getname(GetName, 35);
		} else {
			sprintf(GetName, "%s", ieUserName);
		}

		if ((strcmp(GetName,"")) == 0) {
			Count++;

			if (Count >= CFG.iCRLoginCount) {
				Enter(1);
				/* Disconnecting user ... */
				language(CFG.HiliteF, CFG.HiliteB, 2);
				Enter(2);
				Syslog('!', "Exceeded maximum login attempts");
				free(Passwd);
				Quick_Bye(0);
			}
		}
	} while (strcmp(GetName, "") == 0);

	strcpy(temp, GetName);

	/*
	 * Secret escape name
	 */
	if ((strcasecmp(temp, "off")) == 0) {
		Syslog('+', "Quick \"off\" logout");
		free(Passwd);
		Quick_Bye(0);
	}

	/*
	 * Check for singlename
	 */
	if ((strchr(GetName,' ') == NULL && !CFG.iOneName)) {
		Syslog('+', "Did not enter a full name");

		Count = -1;
		while (TRUE) {
			do {
				Count++;
				if (Count >= CFG.iCRLoginCount) {
					Enter(1);
					/* Disconnecting user ... */
					language(CFG.HiliteF, CFG.HiliteB, 2);
					Enter(2);
					Syslog('!', "Exceeded maximum login attempts");
					free(Passwd);
					Quick_Bye(0);
				}

				/* Please enter your Last name: */
				language(LIGHTGRAY, BLACK, 1);
				fflush(stdout);
				alarm_on();
				Getname(temp, 34 - strlen(GetName));
			} while ((strcmp(temp, "")) == 0);

			if((strcmp(temp,"")) != 0) {
				strcat(GetName," ");
				strcat(GetName,temp);
				break;
			}
		}
	}

	alarm_off();
	return GetName;
}



/*
 *  The next function is "borrowed" from Eugene M. Crossers ifcico.
 */
char *sel_brace(char *);
char *sel_brace(char *s)
{
	static char	*save;
	char		*p, *q;
	int		i;

	if (s == NULL)
		s = save;
	for (; *s && (*s != '{'); s++);
	if (*s == '\0') {
		save = s;
		return NULL;
	} else
		s++;

	for (p = s, q = s; *p; p++)
		switch(*p) {
			case '}':	if (*(p+1) == '}')
						*q++ = *p++;
					else {
						*q = '\0';
						save = p+1;
						goto exit;
					}
					break;

			case '\\':	if (*(p+1) == '\\')
						*q++ = *p++;
					else {
						sscanf(p+1, "%02x", &i);
						*q++ = i;
						p+=2;
					}
					break;

			default:	*q++ = *p;
					break;
		}
exit:
	return s;
}



int scanemsiici(char *);
int scanemsiici(char *buf)
{
	char	*p, *q;

	ieUserName = xstrcpy(sel_brace(buf));
	Syslog('i', "Username    : %s", MBSE_SS(ieUserName));
	ieHandle = xstrcpy(sel_brace(NULL));
	Syslog('i', "Handle      : %s", MBSE_SS(ieHandle));
	ieLocation = xstrcpy(sel_brace(NULL));
	Syslog('i', "Location    : %s", MBSE_SS(ieLocation));
	ieVoicePhone = xstrcpy(sel_brace(NULL));
	Syslog('i', "Voice Phone : %s", MBSE_SS(ieVoicePhone));
	ieDataPhone = xstrcpy(sel_brace(NULL));
	Syslog('i', "Data Phone  : %s", MBSE_SS(ieDataPhone));
	p = sel_brace(NULL);
	memset(Passwd, 0, 16);
	if (strlen(p) < 16)
		sprintf(Passwd, "%s", p);
	iePassword = StringCRC32(tu(p));
	Syslog('i', "Password    : %s (%ld)", p, iePassword);
	ieBirthDate = xstrcpy(sel_brace(NULL));
	Syslog('i', "Birthdate   : %s", MBSE_SS(ieBirthDate));
	p = sel_brace(NULL);
	ieTerminal = strtok(p, ",");
	ieRows = atoi(strtok(NULL, ","));
	ieCols = atoi(strtok(NULL, ","));
	ieNulls = atoi(strtok(NULL, ","));
	Syslog('i', "Terminal    : %s (%d x %d) [%d]", ieTerminal, ieRows, ieCols, ieNulls);

	p = sel_brace(NULL);
	Syslog('i', "Protocols   : %s", MBSE_SS(p));
	for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
		if (strcasecmp(q, "DZA") == 0)
			ieDZA = TRUE;
		else if (strcasecmp(q, "ZAP") == 0)
			ieZAP = TRUE;
		else if (strcasecmp(q, "ZMO") == 0)
			ieZMO = TRUE;
		else if (strcasecmp(q, "SLK") == 0)
			ieSLK = TRUE;
		else if (strcasecmp(q, "KER") == 0)
			ieKER = TRUE;
		else 
			Syslog('+', "Unrecognized IEMSI Protocol \"%s\"", q);
	}

	p = sel_brace(NULL);
	Syslog('i', "Capabilities: %s", MBSE_SS(p));
	for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
		if (strcasecmp(q, "CHT") == 0)
			ieCHT = TRUE;
		else if (strcasecmp(q, "TAB") == 0)
			ieTAB = TRUE;
		else if (strcasecmp(q, "MNU") == 0)
			ieMNU = TRUE;
		else if (strcasecmp(q, "ASCII8") == 0)
			ieASCII8 = TRUE;
		else
			Syslog('+', "Unrecognized IEMSI Capability \"%s\"", q);
	}

	p = sel_brace(NULL);
	Syslog('i', "Requests    : %s", MBSE_SS(p));
	for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
		if (strcasecmp(q, "NEWS") == 0)
			ieNEWS = TRUE;
		else if (strcasecmp(q, "MAIL") == 0)
			ieMAIL = TRUE;
		else if (strcasecmp(q, "FILE") == 0)
			ieFILE = TRUE;
		else if (strcasecmp(q, "HOT") == 0)
			ieHOT = TRUE;
		else if (strcasecmp(q, "CLR") == 0)
			ieCLR = TRUE;
		else if (strcasecmp(q, "HUSH") == 0)
			ieHUSH = TRUE;
		else if (strcasecmp(q, "MORE") == 0)
			ieMORE = TRUE;
		else if (strcasecmp(q, "FSED") == 0)
			ieFSED = TRUE;
		else if (strcasecmp(q, "XPRS") == 0)
			ieXPRS = TRUE;
		else
			Syslog('+', "Unrecognized IEMSI Request \"%s\"", q);
	}

	ieSoftware = xstrcpy(sel_brace(NULL));
	Syslog('i', "Software    : %s", MBSE_SS(ieSoftware));

	return 0;
}



char *mkiemsiisi(void);
char *mkiemsiisi(void)
{
	char	*p, cbuf[16];
	time_t	tt;

	p = xstrcpy((char *)"EMSI_ISI0000{MBSE BBS,");
	p = xstrcat(p, (char *)VERSION);
	p = xstrcat(p, (char *)",Linux}{");
	p = xstrcat(p, CFG.bbs_name);
	p = xstrcat(p, (char *)"}{");
	p = xstrcat(p, CFG.location);
	p = xstrcat(p, (char *)"}{");
	p = xstrcat(p, CFG.sysop_name);
	p = xstrcat(p, (char *)"}{");
	(void)time(&tt);
	sprintf(cbuf, "%08lX", mktime(localtime(&tt)));
	p = xstrcat(p, cbuf);
	p = xstrcat(p, (char *)"}{");
	p = xstrcat(p, CFG.comment);
	p = xstrcat(p, (char *)"}{0}{}");
	sprintf(cbuf, "%04X", (unsigned int)strlen(p+12));
	memcpy(p+8, cbuf, 4);

	Syslog('i', "Prepared \"%s\"", p);
	return p;
}



SM_DECL(rxemsi, (char *)"rxemsi")
SM_STATES
	prompt,
	getpkt,	
	chkpkt,
	chkici,
	sendnak,
	sendisi,
	human
SM_NAMES
	(char *)"prompt",
	(char *)"getpkt",
	(char *)"chkpkt",
	(char *)"chkici",
	(char *)"sendnak",
	(char *)"sendisi",
	(char *)"human"
SM_EDECL
	int		len, iemsi, newpos, tries = 0;
	char		*p, *buf = NULL;
	unsigned char	c = 0;
	unsigned short	lcrc, rcrc;
	unsigned long	llcrc, lrcrc;

	buf = calloc(2048, sizeof(char));

SM_START(prompt)

SM_STATE(prompt)
	Syslog('I', "SM: prompt");

	/*
	 * Issue EMSI_IRQ and overwrite it with the prompt.
	 */
	printf("**EMSI_IRQ8E08\r              \r");
	/* Please enter your First and Last name: */
	language(7, 0, 0);
	fflush(stdout);
	tries = 0;
	SM_PROCEED(getpkt);

SM_STATE(getpkt)
	Syslog('I', "SM: getpkt");

	fflush(stdin);
	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		WriteError("$Can't open /dev/tty");
		SM_ERROR;
	}
	Setraw();
	len = 0;
	iemsi = FALSE;
	newpos = 0;
	buf[0] = '\0';
	alarm_on();

	while(TRUE) {
		c = Readkey();

		Syslog('I', "c=%s len=%d iemsi=%d", printablec(c), len, iemsi);

		if ((len == 0) && (c == '*')) {
			iemsi = TRUE;
		} else {
			if (((c == 8) || (c == KEY_DEL) || (c == 127)) && (len > 0)) {
				printf("\b \b");
				fflush(stdout);
				buf[--len] = '\0';
			}

			if (c > 31 && c < 127) {
				if ((len == 0) && (CFG.iCapUserName))
					c = toupper(c);
	
				if (c == 32) {
					newpos = len;
					newpos++;
				}

				if (!iemsi) {
					if (newpos == len && CFG.iCapUserName)
						c = toupper(c);
					else {
						if (CFG.iCapUserName)
							c = tolower(c);
					}
					printf("%c", c);
					fflush(stdout);
				}
				buf[len] = c;
				len++;
			}
		}

		if ((c == '\r') || (!iemsi && (len == 35)) || (iemsi && (len == 2047))) {
			buf[len] = '\0';
			break;
		}
	}

	Unsetraw();
	close(ttyfd);
	Syslog('I', "Buf \"%s\"", printable(buf,0));

	if (strncasecmp(buf, "EMSI_", 5)) {
		SM_PROCEED(human);
	} else {
		SM_PROCEED(chkpkt);
	}

SM_STATE(chkpkt)
	Syslog('I', "SM: chkpkt");
	if (strncasecmp(buf, "EMSI_ICI", 8) == 0) {
		SM_PROCEED(chkici)
	}
	lcrc = crc16xmodem(buf, 8);
	sscanf(buf+8, "%04hx", &rcrc);
	if (lcrc != rcrc) {
		Syslog('+', "Got IEMSI packet \"%s\" with bad crc: %04x/%04x", printable(buf, 0), lcrc, rcrc);
		SM_PROCEED(sendnak);
	}
	if (strncasecmp(buf, "EMSI_HBT", 8) == 0) {
		tries = 0;
		SM_PROCEED(getpkt);
	} else if (strncasecmp(buf, "EMSI_IIR61E2", 12) == 0) {
		Syslog('+', "IEMSI Interrupt Request received");
		SM_ERROR;
	} else if (strncasecmp(buf, "EMSI_ACKA490", 12) == 0) {
		Syslog('+', "Established IEMSI session");
		ieLogin = TRUE;
		/*
		 * Clearup the part of the users screen where some IEMSI
		 * codes may be hanging around
		 */
		fflush(stdin);
		printf("\r                                              \r");
		fflush(stdout);
		SM_SUCCESS;
	} else if (strncasecmp(buf, "EMSI_NAKEEC3", 12) == 0) {
		Syslog('+', "IEMSI NAK received");
		SM_PROCEED(getpkt);
	} else {
		Syslog('I', "rxemsi ignores packet \"%s\"", buf);
		SM_PROCEED(getpkt);
	}

SM_STATE(chkici)
	Syslog('I', "SM: chkici");
	sscanf(buf+8, "%04x", &len);
	if (len != (strlen(buf) - 20)) {
		Syslog('+', "Bad EMSI_ICI length: %d/%d", len, strlen(buf)-20);
		SM_PROCEED(sendnak);
	}
	sscanf(buf+strlen(buf)-8, "%08lx", &lrcrc);
	*(buf+strlen(buf)-8) = '\0';
	llcrc = crc32ccitt(buf, strlen(buf));
	if (llcrc != lrcrc) {
		Syslog('+', "Got EMSI_ICI packet \"%s\" with bad crc: %08x/%08x", printable(buf, 0), llcrc, lrcrc);
		SM_PROCEED(sendnak);
	}
	if (scanemsiici(buf+12) == 0) {
		SM_PROCEED(sendisi);
	} else {
		WriteError("Could not parse EMSI_ICI packet \"%s\"", buf);
		SM_ERROR;
	}

SM_STATE(sendnak)
	Syslog('I', "SM: sendnak");
	if (++tries > 9) {
		Syslog('+', "too many tries getting EMSI_ICI");
		SM_ERROR;
	}
	printf((char *)"**EMSI_IRQ8E08\r");
	if (tries > 1)
		printf((char *)"**EMSI_NAKEEC3\r");
	fflush(stdout);
	SM_PROCEED(getpkt);

SM_STATE(sendisi)
	Syslog('I', "SM: sendisi");
	p = mkiemsiisi();
	printf("**%s%08lX\r", p, crc32ccitt(p, strlen(p)));
	fflush(stdout);
	free(p);
	SM_PROCEED(getpkt);

SM_STATE(human)
	printf("\n");
	fflush(stdout);
	ieUserName = xstrcpy(buf);
	Syslog('+', "Human caller (%s)", MBSE_SS(ieUserName));
	SM_SUCCESS;

SM_END
	free(buf);

SM_RETURN



void user()
{
	FILE		*pUsrConfig, *pLimits;
	int		i, x, z;
	int		FoundName = FALSE, iFoundLimit = FALSE;
	register int 	recno;
	int		lrecno = 0;
	long		l1, l2;
	unsigned 	crc = 0;
	char		*token;
	char		temp[PATH_MAX];
	char		temp1[84];
	char		sGetName[84];
	char		*FileName;
	char		*handle;
	struct passwd	*pw;
	char		*sGetPassword;
	long		offset;
	time_t		LastLogin;
	struct stat 	st;
	char		UserName[36];
	int		IsNew = FALSE;


	recno=0;
	LoginPrompt = TRUE;

	/*
	 * If not in unix mode ask for login
	 */
	if (!iUnixMode) {
		strcpy(sGetName, AskLogin());
	} else {
		Syslog('+', "Unixmode login: %s", sUnixName);
		if ((pw = getpwnam(sUnixName)))
			strcpy(sGetName, pw->pw_gecos);

		/*
		 * If there are more fields in the passwd gecos field
		 * then only get the first field.
		 */
		if (strchr(sGetName, ',') != NULL)
			strcpy(sGetName, strtok(sGetName, ","));

		if (!(CheckName(sGetName))) {
			printf("Unknown username: %s\n", sGetName);
			/* FATAL ERROR: You are not in the BBS users file.*/
			printf("%s\n", (char *) Language(389));
			/* Please run 'newuser' to create an account */
			printf("%s\n", (char *) Language(390));
			Syslog('?', "FATAL: Could not find user in BBS users file.");
			Syslog('?', "       and system is using unix accounts\n");
			free(Passwd);
			ExitClient(0);
		}
	}

	if (CFG.iCapUserName || SYSOP)
		strcpy(sGetName, tlcap(sGetName));

	/*
	 * Copy username, split first and lastname.
	 */
	strcpy(UserName, tlcap(sGetName));

	if ((strchr(sGetName,' ') == NULL && !CFG.iOneName)) {
		token = strtok(sGetName, " ");
  		strcpy(FirstName, token);
  		token = strtok(NULL, "\0");
		i = strlen(token);
		for(x = 2; x < i; x++) {
			if(token[x] == ' ')
				token[x] = '\0';
		}
	 	strcpy(LastName, token);
	} else
		strcpy(FirstName, sGetName);

	Syslog('+', "%s On-Line at %s", UserName, ttyinfo.comment);

	if ((strlen(sGetName)) < 2)
		user();

	/*
	 * Check some essential files, create them if they don't exist.
	 */
 	ChkFiles();


	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT")); 
	if ((pUsrConfig = fopen(temp,"r+b")) == NULL) {
		/*
		 * This should only happen once, when you build the BBS
		 */
		WriteError("Can't open users file: %s", temp);
		printf("Can't open userfile, run \"newuser\" first");
		free(Passwd);
		ExitClient(0);
	}

	handle = calloc(40, sizeof(char));
	fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
	strcpy(temp1, UserName);
	while (fread(&usrconfig, usrconfighdr.recsize, 1, pUsrConfig) == 1) {
		strcpy(temp, usrconfig.sUserName);
		strcpy(handle, usrconfig.sHandle);

		if ((strcasecmp(temp, temp1) == 0 || strcasecmp(handle, temp1) == 0)) {
			FoundName = TRUE;
			break;
		} else
			recno++;
	}
	free(handle);

	if (!FoundName) {
		Syslog('+', "Name not in user file");
		Enter(1);
		/* Scanning User File */
		language(LIGHTGRAY, BLACK, 3);
		Enter(1);

		usrconfig.GraphMode = FALSE;
		DisplayFile((char *)"notfound");

		Enter(1);
		/* Name entered: */
		language(LIGHTGRAY, BLACK, 5);
		printf("%s\n\n", UserName);
		/* Did you spell your name correctly [Y/n] */
		language(WHITE, BLACK, 4);
		fflush(stdout);
		fflush(stdin);
		i = toupper(Getone());
		if (i == Keystroke(4, 0) || i == '\r') {
			if (!CFG.elite_mode) {
				/*
				 * Here we run newuser.
				 */
				Syslog('+', "Creating user ...");
				recno = newuser(UserName);
				IsNew = TRUE;
			} else {
				if (!DisplayFile((char *)"private")) {
					/* If FALSE display hard coded message */
					Enter(1);
					/* This is a PRIVATE System, Type "off" to leave */
					language(LIGHTRED, BLACK, 6);
					Enter(2);
				}

				Syslog('!', "NewUser tried to login to \"Private System\"");
				free(Passwd);
				Quick_Bye(0);
			}

		} else {
			Enter(1);
			Syslog('+', "User spelt his/her name incorrectly");
			user();
		}
	}

	/*
	 * Setup users favourite language.
	 */
	Set_Language(usrconfig.iLanguage);
	Free_Language();
	InitLanguage();

	/*
	 * User logged in, tell it to the server.
	 */
	UserCity(mypid, usrconfig.sUserName, usrconfig.sLocation);

	/*
	 * See if this user is the Sysop.
	 */
	strcpy(temp, UserName);
	strcpy(temp1, CFG.sysop_name);
	if ((strcasecmp(temp1, temp)) == 0)
		SYSOP = TRUE; /* If login name is sysop, set SYSOP true */

	grecno = recno;

	offset = usrconfighdr.hdrsize + (recno * usrconfighdr.recsize);
	if (fseek(pUsrConfig, offset, 0) != 0) {
		printf("Can't move pointer there."); 
		getchar();
		free(Passwd);
		ExitClient(1);
	}

	fread(&usrconfig, usrconfighdr.recsize, 1, pUsrConfig);
	TermInit(usrconfig.GraphMode);
        sGetPassword = malloc(Max_passlen+1);

	/*
	 * If UnixMode is False, else let crc = iPassword to bypass the
	 * passwd
	 */
	if (!iUnixMode && !IsNew) {
		/*
		 * Check for a blank or expired password, they do exist
		 * after upgrading from RA 2.xx due to a bug in RA (or feature)
		 */
		if (usrconfig.iPassword == 0) {
			Syslog('!', "User has blank password, asking new");
			z = 0;
			while (TRUE) {
				Enter(1);
				/* Your password is expired, enter password: */
				language(LIGHTGRAY, BLACK, 435);
				fflush(stdout);
				alarm_on();
				Getpass(temp);
				if ((x = strlen(temp)) >= CFG.password_length) {
					Enter(1);
					/* Please enter password again: */
					language(LIGHTGRAY, BLACK, 40);
					fflush(stdout);
					alarm_on();
					Getpass(temp1);
					if ((i = strcmp(temp, temp1)) != 0) {
						Enter(2);
						/* Passwords do not match */
						language(LIGHTRED, BLACK, 41);
						Enter(1);
					} else {
						memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
						sprintf(usrconfig.Password, "%s", temp);
						memset(Passwd, 0, 16);
						sprintf(Passwd, "%s", temp);
						crc = StringCRC32(tu(temp));
						usrconfig.iPassword = crc;
						break;
					}
				} else {
					z++;
					if (z == CFG.iCRLoginCount) {
						Syslog('!', "User did not enter new password");
						Enter(1);
						language(CFG.HiliteF, CFG.HiliteB, 2);
						Enter(2);
						free(Passwd);
						Quick_Bye(0);
					}

					Enter(2);
					/* Your password must contain at least */
					language(LIGHTRED, BLACK, 42);
					printf("%d ", CFG.password_length);
					/* characters! Try again */
					language(LIGHTRED, BLACK, 43);
					Enter(1);
				}
			}
		} else {
			if ((ieLogin) && (iePassword == usrconfig.iPassword)) {
				crc = iePassword;
			} else {
				/* Pasword: */
				language(LIGHTGRAY, BLACK, 8);
				fflush(stdout);
				alarm_on();
				Getpass(sGetPassword);
				/*
				 * Password is CRC32 of uppercase string
				 */
				memset(Passwd, 0, 16);
				sprintf(Passwd, "%s", sGetPassword);
				memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
				sprintf(usrconfig.Password, "%s", sGetPassword);
				crc = StringCRC32(tu(sGetPassword));
			}
		}
	} else {
		crc = usrconfig.iPassword;
		sprintf(Passwd, "%s", usrconfig.Password);
	}

	IsDoing("Just Logged In");

	/*
	 * If password already OK, give pause prompt so the bbs logo
	 * will stay on the users screen.
	 */
	if ((iUnixMode) || ((ieLogin) && (iePassword == usrconfig.iPassword))) {
		alarm_on();
		Pause();
	}

	if (usrconfig.Archiver[0] == '\0') {
		usrconfig.Archiver[0] = 'Z';
		usrconfig.Archiver[1] = 'I';
		usrconfig.Archiver[2] = 'P';
		Syslog('+', "Setup default archiver ZIP");
	}

 	if (usrconfig.iPassword == crc) {
		recno = 0;
		free(sGetPassword);

		/*
		 * Check if user has an Unix account, if not create one
		 */
		if (usrconfig.Name[0] == '\0') {
			alarm_on();
			sprintf(usrconfig.Name, "%s", (char *)NameCreate(NameGen(UserName), UserName, Passwd));
			sprintf(sUnixName, "%s", usrconfig.Name);
			Pause();
		}

		/*
		 * Check users date format. We do it strict as we
		 * need this to be good for several other purposes.
		 * If it is correct, the users age is set in UserAge
		 */
		if (!Test_DOB(usrconfig.sDateOfBirth)) {
			Syslog('!', "Error in Date of Birth");
			Chg_DOB();
			strcpy(usrconfig.sDateOfBirth, exitinfo.sDateOfBirth);
		}

		/*
		 * Check to see if user must expire
		 */
		sprintf(temp,"%s", (char *) GetDateDMY());
		SwapDate(temp, usrconfig.sExpiryDate);

		/* Convert Date1 & Date2 to longs for compare */
		l1 = atol(Date1);
		l2 = atol(Date2);

		if(l1 >= l2 && l2 != 0) {
			/* 
			 * If Expiry Date is the same as today expire to 
			 * Expire Sec level
			 */
			usrconfig.Security = usrconfig.ExpirySec;
			Syslog('!', "User is expired, resetting level");
			/*
			 * Show texfile to user telling him about this.
			 */
			DisplayFile((char *)"expired");
		}

		free(Date1); 
		free(Date2);

		/* 
		 * Copy limits.data into memory
		 */
		FileName = calloc(84, sizeof(char));
		sprintf(FileName, "%s/etc/limits.data", getenv("MBSE_ROOT"));

		if(( pLimits = fopen(FileName,"rb")) == NULL) {
			perror("");
			WriteError("Can't open file: %s", FileName);
		} else {
			fread(&LIMIThdr, sizeof(LIMIThdr), 1, pLimits);

			while (fread(&LIMIT, sizeof(LIMIT), 1, pLimits) == 1) {
	 			if (LIMIT.Security == usrconfig.Security.level) {
					iFoundLimit = TRUE;
					break;
				} else
					lrecno++;
			}
			fclose(pLimits);
		}
		free(FileName);

		if(!iFoundLimit) {
			Syslog('?', "Unknown Security Level in limits.data");
			usrconfig.iTimeLeft = 0; /* Could not find limit, so set to Zero */
			usrconfig.iTimeUsed = 0; /* Set to Zero as well  */
		} else {
			/*
			 * Give user new time limit everyday
			 */
			sprintf(temp,"%s", (char *) GetDateDMY());

			if((strcmp(StrDateDMY(usrconfig.tLastLoginDate), temp)) != 0) {
				/*
				 *  If no timelimit set give user 24 hours.
				 */
				if (LIMIT.Time)
					usrconfig.iTimeLeft = LIMIT.Time;
				else
					usrconfig.iTimeLeft = 86400;
				usrconfig.iTimeUsed    = 0;          /* Set time used today to Zero       */
				usrconfig.iConnectTime = 0;	     /* Set connect time to Zero          */

				/*
				 * Give user new bytes and files every day if needed.
				 */
				if (LIMIT.DownK && LIMIT.DownF) {
					usrconfig.DownloadKToday = LIMIT.DownK;
					usrconfig.DownloadsToday = LIMIT.DownF;
				}
			}
		} /* End of else  */

		usrconfig.iConnectTime = 0;

		/* Copy Users Protocol into Memory */
		Set_Protocol(usrconfig.sProtocol);
		tlf(usrconfig.sProtocol);

		/* 
		 * Set last login Date and Time, copy previous session
		 * values in memory.
		 */
		sprintf(LastLoginDate, "%s", StrDateDMY(usrconfig.tLastLoginDate));
		sprintf(LastLoginTime, "%s", StrTimeHMS(usrconfig.tLastLoginDate));
		LastLogin = usrconfig.tLastLoginDate;
		usrconfig.tLastLoginDate = ltime; /* Set current login to current date */
		usrconfig.iTotalCalls = ++usrconfig.iTotalCalls;
		memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
		sprintf(usrconfig.Password, "%s", Passwd);

		/*
		 * If IEMSI login, update some settings
		 */
		if (ieLogin) {
			usrconfig.Cls          = ieCLR;
			usrconfig.HotKeys      = ieHOT;
			usrconfig.ieNEWS       = ieNEWS;
			usrconfig.MailScan     = ieMAIL;
			usrconfig.ieFILE       = ieFILE;
			usrconfig.Chat         = ieCHT;
			usrconfig.ieASCII8     = ieASCII8;
			usrconfig.DoNotDisturb = ieHUSH;
			usrconfig.FsMsged      = ieFSED;
		}

		/*
		 * Update user record.
		 */
		if(fseek(pUsrConfig, offset, 0) != 0)
			WriteError("Can't move pointer in file: %s", temp);
		else {
			fwrite(&usrconfig, sizeof(usrconfig), 1, pUsrConfig);
			fclose(pUsrConfig);
		}

		/*
		 * Write users structure to tmp file in ~/tmp
		 */
		InitExitinfo();
		GetLastUser();
		StartTime = xstrcpy(GetLocalHM());
		ChangeHomeDir(exitinfo.Name, exitinfo.Email);

		Syslog('+', "User successfully logged into BBS");
		Syslog('+', "Level %d (%s), %d mins. left, port %s", usrconfig.Security.level, LIMIT.Description, usrconfig.iTimeLeft, pTTY);
		time(&Time2Go);
		Time2Go += usrconfig.iTimeLeft * 60;
		iUserTimeLeft = usrconfig.iTimeLeft;

		DisplayFile((char *)"mainlogo");
		DisplayFile((char *)"welcome");

		/*
		 * The following files are only displayed if the user has
		 * turned the Bulletins on.
		 */
		if (exitinfo.ieNEWS) {
			DisplayFile((char *)"welcome1");
			DisplayFile((char *)"welcome2");
			DisplayFile((char *)"welcome3");
			DisplayFile((char *)"welcome4");
			DisplayFile((char *)"welcome5");
			DisplayFile((char *)"welcome6");
			DisplayFile((char *)"welcome7");
			DisplayFile((char *)"welcome8");
			DisplayFile((char *)"welcome9");

			sprintf(temp, "%s", (char *) GetDateDMY() );
			if ((strcmp(usrconfig.sDateOfBirth, temp)) == 0)
				DisplayFile((char *)"birthday");

			/*
			 * Displays file if it exists DD-MM.A??
			 */
			sprintf(temp, "%s", (char *) GetDateDMY());
			strcpy(temp1, "");
			strncat(temp1, temp, 5);
			sprintf(temp, "%s", temp1);
			DisplayFile(temp);
	
			/*
			 * Displays users security file if it exists
			 */
			sprintf(temp, "sec%d", usrconfig.Security.level);
			DisplayFile(temp);

			/*
			 * Display News file
			 */
			DisplayFile((char *)"news");
		}

		/*
		 * Display Onceonly file, first get the date of that
		 * file, search order is the same as in DisplayFile()
		 */
		st.st_mtime = 0;
		if (usrconfig.GraphMode) {
			sprintf(temp, "%s/onceonly.ans", lang.TextPath);
			stat(temp, &st);
			if (st.st_mtime == 0) {
				sprintf(temp, "%s/onceonly.ans", CFG.bbs_txtfiles);
				stat(temp, &st);
			}
		}
		if (st.st_mtime == 0) {
			sprintf(temp, "%s/onceonly.asc", lang.TextPath);
			stat(temp, &st);
			if (st.st_mtime == 0) {
				sprintf(temp, "%s/onceonly.asc", CFG.bbs_txtfiles);
				stat(temp, &st);
			}
		}

		if ((st.st_mtime != 0) && (LastLogin < st.st_mtime))
			DisplayFile((char *)"onceonly");
	
		LoginPrompt = FALSE;
		OLR_SyncTags();

		if (usrconfig.MailScan)
			CheckMail();

		/*
		 * We don't show new files to new users, their lastlogin
		 * date is not yet set so they would see all the files
		 * which can be boring...
		 */
		if (usrconfig.ieFILE && (!IsNew))
			NewfileScan(FALSE);

		/* 
		 * Copy last file Area in to current Area 
		 */
		SetFileArea(usrconfig.iLastFileArea);

		/*
		 * Copy Last Message Area in to Current Msg Area
		 */
		SetMsgArea(usrconfig.iLastMsgArea);
		SetEmailArea((char *)"mailbox");

		/*
		 * Set or Reset the DoNotDisturb flag, now is the time
		 * we may be interrupted.
		 */
		UserSilent(usrconfig.DoNotDisturb);

		/*
		 * Start the menu, but first, wipe the password.
		 */
		memset(Passwd, 0, sizeof(Passwd));
		free(Passwd);
        	menu();
	} else {
		Syslog('+',"Login attempt: %d, password: %s", iMaxLogin, sGetPassword);
		free(sGetPassword);
		iMaxLogin++;
		if(iMaxLogin == CFG.max_login + 1) {
			Enter(2);
			language(LIGHTGRAY, BLACK, 9);
			Enter(1);
			Syslog('!', "Exceeded maximum login attempts, user disconnected");
			ExitClient(1);
		}
		Enter(2);
		language(LIGHTGRAY, BLACK, 10);
		Enter(2);
		alarm_on();
		Pause();
		printf("\n");
		user();
	}
}


