/*
 * $Id: zmmisc.c,v 1.7 2005/10/11 20:49:46 mbse Exp $
 *
 *   Z M . C
 *    Copyright 1994 Omen Technology Inc All Rights Reserved
 *    ZMODEM protocol primitives
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 * 
 *
 *	This version implements numerous enhancements including ZMODEM
 *	Run Length Encoding and variable length headers.  These
 *	features were not funded by the original Telenet development
 *	contract.
 * 
 *  This software may be freely used for educational (didactic
 *  only) purposes.  This software may also be freely used to
 *  support file transfer operations to or from licensed Omen
 *  Technology products.  Use with other commercial or shareware
 *  programs (Crosstalk, Procomm, etc.) REQUIRES REGISTRATION.
 *
 *  Any programs which use part or all of this software must be
 *  provided in source form with this notice intact except by
 *  written permission from Omen Technology Incorporated.
 *
 * Use of this software for commercial or administrative purposes
 * except when exclusively limited to interfacing Omen Technology
 * products requires a per port license payment of $20.00 US per
 * port (less in quantity).  Use of this code by inclusion,
 * decompilation, reverse engineering or any other means
 * constitutes agreement to these conditions and acceptance of
 * liability to license the materials and payment of reasonable
 * legal costs necessary to enforce this license agreement.
 *
 *
 *              Omen Technology Inc
 *              Post Office Box 4681
 *              Portland OR 97208
 *
 *      This code is made available in the hope it will be useful,
 *      BUT WITHOUT ANY WARRANTY OF ANY KIND OR LIABILITY FOR ANY
 *      DAMAGES OF ANY KIND.
 *
 */

static void zputhex(int);
static void zsbh32(int,char*,int,int);
static void zsda32(char*,int,int);
static int zrdat32(char*,int);
static int noxrd7(void);
static int zrbhd32(char*);
static int zrbhdr(char*);
static int zrhhdr(char*);
static int zgethex(void);
static int zgeth1(void);
static void garbitch(void);

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "zmodem.h"


/* Original zm.c timing was in tenths of seconds, but our current ttyio driver
   does timing in whole seconds.
*/
static int Rxtimeout = 10;	/* Seconds to wait for something */
int Zctlesc;

/* Globals used by ZMODEM functions */
int Rxframeind;		/* ZBIN ZBIN32, or ZHEX type of frame */
int Rxtype;		/* Type of header received */
int Rxhlen;		/* Length of header received */
int Rxcount;		/* Count of data bytes received */
char Rxhdr[ZMAXHLEN];	/* Received header */
char Txhdr[ZMAXHLEN];	/* Transmitted header */
int Rxpos;		/* Received file position */
int Txpos;		/* Transmitted file position */
int Txfcs32;		/* TRUE means send binary frames with 32 bit FCS */
int Crc32t;		/* Controls 32 bit CRC being sent */
			/* 1 == CRC32,  2 == CRC32 + RLE */
int Crc32r;		/* Indicates/controls 32 bit CRC being received */
			/* 0 == CRC16,  1 == CRC32,  2 == CRC32 + RLE */
int Usevhdrs;		/* Use variable length headers */
int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */
char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
char *Altcan;		/* Alternate canit string */

char *txbuf=NULL;
char *rxbuf=NULL;

static int lastsent;	/* Last char we sent */
static int Not8bit;	/* Seven bits seen on header */

char *frametypes[] = {
	(char *)"EMPTY",		/* -16 */
	(char *)"Can't be (-15)",
	(char *)"Can't be (-14)",
	(char *)"Can't be (-13)",
	(char *)"Can't be (-12)",
	(char *)"Can't be (-11)",
	(char *)"Can't be (-10)",
	(char *)"Can't be (-9)",
	(char *)"HANGUP",		/* -8 */
	(char *)"Can't be (-7)",
	(char *)"Can't be (-6)",
	(char *)"Can't be (-5)",
	(char *)"EOFILE",		/* -4 */
	(char *)"Can't be (-3)",
	(char *)"TIMEOUT",		/* -2 */
	(char *)"ERROR",		/* -1 */
	(char *)"ZRQINIT",
	(char *)"ZRINIT",
	(char *)"ZSINIT",
	(char *)"ZACK",
	(char *)"ZFILE",
	(char *)"ZSKIP",
	(char *)"ZNAK",
	(char *)"ZABORT",
	(char *)"ZFIN",
	(char *)"ZRPOS",
	(char *)"ZDATA",
	(char *)"ZEOF",
	(char *)"ZFERR",
	(char *)"ZCRC",
	(char *)"ZCHALLENGE",
	(char *)"ZCOMPL",
	(char *)"ZCAN",
	(char *)"ZFREECNT",
	(char *)"ZCOMMAND",
	(char *)"ZSTDERR",
	(char *)"xxxxx"
#define FRTYPES 22	/* Total number of frame types in this array */
			/*  not including psuedo negative entries */
};


/***** Hack by mj ***********************************************************/
/*
 * Buffer for outgoing frames. Sending them with single character write()'s
 * is a waste of processor time and causes severe performance degradation
 * on TCP and ISDN connections.
 */
#define FRAME_BUFFER_SIZE	16384
static char *frame_buffer=NULL;
static int  frame_length = 0;

#define BUFFER_CLEAR()	do { frame_length=0; } while(0)
#define BUFFER_BYTE(c)	do { frame_buffer[frame_length++]=(c); } while(0)
#define BUFFER_FLUSH()	do { PUT(frame_buffer, frame_length); \
				 frame_length=0; } while(0);
/****************************************************************************/

void get_frame_buffer(void)
{
	if (frame_buffer == NULL) 
		frame_buffer = malloc(FRAME_BUFFER_SIZE);
}


void free_frame_buffer(void)
{
	if (frame_buffer)
		free(frame_buffer);
	frame_buffer = NULL;
}



/*
 * Send ZMODEM binary header hdr of type type
 */
void zsbhdr(int len, int type, register char *shdr)
{
	register int		n;
	register unsigned short	crc;

	Syslog('z', "zsbhdr: %c %d %s %lx", Usevhdrs?'v':'f', len, frametypes[type+FTOFFSET], rclhdr(shdr));

	BUFFER_CLEAR();
	
	if (type == ZDATA)
		for (n = Znulls; --n >=0; )
			BUFFER_BYTE(0);

	BUFFER_BYTE(ZPAD); BUFFER_BYTE(ZDLE);

	switch (Crc32t=Txfcs32) {
	case 2:
		zsbh32(len, shdr, type, Usevhdrs?ZVBINR32:ZBINR32);
		BUFFER_FLUSH(); break;
        case 1:
                zsbh32(len, shdr, type, Usevhdrs?ZVBIN32:ZBIN32);  break;
	default:
		if (Usevhdrs) {
			BUFFER_BYTE(ZVBIN);
			zsendline(len);
		}
		else
			BUFFER_BYTE(ZBIN);
		zsendline(type);
		crc = updcrc16(type, 0);

		for (n=len; --n >= 0; ++shdr) {
			zsendline(*shdr);
			crc = updcrc16((0377& *shdr), crc);
		}
		crc = updcrc16(0,updcrc16(0,crc));
		zsendline(((int)(crc>>8)));
		zsendline(crc);
	}

	BUFFER_FLUSH();
}



/*
 * Send ZMODEM binary header hdr of type type 
 */
void zsbh32(int len, register char *shdr, int type, int flavour)
{
	register int n;
	register unsigned int crc;

	BUFFER_BYTE(flavour); 
	if (Usevhdrs) 
		zsendline(len);
	zsendline(type);
	crc = 0xFFFFFFFFL; crc = updcrc32(type, crc);

	for (n=len; --n >= 0; ++shdr) {
		crc = updcrc32((0377 & *shdr), crc);
		zsendline(*shdr);
	}
	crc = ~crc;
	for (n=4; --n >= 0;) {
		zsendline((int)crc);
		crc >>= 8;
	}
}



/*
 * Send ZMODEM HEX header hdr of type type
 */
void zshhdr(int len, int type, register char *shdr)
{
	register int n;
	register unsigned short crc;

	Syslog('z', "zshhdr: %c %d %s %ld", Usevhdrs?'v':'f', len, frametypes[type+FTOFFSET], rclhdr(shdr));

	BUFFER_CLEAR();
	
	BUFFER_BYTE(ZPAD); 
	BUFFER_BYTE(ZPAD); 
	BUFFER_BYTE(ZDLE);
	if (Usevhdrs) {
		BUFFER_BYTE(ZVHEX);
		zputhex(len);
	}
	else
		BUFFER_BYTE(ZHEX);
	zputhex(type);
	Crc32t = 0;

	crc = updcrc16(type, 0);
	for (n=len; --n >= 0; ++shdr) {
		zputhex(*shdr); crc = updcrc16((0377 & *shdr), crc);
	}
	crc = updcrc16(0,updcrc16(0,crc));
	zputhex(((int)(crc>>8))); zputhex(crc);

	/*
	 * Make it printable on remote machine
	 */
	BUFFER_BYTE(015); 
	BUFFER_BYTE(0212);

	/*
	 * Uncork the remote in case a fake XOFF has stopped data flow
	 */
	if (type != ZFIN && type != ZACK)
		BUFFER_BYTE(021);

	BUFFER_FLUSH();
}



/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
char *Zendnames[] = {(char *)"ZCRCE",(char *)"ZCRCG",(char *)"ZCRCQ",(char *)"ZCRCW"};
void zsdata(register char *buf, int length, int frameend)
{
	register unsigned short crc;

	Syslog('z', "zsdata: %d %s", length, Zendnames[(frameend-ZCRCE)&3]);

	BUFFER_CLEAR();
	
	switch (Crc32t) {
        case 1:
                zsda32(buf, length, frameend);  break;
        case 2:
                zsdar32(buf, length, frameend);  break;
	default:
		crc = 0;
		for (;--length >= 0; ++buf) {
			zsendline(*buf); crc = updcrc16((0377 & *buf), crc);
		}
		BUFFER_BYTE(ZDLE); BUFFER_BYTE(frameend);
		crc = updcrc16(frameend, crc);

		crc = updcrc16(0,updcrc16(0,crc));
		zsendline(((int)(crc>>8))); zsendline(crc);
	}
	if (frameend == ZCRCW)
		BUFFER_BYTE(XON);

	BUFFER_FLUSH();
}



void zsda32(register char *buf, int length, int frameend)
{
	register int c;
	register unsigned int crc;

	crc = 0xFFFFFFFFL;
	for (;--length >= 0; ++buf) {
		c = *buf & 0377;
		if (c & 0140)
			BUFFER_BYTE(lastsent = c);
		else
			zsendline(c);
		crc = updcrc32(c, crc);
	}
	BUFFER_BYTE(ZDLE); 
	BUFFER_BYTE(frameend);
	crc = updcrc32(frameend, crc);

	crc = ~crc;
	for (c=4; --c >= 0;) {
		zsendline((int)crc);  crc >>= 8;
	}
}



/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 *  NB: On errors may store length+1 bytes!
 */
int zrdata(register char *buf, int length)
{
	register int c;
	register unsigned short crc;
	register char *end;
	register int d;

	switch (Crc32r) {
	case 1:
		return zrdat32(buf, length);
	case 2:
		return zrdatr32(buf, length);
	}

	crc = Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				crc = updcrc16((((d=c))&0377), crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc16(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc16(c, crc);
				if (crc & 0xFFFF) {
					Syslog('+', "Zmodem zrdata: Bad CRC");
					return TERROR;
				}
				Rxcount = length - (end - buf);
				Syslog('z', "zrdata: %d  %s", Rxcount, Zendnames[(d-GOTCRCE)&3]);
				return d;
			case GOTCAN:
				Syslog('+', "Zmodem: Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				Syslog('+', "Zmodem: TIMEOUT receiving data");
				return c;
			case HANGUP:
				Syslog('+', "Zmodem: Carrier lost while receiving");
				return c;
			default:
				garbitch(); 
				return c;
			}
		}
		*buf++ = c;
		crc = updcrc16(c, crc);
	}
	Syslog('+', "Zmodem: Data subpacket too long");
	return TERROR;
}



int zrdat32(register char *buf, int length)
{
	register int c;
	register unsigned int crc;
	register char *end;
	register int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				d = c;  c &= 0377;
				crc = updcrc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc32(c, crc);
				if (crc != 0xDEBB20E3) {
					Syslog('+', "Zmodem zrdat32: Bad CRC");
					return TERROR;
				}
				Rxcount = length - (end - buf);

				Syslog('z', "zrdat32: %d %s", Rxcount, Zendnames[(d-GOTCRCE)&3]);

				return d;
			case GOTCAN:
				Syslog('+', "Zmodem: Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				Syslog('+', "Zmodem: TIMEOUT");
				return c;
			case HANGUP:
				Syslog('+', "Zmodem: Carrier lost while receiving");
				return c;
			default:
				garbitch(); 
				return c;
			}
		}
		*buf++ = c;
		crc = updcrc32(c, crc);
	}
	Syslog('+', "Zmodem: Data subpacket too long");
	return TERROR;
}



void garbitch(void)
{
	Syslog('+', "Zmodem: Garbled data subpacket");
}



/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *
 *   Set Rxhlen to size of header (default 4) (valid if good hdr)
 *  On success, set Zmodem to 1, set Rxpos and return type of header.
 *   Otherwise return negative on error.
 *   Return ERROR instantly if ZCRCW sequence, for fast error recovery.
 */
int zgethdr(char *shdr)
{
	register int c, n, cancount;

	int Zrwindow = 1400;
	int Baudrate = 9600;
	n = Zrwindow + Baudrate;
	Rxframeind = Rxtype = 0;

startover:
	cancount = 5;
again:
	/*
	 * Return immediate ERROR if ZCRCW sequence seen 
	 */
	if (((c = GETCHAR(Rxtimeout)) < 0) && (c != TIMEOUT))
		goto fifi;
	else switch(c) {
	case 021: case 0221:
		goto again;
	case HANGUP:
	case TIMEOUT:
		goto fifi;
	case CAN:
gotcan:
		if (--cancount <= 0) {
			c = ZCAN; goto fifi;
		}
		switch (c = GETCHAR(Rxtimeout)) {
		case TIMEOUT:
			goto again;
		case ZCRCW:
			switch (GETCHAR(Rxtimeout)) {
			case TIMEOUT:
				c = TERROR; goto fifi;
			case HANGUP:
				goto fifi;
			default:
				goto agn2;
			}
		case HANGUP:
			goto fifi;
		default:
			break;
		case CAN:
			if (--cancount <= 0) {
				c = ZCAN; goto fifi;
			}
			goto again;
		}
	/* **** FALL THRU TO **** */
	default:
agn2:
#define GCOUNT (-4)
		if ( --n == 0) {
			c = GCOUNT; goto fifi;
		}
		goto startover;
	case ZPAD|0200:		/* This is what we want. */
		Not8bit = c;
	case ZPAD:		/* This is what we want. */
		break;
	}
	cancount = 5;
splat:
	switch (c = noxrd7()) {
	case ZPAD:
		goto splat;
	case HANGUP:
	case TIMEOUT:
		goto fifi;
	default:
		goto agn2;
	case ZDLE:		/* This is what we want. */
		break;
	}


	Rxhlen = 4;		/* Set default length */
	Rxframeind = c = noxrd7();
	switch (c) {
	case ZVBIN32:
		if ((Rxhlen = c = zdlread()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 1;  c = zrbhd32(shdr); break;
	case ZBIN32:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 1;  c = zrbhd32(shdr); break;
	case ZVBINR32:
		if ((Rxhlen = c = zdlread()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 2;  c = zrbhd32(shdr); break;
	case ZBINR32:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 2;  c = zrbhd32(shdr); break;
	case HANGUP:
	case TIMEOUT:
		goto fifi;
	case ZVBIN:
		if ((Rxhlen = c = zdlread()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 0;  c = zrbhdr(shdr); break;
	case ZBIN:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 0;  c = zrbhdr(shdr); break;
	case ZVHEX:
		if ((Rxhlen = c = zgethex()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 0;  c = zrhhdr(shdr); break;
	case ZHEX:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 0;  c = zrhhdr(shdr); break;
	case CAN:
		goto gotcan;
	default:
		goto agn2;
	}
	for (n = Rxhlen; ++n < ZMAXHLEN; )	/* Clear unused hdr bytes */
		shdr[n] = 0;
	Rxpos = shdr[ZP3] & 0377;
	Rxpos = (Rxpos<<8) + (shdr[ZP2] & 0377);
	Rxpos = (Rxpos<<8) + (shdr[ZP1] & 0377);
	Rxpos = (Rxpos<<8) + (shdr[ZP0] & 0377);
fifi:
	switch (c) {
	case GOTCAN:
		c = ZCAN;
	/* **** FALL THRU TO **** */
	case ZNAK:
	case ZCAN:
	case TERROR:
	case TIMEOUT:
	case HANGUP:
		Syslog('+', "Zmodem: Got %s", frametypes[c+FTOFFSET]);
	/* **** FALL THRU TO **** */
	default:
		if (c >= -FTOFFSET && c <= FRTYPES)
			Syslog('z', "zgethdr: %c %d %s %ld", Rxframeind, Rxhlen, frametypes[c+FTOFFSET], Rxpos);
		else
			Syslog('z', "zgethdr: %c %d %ld", Rxframeind, c, Rxpos);
	}
	/* Use variable length headers if we got one */
	if (c >= 0 && c <= FRTYPES && Rxframeind & 040)
		Usevhdrs = 1;
	return c;
}



/*
 * Receive a binary style header (type and position)
 */
int zrbhdr(register char *shdr)
{
	register int c, n;
	register unsigned short crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = updcrc16(c, 0);

	for (n=Rxhlen; --n >= 0; ++shdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc16(c, crc);
		*shdr = c;
	}
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc16(c, crc);
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc16(c, crc);
	if (crc & 0xFFFF) {
		Syslog('+', "Zmodem zrbhdr: Bad CRC");
		return TERROR;
	}
	return Rxtype;
}



/*
 * Receive a binary style header (type and position) with 32 bit FCS
 */
int zrbhd32(register char *shdr)
{
	register int c, n;
	register unsigned int crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = 0xFFFFFFFFL; crc = updcrc32(c, crc);

	for (n=Rxhlen; --n >= 0; ++shdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc32(c, crc);
		*shdr = c;
	}
	for (n=4; --n >= 0;) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc32(c, crc);
	}
	if (crc != 0xDEBB20E3) {
		Syslog('+', "Zmodem zrbhd32: Bad CRC");
		return TERROR;
	}
	return Rxtype;
}



/*
 * Receive a hex style header (type and position)
 */
int zrhhdr(char *shdr)
{
	register int c;
	register unsigned short crc;
	register int n;

	if ((c = zgethex()) < 0)
		return c;
	Rxtype = c;
	crc = updcrc16(c, 0);

	for (n=Rxhlen; --n >= 0; ++shdr) {
		if ((c = zgethex()) < 0)
			return c;
		crc = updcrc16(c, crc);
		*shdr = c;
	}
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc16(c, crc);
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc16(c, crc);
	if (crc & 0xFFFF) {
		Syslog('+', "Zmodem zrhhdr: Bad CRC"); 
		return TERROR;
	}
	switch (c = GETCHAR(Rxtimeout)) {
	case 0215:
		Not8bit = c;
		/* **** FALL THRU TO **** */
	case 015:
	 	/* Throw away possible cr/lf */
		switch (c = GETCHAR(Rxtimeout)) {
		case 012:
			Not8bit |= c;
		}
	}
	if (c < 0)
		return c;
	return Rxtype;
}



/*
 * Send a byte as two hex digits
 */
void zputhex(register int c)
{
	static char	digits[]	= "0123456789abcdef";

	BUFFER_BYTE(digits[(c&0xF0)>>4]);
	BUFFER_BYTE(digits[(c)&0xF]);
}



/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
void zsendline(int c)
{
	/* Quick check for non control characters */
	if (c & 0140)
		BUFFER_BYTE(lastsent = c);
	else {
		switch (c &= 0377) {
		case ZDLE:
			BUFFER_BYTE(ZDLE);
			BUFFER_BYTE (lastsent = (c ^= 0100));
			break;
		case 015:
		case 0215:
			if (!Zctlesc && (lastsent & 0177) != '@')
				goto sendit;
		/* **** FALL THRU TO **** */
		case 020:
		case 021:
		case 023:
		case 0220:
		case 0221:
		case 0223:
			BUFFER_BYTE(ZDLE);
			c ^= 0100;
	sendit:
			BUFFER_BYTE(lastsent = c);
			break;
		default:
			if (Zctlesc && ! (c & 0140)) {
				BUFFER_BYTE(ZDLE);
				c ^= 0100;
			}
			BUFFER_BYTE(lastsent = c);
		}
	}
}



/* Decode two lower case hex digits into an 8 bit byte value */
int zgethex(void)
{
	register int c;

	c = zgeth1();
	return c;
}



int zgeth1(void)
{
	register int c, n;

	if ((c = noxrd7()) < 0)
		return c;
	n = c - '0';
	if (n > 9)
		n -= ('a' - ':');
	if (n & ~0xF)
		return TERROR;
	if ((c = noxrd7()) < 0)
		return c;
	c -= '0';
	if (c > 9)
		c -= ('a' - ':');
	if (c & ~0xF)
		return TERROR;
	c += (n<<4);
	return c;
}



/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
int zdlread(void)
{
	register int c;

again:
	/* Quick check for non control characters */
	if ((c = GETCHAR(Rxtimeout)) & 0140)
		return c;
	switch (c) {
	case ZDLE:
		break;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again;
	default:
		if (Zctlesc && !(c & 0140)) {
			goto again;
		}
		return c;
	}
again2:
	if ((c = GETCHAR(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = GETCHAR(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = GETCHAR(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = GETCHAR(Rxtimeout)) < 0)
		return c;
	switch (c) {
	case CAN:
		return GOTCAN;
	case ZCRCE:
	case ZCRCG:
	case ZCRCQ:
	case ZCRCW:
		return (c | GOTOR);
	case ZRUB0:
		return 0177;
	case ZRUB1:
		return 0377;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again2;
	default:
		if (Zctlesc && ! (c & 0140)) {
			goto again2;
		}
		if ((c & 0140) ==  0100)
			return (c ^ 0100);
		break;
	}
	Syslog('+', "Zmodem: Bad escape sequence 0x%x", c);
	return TERROR;
}



/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
int noxrd7(void)
{
	register int c;

	for (;;) {
		if ((c = GETCHAR(Rxtimeout)) < 0)
			return c;
		switch (c &= 0177) {
		case XON:
		case XOFF:
			continue;
		default:
			if (Zctlesc && !(c & 0140))
				continue;
		case '\r':
		case '\n':
		case ZDLE:
			return c;
		}
	}
}



/*
 * Store long integer pos in Txhdr
 */
void stohdr(int pos)
{
	Txhdr[ZP0] = pos;
	Txhdr[ZP1] = pos>>8;
	Txhdr[ZP2] = pos>>16;
	Txhdr[ZP3] = pos>>24;
}



/*
 * Recover a long integer from a header
 */
int rclhdr(register char *shdr)
{
	register int l;

	l = (shdr[ZP3] & 0377);
	l = (l << 8) | (shdr[ZP2] & 0377);
	l = (l << 8) | (shdr[ZP1] & 0377);
	l = (l << 8) | (shdr[ZP0] & 0377);
	return l;
}

/* End of zmmisc.c */
