/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Post a message from a file.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "post.h"


extern int		do_quiet;		/* Suppress screen output    */



void Post(char *To, long Area, char *Subj, char *File, char *Flavor)
{
	int		i, rc = FALSE;
	char		*aka, *temp, *sAreas;
	FILE		*fp, *tp;
	unsigned long	crc = -1;
	time_t		tt;
	struct tm	*t;


	if (!do_quiet) {
		colour(3, 0);
		printf("Post \"%s\" to \"%s\" in area %ld\n", File, To, Area);
	}

	IsDoing("Posting");
	Syslog('+', "Post \"%s\" area %ld to \"%s\" flavor %s", File, Area, To, Flavor);
	Syslog('+', "Subject: \"%s\"", Subj);

	if ((tp = fopen(File, "r")) == NULL) {
		WriteError("$Can't open %s", File);
		return;
	}

	sAreas = calloc(128, sizeof(char));
	sprintf(sAreas, "%s//etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(sAreas, "r")) == NULL) {
		WriteError("$Can't open %s", sAreas);
		free(sAreas);
		fclose(tp);
		return;
	}

	fread(&msgshdr, sizeof(msgshdr), 1, fp);
	if (fseek(fp, (msgshdr.recsize + msgshdr.syssize) * (Area - 1), SEEK_CUR) == 0) {
		if (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
			rc = TRUE;
		} else {
			WriteError("$Can't read area %ld", Area);
		}
	} else {
		WriteError("$Can't seek area %ld", Area);
	}

	free(sAreas);
	if (rc == FALSE) {
		fclose(fp);
		fclose(tp);
		return;
	}

	if (!msgs.Active) {
		WriteError("Area %s not active", msgs.Name);
		fclose(fp);
		fclose(tp);
		return;
	}

	if (!Msg_Open(msgs.Base)) {
		WriteError("Can't open %s", msgs.Base);
		fclose(fp);
		fclose(tp);
		return;
	}

	if (!Msg_Lock(30L)) {
		WriteError("Can't lock %s", msgs.Base);
		Msg_Close();
		fclose(fp);
		fclose(tp);
		return;
	}

	tt = time(NULL);
	t = localtime(&tt);
	Diw = t->tm_wday;
	Miy = t->tm_mon;
	memset(&Msg, 0, sizeof(Msg));
	Msg_New();

	/*
	 * Update statistic counter for message area
	 */
	fseek(fp, - msgshdr.recsize, SEEK_CUR);
	msgs.Posted.total++;
	msgs.Posted.tweek++;
	msgs.Posted.tdow[Diw]++;
	msgs.Posted.month[Miy]++;
	fwrite(&msgs, msgshdr.recsize, 1, fp);
	fclose(fp);

	/*
	 * Start writing the message
	 */
	sprintf(Msg.From, CFG.sysop_name);
	sprintf(Msg.To, To);

	/*
	 * If netmail, clean the To field.
	 */
	if ((msgs.Type == NETMAIL) && strchr(To, '@')) {
		for (i = 0; i < strlen(Msg.To); i++) {
			if (Msg.To[i] == '_')
				Msg.To[i] = ' ';
			if (Msg.To[i] == '@') {
				Msg.To[i] = '\0';
				break;
			}
		}
	}

	sprintf(Msg.Subject, "%s", Subj);
	sprintf(Msg.FromAddress, "%s", aka2str(msgs.Aka));
	Msg.Written = time(NULL);
	Msg.Arrived = time(NULL);
	Msg.Local = TRUE;

	if (strchr(Flavor, 'c'))
		Msg.Crash = TRUE;
	if (strchr(Flavor, 'p'))
		Msg.Private = TRUE;
	if (strchr(Flavor, 'h'))
		Msg.Hold = TRUE;

	switch (msgs.Type) {
		case LOCALMAIL:	
				Msg.Localmail = TRUE;
				break;

		case NETMAIL:
				Msg.Netmail = TRUE;
				sprintf(Msg.ToAddress, "%s", ascfnode(parsefaddr(To), 0xff));
				break;

		case ECHOMAIL:
				Msg.Echomail = TRUE;
				break;

		case NEWS:
				Msg.News = TRUE;
				break;
	}

	temp = calloc(128, sizeof(char));
	sprintf(temp, "\001MSGID: %s %08lx", aka2str(msgs.Aka), sequencer());
	MsgText_Add2(temp);
	Msg.MsgIdCRC = upd_crc32(temp, crc, strlen(temp));
	Msg.ReplyCRC = 0xffffffff;
	sprintf(temp, "\001PID: MBSE-FIDO %s", VERSION);
	MsgText_Add2(temp);
//	sprintf(temp, "\001CHRS: %s", getchrs(msgs.Ftncode));
//	MsgText_Add2(temp);
	sprintf(temp, "\001TZUTC: %s", gmtoffset(tt));
	MsgText_Add2(temp);

	/*
	 * Add the file as text
	 */
	Msg_Write(tp);
	fclose(tp);

	/*
	 * Finish the message
	 */
	aka = calloc(40, sizeof(char));
	MsgText_Add2((char *)"");
	MsgText_Add2(TearLine());

	if (msgs.Aka.point)
		sprintf(aka, "(%d:%d/%d.%d)", msgs.Aka.zone, msgs.Aka.net, msgs.Aka.node, msgs.Aka.point);
	else
		sprintf(aka, "(%d:%d/%d)", msgs.Aka.zone, msgs.Aka.net, msgs.Aka.node);

	if (strlen(msgs.Origin))
		sprintf(temp, " * Origin: %s %s", msgs.Origin, aka);
	else
		sprintf(temp, " * Origin: %s %s", CFG.origin, aka);

	MsgText_Add2(temp);
	free(aka);

	Msg_AddMsg();
	Msg_UnLock();
	Syslog('+', "Posted message %ld", Msg.Id);

	sprintf(temp, "%s/tmp/%smail.jam", getenv("MBSE_ROOT"), (msgs.Type == ECHOMAIL) ? "echo" : "net");
	if ((fp = fopen(temp, "a")) != NULL) {
		fprintf(fp, "%s %lu\n", msgs.Base, Msg.Id);
		fclose(fp);
	}
	free(temp);
	Msg_Close();
	CreateSema((char *)"mailout");

	return;
}


