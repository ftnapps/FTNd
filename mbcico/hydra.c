/*****************************************************************************
 *
 * $Id: hydra.c,v 1.36 2007/08/25 18:32:07 mbse Exp $
 * Purpose ...............: Fidonet mailer - Hydra protocol driver
 * Remark ................: See below for more copyright details and credits.
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
 * ifcico v3.0.cm - hydra protocol module
 * Copyright (C) 1996-98  Christof Meerwald.
 */

/*
 * The HYDRA protocol was designed by
 * Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
 * Joaquim H. Homrighausen
 * COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED
 */

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "session.h"
#include "filelist.h"
#include "filetime.h"
#include "ttyio.h"
#include "statetbl.h"
#include "config.h"
#include "emsi.h"
#include "openfile.h"
#include "lutil.h"
#include "respfreq.h"
#include "mbcico.h"
#include "hydra.h"



#define H_RXWINDOW	0L
#define H_TXWINDOW	0L



static int put_binbyte(char *outbuf, char c);
static int put_hexbyte(char *outbuf, char c);
static enum HyPktTypes hyrxpkt(char *rxbuf, int *rxlen, int tot);
static void hytxpkt(enum HyPktTypes pkttype, char *txbuf, int txlen);
static int put_flags(char *buf, unsigned int Flags);
static unsigned int get_flags(char *buf);
static int resync(off_t off);
static int hydra_batch(int role, file_list *to_send);

extern unsigned int	sentbytes;
extern unsigned int	rcvdbytes;



static struct h_flags_struct {
	char *str;
	unsigned int	val;
} h_flags[] =
	{
	{ (char *)"XON", HOPT_XONXOFF },
	{ (char *)"TLN", HOPT_TELENET },
	{ (char *)"CTL", HOPT_CTLCHRS },
	{ (char *)"HIC", HOPT_HIGHCTL },
	{ (char *)"HI8", HOPT_HIGHBIT },
	{ (char *)"BRK", HOPT_CANBRK  },
	{ (char *)"ASC", HOPT_CANASC  },
	{ (char *)"UUE", HOPT_CANUUE  },
	{ (char *)"C32", HOPT_CRC32   },
	{ (char *)"DEV", HOPT_DEVICE  },
	{ (char *)"FPT", HOPT_FPT     },
#ifdef HAVE_ZLIB_H
	{ (char *)"PLZ", HOPT_CANPLZ  },
#endif
	{ NULL , 0x0L         }
};

static int txoptions, rxoptions;


static char *put_long(char *buffer, int val)
{
#ifdef WORDS_BIGENDIAN
    buffer[0] =  (unsigned int) val & 0xff;
    buffer[1] = ((unsigned int) val >> 8) & 0xff;
    buffer[2] = ((unsigned int) val >> 16) & 0xff;
    buffer[3] = ((unsigned int) val >> 24) & 0xff;
#else
    *(unsigned int *) buffer = (unsigned int) val;
#endif
    return buffer;
}



static int get_long(char *buffer)
{
#ifdef WORDS_BIGENDIAN
    return ((unsigned int) ((unsigned char) buffer[0])) | ((unsigned int) ((unsigned char) buffer[1]) << 8) |
	   ((unsigned int) ((unsigned char) buffer[2]) << 16) | ((unsigned int) ((unsigned char) buffer[3]) << 24);
#else
    return *(int *) buffer;
#endif
}



char *PktS(int);
char *PktS(int c)
{
    switch (c) {
	case 'A' :  return (char *)"START";
	case 'B' :  return (char *)"INIT";
	case 'C' :  return (char *)"INITACK";
	case 'D' :  return (char *)"FINFO";
	case 'E' :  return (char *)"FINFOACK";
	case 'F' :  return (char *)"DATA";
	case 'G' :  return (char *)"DATAACK";
	case 'H' :  return (char *)"RPOS";
	case 'I' :  return (char *)"EOF";
	case 'J' :  return (char *)"EOFACK";
	case 'K' :  return (char *)"END";
	case 'L' :  return (char *)"IDLE";
	case 'M' :  return (char *)"DEVDATA";
	case 'N' :  return (char *)"DEVDACK";
#ifdef HAVE_ZLIB_H
	case 'O' :  return (char *)"ZIPDATA";
#endif
	case 'a' :  return (char *)"PKTEND";
	case 'b' :  return (char *)"BINPKT";
	case 'c' :  return (char *)"HEXPKT";
	case 'd' :  return (char *)"ASCPKT";
	case 'e' :  return (char *)"UUEPKT";
	default:    break;
    }
    return (char *)"";
}



int put_binbyte(char *outbuf, char c)
{
	static int lastc = -1;
	register int count = 0;
	register char n;


	n = c;
	if (txoptions & HOPT_HIGHCTL) {
		n &= 0x7f;
	}

	if ((n == H_DLE)
	    || ((txoptions & HOPT_XONXOFF) && ((n == XON) || (n == XOFF)))
	    || ((txoptions & HOPT_TELENET) && (n == '\r') && (lastc == '@'))
	    || ((txoptions & HOPT_CTLCHRS) && ((n < 32) || (n == 127)))) {
		*outbuf++ = H_DLE;
		c ^= 0x40;
		count++;
	}

	*outbuf++ = c;
	lastc = n;

	return count + 1;
}



int put_hexbyte(char *outbuf, char c)
{
    static const char	hexdigit[] = "0123456789abcdef";
    register int	count = 0;

    if (c & 0x80) {
	*outbuf++ = '\\';
	*outbuf++ = hexdigit[(c >> 4) & 0x0f];
	*outbuf++ = hexdigit[c & 0x0f];
	count = 3;
    } else if ((c < 32) || (c == 127)) {
	*outbuf++ = H_DLE;
	*outbuf++ = c ^ 0x40;
	count = 2;
    } else if (c == '\\') {
	*outbuf++ = '\\';
	*outbuf++ = '\\';
	count = 2;
    } else {
	*outbuf++ = c;
	count = 1;
    }

    return count;
}



/* TODO: code cleanup */
/* TODO: error handling */
enum HyPktTypes hyrxpkt(char *rxbuf, int *rxlen, int tot)
{
    static char	rxencbuf[H_ZIPBUFLEN];
    static enum HyPktTypes pkttype = H_NOPKT;
    static char *inbuf = rxencbuf, *outbuf;
    int		c, i, n;
    static int	rxdle = 0;
    static enum HyPktFormats format;

    while ((c = GETCHAR(tot)) >= 0) {
	if (rxoptions & HOPT_HIGHBIT)
	    c &= 0x7f;

	n = c;
	if (rxoptions & HOPT_HIGHCTL)
	    n &= 0x7f;

	if ((n != H_DLE)
		&& (((rxoptions & HOPT_XONXOFF) && ((n == XON) || (n == XOFF)))
		|| ((rxoptions & HOPT_CTLCHRS) && ((n < 32) || (n == 127)))))
	    continue;


	if ((rxdle) || (c == H_DLE)) {
	    switch (c) {
		case H_DLE:
		    rxdle++;
		    if (rxdle >= 5) {
			inbuf = rxencbuf;
			return H_CANCEL;
		    }
		    break;

		case HCHR_PKTEND:
		    switch (format) {
			case HCHR_BINPKT:
			    *rxlen = inbuf - rxencbuf;
			    memcpy(rxbuf, rxencbuf, *rxlen);
			    break;

			case HCHR_HEXPKT:
			    outbuf = rxencbuf;
			    *rxlen = 0;

			    while (outbuf < inbuf) {
				if ((*outbuf == '\\') && (*++outbuf != '\\')) {
				    i = *outbuf++;
				    n = *outbuf++;

				    if ((i -= '0') > 9)
					i -= ('a' - ':');
				    if ((n -= '0') > 9)
					n -= ('a' - ':');

				    if ((i & ~0x0f) || (n & ~ 0x0f)) {
					Syslog('+', "Hydra: RXPKT assert");
					die(MBERR_FTRANSFER);
					break;
				    }

				    rxbuf[*rxlen] = (i << 4) | n;
				    *rxlen += 1;
				} else {
				    rxbuf[*rxlen] = *outbuf++;
				    *rxlen += 1;
				}
			    }
			    break;

			case HCHR_ASCPKT:
			    case HCHR_UUEPKT:
			    default:
				Syslog('+', "Hydra: RXPKT assert");
				die(MBERR_FTRANSFER);
		    }

		    if ((format != HCHR_HEXPKT) && (rxoptions & HOPT_CRC32)) {
			n = h_crc32test(crc32ccitt(rxbuf, *rxlen));
			*rxlen -= 4;		/* remove CRC-32 */
		    } else {
			n = h_crc16test(crc16ccitt(rxbuf, *rxlen));
			*rxlen -= 2;		/* remove CRC-16 */
		    }

		    *rxlen -= 1;		/* remove packet type */
		    pkttype = rxbuf[*rxlen];

		    /* check if CRC test succeeded */
		    if (n) {
			inbuf = rxencbuf;
			Syslog('h', "Hydra: RXPKT rcvd %s", PktS(pkttype));
			return pkttype;
		    } else {
			Syslog('+', "Hydra: RXPKT CRC test failed");
		    }
		    break;

		case HCHR_BINPKT:
		case HCHR_HEXPKT:
		case HCHR_ASCPKT:
		case HCHR_UUEPKT:
				format = c;
				inbuf = rxencbuf;
				rxdle = 0;
				break;

		default:
				*inbuf++ = c ^ 0x40;
				rxdle = 0;
				break;
	    }
	} else {
	    *inbuf++ = c;
	}
    }

    Syslog('h', "Hydra: GETCHAR returned %i", c);

    if ((c == TERROR) || (c == EOFILE) || (c == HANGUP)) {
	return H_CARRIER;
    }

    return H_NOPKT;
}



/* TODO: support packet prefix string */
void hytxpkt(enum HyPktTypes pkttype, char *txbuf, int txlen)
{
    static char txencbuf[H_ZIPBUFLEN];
    char	*outbuf, *inbuf;
    enum	HyPktFormats format;

    if (pkttype == HPKT_DATAACK)
        Syslog('h', "Hydra: ACK 0x%02x%02x%02x%02x", txbuf[0], txbuf[1], txbuf[2], txbuf[3]);

    /*
     * some packets have to be transferred in HEX mode
     */
    if ((pkttype == HPKT_START) || (pkttype == HPKT_INIT) || (pkttype == HPKT_INITACK) || 
	(pkttype == HPKT_END) || (pkttype == HPKT_IDLE)) {
	format = HCHR_HEXPKT;
    } else {
	/* do we need to strip high bit */
	if (txoptions & HOPT_HIGHBIT) {
	    if ((txoptions & HOPT_CTLCHRS) && (txoptions & HOPT_CANUUE)) {
		format = HCHR_UUEPKT;	/* use UUE packet encoding */
	    } else if (txoptions & HOPT_CANASC) {
		format = HCHR_ASCPKT;	/* we can use ASCII packet encoding */
	    } else {
		format = HCHR_HEXPKT;	/* fall back to hex packet encoding */
	    }
	} else {
	    format = HCHR_BINPKT;	/* we can use binary packet encoding */
	}
    }

    /*
     * Append format byte to data
     */
    txbuf[txlen] = pkttype;
    txlen++;

    /*
     * check if we can use 32-bit CRC's
     */
    if ((format != HCHR_HEXPKT) && (txoptions & HOPT_CRC32)) {
	unsigned int crc;

	/*
	 * Calc CRC-32 of data + pkttype
	 */
	crc = ~crc32ccitt(txbuf, txlen);

	/*
	 * Append one's complement of CRC to data, lowbyte first
	 */
	txbuf[txlen++] = crc;
	txbuf[txlen++] = crc >> 8;
	txbuf[txlen++] = crc >> 16;
	txbuf[txlen++] = crc >> 24;
    } else {
	unsigned short crc;

	/*
	 * Calc CRC-16 of data + pkttype
	 */
	crc = ~crc16ccitt(txbuf, txlen);

	/*
	 * Append one's complement of CRC to data, lowbyte first
	 */
	txbuf[txlen++] = crc;
	txbuf[txlen++] = crc >> 8;
    }

    inbuf = txbuf;

    outbuf = txencbuf;
    *outbuf++ = H_DLE;
    *outbuf++ = format;

    /* encode packet data */
    switch (format) {
	case HCHR_BINPKT:
	    while (txlen > 0) {
		outbuf += put_binbyte(outbuf, *inbuf);
		inbuf++;
		txlen--;
	    }
	    break;

	case HCHR_HEXPKT:
	    while (txlen > 0) {
		outbuf += put_hexbyte(outbuf, *inbuf);
		inbuf++;
		txlen--;
	    }
	    break;

	/* ASCII and UUE packets are not yet supported */
	case HCHR_ASCPKT:
	case HCHR_UUEPKT:
	default:
	    Syslog('+', "Hydra: TXPKT assert");
	    die(MBERR_FTRANSFER);
    }

    *outbuf++ = H_DLE;
    *outbuf++ = HCHR_PKTEND;

    if ((pkttype != HPKT_DATA) 
#ifdef HAVE_ZLIB_H
	&& (pkttype != HPKT_ZIPDATA)
#endif
	&& (format != HCHR_BINPKT)) {
	*outbuf++ = '\r';
	*outbuf++ = '\n';
    }

    Syslog('h', "Hydra: TXPKT send %s", PktS(pkttype));
    PUT(txencbuf, outbuf - txencbuf);
	
    return;
}



int put_flags(char *buf, unsigned int Flags)
{
    int i, count = 0;

    for (i = 0; h_flags[i].val; i++) {
	if (Flags & h_flags[i].val) {
	    if (count > 0) {
		*buf++ = ',';
		count++;
	    }
	
	    strcpy(buf, h_flags[i].str);
	    buf += H_FLAGLEN;
	    count += H_FLAGLEN;
	}
    }

    *buf = 0;
    return count;
}



unsigned int get_flags(char *buf)
{
    unsigned int    Flags = 0L;
    char	    *p;
    int		    i;


    for (p = strtok(buf, ","); p != NULL; p = strtok(NULL, ",")) {
	for (i = 0; h_flags[i].val; i++) {
	    if (!strcmp(p, h_flags[i].str)) {
		Flags |= h_flags[i].val;
		break;
	    }
	}
    }

    return Flags;
}



int resync(off_t off)
{
    return 0;
}



int hydra_batch(int role, file_list *to_send)
{
    static char	    txbuf[H_ZIPBUFLEN], rxbuf[H_ZIPBUFLEN];
    struct stat	    txstat;			/* file stat being transmitted */
    FILE	    *txfp = NULL;		/* file currently being transmitted */
    FILE	    *rxfp = NULL;		/* file currently being received */
    char	    *inbuf, *outbuf;
    int		    rxlen, txlen;		/* length of receive/transmit buffer */
    int		    txwindow, rxwindow;		/* window sizes */
    int		    txpos;
    off_t	    rxpos;			/* file positions */
    int		    stxpos, srxpos;
    int		    longnum;
    int		    hdxlink = FALSE;
    int		    txretries, rxretries;
    int		    txlastack, txsyncid;
    int		    rxlastsync, rxsyncid;
    int		    rxlastdatalen;
    int		    blksize;
    int		    goodbytes;
    int		    goodneeded;
    enum	    HyTxStates txstate;
    enum	    HyRxStates rxstate;
    enum	    HyCompStates compstate;
    int		    txwaitpkt, rxwaitpkt;
    enum	    HyPktTypes pkttype;
    int		    waitputget = 0;
    struct timeval  txstarttime, txendtime;
    struct timeval  rxstarttime, rxendtime;
    struct timezone tz;
    int		    sverr;
    int		    txcompressed, rxctries;
#ifdef HAVE_ZLIB_H
    static char	    txzbuf[H_ZIPBUFLEN], rxzbuf[H_ZIPBUFLEN];
    unsigned int    rxzlen, txzlen;             /* length of receive/transmit compressed buffer */
    int		    rcz, cmpblksize;
    uLongf	    destLen;
#endif
    
    Syslog('h', "Hydra: resettimers");
    RESETTIMERS();

    txcompressed = 0;
    txpos = rxpos = 0;
    stxpos = srxpos = 0;
    txretries = rxretries = 0;
    txlastack = txsyncid = 0;
    rxlastsync = rxsyncid = 0;
    rxlastdatalen = 0;
    blksize = 512;
#ifdef HAVE_ZLIB_H
    cmpblksize = H_UNCBLKLEN;
#endif
    goodbytes = 0;
    goodneeded = 1024;
    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
    txstate = HTX_START;
    txoptions = HTXI_OPTIONS;
    txstarttime.tv_sec = txstarttime.tv_usec = txendtime.tv_sec = txendtime.tv_usec = 0;
    rxstarttime.tv_sec = rxstarttime.tv_usec = rxendtime.tv_sec = rxendtime.tv_usec = 0;
    tz.tz_minuteswest = tz.tz_dsttime = 0;
    rxstate = HRX_INIT;
    rxoptions = HRXI_OPTIONS;
    compstate = HCMP_NONE;
    rxctries = 0;

    while ((txstate != HTX_DONE) && (txstate != HTX_Abort)) {
	/*
	 * Is transmitter waiting for packet? 
	 */
	txwaitpkt = ((txstate == HTX_SWAIT) || (txstate == HTX_INITACK)
	         || ((txstate == HTX_RINIT) && (rxstate == HRX_INIT))
		 || (txstate == HTX_FINFOACK) || (txstate == HTX_DATA) || (txstate == HTX_DATAACK)
		 || ((txstate == HTX_XWAIT) && (rxstate != HRX_DONE))
		 || (txstate == HTX_EOFACK) || (txstate == HTX_ENDACK) || (txstate == HTX_REND));

	/*
	 * Is receiver waiting for packet?
	 */
	rxwaitpkt = ((rxstate == HRX_INIT) || (rxstate == HRX_FINFO) || (rxstate == HRX_DATA) || (rxstate == HRX_DONE));

	/*
	 * Do we have to wait for a packet?
	 */
	if (txwaitpkt && rxwaitpkt) {
	    /*
	     * Don't wait for a packet if transmitter is in DATA state
	     */
	    if ((txstate == HTX_DATA) || ((txstate == HTX_REND) && (rxstate == HRX_DONE))) {
		if (txstate == HTX_DATA) {
		    waitputget = WAITPUTGET(-1);
		    if (waitputget & 1) {
			pkttype = hyrxpkt(rxbuf, &rxlen, 0);
		    } else {
			pkttype = H_NOPKT;
		    }
		} else {
		    pkttype = hyrxpkt(rxbuf, &rxlen, 0);
		}
	    } else {
		pkttype = hyrxpkt(rxbuf, &rxlen, -1);
	    }

	    if (EXPIRED(TIMERNO_BRAIN)) {
		Syslog('+', "Hydra: BRAIN timer expired");
		txstate = HTX_Abort;
		break;
	    }
	    if (pkttype == H_CARRIER) {
		Syslog('+', "Hydra: lost CARRIER");
		txstate = HTX_Abort;
		break;
	    }
	} else {
	    pkttype = H_NOPKT;
	}

	/*
	 * Special handling for RPOS packet
	 */
	if ((pkttype == HPKT_RPOS) && (rxlen == 12)
		&& ((txstate == HTX_DATA) || (txstate == HTX_DATAACK)
		|| (txstate == HTX_XWAIT) || (txstate == HTX_EOFACK))) {
	    int rpos_pos = get_long(rxbuf);
	    int rpos_blksize = get_long(rxbuf + 4);
	    int rpos_id = get_long(rxbuf + 8);

#ifdef HAVE_ZLIB_H
	    /*
	     * If rpos_id is -1 the drop compression mode.
	     */
	    if ((rpos_id == -1) && (compstate != HCMP_NONE)) {
		Syslog('+', "Hydra: remote asked stop compression");
		compstate = HCMP_NONE;
		/*
		 * Adjust blocksize for normal uncompressed transfers
		 */
		if (blksize > H_UNCBLKLEN) {
		    blksize = H_UNCBLKLEN;
		}
	    }
#endif

	    if (rpos_pos < 0) {
		/*
	         * this differs from the protocol definition: txpos is used
		 * instead of rxpos (I think it's wrong in the protocol
		 * definition as in the example source also txpos is used)
		 */
		if ((rpos_pos == -2) && (txpos != -2) && (txstate == HTX_EOFACK)) {
		    txpos = -2;
		    txstate = HTX_EOF;
		} else {
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		    txstate = HTX_SkipFile;
		}
	    }

	    if (rpos_id == txsyncid) {
		txretries++;

		if (txretries >= 10) {
		    Syslog('+', "Hydra: too many errors");
		    txstate = HTX_Abort;
		    break;
		}
	    } else {
		if (rpos_pos >= 0) {
		    txpos = rpos_pos;
		    txsyncid = rpos_id;
		    txretries = 1;
		    blksize = rpos_blksize;
		    goodbytes = 0;
		    goodneeded += 1024;
		    if (goodneeded > 8192)
			goodneeded = 8192;

		    /* 
		     * if we receive an RPOS packet in EOFACK-state we have to
		     * change back to DATA-state
		     */
		    if (txstate == HTX_EOFACK)
			txstate = HTX_DATA;

		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		}
	    }
	
	    pkttype = H_NOPKT;	/* packet has already been processed */
	}


	switch (txstate) {
	    /*
	     * initiate Hydra session
	     */
	    case HTX_START: 
		Syslog('h', "SM 'HTX' entering 'START'");
		if (txretries < 10) {
		    PUT((char *)"hydra\r", 6);	/* send AutoStart string */
		    hytxpkt(HPKT_START, txbuf, 0);
		    Syslog('h', "Hydra: set TX timer %d", H_START);
		    SETTIMER(TIMERNO_TX, H_START);
		    txstate = HTX_SWAIT;
		} else {
		    Syslog('+', "Hydra: transmitter start TIMEOUT");
		    txstate = HTX_Abort;
		    break;
		}
		break;

	    /*
	     * wait for START packet
	     */
	    case HTX_SWAIT: 
		Syslog('h', "SM 'HTX' entering 'SWAIT'");
		if (((pkttype == HPKT_START) && (rxlen == 0)) || (pkttype == HPKT_INIT)) {
		    txretries = 0;
		    Syslog('h', "Hydra: reset TX timer");
		    RESETTIMER(TIMERNO_TX);
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		    txstate = HTX_INIT;

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('+', "Hydra: transmitter timeout (HTX_SWAIT)");
		    txretries++;
		    txstate = HTX_START;
		}
		break;

	    /*
	     * send INIT packet
	     */
	    case HTX_INIT: 
		Syslog('h', "SM 'HTX' entering 'INIT'");
		if (txretries < 10) {
		    outbuf = txbuf;

		    /* Application ID string */
		    outbuf += snprintf(outbuf, H_ZIPBUFLEN, "%08lx%s,%s", H_REVSTAMP, "mbcico", VERSION) + 1;

		    /* Supported options */
		    outbuf += put_flags(outbuf, HCAN_OPTIONS) + 1;
		    Syslog('h', "Hydra: supported options: %08lx", HCAN_OPTIONS);

		    /* Desired options */
		    outbuf += put_flags(outbuf, HDEF_OPTIONS & HCAN_OPTIONS & ~HUNN_OPTIONS) + 1;
		    Syslog('h', "Hydra: desired options  : %08lx", HDEF_OPTIONS & HCAN_OPTIONS & ~HUNN_OPTIONS);

		    /* Desired transmitter and receiver window size */
		    outbuf += snprintf(outbuf, H_ZIPBUFLEN, "%08lx%08lx", H_TXWINDOW, H_RXWINDOW) + 1;

		    /* Packet prefix string */
		    *outbuf++ = 0;

		    hytxpkt(HPKT_INIT, txbuf, outbuf - txbuf);

		    Syslog('h', "Hydra: set TX timer %d", H_MINTIMER/2);
		    SETTIMER(TIMERNO_TX, H_MINTIMER/2);
		    txstate = HTX_INITACK;
		} else {
		    Syslog('+', "Hydra: too many errors waiting for INITACK");
		    txstate = HTX_Abort;
		    break;
		}
		break;

	    /*
	     * wait for an INIT acknowledge packet
	     */
	    case HTX_INITACK: 
		Syslog('h', "SM 'HTX' entering 'INITACK'");
		if ((pkttype == HPKT_INITACK) && (rxlen == 0)) {
		    txretries = 0;
		    Syslog('h', "Hydra: reset TX timer");
		    RESETTIMER(TIMERNO_TX);
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		    txstate = HTX_RINIT;

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('+', "Hydra: tx timeout");
		    txretries++;
		    txstate = HTX_INIT;
		}
		break;

	    /*
	     * wait for receiver to leave INIT state
	     */
	    case HTX_RINIT: 
		Syslog('h', "SM 'HTX' entering 'RINIT'");
		if (rxstate != HRX_INIT)
		    txstate = HTX_NextFile;
		break;

	    /*
	     * prepare next file for transmitting
	     */
	    case HTX_NextFile: 
		Syslog('h', "SM 'HTX' entering 'NextFile'");
		/*
		 * skip file with NULL remote name
		 */
		while (to_send && (to_send->remote == NULL)) {
		    execute_disposition(to_send);
		    to_send = to_send->next;
		}

		if (to_send) {
		    struct flock txflock;

		    if (to_send->remote == NULL)
			break;

		    txflock.l_type=F_RDLCK;
		    txflock.l_whence=0;
		    txflock.l_start=0L;
		    txflock.l_len=0L;

		    txfp = fopen(to_send->local, "r");
		    if (txfp == NULL) {
			sverr = errno;
			if ((sverr == ENOENT) || (sverr == EINVAL)) {
			    Syslog('+', "File %s doesn't exist, removing", MBSE_SS(to_send->local));
			    execute_disposition(to_send); // Added 18-10-99 MB.
			} else {
			    WriteError("$Hydra: cannot open file %s, skipping", MBSE_SS(to_send->local));
			}
			to_send = to_send->next;
			break;
		    }

		    if (fcntl(fileno(txfp), F_SETLK, &txflock) != 0) {
			WriteError("$Hydra: cannot lock file %s, skipping", MBSE_SS(to_send->local));
			fclose(txfp);
			to_send = to_send->next;
			break;
		    }

		    if (stat(to_send->local, &txstat) != 0) {
			WriteError("$Hydra: cannot access \"%s\", skipping",MBSE_SS(to_send->local));
			fclose(txfp);
			to_send = to_send->next;
			break;
		    }

		    Syslog('+', "Hydra: send \"%s\" as \"%s\"", MBSE_SS(to_send->local), MBSE_SS(to_send->remote));
		    Syslog('+', "Hydra: size %lu bytes, dated %s",(unsigned int)txstat.st_size, date(txstat.st_mtime));
		    gettimeofday(&txstarttime, &tz);
		}

		txstate = HTX_ToFName;
		break;			/* TODO: fallthrough */

	    case HTX_ToFName: 
		Syslog('h', "SM 'HTX' entering 'ToFName'");
		txsyncid = 0;
		txretries = 0;
		Syslog('h', "Hydra: reset TX timer");
		RESETTIMER(TIMERNO_TX);
		txstate = HTX_FINFO;
		break;			/* TODO: fallthrough */

	    /*
	     * transmit File Information packet
	     */
	    case HTX_FINFO: 
		Syslog('h', "SM 'HTX' entering 'FINFO'");
		if (txretries >= 10) {
		    txstate = HTX_Abort;
		    break;
		} else {
		    if (to_send) {
			txlen = snprintf(txbuf, H_ZIPBUFLEN, "%08x%08x%08x%08x%08x",
					(int)mtime2sl(txstat.st_mtime+(txstat.st_mtime%2)),
					(int)(txstat.st_size), 0U, 0U, 0U);

			/*
			 * convert file name to DOS-format
			 */
			outbuf = xstrcpy(to_send->remote);
			name_mangle(outbuf);
			strcpy(txbuf + txlen, outbuf);
			free(outbuf);
			for(; txbuf[txlen]; txlen++) {
			    txbuf[txlen] = tolower(txbuf[txlen]);
			}
			txlen++;

			strcpy(txbuf + txlen, to_send->remote);
			txlen += strlen(to_send->remote) + 1;
		    } else {
			txbuf[0] = 0;
			txlen = 1;
		    }

		    txcompressed = 0;
		    hytxpkt(HPKT_FINFO, txbuf, txlen);

		    if (txretries > 0) {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER/2);
			SETTIMER(TIMERNO_TX, H_MINTIMER/2);
		    } else {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
			SETTIMER(TIMERNO_TX, H_MINTIMER);
		    }

		    txstate = HTX_FINFOACK;
		}
		break;

	    /*
	     * get FINFOACK packet
	     */	
	    case HTX_FINFOACK: 
		Syslog('h', "SM 'HTX' entering 'FINFOACK'");
		if ((pkttype == HPKT_FINFOACK) && (rxlen == 4)) {
		    txpos = get_long(rxbuf);
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);

		    if (to_send == NULL) {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
			SETTIMER(TIMERNO_TX, H_MINTIMER);
			txstate = HTX_REND;
		    } else if (txpos >= 0L) {
			if (txpos > 0L)
			    Syslog('+', "Hydra: restart from %lu", txpos);
			stxpos = txpos;
			txretries = 0;
			txlastack = 0;
			Syslog('h', "Hydra: reset TX timer");
			RESETTIMER(TIMERNO_TX);
			txstate = HTX_DATA;
		    } else if (txpos == -1) {
			Syslog('+', "Hydra: Receiver already has this file, skipping");
			fclose(txfp);
			execute_disposition(to_send);
			to_send = to_send->next;
			txstate = HTX_NextFile;
		    } else if (txpos == -2) {
			Syslog('+', "Hydra: receiver requested to skip this file for now");
			fclose(txfp);
			to_send = to_send->next;
			txstate = HTX_NextFile;
		    }

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('+', "Hydra: transmitter timeout (HTX_FINFOACK)");
		    txretries++;
		    txstate = HTX_FINFO;
		}
		break;

	    case HTX_DATA: 
		Syslog('h', "SM 'HTX' entering 'DATA'");
		Syslog('h', "txwindow=%d txpos=%d txlastack=%d", txwindow, txpos, txlastack);
		if ((rxstate != HRX_DONE) && (hdxlink)) {
		    Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
		    SETTIMER(TIMERNO_TX, H_MINTIMER);
		    txstate = HTX_XWAIT;
		} else if ((pkttype == HPKT_DATAACK) && (rxlen == 4)) {
		    longnum = get_long(rxbuf);

		    Syslog('h', "received 'DATAACK' (0x%08lx)", longnum);

		    if (longnum > txlastack) {
			txlastack = longnum;
		    }

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if ((txwindow) && (txpos >= txlastack + txwindow)) {
		    /*
		     * We have to wait for an ACK before we can continue
		     */
		    if (txretries > 0) {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER/2);
			SETTIMER(TIMERNO_TX, H_MINTIMER/2);
		    } else {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
			SETTIMER(TIMERNO_TX, H_MINTIMER);
		    }
		    txstate = HTX_DATAACK;
		    break;
		} else {
		    /*
		     * check if there is enough room in output
		     */
		    if ((waitputget & 2) == 0) {
			break;
		    }

		    fseek(txfp, txpos, SEEK_SET);
		    put_long(txbuf, txpos);
		    Nopper();
		    txlen = fread(txbuf + 4, 1, blksize, txfp);
		    Syslog('h', "Hydra: send DATA (0x%08lx) %lu", txpos, txlen);

		    if (txlen == 0) {
			if (ferror(txfp)) {
			    WriteError("$Hydra: error reading from file");
			    txstate = HTX_SkipFile;
			} else {
			    txstate = HTX_EOF;
			}
		    } else {
			Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);	// 03-11-2003 MB.
			SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);		// 03-11-2003 MB.
#ifdef HAVE_ZLIB_H
			if (compstate == HCMP_GZ) {
			    txzlen = H_ZIPBUFLEN - 4;
			    destLen = (uLongf)txzlen;
			    rcz = compress2((Bytef *)txzbuf + 4, &destLen, (Bytef *)txbuf + 4, txlen, 9);
			    txzlen = (int)destLen;
			    if (rcz == Z_OK) {
				Syslog('h', "Hydra: compressed OK, srclen=%d, destlen=%d, will send compressed=%s", txlen, txzlen,
					(txzlen < txlen) ?"yes":"no");
				if (txzlen < txlen) {
				    txcompressed += (txlen - txzlen);
				    put_long(txzbuf, txpos);
				    txpos += txlen;
				    sentbytes += txlen;
				    goodbytes += txlen;
				    txzlen += 4;
				    hytxpkt(HPKT_ZIPDATA, txzbuf, txzlen);
				    /*
				     * Calculate the perfect blocksize for the next block
				     * using the current compression ratio. This gives
				     * a dynamic optimal blocksize. The average maximum
				     * blocksize on the line will be 2048 bytes.
				     */
				    cmpblksize = ((txlen * 4) / txzlen) * 512;
				    if (cmpblksize < H_UNCBLKLEN)
					cmpblksize = H_UNCBLKLEN;
				    if (cmpblksize > H_MAXBLKLEN)
					cmpblksize = H_MAXBLKLEN;
				    Syslog('h', "Hydra: adjusting next blocksize to %d bytes", cmpblksize);
				} else {
				    txpos += txlen;
				    sentbytes += txlen;
				    goodbytes += txlen;
				    txlen += 4;
				    hytxpkt(HPKT_DATA, txbuf, txlen);
				    cmpblksize = H_UNCBLKLEN;
				}
			    } else {
				/*
				 * Compress failed, send data uncompressed
				 */
				Syslog('h', "Hydra: compress error");
				txpos += txlen;
				sentbytes += txlen;
				goodbytes += txlen;
				txlen += 4;
				hytxpkt(HPKT_DATA, txbuf, txlen);
				cmpblksize = H_UNCBLKLEN;
			    }
			} else {
			    /*
			     * Remote doesn't support PLZ, use standard hydra method.
			     */
			    txpos += txlen;
			    sentbytes += txlen;
			    goodbytes += txlen;
			    txlen += 4;
			    hytxpkt(HPKT_DATA, txbuf, txlen);
			    cmpblksize = H_UNCBLKLEN;
			}
			if (goodbytes > goodneeded) {
			    blksize *= 2;
			    if (compstate != HCMP_NONE) {
				if (blksize > cmpblksize) {
				    blksize = cmpblksize;
				}
			    } else {
				if (blksize > H_UNCBLKLEN) {
				    blksize = H_UNCBLKLEN;
				}
			    }
			}
#else
			txpos += txlen;
			sentbytes += txlen;
			goodbytes += txlen;
			txlen += 4;
			hytxpkt(HPKT_DATA, txbuf, txlen);
			if (goodbytes > goodneeded) {
			    blksize *= 2;
			    if (blksize > H_UNCBLKLEN) {
				blksize = H_UNCBLKLEN;
			    }
			}
#endif
		    }
		}
		break;

	    case HTX_SkipFile: 
		Syslog('h', "SM 'HTX' entering 'SkipFile'");
		/*
		 * this differs from the protocol definition: -2 is used
		 * instead of -1 (I think it's wrong in the protocol
		 * definition as in the example source also -2 is used)
		 * MB: No, I don't think this is wrong, -1 means file already
		 * there, -2 means skip for now, try another time.
		 */
		txpos = -2;
		txretries = 0;
		txstate = HTX_EOF;
		break;

	    case HTX_DATAACK: 
		Syslog('h', "SM 'HTX' entering 'DATAACK'");
		if ((pkttype == HPKT_DATAACK) && (rxlen == 4)) {
		    longnum = get_long(rxbuf);

		    if ((longnum > txlastack) && (txpos < longnum + txwindow)) {
			txlastack = longnum;
			txretries = 0;
			Syslog('h', "Hydra: reset TX timer");
			RESETTIMER(TIMERNO_TX);
			txstate = HTX_DATA;
		    }

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (txretries >= 10) {
		    Syslog('+', "Hydra: too many errors");
		    txstate = HTX_Abort;
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('+', "Hydra: tx timeout");
		    txretries++;
		    txstate = HTX_DATA;
		}
		break;

	    case HTX_XWAIT: 
		Syslog('h', "SM 'HTX' entering 'XWAIT'");
		if (rxstate == HRX_DONE) {
		    Syslog('h', "Hydra: reset RX timer");
		    RESETTIMER(TIMERNO_RX);
		    txstate = HTX_DATA;
		} else if ((pkttype == HPKT_DATAACK) && (rxlen == 4)) {
		    longnum = get_long(rxbuf);
		    if (longnum > txlastack) {
			txlastack = longnum;
		    }
		    pkttype = H_NOPKT;	/* packet has already been processed */

		} else if (pkttype == HPKT_RPOS) {
		    /*
		     * Handle RPOS in state DATA but stay in this state
		     */
		    pkttype = H_NOPKT;      /* packet has already been processed */

		} else if ((pkttype == HPKT_IDLE) && (rxlen == 0)) {
		    hdxlink = FALSE;
		    Syslog('h', "Hydra: reset TX timer");
		    RESETTIMER(TIMERNO_TX);
		    txstate = HTX_DATA;
		    pkttype = H_NOPKT;	/* packet has already been processed */

		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('h', "Hydra: TX timer expired");
		    hytxpkt(HPKT_IDLE, txbuf, 0);
		    Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
		    SETTIMER(TIMERNO_TX, H_MINTIMER);
		}
		break;

	    case HTX_EOF: 
		Syslog('h', "SM 'HTX' entering 'EOF'");
		if (txretries >= 10) {
		    txstate = HTX_Abort;
		    break;
		} else {
		    put_long(txbuf, txpos);
		    hytxpkt(HPKT_EOF, txbuf, 4);

		    if (txretries > 0) {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER/2);
			SETTIMER(TIMERNO_TX, H_MINTIMER/2);
		    } else {
			Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
			SETTIMER(TIMERNO_TX, H_MINTIMER);
		    }
		    Syslog('h', "Hydra: entering TX EOFACK");
		    txstate = HTX_EOFACK;
		}
		break;

	    case HTX_EOFACK: 
		Syslog('h', "SM 'HTX' entering 'EOFACK'");
		if ((pkttype == HPKT_EOFACK) && (rxlen == 0)) {
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);

		    /*
		     * calculate time needed and bytes transferred
		     */
		    gettimeofday(&txendtime, &tz);

		    /* close transmitter file */
		    fclose(txfp);

		    if (txpos >= 0) {
			stxpos = txpos - stxpos;
			if (txcompressed && (compstate != HCMP_NONE))
			    Syslog('+', "Hydra: %s", compress_stat(stxpos, txcompressed));
			Syslog('+', "Hydra: OK %s", transfertime(txstarttime, txendtime, stxpos, TRUE));
			execute_disposition(to_send);
		    } else {
			Syslog('+', "Hydra: transmitter skipped file after %ld seconds", 
			txendtime.tv_sec - txstarttime.tv_sec);
		    }

		    to_send = to_send->next;
		    txstate = HTX_NextFile;

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if ((pkttype == HPKT_DATAACK) && (rxlen == 4)) {
		    longnum = get_long(rxbuf);
		    txlastack = longnum;

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('+', "Hydra: transmitter timeout (HTX_EOFACK)");
		    txretries++;
		    txstate = HTX_EOF;
		}
		break;

	    case HTX_REND: 
		Syslog('h', "SM 'HTX' entering 'REND'");
		if (rxstate == HRX_DONE) {
		    txretries = 0;
		    txstate = HTX_END;
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('h', "Hydra: TX timer expired");
		    hytxpkt(HPKT_IDLE, txbuf, 0);
		    Syslog('h', "Hydra: set TX timer %d", H_MINTIMER/2);
		    SETTIMER(TIMERNO_TX, H_MINTIMER/2);
		}
		break;

	    case HTX_END: 
		Syslog('h', "SM 'HTX' entering 'END'");
		if (txretries >= 10) {
		    txstate = HTX_Abort;
		    break;
		} else {
		    hytxpkt(HPKT_END, txbuf, 0);
		    hytxpkt(HPKT_END, txbuf, 0);
		    Syslog('h', "Hydra: set TX timer %d", H_MINTIMER/2);
		    SETTIMER(TIMERNO_TX, H_MINTIMER/2);
		    txstate = HTX_ENDACK;
		}
		break;

	    case HTX_ENDACK: 
		Syslog('h', "SM 'HTX' entering 'ENDACK'");
		if ((pkttype == HPKT_END) && (rxlen == 0)) {
		    hytxpkt(HPKT_END, txbuf, 0);
		    hytxpkt(HPKT_END, txbuf, 0);
		    hytxpkt(HPKT_END, txbuf, 0);
		    txstate = HTX_DONE;

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (EXPIRED(TIMERNO_TX)) {
		    Syslog('+', "Hydra: transmitter timeout (HTX_ENDACK)");
		    txretries++;
		    txstate = HTX_END;
		}
		break;

	    case HTX_DONE:
	    case HTX_Abort:
		/* nothing to do */
		break;

	    default:
		die(MBERR_FTRANSFER);
	} /* switch (txstate) */

	switch (rxstate) {
	    case HRX_INIT: 
		Syslog('h', "SM 'HRX' entering 'INIT'");
		if (pkttype == HPKT_INIT) {
		    /* TODO: some checking, error handling */

		    /* Skip application ID */
		    Syslog('+', "Hydra: remote \"%s\"", rxbuf+8);
		    inbuf = rxbuf + strlen(rxbuf) + 1;

		    inbuf += strlen(inbuf) + 1;
	
		    rxoptions = (HDEF_OPTIONS & HCAN_OPTIONS) | HUNN_OPTIONS;

		    /*
		     * get supported options
		     */
		    rxoptions |= get_flags(inbuf);
		    inbuf += strlen(inbuf) + 1;

		    /*
		     * get desired options
		     */
		    rxoptions &= get_flags(rxbuf + strlen(rxbuf) + 1);
		    rxoptions &= HCAN_OPTIONS;

		    /*
		     * set options
		     */
		    txoptions = rxoptions;
		    put_flags(txbuf, rxoptions);
		    Syslog('+', "Hydra: options: %s (%08lx)", txbuf, rxoptions);

#ifdef HAVE_ZLIB_H
		    /*
		     * Set zlib gzip compression mode
		     */
		    if (txoptions & HOPT_CANPLZ) {
			Syslog('h', "Hydra: compstate => HCMP_GZ");
			compstate = HCMP_GZ;
		    }
#endif

		    /*
 		     * get desired window sizes
		     */
		    txwindow = rxwindow = 0;
		    sscanf(inbuf, "%08x%08x", &rxwindow, &txwindow);

		    if (rxwindow < 0)
			rxwindow = 0;

		    if (H_RXWINDOW && ((rxwindow == 0) || (rxwindow > H_RXWINDOW)))
			rxwindow = H_RXWINDOW;

		    if (txwindow < 0)
			txwindow = 0;

		    if (H_TXWINDOW && ((txwindow == 0) || (txwindow > H_TXWINDOW)))
			txwindow = H_TXWINDOW;

		    Syslog('h', "Hydra: txwindow=%d, rxwindow=%d", txwindow, rxwindow);

		    hytxpkt(HPKT_INITACK, txbuf, 0);
		    Syslog('h', "Hydra: set BRAIN timer", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		    rxstate = HRX_FINFO;

		    pkttype = H_NOPKT;	/* packet has already been processed */
		}
		break;

	    case HRX_FINFO: 
		Syslog('h', "SM 'HRX' entering 'FINFO'");
		if (pkttype == HPKT_FINFO) {
		    /*
		     * check if we have received an `End of batch' FINFO packet
		     */
		    if ((rxlen == 1) && (rxbuf[0] == 0)) {
			put_long(txbuf, 0);
			hytxpkt(HPKT_FINFOACK, txbuf, 4);
		
			Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
			SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
			rxstate = HRX_DONE;
		    }
		    /*
		     * check if FINFO packet is at least long enough to contain
		     * file information
		     */
		    else if ((rxlen > 41) && (rxbuf[rxlen - 1] == 0)) {
			time_t timestamp;
			time_t orgstamp;
			int filesize, tt;
			char dosname[8 + 1 + 3 + 1], *Name;

			sscanf(rxbuf, "%08x%08x%*08x%*08x%*08x",  &tt, &filesize);
			timestamp = (time_t)tt;

			/* convert timestamp to UNIX time */
			orgstamp = timestamp;
			timestamp = sl2mtime(timestamp);

			/*
			 * check if DOS conforming file name is max. 8+1+3 chars long
			 */
			if (strlen(rxbuf + 40) <= 12) {
			    strcpy(dosname, rxbuf + 40);
			} else {
			    strcpy(dosname, "BadWazoo");
			}

			/*
			 * check if real file name is specified
			 */
			if ((strlen(rxbuf + 40) + 41) < rxlen) {
			    Name = rxbuf + strlen(rxbuf + 40) + 41;

			    /*
			     * use DOS-filename if real- and DOS-name only
			     * differ in case sensitivity
			     */
			    if (strcasecmp(Name, dosname) == 0) {
				Name = dosname;
			    }
			} else {
			    Name = dosname;
			}

			Syslog('+', "Hydra: receive \"%s\" (%ld bytes) dated %s", Name, filesize, date(timestamp));
			rxfp = openfile(Name, timestamp, filesize, &rxpos, resync);
			gettimeofday(&rxstarttime, &tz);

			/* check for error opening file */
			if (rxfp) {
			    srxpos = rxpos;
			    rxstate = HRX_ToData;
			} else {
			    if (filesize == rxpos) {
				/* Skip this file, it's already here */
				Syslog('+', "Hydra: Skipping file %s", Name);
				put_long(txbuf, -1);
			    } else {
				/* Skip this file for now if error opening file */
				put_long(txbuf, -2);
			    }
			    hytxpkt(HPKT_FINFOACK, txbuf, 4);
	
			    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
			    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
			}
		    }

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (pkttype == HPKT_INIT) {
		    hytxpkt(HPKT_INITACK, txbuf, 0);

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if ((pkttype == HPKT_EOF) && (rxlen == 4)) {
		    hytxpkt(HPKT_EOFACK, txbuf, 0);

		    pkttype = H_NOPKT;	/* packet has already been processed */
		}
		break;

	    case HRX_ToData: 
		Syslog('h', "SM 'HRX' entering 'ToData'");
		put_long(txbuf, rxpos);
		hytxpkt(HPKT_FINFOACK, txbuf, 4);

		rxsyncid = 0;
		rxlastsync = 0;
		rxretries = 0;
		Syslog('h', "Hydra: reset RX timer");
		RESETTIMER(TIMERNO_RX);
		Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		rxstate = HRX_DATA;
		break;

	    case HRX_DATA: 
		Syslog('h', "SM 'HRX' entering 'DATA'");
#ifdef HAVE_ZLIB_H
		if (((pkttype == HPKT_DATA) || (pkttype == HPKT_ZIPDATA)) && (rxlen > 4)) {
		    /*
		     * If data packet is a zlib compressed packet, uncompress it first.
		     */
		    if (pkttype == HPKT_ZIPDATA) {
			rxzlen = H_ZIPBUFLEN;
			destLen = (uLongf)rxzlen;
			rcz = uncompress((Bytef *)rxzbuf, &destLen, (Bytef *)rxbuf + 4, rxlen - 4);
			rxzlen = (int)destLen;
			if (rcz == Z_OK) {
			    /*
			     * Uncompress data and put the data into the normal receive buffer.
			     */
			    Syslog('h', "Hydra: uncompressed size %d => %d", rxlen -4, rxzlen);
			    memcpy(rxbuf + 4, rxzbuf, rxzlen);
			    rxlen = rxzlen + 4;
			} else {
			    /*
			     * Send BadPos if uncompress failed, the transmitter should
			     * resent the block without compression.
			     */
			    Syslog('+', "Hydra: ZIPDATA uncompress error, sending BadPos");
			    longnum = get_long(rxbuf);
			    rxstate = HRX_BadPos;
			    rxctries++;
			    pkttype = H_NOPKT;  /* packet has already been processed */
			    break;
			}
		    }
		    longnum = get_long(rxbuf);
		    Syslog('h', "Hydra: rcvd %sDATA (0x%08lx, 0x%08lx) %lu", (pkttype == HPKT_ZIPDATA) ? "ZIP":"",
			    longnum, rxpos, rxlen-4);
#else
		if ((pkttype == HPKT_DATA) && (rxlen > 4)) {
		    longnum = get_long(rxbuf);
		    Syslog('h', "Hydra: rcvd DATA (0x%08lx, 0x%08lx) %lu", longnum, rxpos, rxlen-4);
#endif
		    Nopper();
		    Syslog('h', "Hydra: longnum=%d, rxpos=%d", longnum, rxpos);
		    if (longnum == rxpos) {
			if (fwrite(rxbuf + 4, 1, rxlen - 4, rxfp) != (rxlen - 4)) {
			    WriteError("$Hydra: error writing to file");
			    rxpos = -2;
			} else {
			    rxlastdatalen = rxlen - 4;
			    rxpos += rxlen - 4;
			    rcvdbytes += rxlen - 4;
			    rxretries = 0;
			    rxlastsync = rxpos;
			    Syslog('h', "Hydra: reset RX timer");
			    RESETTIMER(TIMERNO_RX);
			    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
			    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
			    if (rxwindow) {
				put_long(txbuf, rxpos);
				hytxpkt(HPKT_DATAACK, txbuf, 4);
			    }
			}
		    } else {
			if (rxpos >= 0) {
			    Syslog('+', "Hydra: received bad rxpos %d", rxpos);
			}
			rxstate = HRX_BadPos;
		    }

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if ((pkttype == HPKT_EOF) && (rxlen == 4)) {
		    longnum = get_long(rxbuf);
	
		    if (longnum == rxpos) {
			/*
			 * calculate time and CPU usage needed
			 */
			gettimeofday(&rxendtime, &tz);
	
			if (rxpos >= 0) {
			    rxfp = NULL;
			    if (!closefile()) {
				srxpos = rxpos - srxpos;

				Syslog('+', "Hydra: OK %s", transfertime(rxstarttime, rxendtime, srxpos, FALSE));

				rxstate = HRX_OkEOF;
			    } else {
				Syslog('+', "Hydra: error closing file");
				rxpos = -2;

				/*
				 * Note: the following state change isn't 100 %
				 * conformant with the Hydra protocol specification.
				 */
				rxstate = HRX_BadPos;
			    }
			} else {
			    Syslog('+', "Hydra: receiver skipped file after %ld seconds", rxendtime.tv_sec - rxstarttime.tv_sec);

			    if (rxfp) {
				closefile();
				rxfp = NULL;
			    }

			    rxstate = HRX_OkEOF;
			}
		    } else if (longnum == -2) {
			if (rxfp) {
			    closefile();
			    rxfp = NULL;
			}

			rxstate = HRX_OkEOF;
		    } else {
			if (longnum >= 0) {
			    Syslog('+', "Hydra: received bad rxpos");
			}
			rxstate = HRX_BadPos;	/* TODO: fallthrough */
		    }

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (pkttype == HPKT_FINFO) {
		    put_long(txbuf, rxpos);
		    hytxpkt(HPKT_FINFOACK, txbuf, 4);

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if ((pkttype == HPKT_IDLE) && (rxlen == 0) && (hdxlink == FALSE)) {
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if (EXPIRED(TIMERNO_RX)) {
		    Syslog('h', "Hydra: RX timer expired");
		    rxstate = HRX_HdxLink;
		}
		break;

	    case HRX_BadPos: 
		Syslog('h', "SM 'HRX' entering 'BadPos'");
		longnum = get_long(rxbuf);

		if (longnum <= rxlastsync) {
		    rxretries = 0;
		    Syslog('h', "Hydra: reset RX timer");
		    RESETTIMER(TIMERNO_RX);
		}
		rxlastsync = longnum;
		rxstate = HRX_Timer;
		break;

	    case HRX_Timer: 
		Syslog('h', "SM 'HRX' entering 'Timer'");
		if ((!RUNNING(TIMERNO_RX)) || (EXPIRED(TIMERNO_RX))) {
		    Syslog('h', "Hydra: RX timer expired");
		    rxstate = HRX_HdxLink;
		} else {
		    rxstate = HRX_DATA;
		}
		break;

	    case HRX_HdxLink: 
		Syslog('h', "SM 'HRX' entering 'HdxLink'");
		if ((rxretries > 4) && (txstate != HTX_REND) && (role == 0) && (hdxlink == FALSE)) {
		    rxretries = 0;
		    hdxlink = TRUE;
		}
		rxstate = HRX_Retries;
		break;	/* TODO: fallthrough */

	    case HRX_Retries:
		rxretries++;
		Syslog('h', "SM 'HRX' entering 'Retries' (%d)", rxretries);
		if (rxretries >= 10) {
		    Syslog('+', "Hydra: too many errors");
		    txstate = HTX_Abort;
		} else if (rxretries == 1) {
		    rxsyncid++;
		}
		rxstate = HRX_RPos;
		break;	/* TODO: fallthrough */

	    case HRX_RPos: 
		Syslog('h', "SM 'HRX' entering 'RPos'");
		rxlastdatalen /= 2;
		if (rxlastdatalen < 64)
		    rxlastdatalen = 64;

		put_long(txbuf, rxpos);
		put_long(txbuf + 4, rxlastdatalen);
#ifdef HAVE_ZLIB_H
		/*
		 * FIXME: after some errors and we are in gzip compression
		 * mode we should send ID -1 to instruct the remote to
		 * stop compression mode.
		 */
		if ((compstate != HCMP_NONE) && (rxctries > 2)) {
		    Syslog('+', "Hydra: too much compress errors, instructing remote to stop compression");
		    put_long(txbuf + 8, (int)-1L);
		} else {
		    put_long(txbuf + 8, rxsyncid);
		}
#else
		put_long(txbuf + 8, rxsyncid);
#endif
		hytxpkt(HPKT_RPOS, txbuf, 12);
		Syslog('h', "Hydra: set TX timer %d", H_MINTIMER);
		SETTIMER(TIMERNO_RX, H_MINTIMER);
		rxstate = HRX_DATA;
		break;

	    case HRX_OkEOF: 
		Syslog('h', "SM 'HRX' entering 'OkEOF'");
		hytxpkt(HPKT_EOFACK, txbuf, 0);
		Syslog('h', "Hydra: reset RX timer");
		RESETTIMER(TIMERNO_RX);
		Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);
		rxstate = HRX_FINFO;
		break;

	    case HRX_DONE: 
		Syslog('h', "SM 'HRX' entering 'DONE'");
		if (pkttype == HPKT_FINFO) {
		    put_long(txbuf, -2);
		    hytxpkt(HPKT_FINFOACK, txbuf, 4);

		    pkttype = H_NOPKT;	/* packet has already been processed */
		} else if ((pkttype == HPKT_IDLE) && (rxlen == 0)) {
		    Syslog('h', "Hydra: set BRAIN timer %d", H_BRAINDEAD);
		    SETTIMER(TIMERNO_BRAIN, H_BRAINDEAD);

		    pkttype = H_NOPKT;	/* packet has already been processed */
		}
		break;

	} /* switch(rxstate) */

	if (pkttype != H_NOPKT) {
	    Syslog('h', "Hydra: rcvd packet %s - ignored", PktS(pkttype));
	    pkttype = H_NOPKT;	/* ignore received packet */
	}

    } /* while() */

    Syslog('h', "Hydra: resettimers");
    RESETTIMERS();

    if (txstate == HTX_Abort) {
	/* check if file is still open */
	if (rxfp) {
	    rxfp = NULL;
	    closefile();
	}

	Syslog('+', "Hydra: signal CAN to remote");
	FLUSHOUT();
	/* 8 times CAN and 10 times BS */
	PUT((char *)"\030\030\030\030\030\030\030\030\010\010\010\010\010\010\010\010\010\010", 18);
	sleep(4);				/* wait a few seconds... */
	FLUSHIN();

	return MBERR_FTRANSFER;
    }

    return MBERR_OK;
}



int hydra(int role)
{
    int		rc;
    fa_list	*eff_remote, tmpl;
    file_list	*tosend = NULL, *request = NULL, *respond = NULL, *tmpfl;
    char	*nonhold_mail;

    Syslog('+', "Hydra: start transfer");
    session_flags |= SESSION_HYDRA;		/* Hydra special file requests */

    if (emsi_remote_lcodes & LCODE_NPU) {
	Syslog('+', "Hydra: remote requested \"no pickup\", no send");
	eff_remote = NULL;
    } else if (emsi_remote_lcodes & LCODE_PUP) {
	Syslog('+', "Hydra: remote requested \"pickup primary\"");
	tmpl.addr = remote->addr;
	tmpl.next = NULL;
	eff_remote = &tmpl;
    } else {
	eff_remote = remote;
    }

    nonhold_mail = (char *)ALL_MAIL;

    if (emsi_remote_lcodes & LCODE_HAT) {
	Syslog('+', "Hydra: remote requested \"hold all traffic\", no send");
        tosend = NULL;
    } else {
        tosend = create_filelist(eff_remote, nonhold_mail, 0);
    }

    if (session_flags & SESSION_WAZOO)
        request = create_freqlist(remote);

    Syslog('h', "H_BUFLEN=%d H_ZIPBUFLEN=%d", H_BUFLEN, H_ZIPBUFLEN);


    /*
     * Send only file requests during first batch if remote supports
     * "RH1" flag.
     */
    if (emsi_remote_lcodes & LCODE_RH1) {
	rc = hydra_batch(role, request);
    } else {
	if (request != NULL) {
	    tmpfl = tosend;
	    tosend = request;
	    for (; request->next; request = request->next);
		request->next = tmpfl;

	    request = NULL;
	}    

	rc = hydra_batch(role, tosend);
    }

    Syslog('+', "Hydra: start second batch");

    if (rc == 0) {
	if ((emsi_local_opts & OPT_NRQ) == 0)
	    respond = respond_wazoo();

	if (emsi_remote_lcodes & LCODE_RH1) {
	    for (tmpfl = tosend; tmpfl->next; tmpfl = tmpfl->next);
		tmpfl->next = respond;

	    rc = hydra_batch(role, tosend);
	    tmpfl->next = NULL;	/* split filelist into tosend and respond again */
	} else {
	    rc = hydra_batch(role, respond);
	}
    }

    tidy_filelist(request, (rc == 0));
    tidy_filelist(tosend, (rc == 0));
    tidy_filelist(respond, 0);

    Syslog('+', "Hydra: end transfer");

    return rc;
}


