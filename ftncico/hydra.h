/* $Id: hydra.h,v 1.4 2003/10/22 21:11:40 mbroek Exp $ */
/* 
 * As this file has been derived from the HydraCom source, here is the
 * original copyright information:
 *
 * Note that you can find the file LICENSE.DOC from HydraCom in
 * misc/HYDRACOM-LICENSE
 *
 * Some changes are made in this file to customize for use with MBSE BBS.
 * There are also some extensions not in the original Hydra code for zlib
 * packet compression of data packets.
 */
/*=============================================================================

                              HydraCom Version 1.00

                         A sample implementation of the
                   HYDRA Bi-Directional File Transfer Protocol

                             HydraCom was written by
                   Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED

                       The HYDRA protocol was designed by
                 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
                             Joaquim H. Homrighausen
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED


  Revision history:
  06 Sep 1991 - (AGL) First tryout
  .. ... .... - Internal development
  11 Jan 1993 - HydraCom version 1.00, Hydra revision 001 (01 Dec 1992)


  For complete details of the Hydra and HydraCom licensing restrictions,
  please refer to the license agreements which are published in their entirety
  in HYDRACOM.C and LICENSE.DOC, and also contained in the documentation file
  HYDRACOM.DOC

  Use of this file is subject to the restrictions contained in the Hydra and
  HydraCom licensing agreements. If you do not find the text of this agreement
  in any of the aforementioned files, or if you do not have these files, you
  should immediately contact LENTZ SOFTWARE-DEVELOPMENT and/or Joaquim
  Homrighausen at one of the addresses listed below. In no event should you
  proceed to use this file without having accepted the terms of the Hydra and
  HydraCom licensing agreements, or such other agreement as you are able to
  reach with LENTZ SOFTWARE-DEVELOMENT and Joaquim Homrighausen.


  Hydra protocol design and HydraCom driver:         Hydra protocol design:
  Arjen G. Lentz                                     Joaquim H. Homrighausen
  LENTZ SOFTWARE-DEVELOPMENT                         389, route d'Arlon
  Langegracht 7B                                     L-8011 Strassen
  3811 BT  Amersfoort                                Luxembourg
  The Netherlands
  FidoNet 2:283/512, AINEX-BBS +31-33-633916         FidoNet 2:270/17
  arjen_lentz@f512.n283.z2.fidonet.org               joho@ae.lu

  Please feel free to contact us at any time to share your comments about our
  software and/or licensing policies.

=============================================================================*/

#ifndef	_HYDRA_H
#define	_HYDRA_H

/* HYDRA Specification Revision/Timestamp ---------Revision------Date------- */
#define H_REVSTAMP   0x2b1aab00L		/* 001		 01 Dec 1992 */
#define H_REVISION   1

/* HYDRA Basic Values ------------------------------------------------------ */
#ifndef XON
#define XON	     ('Q' - '@')	/* Ctrl-Q (^Q) xmit-on character     */
#define XOFF	     ('S' - '@')	/* Ctrl-S (^S) xmit-off character    */
#endif
#define H_DLE	     ('X' - '@')	/* Ctrl-X (^X) HYDRA DataLinkEscape  */
#define H_UNCBLKLEN  2048		/* Max. uncompressed blocklen	     */
#define H_MINBLKLEN    64		/* Min. length of a HYDRA data block */
#define H_MAXBLKLEN  8192		/* Max. length of a HYDRA data block */
					/* NB: official is 2048, here we use */
					/* this to prepare for receive large */
					/* compressed blocks. Uncompressed   */
					/* we still use 2048 maximum.	     */
#define H_OVERHEAD	8		/* Max. no. control bytes in a pkt   */
#define H_MAXPKTLEN  ((H_MAXBLKLEN + H_OVERHEAD + 5) * 3)     /* Encoded pkt */
#define H_BUFLEN     (H_MAXPKTLEN + 16) /* Buffer sizes: max.enc.pkt + slack */
#define	H_ZIPBUFLEN  (((H_BUFLEN * 11) / 10) + 12) /* Compressed data pkt    */
#define H_PKTPREFIX    31		/* Max length of pkt prefix string   */
#define H_FLAGLEN	3		/* Length of a flag field	     */
#define H_RETRIES      10		/* No. retries in case of an error   */
#define H_MINTIMER     10		/* Minimum timeout period	     */
#define H_MAXTIMER     60		/* Maximum timeout period	     */
#define H_START		5		/* Timeout for re-sending startstuff */
#define H_IDLE	       20		/* Idle? tx IDLE pkt every 20 secs   */
#define H_BRAINDEAD   120		/* Braindead in 2 mins (120 secs)    */

/* HYDRA Return codes ------------------------------------------------------ */
#define XFER_ABORT    (-1)		/* Failed on this file & abort xfer  */
#define XFER_SKIP	0		/* Skip this file but continue xfer  */
#define XFER_OK		1		/* File was sent, continue transfer  */


/* HYDRA Transmitter States ------------------------------------------------ */
enum HyTxStates
{
  HTX_DONE,		/* All over and done		     */
  HTX_START,		/* Send start autostr + START pkt    */
  HTX_SWAIT,		/* Wait for any pkt or timeout	     */
  HTX_INIT,		/* Send INIT pkt		     */
  HTX_INITACK,		/* Wait for INITACK pkt		     */
  HTX_RINIT,		/* Wait for HRX_INIT -> HRX_FINFO    */
  HTX_NextFile,
  HTX_ToFName,
  HTX_FINFO,		/* Send FINFO pkt		     */
  HTX_FINFOACK,		/* Wait for FINFOACK pkt	     */
  HTX_DATA,		/* Send next packet with file data   */
  HTX_SkipFile,
  HTX_DATAACK,		/* Wait for DATAACK packet	     */
  HTX_XWAIT,		/* Wait for HRX_END		     */
  HTX_EOF,		/* Send EOF pkt			     */
  HTX_EOFACK,		/* End of file, wait for EOFACK pkt  */
  HTX_REND,		/* Wait for HRX_END && HTD_DONE	     */
  HTX_END,		/* Send END pkt (finish session)     */
  HTX_ENDACK,		/* Wait for END pkt from other side  */
  HTX_Abort,
};

/* HYDRA Receiver States --------------------------------------------------- */
enum HyRxStates
{
  HRX_DONE,		/* All over and done		     */
  HRX_INIT,		/* Wait for INIT pkt		     */
  HRX_FINFO,		/* Wait for FINFO pkt of next file   */
  HRX_ToData,
  HRX_DATA,		/* Wait for next DATA pkt	     */
  HRX_BadPos,
  HRX_Timer,
  HRX_HdxLink,
  HRX_Retries,
  HRX_RPos,
  HRX_OkEOF,
};

/* HYDRA Packet Types ------------------------------------------------------ */
enum HyPktTypes
{
  HPKT_START = 'A',	/* Startup sequence		     */
  HPKT_INIT = 'B',	/* Session initialisation	     */
  HPKT_INITACK = 'C',	/* Response to INIT pkt		     */
  HPKT_FINFO = 'D',	/* File info (name, size, time)	     */
  HPKT_FINFOACK = 'E',	/* Response to FINFO pkt	     */
  HPKT_DATA = 'F',	/* File data packet		     */
  HPKT_DATAACK = 'G',	/* File data position ACK packet     */
  HPKT_RPOS = 'H',	/* Transmitter reposition packet     */
  HPKT_EOF = 'I',	/* End of file packet		     */
  HPKT_EOFACK = 'J',	/* Response to EOF packet	     */
  HPKT_END = 'K',	/* End of session		     */
  HPKT_IDLE = 'L',	/* Idle - just saying I'm alive	     */
  HPKT_DEVDATA = 'M',	/* Data to specified device	     */
  HPKT_DEVDACK = 'N',	/* Response to DEVDATA pkt	     */
#ifdef	HAVE_ZLIB_H
  HPKT_ZIPDATA = 'O',	/* Zlib compressed file data packet  */

  HPKT_HIGHEST = 'O'	/* Highest known pkttype in this imp */
#else
  HPKT_HIGHEST = 'N'    /* Highest known pkttype in this imp */
#endif
};

/* HYDRA compression types ------------------------------------------------- */
enum HyCompStates
{
  HCMP_NONE,		/* No compression, default	     */
  HCMP_GZ,		/* Gzip compression		     */
  HCMP_BZ2,		/* Bzip2 compression		     */
};


/* HYDRA Internal Pseudo Packet Types -------------------------------------- */
#define H_NOPKT		0		/* No packet (yet)		     */
#define H_CANCEL      (-1)		/* Received cancel sequence 5*Ctrl-X */
#define H_CARRIER     (-2)		/* Lost carrier			     */
#define H_SYSABORT    (-3)		/* Aborted by operator on this side  */
#define H_TXTIME      (-4)		/* Transmitter timeout		     */
#define H_DEVTXTIME   (-5)		/* Device transmitter timeout	     */
#define H_BRAINTIME   (-6)		/* Braindead timeout (quite fatal)   */

/* HYDRA Packet Format: START[<data>]<type><crc>END ------------------------ */
enum HyPktFormats
{
  HCHR_PKTEND = 'a',	/* End of packet (any format)	     */
  HCHR_BINPKT = 'b',	/* Start of binary packet	     */
  HCHR_HEXPKT = 'c',	/* Start of hex encoded packet	     */
  HCHR_ASCPKT = 'd',	/* Start of shifted 7bit encoded pkt */
  HCHR_UUEPKT = 'e',	/* Start of uuencoded packet	     */
};

/* HYDRA Local Storage of INIT Options (Bitmapped) ------------------------- */
#define HOPT_XONXOFF  (0x00000001L)	/* Escape XON/XOFF		     */
#define HOPT_TELENET  (0x00000002L)	/* Escape CR-'@'-CR (Telenet escape) */
#define HOPT_CTLCHRS  (0x00000004L)	/* Escape ASCII 0-31 and 127	     */
#define HOPT_HIGHCTL  (0x00000008L)	/* Escape above 3 with 8th bit too   */
#define HOPT_HIGHBIT  (0x00000010L)	/* Escape ASCII 128-255 + strip high */
#define HOPT_CANBRK   (0x00000020L)	/* Can transmit a break signal	     */
#define HOPT_CANASC   (0x00000040L)	/* Can transmit/handle ASC packets   */
#define HOPT_CANUUE   (0x00000080L)	/* Can transmit/handle UUE packets   */
#define HOPT_CRC32    (0x00000100L)	/* Packets with CRC-32 allowed	     */
#define HOPT_DEVICE   (0x00000200L)	/* DEVICE packets allowed	     */
#define HOPT_FPT      (0x00000400L)	/* Can handle filenames with paths   */
#ifdef HAVE_ZLIB_H
#define HOPT_CANPLZ   (0x00000800L)	/* Can handle zlib packet compress   */
#endif

/* What we can do */
#ifdef HAVE_ZLIB_H
#define HCAN_OPTIONS  (HOPT_XONXOFF | HOPT_TELENET | HOPT_CTLCHRS | HOPT_HIGHCTL | HOPT_HIGHBIT | HOPT_CRC32 | HOPT_CANPLZ)
#else
#define HCAN_OPTIONS  (HOPT_XONXOFF | HOPT_TELENET | HOPT_CTLCHRS | HOPT_HIGHCTL | HOPT_HIGHBIT | HOPT_CRC32)
#endif

/* Vital options if we ask for any; abort if other side doesn't support them */
#define HNEC_OPTIONS  (HOPT_XONXOFF | HOPT_TELENET | HOPT_CTLCHRS | HOPT_HIGHCTL | HOPT_HIGHBIT | HOPT_CANBRK)

/* Non-vital options; nice if other side supports them, but doesn't matter */
#ifdef HAVE_ZLIB_H
#define HUNN_OPTIONS  (HOPT_CANASC | HOPT_CANUUE | HOPT_CRC32 | HOPT_CANPLZ)
#else
#define HUNN_OPTIONS  (HOPT_CANASC | HOPT_CANUUE | HOPT_CRC32)
#endif

/* Default options */
#define HDEF_OPTIONS  (HOPT_CRC32)

/* rxoptions during init (needs to handle ANY link yet unknown at that point */
#define HRXI_OPTIONS  (HOPT_XONXOFF | HOPT_TELENET | HOPT_CTLCHRS | HOPT_HIGHCTL | HOPT_HIGHBIT)

/* ditto, but this time txoptions */
#define HTXI_OPTIONS  (HOPT_XONXOFF | HOPT_TELENET | HOPT_CTLCHRS | HOPT_HIGHCTL | HOPT_HIGHBIT)


#define h_crc16test(crc)   (((crc) == 0xf0b8     ) ? 1 : 0)
#define h_crc32test(crc)   (((crc) == 0xdebb20e3L) ? 1 : 0)

int hydra(int);

#endif

