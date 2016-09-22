/*
**  JAM(mbp) - The Joaquim-Andrew-Mats Message Base Proposal
**
**  C API
**
**  Written by Joaquim Homrighausen.
**
**  ----------------------------------------------------------------------
**
**  jam.h (JAMmb)
**
**  Prototypes and definitions for the JAM message base format
**
**  Copyright 1993 Joaquim Homrighausen, Andrew Milner, Mats Birch, and
**  Mats Wallin. ALL RIGHTS RESERVED.
**
**  93-06-28    JoHo
**  Initial coding.
**
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __JAM_H__
#define __JAM_H__

#ifndef __JAMSYS_H__
#include "jamsys.h"
#endif

/*
**  File extensions
*/
#define EXT_HDRFILE     ".jhr"
#define EXT_TXTFILE     ".jdt"
#define EXT_IDXFILE     ".jdx"
#define EXT_LRDFILE     ".jlr"

/*
**  Revision level and header signature
*/
#define CURRENTREVLEV   1
#define HEADERSIGNATURE "JAM"

/*
**  Header file information block, stored first in all .JHR files
*/
typedef struct
    {
    CHAR8   Signature[4];              /* <J><A><M> followed by <NUL> */
    UINT32  DateCreated;               /* Creation date */
    UINT32  ModCounter;                /* Last processed counter */
    UINT32  ActiveMsgs;                /* Number of active (not deleted) msgs */
    UINT32  PasswordCRC;               /* CRC-32 of password to access */
    UINT32  BaseMsgNum;                /* Lowest message number in index file */
    CHAR8   RSRVD[1000];               /* Reserved space */
    }
    JAMHDRINFO, _JAMDATA * JAMHDRINFOptr;

/*
**  Message status bits
*/
#define MSG_LOCAL       0x00000001L    /* Msg created locally */
#define MSG_INTRANSIT   0x00000002L    /* Msg is in-transit */
#define MSG_PRIVATE     0x00000004L    /* Private */
#define MSG_READ        0x00000008L    /* Read by addressee */
#define MSG_SENT        0x00000010L    /* Sent to remote */
#define MSG_KILLSENT    0x00000020L    /* Kill when sent */
#define MSG_ARCHIVESENT 0x00000040L    /* Archive when sent */
#define MSG_HOLD        0x00000080L    /* Hold for pick-up */
#define MSG_CRASH       0x00000100L    /* Crash */
#define MSG_IMMEDIATE   0x00000200L    /* Send Msg now, ignore restrictions */
#define MSG_DIRECT      0x00000400L    /* Send directly to destination */
#define MSG_GATE        0x00000800L    /* Send via gateway */
#define MSG_FILEREQUEST 0x00001000L    /* File request */
#define MSG_FILEATTACH  0x00002000L    /* File(s) attached to Msg */
#define MSG_TRUNCFILE   0x00004000L    /* Truncate file(s) when sent */
#define MSG_KILLFILE    0x00008000L    /* Delete file(s) when sent */
#define MSG_RECEIPTREQ  0x00010000L    /* Return receipt requested */
#define MSG_CONFIRMREQ  0x00020000L    /* Confirmation receipt requested */
#define MSG_ORPHAN      0x00040000L    /* Unknown destination */
#define MSG_ENCRYPT     0x00080000L    /* Msg text is encrypted */
#define MSG_COMPRESS    0x00100000L    /* Msg text is compressed */
#define MSG_ESCAPED     0x00200000L    /* Msg text is seven bit ASCII */
#define MSG_FPU         0x00400000L    /* Force pickup */
#define MSG_TYPELOCAL   0x00800000L    /* Msg is for local use only (not for export) */
#define MSG_TYPEECHO    0x01000000L    /* Msg is for conference distribution */
#define MSG_TYPENET     0x02000000L    /* Msg is direct network mail */
#define MSG_NODISP      0x20000000L    /* Msg may not be displayed to user */
#define MSG_LOCKED      0x40000000L    /* Msg is locked, no editing possible */
#define MSG_DELETED     0x80000000L    /* Msg is deleted */


/*
**  Message header
*/
typedef struct
    {
    CHAR8   Signature[4];              /* <J><A><M> followed by <NUL> */
    UINT16  Revision;                  /* CURRENTREVLEV */
    UINT16  ReservedWord;              /* Reserved */
    UINT32  SubfieldLen;               /* Length of subfields */
    UINT32  TimesRead;                 /* Number of times message read */
    UINT32  MsgIdCRC;                  /* CRC-32 of MSGID line */
    UINT32  ReplyCRC;                  /* CRC-32 of REPLY line */
    UINT32  ReplyTo;                   /* This msg is a reply to.. */
    UINT32  Reply1st;                  /* First reply to this msg */
    UINT32  ReplyNext;                 /* Next msg in reply chain */
    UINT32  DateWritten;               /* When msg was written */
    UINT32  DateReceived;              /* When msg was received/read */
    UINT32  DateProcessed;             /* When msg was processed by packer */
    UINT32  MsgNum;                    /* Message number (1-based) */
    UINT32  Attribute;                 /* Msg attribute, see "Status bits" */
    UINT32  Attribute2;                /* Reserved for future use */
    UINT32  TxtOffset;                 /* Offset of text in text file */
    UINT32  TxtLen;                    /* Length of message text */
    UINT32  PasswordCRC;               /* CRC-32 of password to access msg */
    UINT32  Cost;                      /* Cost of message */
    }
    JAMHDR, _JAMDATA * JAMHDRptr;

/*
**  Message header subfield types
*/
#define JAMSFLD_OADDRESS    0
#define JAMSFLD_DADDRESS    1
#define JAMSFLD_SENDERNAME  2
#define JAMSFLD_RECVRNAME   3
#define JAMSFLD_MSGID       4
#define JAMSFLD_REPLYID     5
#define JAMSFLD_SUBJECT     6
#define JAMSFLD_PID         7
#define JAMSFLD_TRACE       8
#define JAMSFLD_ENCLFILE    9
#define JAMSFLD_ENCLFWALIAS 10
#define JAMSFLD_ENCLFREQ    11
#define JAMSFLD_ENCLFILEWC  12
#define JAMSFLD_ENCLINDFILE 13
#define JAMSFLD_EMBINDAT    1000
#define JAMSFLD_FTSKLUDGE   2000
#define JAMSFLD_SEENBY2D    2001
#define JAMSFLD_PATH2D      2002
#define JAMSFLD_FLAGS       2003
#define JAMSFLD_TZUTCINFO   2004
#define JAMSFLD_UNKNOWN     0xffff

/*
**  Message header subfield
*/
typedef struct
    {
    UINT16  LoID;                      /* Field ID, 0 - 0xffff */
    UINT16  HiID;                      /* Reserved for future use */
    UINT32  DatLen;                    /* Length of buffer that follows */
    CHAR8   Buffer[1];                 /* DatLen bytes of data */
    }
    JAMSUBFIELD, _JAMDATA * JAMSUBFIELDptr;

typedef struct
    {
    UINT16  LoID;                      /* Field ID, 0 - 0xffff */
    UINT16  HiID;                      /* Reserved for future use */
    UINT32  DatLen;                    /* Length of buffer that follows */
    }
    JAMBINSUBFIELD, _JAMDATA * JAMBINSUBFIELDptr;

/*
**  Message index record
*/
typedef struct
    {
    UINT32  UserCRC;                   /* CRC-32 of destination username */
    UINT32  HdrOffset;                 /* Offset of header in .JHR file */
    }
    JAMIDXREC, _JAMDATA * JAMIDXRECptr;

/*
**  Lastread structure, one per user
*/
typedef struct
    {
    UINT32  UserCRC;                   /* CRC-32 of user name (lowercase) */
    UINT32  UserID;                    /* Unique UserID */
    UINT32  LastReadMsg;               /* Last read message number */
    UINT32  HighReadMsg;               /* Highest read message number */
    }
    JAMLREAD, _JAMDATA * JAMLREADptr;

#endif /* __JAM_H__ */

#ifdef __cplusplus
}
#endif

/* end of file "jam.h" */
