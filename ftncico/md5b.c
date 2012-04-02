/*****************************************************************************
 *
 * $Id: md5b.c,v 1.5 2007/08/25 15:29:14 mbse Exp $
 * Purpose ...............: MD5 for binkp protocol driver
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek                FIDO:   2:280/2802
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
 * MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "lutil.h"
#include "md5b.h"

extern int  ext_rand;

/* 
 * Constants for MD5Transform routine.
 */

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static void MD5Transform(UINT4 [4], unsigned char [64]);
static void Encode(unsigned char *, UINT4 *, unsigned int);
static void Decode(UINT4 *, unsigned char *, unsigned int);
static void MD5_memcpy(POINTER, POINTER, unsigned int);
static void MD5_memset(POINTER, int, unsigned int);

static unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { (a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define GG(a, b, c, d, x, s, ac) { (a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define HH(a, b, c, d, x, s, ac) { (a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define II(a, b, c, d, x, s, ac) { (a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }

/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
static void MD5Init(MD5_CTX *context)
{
  context->count[0] = context->count[1] = 0;
  /* Load magic initialization constants.
*/
  context->state[0] = 0x67452301;
  context->state[1] = 0xefcdab89;
  context->state[2] = 0x98badcfe;
  context->state[3] = 0x10325476;
}



/* 
 *  MD5 block update operation. Continues an MD5 message-digest
 *  operation, processing another message block, and updating the
 *  context.
 */
static void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputLen)
{
    unsigned int    i, mdindex, partLen;

    /* Compute number of bytes mod 64 */
    mdindex = (unsigned int)((context->count[0] >> 3) & 0x3F);

    /* Update number of bits */
    if ((context->count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3))
	context->count[1]++;
    context->count[1] += ((UINT4)inputLen >> 29);

    partLen = 64 - mdindex;

    /* Transform as many times as possible.  */
    if (inputLen >= partLen) {
	MD5_memcpy ((POINTER)&context->buffer[mdindex], (POINTER)input, partLen);
	MD5Transform (context->state, context->buffer);

	for (i = partLen; i + 63 < inputLen; i += 64)
	    MD5Transform (context->state, &input[i]);

	mdindex = 0;
    } else
	i = 0;

    /* Buffer remaining input */
    MD5_memcpy ((POINTER)&context->buffer[mdindex], (POINTER)&input[i], inputLen-i);
}



/* 
 *  MD5 finalization. Ends an MD5 message-digest operation, writing the
 *  the message digest and zeroizing the context.
 */
static void MD5Final(unsigned char digest[16], MD5_CTX *context)
{
    unsigned char   bits[8];
    unsigned int    mdindex, padLen;

    /* Save number of bits */
    Encode (bits, context->count, 8);

    /* Pad out to 56 mod 64.  */
    mdindex = (unsigned int)((context->count[0] >> 3) & 0x3f);
    padLen = (mdindex < 56) ? (56 - mdindex) : (120 - mdindex);
    MD5Update (context, PADDING, padLen);

    /* Append length (before padding) */
    MD5Update (context, bits, 8);

    /* Store state in digest */
    Encode (digest, context->state, 16);

    /* Zeroize sensitive information.  */
    MD5_memset ((POINTER)context, 0, sizeof (*context));
}



/* 
 *  MD5 basic transformation. Transforms state based on block.
 */
static void MD5Transform (UINT4 state[4], unsigned char block[64])
{
  UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  Decode (x, block, 64);

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
  FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
  FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
  FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
  FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
  FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
  FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
  FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

 /* Round 2 */
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
  GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
  GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
  GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
  GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
  GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
  GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
  GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
  GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
  HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
  HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
  HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
  HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
  HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
  II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
  II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
  II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
  II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
  II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
  II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
  II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
  II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information.  */
  MD5_memset ((POINTER)x, 0, sizeof (x));
}



/* 
 *  Encodes input (UINT4) into output (unsigned char). Assumes len is
 *  a multiple of 4.
 */
static void Encode (unsigned char *output, UINT4 *input, unsigned int len)
{
    unsigned int i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
	output[j] = (unsigned char)(input[i] & 0xff);
	output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
	output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
	output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
    }
}



/* 
 *  Decodes input (unsigned char) into output (UINT4). Assumes len is
 *  a multiple of 4.
 */
static void Decode (UINT4 *output, unsigned char *input, unsigned int len)
{
    unsigned int i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
	output[i] = ((UINT4)input[j]) | (((UINT4)input[j+1]) << 8) | (((UINT4)input[j+2]) << 16) | (((UINT4)input[j+3]) << 24);
}



/* 
 *  Note: Replace "for loop" with standard memcpy if possible.
 */
static void MD5_memcpy (POINTER output, POINTER input, unsigned int len)
{
    unsigned int i;

    for (i = 0; i < len; i++)
	output[i] = input[i];
}



/* 
 *  Note: Replace "for loop" with standard memset if possible.
 */
static void MD5_memset (POINTER output, int value, unsigned int len)
{
    unsigned int i;

    for (i = 0; i < len; i++)
	((char *)output)[i] = (char)value;
}


/* ---------------------------------------------------------- */


static void hmac_md5(unsigned char *text, int text_len, unsigned char *key, int key_len, MDcaddr_t digest)
{
    MD5_CTX context;
    unsigned char k_ipad[65];	/* inner padding - key XORd with ipad */
    unsigned char k_opad[65];   /* outer padding - key XORd with opad */
    unsigned char tk[16];
    int i;

    /* if key is longer than 64 bytes reset it to key=MD5(key) */
    if (key_len > 64) {

        MD5_CTX      tctx;

        MD5Init(&tctx);
        MD5Update(&tctx, key, key_len);
        MD5Final(tk, &tctx);

        key = tk;
        key_len = 16;
    }

    /*
     * the HMAC_MD5 transform looks like:
     *
     * MD5(K XOR opad, MD5(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* start out by storing key in pads */
    memset((char *) k_ipad, 0, sizeof k_ipad);
    memset((char *) k_opad, 0, sizeof k_opad);
    memmove( k_ipad, key, key_len);
    memmove( k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < 64; i++) {
	k_ipad[i] ^= 0x36;
	k_opad[i] ^= 0x5c;
    }

    /*
     * perform inner MD5
     */
    MD5Init(&context);                   /* init context for 1st pass	*/
    MD5Update(&context, k_ipad, 64);     /* start with inner pad	*/
    MD5Update(&context, text, text_len); /* then text of datagram	*/
    MD5Final(digest, &context);          /* finish up 1st pass		*/

    /*
     * perform outer MD5
     */
    MD5Init(&context);                   /* init context for 2nd pass	*/
    MD5Update(&context, k_opad, 64);     /* start with outer pad	*/
    MD5Update(&context, digest, 16);     /* then results of 1st hash	*/
    MD5Final(digest, &context);          /* finish up 2nd pass		*/
}

/* ---------------------------------------------------------- */

static void getrand(unsigned char *res, int len, struct sockaddr_in *peer_name)
{
    MDcaddr_t	digest;
    struct {
	time_t		tm;
	unsigned short	pid;
	unsigned short	rand;
	int		ext_rand;
    } rd;

    time(&rd.tm);
    rd.pid = (int)getpid();
    rd.rand = rand();
    rd.ext_rand = ext_rand;
    hmac_md5((void *)&rd, sizeof(rd), (void *)peer_name, sizeof(struct sockaddr_in), digest);
    if ((peer_name) /* &&  (peer_name[0]) */)
	hmac_md5((void *)peer_name, sizeof(struct sockaddr_in), digest, sizeof(digest), digest);
    memcpy(res, digest, len);
}



unsigned char *MD_getChallenge(char *str, struct sockaddr_in *crnd)
{
    unsigned char   *res = NULL;
    int		    i;

    if (!str) {
	res = (unsigned char*)xmalloc(MD_CHALLENGE_LEN+1);
	res[0] = MD_CHALLENGE_LEN;
	for (i = 1; i < (MD_CHALLENGE_LEN + 1); i += MD5_DIGEST_LEN) {
	    int	k = MD5_DIGEST_LEN;
	    if (k >= i + MD_CHALLENGE_LEN)
	       	k = MD_CHALLENGE_LEN + 1 - i;
	    getrand(res+i, k, crnd);
	}
    } else {
	char	*sp;
	if ((sp = strstr(str, "CRAM")) == NULL) 
	    return NULL;
	if ((sp = strstr(sp, "MD5")) == NULL) 
	    return NULL;
	while (sp[0]) 
	    if (*sp++ == '-') 
		break;
	if (!sp[0]) 
	    return NULL;
	for (i = 0; isxdigit((int)sp[i]); i++) 
	    if (i >= 128) 
		break;
	i /= 2;
	res = (unsigned char*)xmalloc(i+1);
	res[0] = (char)i;
	for (i = 0; isxdigit((int)sp[i]) && (i < 128); i++) {
	    unsigned char   c=tolower(sp[i]);
	    if (c > '9') 
		c -= 'a'-10;
	    else 
		c -= '0';
	    if (!(i % 2)) 
		res[i / 2 + 1] = c << 4;
	    else 
		res[i / 2 + 1] |= c;
	}
    }

    return res;
}



void MD_toString(char *rs, int len, unsigned char *digest)
{
    int	i,j;
    if (!rs) 
	return;
    strcpy(rs, "CRAM-MD5-");
    for (i = 0, j = 9; i < len; i++) {
	unsigned char	c = (digest[i] >> 4);
	if (c > 9) 
	    c += 'a'-10;
	else 
	    c += '0';
	rs[j++] = c;
	c = (digest[i] & 0xF);
	if (c > 9) 
	    c += 'a'-10;
	else 
	    c += '0';
	rs[j++] = c;
    }
    rs[j] = 0;
}



char *MD_buildDigest(char *pw, unsigned char *challenge)
{
    char	*rs = NULL;
    MDcaddr_t	digest;

    if ((!pw) || (!challenge)) 
	return rs;

    hmac_md5(challenge+1, challenge[0], (unsigned char *)pw, strlen(pw), digest);
    rs = (char *)xmalloc(MD5_DIGEST_LEN * 2 + 10);
    MD_toString(rs, MD5_DIGEST_LEN, digest);
    return rs;
}

