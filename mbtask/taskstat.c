/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Keep track of server status 
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "../lib/structs.h"
#include "taskstat.h"
#include "callstat.h"
#include "outstat.h"
#include "taskutil.h"



/*
 *  Semafores
 */
int			s_scanout  = FALSE;
int			s_mailout  = FALSE;
int			s_mailin   = FALSE;
int			s_reqindex = FALSE;
int			s_index    = FALSE;
int			s_msglink  = FALSE;
int			s_newnews  = FALSE;
int			s_bbsopen  = FALSE;
int			s_do_inet  = FALSE;
extern int		UPSalarm;
extern int		ptimer;



extern struct taskrec	TCFG;
extern int		internet;
extern int		ZMH;


typedef struct {
	long		tot_clt;	/* Total client connects 	*/
	long		peak_clt;	/* Peak simultaneous tot_cltes	*/
	long		s_error;	/* Syntax errors from clients	*/
	long		c_error;	/* Comms errors from clients	*/
} cl_stat;


typedef struct {
	time_t		start;		/* Start date/time		*/
	time_t		laststart;	/* Last start date/time		*/
	time_t		daily;		/* Last daily update		*/
	long		startups;	/* Total starts			*/
	long		clients;	/* Connected clients		*/
	cl_stat		total;		/* Total statistics		*/
	cl_stat		today;		/* Todays statistics		*/
	unsigned	open	: 1;	/* Is BBS open			*/
	unsigned long	sequence;	/* Sequencer counter		*/
} status_r;


static char	stat_fn[PATH_MAX];	/* Statusfile name		*/
static status_r	status;			/* Status data			*/
extern double	Load;			/* System Load			*/
extern int	Processing;		/* Is system running		*/


/************************************************************************
 *
 *  Initialize the statusfile, create it if necesary.
 */
void status_init()
{
	size_t	cnt;
	int	stat_fd;

	sprintf(stat_fn,  "%s/var/status.mbsed", getenv("MBSE_ROOT"));

	/*
	 * First check if this is the very first time we start the show.
	 * If so, we generate an empty status file with only the start
	 * date in it.
	 */
	stat_fd = open(stat_fn, O_RDWR);
	if (stat_fd == -1) {
		memset((char *)&status, 0, sizeof(status_r));
		status.start = time(NULL);
		status.daily = time(NULL);
		status.sequence = (unsigned long)time(NULL);
		stat_fd = open(stat_fn, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); 
		cnt = write(stat_fd, &status, sizeof(status_r));
		tasklog('+', "New statusfile created");
		lseek(stat_fd, 0, SEEK_SET);
	}
	
	cnt = read(stat_fd, &status, sizeof(status_r));
	if (cnt != sizeof(status_r)) {
		printf("Error reading status file\n");
		exit(1);
	}
	status.startups++;
	status.laststart = time(NULL);
	status.clients = 1;		/* We are a client ourself */
	s_bbsopen = status.open;
	lseek(stat_fd, 0, SEEK_SET);
	cnt = write(stat_fd, &status, sizeof(status_r));
	if (cnt != sizeof(status_r)) {
		tasklog('?', "$Error rewrite status file\n");
		exit(1);
	}
	close(stat_fd);
}



/*
 *  Writeback the updated status record.
 */
void status_write(void);
void status_write(void)
{
	int	 	d, stat_fd;
	struct tm	*ttm, *ytm;
	time_t		temp;

	temp = time(NULL);
	ttm = localtime(&temp);
	temp = status.daily;	// On a Sparc, first put the time in temp, then pass it to locattime.
	ytm = localtime(&temp);

	/*
	 * If we passed to the next day, zero the today counters 
	 */
	if (ttm->tm_yday != ytm->tm_yday) {
		tasklog('+', "Last days statistics:");
		tasklog('+', "Total clients : %lu", status.today.tot_clt);
		tasklog('+', "Peak clients  : %lu", status.today.peak_clt);
		tasklog('+', "Syntax errors : %lu", status.today.s_error);
		tasklog('+', "Comms errors  : %lu", status.today.c_error);

		memset((char *)&status.today, 0, sizeof(cl_stat));
		status.daily = time(NULL);
		tasklog('+', "Zeroed todays status counters");
	}

	if ((stat_fd = open(stat_fn, O_RDWR)) == -1) {
		tasklog('?', "$Error open statusfile %s", stat_fn);
		return;
	}

	if ((d = lseek(stat_fd, 0, SEEK_SET)) != 0) {
		tasklog('?', "$Error seeking in statusfile");
		return;
	}

	d = write(stat_fd, &status, sizeof(status_r));
	if (d != sizeof(status_r))
		tasklog('?', "$Error writing statusfile, only %d bytes", d);

	/*
	 * CLose the statusfile
	 */
	if (close(stat_fd) != 0)
		tasklog('?', "$Error closing statusfile");
}



/*************************************************************************
 *
 *  Various actions on the statusfile.
 */


/*
 * Check for Zone Mail Hour, return TRUE if it is.
 */
int get_zmh()
{
	struct	tm *l_date; 
	char	sstime[6];
	time_t	Now;

	Now = time(NULL);
	l_date = gmtime(&Now);
	sprintf(sstime, "%02d:%02d", l_date->tm_hour, l_date->tm_min);

	if ((strncmp(sstime, TCFG.zmh_start, 5) >= 0) && (strncmp(sstime, TCFG.zmh_end, 5) < 0)) {
		if (!ZMH) {
			CreateSema((char *)"zmh");
			tasklog('!', "Start of Zone Mail Hour");
			ZMH = TRUE;
		}
	} else {
		if (ZMH) {
			RemoveSema((char *)"zmh");
			tasklog('!', "End of Zone Mail Hour");
			ZMH = FALSE;
		}
	}
	return ZMH;
}



void stat_inc_clients()
{
	status.clients++;
	status.total.tot_clt++;	
	status.today.tot_clt++;
	if (status.clients >= status.total.peak_clt)
		status.total.peak_clt = status.clients;
	if (status.clients >= status.today.peak_clt)
		status.today.peak_clt = status.clients;	

	status_write();
}



void stat_dec_clients()
{
	status.clients--; 
	status_write();
}



void stat_set_open(int op)
{
	if (op) {
		if (!s_bbsopen) {
			tasklog('!', "The bbs is open");
			sem_set((char *)"scanout", TRUE);
		}
	} else {
		if (s_bbsopen) {
			tasklog('!', "The bbs is closed");
		}
	}
	s_bbsopen = status.open = op;
	status_write();
}



void stat_inc_serr()
{
	status.total.s_error++;
	status.today.s_error++;
	status_write();
}



void stat_inc_cerr()
{
	status.total.c_error++;
	status.today.c_error++;
	status_write();
}



char *stat_status()
{
	static char buf[160];

	buf[0] = '\0';
	sprintf(buf, "100:20,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d,%2.2f,%lu;",
		status.start, status.laststart, status.daily,
		status.startups, status.clients, 
		status.total.tot_clt, status.total.peak_clt,
		status.total.s_error, status.total.c_error,
		status.today.tot_clt, status.today.peak_clt,
		status.today.s_error, status.today.c_error,
		status.open, get_zmh(), internet, s_do_inet, Processing, Load, status.sequence);
	return buf;
}



/*
 * Return open status:
 * 0 = open.
 * 1 = closed.
 * 2 = Zone Mail Hour.
 */
int stat_bbs_stat()
{
	if (!status.open) 
		return 1;
	if (get_zmh())
		return 2;
	return 0;
}



/*
 * Get next sequence number
 */
char *getseq(void)
{
	static char	buf[80];

	buf[0] = '\0';
	status.sequence++;
	status_write();
	sprintf(buf, "100:1,%lu;", status.sequence);
	return buf;
}



int sem_set(char *sem, int value)
{
	tasklog('s', "sem_set(%s, %s)", sem, value?"TRUE":"FALSE");

	if (!strcmp(sem, "scanout")) {
		s_scanout = value;
	} else if (!strcmp(sem, "mailout")) {
		s_mailout = value;
	} else if (!strcmp(sem, "mailin")) {
		s_mailin = value;
	} else if (!strcmp(sem, "mbindex")) {
		s_index = value;
	} else if (!strcmp(sem, "newnews")) {
		s_newnews = value;
	} else if (!strcmp(sem, "msglink")) {
		s_msglink = value;
	} else if (!strcmp(sem, "reqindex")) {
		s_reqindex = value;
	} else if (!strcmp(sem, "do_inet")) {
		s_do_inet = value;
	} else {
		return FALSE;
	}
	ptimer = PAUSETIME;
	return TRUE;
}



char *sem_status(char *data)
{
	char		*cnt, *sem;
	static char	buf[40];
	int		value;

	buf[0] = '\0';
	sprintf(buf, "200:1,16;");
	cnt = strtok(data, ",");
	sem = strtok(NULL, ";");

        if (!strcmp(sem, "scanout")) {
		value = s_scanout;
        } else if (!strcmp(sem, "mailout")) {
                value = s_mailout;
        } else if (!strcmp(sem, "mailin")) {
                value = s_mailin;
        } else if (!strcmp(sem, "mbindex")) {
                value = s_index;
        } else if (!strcmp(sem, "newnews")) {
                value = s_newnews;
        } else if (!strcmp(sem, "msglink")) {
                value = s_msglink;
        } else if (!strcmp(sem, "reqindex")) {
                value = s_reqindex;
	} else if (!strcmp(sem, "upsalarm")) {
		value = UPSalarm;
	} else if (!strcmp(sem, "do_inet")) {
	        value = s_do_inet;
        } else {
		tasklog('s', "sem_status(%s) buf=%s", sem, buf);
		return buf;
        }

	sprintf(buf, "100:1,%s;", value ? "1":"0");
	tasklog('s', "sem_status(%s) = %d buf=%s", sem, value, buf);
	return buf;
}



char *sem_create(char *data)
{
	static char     buf[40];
        char    	*cnt, *sem;

        cnt = strtok(data, ",");
        sem = strtok(NULL, ";");
        buf[0] = '\0';
        sprintf(buf, "200:1,16;");

	if (sem_set(sem, TRUE))
		sprintf(buf, "100:0;");

        return buf;
}



char *sem_remove(char *data)
{
        static char     buf[40];
        char    	*cnt, *sem;

        cnt = strtok(data, ",");
        sem = strtok(NULL, ";");
        buf[0] = '\0';
        sprintf(buf, "200:1,16;");

	if (sem_set(sem, FALSE))
		sprintf(buf, "100:0;");

        return buf;
}


