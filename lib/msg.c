/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Global message base functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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

#include "../config.h"
#include "libs.h"
#include "msgtext.h"
#include "msg.h"
#include "clcomm.h"
#include "structs.h"
#include "common.h"
#include "jammsg.h"



char *strlwr (char *s)
{
	char *p = s;

	while (*p != '\0') {
		*p = (char)tolower (*p);
		p++;
	}

	return (s);
}



char *strupr (char *s)
{
	char *p = s;

	while (*p != '\0') {
		*p = (char)toupper (*p);
		p++;
	}

	return (s);
}



long filelength(int fd)
{
	long		retval = -1L;
	struct stat	buf;

	if (fd != -1) {
		fstat(fd, &buf);
		retval = buf.st_size;
	}

	return (retval);
}



long tell(int fd)
{
	long	retval = -1L;

	if (fd != -1)
		retval = lseek(fd, 0L, SEEK_CUR);

	return retval;
}



/*
 * Add a message
 */
int Msg_AddMsg()
{
	if (!MsgBase.Locked)
		return FALSE;

	return JAM_AddMsg();
}



/*
 * Close current message base
 */
void Msg_Close(void)
{
	if (MsgBase.Locked)
		Msg_UnLock();

	JAM_Close();
	MsgText_Clear();
	MsgBase.Open = FALSE;
}



/*
 * Delete message number
 */
int Msg_Delete(unsigned long ulMsg)
{
	if (!MsgBase.Locked)
		return FALSE;
		
	return JAM_Delete(ulMsg);
}



/*
 * Delete message base
 */
void Msg_DeleteMsgBase(char *Base)
{
    JAM_DeleteJAM(Base);
}



int Msg_GetLastRead(lastread *LR)
{
	return JAM_GetLastRead(LR);
}



/*
 * Get highest message number
 */
unsigned long Msg_Highest(void)
{
	return MsgBase.Highest = JAM_Highest();
}



int Msg_Lock(unsigned long ulTimeout)
{
	return MsgBase.Locked = JAM_Lock(ulTimeout);
}



/*
 * Get lowest message number 
 */
unsigned long Msg_Lowest(void)
{
	return MsgBase.Lowest = JAM_Lowest();
}



void Msg_New(void)
{
	JAM_New();
}



int Msg_NewLastRead(lastread LR)
{
	return JAM_NewLastRead(LR);
}



int Msg_Next(unsigned long * ulMsg)
{
	return JAM_Next(ulMsg);
}



/*
 * Return number of messages
 */
unsigned long Msg_Number(void)
{
	return MsgBase.Total = JAM_Number();
}



/*
 * Open specified message base
 */
int Msg_Open(char *Base)
{
	int	RetVal = FALSE;

	if (MsgBase.Open) {
		if (strcmp(MsgBase.Path, Base) != 0)
			Msg_Close();
		else
			return TRUE;
	}

	RetVal = JAM_Open(Base);

	MsgBase.Open = RetVal;

	strcpy(MsgBase.Path, Base);
	return RetVal;
}



/*
 * Pack deleted messages from the message base.
 */
void Msg_Pack(void)
{
	if (!MsgBase.Locked)
		return;
		
	JAM_Pack();
}



int Msg_Previous (unsigned long * ulMsg)
{
	return JAM_Previous(ulMsg);
}



int Msg_ReadHeader (unsigned long ulMsg)
{
	return JAM_ReadHeader(ulMsg);
}



/*
 * Read message
 */
int Msg_Read(unsigned long ulMsg, int nWidth)
{
	return JAM_Read(ulMsg, nWidth);
}



int Msg_SetLastRead(lastread LR)
{
	if (!MsgBase.Locked)
		return FALSE;
		
	return JAM_SetLastRead(LR);
}



/*
 * Unlock the message base
 */
void Msg_UnLock(void)
{
	JAM_UnLock();
	MsgBase.Locked = FALSE;
}



/*
 * Write message header
 */
int Msg_WriteHeader (unsigned long ulMsg)
{
	if (!MsgBase.Locked)
		return FALSE;
		
	return JAM_WriteHeader(ulMsg);
}



/*
 * Write messagetext from file, strip linefeeds.
 */
void Msg_Write(FILE *fp)
{
	char	*Buf;
	int	i;

	Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
	while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {

		for (i = 0; i < strlen(Buf); i++) {
			if (*(Buf + i) == '\0')
				break;
			if (*(Buf + i) == '\n')
				*(Buf + i) = '\0';
			if (*(Buf + i) == '\r')
				*(Buf + i) = '\0';
		}

		MsgText_Add2(Buf);
	}

	free(Buf);
}


typedef struct {
    unsigned long   Subject;
    unsigned long   Number;
} MSGLINK;



/*
 * Link messages in one area.
 * Returns -1 if error, else the number of linked messages.
 */
int Msg_Link(char *Path, int do_quiet, int slow_util)
{
    int             i, m, msg_link = 0;
    unsigned long   Number, Prev, Next, Crc, Total;
    char            Temp[128], *p;
    MSGLINK         *Link;
        
    if (! Msg_Open(Path)) {
	return -1;
    }

    if (!do_quiet) {
	colour(12, 0);
	printf(" (linking)");
	colour(13, 0);
	fflush(stdout);
    }

    if ((Total = Msg_Number()) != 0L) {
	if (Msg_Lock(30L)) {
	    if ((Link = (MSGLINK *)malloc((Total + 1) * sizeof(MSGLINK))) != NULL) {
		memset(Link, 0, (Total + 1) * sizeof(MSGLINK));
		Number = Msg_Lowest();
		i = 0;
		do {
		    Msg_ReadHeader(Number);
		    strcpy(Temp, Msg.Subject);
		    p = strupr(Temp);
		    if (!strncmp(p, "RE:", 3)) {
			p += 3;
			if (*p == ' ')
			    p++;
		    }
		    Link[i].Subject = StringCRC32(p);
		    Link[i].Number = Number;
		    i++;

		    if (slow_util && do_quiet && ((i % 5) == 0))
			msleep(1);

		    if (((i % 10) == 0) && (!do_quiet)) {
			printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
			fflush(stdout);
		    }
		} while(Msg_Next(&Number) == TRUE);

		if (!do_quiet) {
		    printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
		    fflush(stdout);
		}
		Number = Msg_Lowest();
		i = 0;
		do {
		    Msg_ReadHeader(Number);
		    Prev = Next = 0;
		    Crc = Link[i].Subject;

		    for (m = 0; m < Total; m++) {
			if (m == i)
			    continue;
			if (Link[m].Subject == Crc) {
			    if (m < i)
				Prev = Link[m].Number;
			    else if (m > i) {
				Next = Link[m].Number;
				break;
			    }
			}
		    }

		    if (slow_util && do_quiet && ((i % 5) == 0))
			msleep(1);
                                                
		    if (((i % 10) == 0) && (!do_quiet)) {
			printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
			fflush(stdout);
		    }

		    if (Msg.Original != Prev || Msg.Reply != Next) {
			Msg.Original = Prev;
			Msg.Reply = Next;
			Msg_WriteHeader(Number);
			msg_link++;
		    }

		    i++;
	    
		} while(Msg_Next(&Number) == TRUE);

		if (!do_quiet) {
		    printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
		    fflush(stdout);
		}

		free(Link);
	    }

	    if (!do_quiet) {
		printf("               \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	    }
	    Msg_UnLock();
	} else {
	    Syslog('+', "Can't lock %s", Path);
	    return -1;
	}
    }

    Msg_Close();

    if (!do_quiet) {
	printf("\b\b\b\b\b\b\b\b\b\b          \b\b\b\b\b\b\b\b\b\b");
	fflush(stdout);
    }
    return msg_link;
}


