/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Client/Server communications
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:	2:280/2802
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

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "mberrors.h"
#include "clcomm.h"


int		do_quiet = FALSE;	/* Quiet flag			    */
int		show_log = FALSE;	/* Show loglines on screen	    */
int		most_debug = FALSE;	/* Toggle normal/most debugging	    */
char		progname[21];		/* Program name			    */
char		logfile[PATH_MAX];	/* Normal logfile		    */
char		errfile[PATH_MAX];	/* Error logfile		    */
char		mgrfile[PATH_MAX];	/* Area/File- mgr logfile	    */
long		loggrade;		/* Logging grade		    */
pid_t		mypid;			/* Original parent pid if child	    */
unsigned long	lcrc = 0, tcrc = 1;	/* CRC value of logstring	    */
int		lcnt = 0;		/* Same message counter		    */
static char	*pbuff = NULL;
extern char	cpath[108];
extern char	spath[108];



char *xmalloc(size_t size)
{
        char *tmp;

        tmp = malloc(size);
        if (!tmp) 
		abort();
        
        return tmp;
}



char *xstrcpy(char *src)
{
	char	*tmp;

	if (src == NULL) 
		return(NULL);
	tmp = xmalloc(strlen(src)+1);
	strcpy(tmp, src);
	return tmp;
}



char *xstrcat(char *src, char *add)
{
	char	*tmp;
	size_t	size = 0;

	if ((add == NULL) || (strlen(add) == 0))
		return src;
	if (src)
		size = strlen(src);
	size += strlen(add);
	tmp = xmalloc(size + 1);
	*tmp = '\0';
	if (src) {
		strcpy(tmp, src);
		free(src);
	}
	strcat(tmp, add);
	return tmp;
}




void InitClient(char *user, char *myname, char *where, char *log, long loggr, char *err, char *mgr)
{
	if ((getenv("MBSE_ROOT")) == NULL) {
		printf("Could not get the MBSE_ROOT environment variable\n");
		printf("Please set the environment variable ie:\n");
		printf("\"MBSE_ROOT=/opt/mbse; export MBSE_ROOT\"\n\n");
		exit(MBERR_INIT_ERROR);
	}

	sprintf(progname, "%s", myname);
	sprintf(logfile, "%s", log);
	sprintf(errfile, "%s", err);
	sprintf(mgrfile, "%s", mgr);
	loggrade = loggr;

        sprintf(cpath, "%s/tmp/%s%d", getenv("MBSE_ROOT"), progname, getpid());
        sprintf(spath, "%s/tmp/mbtask", getenv("MBSE_ROOT"));

        /*
         * Store my pid in case a child process is forked and wants to do
         * some communications with the mbsed server.
         */
        mypid = getpid();
        if (socket_connect(user, myname, where) == -1) {
                printf("PANIC: cannot access socket\n");
                exit(MBERR_INIT_ERROR);
        }
}



void ExitClient(int errcode)
{
	if (socket_shutdown(mypid) == -1)
		printf("PANIC: unable to shutdown socket\n");
	unlink(cpath);
	fflush(stdout);
	fflush(stdin);

	if (pbuff)
		free(pbuff);

#ifdef MEMWATCH
	mwTerm();
#endif
	exit(errcode);
}



void SockS(const char *format, ...)
{
	char	*out;
	va_list	va_ptr;

	out = calloc(SS_BUFSIZE, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(out, format, va_ptr);
	va_end(va_ptr);

	if (socket_send(out) == 0)
		socket_receive();

	free(out);
}



char *SockR(const char *format, ...)
{
	static char	buf[SS_BUFSIZE];
	char		*out;
	va_list		va_ptr;

	memset(&buf, 0, SS_BUFSIZE);
	out = calloc(SS_BUFSIZE, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(out, format, va_ptr);
	va_end(va_ptr);

	if (socket_send(out) == 0)
		sprintf(buf, "%s", socket_receive());

	free(out);
	return buf;
}



void WriteError(const char *format, ...)
{
	char		*outputstr;
	va_list		va_ptr;
	int		i;

	outputstr = calloc(10240, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outputstr, format, va_ptr);
	va_end(va_ptr);

	for (i = 0; i < strlen(outputstr); i++)
		if (outputstr[i] == '\r' || outputstr[i] == '\n')
			outputstr[i] = ' ';

	if (*outputstr == '$')
		sprintf(outputstr+strlen(outputstr), ": %s", strerror(errno));

	if (strlen(outputstr) > (SS_BUFSIZE - 64)) {
		outputstr[SS_BUFSIZE - 65] = ';';
		outputstr[SS_BUFSIZE - 64] = '\0';
	}
	tcrc = StringCRC32(outputstr);
	if (tcrc == lcrc) {
		lcnt++;
		free(outputstr);
		return;
	} else {
		lcrc = tcrc;
		if (lcnt) {
			lcnt++;
			SockS("ALOG:5,%s,%s,%d,?,Last message repeated %d times;", logfile, progname, mypid, lcnt);
			SockS("ALOG:5,%s,%s,%d,?,Last message repeated %d times;", errfile, progname, mypid, lcnt);
		}
		lcnt = 0;
	}

	SockS("ALOG:5,%s,%s,%d,?,%s;", logfile, progname, mypid, *outputstr == '$' ? outputstr+1 : outputstr);
	SockS("ALOG:5,%s,%s,%d,?,%s;", errfile, progname, mypid, *outputstr == '$' ? outputstr+1 : outputstr);
	free(outputstr);
}



/*
 * Standard system logging
 */
void Syslog(int level, const char *format, ...)
{
	char		*outstr;
	va_list		va_ptr;

	outstr = calloc(10240, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outstr, format, va_ptr);
	va_end(va_ptr);
	Syslogp(level, outstr);
	free(outstr);
}



/*
 * System logging without string formatting.
 */
void Syslogp(int level, char *outstr)
{
    long    mask = 0;
    int	    i, upper;

    upper = isupper(level);
    switch(tolower(level)) {
	case ' ' : mask = DLOG_ALLWAYS;	    break;
	case '?' : mask = DLOG_ERROR;	    break;	
	case '!' : mask = DLOG_ATTENT;	    break;
	case '+' : mask = DLOG_NORMAL;	    break;
	case '-' : mask = DLOG_VERBOSE;	    break;
	case 'a' : mask = DLOG_TCP;	    break;
	case 'b' : mask = DLOG_BBS;	    break;
	case 'c' : mask = DLOG_CHAT;	    break;
	case 'd' : mask = DLOG_DEVIO;	    break;
	case 'e' : mask = DLOG_EXEC;	    break;
	case 'f' : mask = DLOG_FILEFWD;	    break;
	case 'h' : mask = DLOG_HYDRA;	    break;
	case 'i' : mask = DLOG_IEMSI;	    break;
	case 'l' : mask = DLOG_LOCK;	    break;
	case 'm' : mask = DLOG_MAIL;	    break;
	case 'n' : mask = DLOG_NODELIST;    break;
	case 'o' : mask = DLOG_OUTSCAN;	    break;
	case 'p' : mask = DLOG_PACK;	    break;
	case 'r' : mask = DLOG_ROUTE;	    break;
	case 's' : mask = DLOG_SESSION;	    break;
	case 't' : mask = DLOG_TTY;	    break;
	case 'x' : mask = DLOG_XMODEM;	    break;
	case 'z' : mask = DLOG_ZMODEM;	    break;
    }

    if (((loggrade | DLOG_ALLWAYS | DLOG_ERROR) & mask) == 0)
	return;

    /*
     * Don't log uppercase debug levels when most_debug is FALSE
     */
    if (upper && !most_debug)
	return;

    for (i = 0; i < strlen(outstr); i++)
	if (outstr[i] == '\r' || outstr[i] == '\n')
	    outstr[i] = ' ';
    if (strlen(outstr) > (SS_BUFSIZE - 64))
	outstr[SS_BUFSIZE - 64] = '\0';

    tcrc = StringCRC32(outstr);
    if (tcrc == lcrc) {
	lcnt++;
	return;
    } else {
	lcrc = tcrc;
	if (lcnt) {
	    lcnt++;
	    SockS("ALOG:5,%s,%s,%d,%c,Last message repeated %d times;", logfile, progname, mypid, level, lcnt);
	}
	lcnt = 0;
    }

    if (show_log)
	printf("%c %s\n", level, outstr);

    if (*outstr == '$')
	SockS("ALOG:5,%s,%s,%d,%c,%s: %s;", logfile, progname, mypid, level, outstr+1, strerror(errno));
    else
	SockS("ALOG:5,%s,%s,%d,%c,%s;", logfile, progname, mypid, level, outstr);
}



void Mgrlog(const char *format, ...)
{
    char    *outstr;
    va_list va_ptr;
    int	    i;

    outstr = calloc(10240, sizeof(char));

    va_start(va_ptr, format);
    vsprintf(outstr, format, va_ptr);
    va_end(va_ptr);

    for (i = 0; i < strlen(outstr); i++)
	if (outstr[i] == '\r' || outstr[i] == '\n')
	    outstr[i] = ' ';
    if (strlen(outstr) > (SS_BUFSIZE - 64))
	outstr[SS_BUFSIZE - 64] = '\0';
    
    SockS("ALOG:5,%s,%s,%d,+,%s;", mgrfile, progname, mypid, outstr);
    Syslogp('+', outstr);
    free(outstr);
}



void IsDoing(const char *format, ...)
{
	char	*outputstr;
	va_list	va_ptr;

	outputstr = calloc(SS_BUFSIZE, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outputstr, format, va_ptr);
	va_end(va_ptr);

	SockS("ADOI:2,%d,%s;", mypid, outputstr);
	free(outputstr);
}



void RegTCP(void)
{
    SockS("ATCP:1,%d;", mypid);
}



void SetTTY(char *tty)
{
	SockS("ATTY:2,%d,%s;", mypid, tty);
}



void UserCity(pid_t pid, char *user, char *city)
{
        SockS("AUSR:3,%d,%s,%s;", pid, user, city);
}



void DoNop()
{
	SockS("GNOP:1,%d;", mypid);
}



static time_t	nop = 0;

/*
 * This function can be called very often but will only send once a minute
 * a NOP to the server. This is a simple solution to keep server trafic low.
 */
void Nopper(void)
{
	time_t	now;

	now = time(NULL);
	if (((time_t)now - (time_t)nop) > 60) {
		nop = now;
		SockS("GNOP:1,%d;", mypid);
	}
}



/*
 * Set new alarmtime for Client/Server connection,
 * if zero set the default time.
 */
void Altime(int altime)
{
	if (altime)
		SockS("ATIM:2,%d,%d;", mypid, altime);
	else
		SockS("ADEF:1,%d;", mypid);
}



unsigned long sequencer()
{
	char		*buf, *res;
	unsigned long	seq = 0;

	buf = calloc(SS_BUFSIZE, sizeof(char));
	sprintf(buf, "SSEQ:0;");

	if (socket_send(buf) == 0) {
		free(buf);
		buf = socket_receive();
		res = strtok(buf, ",");
		res = strtok(NULL, ";");
		seq = atol(res);
	}

	return seq;
}



char *printable(char *s, int l)
{
	int	len;
	char	*p;

	if (pbuff) 
		free(pbuff);
	pbuff=NULL;

	if (s == NULL) 
		return (char *)"(null)";

	if (l > 0) 
		len=l;
	else if (l == 0) 
		len=strlen(s);
	else {
		len=strlen(s);
		if (len > -l) 
			len=-l;
	}

	pbuff=(char*)xmalloc(len*4+1);
	p=pbuff;
	while (len--) {
		if (*(unsigned char*)s >= ' ') 
			*p++=*s;
		else switch (*s) {
			case '\\': *p++='\\'; *p++='\\'; break;
			case '\r': *p++='\\'; *p++='r'; break;
			case '\n': *p++='\\'; *p++='n'; break;
			case '\t': *p++='\\'; *p++='t'; break;
			case '\b': *p++='\\'; *p++='b'; break;
			default:   sprintf(p,"\\%03o",*s); p+=4; break;
		}
		s++;
	}
	*p='\0';
	return pbuff;
}



char *printablec(char c)
{
	return printable(&c,1);
}


