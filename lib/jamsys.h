/*
**  $Id: jamsys.h,v 1.2 2005/10/11 20:49:42 mbse Exp $
**
**  JAM(mbp) - The Joaquim-Andrew-Mats Message Base Proposal
**
**  C API
**
**  Written by Joaquim Homrighausen and Mats Wallin.
**
**  ----------------------------------------------------------------------
**
**  jamsys.h (JAMmb)
**
**  Compiler and platform dependant definitions
**
**  Copyright 1993 Joaquim Homrighausen, Andrew Milner, Mats Birch, and
**  Mats Wallin. ALL RIGHTS RESERVED.
**
**  93-06-28    JoHo/MW
**  Initial coding.
*/

#ifndef __JAMSYS_H__
#define __JAMSYS_H__

/*
**  The following assumptions are made about compilers and platforms:
**
**  __MSDOS__       Defined if compiling for MS-DOS
**  _WINDOWS        Defined if compiling for Microsoft Windows
**  __NT__          Defined if compiling for Windows NT
**  __OS2__         Defined if compiling for OS/2 2.x
**  __sparc__       Defined if compiling for Sun Sparcstation
**  __50SERIES      Defined if compiling for Prime with Primos
**
**  __SMALL__       Defined if compiling under MS-DOS in small memory model
**  __MEDIUM__      Defined if compiling under MS-DOS in medium memory model
**  __COMPACT__     Defined if compiling under MS-DOS in compact memory model
**  __LARGE__       Defined if compiling under MS-DOS in large memory model
**
**  __ZTC__         Zortech C++ 3.x
**  __BORLANDC__    Borland C++ 3.x
**  __TURBOC__      Turbo C 2.0
**  __TSC__         JPI TopSpeed C 1.06
**  _MSC_VER        Microsoft C 6.0 and later
**  _QC             Microsoft Quick C
*/

typedef int		    INT32;      /* 32 bits signed integer     */
typedef unsigned int        UINT32;     /* 32 bits unsigned integer   */
typedef short int           INT16;      /* 16 bits signed integer     */
typedef unsigned short int  UINT16;     /* 16 bits unsigned integer   */
typedef char                CHAR8;      /* 8 bits signed integer      */
typedef unsigned char       UCHAR8;     /* 8 bits unsigned integer    */
typedef int                 FHANDLE;    /* File handle                */

#define _JAMFAR
#define _JAMPROC
#define _JAMDATA


typedef INT32 _JAMDATA *        INT32ptr;
typedef UINT32 _JAMDATA *       UINT32ptr;
typedef INT16 _JAMDATA *        INT16ptr;
typedef UINT16 _JAMDATA *       UINT16ptr;
typedef CHAR8 _JAMDATA *        CHAR8ptr;
typedef UCHAR8 _JAMDATA *       UCHAR8ptr;
typedef void _JAMDATA *         VOIDptr;

/*
**  Values for "AccessMode" and "ShareMode" parameter to JAMsysSopen.
*/

#define JAMO_RDWR           O_RDWR
#define JAMO_RDONLY         O_RDONLY
#define JAMO_WRONLY         O_WRONLY
#define JAMSH_DENYNO        0
#define JAMSH_DENYRD        0
#define JAMSH_DENYWR        0
#define JAMSH_DENYRW        0


/*
**  Structure to contain date/time information
*/
typedef struct JAMtm
    {
    int     tm_sec,                    /* Seconds 0..59                     */
            tm_min,                    /* Minutes 0..59                     */
            tm_hour,                   /* Hour of day 0..23                 */
            tm_mday,                   /* Day of month 1..31                */
            tm_mon,                    /* Month 0..11                       */
            tm_year,                   /* Years since 1900                  */
            tm_wday,                   /* Day of week 0..6 (Sun..Sat)       */
            tm_yday,                   /* Day of year 0..365                */
            tm_isdst;                  /* Daylight savings time (not used)  */
    } JAMTM, _JAMDATA * JAMTMptr;

#endif /* __JAMSYS_H__ */


/* end of file "jamsys.h" */
