#ifndef MD5B_H
#define	MD5B_H

/* $Id: md5b.h,v 1.2 2005/10/11 20:49:46 mbse Exp $ */

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

/* ------------------------------------------------------------------ */
/* GLOBAL.H - RSAREF types and constants
 */

/* RFC 1321              MD5 Message-Digest Algorithm            April 1992 */


/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;
/* UINT4 defines a four byte word */
typedef unsigned int UINT4;

/* end of GLOBAL.H ---------------------------------------------------------- */

/* MD5 context. */
typedef struct {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

#define MD5_DIGEST_LEN 16

/* MD5 digest */
typedef unsigned char MDcaddr_t[MD5_DIGEST_LEN];

#define MD_CHALLENGE_LEN 16


unsigned char *MD_getChallenge(char *, struct sockaddr_in *);
char *MD_buildDigest(char *, unsigned char *);
void MD_toString(char *, int, unsigned char *);


#endif
