/*****************************************************************************
 *
 * $Id: msgtext.c,v 1.5 2004/02/21 14:24:04 mbroek Exp $
 * Purpose ...............: Message text memory storage.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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


LDATA	*List;




unsigned short MsgText_Add1(void * lpData)
{
	unsigned short	RetVal = 0;
	LDATA		*New;

	if ((New = (LDATA *)malloc (sizeof (LDATA))) != NULL) {
		memset (New, 0, sizeof (LDATA));
		New->Value = (void *)lpData;

		if (List != NULL) {
			while (List->Next != NULL)
				List = List->Next;

			New->Previous = List;
			New->Next = List->Next;
			if (New->Next != NULL)
				New->Next->Previous = New;
			List->Next = New;
		}

		Elements++;
		Msg.Size += sizeof((void *)lpData);
		List = New;
		RetVal = 1;
	}

	return (RetVal);
}



unsigned short MsgText_Add2(char * lpData)
{
	return (MsgText_Add3((void *)lpData, (unsigned short)(strlen (lpData) + 1)));
}



unsigned short MsgText_Add3(void * lpData, unsigned short usSize)
{
	unsigned short	RetVal = 0;
	LDATA		*New;

	if ((New = (LDATA *)malloc(sizeof (LDATA) + usSize)) != NULL) {
		memset (New, 0, sizeof (LDATA) + usSize);
		memcpy (New->Data, lpData, usSize);
		New->Value = (void *)New->Data;

		if (List != NULL) {
			while (List->Next != NULL)
				List = List->Next;

			New->Previous = List;
			New->Next = List->Next;
			if (New->Next != NULL)
				New->Next->Previous = New;
			List->Next = New;
		}

		Elements++;
		Msg.Size += usSize;
		List = New;
		RetVal = 1;
	}

	return (RetVal);
}



void MsgText_Clear (void)
{
	while (List != NULL)
		MsgText_Remove();
	Elements = 0;
}



void *MsgText_First (void)
{
	void	*RetVal = NULL;

	if (List != NULL) {
		while (List->Previous != NULL)
			List = List->Previous;
		RetVal = List->Value;
	}

	return RetVal;
}



unsigned short MsgText_Insert1(void * lpData)
{
	unsigned short RetVal = 0;
	LDATA *New;

	if ((New = (LDATA *)malloc (sizeof (LDATA))) != NULL) {
		memset (New, 0, sizeof (LDATA));
		New->Value = (void *)lpData;

		if (List != NULL) {
			New->Previous = List;
			New->Next = List->Next;
			if (New->Next != NULL)
				New->Next->Previous = New;
			List->Next = New;
		}

		Elements++;
		List = New;
		RetVal = 1;
	}

	return (RetVal);
}



unsigned short MsgText_Insert2(char * lpData)
{
	return (MsgText_Insert3(lpData, (unsigned short)(strlen (lpData) + 1)));
}



unsigned short MsgText_Insert3(void * lpData, unsigned short usSize)
{
	unsigned short RetVal = 0;
	LDATA *New;

	if ((New = (LDATA *)malloc (sizeof (LDATA) + usSize)) != NULL) {
		memset (New, 0, sizeof (LDATA) + usSize);
		memcpy (New->Data, lpData, usSize);
		New->Value = (void *)New->Data;

		if (List != NULL) {
			New->Previous = List;
			New->Next = List->Next;
			if (New->Next != NULL)
				New->Next->Previous = New;
			List->Next = New;
		}

		Elements++;
		List = New;
		RetVal = 1;
	}

	return (RetVal);
}



void * MsgText_Last(void)
{
	void * RetVal = NULL;

	if (List != NULL) {
		while (List->Next != NULL)
			List = List->Next;
		RetVal = List->Value;
	}

	return (RetVal);
}



void * MsgText_Next (void)
{
	void * RetVal = NULL;

	if (List != NULL) {
		if (List->Next != NULL) {
			List = List->Next;
			RetVal = List->Value;
		}
	}

	return (RetVal);
}



void * MsgText_Previous (void)
{
   void * RetVal = NULL;

   if (List != NULL) {
      if (List->Previous != NULL) {
         List = List->Previous;
         RetVal = List->Value;
      }
   }

   return (RetVal);
}



void MsgText_Remove(void)
{
	LDATA *Temp;

	if (List != NULL) {
		if (List->Previous != NULL)
			List->Previous->Next = List->Next;
		if (List->Next != NULL)
			List->Next->Previous = List->Previous;
		Temp = List;
		if (List->Next != NULL)
			List = List->Next;
		else if (List->Previous != NULL)
			List = List->Previous;
		else
			List = NULL;
		free (Temp);
		Elements--;
	}
}



unsigned short MsgText_Replace1(void * lpData)
{
	unsigned short RetVal = 0;
	LDATA *New;

	if (List != NULL) {
		if ((New = (LDATA *)malloc (sizeof (LDATA))) != NULL) {
			memset (New, 0, sizeof (LDATA));
			New->Value = (void *)lpData;
			New->Next = List->Next;
			New->Previous = List->Previous;

			if (New->Next != NULL)
				New->Next->Previous = New;
			if (New->Previous != NULL)
				New->Previous->Next = New;

			free (List);
			List = New;
			RetVal = 1;
		}
	}

	return (RetVal);
}



unsigned short MsgText_Replace2(char * lpData)
{
	return (MsgText_Replace3(lpData, (unsigned short)(strlen (lpData) + 1)));
}



unsigned short MsgText_Replace3(void * lpData, unsigned short usSize)
{
	unsigned short RetVal = 0;
	LDATA *New;

	if (List != NULL) {
		if ((New = (LDATA *)malloc (sizeof (LDATA) + usSize)) != NULL) {
			memset (New, 0, sizeof (LDATA) + usSize);
			memcpy (New->Data, lpData, usSize);
			New->Value = (void *)New->Data;
			New->Next = List->Next;
			New->Previous = List->Previous;

			if (New->Next != NULL)
				New->Next->Previous = New;
			if (New->Previous != NULL)
				New->Previous->Next = New;
			free (List);
			List = New;
			RetVal = 1;
		}
	}

	return (RetVal);
}



void * MsgText_Value(void)
{
	return ((List == NULL) ? NULL : List->Value);
}

