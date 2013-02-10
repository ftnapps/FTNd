/*****************************************************************************
 *
 * funcs.c
 * Purpose ...............: Misc functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/ftnd.h"
#include "../lib/users.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "funcs.h"
#include "term.h"
#include "ttyio.h"


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
    static	char buf[81], msg[81];

    snprintf(buf, 81, "SBBS:0;");
    if (socket_send(buf) == 0) {
	strncpy(buf, socket_receive(), 80);
	if (strncmp(buf, "100:2,0", 7) == 0)
	    return TRUE;
	if ((strncmp(buf, "100:2,2", 7) == 0) && (!ttyinfo.honor_zmh))
	    return TRUE;
	buf[strlen(buf) -1] = '\0';
	Enter(2);
	PUTCHAR('\007');
	snprintf(msg, 81, "*** %s ***", cldecode(buf+8));
	PUTSTR(msg);
	Syslog('+', "Send user message \"%s\"", cldecode(buf+8));
	Enter(3);
    }
    return FALSE;
}



/*
 * Function to check if UserName/Handle  exists and returns a 0 or 1
 */
int CheckName(char *Name)
{
    FILE	    *fp;
    int		    Status = FALSE;
    char	    *temp;
    struct userhdr  ushdr;
    struct userrec  us;

    temp   = calloc(PATH_MAX, sizeof(char));

    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("FTND_ROOT"));
    if ((fp = fopen(temp,"rb")) != NULL) {
	fread(&ushdr, sizeof(ushdr), 1, fp);

 	while (fread(&us, ushdr.recsize, 1, fp) == 1) {
	    if ((strcasecmp(Name, us.sUserName) == 0) || (strcasecmp(Name, us.sHandle) == 0)) {
		Status = TRUE;
		break;
	    }
 	}
	fclose(fp);
    }

    free(temp);
    return Status;
}



/*
 * Check several Unix names and other build-in system names
 * that are forbidden to select for new users.
 */
int CheckUnixNames(char *name)
{
    struct passwd   *pw;
    char            *temp;
    FILE            *fp;
    int             rc = FALSE;

    /*
     * Basic checks
     */
    if (name == NULL)
        rc = TRUE;
    else if (strlen(name) == 0)
        rc = TRUE;
    else if (strlen(name) > 8)
        rc = TRUE;

    /*
     * Check Unix names in the password file
     */
    if (! rc) {
        if ((pw = getpwnam(name)) != NULL)
            rc = TRUE;
        endpwent();
    }

    /*
     * Username ping is used by the PING function
     */
    if (! rc) {
        if (strcasecmp(name, (char *)"ping") == 0)
            rc = TRUE;
    }

    /*
     * Check service names
     */
    if (! rc) {
        temp = calloc(PATH_MAX, sizeof(char));
        snprintf(temp, PATH_MAX, "%s/etc/service.data", getenv("FTND_ROOT"));
        if ((fp = fopen(temp, "r")) != NULL) {
            fread(&servhdr, sizeof(servhdr), 1, fp);

            while (fread(&servrec, servhdr.recsize, 1, fp) == 1) {
                if ((strcasecmp(servrec.Service, name) == 0) && servrec.Active) {
                    rc = TRUE;
                    break;
                }
            }
            fclose(fp);
        }
        free(temp);
    }

    return rc;
}



/*
 * Function will check and create a home directory for the user if
 * needed. It will also change into the users home directory when
 * they login.
 */
char *ChangeHomeDir(char *Name, int Mailboxes)
{
    char	*temp;
    static char	temp1[PATH_MAX];
    FILE	*fp;

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
	ExitClient(FTNERR_INIT_ERROR);
    }

    snprintf(temp1, PATH_MAX, "%s/%s", CFG.bbs_usersdir, Name);

    /*
     * Then check to see if users directory exists in the home dir
     */
    if ((access(temp1, R_OK)) != 0) {
	WriteError("$FATAL: Users homedir %s doesn't exist", temp1);
	free(temp);
	ExitClient(FTNERR_INIT_ERROR);
    }

    /*
     * Change to users home directory
     */
    if (chdir(temp1) != 0) {
	WriteError("$FATAL: Can't change to users home dir, aborting: %s", temp1);
	free(temp);
	ExitClient(FTNERR_INIT_ERROR);
    }
    setenv("HOME", temp1, 1);

    /*
     * Check if user has a .signature file.
     * If not, create a simple one.
     */
    snprintf(temp, PATH_MAX, "%s/%s/.signature", CFG.bbs_usersdir, Name);
    if (access(temp, R_OK)) {
	Syslog('+', "Creating users .signature file");
        if ((fp = fopen(temp, "w")) == NULL) {
	    WriteError("$Can't create %s", temp);
	} else {
	    fprintf(fp,     "    Greetings, %s\n", exitinfo.sUserName);
	    if ((CFG.EmailMode == E_PRMISP) && exitinfo.Email && CFG.GiveEmail)
	        fprintf(fp, "    email: %s@%s\n", exitinfo.Name, CFG.sysdomain);
	    fclose(fp);
	}
    }

    /*
     * Check subdirectories, create them if they don't exist.
     */
    snprintf(temp, PATH_MAX, "%s/wrk", temp1);
    CheckDir(temp);
    snprintf(temp, PATH_MAX, "%s/tag", temp1);
    CheckDir(temp);
    snprintf(temp, PATH_MAX, "%s/upl", temp1);
    CheckDir(temp);
    snprintf(temp, PATH_MAX, "%s/tmp", temp1);
    CheckDir(temp);
    snprintf(temp, PATH_MAX, "%s/.dosemu", temp1);
    CheckDir(temp);
    snprintf(temp, PATH_MAX, "%s/.dosemu/run", temp1);
    CheckDir(temp);
    snprintf(temp, PATH_MAX, "%s/.dosemu/tmp", temp1);
    CheckDir(temp);
    umask(007);

    /*
     * Check users private emailboxes
     */
    if (Mailboxes) {
	snprintf(temp, PATH_MAX, "%s/mailbox", temp1);
	if (Msg_Open(temp))
	    Msg_Close();
	snprintf(temp, PATH_MAX, "%s/archive", temp1);
	if (Msg_Open(temp))
	    Msg_Close();
	snprintf(temp, PATH_MAX, "%s/trash", temp1);
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
 * Function will find where FTND is located on system and load
 * the file $FTND_ROOT/etc/config.data in memory.
 */
void FindFTND()
{
    FILE	    *pDataFile;
    static char	    p[81];
    char	    *FileName;
    struct passwd   *pw;

    FileName = calloc(PATH_MAX, sizeof(char));

    /*
     * Check if the environment is set, if not, then we create the
     * environment from the passwd file.
     */
    if (getenv("FTND_ROOT") == NULL) {
	pw = getpwnam("ftnd");
	memset(&p, 0, sizeof(p));
	snprintf(p, 81, "FTND_ROOT=%s", pw->pw_dir);
	putenv(p);
    }

    if (getenv("FTND_ROOT") == NULL) {
	printf("FATAL ERROR: Environment variable FTND_ROOT not set\n");
	free(FileName);
	exit(FTNERR_INIT_ERROR);
    }
    snprintf(FileName, PATH_MAX, "%s/etc/config.data", getenv("FTND_ROOT"));

    if (( pDataFile = fopen(FileName, "rb")) == NULL) {
	printf("FATAL ERROR: Can't open %s for reading!\n", FileName);
	printf("Please run ftnsetup to create configuration file.\n");
	printf("Or check that your environment variable FTND_ROOT is set to the BBS Path!\n");
	free(FileName);
	exit(FTNERR_CONFIG_ERROR);
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
	case 1:	    strcpy(month, *(mLanguage + 398)); break;
	case 2:	    strcpy(month, *(mLanguage + 399)); break;                    
	case 3:	    strcpy(month, *(mLanguage + 400)); break;                    
	case 4:	    strcpy(month, *(mLanguage + 401)); break;                    
	case 5:	    strcpy(month, *(mLanguage + 402)); break;                    
	case 6:	    strcpy(month, *(mLanguage + 403)); break;                    
	case 7:	    strcpy(month, *(mLanguage + 404)); break;                    
	case 8:	    strcpy(month, *(mLanguage + 405)); break;                    
	case 9:	    strcpy(month, *(mLanguage + 406)); break;                    
	case 10:    strcpy(month, *(mLanguage + 407)); break;                    
	case 11:    strcpy(month, *(mLanguage + 408)); break;                    
	case 12:    strcpy(month, *(mLanguage + 409)); break;
	default:    strcpy(month, "Unknown");      
    }                                              

    return(month);
}



/* Returns DD-Mmm-YYYY */
char *GLCdateyy()
{
    static char	GLcdateyy[15];
    char	ntime[15];

    Time_Now = time(NULL);
    l_date = localtime(&Time_Now);

    snprintf(GLcdateyy, 15, "%02d-", l_date->tm_mday);

    snprintf(ntime, 15, "-%02d", l_date->tm_year+1900);
    strcat(GLcdateyy, GetMonth(l_date->tm_mon+1));
    strcat(GLcdateyy,ntime);

    return (GLcdateyy);
}


