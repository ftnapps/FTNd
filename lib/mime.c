/*****************************************************************************
 *
 * File ..................: mime.c
 * Purpose ...............: Common library
 * Last modification date : 25-Aug-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "clcomm.h"
#include "common.h"


/* QP-Decode code by T.Tanaka <tt@efnet.com> */
/* QP-Encode inspired from sendmail code of Berkley */

/* XD() converts hexadecimal digit to decimal */
#define        XD(c)   ( (((c) >= '0') && ((c) <= '9')) ? (c) - '0' :   \
		(((c) >= 'A') && ((c) <= 'F')) ? (c) - 'A' + 10 :       \
		(((c) >= 'a') && ((c) <= 'f')) ? (c) - 'a' + 10 : 0)

/* chars to be converted in qp_encode() */
char	badchars[] = 	    "\001\002\003\004\005\006\007" \
			"\010\011\012\013\014\015\016\017" \
			"\020\021\022\023\024\025\026\027" \
			"\030\031\032\033\034\035\036\037" \
			"\177" \
			"\200\201\202\203\204\205\206\207" \
			"\210\211\212\213\214\215\216\217" \
			"\220\221\222\223\224\225\226\227" \
			"\230\231\232\233\234\235\236\237" \
			"\240\241\242\243\244\245\246\247" \
			"\250\251\252\253\254\255\256\257" \
			"\260\261\262\263\264\265\266\267" \
			"\270\271\272\273\274\275\276\277" \
			"\300\301\302\303\304\305\306\307" \
			"\310\311\312\313\314\315\316\317" \
			"\320\321\322\323\324\325\326\327" \
			"\330\331\332\333\334\335\336\337" \
			"\340\341\342\343\344\345\346\347" \
			"\350\351\352\353\354\355\356\357" \
			"\360\361\362\363\364\365\366\367" \
			"\370\371\372\373\374\375\376\377";
char	badchars2[] =	"!\"#$@[\\]^`{|}~()<>,;:/_";

char    Base16Code[] =  "0123456789ABCDEF";
char    Base64Code[] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* returns numeric value from a Base64Code[] digit */
static int index_hex[128] = {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,0x3e,  -1,  -1,  -1,0x3f,
    0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,
    0x3c,0x3d,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
    0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
    0x17,0x18,0x19,  -1,  -1,  -1,  -1,  -1,
      -1,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,
    0x31,0x32,0x33,  -1,  -1,  -1,  -1,  -1
};



char *qp_decode(char *s)
{
	static char	*buf;
	char		*p, *q;

	if (buf)
		free(buf);
	if ((buf = malloc(strlen(s) + 1 * sizeof(char))) == NULL) {
		WriteError("qp_decode: out of memory:string too long:\"%s\"", s);
		return s;
	}
	for (p = s, q = buf; *p != '\0';) {
		if (*p == '=') {
			++p;
			if (*p == '\0') {	/* ends with "=(null)" */
				*q++ = '=';
				break;
			} else if (*p == '\n')
				break;
			else if (isxdigit(*p) && isxdigit(*(p + 1))) {
				*q++ = 16 * XD(*p) + XD(*(p + 1));
				++p;
				++p;
			} else {	/* "=1x" "=5(null)" "=G\n" "=0=" etc. */
				*q++ = '=';
				*q++ = *p++;
			}
		} else
			*q++ = *p++;
	}
	*q = '\0';

	return buf;
}



char *qp_encode(char *s, int mode)
{
	static char	*buf;
	char		*p, *q;
	int		linelen = 0;

	if (buf)
		free(buf);
	if ((buf = malloc(3 * strlen(s) + 1 * sizeof(char))) == NULL) {
		WriteError("qp_encode: out of memory:string too long:\"%s\"", s);
		return s;
	}
	for (p = s, q = buf; *p != '\0';) {
		if (*p == '\n') {
			*q++ = *p++;
			linelen = 0;
		} else if ((mode == 1) && (*p == ' ')) {
			*q++ = '_';
			p++;
			linelen += 1;
		} else if (*p == ' ' || *p == '\t') {
			if ((linelen > 72) && (*(p+1) != '\n')) {
				*q++ = *p++;
				*q++ = '=';
				*q++ = '\n';
				linelen = 0;
			} else if (*(p+1) == '\n') {
				*q++ = *p++;
				*q++ = '=';
				*q++ = *p++;
				linelen = 0;
			} else {
				*q++ = *p++; 
				linelen += 1;
			}
		} else if ((strchr(badchars,*p)) || (*p == '=') || ((mode==1) && (strchr(badchars2,*p)))) {
			if (linelen > 72) {
				*q++ = '\n';
				linelen = 0;
			}
			*q++ = '=';
			*q++ = Base16Code[(*p >> 4) & 0x0f];
			*q++ = Base16Code[*p & 0x0f];
			linelen += 3;
			p++;
		} else {
			*q++ = *p++;
			linelen++;
		}
	}
	*q = '\0';

	return buf;
}



/* 
 * Base64 stores 3 bytes of 8bits into 4 bytes of six bits (the 2 remaining
 * bits are left to 0).
 *
 * AAAAAAAA BBBBBBBB CCCCCCCC --> 00AAAAAA 00AABBBB 00BBBBCC 00CCCCCC
 *
 */

char *b64_decode(char *s)
{
        static char	*buf;
	static char	buf2[4];
        char		*p, *q;
        int		i,t;

        if (buf)
                free(buf);
        if ((buf = malloc(3 * strlen(s) + 1 * sizeof(char))) == NULL) {
                WriteError("b64_decode:out of memory:string too long:\"%s\"", s);
                return s;
        }
        for (p = s, q = buf; *p != '\0';) {
		for (i = 0; i < 4; i++) 
			buf2[i]=0x40;	
		for (i = 0;((i < 4) && (*p != '\0'));) {
			t = (index_hex[(unsigned int)*p]);
			if (*p == '=') 
				buf2[i++]=0x40;
			else if (t != -1) 
				buf2[i++]=(char)t;
			p++;
		}
		if ((buf2[0] < 0x40) && (buf2[1] < 0x40))
			*q++=(((buf2[0] & 0x3f) << 2) | ((buf2[1] >> 4) & 0x03));
		if ((buf2[1] < 0x40) && (buf2[2] < 0x40))
			*q++=(((buf2[1] & 0x0f) << 4) | ((buf2[2] >> 2) & 0x0f));
		if ((buf2[2] < 0x40) && (buf2[3] < 0x40))
			*q++=(((buf2[2] & 0x03) << 6) | (buf2[3] & 0x3f));
	}
        *q = '\0';

        return buf;
}



char *b64_encode(char *s)
{
        static char	*buf;
        static char	buf2[3];
        char		*p, *q;
        int		i;

        if (buf)
                free(buf);
        if ((buf = malloc(3 * strlen(s) + 1 * sizeof(char))) == NULL) {
                WriteError("b64_encode:out of memory:string too long:\"%s\"", s);
                return s;
        }
        for (p = s, q = buf; *p != '\0';) {
		for (i = 0; ((i < 3) && (*p != '\0')); )
                        buf2[i++] = *p++;
		*q++=Base64Code[((buf2[0] >> 2) & 0x3f)];
		*q++=Base64Code[(((buf2[0] & 0x03) << 4) | ((buf2[1] >> 4) & 0x0f))];
		if (i<2) 
			*q++='=';
 		else 
			*q++=Base64Code[(((buf2[1] & 0x0f) << 2) | ((buf2[2] >> 6) & 0x03))];
		if (i<3) 
			*q++='=';
 		else 
			*q++=Base64Code[(buf2[2] & 0x3f)];
        }
        *q = '\0';

        return buf;
}

