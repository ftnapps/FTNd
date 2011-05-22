/*****************************************************************************
 *
 * Purpose ...............: Keep track of server status 
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "callstat.h"
#include "outstat.h"
#include "taskibc.h"
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
int			tosswait   = TOSSWAIT_TIME;
extern int		UPSalarm;
extern int		UPSdown;
extern int		ptimer;
extern int		rescan;


extern struct taskrec	TCFG;
extern int		internet;
extern int		ZMH;


typedef struct {
	int		tot_clt;	/* Total client connects 	*/
	int		peak_clt;	/* Peak simultaneous tot_cltes	*/
	int		s_error;	/* Syntax errors from clients	*/
	int		c_error;	/* Comms errors from clients	*/
} cl_stat;


typedef struct {
	int		start;		/* Start date/time		*/
	int		laststart;	/* Last start date/time		*/
	int		daily;		/* Last daily update		*/
	int		startups;	/* Total starts			*/
	int		clients;	/* Connected clients		*/
	cl_stat		total;		/* Total statistics		*/
	cl_stat		today;		/* Todays statistics		*/
	unsigned	open	: 1;	/* Is BBS open			*/
	unsigned int	sequence;	/* Sequencer counter		*/

	unsigned int	mkbrcvd;	/* MIB mailer Kbytes received	*/
	unsigned int	mkbsent;	/* MIB mailer Kbytes sent	*/
	unsigned int	msessin;	/* MIB mailer inbound sessions	*/
	unsigned int	msessout;	/* MIB mailer outbound sessions	*/
	unsigned int	msesssecure;	/* MIB mailer secure sessions	*/
	unsigned int	msessunsec;	/* MIB mailer unsecure sessions	*/
	unsigned int	msessbad;	/* MIB mailer bad sessions	*/
	unsigned int	mftsc;		/* MIB mailer FTSC sessions	*/
	unsigned int	myoohoo;	/* MIB mailer YooHoo sessions	*/
	unsigned int	memsi;		/* MIB mailer EMSI sessions	*/
	unsigned int	mbinkp;		/* MIB mailer Binkp sessions	*/
	unsigned int	mfreqs;		/* MIB mailer file requests	*/

	unsigned int	tmsgsin;	/* MIB tosser messages in	*/
	unsigned int	tmsgsout;	/* MIB tosser messages out	*/
	unsigned int	tmsgsbad;	/* MIB tosser messages bad	*/
	unsigned int	tmsgsdupe;	/* MIB tosser messages dupe	*/

	unsigned int	tnetin;		/* MIB tosser netmail in	*/
	unsigned int	tnetout;	/* MIB tosser netmail out	*/
	unsigned int	tnetbad;	/* MIB tosser netmail bad	*/

	unsigned int	temailin;	/* MIB tosser email in		*/
	unsigned int	temailout;	/* MIB tosser email out		*/
	unsigned int	temailbad;	/* MIB tosser email bad		*/

	unsigned int	techoin;	/* MIB tosser echomail in	*/
	unsigned int	techoout;	/* MIB tosser echomail out	*/
	unsigned int	techobad;	/* MIB tosser echomail bad	*/
	unsigned int	techodupe;	/* MIB tosser echomail dupe	*/

	unsigned int	tnewsin;	/* MIB tosser news in		*/
	unsigned int	tnewsout;	/* MIB tosser news out		*/
	unsigned int	tnewsbad;	/* MIB tosser news bad		*/
	unsigned int	tnewsdupe;	/* MIB tosser news dupe		*/

	unsigned int	tfilesin;	/* MIB tosser files in		*/
	unsigned int	tfilesout;	/* MIB tosser files out		*/
	unsigned int	tfilesbad;	/* MIB tosser bad files		*/
	unsigned int	tfilesdupe;	/* MIB tosser dupe files	*/

	unsigned int	tfilesmagic;	/* MIB tosser files magics	*/
	unsigned int	tfileshatched;	/* MIB tosser files hatched	*/

	unsigned int	ooutsize;	/* MIB outbound size		*/

	unsigned int	bsessions;	/* MIB BBS sessions		*/
	unsigned int	bminutes;	/* MIB BBS minutes used		*/
	unsigned int	bposted;	/* MIB BBS msgs posted		*/
	unsigned int	buploads;	/* MIB BBS file uploads		*/
	unsigned int	bkbupload;	/* MIB BBS kbytes uploaded	*/
	unsigned int	bdownloads;	/* MIB BBS file downloads	*/
	unsigned int	bkbdownload;	/* MIB BBS kbytes downloaded	*/
	unsigned int	bchats;		/* MIB BBS chatsessions		*/
	unsigned int	bchatminutes;	/* MIB BBS chat minutes		*/
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
    size_t  cnt;
    int	    stat_fd;

    snprintf(stat_fn, PATH_MAX, "%s/var/status.mbsed", getenv("MBSE_ROOT"));

    /*
     * First check if this is the very first time we start the show.
     * If so, we generate an empty status file with only the start
     * date in it.
     */
    memset((char *)&status, 0, sizeof(status_r));
    stat_fd = open(stat_fn, O_RDWR);
    if (stat_fd == -1) {
	status.start = (int)time(NULL);
	status.daily = (int)time(NULL);
	status.sequence = (unsigned int)time(NULL);
	stat_fd = open(stat_fn, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); 
	cnt = write(stat_fd, &status, sizeof(status_r));
	Syslog('+', "New statusfile created");
	lseek(stat_fd, 0, SEEK_SET);
    }
	
    cnt = read(stat_fd, &status, sizeof(status_r));
    if ((cnt != sizeof(status_r)) && (cnt < 50))  {
	printf("Error reading status file\n");
	exit(MBERR_INIT_ERROR);
    }
    status.startups++;
    status.laststart = (int)time(NULL);
    status.clients = 1;		/* We are a client ourself */
    s_bbsopen = status.open;
    lseek(stat_fd, 0, SEEK_SET);
    cnt = write(stat_fd, &status, sizeof(status_r));
    if (cnt != sizeof(status_r)) {
	Syslog('?', "$Error rewrite status file\n");
	exit(MBERR_INIT_ERROR);
    }
    close(stat_fd);
}



/*
 *  Writeback the updated status record.
 */
void status_write(void);
void status_write(void)
{
    int	 	d, stat_fd, yday;
    struct tm	ttm;
    time_t	temp;

    temp = time(NULL);
    localtime_r(&temp, &ttm);
    yday = ttm.tm_yday;
    temp = (time_t)status.daily;
    localtime_r(&temp, &ttm);

    /*
     * If we passed to the next day, zero the today counters 
     */
    if (yday != ttm.tm_yday) {
	Syslog('+', "Last days statistics:");
	Syslog('+', "Total clients : %lu", status.today.tot_clt);
	Syslog('+', "Peak clients  : %lu", status.today.peak_clt);
	Syslog('+', "Syntax errors : %lu", status.today.s_error);
	Syslog('+', "Comms errors  : %lu", status.today.c_error);

	memset((char *)&status.today, 0, sizeof(cl_stat));
	status.daily = time(NULL);
	Syslog('+', "Zeroed todays status counters");
    }

    if ((stat_fd = open(stat_fn, O_RDWR)) == -1) {
	Syslog('?', "$Error open statusfile %s", stat_fn);
	return;
    }

    if ((d = lseek(stat_fd, 0, SEEK_SET)) != 0) {
	Syslog('?', "$Error seeking in statusfile");
	return;
    }

    d = write(stat_fd, &status, sizeof(status_r));
    if (d != sizeof(status_r))
	Syslog('?', "$Error writing statusfile, only %d bytes", d);

    /*
     * CLose the statusfile
     */
    if (close(stat_fd) != 0)
	Syslog('?', "$Error closing statusfile");
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
    struct tm	l_date; 
    char	sstime[6];
    time_t	Now;

    Now = time(NULL);
    gmtime_r(&Now, &l_date);
    snprintf(sstime, 6, "%02d:%02d", l_date.tm_hour, l_date.tm_min);

    if ((strncmp(sstime, TCFG.zmh_start, 5) >= 0) && (strncmp(sstime, TCFG.zmh_end, 5) < 0)) {
	if (!ZMH) {
	    CreateSema((char *)"zmh");
	    Syslog('!', "Start of Zone Mail Hour");
	    ZMH = TRUE;
	}
    } else {
	if (ZMH) {
	    RemoveSema((char *)"zmh");
	    Syslog('!', "End of Zone Mail Hour");
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
	    Syslog('!', "The bbs is open");
	    sem_set((char *)"scanout", TRUE);
	}
    } else {
	if (s_bbsopen) {
	    Syslog('!', "The bbs is closed");
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



void stat_status_r(char *buf)
{
    int		i, srvcnt = 0, chncnt = 0, usrcnt = 0;

    for (i = 0; i < MAXIBC_SRV; i++)
	if (strlen(srv_list[i].server))
	    srvcnt++;
    for (i = 0; i < MAXIBC_CHN; i++)
	if (strlen(chn_list[i].server))
	    chncnt++;
    for (i = 0; i < MAXIBC_USR; i++)
	if (strlen(usr_list[i].server))
	    usrcnt++;
    snprintf(buf, SS_BUFSIZE, "100:23,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%2.2f,%u,%d,%d,%d;",
	status.start, status.laststart, status.daily,
	status.startups, status.clients, 
	status.total.tot_clt, status.total.peak_clt,
	status.total.s_error, status.total.c_error,
	status.today.tot_clt, status.today.peak_clt,
	status.today.s_error, status.today.c_error,
	status.open, get_zmh(), internet, s_do_inet, Processing, Load, status.sequence,
	srvcnt, chncnt, usrcnt);
    return;
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
void getseq_r(char *buf)
{
    status.sequence++;
    status_write();
    snprintf(buf, 80, "100:1,%u;", status.sequence);
    return;
}



unsigned int gettoken(void)
{
    status.sequence++;
    status_write();
    return status.sequence;
}



int sem_set(char *sem, int value)
{
    if (!strcmp(sem, "scanout")) {
	s_scanout = value;
	if (value)
	    rescan = TRUE;
    } else if (!strcmp(sem, "mailout")) {
	s_mailout = value;
    } else if (!strcmp(sem, "mailin")) {
	s_mailin = value;
	if (value)
	    tosswait = TOSSWAIT_TIME;
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



void sem_status_r(char *data, char *buf)
{
    char	*sem;
    int		value;

    snprintf(buf, 40, "200:1,16;");
    strtok(data, ",");
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
    } else if (!strcmp(sem, "upsdown")) {
	value = UPSdown;
    } else if (!strcmp(sem, "do_inet")) {
	value = s_do_inet;
    } else {
	Syslog('s', "sem_status(%s) buf=%s", sem, buf);
	return;
    }

    snprintf(buf, 40, "100:1,%s;", value ? "1":"0");
    return;
}



void sem_create_r(char *data, char *buf)
{
    char   *sem;

    strtok(data, ",");
    sem = xstrcpy(strtok(NULL, ";"));

    if (sem_set(sem, TRUE))
	snprintf(buf, 40, "100:0;");
    else
	snprintf(buf, 40, "200:1,16;");

    free(sem);
    return;
}



void sem_remove_r(char *data, char *buf)
{
    char    *sem;

    strtok(data, ",");
    sem = xstrcpy(strtok(NULL, ";"));

    if (sem_set(sem, FALSE))
	snprintf(buf, 40, "100:0;");
    else
	snprintf(buf, 40, "200:1,16;");

    free(sem);
    return;
}



void mib_set_mailer(char *data)
{
    unsigned int	kbrcvd, kbsent, direction, state, type, freqs;

    Syslog('m', "MIB set mailer %s", data);
    strtok(data, ",");
    kbrcvd    = atoi(strtok(NULL, ","));
    kbsent    = atoi(strtok(NULL, ","));
    direction = atoi(strtok(NULL, ","));
    state     = atoi(strtok(NULL, ","));
    type      = atoi(strtok(NULL, ","));
    freqs     = atoi(strtok(NULL, ";"));

    status.mkbrcvd += kbrcvd;
    status.mkbsent += kbsent;
    if (direction)
	status.msessout++;
    else
	status.msessin++;
    switch (state) {
	case 0:	status.msesssecure++;	break;
	case 1:	status.msessunsec++;	break;
	case 2:	status.msessbad++;	break;
    }
    switch (type) {
	case 1: status.mftsc++;		break;
	case 2: status.myoohoo++;	break;
	case 3: status.memsi++;		break;
	case 4: status.mbinkp++;	break;
    }
    status.mfreqs += freqs;
    Syslog('m', "MIB mailer: rcvd=%d sent=%d in=%d out=%d sec=%d unsec=%d bad=%d ftsc=%d yoohoo=%d emsi=%d binkp=%d freq=%d", 
	    status.mkbrcvd, status.mkbsent, status.msessin,
	    status.msessout, status.msesssecure, status.msessunsec, status.msessbad, 
	    status.mftsc, status.myoohoo, status.memsi, status.mbinkp, status.mfreqs);

    status_write();
}



void mib_get_mailer_r(char *buf)
{

    snprintf(buf, 127, "100:12,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;",
	    status.mkbrcvd, status.mkbsent, status.msessin,
	    status.msessout, status.msesssecure, status.msessunsec, status.msessbad,
	    status.mftsc, status.myoohoo, status.memsi, status.mbinkp, status.mfreqs);
    return;
}



void mib_set_netmail(char *data)
{
    unsigned int	in, out, bad;

    Syslog('m', "MIB set netmail %s", data);
    strtok(data, ",");
    in  = atoi(strtok(NULL, ","));
    out = atoi(strtok(NULL, ","));
    bad = atoi(strtok(NULL, ";"));

    status.tmsgsin += in;
    status.tnetin += in;
    status.tmsgsout += out;
    status.tnetout += out;
    status.tmsgsbad += bad;
    status.tnetbad += bad;
    Syslog('m', "MIB netmail: in=%d out=%d bad=%d in=%d out=%d bad=%d", status.tmsgsin, status.tmsgsout, status.tmsgsbad,
	    status.tnetin, status.tnetout, status.tnetbad);

    status_write();
}



void mib_get_netmail_r(char *buf)
{
    snprintf(buf, 127, "100:3,%d,%d,%d;", status.tnetin, status.tnetout, status.tnetbad);
    return;
}



void mib_set_email(char *data)
{
    unsigned int        in, out, bad;
    
    Syslog('m', "MIB set email %s", data);
    strtok(data, ",");
    in  = atoi(strtok(NULL, ","));
    out = atoi(strtok(NULL, ","));
    bad = atoi(strtok(NULL, ";"));

    status.tmsgsin += in;
    status.temailin += in;
    status.tmsgsout += out;
    status.temailout += out;
    status.tmsgsbad += bad;
    status.temailbad += bad;

    Syslog('m', "MIB email: in=%d out=%d bad=%d in=%d out=%d bad=%d", status.tmsgsin, status.tmsgsout, status.tmsgsbad,
	    status.temailin, status.temailout, status.temailbad);

    status_write();
}



void mib_get_email_r(char *buf)
{
    snprintf(buf, 127, "100:3,%d,%d,%d;", status.temailin, status.temailout, status.temailbad);
    return;
}



void mib_set_news(char *data)
{
    unsigned int        in, out, bad, dupe;

    Syslog('m', "MIB set news %s", data);
    strtok(data, ",");
    in   = atoi(strtok(NULL, ","));
    out  = atoi(strtok(NULL, ","));
    bad  = atoi(strtok(NULL, ","));
    dupe = atoi(strtok(NULL, ";"));


    status.tmsgsin += in;
    status.tnewsin += in;
    status.tmsgsout += out;
    status.tnewsout += out;
    status.tmsgsbad += bad;
    status.tnewsbad += bad;
    status.tmsgsdupe += dupe;
    status.tnewsdupe += dupe;
    
    Syslog('m', "MIB news: in=%d out=%d bad=%d dupe=%d in=%d out=%d bad=%d dupe=%d", 
	    status.tmsgsin, status.tmsgsout, status.tmsgsbad, status.tmsgsdupe,
	    status.tnewsin, status.tnewsout, status.tnewsbad, status.tnewsdupe);

    status_write();
}



void mib_get_news_r(char *buf)
{
    snprintf(buf, 127, "100:4,%d,%d,%d,%d;", status.tnewsin, status.tnewsout, status.tnewsbad, status.tnewsdupe);
    return;
}



void mib_set_echo(char *data)
{
    unsigned int        in, out, bad, dupe;

    Syslog('m', "MIB set echo %s", data);
    strtok(data, ",");
    in   = atoi(strtok(NULL, ","));
    out  = atoi(strtok(NULL, ","));
    bad  = atoi(strtok(NULL, ","));
    dupe = atoi(strtok(NULL, ";"));

    status.tmsgsin += in;
    status.techoin += in;
    status.tmsgsout += out;
    status.techoout += out;
    status.tmsgsbad += bad;
    status.techobad += bad;
    status.tmsgsdupe += dupe;
    status.techodupe += dupe;

    Syslog('m', "MIB echo: in=%d out=%d bad=%d dupe=%d in=%d out=%d bad=%d dupe=%d", 
	    status.tmsgsin, status.tmsgsout, status.tmsgsbad, status.tmsgsdupe,
	    status.techoin, status.techoout, status.techobad, status.techodupe);

    status_write();
}



void mib_get_echo_r(char *buf)
{
    snprintf(buf, 127, "100:4,%d,%d,%d,%d;", status.techoin, status.techoout, status.techobad, status.techodupe);
    return;
}



void mib_get_tosser_r(char *buf)
{
    snprintf(buf, 127, "100:4,%d,%d,%d,%d;", status.tmsgsin, status.tmsgsout, status.tmsgsbad, status.tmsgsdupe);
    return;
}



void mib_set_files(char *data)
{
    unsigned int        in, out, bad, dupe, magics, hatched;

    Syslog('m', "MIB set files %s", data);
    strtok(data, ",");
    in      = atoi(strtok(NULL, ","));
    out     = atoi(strtok(NULL, ","));
    bad     = atoi(strtok(NULL, ","));
    dupe    = atoi(strtok(NULL, ","));
    magics  = atoi(strtok(NULL, ","));
    hatched = atoi(strtok(NULL, ";"));

    status.tfilesin += in;
    status.tfilesout += out;
    status.tfilesbad += bad;
    status.tfilesdupe += dupe;
    status.tfilesmagic += magics;
    status.tfileshatched += hatched;

    Syslog('m', "MIB files: in=%d out=%d bad=%d dupe=%d magic=%d hatch=%d", 
	    status.tfilesin, status.tfilesout, status.tfilesbad, status.tfilesdupe,
	    status.tfilesmagic, status.tfileshatched);

    status_write();
}



void mib_get_files_r(char *buf)
{
    snprintf(buf, 127, "100:6,%d,%d,%d,%d,%d,%d;", status.tfilesin, status.tfilesout, status.tfilesbad, status.tfilesdupe,
	    status.tfilesmagic, status.tfileshatched);
    return;
}



void mib_set_outsize(unsigned int size)
{
    status.ooutsize = size;
    Syslog('m', "MIB outbound: %d", status.ooutsize);

    status_write();
}



void mib_get_outsize_r(char *buf)
{
    snprintf(buf, 127, "100:1,%d;", status.ooutsize);
    return;
}



void mib_set_bbs(char *data)
{
    unsigned int	sessions, minutes, posted, uploads, kbupload, downloads, kbdownload, chats, chatminutes;

    Syslog('m', "MIB set bbs %s", data);
    strtok(data, ",");
    sessions	= atoi(strtok(NULL, ","));
    minutes	= atoi(strtok(NULL, ","));
    posted	= atoi(strtok(NULL, ","));
    uploads	= atoi(strtok(NULL, ","));
    kbupload	= atoi(strtok(NULL, ","));
    downloads	= atoi(strtok(NULL, ","));
    kbdownload	= atoi(strtok(NULL, ","));
    chats	= atoi(strtok(NULL, ","));
    chatminutes	= atoi(strtok(NULL, ";"));
					    
    status.bsessions += sessions;
    status.bminutes += minutes;
    status.bposted += posted;
    status.buploads += uploads;
    status.bkbupload += kbupload;
    status.bdownloads += downloads;
    status.bkbdownload += kbdownload;
    status.bchats += chats;
    status.bchatminutes += chatminutes;
								    
    Syslog('m', "MIB bbs: sess=%d mins=%d posted=%d upls=%d kbup=%d downs=%d kbdown=%d chat=%d chatmins=%d",
	    status.bsessions, status.bminutes, status.bposted, status.buploads, status.bkbupload,
	    status.bdownloads, status.bkbdownload, status.bchats, status.bchatminutes);

    status_write();
}



void mib_get_bbs_r(char *buf)
{
    snprintf(buf, 127, "100:9,%d,%d,%d,%d,%d,%d,%d,%d,%d;", 
	    status.bsessions, status.bminutes, status.bposted, status.buploads, status.bkbupload,
	    status.bdownloads, status.bkbdownload, status.bchats, status.bchatminutes);
    return;
}


