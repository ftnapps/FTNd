/*****************************************************************************
 *
 * $Id: jammsg.c,v 1.29 2007/10/11 18:14:54 mbse Exp $
 * Purpose ...............: JAM message base functions
 *
 *****************************************************************************
 *
 * Original written in C++ by Marco Maccaferri for LoraBBS and was 
 * distributed under GNU GPL. This version is modified for use with
 * MBSE BBS and utilities.
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "mbselib.h"
#include "msgtext.h"
#include "msg.h"
#include "jam.h"
#include "jammsg.h"
#include "users.h"


#define	MAX_TEXT	2048

int		fdHdr = -1;
int		fdJdt = -1;
int		fdJdx = -1;
int		fdJlr = -1;
unsigned char	*pSubfield;
char		BaseName[PATH_MAX];
char		*pLine, *pBuff;
char		szBuff[MAX_LINE_LENGTH + 1];
char		szLine[MAX_LINE_LENGTH + 1];
JAMHDRINFO	jamHdrInfo;
JAMHDR		jamHdr;
unsigned int	LastReadRec;



unsigned int AddSubfield(unsigned int, char *);
unsigned int AddSubfield(unsigned int JamSFld, char *SubStr)
{
	JAMSUBFIELD	jamSubfield;
	unsigned int	Len;

	jamSubfield.HiID = 0;
	jamSubfield.LoID = JamSFld;
	jamSubfield.DatLen = strlen(SubStr); /* + 1; */
	Len = jamSubfield.DatLen + sizeof(JAMBINSUBFIELD);
	write(fdHdr, &jamSubfield, sizeof(JAMBINSUBFIELD));
	write(fdHdr, SubStr, strlen(SubStr)); /* + 1); */

	return Len;
}



void JAMset_flags(void);
void JAMset_flags()
{
	jamHdr.Attribute |= (Msg.Local)          ? MSG_LOCAL : 0;
	jamHdr.Attribute |= (Msg.Intransit)      ? MSG_INTRANSIT : 0;
	jamHdr.Attribute |= (Msg.Private)        ? MSG_PRIVATE : 0;
	jamHdr.Attribute |= (Msg.Received)       ? MSG_READ : 0;
	jamHdr.Attribute |= (Msg.Sent)           ? MSG_SENT : 0;
	jamHdr.Attribute |= (Msg.KillSent)       ? MSG_KILLSENT : 0;
	jamHdr.Attribute |= (Msg.ArchiveSent)    ? MSG_ARCHIVESENT : 0;
	jamHdr.Attribute |= (Msg.Hold)           ? MSG_HOLD : 0;
	jamHdr.Attribute |= (Msg.Crash)          ? MSG_CRASH : 0;
	jamHdr.Attribute |= (Msg.Immediate)      ? MSG_IMMEDIATE : 0;
	jamHdr.Attribute |= (Msg.Direct)         ? MSG_DIRECT : 0;
	jamHdr.Attribute |= (Msg.Gate)           ? MSG_GATE : 0;
	jamHdr.Attribute |= (Msg.FileRequest)    ? MSG_FILEREQUEST : 0;
	jamHdr.Attribute |= (Msg.FileAttach)     ? MSG_FILEATTACH : 0;
	jamHdr.Attribute |= (Msg.TruncFile)      ? MSG_TRUNCFILE : 0;
	jamHdr.Attribute |= (Msg.KillFile)       ? MSG_KILLFILE : 0;
	jamHdr.Attribute |= (Msg.ReceiptRequest) ? MSG_RECEIPTREQ : 0;
	jamHdr.Attribute |= (Msg.ConfirmRequest) ? MSG_CONFIRMREQ : 0;
	jamHdr.Attribute |= (Msg.Orphan)         ? MSG_ORPHAN : 0;
	jamHdr.Attribute |= (Msg.Encrypt)        ? MSG_ENCRYPT : 0;
	jamHdr.Attribute |= (Msg.Compressed)     ? MSG_COMPRESS : 0;
	jamHdr.Attribute |= (Msg.Escaped)        ? MSG_ESCAPED : 0;
	jamHdr.Attribute |= (Msg.ForcePU)	 ? MSG_FPU : 0;
	jamHdr.Attribute |= (Msg.Localmail)      ? MSG_TYPELOCAL : 0;
	jamHdr.Attribute |= (Msg.Echomail)	 ? MSG_TYPEECHO : 0;
	jamHdr.Attribute |= (Msg.Netmail)        ? MSG_TYPENET : 0;
	jamHdr.Attribute |= (Msg.Nodisplay)	 ? MSG_NODISP : 0;
	jamHdr.Attribute |= (Msg.Locked)         ? MSG_LOCKED : 0;
	jamHdr.Attribute |= (Msg.Deleted)        ? MSG_DELETED : 0;

	jamHdr.ReplyTo = Msg.Original;
	jamHdr.ReplyNext = Msg.Reply;
	jamHdr.DateReceived = Msg.Read;
	jamHdr.MsgIdCRC = Msg.MsgIdCRC;
	jamHdr.ReplyCRC = Msg.ReplyCRC;
}



int JAM_SearchUser(unsigned int crcval)
{
    char		*temp;
    FILE		*fp;
    struct userhdr	usrhdr;
    struct userrec	usr;
    unsigned int	mycrc;
    int			rc = 0;

    if (crcval == 0)
	return -1;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX -1, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return -1;
    }
    free(temp);

    fread(&usrhdr, sizeof(usrhdr), 1, fp);
    while (fread(&usr, usrhdr.recsize, 1, fp) == 1) {
	mycrc = StringCRC32(tl(usr.sUserName));
	if (mycrc == crcval) {
	    fclose(fp);
	    return rc;
	}
	rc++;
    }

    /*
     * Nothing found
     */
    fclose(fp);
    return -1;
}




/*
 * Add a message, the structure msg must contain all needed
 * information.
 */
int JAM_AddMsg()
{
	int		i, RetVal = TRUE;
	unsigned int	ulMsg = JAM_Highest() + 1L;
	char		*pszText, *Sign= (char *)HEADERSIGNATURE;
	JAMIDXREC	jamIdx;
	int		Oke;

	memset(&jamHdr, 0, sizeof(JAMHDR));
	jamHdr.Signature[0] = Sign[0];
	jamHdr.Signature[1] = Sign[1];
	jamHdr.Signature[2] = Sign[2];
	jamHdr.Signature[3] = Sign[3];
	jamHdr.Revision = CURRENTREVLEV;
	jamHdr.MsgNum = ulMsg;

	jamHdr.DateWritten = Msg.Written;
	jamHdr.DateProcessed = Msg.Arrived;

	JAMset_flags();
	lseek(fdHdr, 0L, SEEK_END);

	jamIdx.UserCRC = 0;
	jamIdx.HdrOffset = tell(fdHdr);
	lseek(fdJdx, 0L, SEEK_END);
	write(fdJdx, &jamIdx, sizeof(JAMIDXREC));

	write(fdHdr, &jamHdr, sizeof(JAMHDR));

	jamHdr.SubfieldLen += AddSubfield(JAMSFLD_SENDERNAME, Msg.From);

	if (Msg.To[0])
		jamHdr.SubfieldLen += AddSubfield(JAMSFLD_RECVRNAME, Msg.To);

	jamHdr.SubfieldLen += AddSubfield(JAMSFLD_SUBJECT, Msg.Subject);

	if (Msg.FromAddress[0] != '\0')
		jamHdr.SubfieldLen += AddSubfield(JAMSFLD_OADDRESS, Msg.FromAddress);

	if (Msg.ToAddress[0] != '\0')
		jamHdr.SubfieldLen += AddSubfield(JAMSFLD_DADDRESS, Msg.ToAddress);

	Msg.Id = jamHdr.MsgNum;
	
	lseek(fdJdt, 0L, SEEK_END);
	jamHdr.TxtOffset = tell(fdJdt);
	jamHdr.TxtLen = 0;

	/*
	 * Read message text from memory, this also contains kludges.
	 * Extract those that are defined by the JAMmb specs, except
	 * the AREA: kludge. This one is only present in bad and dupe
	 * echomail areas and is present for tossbad and tossdupe.
	 */
	if ((pszText = (char *)MsgText_First ()) != NULL)
		do {
			if ((pszText[0] == '\001') || (!strncmp(pszText, "SEEN-BY:", 8))) {
				Oke = FALSE;

				if (!strncmp(pszText, "\001PID: ", 6)) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_PID, pszText + 6);
					Oke = TRUE;
				}

				if (!strncmp(pszText, "\001MSGID: ", 8)) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_MSGID, pszText + 8);
					Oke = TRUE;
				}

				if (!strncmp(pszText, "\001REPLY: ", 8)) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_REPLYID, pszText + 8);
					Oke = TRUE;
				}

				if (!strncmp(pszText, "\001PATH: ", 7)) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_PATH2D, pszText + 7);
					Oke = TRUE;
				}

				if (!strncmp(pszText, "\001Via", 4)) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_TRACE, pszText + 5);
					Oke = TRUE;
				}

				if (!strncmp(pszText, "SEEN-BY: ", 9)) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_SEENBY2D, pszText + 9);
					Oke = TRUE;
				}

				/*
				 * Other non-JAM kludges
				 */
				if ((!Oke) && (pszText[0] == '\001')) {
					jamHdr.SubfieldLen += AddSubfield(JAMSFLD_FTSKLUDGE, pszText + 1);
					Oke = TRUE;
				}

				if (!Oke) {
					for (i = 0; i < strlen(pszText); i++) {
						if (pszText[i] < 32)
							printf("<%x>", pszText[i]);
						else
							printf("%c", pszText[i]);
					}
				}
			} else {
				write(fdJdt, pszText, strlen (pszText));
				jamHdr.TxtLen += strlen (pszText);
				write(fdJdt, "\r", 1);
				jamHdr.TxtLen += 1;
			}
		} while ((pszText = (char *)MsgText_Next ()) != NULL);

	/*
	 * Write final message header
	 */
	lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
	write(fdHdr, &jamHdr, sizeof (JAMHDR));


	/*
	 * Update area information
	 */
	lseek(fdHdr, 0L, SEEK_SET);
	read(fdHdr, &jamHdrInfo, sizeof (JAMHDRINFO));
	jamHdrInfo.ActiveMsgs++;
	jamHdrInfo.ModCounter++;
	lseek(fdHdr, 0L, SEEK_SET);
	write(fdHdr, &jamHdrInfo, sizeof (JAMHDRINFO));

	return RetVal;
}



/*
 * Close current message base
 */
void JAM_Close(void)
{
	if (fdJdx != -1)
		close(fdJdx);
	if (fdJdt != -1)
		close(fdJdt);
	if (fdHdr != -1)
		close(fdHdr);
	if (fdJlr != -1)
		close(fdJlr);

	if (pSubfield != NULL)
		free(pSubfield);

	fdHdr = fdJdt = fdJdx = fdJlr = -1;
	pSubfield = NULL;
	Msg.Id = 0L;
}



/*
 * Delete message number
 */
int JAM_Delete(unsigned int ulMsg)
{
	int		RetVal = FALSE;
	JAMIDXREC	jamIdx;

	if (JAM_ReadHeader(ulMsg) == TRUE) {
		jamHdr.Attribute |= MSG_DELETED;

		lseek(fdJdx, tell(fdJdx) - sizeof(jamIdx), SEEK_SET);
		if (read(fdJdx, &jamIdx, sizeof(jamIdx)) == sizeof(jamIdx)) {
			lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
			write(fdHdr, &jamHdr, sizeof(JAMHDR));

			lseek(fdHdr, 0L, SEEK_SET);
			read(fdHdr, &jamHdrInfo, sizeof (JAMHDRINFO));
			jamHdrInfo.ActiveMsgs--;
			lseek(fdHdr, 0L, SEEK_SET);
			write(fdHdr, &jamHdrInfo, sizeof (JAMHDRINFO));
			RetVal = TRUE;
		}
	}

	return RetVal;
}



/*
 * Delete JAM area files
 */
void JAM_DeleteJAM(char *Base)
{
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX -1, "%s%s", Base, EXT_HDRFILE);
    unlink(temp);
    snprintf(temp, PATH_MAX -1, "%s%s", Base, EXT_IDXFILE);
    unlink(temp);
    snprintf(temp, PATH_MAX -1, "%s%s", Base, EXT_TXTFILE);
    unlink(temp);
    snprintf(temp, PATH_MAX -1, "%s%s", Base, EXT_LRDFILE);
    unlink(temp);
    free(temp);
    Syslog('+', "JAM deleted %s", Base);
}



/*
 * Search for requested LastRead record.
 */
int JAM_GetLastRead(lastread *LR)
{
    lastread	lr;

    LastReadRec = 0L;
    lseek(fdJlr, 0, SEEK_SET);

    while (read(fdJlr, &lr, sizeof(lastread)) == sizeof(lastread)) {
	/*
	 * Check for GoldED bug, the CRC == ID so the ID is invalid.
	 * Test for a valid CRC to find the right record.
	 */
	if ((lr.UserID == lr.UserCRC) && (lr.UserCRC == LR->UserCRC)) {
	    Syslog('m', "Found LR record for user %d using a workaround", LR->UserID);
	    LR->LastReadMsg = lr.LastReadMsg;
	    LR->HighReadMsg = lr.HighReadMsg;
	    return TRUE;
	}
	/*
	 * The way it should be.
	 */
	if ((lr.UserID != lr.UserCRC) && (lr.UserID == LR->UserID)) {
	    LR->LastReadMsg = lr.LastReadMsg;
	    LR->HighReadMsg = lr.HighReadMsg;
	    return TRUE;
	}
	LastReadRec++;
	
    }

    return FALSE;
}



/*
 * Get highest message number
 */
unsigned int JAM_Highest(void)
{
	unsigned int	RetVal = 0L;
	JAMIDXREC	jamIdx;

	if (jamHdrInfo.ActiveMsgs > 0L) {
		lseek(fdJdx, filelength(fdJdx) - sizeof(jamIdx), SEEK_SET);
		if (read(fdJdx, &jamIdx, sizeof(jamIdx)) == sizeof(jamIdx)) {
			lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
			read(fdHdr, &jamHdr, sizeof(JAMHDR));
			RetVal = jamHdr.MsgNum;
		}
	}

	Msg.Id = RetVal;

	return RetVal;
}



int JAM_Lock(unsigned int ulTimeout)
{
	int		rc, Tries = 0;
	struct flock	fl;

	fl.l_type   = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0L;
	fl.l_len    = 1L;		/* GoldED locks 1 byte as well */
	fl.l_pid    = getpid();

	while ((rc = fcntl(fdHdr, F_SETLK, &fl)) && ((errno == EACCES) || (errno == EAGAIN))) {
		if (++Tries >= (ulTimeout * 4)) {
			fcntl(fdHdr, F_GETLK, &fl);
			WriteError("JAM messagebase is locked by pid %d", fl.l_pid);
			return FALSE;
		}
		msleep(250);
		Syslog('m', "JAM messagebase lock attempt %d", Tries);
	}

	if (rc) {
		WriteError("$%s lock error", BaseName);
		return FALSE;
	}
	return TRUE;
}



/*
 * Get lowest message number 
 */
unsigned int JAM_Lowest(void)
{
	unsigned int	RetVal = 0L;
	JAMIDXREC	jamIdx;

	if (jamHdrInfo.ActiveMsgs > 0L) {
		lseek(fdJdx, 0L, SEEK_SET);
		if (read(fdJdx, &jamIdx, sizeof(jamIdx)) == sizeof(jamIdx)) {
			lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
			read(fdHdr, &jamHdr, sizeof(JAMHDR));
			RetVal = jamHdr.MsgNum;
		}
	}

	Msg.Id = RetVal;

	return RetVal;
}



void JAM_New(void)
{
	memset(&Msg, 0, sizeof(Msg));
	MsgText_Clear();
}



int JAM_NewLastRead(lastread LR)
{
	lseek(fdJlr, 0, SEEK_END);
	return (write(fdJlr, &LR, sizeof(lastread)) == sizeof(lastread));
}



int JAM_Next(unsigned int * ulMsg)
{
	int		RetVal = FALSE, MayBeNext = FALSE;
	JAMIDXREC	jamIdx;
	unsigned int	_Msg;

	_Msg = *ulMsg;

	if (jamHdrInfo.ActiveMsgs > 0L) {
		// --------------------------------------------------------------------
		// The first attempt to retrive the next message number suppose that
		// the file pointers are located after the current message number.
		// Usually this is the 99% of the situations because the messages are
		// often readed sequentially.
		// --------------------------------------------------------------------
		if (tell(fdJdx) >= sizeof (jamIdx))
			lseek(fdJdx, tell(fdJdx) - sizeof(jamIdx), SEEK_SET);
		do {
			if (read(fdJdx, &jamIdx, sizeof(jamIdx)) == sizeof(jamIdx)) {
				lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
				read(fdHdr, &jamHdr, sizeof (JAMHDR));
				if (MayBeNext == TRUE) {
					if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum > _Msg) {
						_Msg = jamHdr.MsgNum;
						RetVal = TRUE;
					}
				}
				if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == _Msg)
					MayBeNext = TRUE;
			}
		} while (RetVal == FALSE && tell(fdJdx) < filelength(fdJdx));

		if (RetVal == FALSE && MayBeNext == FALSE) {
		// --------------------------------------------------------------------
		// It seems that the file pointers are not located where they should be
		// so our next attempt is to scan the database from the beginning to
		// find the next message number.
		// --------------------------------------------------------------------
			lseek(fdJdx, 0L, SEEK_SET);
			do {
				if (read(fdJdx, &jamIdx, sizeof(jamIdx)) == sizeof(jamIdx)) {
					lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
					read(fdHdr, &jamHdr, sizeof (JAMHDR));
					if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum > _Msg) {
						_Msg = jamHdr.MsgNum;
						RetVal = TRUE;
					}
				}
			} while (RetVal == FALSE && tell(fdJdx) < filelength(fdJdx));
		}

		Msg.Id = 0L;
		if (RetVal == TRUE)
			Msg.Id = _Msg;
	}

	memcpy(ulMsg, &_Msg, sizeof(unsigned int));
	return RetVal;
}



/*
 * Return number of messages
 */
unsigned int JAM_Number(void)
{
	return jamHdrInfo.ActiveMsgs;
}



/*
 * Open specified JAM message base
 */
int JAM_Open(char *Msgbase)
{
	int	RetVal = FALSE;
	char	*File;
	char	*Signature = (char *)HEADERSIGNATURE;

	fdJdt = fdJdx = fdJlr = -1;
	pSubfield = NULL;
	File = calloc(PATH_MAX, sizeof(char));

	snprintf(File, PATH_MAX -1, "%s%s", Msgbase, EXT_HDRFILE);
	if ((fdHdr = open(File, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) != -1) {
		if (read(fdHdr, &jamHdrInfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO)) {
			memset(&jamHdrInfo, 0, sizeof(JAMHDRINFO));
			jamHdrInfo.Signature[0] = Signature[0];
			jamHdrInfo.Signature[1] = Signature[1];
			jamHdrInfo.Signature[2] = Signature[2];
			jamHdrInfo.Signature[3] = Signature[3];
			jamHdrInfo.DateCreated = time(NULL);
			jamHdrInfo.BaseMsgNum = 1;

			lseek(fdHdr, 0, SEEK_SET);
			write(fdHdr, &jamHdrInfo, sizeof(JAMHDRINFO));
			Syslog('+', "JAM created %s", Msgbase);
		}

		if (jamHdrInfo.Signature[0] == Signature[0] &&
		    jamHdrInfo.Signature[1] == Signature[1] &&
		    jamHdrInfo.Signature[2] == Signature[2] &&
		    jamHdrInfo.Signature[3] == Signature[3]) {
			snprintf(File, PATH_MAX -1, "%s%s", Msgbase, EXT_TXTFILE);
			fdJdt = open(File, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
			snprintf(File, PATH_MAX -1, "%s%s", Msgbase, EXT_IDXFILE);
			fdJdx = open(File, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
			snprintf(File, PATH_MAX -1, "%s%s", Msgbase, EXT_LRDFILE);
			fdJlr = open(File, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
			RetVal = TRUE;

			strcpy(BaseName, Msgbase);
		} else {
			close(fdHdr);
			fdHdr = -1;
		}
	} else
		memset(&jamHdrInfo, 0, sizeof(JAMHDRINFO));

	Msg.Id = 0L;
	free(File);

	if (!RetVal)
	    WriteError("JAM error open %s", Msgbase);

	return RetVal;
}



/*
 * Pack deleted messages from the message base. The messages are
 * renumbered on the fly and the LastRead pointers are updated.
 */
void JAM_Pack(void)
{
    int		    fdnHdr, fdnJdx, fdnJdt, fdnJlr;
    int		    ToRead, Readed, i, count;
    char	    *File, *New, *Subfield, *Temp;
    JAMIDXREC	    jamIdx;
    unsigned int    NewNumber = 0, RefNumber = 0, Written = 0, mycrc, myrec;
    lastread	    LR;
    FILE	    *usrF;
    struct userhdr  usrhdr;
    struct userrec  usr;

    File = calloc(PATH_MAX, sizeof(char));
    New  = calloc(PATH_MAX, sizeof(char));
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dr");
    fdnHdr = open(File, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dt");
    fdnJdt = open(File, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dx");
    fdnJdx = open(File, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$lr");
    fdnJlr = open(File, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    /*
     * Get the number of LastRead records, this number is needed to prevent
     * that FreeBSD makes garbage of the LastRead pointers for some reason.
     */
    count = lseek(fdJlr, 0, SEEK_END) / sizeof(lastread);

    if (fdnHdr != -1 && fdnJdt != -1 && fdnJdx != -1 && fdnJlr != -1) {
	lseek(fdHdr, 0L, SEEK_SET);
	if (read(fdHdr, &jamHdrInfo, sizeof(JAMHDRINFO)) == sizeof(JAMHDRINFO)) {
	    write(fdnHdr, &jamHdrInfo, sizeof(JAMHDRINFO));
	    while (read(fdHdr, &jamHdr, sizeof(JAMHDR)) == sizeof(JAMHDR)) {
		RefNumber++;
		if (strncmp(jamHdr.Signature, "JAM", 3)) {
		    WriteError("jamPack: %s headerfile corrupt", BaseName);
		    lseek(fdJdx, (RefNumber -1) * sizeof(JAMIDXREC), SEEK_SET);
		    read(fdJdx, &jamIdx, sizeof(JAMIDXREC));
		    lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
		    read(fdHdr, &jamHdr, sizeof(JAMHDR));
		    if ((strncmp(jamHdr.Signature, "JAM", 3) == 0) && (jamHdr.MsgNum == RefNumber))
			WriteError("jamPack: corrected the problem");
		    else {
			WriteError("jamPack: PANIC, problem cannot be solved, skipping this area");
			Written = 0;
			break;
		    }
		}
		if (jamHdr.Attribute & MSG_DELETED) {
		    if (jamHdr.SubfieldLen > 0L)
			lseek (fdHdr, jamHdr.SubfieldLen, SEEK_CUR);
                    lseek(fdJlr, 0, SEEK_SET);
		    for (i = 0; i < count; i++) {
			if ((read(fdJlr, &LR, sizeof(lastread)) == sizeof(lastread))) {
			    /*
			     * Test if one of the lastread pointer is the current
			     * old message number and is different then the new number.
			     */
			    if (((LR.LastReadMsg == jamHdr.MsgNum) || (LR.HighReadMsg == jamHdr.MsgNum)) &&
				    (jamHdr.MsgNum != NewNumber)) {
				/*
				 * Adjust the matching numbers
				 */
				if (LR.LastReadMsg == jamHdr.MsgNum) {
				    Syslog('m', "JAM_Pack (deleted) %s recno %d user %d LastRead %u -> %u",
					    BaseName, i, LR.UserID, jamHdr.MsgNum, NewNumber);
				    LR.LastReadMsg = NewNumber;
				}
				if (LR.HighReadMsg == jamHdr.MsgNum) {
				    Syslog('m', "JAM_Pack (deleted) %s recno %d user %d HighRead %u -> %u",
					    BaseName, i, LR.UserID, jamHdr.MsgNum, NewNumber);
				    LR.HighReadMsg = NewNumber;
				}
				lseek(fdJlr, - sizeof(lastread), SEEK_CUR);
				write(fdJlr, &LR, sizeof(lastread));
			    }
			}
		    }
		} else {
		    jamIdx.UserCRC = 0;
		    jamIdx.HdrOffset = tell(fdnHdr);
		    write(fdnJdx, &jamIdx, sizeof(JAMIDXREC));

		    lseek(fdJdt, jamHdr.TxtOffset, SEEK_SET);
		    jamHdr.TxtOffset = tell(fdnJdt);
		    NewNumber++;
		    Written++;

		    lseek(fdJlr, 0, SEEK_SET);
		    for (i = 0; i < count; i++) {
			if ((read(fdJlr, &LR, sizeof(lastread)) == sizeof(lastread))) {
			    /*
			     * Test if one of the lastread pointer is the current
			     * old message number and is different then the new number.
			     */
			    if (((LR.LastReadMsg == jamHdr.MsgNum) || (LR.HighReadMsg == jamHdr.MsgNum)) && 
				    (jamHdr.MsgNum != NewNumber)) {
				/*
				 * Adjust the matching numbers
				 */
				if (LR.LastReadMsg == jamHdr.MsgNum) {
				    Syslog('m', "JAM_Pack (active) %s recno %d user %d LastRead %u -> %u", 
					    BaseName, i, LR.UserID, jamHdr.MsgNum, NewNumber);
				    LR.LastReadMsg = NewNumber;
				}
				if (LR.HighReadMsg == jamHdr.MsgNum) {
				    Syslog('m', "JAM_Pack (active) %s recno %d user %d HighRead %u -> %u", 
					    BaseName, i, LR.UserID, jamHdr.MsgNum, NewNumber);
				    LR.HighReadMsg = NewNumber;
				}
				lseek(fdJlr, - sizeof(lastread), SEEK_CUR);
				write(fdJlr, &LR, sizeof(lastread));
			    }
			}
		    }
		    jamHdr.MsgNum = NewNumber;
		    write(fdnHdr, &jamHdr, sizeof(JAMHDR));

		    if (jamHdr.SubfieldLen > 0L) {
			if ((Subfield = (char *)malloc ((size_t)(jamHdr.SubfieldLen + 1))) != NULL) {
			    read (fdHdr, Subfield, (size_t)jamHdr.SubfieldLen);
			    write (fdnHdr, Subfield, (size_t)jamHdr.SubfieldLen);
			    free(Subfield);
			}
		    }

		    if ((Temp = (char *)malloc (MAX_TEXT)) != NULL) {
			do {
			    if ((ToRead = MAX_TEXT) > jamHdr.TxtLen)
				ToRead = (int)jamHdr.TxtLen;
			    Readed = (int)read (fdJdt, Temp, ToRead);
			    write (fdnJdt, Temp, Readed);
			    jamHdr.TxtLen -= Readed;
			} while (jamHdr.TxtLen > 0);
			free(Temp);
		    }
		}
	    }
	}

	/*
	 * Correct any errors in the header
	 */
	if (Written) {
	    lseek(fdnHdr, 0, SEEK_SET);
	    if (read(fdnHdr, &jamHdrInfo, sizeof(JAMHDRINFO)) == sizeof(JAMHDRINFO)) {
		if (jamHdrInfo.ActiveMsgs != Written) {
		    WriteError("jamPack: repair msgs %lu to %lu area %s", jamHdrInfo.ActiveMsgs, Written, BaseName);
		    jamHdrInfo.ActiveMsgs = Written;
		    lseek(fdnHdr, 0, SEEK_SET);
		    write(fdnHdr, &jamHdrInfo, sizeof(JAMHDRINFO));
		}
	    }
	}

	/*
	 * Now copy the lastread file, reset LastRead pointers if area is empty.
	 * Check for deleted users.
	 */
	snprintf(File, PATH_MAX -1, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((usrF = fopen(File, "r"))) {
	    fread(&usrhdr, sizeof(usrhdr), 1, usrF);
	}
	lseek(fdJlr, 0, SEEK_SET);
	for (i = 0; i < count; i++) {
	    if (read(fdJlr, &LR, sizeof(lastread)) == sizeof(lastread)) {
		if (jamHdrInfo.ActiveMsgs == 0 && (LR.LastReadMsg || LR.HighReadMsg)) {
		    LR.LastReadMsg = 0;
		    LR.HighReadMsg = 0;
		    Syslog('m', "JAM_Pack %s recno %d user %d LastRead and HighRead are reset", BaseName, i, LR.UserID);
		}
		if (jamHdrInfo.ActiveMsgs && (LR.LastReadMsg > jamHdrInfo.ActiveMsgs)) {
		    LR.LastReadMsg = jamHdrInfo.ActiveMsgs;
		    Syslog('m', "JAM_Pack %s recno %d user %d LastRead is reset to %d", BaseName, i, LR.UserID, LR.LastReadMsg);
		}
		if (jamHdrInfo.ActiveMsgs && (LR.HighReadMsg > jamHdrInfo.ActiveMsgs)) {
		    LR.HighReadMsg = jamHdrInfo.ActiveMsgs;
		    Syslog('m', "JAM_Pack %s recno %d user %d HighRead is reset to %d", BaseName, i, LR.UserID, LR.HighReadMsg);
		}
		if (usrF) {
		    if ((LR.UserID == LR.UserCRC) && LR.UserCRC) {
			/*
			 * GoldED bug, try to fix it. This might leave double records, we
			 * will deal with that later.
			 */
			fseek(usrF, usrhdr.hdrsize, SEEK_SET);
			myrec = 0;
			while (fread(&usr, usrhdr.recsize, 1, usrF) == 1) {
			    mycrc = StringCRC32(tl(usr.sUserName));
			    if (LR.UserCRC == mycrc) {
				LR.UserID = myrec;
				Syslog('m', "JAM_Pack %s recno %d LastRead UserID set to %d", BaseName, i, myrec);
			    }
			    myrec++;
			    write(fdnJlr, &LR, sizeof(lastread));
			}
		    } else {
			/*
		    	 * Search user record for LR pointer. If the record is valid and the
		    	 * user still exists then copy the LR record, else we drop it.
		    	 */
		    	fseek(usrF, usrhdr.hdrsize + (usrhdr.recsize * LR.UserID), SEEK_SET);
		    	memset(&usr, 0, sizeof(usr));
		    	if (fread(&usr, usrhdr.recsize, 1, usrF) == 1) {
			    mycrc = StringCRC32(tl(usr.sUserName));
			    if (mycrc == LR.UserCRC) {
			    	write(fdnJlr, &LR, sizeof(lastread));
			    } else {
			    	Syslog('-', "JAM_Pack %s purged LR record %d", BaseName, i);
			    }
			} else {
			    Syslog('-', "JAM_Pack %s purged LR record %d", BaseName, i);
			}
		    }
		} else {
		    /*
		     * Should not be possible, but simply write LR records
		     * if no user data is available.
		     */
		    write(fdnJlr, &LR, sizeof(lastread));
		}
	    }
	}
	if (usrF)
	    fclose(usrF);

	/*
	 * Close all files
	 */
	close(fdnHdr);
	close(fdnJdt);
	close(fdnJdx);
	close(fdnJlr);
	fdnHdr = fdnJdt = fdnJdx = fdnJlr = -1;

	close(fdHdr);
	close(fdJdt);
	close(fdJdx);
	close(fdJlr);
	fdHdr = fdJdt = fdJdx = fdJlr = -1;

	snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dr");
	snprintf(New, PATH_MAX -1, "%s%s", BaseName, EXT_HDRFILE);
	unlink(New);
	rename(File, New);
	snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dt");
	snprintf(New, PATH_MAX -1, "%s%s", BaseName, EXT_TXTFILE);
	unlink(New);
	rename(File, New);
	snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dx");
	snprintf(New, PATH_MAX -1, "%s%s", BaseName, EXT_IDXFILE);
	unlink(New);
	rename(File, New);
	snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$lr");
	snprintf(New, PATH_MAX -1, "%s%s", BaseName, EXT_LRDFILE);
	unlink(New);
	rename(File, New);

	snprintf(File, PATH_MAX -1, "%s", BaseName);
	JAM_Open(File);
    }

    if (fdnHdr != -1)
	close(fdnHdr);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dr");
    unlink(File);
    if (fdnJdt != -1)
	close(fdnJdt);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dt");
    unlink(File);
    if (fdnJdx != -1)
	close(fdnJdx);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$dx");
    unlink(File);
    if (fdnJlr != -1)
	close(fdnJlr);
    snprintf(File, PATH_MAX -1, "%s%s", BaseName, ".$lr");
    unlink(File);
    free(File);
    free(New);
}



int JAM_Previous (unsigned int *ulMsg)
{
	int		RetVal = FALSE, MayBeNext = FALSE;
	int		Pos;
	JAMIDXREC	jamIdx;
	unsigned int	_Msg;

	_Msg = *ulMsg;

	if (jamHdrInfo.ActiveMsgs > 0L) {
	// --------------------------------------------------------------------
	// The first attempt to retrive the next message number suppose that
	// the file pointers are located after the current message number.
	// Usually this is the 99% of the situations because the messages are
	// often readed sequentially.
	// --------------------------------------------------------------------
		if (tell (fdJdx) >= sizeof (jamIdx)) {
			Pos = tell (fdJdx) - sizeof (jamIdx);
			do {
				lseek (fdJdx, Pos, SEEK_SET);
				if (read (fdJdx, &jamIdx, sizeof (jamIdx)) == sizeof (jamIdx)) {
					lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
					read (fdHdr, &jamHdr, sizeof (JAMHDR));
					if (MayBeNext == TRUE) {
						if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum < _Msg) {
							_Msg = jamHdr.MsgNum;
							RetVal = TRUE;
						}
					}
					if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == _Msg)
						MayBeNext = TRUE;
				}
				Pos -= sizeof (jamIdx);
			} while (RetVal == FALSE && Pos >= 0L);
		}

		if (RetVal == FALSE && MayBeNext == FALSE) {
		// --------------------------------------------------------------------
		// It seems that the file pointers are not located where they should be
		// so our next attempt is to scan the database from the end to find
		// the next message number.
		// --------------------------------------------------------------------
			Pos = filelength (fdJdx) - sizeof (jamIdx);
			do {
				lseek (fdJdx, Pos, SEEK_SET);
				if (read (fdJdx, &jamIdx, sizeof (jamIdx)) == sizeof (jamIdx)) {
					lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
					read (fdHdr, &jamHdr, sizeof (JAMHDR));
					if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum < _Msg) {
						_Msg = jamHdr.MsgNum;
						RetVal = TRUE;
					}
				}
				Pos -= sizeof (jamIdx);
			} while (RetVal == FALSE && Pos >= 0L);
		}

		Msg.Id = 0L;
		if (RetVal == TRUE)
			Msg.Id = _Msg;
	}

	memcpy(ulMsg, &_Msg, sizeof(unsigned int));
	return (RetVal);
}



int JAM_ReadHeader (unsigned int ulMsg)
{
    int		    i, RetVal = FALSE;
    unsigned char   *pPos;
    unsigned int    ulSubfieldLen, tmp;
    JAMIDXREC	    jamIdx;
    JAMBINSUBFIELD  *jamSubField;

    tmp = Msg.Id;
    JAM_New ();
    Msg.Id = tmp;

    if (Msg.Id == ulMsg) {
	// --------------------------------------------------------------------
	// The user is requesting the header of the last message retrived
	// so our first attempt is to read the last index from the file and
	// check if this is the correct one.
	// --------------------------------------------------------------------
	lseek (fdJdx, tell (fdJdx) - sizeof (jamIdx), SEEK_SET);
	if (read (fdJdx, &jamIdx, sizeof (jamIdx)) == sizeof (jamIdx)) {
	    lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
	    read (fdHdr, &jamHdr, sizeof (JAMHDR));
	    if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == ulMsg)
		RetVal = TRUE;
	}
    }

    if (((Msg.Id + 1) == ulMsg) && (RetVal == FALSE)) {
	//---------------------------------------------------------------------
	// If the user is requesting the header of the next message we attempt
	// to read the next header and check if this is the correct one.
	// This is EXPERIMENTAL
	//---------------------------------------------------------------------
	if (read(fdJdx, &jamIdx, sizeof(jamIdx)) == sizeof(jamIdx)) {
	    lseek(fdHdr, jamIdx.HdrOffset, SEEK_SET);
	    read(fdHdr, &jamHdr, sizeof(JAMHDR));
	    if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == ulMsg)
		RetVal = TRUE;
	}
    }


    if (RetVal == FALSE) {
	// --------------------------------------------------------------------
	// The message request is not the last retrived or the file pointers
	// are not positioned where they should be, so now we attempt to
	// retrieve the message header scanning the database from the beginning.
	// --------------------------------------------------------------------
	Msg.Id = 0L;
	lseek (fdJdx, 0L, SEEK_SET);
	do {
	    if (read (fdJdx, &jamIdx, sizeof (jamIdx)) == sizeof (jamIdx)) {
		lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
		read (fdHdr, &jamHdr, sizeof (JAMHDR));
		if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == ulMsg)
		    RetVal = TRUE;
	    }
	} while (RetVal == FALSE && tell (fdJdx) < filelength (fdJdx));
    }

    if (RetVal == TRUE) {
	Msg.Current = Msg.Id = ulMsg;

	Msg.Local          = (unsigned char)((jamHdr.Attribute & MSG_LOCAL)       ? TRUE : FALSE);
	Msg.Intransit      = (unsigned char)((jamHdr.Attribute & MSG_INTRANSIT)   ? TRUE : FALSE);
	Msg.Private        = (unsigned char)((jamHdr.Attribute & MSG_PRIVATE)     ? TRUE : FALSE);
	Msg.Received       = (unsigned char)((jamHdr.Attribute & MSG_READ)        ? TRUE : FALSE);
	Msg.Sent           = (unsigned char)((jamHdr.Attribute & MSG_SENT)        ? TRUE : FALSE);
	Msg.KillSent       = (unsigned char)((jamHdr.Attribute & MSG_KILLSENT)    ? TRUE : FALSE);
	Msg.ArchiveSent    = (unsigned char)((jamHdr.Attribute & MSG_ARCHIVESENT) ? TRUE : FALSE);
	Msg.Hold           = (unsigned char)((jamHdr.Attribute & MSG_HOLD)        ? TRUE : FALSE);
	Msg.Crash          = (unsigned char)((jamHdr.Attribute & MSG_CRASH)       ? TRUE : FALSE);
	Msg.Immediate      = (unsigned char)((jamHdr.Attribute & MSG_IMMEDIATE)   ? TRUE : FALSE);
	Msg.Direct         = (unsigned char)((jamHdr.Attribute & MSG_DIRECT)      ? TRUE : FALSE);
	Msg.Gate	   = (unsigned char)((jamHdr.Attribute & MSG_GATE)        ? TRUE : FALSE);
	Msg.FileRequest    = (unsigned char)((jamHdr.Attribute & MSG_FILEREQUEST) ? TRUE : FALSE);
	Msg.FileAttach     = (unsigned char)((jamHdr.Attribute & MSG_FILEATTACH)  ? TRUE : FALSE);
	Msg.TruncFile      = (unsigned char)((jamHdr.Attribute & MSG_TRUNCFILE)   ? TRUE : FALSE);
	Msg.KillFile       = (unsigned char)((jamHdr.Attribute & MSG_KILLFILE)    ? TRUE : FALSE);
	Msg.ReceiptRequest = (unsigned char)((jamHdr.Attribute & MSG_RECEIPTREQ)  ? TRUE : FALSE);
	Msg.ConfirmRequest = (unsigned char)((jamHdr.Attribute & MSG_CONFIRMREQ)  ? TRUE : FALSE);
	Msg.Orphan         = (unsigned char)((jamHdr.Attribute & MSG_ORPHAN)      ? TRUE : FALSE);
	Msg.Encrypt        = (unsigned char)((jamHdr.Attribute & MSG_ENCRYPT)     ? TRUE : FALSE);
	Msg.Compressed     = (unsigned char)((jamHdr.Attribute & MSG_COMPRESS)    ? TRUE : FALSE);
	Msg.Escaped        = (unsigned char)((jamHdr.Attribute & MSG_ESCAPED)     ? TRUE : FALSE);
	Msg.ForcePU        = (unsigned char)((jamHdr.Attribute & MSG_FPU)         ? TRUE : FALSE);
	Msg.Localmail	   = (unsigned char)((jamHdr.Attribute & MSG_TYPELOCAL)   ? TRUE : FALSE);
	Msg.Echomail       = (unsigned char)((jamHdr.Attribute & MSG_TYPEECHO)    ? TRUE : FALSE);
	Msg.Netmail        = (unsigned char)((jamHdr.Attribute & MSG_TYPENET)     ? TRUE : FALSE);
	Msg.Nodisplay      = (unsigned char)((jamHdr.Attribute & MSG_NODISP)      ? TRUE : FALSE);
	Msg.Locked         = (unsigned char)((jamHdr.Attribute & MSG_LOCKED)      ? TRUE : FALSE);
	Msg.Deleted        = (unsigned char)((jamHdr.Attribute & MSG_DELETED)     ? TRUE : FALSE);

	Msg.Written = jamHdr.DateWritten;
	Msg.Arrived = jamHdr.DateProcessed;
	Msg.Read    = jamHdr.DateReceived;

	Msg.Original = jamHdr.ReplyTo;
	Msg.Reply = jamHdr.ReplyNext;

	if (pSubfield != NULL)
	    free (pSubfield);
	pSubfield = NULL;

	if (jamHdr.SubfieldLen > 0L) {
	    ulSubfieldLen = jamHdr.SubfieldLen;
	    pPos = pSubfield = (unsigned char *)malloc ((size_t)(ulSubfieldLen + 1));
	    if (pSubfield == NULL)
		return (FALSE);

	    read (fdHdr, pSubfield, (size_t)jamHdr.SubfieldLen);

	    while (ulSubfieldLen > 0L) {
		jamSubField = (JAMBINSUBFIELD *)pPos;
		pPos += sizeof (JAMBINSUBFIELD);
		/*
		 * The next check is to prevent a segmentation
		 * fault by corrupted subfields.
		 */
		if ((jamSubField->DatLen < 0) || (jamSubField->DatLen > jamHdr.SubfieldLen))
		    return FALSE;

		switch (jamSubField->LoID) {
		    case JAMSFLD_SENDERNAME:
			if (jamSubField->DatLen > 100) {
			    memcpy (Msg.From, pPos, 100);
			    Msg.From[100] = '\0';
			} else {
			    memcpy (Msg.From, pPos, (int)jamSubField->DatLen);
			    Msg.From[(int)jamSubField->DatLen] = '\0';
			}
			break;

		    case JAMSFLD_RECVRNAME:
			if (jamSubField->DatLen > 100) {
			    memcpy (Msg.To, pPos, 100);
			    Msg.To[100] = '\0';
			} else {
			    memcpy (Msg.To, pPos, (int)jamSubField->DatLen);
			    Msg.To[(int)jamSubField->DatLen] = '\0';
			}
			break;

		    case JAMSFLD_SUBJECT:
			if (jamSubField->DatLen > 100) {
			    memcpy (Msg.Subject, pPos, 100);
			    Msg.Subject[100] = '\0';
			} else {
			    memcpy (Msg.Subject, pPos, (int)jamSubField->DatLen);
			    Msg.Subject[(int)jamSubField->DatLen] = '\0';
			}
			break;

		    case JAMSFLD_OADDRESS:
			if (jamSubField->DatLen > 100) {
			    memcpy (Msg.FromAddress, pPos, 100);
			    Msg.FromAddress[100] = '\0';
			} else {
			    memcpy (Msg.FromAddress, pPos, (int)jamSubField->DatLen);
			    Msg.FromAddress[(int)jamSubField->DatLen] = '\0';
			}
			break;

		    case JAMSFLD_DADDRESS:
			if (jamSubField->DatLen > 100) {
			    memcpy(Msg.ToAddress, pPos, 100);
			    Msg.ToAddress[100] = '\0';
			} else {
			    memcpy (Msg.ToAddress, pPos, (int)jamSubField->DatLen);
			    Msg.ToAddress[(int)jamSubField->DatLen] = '\0';
			}
			break;

		    case JAMSFLD_MSGID:
			memcpy (Msg.Msgid, pPos, (int)jamSubField->DatLen);
			Msg.Msgid[(int)jamSubField->DatLen] = '\0';
			break;

		    case JAMSFLD_REPLYID:
			memcpy (Msg.Replyid, pPos, (int)jamSubField->DatLen);
			Msg.Replyid[(int)jamSubField->DatLen] = '\0';
			break;

		    default:
			break;
		}
		ulSubfieldLen -= sizeof (JAMBINSUBFIELD) + jamSubField->DatLen;
		if (ulSubfieldLen > 0)
		    pPos += (int)jamSubField->DatLen;
	    }
	}
	/*
	 *  In the original BBS we found that GEcho was not
	 *  setting the FromAddress. We take it from the MSGID
	 *  if there is one.
	 */
	if ((!strlen(Msg.FromAddress)) && (strlen(Msg.Msgid))) {
	    for (i = 0; i < strlen(Msg.Msgid); i++) {
		if ((Msg.Msgid[i] == '@') || (Msg.Msgid[i] == ' '))
		    break;
		Msg.FromAddress[i] = Msg.Msgid[i];
	    }
	}
    }

    return (RetVal);
}



/*
 * Read message
 */
int JAM_Read(unsigned int ulMsg, int nWidth)
{
	int		RetVal = FALSE, SkipNext;
	int		i, nReaded, nCol, nRead;
	unsigned char	*pPos;
	unsigned int	ulTxtLen, ulSubfieldLen;
	JAMIDXREC	jamIdx;
	JAMBINSUBFIELD	*jamSubField;
	LDATA		*Bottom = NULL, *New;

	MsgText_Clear();

	if ((RetVal = JAM_ReadHeader(ulMsg)) == TRUE) {
		lseek (fdJdx, tell (fdJdx) - sizeof (jamIdx), SEEK_SET);
		read (fdJdx, &jamIdx, sizeof (jamIdx));
		lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
		read (fdHdr, &jamHdr, sizeof (JAMHDR));

		if (pSubfield != NULL)
			free (pSubfield);
		pSubfield = NULL;

		if (jamHdr.SubfieldLen > 0L) {
			ulSubfieldLen = jamHdr.SubfieldLen;
			pPos = pSubfield = (unsigned char *)malloc ((size_t)(ulSubfieldLen + 1));
			if (pSubfield == NULL)
				return (FALSE);

			read (fdHdr, pSubfield, (size_t)jamHdr.SubfieldLen);

			while (ulSubfieldLen > 0L) {
				jamSubField = (JAMBINSUBFIELD *)pPos;
				pPos += sizeof (JAMBINSUBFIELD);

				/*
				 * Check for corrupted subfields
				 */
				if ((jamSubField->DatLen < 0) || (jamSubField->DatLen > jamHdr.SubfieldLen))
					return FALSE;

				switch (jamSubField->LoID) {
				case JAMSFLD_MSGID:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					memset(&Msg.Msgid, 0, sizeof(Msg.Msgid));
					snprintf(Msg.Msgid, 80, "%s", szBuff);
					snprintf(szLine, MAX_LINE_LENGTH, "\001MSGID: %s", szBuff);
					MsgText_Add2(szLine);
					break;

				case JAMSFLD_REPLYID:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf(szLine, MAX_LINE_LENGTH, "\001REPLY: %s", szBuff);
					MsgText_Add2(szLine);
					break;

				case JAMSFLD_PID:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf(szLine, MAX_LINE_LENGTH, "\001PID: %s", szBuff);
					MsgText_Add2(szLine);
					break;

				case JAMSFLD_TRACE:
					memcpy(szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf(szLine, MAX_LINE_LENGTH, "\001Via %s", szBuff);
					MsgText_Add2(szLine);
					break;

				case JAMSFLD_FTSKLUDGE:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					if (!strncmp(szBuff, "AREA:", 5))
						snprintf(szLine, MAX_LINE_LENGTH, "%s", szBuff);
					else {
						snprintf(szLine, MAX_LINE_LENGTH, "\001%s", szBuff);
						if (strncmp(szLine, "\001REPLYADDR:", 11) == 0) {
							snprintf(Msg.ReplyAddr, 80, "%s", szLine+12);
						}
						if (strncmp(szLine, "\001REPLYTO:", 9) == 0) {
							snprintf(Msg.ReplyTo, 80, "%s", szLine+10);
						}
						if (strncmp(szLine, "\001REPLYADDR", 10) == 0) {
							snprintf(Msg.ReplyAddr, 80, "%s", szLine+11);
						}
						if (strncmp(szLine, "\001REPLYTO", 8) == 0) {
							snprintf(Msg.ReplyTo, 80, "%s", szLine+9);
						}
					}
					MsgText_Add2(szLine);
					break;

				case JAMSFLD_SEENBY2D:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf (szLine, MAX_LINE_LENGTH, "SEEN-BY: %s", szBuff);
					if ((New = (LDATA *)malloc(sizeof(LDATA))) != NULL) {
						memset(New, 0, sizeof(LDATA));
						New->Value = strdup(szLine);
						if (Bottom != NULL) {
							while (Bottom->Next != NULL)
								Bottom = Bottom->Next;
							New->Previous = Bottom;
							New->Next = Bottom->Next;
							if (New->Next != NULL)
								New->Next->Previous = New;
							Bottom->Next = New;
						}
						Bottom = New;
					}
					break;

				case JAMSFLD_PATH2D:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf(szLine, MAX_LINE_LENGTH, "\001PATH: %s", szBuff);
					if ((New = (LDATA *)malloc(sizeof(LDATA))) != NULL) {
						memset(New, 0, sizeof(LDATA));
						New->Value = strdup(szLine);
						if (Bottom != NULL) {
							while (Bottom->Next != NULL)
								Bottom = Bottom->Next;
							New->Previous = Bottom;
							New->Next = Bottom->Next;
							if (New->Next != NULL)
								New->Next->Previous = New;
							Bottom->Next = New;
						}
						Bottom = New;
					}
					break;

				case JAMSFLD_FLAGS:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf(szLine, MAX_LINE_LENGTH, "\001FLAGS %s", szLine);
					MsgText_Add2(szLine);
					break;

				case JAMSFLD_TZUTCINFO:
					memcpy (szBuff, pPos, (int)jamSubField->DatLen);
					szBuff[(int)jamSubField->DatLen] = '\0';
					snprintf(szBuff, MAX_LINE_LENGTH, "\001TZUTC %s", szLine);
					MsgText_Add2(szLine);
					break;

				default:
					break;
				}

				ulSubfieldLen -= sizeof (JAMBINSUBFIELD) + jamSubField->DatLen;
				if (ulSubfieldLen > 0)
					pPos += (int)jamSubField->DatLen;
			}
		}

		lseek (fdJdt, jamHdr.TxtOffset, SEEK_SET);
		ulTxtLen = jamHdr.TxtLen;
		pLine = szLine;
		nCol = 0;
		SkipNext = FALSE;

		do {
			if ((unsigned int)(nRead = sizeof (szBuff)) > ulTxtLen)
				nRead = (int)ulTxtLen;

			nReaded = (int)read (fdJdt, szBuff, nRead);

			for (i = 0, pBuff = szBuff; i < nReaded; i++, pBuff++) {
				if (*pBuff == '\r') {
					*pLine = '\0';
					if (pLine > szLine && SkipNext == TRUE) {
						pLine--;
						while (pLine > szLine && *pLine == ' ')
							*pLine-- = '\0';
						if (pLine > szLine)
							MsgText_Add3(szLine, (int)(strlen (szLine) + 1));
					} else 
						if (SkipNext == FALSE)
							MsgText_Add2(szLine);
					SkipNext = FALSE;
					pLine = szLine;
					nCol = 0;
				} else
					if (*pBuff != '\n') {
						*pLine++ = *pBuff;
						nCol++;
						if (nCol >= nWidth) {
							*pLine = '\0';
							if (strchr (szLine, ' ') != NULL) {
								while (nCol > 1 && *pLine != ' ') {
									nCol--;
									pLine--;
								}
								if (nCol > 0) {
									while (*pLine == ' ')
										pLine++;
									strcpy (szWrp, pLine);
								}
								*pLine = '\0';
							} else
								szWrp[0] = '\0';
							MsgText_Add2(szLine);
							strcpy (szLine, szWrp);
							pLine = strchr (szLine, '\0');
							nCol = (int)strlen (szLine);
							SkipNext = TRUE;
						}
					}
			}

			ulTxtLen -= nRead;
		} while (ulTxtLen > 0);

		if (Bottom != NULL) {
			while (Bottom->Previous != NULL)
				Bottom = Bottom->Previous;
			MsgText_Add2(Bottom->Value);

			while (Bottom->Next != NULL) {
				Bottom = Bottom->Next;
				MsgText_Add2(Bottom->Value);
			}
			while (Bottom != NULL) {
				if (Bottom->Previous != NULL)
					Bottom->Previous->Next = Bottom->Next;
				if (Bottom->Next != NULL)
					Bottom->Next->Previous = Bottom->Previous;
				New = Bottom;
				if (Bottom->Next != NULL)
					Bottom = Bottom->Next;
				else if (Bottom->Previous != NULL)
					Bottom = Bottom->Previous;
				else
					Bottom = NULL;
				free(New->Value);
				free(New);
			}
		}
	}

	return (RetVal);
}



int JAM_SetLastRead(lastread LR)
{
	if (lseek(fdJlr, LastReadRec * sizeof(lastread), SEEK_SET) != -1)
		if (write(fdJlr, &LR, sizeof(lastread)) == sizeof(lastread))
			return TRUE;
	return FALSE;
}



/*
 * Unlock the message base
 */
void JAM_UnLock(void)
{
	struct flock    fl;

	fl.l_type   = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0L;
	fl.l_len    = 1L;               /* GoldED locks 1 byte as well */
	fl.l_pid    = getpid();

	if (fcntl(fdHdr, F_SETLK, &fl)) {
		WriteError("$Can't unlock JAM message base %s", BaseName);
	}
}



/*
 * Write message header
 */
int JAM_WriteHeader (unsigned int ulMsg)
{
	int		RetVal = FALSE;
	JAMIDXREC	jamIdx;

	if (Msg.Id == ulMsg) {
	// --------------------------------------------------------------------
	// The user is requesting to write the header of the last message
	// retrived so our first attempt is to read the last index from the
	// file and check if this is the correct one.
	// --------------------------------------------------------------------
		lseek (fdJdx, tell (fdJdx) - sizeof (jamIdx), SEEK_SET);
		if (read (fdJdx, &jamIdx, sizeof (jamIdx)) == sizeof (jamIdx)) {
			lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
			read (fdHdr, &jamHdr, sizeof (JAMHDR));
			if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == ulMsg)
				RetVal = TRUE;
		}
	}

	if (RetVal == FALSE) {
	// --------------------------------------------------------------------
	// The message requested is not the last retrived or the file pointers
	// are not positioned where they should be, so now we attempt to
	// retrive the message header scanning the database from the beginning.
	// --------------------------------------------------------------------
		Msg.Id = 0L;
		lseek (fdJdx, 0L, SEEK_SET);
		do {
			if (read (fdJdx, &jamIdx, sizeof (jamIdx)) == sizeof (jamIdx)) {
				lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
				read (fdHdr, &jamHdr, sizeof (JAMHDR));
				if (!(jamHdr.Attribute & MSG_DELETED) && jamHdr.MsgNum == ulMsg)
					RetVal = TRUE;
			}
		} while (RetVal == FALSE && tell (fdJdx) < filelength (fdJdx));
	}

	if (RetVal == TRUE) {
		Msg.Id = jamHdr.MsgNum;
		jamHdr.Attribute &= MSG_DELETED;
		JAMset_flags();
		
		lseek (fdHdr, jamIdx.HdrOffset, SEEK_SET);
		write (fdHdr, &jamHdr, sizeof (JAMHDR));
	}

	return RetVal;
}


