/*****************************************************************************
 *
 * $Id: taskregs.c,v 1.27 2006/06/10 11:59:02 mbse Exp $
 * Purpose ...............: Buffers for registration information.
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
#include "../lib/mbselib.h"
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
int reg_find(pid_t);
int reg_find(pid_t pids)
{
    int	i;

    for (i = 0; i < MAXCLIENT; i++) {
	if (reginfo[i].pid == pids)
	    return i;
    }

    WriteError("Panic, pid %d not found", (int)pids);
    return -1;
}



/***********************************************************************
 *
 *  Registrate a new connection and fill the data.
 */

int reg_newcon(char *data)
{
    char    *tty, *uid, *prg, *city;
    int	    retval;
    pid_t   pid;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    tty = xstrcpy(strtok(NULL, ","));
    uid = xstrcpy(cldecode(strtok(NULL, ",")));
    prg = xstrcpy(cldecode(strtok(NULL, ",")));
    city = xstrcpy(cldecode(strtok(NULL, ";")));

    /*
     * Abort if no empty record is found 
     */
    if ((retval = reg_find((pid_t)0)) == -1) {
	Syslog('?', "Maximum clients (%d) reached", MAXCLIENT);
	retval = -1;
    } else {
	memset((char *)&reginfo[retval], 0, sizeof(reg_info));
	reginfo[retval].pid = pid;
	strncpy((char *)&reginfo[retval].tty, tty, 6);
	strncpy((char *)&reginfo[retval].uname, uid, 35);
	strncpy((char *)&reginfo[retval].prg, prg, 14); 
	strncpy((char *)&reginfo[retval].city, city, 35);
	strcpy((char *)&reginfo[retval].doing, "-"); 
	reginfo[retval].started = (int)time(NULL); 
	reginfo[retval].lastcon = (int)time(NULL);
	reginfo[retval].altime = 600;

	/*
	 * Everyone says do not disturb, unless the flag
	 * is cleared by the owner of this process.
	 */
	reginfo[retval].silent = 1;

	stat_inc_clients();
	if (strcmp(prg, (char *)"mbcico") == 0)
	    mailers++;
	Syslog('-', "Registered client pgm \"%s\", pid %d, slot %d, mailers %d, TCP/IP %d", 
		prg, (int)pid, retval, mailers, ipmailers);
    }

    free(uid);
    free(prg);
    free(city);
    free(tty);
    return retval;
}



int reg_closecon(char *data)
{
    pid_t   pid;
    int	    rec;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));
    if ((rec = reg_find(pid)) == -1) {
	return -1;
    }

    if (strcmp(reginfo[rec].prg, (char *)"mbcico") == 0)
	mailers--;
    if (reginfo[rec].istcp)
	ipmailers--;
    if ((strcmp(reginfo[rec].prg, (char *)"mbsebbs") == 0) || (strcmp(reginfo[rec].prg, (char *)"mbmon") == 0))
	chat_cleanuser(pid);

    Syslog('-', "Unregistered client pgm \"%s\", pid %d, slot %d, mailers %d, TCP/IP %d", 
		reginfo[rec].prg, (int)pid, rec, mailers, ipmailers);
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
		if (((int)Now - reginfo[i].lastcon) >= reginfo[i].altime) {
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
		    reginfo[i].lastcon = (int)time(NULL);
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
    char    *line;
    int	    rec;
    pid_t   pid;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    line = xstrcpy(cldecode(strtok(NULL, ";")));

    if ((rec = reg_find(pid)) == -1) {
	free(line);
	return -1;
    }

    strncpy(reginfo[rec].doing, line, 35);
    reginfo[rec].lastcon = (int)time(NULL);
    free(line);
    return 0;
}



/*
 * Registrate connection as TCP/IP connection
 */
int reg_ip(char *data)
{
    pid_t   pid;
    int	    rec;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));

    if ((rec = reg_find(pid)) == -1) {
	return -1;
    }

    reginfo[rec].istcp = TRUE;
    reginfo[rec].lastcon = (int)time(NULL);
    ipmailers++;
    Syslog('?', "TCP/IP session registered (%d), now %d sessions", (int)pid, ipmailers);
    return 0;
}



/*
 * Update timer using NOP
 */
int reg_nop(char *data)
{
    pid_t   pid;
    int	    rec;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));
    
    if ((rec = reg_find(pid)) == -1) {
	return -1;
    }
    
    reginfo[rec].lastcon = (int)time(NULL);
    return 0;
}



/*
 * Set new timer value
 */
int reg_timer(int Set, char *data)
{
    pid_t   pid;
    int	    cnt, rec, val;

    cnt = atoi(strtok(data, ","));
    if (Set) {
	if (cnt != 2)
	    return -1;
	pid = (pid_t)atoi(strtok(NULL, ","));
	val = atoi(strtok(NULL, ";"));
	if (val < 600)
	    val = 600;
    } else {
	if (cnt != 1)
	    return -1;
	pid = (pid_t)atoi(strtok(NULL, ";"));
	val = 600;
    }

    if ((rec = reg_find(pid)) == -1) {
	return -1;
    }

    reginfo[rec].altime = val;
    reginfo[rec].lastcon = (int)time(NULL);
    Syslog('r', "Set timeout value for %d to %d", (int)pid, val);
    return 0;
}



/*
 * Update tty information for this process.
 */
int reg_tty(char *data)
{
    char    *tty;
    int	    rec;
    pid_t   pid;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    tty = xstrcpy(strtok(NULL, ";"));

    if ((rec = reg_find(pid)) == -1) {
	free(tty);
	return -1;
    }
    
    strncpy((char *)&reginfo[rec].tty, tty, 6);
    reginfo[rec].lastcon = (int)time(NULL);
    free(tty);
    return 0;
}



/*
 * Update the "do not disturb" flag.
 */
int reg_silent(char *data)
{
    int	    rec, line;
    pid_t   pid;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    line = atoi(strtok(NULL, ";"));

    if ((rec = reg_find(pid)) == -1) {
	return -1;
    }
    
    reginfo[rec].silent = line;
    reginfo[rec].lastcon = (int)time(NULL);
    return 0;
}



/*
 * Update username and city for this process.
 */
int reg_user(char *data)
{
    char    *user, *city;
    int	    rec;
    pid_t   pid;

    strtok(data, ",");
    pid  = (pid_t)atoi(strtok(NULL, ","));
    user = xstrcpy(cldecode(strtok(NULL, ",")));
    city = xstrcpy(cldecode(strtok(NULL, ";")));

    if ((rec = reg_find(pid)) == -1) {
	free(user);
	free(city);
	return -1;
    }

    strncpy((char *)&reginfo[rec].uname, user, 35);
    strncpy((char *)&reginfo[rec].city,  city, 35);
    reginfo[rec].lastcon = (int)time(NULL);
    free(user);
    free(city);
    return 0;
}



/*
 * Register sysop available for chat
 */
int reg_sysop(char *data)
{
    pid_t   pid;
    int	    rec;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    sysop_present = atoi(strtok(NULL, ";"));
 
    if ((rec = reg_find(pid)) != -1) {
	reginfo[rec].lastcon = (int)time(NULL);
    }

    Syslog('+', "Sysop present for chat: %s", sysop_present ? "True":"False");
    return 0;
}



/*
 * Check for personal message
 */
void reg_ipm_r(char *data, char *buf)
{
    char    *name, *msg;
    int	    rec;
    pid_t   pid;

    buf[0] = '\0';
    snprintf(buf, SS_BUFSIZE, "100:0;");
    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));

    if ((rec = reg_find(pid)) == -1)
	return;

    reginfo[rec].lastcon = (int)time(NULL);
    if (!reginfo[rec].ismsg)
	return;

    name = xstrcpy(clencode(reginfo[rec].fname[reginfo[rec].ptr_out]));
    msg  = xstrcpy(clencode(reginfo[rec].msg[reginfo[rec].ptr_out]));
    snprintf(buf, SS_BUFSIZE, "100:2,%s,%s;", name, msg);
    if (reginfo[rec].ptr_out < RB)
	reginfo[rec].ptr_out++;
    else
	reginfo[rec].ptr_out = 0;
    if (reginfo[rec].ptr_out == reginfo[rec].ptr_in)
	reginfo[rec].ismsg = FALSE;
    
    Syslog('+', "reg_ipm: in=%d out=%d ismsg=%d", reginfo[rec].ptr_in, reginfo[rec].ptr_out, reginfo[rec].ismsg);

    free(name);
    free(msg);
    return;
}



/*
 * Send personal message
 */
int reg_spm(char *data)
{
    char    *from, *too, *txt, *logm;
    int	    i;

    strtok(data, ",");
    from = xstrcpy(cldecode(strtok(NULL, ",")));
    too  = xstrcpy(cldecode(strtok(NULL, ",")));
    txt  = xstrcpy(cldecode(strtok(NULL, ";")));
    Syslog('+', "PM from \"%s\" to \"%s\": \"%s\"", from, too, txt);

    for (i = 0; i < MAXCLIENT; i++) {
	if (reginfo[i].pid && (strcasecmp(reginfo[i].uname, too) == 0)) {
	    /*
	     *  If the in and out pointers are the same and the 
	     *  message present flag is still set, then this user
	     *  can't get anymore new messages.
	     */
	    if (reginfo[i].ismsg && (reginfo[i].ptr_in == reginfo[i].ptr_out)) {
		free(from);
		free(too);
		free(txt);
		return 2;
	    }

	    /*
	     *  If user has the "do not distrurb" flag set, but the sysop ignore's this.
	     */
	    if (reginfo[i].silent) {
		free(from);
		free(too);
		free(txt);
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
		logm = calloc(PATH_MAX, sizeof(char));
		snprintf(logm, PATH_MAX, "%s/log/%s", getenv("MBSE_ROOT"), CFG.chat_log);
		ulog(logm, (char *)"+", from, (char *)"-1", txt);
		free(logm);
	    }

	    Syslog('+', "reg_spm: rec=%d in=%d out=%d ismsg=%d", i, reginfo[i].ptr_in, reginfo[i].ptr_out, reginfo[i].ismsg);
	    free(from);
	    free(too);
	    free(txt);
	    return 0;
	}
    }
    
    free(from);
    free(too);
    free(txt);
    return 3;   // Error, user not found
}



void reg_fre_r(char *buf)
{
    int		i, users = 0, utils = 0;

    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid) {
	    if ((!strncmp(reginfo[i].prg, "mbsebbs", 7)) ||
		(!strncmp(reginfo[i].prg, "mbnewusr", 8)) ||
		(!strncmp(reginfo[i].prg, "mbnntp", 6))) 
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
	snprintf(buf, 80, "100:1,Running utilities: %02d  Active users: %02d;", utils, users);
    else
	snprintf(buf, 80, "100:0;");
    return;
}



/*
 * Get registration information. The first time the parameter
 * must be 1, for the next searches 0. Returns 100:0; if there
 * is an error or the end of file is reached.
 */
void get_reginfo_r(int first, char *buf)
{
    char    *u, *p, *c, *d;

    snprintf(buf, SS_BUFSIZE, "100:0;");

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
	    return;

	if ((int)reginfo[entrypos].pid != 0) {
	    u = xstrcpy(clencode(reginfo[entrypos].uname));
	    p = xstrcpy(clencode(reginfo[entrypos].prg));
	    c = xstrcpy(clencode(reginfo[entrypos].city));
	    d = xstrcpy(clencode(reginfo[entrypos].doing));
	    snprintf(buf, SS_BUFSIZE, "100:7,%d,%s,%s,%s,%s,%s,%d;", 
				reginfo[entrypos].pid, reginfo[entrypos].tty,
				u, p, c, d, reginfo[entrypos].started);
	    free(u);
	    free(p);
	    free(c);
	    free(d);
	    return;
	}
    }
    /* never reached */
}



/*
 * Page sysop for a chat
 */
int reg_page(char *data)
{
    char    *reason;
    int     i, rec;
    pid_t   pid;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    reason = xstrcpy(cldecode(strtok(NULL, ";")));

    Syslog('+', "reg_page: pid=%d, reason=\"%s\"", (int)pid, reason);

    if (!sysop_present) {
	free(reason);
	return 2;
    }

    /*
     * Check if another user is paging the sysop or has paged the sysop.
     * If so, mark sysop busy.
     */
    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid && (reginfo[i].pid != pid) && (reginfo[i].paging || reginfo[i].haspaged)) {
	    free(reason);
	    return 1;
	}
    }

    if ((rec = reg_find(pid)) == -1) {
	free(reason);
	return 3;
    }

    /*
     * All seems well, accept the page
     */
    reginfo[rec].paging = TRUE;
    strncpy(reginfo[rec].reason, reason, 80);
    reginfo[rec].lastcon = (int)time(NULL);
    return 0;
}



/*
 * Cancel page sysop for a chat
 */
int reg_cancel(char *data)
{
    pid_t   pid;
    int     rec;

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));

    Syslog('+', "reg_cancel: pid=%d", (int)pid);

    if ((rec = reg_find(pid)) == -1)
	return -1;

    if (reginfo[rec].paging) {
	reginfo[rec].paging = FALSE;
	reginfo[rec].haspaged = TRUE;
    }

    reginfo[rec].lastcon = (int)time(NULL);
    return 0;
}



/*
 * Check paging status for from mbmon
 */
void reg_checkpage_r(char *data, char *buf)
{
    int		i;

    for (i = 1; i < MAXCLIENT; i++) {
	if (reginfo[i].pid && reginfo[i].paging) {
	    snprintf(buf, SS_BUFSIZE, "100:3,%d,1,%s;", reginfo[i].pid, clencode(reginfo[i].reason));
	    return;
	}
	if (reginfo[i].pid && reginfo[i].haspaged) {
	    snprintf(buf, SS_BUFSIZE, "100:3,%d,0,%s;", reginfo[i].pid, clencode(reginfo[i].reason));
	    return;
	}
    }
    snprintf(buf, SS_BUFSIZE, "100:0;");
    return;
}



/*
 * Check if this user has paged or is paging
 */
int reg_ispaging(pid_t pid)
{
    int	    rec;
    
    if ((rec = reg_find(pid)) == -1)
	return FALSE;

    return (reginfo[rec].paging || reginfo[rec].haspaged);
}



/*
 * Mark that this user is now talking to the sysop
 */
void reg_sysoptalk(pid_t pid)
{
    int	    rec;

    if ((rec = reg_find(pid)) == -1)
	return;

    reginfo[rec].paging = FALSE;
    reginfo[rec].haspaged = FALSE;
}


