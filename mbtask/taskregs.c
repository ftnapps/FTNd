/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Buffers for registration information.
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "libs.h"
#include "../lib/structs.h"
#include "taskstat.h"
#include "taskregs.h"
#include "taskchat.h"
#include "taskutil.h"

extern reg_info		reginfo[MAXCLIENT];     /* Array with clients   */
extern struct sysconfig	CFG;			/* System config	*/
static int 		entrypos = 0;		/* Status pointer	*/
static int		mailers = 0;		/* Registered mailers	*/
static int		sysop_present = 0;	/* Sysop present	*/
int			ipmailers = 0;		/* TCP/IP mail sessions	*/


/***********************************************************************
 *
 * Search for a pid.
 */
int reg_find(char *);
int reg_find(char *pids)
{
    int	i;

    for (i = 0; i < MAXCLIENT; i++) {
	if ((int)reginfo[i].pid == atoi(pids))
	    return i;
    }

    Syslog('?', "Panic, pid %s not found", pids);
    return -1;
}



/***********************************************************************
 *
 *  Registrate a new connection and fill the data.
 */

int reg_newcon(char *data)
{
    char    *cnt, *pid, *tty, *uid, *prg, *city;
    int	    retval;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ",");
    tty = strtok(NULL, ",");
    uid = strtok(NULL, ",");
    prg = strtok(NULL, ",");
    city = strtok(NULL, ";");

    /*
     * Abort if no empty record is found 
     */
    if ((retval = reg_find((char *)"0")) == -1) {
	Syslog('?', "Maximum clients (%d) reached", MAXCLIENT);
	return -1;
    }

    memset((char *)&reginfo[retval], 0, sizeof(reg_info));
    reginfo[retval].pid = atoi(pid);
    strncpy((char *)&reginfo[retval].tty, tty, 6);
    strncpy((char *)&reginfo[retval].uname, uid, 35);
    strncpy((char *)&reginfo[retval].prg, prg, 14); 
    strncpy((char *)&reginfo[retval].city, city, 35);
    strcpy((char *)&reginfo[retval].doing, "-"); 
    reginfo[retval].started = time(NULL); 
    reginfo[retval].lastcon = time(NULL);
    reginfo[retval].altime = 600;

    /*
     * Everyone says do not disturb, unless the flag
     * is cleared by the owner of this process.
     */
    reginfo[retval].silent = 1;

    stat_inc_clients();
    if (strcmp(prg, (char *)"mbcico") == 0)
	mailers++;
    Syslog('-', "Registered client pgm \"%s\", pid %s, slot %d, mailers %d, TCP/IP %d", 
		prg, pid, retval, mailers, ipmailers);
    return retval;
}



int reg_closecon(char *data)
{
    char    *cnt, *pid;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ";");
    if ((rec = reg_find(pid)) == -1)
	return -1;

    if (strcmp(reginfo[rec].prg, (char *)"mbcico") == 0)
	mailers--;
    if (reginfo[rec].istcp)
	ipmailers--;
    if ((strcmp(reginfo[rec].prg, (char *)"mbsebbs") == 0) || (strcmp(reginfo[rec].prg, (char *)"mbmon") == 0))
	chat_cleanuser(atoi(pid));

    Syslog('-', "Unregistered client pgm \"%s\", pid %s, slot %d, mailers %d, TCP/IP %d", 
		reginfo[rec].prg, pid, rec, mailers, ipmailers);
    memset(&reginfo[rec], 0, sizeof(reg_info)); 
    stat_dec_clients();
    return 0;
}



/*
 * Check all registred connections.
 */
void reg_check(void)
{
    int	    i;
    time_t  Now;

    Now = time(NULL);
    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid) {
	    if (kill(reginfo[i].pid, 0) == -1) {
		if (errno == ESRCH) {
		    if (strcmp(reginfo[i].prg, (char *)"mbcico") == 0)
			mailers--;
		    if (reginfo[i].istcp)
			ipmailers--;
		    if ((strcmp(reginfo[i].prg, (char *)"mbsebbs") == 0) || (strcmp(reginfo[i].prg, (char *)"mbmon") == 0))
			chat_cleanuser(reginfo[i].pid);

		    Syslog('?', "Stale registration found for pid %d (%s), mailers now %d, TCP/IP now %d", 
						reginfo[i].pid, reginfo[i].prg, mailers, ipmailers);
		    memset(&reginfo[i], 0, sizeof(reg_info));
		    stat_dec_clients();
		}
	    } else {
		/*
		 * Check timeout
		 */
		if ((Now - reginfo[i].lastcon) >= reginfo[i].altime) {
		    if (reginfo[i].altime < 600) {
			kill(reginfo[i].pid, SIGKILL);
			Syslog('+', "Send SIGKILL to pid %d", reginfo[i].pid);
		    } else {
			kill(reginfo[i].pid, SIGTERM);
			Syslog('+', "Send SIGTERM to pid %d", reginfo[i].pid);
		    }
		    /*
		     *  10 seconds to the next kill
		     */
		    reginfo[i].altime = 10;
		    reginfo[i].lastcon = time(NULL);
		}
	    }
	}
    }
}



/*
 * Update doing information for this process.
 */
int reg_doing(char *data)
{
    char    *cnt, *pid, *line;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ",");
    line = strtok(NULL, ";");

    if ((rec = reg_find(pid)) == -1)
	return -1;

    strncpy(reginfo[rec].doing, line, 35);
    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Registrate connection as TCP/IP connection
 */
int reg_ip(char *data)
{
    char    *cnt, *pid;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ";");

    if ((rec = reg_find(pid)) == -1)
	return -1;

    reginfo[rec].istcp = TRUE;
    reginfo[rec].lastcon = time(NULL);
    ipmailers++;
    Syslog('?', "TCP/IP session registered (%s), now %d sessions", pid, ipmailers);
    return 0;
}



/*
 * Update timer using NOP
 */
int reg_nop(char *data)
{
    char    *cnt, *pid;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ";");
    if ((rec = reg_find(pid)) == -1)
	return -1;

    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Set new timer value
 */
int reg_timer(int Set, char *data)
{
    char    *pid;
    int	    cnt, rec, val;

    cnt = atoi(strtok(data, ","));
    if (Set) {
	if (cnt != 2)
	    return -1;
	pid = strtok(NULL, ",");
	val = atoi(strtok(NULL, ";"));
	if (val < 600)
	    val = 600;
    } else {
	if (cnt != 1)
	    return -1;
	pid = strtok(NULL, ";");
	val = 600;
    }

    if ((rec = reg_find(pid)) == -1)
	return -1;

    reginfo[rec].altime = val;
    reginfo[rec].lastcon = time(NULL);
    Syslog('r', "Set timeout value for %d to %d", reginfo[rec].pid, val);
    return 0;
}



/*
 * Update tty information for this process.
 */
int reg_tty(char *data)
{
    char    *cnt, *pid, *tty;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ",");
    tty = strtok(NULL, ";");

    if ((rec = reg_find(pid)) == -1)
	return -1;

    strncpy((char *)&reginfo[rec].tty, tty, 6);
    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Update the "do not disturb" flag.
 */
int reg_silent(char *data)
{
    char    *cnt, *pid, *line;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ",");
    line = strtok(NULL, ";");

    if ((rec = reg_find(pid)) == -1)
	return -1;

    reginfo[rec].silent = atoi(line);
    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Update username and city for this process.
 */
int reg_user(char *data)
{
    char    *cnt, *pid, *user, *city;
    int	    rec;

    cnt  = strtok(data, ",");
    pid  = strtok(NULL, ",");
    user = strtok(NULL, ",");
    city = strtok(NULL, ";");

    if ((rec = reg_find(pid)) == -1)
	return -1;

    strncpy((char *)&reginfo[rec].uname, user, 35);
    strncpy((char *)&reginfo[rec].city,  city, 35);
    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Register sysop available for chat
 */
int reg_sysop(char *data)
{
    char    *cnt, *pid;
    int	    rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ",");
    sysop_present = atoi(strtok(NULL, ";"));
 
    if ((rec = reg_find(pid)) != -1) {
	reginfo[rec].lastcon = time(NULL);
    }

    Syslog('+', "Sysop present for chat: %s", sysop_present ? "True":"False");
    return 0;
}



/*
 * Check for personal message
 */
char *reg_ipm(char *data)
{
    char	*cnt, *pid;
    static char	buf[128];
    int		rec;

    buf[0] = '\0';
    sprintf(buf, "100:0;");
    cnt = strtok(data, ",");
    pid = strtok(NULL, ";");

    if ((rec = reg_find(pid)) == -1)
	return buf;

    reginfo[rec].lastcon = time(NULL);
    if (!reginfo[rec].ismsg)
	return buf;

    buf[0] = '\0';
    sprintf(buf, "100:2,%s,%s;", reginfo[rec].fname[reginfo[rec].ptr_out], reginfo[rec].msg[reginfo[rec].ptr_out]);
    if (reginfo[rec].ptr_out < RB)
	reginfo[rec].ptr_out++;
    else
	reginfo[rec].ptr_out = 0;
    if (reginfo[rec].ptr_out == reginfo[rec].ptr_in)
	reginfo[rec].ismsg = FALSE;
    
    Syslog('+', "reg_ipm: in=%d out=%d ismsg=%d", reginfo[rec].ptr_in, reginfo[rec].ptr_out, reginfo[rec].ismsg);

    return buf;
}



/*
 * Send personal message
 */
int reg_spm(char *data)
{
    char    *cnt, *from, *too, *txt, *log;
    int	    i;

    cnt  = strtok(data, ",");
    from = strtok(NULL, ",");
    too  = strtok(NULL, ",");
    txt  = strtok(NULL, "\0");
    txt[strlen(txt)-1] = '\0';

    Syslog('-', "SIPM:%s,%s,%s,%s;", cnt, from, too, txt);

    for (i = 0; i < MAXCLIENT; i++) {
	if (reginfo[i].pid && (strcasecmp(reginfo[i].uname, too) == 0)) {
	    /*
	     *  If the in and out pointers are the same and the 
	     *  message present flag is still set, then this user
	     *  can't get anymore new messages.
	     */
	    if (reginfo[i].ismsg && (reginfo[i].ptr_in == reginfo[i].ptr_out)) {
		return 2;
	    }

	    /*
	     *  If user has the "do not distrurb" flag set, but the sysop ignore's this.
	     */
	    if (reginfo[i].silent) {
		return 1;
	    }

	    /*
	     *  If all is well, insert the new message.
	     */
	    strncpy((char *)&reginfo[i].fname[reginfo[i].ptr_in], from, 35);
	    strncpy((char *)&reginfo[i].msg[reginfo[i].ptr_in], txt, 80);
	    if (reginfo[i].ptr_in < RB)
		reginfo[i].ptr_in++;
	    else
		reginfo[i].ptr_in = 0;
	    reginfo[i].ismsg = TRUE;

	    if (CFG.iAutoLog && strlen(CFG.chat_log)) {
		log = calloc(PATH_MAX, sizeof(char));
		sprintf(log, "%s/log/%s", getenv("MBSE_ROOT"), CFG.chat_log);
		ulog(log, (char *)"+", from, (char *)"-1", txt);
		free(log);
	    }

	    Syslog('+', "reg_spm: rec=%d in=%d out=%d ismsg=%d", i, reginfo[i].ptr_in, reginfo[i].ptr_out, reginfo[i].ismsg);
	    return 0;
	}
    }
    
    return 3;   // Error, user not found
}



char *reg_fre(void)
{
    static char	buf[80];
    int		i, users = 0, utils = 0;

    buf[0] = '\0';

    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid) {
	    if ((!strncmp(reginfo[i].prg, "mbsebbs", 7)) ||
		(!strncmp(reginfo[i].prg, "mbnewusr", 8)) ||
		(!strncmp(reginfo[i].prg, "mbftpd", 6)))
		users++;

	    if ((!strncmp(reginfo[i].prg, "mbfido", 6)) ||
		(!strncmp(reginfo[i].prg, "mbmail", 6)) ||
		(!strncmp(reginfo[i].prg, "mball", 5)) ||
	        (!strncmp(reginfo[i].prg, "mbaff", 5)) ||
	        (!strncmp(reginfo[i].prg, "mbcico", 6)) ||
	        (!strncmp(reginfo[i].prg, "mbfile", 6)) ||
	        (!strncmp(reginfo[i].prg, "mbmsg", 5)) ||
	        (!strncmp(reginfo[i].prg, "mbindex", 7)) ||
	        (!strncmp(reginfo[i].prg, "mbdiff", 6)) ||
	        (!strncmp(reginfo[i].prg, "mbuser", 6)))
		utils++;
	}
    }

    if (users || utils)
	sprintf(buf, "100:1,Running utilities: %02d  Active users: %02d;", utils, users);
    else
	sprintf(buf, "100:0;");
    return buf;
}



/*
 * Get registration information. The first time the parameter
 * must be 1, for the next searches 0. Returns 100:0; if there
 * is an error or the end of file is reached.
 */
char *get_reginfo(int first)
{
    static char	buf[256];

    memset(&buf, 0, sizeof(buf));
    sprintf(buf, "100:0;");

    /*
     * Loop forever until an error occours, eof is reached or
     * the data is valid. Only in the last case valid data is
     * returned to the caller.
     */
    for (;;) {
	if (first == 1)
	    entrypos = 0;
	else
	    entrypos++;

	if (entrypos == MAXCLIENT)
	    return buf;

	if ((int)reginfo[entrypos].pid != 0) {
	    sprintf(buf, "100:7,%d,%s,%s,%s,%s,%s,%d;", 
				reginfo[entrypos].pid, reginfo[entrypos].tty,
				reginfo[entrypos].uname, reginfo[entrypos].prg,
				reginfo[entrypos].city, reginfo[entrypos].doing,
				(int)reginfo[entrypos].started);
	    return buf;
	}
    }
    /* never reached */
}



/*
 * Page sysop for a chat
 */
int reg_page(char *data)
{
    char    *cnt, *pid, *reason;
    int     i, rec;
	        
    cnt    = strtok(data, ",");
    pid    = strtok(NULL, ",");
    reason = strtok(NULL, "\0");
    reason[strlen(reason)-1] = '\0';

    Syslog('+', "reg_page: pid=%d, reason=\"%s\"", pid, reason);

    if (!sysop_present)
	return 2;

    /*
     * Check if another user is paging the sysop or has paged the sysop.
     * If so, mark sysop busy.
     */
    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid && (reginfo[i].pid != atoi(pid)) && (reginfo[i].paging || reginfo[i].haspaged))
	    return 1;
    }

    if ((rec = reg_find(pid)) == -1)
	return 3;

    /*
     * All seems well, accept the page
     */
    reginfo[rec].paging = TRUE;
    strncpy(reginfo[rec].reason, reason, 80);
    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Cancel page sysop for a chat
 */
int reg_cancel(char *data)
{
    char    *cnt, *pid;
    int     rec;

    cnt = strtok(data, ",");
    pid = strtok(NULL, ";");

    Syslog('+', "reg_cancel: pid=%s", pid);

    if ((rec = reg_find(pid)) == -1)
	return -1;

    if (reginfo[rec].paging) {
	reginfo[rec].paging = FALSE;
	reginfo[rec].haspaged = TRUE;
    }
    reginfo[rec].lastcon = time(NULL);
    return 0;
}



/*
 * Check paging status for from mbmon
 */
char *reg_checkpage(char *data)
{
    static char	buf[128];
    int		i;

    memset(&buf, 0, sizeof(buf));
    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid && reginfo[i].paging) {
	    sprintf(buf, "100:3,%d,1,%s;", reginfo[i].pid, reginfo[i].reason);
	    return buf;
	}
	if (reginfo[i].pid && reginfo[i].haspaged) {
	    sprintf(buf, "100:3,%d,0,%s;", reginfo[i].pid, reginfo[i].reason);
	    return buf;
	}
    }
    sprintf(buf, "100:0;");
    return buf;
}



/*
 * Check if this user has paged or is paging
 */
int reg_ispaging(char *pid)
{
    int	    rec;
    
    if ((rec = reg_find(pid)) == -1)
	return FALSE;

    return (reginfo[rec].paging || reginfo[rec].haspaged);
}



/*
 * Mark that this user is now talking to the sysop
 */
void reg_sysoptalk(char *pid)
{
    int	    rec;

    if ((rec = reg_find(pid)) == -1)
	return;

    reginfo[rec].paging = FALSE;
    reginfo[rec].haspaged = FALSE;
}


