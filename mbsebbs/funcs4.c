/*****************************************************************************
 *
 * File ..................: bbs/funcs4.c
 * Purpose ...............: Misc functions, also for some utils.
 * Last modification date : 18-Oct-2001
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
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "../lib/msg.h"
#include "funcs4.h"
#include "misc.h"
#include "timeout.h"
#include "language.h"


extern	pid_t mypid;		/* Original pid				   */



void UserSilent(int flag)
{
	SockS("ADIS:2,%d,%d;", mypid, flag);
}



/*
 * Check BBS open status, return FALSE if the bbs is closed.
 * Display the reason why to the user.
 */
int CheckStatus()
{
	static	char buf[81];

	sprintf(buf, "SBBS:0;");
	if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "100:2,0", 7) == 0)
			return TRUE;
		if ((strncmp(buf, "100:2,2", 7) == 0) && (!ttyinfo.honor_zmh))
			return TRUE;
		buf[strlen(buf) -1] = '\0';
		printf("\n\n\007*** %s ***\n\n\n", buf+8);
		fflush(stdout);
	}
	return FALSE;
}



/*
 * Get a character string with cursor position
 */
void GetstrP(char *sStr, int iMaxLen, int Position)
{
	unsigned char	ch = 0;
	int		iPos = Position;

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 1");
		return;
	}
	Setraw();

	alarm_on();

	while (ch != KEY_ENTER) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == KEY_BACKSPACE) || (ch == KEY_DEL) || (ch == KEY_RUBOUT)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos <= iMaxLen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a character string
 */
void GetstrC(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0;
	int		iPos = 0;

	fflush(stdout);
	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 6");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 *  get a string, don't allow spaces (for Unix accounts)
 */
void GetstrU(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0;
	int		iPos = 0;

	fflush(stdout);
	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 6");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (isalnum(ch)) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a phone number, only allow digits, + and - characters.
 */
void GetPhone(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	fflush(stdout);

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 5");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) { 
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if ((ch >= '0' && ch <= '9') || (ch == '-') || (ch == '+')) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else 
				putchar('\007');
		}
	}

	Unsetraw();                                           
	close(ttyfd);
	printf("\n");
}



/*
 * Get a number, allow digits, spaces, minus sign, points and comma's
 */
void Getnum(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	fflush(stdout);

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 5");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if ((ch >= '0' && ch <= '9') || (ch == '-') || (ch == ' ') \
		     || (ch == ',') || (ch == '.')) {

			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * This function gets the date from the user checking the length and
 * putting two minus signs in the right places
 */
void GetDate(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 4");
		return;
	}
	Setraw();

	alarm_on();
	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0)
				printf("\b \b");
			else
				putchar('\007');

			if (iPos == 3 || iPos == 6) {
				printf("\b \b"); 
				--iPos;
			}

			sStr[--iPos]='\0';
		}

		if (ch >= '0' && ch <= '9') {
			if (iPos < iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
				if (iPos == 2 || iPos == 5) {
					printf("-");
					sprintf(sStr, "%s-", sStr);
					iPos++;
				}
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a string, capitalize only if set in config.
 */
void Getname(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0, iNewPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 2");
		return;
	}
	Setraw();
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && (ch < 127)) {
			if (iPos < iMaxlen) {
				iPos++;
				if (iPos == 1 && CFG.iCapUserName)
					ch = toupper(ch);

				if (ch == 32) {
					iNewPos = iPos;
					iNewPos++;
				}

				if (iNewPos == iPos && CFG.iCapUserName)
					ch = toupper(ch);
				else
					if (CFG.iCapUserName)
						ch = tolower(ch);

				if (iPos == 1 && CFG.iCapUserName)
					ch = toupper(ch);

				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a Fidonet style username, always capitalize.
 */
void GetnameNE(char *sStr, int iMaxlen)         
{                                              
	unsigned char	ch = 0; 
	int		iPos = 0, iNewPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 2");
		return;
	}
	Setraw();
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else 
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos < iMaxlen) {
				iPos++;

				if (iPos == 1)
					ch = toupper(ch);

				if (ch == 32) {
					iNewPos = iPos;
					iNewPos++;
				}

				if (iNewPos == iPos)
					ch = toupper(ch);
				else
					ch = tolower(ch);

				if (iPos == 1)
					ch = toupper(ch);

				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Function will Scan Users Database for existing phone numbers. If
 * found, it will write a log entry to the logfile. The user WILL NOT
 * be notified about the same numbers
 */
int TelephoneScan(char *Number, char *Name)
{
	FILE	*fp;
	int	Status = FALSE;
	char	*temp;
	struct	userhdr	uhdr;
	struct	userrec	u;

	temp  = calloc(81, sizeof(char));
 
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if(( fp = fopen(temp,"rb")) != NULL) {
		fread(&uhdr, sizeof(uhdr), 1, fp);

 		while (fread(&u, uhdr.recsize, 1, fp) == 1) {
			if (strcasecmp(u.sUserName, Name) != 0)
	 			if ((strlen(u.sVoicePhone) && (strcmp(u.sVoicePhone, Number) == 0)) ||
				    (strlen(u.sDataPhone) &&  (strcmp(u.sDataPhone, Number) == 0))) {
					Status = TRUE;
					Syslog('b', "Dupe phones ref: \"%s\" voice: \"%s\" data: \"%s\"",
						Number, u.sVoicePhone, u.sDataPhone);
					Syslog('+', "Uses the same telephone number as %s", u.sUserName);
 				}
 		}
		fclose(fp);
	}

	free(temp);
	return Status;
}



void Pause()
{
	int	i, x;
	char	*string;

	string = malloc(81);

	/* Press (Enter) to continue: */
	sprintf(string, "\r%s", (char *) Language(375));
	colour(CFG.CRColourF, CFG.CRColourB);
	printf(string);
	
	do {
		fflush(stdout);
		fflush(stdin);
		alarm_on();
		i = Getone();
	} while ((i != '\r') && (i != '\n'));

	x = strlen(string);
	for(i = 0; i < x; i++)
		printf("\b");
	for(i = 0; i < x; i++)
		printf(" ");
	for(i = 0; i < x; i++)
		printf("\b");
	fflush(stdout);

	free(string);
}



/*
 * Function to check if UserName exists and returns a 0 or 1
 */
int CheckName(char *Name)
{
	FILE	*fp;
	int	Status = FALSE;
	char	*temp, *temp1;
	struct	userhdr	ushdr;
	struct	userrec	us;

	temp   = calloc(81, sizeof(char));
	temp1  = calloc(81, sizeof(char));

	strcpy(temp1, tl(Name));

	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp,"rb")) != NULL) {
		fread(&ushdr, sizeof(ushdr), 1, fp);

 		while (fread(&us, ushdr.recsize, 1, fp) == 1) {
			strcpy(temp, tl(us.sUserName));

			if((strcmp(temp, temp1)) == 0) {
				Status = TRUE;
				break;
			}
 		}
		fclose(fp);
	}

	free(temp);
	free(temp1);
	return Status;
}



/*
 * This function returns the date in the following format:
 * DD-Mon HH:MM:SS (Day-Month Time)
 * The users language is used.
 */
char *logdate()
{
	static char Logdate[15];

	time(&Time_Now);
	l_date = localtime(&Time_Now);
	sprintf(Logdate,"%02d-%s %02d:%02d:%02d", l_date->tm_mday, GetMonth(l_date->tm_mon+1),
			l_date->tm_hour, l_date->tm_min, l_date->tm_sec);
	return(Logdate);
}



/*
 * Function will ask user to create a unix login
 * Name cannot be longer than 8 characters
 */
char *NameGen(char *FidoName)
{
	char		*sUserName;
	struct passwd	*pw;

	sUserName = calloc(10, sizeof(char));

	Syslog('+', "NameGen(%s)", FidoName);
	setpwent();
	while ((strcmp(sUserName, "") == 0 || (pw = getpwnam(sUserName)) != NULL) || (strlen(sUserName) < 3)) {
		colour(12, 0);
		printf("\n%s\n\n", (char *) Language(381));
		colour(15, 0);
		/* Please enter a login name (Maximum 8 characters) */
		printf("\n%s\n", (char *) Language(383));
		/* ie. John Doe, login = jdoe */
		printf("%s\n", (char *) Language(384));
		colour(10, 0);
		/* login > */
		printf("%s", (char *) Language(385));
		fflush(stdout);
		fflush(stdin);
		GetstrU(sUserName, 7);

		setpwent();
		if (pw = getpwnam(tl(sUserName)), pw != NULL) {
			/* That login name already exists, please choose another one. */
			colour(12, 0);
			printf("\n%s\n", (char *) Language(386));
			setpwent();
		}
	}
	return tl(sUserName);
}



/*
 * Function will create the users name in the passwd file
 */
char *NameCreate(char *Name, char *Comment, char *Password)
{
	char	*PassEnt;

	PassEnt = calloc(256, sizeof(char));

	/*
	 * Call mbuseradd, this is a special setuid root program to create
	 * unix acounts and home directories.
	 */
	sprintf(PassEnt, "%s/bin/mbuseradd %d %s \"%s\" %s",
		getenv("MBSE_ROOT"), getgid(), Name, Comment, CFG.bbs_usersdir);
	Syslog('+', "%s", PassEnt);
	fflush(stdout);
	fflush(stdin);

	if (system(PassEnt) != 0) {
		WriteError("Failed to create unix account");
		free(PassEnt);
		ExitClient(1);
	} 
	sprintf(PassEnt, "%s/bin/mbpasswd -f %s %s", getenv("MBSE_ROOT"), Name, Password);
	Syslog('+', "%s/bin/mbpasswd -f %s ******", getenv("MBSE_ROOT"), Name);
	if (system(PassEnt) != 0) {
		WriteError("Failed to set unix password");
		free(PassEnt);
		ExitClient(1);
	}

	colour(14, 0);
	/* Your "Unix Account" is created, you may use it the next time you call */
	printf("\n%s\n", (char *) Language(382));
	Syslog('+', "Created Unix account %s for %s", Name, Comment);

	free(PassEnt);
	return Name;
}



/*
 * Function will check and create a home directory for the user if
 * needed. It will also change into the users home directory when
 * they login.
 */
char *ChangeHomeDir(char *Name, int Mailboxes)
{
	char	*temp;
	static	char temp1[PATH_MAX];

	temp  = calloc(PATH_MAX, sizeof(char));

	/*
	 * set umask bits to zero's then reset with mkdir
	 */
	umask(000);
	
	/*
	 * First check to see if users home directory exists
	 * else try create directory, as set in CFG.bbs_usersdir
	 */
	if ((access(CFG.bbs_usersdir, R_OK)) != 0) {
		WriteError("$FATAL: Access to %s failed", CFG.bbs_usersdir);
		free(temp);
		ExitClient(1);
	}

	sprintf(temp1, "%s/%s", CFG.bbs_usersdir, Name);

	/*
	 * Then check to see if users directory exists in the home dir
	 */
	if ((access(temp1, R_OK)) != 0) {
		WriteError("$FATAL: Users homedir %s doesn't exist", temp1);
		free(temp);
		ExitClient(1);
	}

	/*
	 * Change to users home directory
	 */
	if (chdir(temp1) != 0) {
		WriteError("$FATAL: Can't change to users home dir, aborting: %s", temp1);
		free(temp);
		ExitClient(1);
	}
	setenv("HOME", temp1, 1);

	/*
	 * Check subdirectories, create them if they don't exist.
	 */
	sprintf(temp, "%s/wrk", temp1);
	CheckDir(temp);
	sprintf(temp, "%s/tag", temp1);
	CheckDir(temp);
	sprintf(temp, "%s/upl", temp1);
	CheckDir(temp);
	sprintf(temp, "%s/tmp", temp1);
	CheckDir(temp);
	sprintf(temp, "%s/.dosemu", temp1);
	CheckDir(temp);
	sprintf(temp, "%s/.dosemu/run", temp1);
	CheckDir(temp);
	sprintf(temp, "%s/.dosemu/tmp", temp1);
	CheckDir(temp);
	umask(007);

	/*
	 * Check users private emailboxes
	 */
	if (Mailboxes) {
		sprintf(temp, "%s/mailbox", temp1);
		if (Msg_Open(temp))
			Msg_Close();
		sprintf(temp, "%s/archive", temp1);
		if (Msg_Open(temp))
			Msg_Close();
		sprintf(temp, "%s/trash", temp1);
		if (Msg_Open(temp))
			Msg_Close();
	}
	
	free(temp);
	return temp1;
}



void CheckDir(char *dir)
{
	if ((access(dir, R_OK) != 0)) {
		Syslog('+', "Creating %s", dir);
		if (mkdir(dir, 0770))
			WriteError("$Can't create %s", dir);
	}
}



/*
 * Function will check /etc/passwd for users fidonet login name.
 * This will allow users to login in with there full name instead of
 * their login name, to cut out confusion between unix accounts
 * and normal bbs logins.
 */
int Check4UnixLogin(char *UsersName)
{
	unsigned	UID = -1;  /* Set to -1 incase user is not found */
	struct passwd	*pw;

	while ((pw = getpwent())) {
		if(strcmp(pw->pw_gecos, UsersName) == 0) {
			UID = pw->pw_uid;
			break;
		}
	}

	return UID;
}



/*
 * Function to check if User Handle exists and returns a 0 or 1
 */
int CheckHandle(char *Name)
{
	FILE	*fp;
	int	Status = FALSE;
	char	*temp, *temp1;
	struct	userhdr	uhdr;
	struct	userrec	u;

	temp   = calloc(PATH_MAX, sizeof(char));
	temp1  = calloc(PATH_MAX, sizeof(char));

	strcpy(temp1, tl(Name));

	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if(( fp = fopen(temp,"rb")) != NULL) {
		fread(&uhdr, sizeof(uhdr), 1, fp);

 		while (fread(&u, uhdr.recsize, 1, fp) == 1) {
			strcpy(temp, tl(u.sHandle));

			if((strcmp(temp, temp1)) == 0) {
				Status = TRUE;
				break;
			}
 		}
		free(temp);
		free(temp1);
		fclose(fp);
	}

	return Status;
}



/*
 * Function will check for unwanted user names
 */
int BadNames(char *Username)
{
	FILE	*fp;
	short	iFoundName = FALSE;
	char	*temp, *String, *User;

	temp   = calloc(PATH_MAX, sizeof(char));
	String = calloc(81, sizeof(char));
	User   = calloc(81, sizeof(char));

	strcpy(User, tl(Username));

	sprintf(temp, "%s/etc/badnames.ctl", getenv("MBSE_ROOT"));
	if(( fp = fopen(temp, "r")) != NULL) {
		while((fgets(String, 80, fp)) != NULL) {
			strcpy(String, tl(String));
			Striplf(String);
			if((strstr(User, String)) != NULL) {
				printf("\nSorry that name is not acceptable on this system\n");
				iFoundName = TRUE;
				break;
			}
		}
		fclose(fp);
	}

	free(temp);
	free(String);
	free(User);
	return iFoundName;
}



/*
 * Function will find where MBSE is located on system and load
 * the file $MBSE_ROOT/etc/config.data in memory.
 */
void FindMBSE()
{
	FILE		*pDataFile;
	static char	p[81];
	char		*FileName;
	struct passwd	*pw;

        FileName = calloc(PATH_MAX, sizeof(char));

	/*
	 * Check if the environment is set, if not, then we create the
	 * environment from the passwd file.
	 */
	if (getenv("MBSE_ROOT") == NULL) {
		pw = getpwnam("mbse");
		memset(&p, 0, sizeof(p));
		sprintf(p, "MBSE_ROOT=%s", pw->pw_dir);
		putenv(p);
	}

	if (getenv("MBSE_ROOT") == NULL) {
		printf("FATAL ERROR: Environment variable MBSE_ROOT not set\n");
		free(FileName);
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(1);
	}
	sprintf(FileName, "%s/etc/config.data", getenv("MBSE_ROOT"));

	if(( pDataFile = fopen(FileName, "rb")) == NULL) {
		printf("FATAL ERROR: Can't open %s for reading!\n", FileName);
		printf("Please run mbsetup to create configuration file.\n");
		printf("Or check that your environment variable MBSE_ROOT is set to the BBS Path!\n");
		free(FileName);
#ifdef MEMWATCH
                mwTerm();
#endif
		exit(1);
	}

	fread(&CFG, sizeof(CFG), 1, pDataFile);
	free(FileName);
	fclose(pDataFile);
}



/* 
 * Returns Mmm in the users language.
 */
char *GetMonth(int Month)
{
	static char	month[10];

	switch (Month) {
		case 1:
			strcpy(month, *(mLanguage + 398));
			break;
		case 2:                    
			strcpy(month, *(mLanguage + 399));      
			break;                    
		case 3:                    
			strcpy(month, *(mLanguage + 400));      
			break;                    
		case 4:                    
			strcpy(month, *(mLanguage + 401));      
			break;                    
		case 5:                    
			strcpy(month, *(mLanguage + 402));      
			break;                    
		case 6:                    
			strcpy(month, *(mLanguage + 403));      
			break;                    
		case 7:                    
			strcpy(month, *(mLanguage + 404));      
			break;                    
		case 8:                    
			strcpy(month, *(mLanguage + 405));      
			break;                    
		case 9:                    
			strcpy(month, *(mLanguage + 406));      
			break;                    
		case 10:                    
			strcpy(month, *(mLanguage + 407));      
			break;                    
		case 11:                    
			strcpy(month, *(mLanguage + 408));      
			break;                    
		case 12:
			strcpy(month, *(mLanguage + 409));
			break;
		default:                        
			strcpy(month, "Unknown");      
	}                                              

	return(month);
}



/* Returns DD-Mmm-YYYY */
char *GLCdateyy()
{
	static	char	GLcdateyy[15];
	char	ntime[15];

	time(&Time_Now);
	l_date = localtime(&Time_Now);

	sprintf(GLcdateyy,"%02d-",
	  l_date->tm_mday);

	sprintf(ntime,"-%02d", l_date->tm_year+1900);
	strcat(GLcdateyy, GetMonth(l_date->tm_mon+1));
	strcat(GLcdateyy,ntime);

	return(GLcdateyy);
}


