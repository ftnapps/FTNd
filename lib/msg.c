/*****************************************************************************
 *
 * $Id: msg.c,v 1.15 2005/10/11 20:49:44 mbse Exp $
 * Purpose ...............: Global message base functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "mbselib.h"
#include "msgtext.h"
#include "msg.h"
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



int filelength(int fd)
{
    int		retval = -1L;
    struct stat	buf;

    if (fd != -1) {
	fstat(fd, &buf);
	retval = buf.st_size;
    }

    return (retval);
}



int tell(int fd)
{
	int retval = -1L;

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
int Msg_Delete(unsigned int ulMsg)
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
unsigned int Msg_Highest(void)
{
	return MsgBase.Highest = JAM_Highest();
}



int Msg_Lock(unsigned int ulTimeout)
{
	return MsgBase.Locked = JAM_Lock(ulTimeout);
}



/*
 * Get lowest message number 
 */
unsigned int Msg_Lowest(void)
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



int Msg_Next(unsigned int * ulMsg)
{
	return JAM_Next(ulMsg);
}



/*
 * Return number of messages
 */
unsigned int Msg_Number(void)
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



int Msg_Previous (unsigned int * ulMsg)
{
	return JAM_Previous(ulMsg);
}



int Msg_ReadHeader (unsigned int ulMsg)
{
	return JAM_ReadHeader(ulMsg);
}



/*
 * Read message
 */
int Msg_Read(unsigned int ulMsg, int nWidth)
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
int Msg_WriteHeader (unsigned int ulMsg)
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

	Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
	while ((Fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL)
		MsgText_Add2(Buf);

	free(Buf);
}
 

typedef struct {
    unsigned int    Subject;
    unsigned int    Number;
} MSGLINK;



/*
 * Changes ansi background and foreground color
 */
void msg_colour(int, int);
void msg_colour(int fg, int bg)
{
    int att=0, fore=37, back=40;

    if (fg<0 || fg>31 || bg<0 || bg>7) {
	fprintf(stdout, "ANSI: Illegal colour specified: %i, %i\n", fg, bg);
	fflush(stdout);
	return; 
    }
    fprintf(stdout, "[");

    if ( fg > WHITE) {
	fprintf(stdout, "5;");
	fg-= 16;
    }
    if (fg > LIGHTGRAY) {
	att=1;
	fg=fg-8;
    }

    if      (fg == BLACK)   fore=30;
    else if (fg == BLUE)    fore=34;
    else if (fg == GREEN)   fore=32;
    else if (fg == CYAN)    fore=36;
    else if (fg == RED)     fore=31;
    else if (fg == MAGENTA) fore=35;
    else if (fg == BROWN)   fore=33;
    else                    fore=37;

    if      (bg == BLUE)      back=44;
    else if (bg == GREEN)     back=42;
    else if (bg == CYAN)      back=46;
    else if (bg == RED)       back=41;
    else if (bg == MAGENTA)   back=45;
    else if (bg == BROWN)     back=43;
    else if (bg == LIGHTGRAY) back=47;
    else                      back=40;

    fprintf(stdout, "%d;%d;%dm", att, fore, back);
    fflush(stdout);
}




/*
 * Link messages in one area.
 * Returns -1 if error, else the number of linked messages.
 */
int Msg_Link(char *Path, int do_quiet, int slow_util)
{
    int             i, m, msg_link = 0;
    unsigned int    Number, Prev, Next, Crc, Total;
    char            Temp[128], *p;
    MSGLINK         *Link;
        
    if (! Msg_Open(Path)) {
	return -1;
    }

    if (!do_quiet) {
	msg_colour(LIGHTRED, BLACK);
	printf(" (linking)");
	msg_colour(LIGHTMAGENTA, BLACK);
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

		    if (((i % 10) == 0) && (!do_quiet)) {
			printf("%6d / %6u\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
			fflush(stdout);
		    }
		} while(Msg_Next(&Number) == TRUE);

		if (!do_quiet) {
		    printf("%6d / %6u\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
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

		    if (((i % 10) == 0) && (!do_quiet)) {
			printf("%6d / %6u\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
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
		    printf("%6d / %6u\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
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



/*
 * Fgets() is like fgets() but never returns the line terminator
 * at end of line and handles that line terminators:
 *
 * DOS/WINDOWS	->  CR/LF
 * UNIX		->  LF only
 * MAC		->  CR only
 */

char *Fgets(char *l, int size, FILE *f) {
  char *cp = l;
  int  cr, eol = FALSE;

  if (feof(f)) return NULL;

  cr = FALSE;
  while (size>1 && !feof(f)) {
    int c = fgetc(f);
    if (c == EOF) {
      if (ferror(f)) return NULL;
      break;
    }
    if (cr && c != '\n') {
      /* CR end-of-line (MAC) */
      ungetc(c,f);
      eol = TRUE;
      break;
    } else
	 cr = (c=='\r');
	 if ( cr )
      continue;
    --size;
    if (c=='\n') { eol = TRUE; break; }
    *(cp++) = c;
  }
  *cp = '\0';

  cr = FALSE;
  while (!eol && !feof(f)) {
    int c = fgetc(f);
    if (c == EOF)
      break;
    if (cr && c != '\n') {
      /* CR end-of-line (MAC) */
      ungetc(c,f);
      break;
	 } else
	 cr = (c=='\r');
	 if ( cr )
		continue;
    if (c=='\n') break;
  }
  return l;
}
