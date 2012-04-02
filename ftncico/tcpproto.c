/*****************************************************************************
 *
 * $Id: tcpproto.c,v 1.19 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

/*
        contributed by Stanislav Voronyi <stas@uanet.kharkov.ua>
*/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "config.h"
#include "emsi.h"
#include "lutil.h"
#include "openfile.h"
#include "filelist.h"
#include "tcpproto.h"


#define TCP_CMD	    0x87
#define TCP_DATA    0xe1
#define	TCP_BLKSTRT 0xc6
#define	TCP_BLKEND  0x6c
#define TCP_BLKSIZE 2048
#define	TCP_DATSIZE 1024


static FILE 	*fout;
static FILE 	*in;
static char 	txbuf[TCP_BLKSIZE];
static char 	rxbuf[TCP_BLKSIZE];
static int  	rx_type;
static int	sbytes;
struct timeval	starttime, endtime;
struct timezone	tz;
static off_t	rxbytes;

static int	sendtfile(char *,char *);
static int	finsend(void);
static int	receivefile(char *,time_t,off_t);
static int	resync(off_t);
static int	closeit(int);
static int	tcp_sblk(char *,int,int);
static int  	tcp_rblk(char *,int *);
static int  	getsync(void);

extern unsigned int	sentbytes;
extern unsigned int	rcvdbytes;
extern char		*ttystat[];


int tcpsndfiles(file_list *lst)
{
    int		rc = 0, maxrc = 0;
    file_list	*tmpf;

    Syslog('+', "TCP: start send files");

    if (getsync()) {
	WriteError("TCP: can't get synchronization");
	return MBERR_FTRANSFER;
    }

    for (tmpf = lst; tmpf && (maxrc == 0); tmpf = tmpf->next) {
	if (tmpf->remote) {
	    rc = sendtfile(tmpf->local,tmpf->remote);
	    rc = abs(rc);
	    if (rc > maxrc) 
		maxrc=rc;
	    if (rc == 0) 
		execute_disposition(tmpf);
	} else
	    if (maxrc == 0)
		execute_disposition(tmpf);
    }

    if (maxrc < 2) {
	rc = finsend();
	rc = abs(rc);
    }

    if (rc > maxrc) 
	maxrc=rc;

    if (rc) {
	WriteError("TCP: send error: rc=%d",maxrc);
	return MBERR_FTRANSFER;
    } 
    
    Syslog('+', "TCP: send files completed");
    return 0;
}



int tcprcvfiles(void)
{
    int	    rc, bufl;
    int	    filesize, filetime;
    char    *filename = NULL, *p;	

    Syslog('+', "TCP: start receive files");
    if (getsync()) {
	WriteError("TCP: can't get synchronization");
	return MBERR_FTRANSFER;
    }

next:
    if ((rc = tcp_rblk(rxbuf, &bufl)) == 0) {
	if (strncmp(rxbuf, "SN", 2) == 0) {
	    rc = tcp_sblk((char *)"RN", 2, TCP_CMD);
	    return rc;
	} else if (*rxbuf == 'S') {
	    p = strchr(rxbuf+2, ' ');
	    if (p != NULL)
		*p=0;
	    else
		return 1;
	    filename = xstrcpy(rxbuf+2);
	    p++;
	    filesize = strtol(p, &p, 10);
	    filetime = strtol(++p, (char **)NULL, 10);
	} else 
	    return rc==0?1:rc;

	if (strlen(filename) && filesize && filetime) {
	    rc = receivefile(filename,filetime,filesize);
	    if (filename)
		free(filename);
	    filename = NULL;
	}

	if (fout) {
	    if (closeit(0))
		WriteError("TCP: error closing file");
	    (void)tcp_sblk((char *)"FERROR",6,TCP_CMD);
	} else
	    goto next;
    }

    if (rc) {
	WriteError("TCP: receive error: rc=%d", rc);
	return MBERR_FTRANSFER;
    }

    Syslog('+', "TCP: receive files completed");
    return 0;
}



static int sendtfile(char *ln, char *rn)
{
    int		    rc=0;
    struct stat	    st;
    struct flock    fl;
    int		    bufl, sverr;
    int		    offset;

    fl.l_type = F_RDLCK;
    fl.l_whence = 0;
    fl.l_start = 0L;
    fl.l_len = 0L;

    if ((in = fopen(ln,"r")) == NULL) {
	sverr = errno;
	if ((sverr == ENOENT) || (sverr == EINVAL)) {
	    Syslog('+', "TCP: file %s doesn't exist, removing", MBSE_SS(ln));
	    return 0;
	} else {
	    WriteError("$TCP: can't open file %s, skipping", MBSE_SS(ln));
	    return 1;
	}
    }

    if (fcntl(fileno(in), F_SETLK, &fl) != 0) {
	WriteError("$TCP: can't lock file %s, skipping", MBSE_SS(ln));
	fclose(in);
	return 1;
    }

    if (stat(ln, &st) != 0) {
	WriteError("$TCP: can't access \"%s\", skipping", MBSE_SS(ln));
	fclose(in);
	return 1;
    }
	
    if (st.st_size > 0) {
	Syslog('+', "TCP: send \"%s\" as \"%s\"", MBSE_SS(ln), MBSE_SS(rn));
	Syslog('+', "TCP: size %lu bytes, dated %s", (unsigned int)st.st_size, date(st.st_mtime));
	gettimeofday(&starttime, &tz);
    } else {
	Syslog('+', "TCP: file \"%s\" has 0 size, skiped",ln);
	return 0;
    }

    snprintf(txbuf,TCP_BLKSIZE, "S %s %u %u",rn,(unsigned int)st.st_size,(unsigned int)st.st_mtime+(unsigned int)(st.st_mtime%2));
    bufl = strlen(txbuf);
    rc = tcp_sblk(txbuf, bufl, TCP_CMD);
    rc = tcp_rblk(rxbuf, &bufl);

    if (strncmp(rxbuf,"RS",2) == 0) {
	Syslog('+', "TCP: file %s already received, skipping",rn);
	return 0;
    } else if (strncmp(rxbuf,"RN",2) == 0) {
	Syslog('+', "TCP: remote refused file, aborting",rn);
	return 2;
    } else if (strncmp(rxbuf,"ROK",3) == 0) {
	if (bufl > 3 && rxbuf[3]==' ') {
	    offset = strtol(rxbuf+4,(char **)NULL,10);
	    if (fseek(in,offset,SEEK_SET) != 0) {
		WriteError("$TCP: can't seek offset %ld in file %s", offset, ln);
		return 1;
	    }
	} else
	    offset = 0;
    } else
	return rc;

    while ((bufl = fread(&txbuf, 1, TCP_DATSIZE, in)) != 0) {
	if ((rc = tcp_sblk(txbuf, bufl, TCP_DATA)) > 0)
	    break;
    }

    fclose(in);
    if (rc == 0){
	strcpy(txbuf, "EOF");
	rc = tcp_sblk(txbuf, 3, TCP_CMD);
	rc = tcp_rblk(rxbuf, &bufl);
    }

    if (rc == 0 && strncmp(rxbuf,"FOK",3) == 0) {
	gettimeofday(&endtime, &tz);
	Syslog('a', "st_size %d, offset %d",st.st_size,offset);
	Syslog('+', "TCP: OK %s", transfertime(starttime, endtime, st.st_size-offset, TRUE));
	sentbytes += (unsigned int)st.st_size - offset;
	return 0;
    } else if(strncmp(rxbuf,"FERROR",6) == 0){
	WriteError("TCP: remote file error",ln);
	return rc==0?1:rc;
    } else
	return rc==0?1:rc;
}



static int resync(off_t off)
{
    snprintf(txbuf, TCP_BLKSIZE, "ROK %d",(int)off);
    return 0;
}



static int closeit(int success)
{
    int rc;

    rc = closefile();
    fout = NULL;
    sbytes = rxbytes - sbytes;
    gettimeofday(&endtime, &tz);

    if (success)
	Syslog('+', "TCP: OK %s", transfertime(starttime, endtime, sbytes, FALSE));
    else
	Syslog('+', "TCP: dropped after %ld bytes", sbytes);
    rcvdbytes += sbytes;
    return rc;
}



static int finsend(void)
{
    int	rc,bufl;

    rc = tcp_sblk((char *)"SN",2,TCP_CMD);
    if(rc) 
	return rc;

    rc = tcp_rblk(rxbuf,&bufl);
    if (strncmp(rxbuf, "RN", 2) == 0)
	return rc;
    else
	return 1;
}



static int receivefile(char *fn, time_t ft, off_t fs)
{
    int	    rc, bufl;

    Syslog('+', "TCP: receive \"%s\" (%lu bytes) dated %s",fn,fs,date(ft));
    strcpy(txbuf,"ROK");
    fout = openfile(fn, ft, fs, &rxbytes, resync);
    gettimeofday(&starttime, &tz);
    sbytes = rxbytes;

    if (fs == rxbytes) {
	Syslog('+', "TCP: skipping %s", fn);
	fout = NULL;
	rc = tcp_sblk((char *)"RS",2,TCP_CMD);
	return rc;
    }

    if (!fout)
	return 1;

    bufl = strlen(txbuf);
    rc = tcp_sblk(txbuf,bufl,TCP_CMD);

    while ((rc = tcp_rblk(rxbuf, &bufl)) == 0) {
	if (rx_type == TCP_CMD)
	    break;
	if (fwrite(rxbuf, 1, bufl, fout) != bufl)
	    break;
	rxbytes += bufl;
    }

    if (rc) 
	return rc;

    if (rx_type == TCP_CMD && bufl == 3 && strncmp(rxbuf,"EOF",3) == 0) {
	if (ftell(fout) == fs) {
	    closeit(1);
	    rc = tcp_sblk((char *)"FOK",3,TCP_CMD);
	    return rc;
	} else
	    return 1;
    } else	
	return 1;		
}



static int tcp_sblk(char *buf, int len, int typ)
{
    Nopper();
    if (typ == TCP_CMD)
	Syslog('a', "tcp_sblk: cmd: %s", buf);
    else
	Syslog('a', "tcp_sblk: data: %d bytes", len);

    PUTCHAR(TCP_BLKSTRT);
    PUTCHAR(typ);
    PUTCHAR((len >> 8) & 0x0ff);
    PUTCHAR(len & 0x0ff);
    PUT(buf, len);
    PUTCHAR(TCP_BLKEND);
    FLUSHOUT();

    if (tty_status)
	WriteError("TCP: send error: %s", ttystat[tty_status]);
    return tty_status;
}



static int tcp_rblk(char *buf, int *len)
{
    int	c;

    *len = 0;

    /*
     *  Wait up to 3 minutes for the header
     */
    c = GETCHAR(180);
    if (tty_status)
	goto to;
    if (c != TCP_BLKSTRT) {
	WriteError("tcp_rblk: got %d instead of block header", c);
	return c;
    }

    /*
     *  Get block type
     */
    c = GETCHAR(120);
    if (tty_status)
	goto to;
    rx_type = c;
    if (c != TCP_CMD && c != TCP_DATA) {
	WriteError("tcp_rblk: got %d character instead of DATA/CMD", c);
	return c;
    }

    /*
     *  Get block length
     */
    c = GETCHAR(120);
    if (tty_status)
	goto to;
    *len = c << 8;
    c = GETCHAR(120);
    if (tty_status)
	goto to;
    *len += c;

    if (*len > TCP_BLKSIZE) {
	WriteError("TCP: remote sends too large block: %d bytes", len);
	return 1;
    }

    /*
     *  Get actual data block
     */
    if (*len != 0) {
	GET(buf, *len, 120);
	if (tty_status)
	    goto to;
    } else {
	WriteError("TCP: remote sends empty frame");
    }

    /*
     *  Get block trailer
     */
    c = GETCHAR(120);
    if (tty_status)
	goto to;
    if (c != TCP_BLKEND) {
	WriteError("TCP: got %d instead of block trailer", c);
	return c;
    }

    if (rx_type == TCP_CMD) {
	buf[*len] = '\0';
	Syslog('a', "tcp_rblk: cmd: %s", buf);
    } else
	Syslog('a', "tcp_rblk: data: %d bytes", *len);

to:
    if (tty_status)
	WriteError("TCP: receive error: %s", ttystat[tty_status]);
    return tty_status;
}



static int getsync(void)
{
    int	c;

    PUTCHAR(0xaa);
    PUTCHAR(0x55);
    FLUSHOUT();
    Syslog('a', "getsync try to synchronize");

gs:
    if (tty_status) {
	WriteError("TCP: getsync failed %s", ttystat[tty_status]);
	return 1;
    }
    while ((c = GETCHAR(120)) != 0xaa)
	if (tty_status) {
	    WriteError("TCP: getsync failed: %s", ttystat[tty_status]);
	    return 1;
	}

    if ((c = GETCHAR(120)) != 0x55)
	goto gs;

    Syslog('a', "getsync done, tty_status %s", ttystat[tty_status]);
    return tty_status;
}


