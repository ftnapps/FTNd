/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Common utilities
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/mberrors.h"
#include <sys/un.h>
#include "common.h"


pid_t		mypid;			/* Original parent pid if child	*/
unsigned long	lcrc = 0, tcrc = 1;	/* CRC value of logstring	*/
int		lcnt = 0;		/* Same message counter		*/
static char	*pbuff = NULL;
static int      sock = -1;      	/* TCP/IP socket		*/
int		ttyfd;			/* Filedescriptor for raw mode	*/
struct termios	tbufs, tbufsavs;	/* Structure for raw mode	*/

struct sockaddr_un  clntaddr;		/* Client socket address	*/
struct sockaddr_un  servaddr;		/* Server socket address	*/
struct sockaddr_un  from;		/* From socket address		*/
int		    fromlen;
static char	    spath[PATH_MAX];	/* Server socket path		*/
static char	    cpath[PATH_MAX];	/* Client socket path		*/

extern int	lines, columns;


void InitClient(char *user)
{
    sprintf(cpath, "%s/tmp/mbmon%d", getenv("MBSE_ROOT"), getpid());
    sprintf(spath, "%s/tmp/mbtask", getenv("MBSE_ROOT"));

    /*
     * Store my pid in case a child process is forked and wants to do
     * some communications with the mbsed server.
     */
    mypid = getpid();
    if (socket_connect(user) == -1) {
	printf("PANIC: cannot access socket\n");
	exit(MBERR_INIT_ERROR);
    }
}



void ExitClient(int errcode)
{
	if (socket_shutdown(mypid) == -1)
		printf("PANIC: unable to shutdown socket\n");
	fflush(stdout);
	fflush(stdin);

	if (pbuff)
		free(pbuff);

	unlink(cpath);
	exit(errcode);
}



void SockS(const char *format, ...)
{
	char	*out;
	va_list	va_ptr;

	out = calloc(2048, sizeof(char));

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
	out = calloc(2048, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(out, format, va_ptr);
	va_end(va_ptr);

	if (socket_send(out) == 0)
		sprintf(buf, "%s", socket_receive());

	free(out);
	return buf;
}



void Syslog(int level, const char *format, ...)
{
	char		*outstr;
	va_list		va_ptr;
	int		i;

	outstr = calloc(2048, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outstr, format, va_ptr);
	va_end(va_ptr);

	for (i = 0; i < strlen(outstr); i++)
		if (outstr[i] == '\r' || outstr[i] == '\n')
			outstr[i] = ' ';

	tcrc = StringCRC32(outstr);
	if (tcrc == lcrc) {
		lcnt++;
		free(outstr);
		return;
	} else {
		lcrc = tcrc;
		if (lcnt) {
			lcnt++;
			SockS("ALOG:5,mbmon.log,mbmon,%d,+,Last message repeated %d times;", mypid, lcnt);
		}
		lcnt = 0;
	}

	SockS("ALOG:5,mbmon.log,mbmon,%d,+,%s;", mypid, outstr);
	free(outstr);
}



void IsDoing(const char *format, ...)
{
	char	*outputstr;
	va_list	va_ptr;

	outputstr = calloc(64, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outputstr, format, va_ptr);
	va_end(va_ptr);

	SockS("ADOI:2,%d,%s;", mypid, outputstr);
	free(outputstr);
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



/************************************************************************
 *
 * Connect to Unix Datagram socket, return -1 if error or socket no. 
 */

int socket_connect(char *user)
{
    int 	s;
    static char	buf[SS_BUFSIZE], tty[18];

    if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
	perror("mbmon");
	printf("Unable to create Unix Datagram socket\n");
	return -1;
    }

    memset(&clntaddr, 0, sizeof(clntaddr));
    clntaddr.sun_family = AF_UNIX;
    strcpy(clntaddr.sun_path, cpath);

    if (bind(s, (struct sockaddr *)&clntaddr, sizeof(clntaddr)) < 0) {
	close(s);
	perror("mbmon");
	printf("Can't bind socket %s\n", cpath);
	return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    sprintf(servaddr.sun_path, "%s", (char *)spath);

    /*
     * Now that we have an connection, we gather 
     * information to tell the server who we are.
     */
    if (isatty(1) && (ttyname(1) != NULL)) {
	strcpy(tty, ttyname(1));
	if (strchr(tty, 'p'))
	    memccpy(tty, index(tty, 'p'), '\0', strlen(tty));
	else if (strchr(tty, 't'))
	    memccpy(tty, index(tty, 't'), '\0', strlen(tty));
	else if (strchr(tty, 'c'))
	    memccpy(tty, index(tty, 'c'), '\0', strlen(tty));
    } else {
	strcpy(tty, "-");
    }
    sock = s;

    /*
     * Send the information to the server. 
     */
    sprintf(buf, "AINI:5,%d,%s,%s,mbmon,localhost;", getpid(), tty, user);
    if (socket_send(buf) != 0) {
	sock = -1;
	return -1;
    }

    strcpy(buf, socket_receive());
    if (strncmp(buf, "100:0;", 6) != 0) {
	printf("AINI not acknowledged by the server\n");
	sock = -1;
	return -1;
    }

    return s;
}



/*
 * Send data via Unix Datagram socket
 */
int socket_send(char *buf)
{
	if (sock == -1)
		return -1;

	if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) & servaddr, sizeof(servaddr)) != strlen(buf)) {
		printf("Socket send failed error %d\n", errno);
		return -1;
	}

	return 0;
}



/*
 * Return an empty buffer if somthing went wrong, else the complete
 * dataline is returned.
 */
char *socket_receive(void)
{
	static char	buf[SS_BUFSIZE];
	int		rlen;

	memset((char *)&buf, 0, SS_BUFSIZE);
	fromlen = sizeof(from);
	rlen = recvfrom(sock, buf, SS_BUFSIZE, 0, &from, &fromlen);
	if (rlen == -1) {
		perror("recv");
		printf("Error reading socket\n");
		memset((char *)&buf, 0, SS_BUFSIZE);
		return buf;
	}
	return buf;
}



/***************************************************************************
 *
 *  Shutdown the socket, first send the server the close command so this
 *  application will be removed from the servers active clients list.
 *  There must be a parameter with the pid so that client applications
 *  where the shutdown will be done by a child process is able to give
 *  the parent pid as an identifier.
 */

int socket_shutdown(pid_t pid)
{
	static char	buf[SS_BUFSIZE];

	if (sock == -1)
		return 0;

	sprintf(buf, "ACLO:1,%d;", pid);
 	if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "107:0;", 6) != 0) {
			printf("Shutdown not acknowledged by the server\n");
			printf("Got \"%s\"\n", buf);
		}
	}
	
	if (shutdown(sock, 1) == -1) {
		perror("mbmon");
		printf("Cannot shutdown socket\n");
		return -1;
	}

	sock = -1;
	return 0;
}



/*
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 */
/* First, the polynomial itself and its table of feedback terms.  The  */
/* polynomial is							*/
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Note that we take it "backwards" and put the highest-order term in  */
/* the lowest-order bit.  The X^32 term is "implied"; the LSB is the   */
/* X^31 term, etc.  The X^0 term (usually shown as "+1") results in    */
/* the MSB being 1. 							*/

/* Note that the usual hardware shift register implementation, which   */
/* is what we're using (we're merely optimizing it by doing eight-bit  */
/* chunks at a time) shifts bits into the lowest-order term.  In our   */
/* implementation, that means shifting towards the right.  Why do we   */
/* do it this way?  Because the calculated CRC must be transmitted in  */
/* order from highest-order term to lowest-order term.	UARTs transmit */
/* characters in order from LSB to MSB.  By storing the CRC this way,  */
/* we hand it to the UART in the order low-byte to high-byte; the UART */
/* sends each low-bit to hight-bit; and the result is transmission bit */
/* by bit from highest- to lowest-order term without requiring any bit */
/* shuffling on our part.  Reception works similarly.				   */

/* The feedback terms table consists of 256, 32-bit entries.  Notes:   */
/*																	   */
/* The table can be generated at runtime if desired; code to do so 	*/
/* is shown later. It might not be obvious, but the feedback	   	*/
/* terms simply represent the results of eight shift/xor opera-    	*/
/* tions for all combinations of data and CRC register values.	   	*/
/*									*/
/* The values must be right-shifted by eight bits by the "updcrc"  	*/
/* logic; the shift must be unsigned (bring in zeroes).  On some   	*/
/* hardware you could probably optimize the shift in assembler by  	*/
/* using byte-swap instructions.					*/

unsigned long crc32tab[] = {  /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



/* Calculate the CRC of a string. */

unsigned long str_crc32(char *str)
{
	unsigned long crc;

	for (crc=0L; *str; str++) 
		crc = crc32tab[((int)crc^(*str)) & 0xff] ^ ((crc>>8) & 0x00ffffff);
	return crc;
}



unsigned long StringCRC32(char *str)
{
	unsigned long crc;

	for (crc = 0xffffffff; *str; str++)
		crc = crc32tab[((int)crc^(*str)) & 0xff] ^ ((crc>>8) & 0x00ffffff);
	return crc;
}



int rawset = FALSE;


/*
 * Sets raw mode and saves the terminal setup
 */
void Setraw()
{
    int	rc;

    if ((rc = tcgetattr(ttyfd, &tbufs))) {
	perror("");
	printf("$tcgetattr(0, save) return %d\n", rc);
	exit(MBERR_TTYIO_ERROR);
    }

    tbufsavs = tbufs;
    tbufs.c_iflag &= ~(INLCR | ICRNL | ISTRIP | IXON  );
    /*
     *  Map CRNL modes strip control characters and flow control
     */
    tbufs.c_oflag &= ~OPOST;		/* Don't do ouput character translation */
    tbufs.c_lflag &= ~(ICANON | ECHO);  /* No canonical input and no echo */
    tbufs.c_cc[VMIN]  = 1;  		/* Receive 1 character at a time */
    tbufs.c_cc[VTIME] = 0;  		/* No time limit per character */

    if ((rc = tcsetattr(ttyfd, TCSADRAIN, &tbufs))) {
	perror("");
	printf("$tcsetattr(%d, TCSADRAIN, raw) return %d\n", ttyfd, rc);
	exit(MBERR_TTYIO_ERROR);
    }

    rawset = TRUE;
}



/*
 * Unsets raw mode and returns state of terminal
 */
void Unsetraw()
{
    int	rc;

    /*
     * Only unset the mode if it is set to raw mode
     */
    if (rawset == TRUE) {
	if ((rc = tcsetattr(ttyfd, TCSAFLUSH, &tbufsavs))) {
	    perror("");
	    printf("$tcsetattr(%d, TCSAFLUSH, save) return %d\n", ttyfd, rc);
	    exit(MBERR_TTYIO_ERROR);
	}
    }
    rawset = FALSE;
}



/*
 *  Wait for a character for a maximum of wtime * 10 mSec.
 */
int Waitchar(unsigned char *ch, int wtime)
{
	int	i, rc = -1;

	for (i = 0; i < wtime; i++) {
		rc = read(ttyfd, ch, 1);
		if (rc == 1)
			return rc;
		usleep(10000);
	}
	return rc;
}



int Escapechar(unsigned char *ch)
{
	int		rc;
	unsigned char	c;
	
	/* 
	 * Escape character, if nothing follows within 
	 * 50 mSec, the user really pressed <esc>.
	 */
	if ((rc = Waitchar(ch, 5)) == -1)
		return rc;

	if (*ch == '[') {
		/*
		 *  Start of CSI sequence. If nothing follows,
		 *  return immediatly.
		 */
		if ((rc = Waitchar(ch, 5)) == -1)
			return rc;

		/*
		 *  Test for the most important keys. Note
		 *  that only the cursor movement keys are
		 *  guaranteed to work with PC-clients.
		 */
		c = *ch;
		if (c == 'A')
			c = KEY_UP;
		if (c == 'B')
			c = KEY_DOWN;
		if (c == 'C')
			c = KEY_RIGHT;
		if (c == 'D')
			c = KEY_LEFT;
		if ((c == '1') || (c == 'H') || (c == 0))
			c = KEY_HOME;
		if ((c == '4') || (c == 'K') || (c == 101) || (c == 144))
			c = KEY_END;
		if (c == '2')
			c = KEY_INS;
		if (c == '3')
			c = KEY_DEL;
		if (c == '5')
			c = KEY_PGUP;
		if (c == '6')
			c = KEY_PGDN;
		memcpy(ch, &c, sizeof(unsigned char));
		return rc;
	}
	return -1;
}



/*
 * Returns the offset from your location to UTC. So in the MET timezone
 * this returns -60 (wintertime). People in the USA get positive results.
 */
long gmt_offset(time_t now)
{
	struct tm	ptm;
	struct tm	gtm;
	long		offset;

	if (!now) 
		now = time(NULL);
	ptm = *localtime(&now);

	/* 
	 * To get the timezone, compare localtime with GMT.
	 */
	gtm = *gmtime(&now);

	/* 
	 * Assume we are never more than 24 hours away.
	 */
	offset = gtm.tm_yday - ptm.tm_yday;
	if (offset > 1)
	    offset = -24;
	else if (offset < -1)
	    offset = 24;
	else
	    offset *= 24;

	/* 
	 * Scale in the hours and minutes; ignore seconds.
	 */
	offset += gtm.tm_hour - ptm.tm_hour;
	offset *= 60;
	offset += gtm.tm_min - ptm.tm_min;

	return offset;
}



/*
 * Returns the TZUTC string, note that the sign is opposite from the
 * function above.
 */
char *gmtoffset(time_t now)
{
	static char	buf[6]="+0000";
	char		sign;
	int		hr, min;
	long		offset;

	offset = gmt_offset(now);

	if (offset <= 0) {
		sign = '+';
		offset = -offset;
	} else
		sign = '-';

	hr  = offset / 60L;
	min = offset % 60L;

	if (sign == '-')
		sprintf(buf, "%c%02d%02d", sign, hr, min);
	else
		sprintf(buf, "%02d%02d", hr, min);

	return(buf);
}



char *str_time(time_t total)
{
	static char	buf[10];
	int		h, m;

	memset(&buf, 0, sizeof(buf));

	/*
	 * 0 .. 59 seconds
	 */
	if (total < (time_t)60) {
		sprintf(buf, "%2d.00s", (int)total);
		return buf;
	}

	/*
	 * 1:00 .. 59:59 minutes:seconds
	 */
	if (total < (time_t)3600) {
		h = total / 60;
		m = total % 60;
		sprintf(buf, "%2d:%02d ", h, m);
		return buf;
	}

	/*
	 * 1:00 .. 23:59 hours:minutes
	 */
	if (total < (time_t)86400) {
		h = (total / 60) / 60;
		m = (total / 60) % 60;
		sprintf(buf, "%2d:%02dm", h, m);
		return buf;
	}

	/*
	 * 1/00 .. 30/23 days/hours
	 */
	if (total < (time_t)2592000) {
		h = (total / 3600) / 24;
		m = (total / 3600) % 24;
		sprintf(buf, "%2d/%02dh", h, m);
		return buf;
	}

	sprintf(buf, "N/A   ");
	return buf;
}



char *t_elapsed(time_t start, time_t end)
{
	return str_time(end - start);
}



char *xstrcpy(char *src)
{
	char	*tmp;

	if (src == NULL) 
		return(NULL);
	tmp = malloc(strlen(src)+1);
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
	tmp = malloc(size + 1);
	*tmp = '\0';
	if (src) {
		strcpy(tmp, src);
		free(src);
	}
	strcat(tmp, add);
	return tmp;
}



char *padleft(char *str, int size, char pad)
{
	static char stri[256];
	static char temp[256];

	strcpy(stri, str);
	memset(temp, pad, (long)size);
	temp[size] = '\0';
	if (strlen(stri) <= size)
		memmove(temp, stri, (long)strlen(stri));
	else
		memmove(temp, stri, (long)size);
	return temp;
}



void Striplf(char *String)
{
	int i;

	for(i = 0; i < strlen(String); i++) {
		if(*(String + i) == '\0')
			break;
		if(*(String + i) == '\n')
			*(String + i) = '\0';
	}
}



/*
 * Changes ansi background and foreground color
 */
void colour(int fg, int bg)
{
	int att=0, fore=37, back=40;

	if (fg<0 || fg>31 || bg<0 || bg>7) {
		printf("ANSI: Illegal colour specified: %i, %i\n", fg, bg);
		return; 
	}

	printf("[");
	if ( fg > 15) {
		printf("5;");
		fg-=16;
	}
	if (fg > 7) {
		att=1;
		fg=fg-8;
	}

	if      (fg==0) fore=30;
	else if (fg==1) fore=34;
	else if (fg==2) fore=32;
	else if (fg==3) fore=36;
	else if (fg==4) fore=31;
	else if (fg==5) fore=35;
	else if (fg==6) fore=33;
	else            fore=37;

	if      (bg==1) back=44;
	else if (bg==2) back=42;
	else if (bg==3) back=46;
	else if (bg==4) back=41;
	else if (bg==5) back=45;
	else if (bg==6) back=43;
	else if (bg==7) back=47;
	else            back=40;
		
	printf("%d;%d;%dm", att, fore, back);
}



void clear()
{
	colour(LIGHTGRAY, BLACK);
	printf(ANSI_HOME);
	printf(ANSI_CLEAR);
}



/*
 * Moves cursor to specified position
 */
void locate(int y, int x)
{
	if (y > lines || x > columns) {
		printf("ANSI: Invalid screen coordinates: %i, %i\n", y, x);
		return; 
	}
	printf("\x1B[%i;%iH", y, x);
}



/*
 * curses compatible functions
 */
void mvprintw(int y, int x, const char *format, ...)
{
	char		*outputstr;
	va_list		va_ptr;

	outputstr = calloc(2048, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(outputstr, format, va_ptr);
	va_end(va_ptr);

	locate(y, x);
	printf(outputstr);
	free(outputstr);
}



/*
 *  Signal handler signal names.
 */

#ifdef __i386__

char	SigName[32][16] = {	"NOSIGNAL",
		"SIGHUP",	"SIGINT",	"SIGQUIT",	"SIGILL",
		"SIGTRAP",	"SIGIOT",	"SIGBUS",	"SIGFPE",
		"SIGKILL",	"SIGUSR1",	"SIGSEGV",	"SIGUSR2",
		"SIGPIPE",	"SIGALRM",	"SIGTERM",	"SIGSTKFLT",
		"SIGCHLD",	"SIGCONT",	"SIGSTOP",	"SIGTSTP",
		"SIGTTIN",	"SIGTTOU",	"SIGURG",	"SIGXCPU",
		"SIGXFSZ",	"SIGVTALRM",	"SIGPROF",	"SIGWINCH",
		"SIGIO",	"SIGPWR",	"SIGUNUSED"};

#endif

#ifdef __PPC__

char    SigName[32][16] = {     "NOSIGNAL",
                "SIGHUP",       "SIGINT",       "SIGQUIT",      "SIGILL",
                "SIGTRAP",      "SIGIOT",       "SIGBUS",       "SIGFPE",
                "SIGKILL",      "SIGUSR1",      "SIGSEGV",      "SIGUSR2",
                "SIGPIPE",      "SIGALRM",      "SIGTERM",      "SIGSTKFLT",
                "SIGCHLD",      "SIGCONT",      "SIGSTOP",      "SIGTSTP",
                "SIGTTIN",      "SIGTTOU",      "SIGURG",       "SIGXCPU",
                "SIGXFSZ",      "SIGVTALRM",    "SIGPROF",      "SIGWINCH",
                "SIGIO",        "SIGPWR",       "SIGUNUSED"};

#endif

#ifdef __sparc__

char	SigName[32][16] = {	"NOSIGNAL",
		"SIGHUP",	"SIGINT",	"SIGQUIT",	"SIGILL",
		"SIGTRAP",	"SIGIOT",	"SIGEMT",	"SIGFPE",
		"SIGKILL",	"SIGBUS",	"SIGSEGV",	"SIGSYS",
		"SIGPIPE",	"SIGALRM",	"SIGTERM",	"SIGURG",
		"SIGSTOP",	"SIGTSTP",	"SIGCONT",	"SIGCHLD",
		"SIGTTIN",	"SIGTTOU",	"SIGIO",	"SIGXCPU",
		"SIGXFSZ",	"SIGVTALRM",	"SIGPROF",	"SIGWINCH",
		"SIGLOST",	"SIGUSR1",	"SIGUSR2"};
#endif

#ifdef __alpha__

char	SigName[32][16] = {	"NOSIGNAL",
		"SIGHUP",	"SIGINT",	"SIGQUIT",	"SIGILL",
		"SIGTRAP",	"SIGABRT",	"SIGEMT",	"SIGFPE",
		"SIGKILL",	"SIGBUS",	"SIGSEGV",	"SIGSYS",
		"SIGPIPE",	"SIGALRM",	"SIGTERM",	"SIGURG",
		"SIGSTOP",	"SIGTSTP",	"SIGCONT",	"SIGCHLD",
		"SIGTTIN",	"SIGTTOU",	"SIGIO",	"SIGXCPU",
		"SIGXFSZ",	"SIGVTALRM",	"SIGPROF",	"SIGWINCH",
		"SIGINFO",	"SIGUSR1",	"SIGUSR2"};

#endif

#ifdef __hppa__

char    SigName[32][16] = {     "NOSIGNAL",
		"SIGHUP",       "SIGINT",       "SIGQUIT",      "SIGILL",
		"SIGTRAP",      "SIGABRT",      "SIGEMT",       "SIGFPE",
		"SIGKILL",      "SIGBUS",       "SIGSEGV",      "SIGSYS",
		"SIGPIPE",      "SIGALRM",      "SIGTERM",      "SIGUSR1",
		"SIGUSR2",      "SIGCHLD",      "SIGPWR",       "SIGVTALRM",
		"SIGPROF",      "SIGIO",        "SIGWINCH",     "SIGSTOP",
		"SIGTSTP",      "SIGCONT",      "SIGTTIN",      "SIGTTOU",
		"SIGURG",       "SIGLOST",      "SIGUNUSED"};

#endif

