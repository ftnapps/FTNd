/*****************************************************************************
 *
 * $Id: door.c,v 1.26 2007/02/25 20:28:09 mbse Exp $
 * Purpose ...............: Run external door
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

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "language.h"
#include "input.h"
#include "timeout.h"
#include "exitinfo.h"
#include "whoson.h"
#include "door.h"
#include "term.h"
#include "ttyio.h"
#include "openport.h"


extern time_t	t_start;
extern int	e_pid;
extern char	**environ;
extern int	cols;
extern int	rows;


char *Gdate(time_t, int);
char *Gdate(time_t tt, int Y2K)
{
        static char     GLC[15];
        struct tm       *tm;

        tm = localtime(&tt);
        if (Y2K)
                snprintf(GLC, 15, "%02d-%02d-%04d", tm->tm_mon +1, tm->tm_mday, tm->tm_year + 1900);
        else
                snprintf(GLC, 15, "%02d-%02d-%02d", tm->tm_mon +1, tm->tm_mday, tm->tm_year % 100);

        return (GLC);
}



char *Rdate(char *, int);
char *Rdate(char *ind, int Y2K)
{
        static char     GLC[15];

        memset(&GLC, 0, sizeof(GLC));
        GLC[0] = ind[3];
        GLC[1] = ind[4];
        GLC[2] = '-';
        GLC[3] = ind[0];
        GLC[4] = ind[1];
        GLC[5] = '-';
        if (Y2K) {
                GLC[6] = ind[6];
                GLC[7] = ind[7];
                GLC[8] = ind[8];
                GLC[9] = ind[9];
        } else {
                GLC[6] = ind[8];
                GLC[7] = ind[9];
        }

        return GLC;
}



/*
 * Function will run a external program or door
 */
void ExtDoor(char *Program, int NoDoorsys, int Y2Kdoorsys, int Comport, int NoSuid, int NoPrompt, int SingleUser, char *What)
{
    char    *String, *String1;
    int	    i, rc, Start;
    char    *temp1, buf[128];
    FILE    *fp;

    temp1 = calloc(PATH_MAX, sizeof(char));
    String = calloc(81, sizeof(char));

    Start = TRUE;
    if (SingleUser && What) {
	/*
	 * Check if the door is in use
	 */
	while (TRUE) {
	    if (Start)
		snprintf(buf, 128, "GMON:1,1;");
	    else
		snprintf(buf, 128, "GMON:1,0;");
	    Start = FALSE;
	    if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "100:0;", 6) == 0)
		    break;  /* No more data */
		if (strstr(buf, "mbsebbs")) {
		    strtok(buf, ",");   /* Nr of parameters */
		    strtok(NULL, ",");  /* Pid              */
		    strtok(NULL, ",");  /* tty              */
		    strtok(NULL, ",");  /* username         */
		    strtok(NULL, ",");  /* program name     */
		    strtok(NULL, ",");  /* location         */
		    if (strcmp(strtok(NULL, ","), menus.DoorName) == 0) {
			Syslog('+', "User tried single user door %s, but door is in use", menus.DoorName);
			Enter(1);
			/* The door is in use by another user, try again later */
			pout(LIGHTRED, BLACK, (char *) Language(20));
			Enter(2);
			Pause();
			free(temp1);
			free(String);
			return;
		    }
		}
	    }
	}
    }
    
    WhosDoingWhat(DOOR, What);

    if ((strstr(Program, "/N")) != NULL) {
	snprintf(temp1, 81, "%d", iNode);
	strreplace(Program, (char *)"/N", temp1);
    }

    if ((strstr(Program, "/A")) != NULL) {
	Enter(1);
	colour(CYAN, BLACK);
	if ((String = strstr(Program, "/T=")) != NULL) {
	    String1 = String + 3;
	    PUTSTR(String1);
	} else
	    PUTSTR((char *)"Please enter filename: ");

	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp1, 80);

	strreplace(Program, (char *)"/A", temp1);

	for (i = 0; i < strlen(Program); i++) {
	    if (*(Program + i) == '\0')
		break;
	    if (*(Program + i) == '/')
		*(Program + i) = '\0';
	}
    }

    free(String);
    Syslog('+', "Door: %s", Program);
    ReadExitinfo();
    alarm_set((exitinfo.iTimeLeft * 60) - 10);
    Altime((exitinfo.iTimeLeft * 60));

    /*
     * Always remove the old door.sys first.
     */
    snprintf(temp1, PATH_MAX, "%s/%s/door.sys", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp1);

    /*
     * Write door.sys in users homedirectory
     */
    if (!NoDoorsys) {
	if ((fp = fopen(temp1, "w+")) == NULL) {
	    WriteError("$Can't create %s", temp1);
	} else {
	    if (Comport) {
		fprintf(fp, "COM1:\r\n"); /* COM port             */
		fprintf(fp, "19200\r\n");/* Effective baudrate   */
	    } else {
		fprintf(fp, "COM0:\r\n");/* COM port		*/
		fprintf(fp, "0\r\n");	/* Effective baudrate	*/
	    }
	    fprintf(fp, "8\r\n");		/* Databits		*/
	    fprintf(fp, "%d\r\n", iNode);	/* Node number		*/
	    if (Comport)
		fprintf(fp, "19200\r\n");/* Locked baudrate	*/
	    else
		fprintf(fp, "%d\r\n", ttyinfo.portspeed); /* Locked baudrate */
	    fprintf(fp, "Y\r\n");		/* Screen snoop		*/
	    fprintf(fp, "N\r\n");		/* Printer on		*/
	    fprintf(fp, "Y\r\n");		/* Page bell		*/
	    fprintf(fp, "Y\r\n");		/* Caller alarm		*/
	    fprintf(fp, "%s\r\n", exitinfo.sUserName);
	    fprintf(fp, "%s\r\n", exitinfo.sLocation);
	    fprintf(fp, "%s\r\n", exitinfo.sVoicePhone);
	    fprintf(fp, "%s\r\n", exitinfo.sDataPhone);
	    fprintf(fp, "%s\r\n", exitinfo.Password);
	    fprintf(fp, "%d\r\n", exitinfo.Security.level);
	    fprintf(fp, "%d\r\n", exitinfo.iTotalCalls);
	    fprintf(fp, "%s\r\n", Gdate(exitinfo.tLastLoginDate, Y2Kdoorsys));
	    fprintf(fp, "%d\r\n", exitinfo.iTimeLeft * 60);	/* Seconds	*/
	    fprintf(fp, "%d\r\n", exitinfo.iTimeLeft);	/* Minutes	*/
	    fprintf(fp, "GR\r\n");		/* Graphics GR,RIP,NG */
	    fprintf(fp, "%d\r\n", rows);
	    fprintf(fp, "N\r\n");		/* User mode, always N	*/
	    fprintf(fp, "\r\n");		/* Always blank		*/
	    fprintf(fp, "\r\n");		/* Always blank		*/
	    fprintf(fp, "%s\r\n", Rdate(exitinfo.sExpiryDate, Y2Kdoorsys));
	    fprintf(fp, "%d\r\n", grecno);	/* Users recordnumber	*/
	    fprintf(fp, "%s\r\n", exitinfo.sProtocol);
	    fprintf(fp, "%d\r\n", exitinfo.Uploads);
	    fprintf(fp, "%d\r\n", exitinfo.Downloads);
	    fprintf(fp, "%d\r\n", LIMIT.DownK); /* FIXME: Download Kb today */
	    fprintf(fp, "%d\r\n", LIMIT.DownK);
	    fprintf(fp, "%s\r\n", Rdate(exitinfo.sDateOfBirth, Y2Kdoorsys));
	    fprintf(fp, "\r\n");		/* Path to userbase	*/
	    fprintf(fp, "\r\n");		/* Path to messagebase	*/
	    fprintf(fp, "%s\r\n", CFG.sysop_name);
	    fprintf(fp, "%s\r\n", exitinfo.sHandle);
	    fprintf(fp, "none\r\n");	/* Next event time	*/
	    fprintf(fp, "Y\r\n");		/* Error free connect.	*/
	    fprintf(fp, "N\r\n");		/* Always N		*/
	    fprintf(fp, "Y\r\n");		/* Always Y		*/
	    fprintf(fp, "7\r\n");		/* Default textcolor	*/
	    fprintf(fp, "0\r\n");		/* Always 0		*/
	    fprintf(fp, "%s\r\n", Gdate(exitinfo.tLastLoginDate, Y2Kdoorsys)); /* Last newfiles scan date */
	    fprintf(fp, "%s\r\n", StrTimeHM(t_start));  /* Time of this call    */
	    fprintf(fp, "%s\r\n", LastLoginTime);	    /* Time of last call    */
	    fprintf(fp, "32768\r\n");	/* Always 32768		*/
	    fprintf(fp, "%d\r\n", exitinfo.DownloadsToday);
	    fprintf(fp, "%d\r\n", exitinfo.UploadK);
	    fprintf(fp, "%d\r\n", exitinfo.DownloadK);
	    fprintf(fp, "%s\r\n", exitinfo.sComment);
	    fprintf(fp, "0\r\n");		/* Always 0		*/
	    fprintf(fp, "%d\r\n\032", exitinfo.iPosted);
	    fclose(fp);
	}
    }

    /*
     * Always remove the old door32.sys first.
     */
    snprintf(temp1, PATH_MAX, "%s/%s/door32.sys", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp1);

    /*
     * Write door32.sys in users homedirectory
     */
    if (!NoDoorsys) {
	if ((fp = fopen(temp1, "w+")) == NULL) {
	    WriteError("$Can't create %s", temp1);
	} else {
	    if (Comport) {
		fprintf(fp, "1\r\n");			    /* COM type, 1=serial, 2=telnet	*/
		fprintf(fp, "1\r\n");			    /* COM port number			*/
		fprintf(fp, "19200\r\n");		    /* Effective baudrate		*/
	    } else {
		fprintf(fp, "0\r\n");			    /* COM type, 0=local		*/
		fprintf(fp, "0\r\n");			    /* COM port				*/
		fprintf(fp, "0\r\n");			    /* Effective baudrate		*/
	    }
	    fprintf(fp, "%s\r\n", CFG.bbs_name);	    /* BBS name				*/
	    fprintf(fp, "%d\r\n", grecno);		    /* User record			*/
	    fprintf(fp, "%s\r\n", exitinfo.sUserName);	    /* User's real name			*/
	    fprintf(fp, "%s\r\n", exitinfo.sHandle);	    /* User's handle			*/
	    fprintf(fp, "%d\r\n", exitinfo.Security.level); /* User's security level		*/
	    fprintf(fp, "%d\r\n", exitinfo.iTimeLeft);	    /* User's time left in minutes	*/
	    fprintf(fp, "1\r\n");			    /* User's graphic mode		*/
	    fprintf(fp, "%d\r\n\032", iNode);		    /* Node number			*/
	    fclose(fp);
	}
    }

    clear();
    PUTSTR((char *)"Loading, please wait ...");
    Enter(2);

    /*
     * Put terminal back in cooked mode, prefered by some doors.
     */
    cookedport();

    if (NoSuid) 
	rc = exec_nosuid(Program);
    else
	rc = execute_str((char *)"/bin/sh", (char *)"-c", Program, NULL, NULL, NULL);

    /*
     * First set cookedport again, this will load our original saved
     * termios values. This way it doesn't matter what the door might
     * have done with the original termios.
     */
    cookedport();
    rawport();

    Altime(0);
    alarm_off();
    alarm_on();
    Syslog('+', "Door end, rc=%d", rc);

    free(temp1);
    Enter(2);

    if (!NoPrompt)
	Pause();
}



/*
 * Execute a door as real user, not suid.
 */
int exec_nosuid(char *mandato)
{
    int	    rc, status;
    pid_t   pid;
    char    *argv[4];
    
    if (mandato == NULL)
	return 1;   /* Prevent running a shell  */

    Syslog('+', "Execve: /bin/sh -c %s", mandato);
    pid = fork();
    if (pid == -1)
	return 1;
    if (pid == 0) {
	msleep(150);
	argv[0] = (char *)"sh";
	argv[1] = (char *)"-c";
	argv[2] = mandato;
	argv[3] = 0;
	execve("/bin/sh", argv, environ);
	exit(MBERR_EXEC_FAILED);
    }
    e_pid = pid;

    do {
	rc = waitpid(pid, &status, 0);
	e_pid = 0;
    } while (((rc > 0) && (rc != pid)) || ((rc == -1) && (errno == EINTR)));

    switch(rc) {
	case -1:
		WriteError("$Waitpid returned %d, status %d,%d", rc,status>>8,status&0xff);
		return -1;
	case 0:
		return 0;
	default:
		if (WIFEXITED(status)) {
		    rc = WEXITSTATUS(status);
		    if (rc) {
			WriteError("Exec_nosuid: returned error %d", rc);
			return rc;
		    }
		}
		if (WIFSIGNALED(status)) {
		    rc = WTERMSIG(status);
		    WriteError("Wait stopped on signal %d", rc);
		    return rc;
		}
		if (rc)
		    WriteError("Wait stopped unknown, rc=%d", rc);
		return rc;      
	}
	return 0;
}


