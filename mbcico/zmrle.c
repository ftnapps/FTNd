/*
 * File: zmr.c
 * Copyright 1988, 1994 Omen Technology Inc All Rights Reserved
 *
 *
 * 
 * This module implements ZMODEM Run Length Encoding, an
 * extension that was not funded by the original Telenet
 * development contract.
 * 
 * This software may be freely used for non commercial and
 * educational (didactic only) purposes.  This software may also
 * be freely used to support file transfer operations to or from
 * licensed Omen Technology products.  Any programs which use
 * part or all of this software must be provided in source form
 * with this notice intact except by written permission from Omen
 * Technology Incorporated.
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
 *		Omen Technology Inc		FAX: 503-621-3745
 *		Post Office Box 4681
 *		Portland OR 97208
 *
 *	This code is made available in the hope it will be useful,
 *	BUT WITHOUT ANY WARRANTY OF ANY KIND OR LIABILITY FOR ANY
 *	DAMAGES OF ANY KIND.
 *
 *	ZMODEM RLE compression and decompression functions
 */

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "zmodem.h"



/*
 * Send data subpacket RLE encoded with 32 bit FCS
 */
void zsdar32(char *buf, int length, int frameend)
{
	register int c, l, n;
	register unsigned int crc;

	crc = 0xFFFFFFFFL;  l = *buf++ & 0377;
	if (length == 1) {
		zsendline(l); crc = updcrc32(l, crc);
		if (l == ZRESC) {
			zsendline(1); crc = updcrc32(1, crc);
		}
	} else {
		for (n = 0; --length >= 0; ++buf) {
			if ((c = *buf & 0377) == l && n < 126 && length>0) {
				++n;  continue;
			}
			switch (n) {
			case 0:
				zsendline(l);
				crc = updcrc32(l, crc);
				if (l == ZRESC) {
					zsendline(0100); crc = updcrc32(0100, crc);
				}
				l = c; break;
			case 1:
				if (l != ZRESC) {
					zsendline(l); zsendline(l);
					crc = updcrc32(l, crc);
					crc = updcrc32(l, crc);
					n = 0; l = c; break;
				}
				/* **** FALL THRU TO **** */
			default:
				zsendline(ZRESC); crc = updcrc32(ZRESC, crc);
				if (l == 040 && n < 34) {
					n += 036;
					zsendline(n); crc = updcrc32(n, crc);
				}
				else {
					n += 0101;
					zsendline(n); crc = updcrc32(n, crc);
					zsendline(l); crc = updcrc32(l, crc);
				}
				n = 0; l = c; break;
			}
		}
	}
	PUTCHAR(ZDLE); PUTCHAR(frameend);
	crc = updcrc32(frameend, crc);

	crc = ~crc;
	for (length=4; --length >= 0;) {
		zsendline((int)crc);  crc >>= 8;
	}
}


/*
 * Receive data subpacket RLE encoded with 32 bit FCS
 */
int zrdatr32(register char *buf, int length)
{
	register int c;
	register unsigned int crc;
	register char *end;
	register int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	d = 0;	/* Use for RLE decoder state */
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
					Syslog('+', "Zmodem zrdatr32: Bad CRC");
					return TERROR;
				}
				Rxcount = length - (end - buf);

				Syslog('z', "zrdatr32: %d %s", Rxcount,
				  Zendnames[(d-GOTCRCE)&3]);

				return d;
			case GOTCAN:
				Syslog('+', "Zmodem: Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				Syslog('+', "Zmodem: TIMEOUT");
				return c;
			default:
				Syslog('+', "Zmodem: Bad data subpacket");
				return c;
			}
		}
		crc = updcrc32(c, crc);
		switch (d) {
		case 0:
			if (c == ZRESC) {
				d = -1;  continue;
			}
			*buf++ = c;  continue;
		case -1:
			if (c >= 040 && c < 0100) {
				d = c - 035; c = 040;  goto spaces;
			}
			if (c == 0100) {
				d = 0;
				*buf++ = ZRESC;  continue;
			}
			d = c;  continue;
		default:
			d -= 0100;
			if (d < 1)
				goto badpkt;
spaces:
			if ((buf + d) > end)
				goto badpkt;
			while ( --d >= 0)
				*buf++ = c;
			d = 0;  continue;
		}
	}
badpkt:
	Syslog('+', "Zmodem: Data subpacket too long");
	return TERROR;
}

