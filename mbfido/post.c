/*****************************************************************************
 *
 * $Id: post.c,v 1.26 2006/02/21 20:39:52 mbse Exp $
 * Purpose ...............: Post a message from a file.
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "post.h"


extern int		do_quiet;		/* Suppress screen output    */



int Post(char *To, int Area, char *Subj, char *File, char *Flavor)
{
    int		    i, rc = FALSE, has_tear = FALSE, has_origin = FALSE;
    char	    *aka, *temp, *sAreas;
    FILE	    *fp, *tp;
    unsigned int    crc = -1;
    time_t	    tt;
    struct tm	    *t;

    mbse_CleanSubject(Subj);

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("Post \"%s\" to \"%s\" in area %d\n", File, To, Area);
    }

    IsDoing("Posting");
    Syslog('+', "Post \"%s\" area %d to \"%s\" flavor %s", File, Area, To, Flavor);
    Syslog('+', "Subject: \"%s\"", Subj);

    if ((tp = fopen(File, "r")) == NULL) {
	WriteError("$Can't open %s", File);
	if (!do_quiet)
	    printf("Can't open \"%s\"\n", File);
	return -1;
    }

    sAreas = calloc(PATH_MAX, sizeof(char));
    snprintf(sAreas, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	free(sAreas);
	fclose(tp);
	return -1;
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
	return -1;
    }

    if (!msgs.Active) {
	WriteError("Area %s not active", msgs.Name);
	fclose(fp);
	fclose(tp);
	return -1;
    }

    /*
     * Check the proper syntax in the To parameter, in netmail areas
     * it must have a destination address, in all other areas just a
     * full name.
     */
    if (msgs.Type == NETMAIL) {
	if ((strchr(To, '@') == NULL) || (strstr(To, (char *)".n") == NULL) || (strstr(To, (char *)".z") == NULL)) {
	    WriteError("No address in \"%s\" and area is netmail", To);
	    if (!do_quiet)
		printf("No address in \"%s\" and area is netmail\n", To);
	    fclose(fp);
	    fclose(tp);
	    return -1;
	}
    } else {
	if ((strchr(To, '@')) || (strstr(To, (char *)".n")) || (strstr(To, (char *)".z"))) {
	    WriteError("Address present in \"%s\" and area is not netmail", To);
	    if (!do_quiet)
		printf("Address present in \"%s\" and area is not netmail\n", To);
	    fclose(fp);
	    fclose(tp);
	    return -1;
	}
    }

    if (!Msg_Open(msgs.Base)) {
	WriteError("Can't open %s", msgs.Base);
	fclose(fp);
	fclose(tp);
	return -1;
    }

    if (!Msg_Lock(30L)) {
	WriteError("Can't lock %s", msgs.Base);
	Msg_Close();
	fclose(fp);
	fclose(tp);
	return -1;
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
    snprintf(Msg.From, 101, CFG.sysop_name);
    snprintf(Msg.To, 101, To);

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

    snprintf(Msg.Subject, 101, "%s", Subj);
    snprintf(Msg.FromAddress, 101, "%s", aka2str(msgs.Aka));
    Msg.Written = Msg.Arrived = time(NULL) - (gmt_offset((time_t)0) * 60);
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
			snprintf(Msg.ToAddress, 101, "%s", ascfnode(parsefaddr(To), 0xff));
			break;

	case ECHOMAIL:
			Msg.Echomail = TRUE;
			break;

	case NEWS:
			Msg.News = TRUE;
			break;
    }

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "\001MSGID: %s %08x", aka2str(msgs.Aka), sequencer());
    MsgText_Add2(temp);
    Msg.MsgIdCRC = upd_crc32(temp, crc, strlen(temp));
    Msg.ReplyCRC = 0xffffffff;
    snprintf(temp, PATH_MAX, "\001PID: MBSE-FIDO %s (%s-%s)", VERSION, OsName(), OsCPU());
    MsgText_Add2(temp);
    if (msgs.Charset != FTNC_NONE) {
	snprintf(temp, PATH_MAX, "\001CHRS: %s", getftnchrs(msgs.Charset));
    } else {
	snprintf(temp, PATH_MAX, "\001CHRS: %s", getftnchrs(FTNC_LATIN_1));
    }
    MsgText_Add2(temp);
    snprintf(temp, PATH_MAX, "\001TZUTC: %s", gmtoffset(tt));
    MsgText_Add2(temp);

    while ((Fgets(temp, PATH_MAX, tp)) != NULL) {
	if (strncmp(temp, "--- ", 4) == 0)
	    has_tear = TRUE;
	if (strncmp(temp, " * Origin: ", 11) == 0)
	    has_origin = TRUE;
    }
    rewind(tp);
    Syslog('m', "has tearline=%s, has origin=%s", has_tear?"True":"False", has_origin?"True":"False");

    /*
     * Add the file as text
     */
    Msg_Write(tp);
    fclose(tp);

    /*
     * Finish the message
     */
    if ((! has_tear) && (! has_origin)) {
	MsgText_Add2((char *)"");
	MsgText_Add2(TearLine());
    }

    if (! has_origin) {
	aka = calloc(40, sizeof(char));

	if (msgs.Aka.point)
	    snprintf(aka, 40, "(%d:%d/%d.%d)", msgs.Aka.zone, msgs.Aka.net, msgs.Aka.node, msgs.Aka.point);
	else
	    snprintf(aka, 40, "(%d:%d/%d)", msgs.Aka.zone, msgs.Aka.net, msgs.Aka.node);

	if (strlen(msgs.Origin))
	    snprintf(temp, 81, " * Origin: %s %s", msgs.Origin, aka);
	else
	    snprintf(temp, 81, " * Origin: %s %s", CFG.origin, aka);

	MsgText_Add2(temp);
	free(aka);
    }

    Msg_AddMsg();
    Msg_UnLock();
    Syslog('+', "Posted message %ld", Msg.Id);

    if (msgs.Type != LOCALMAIL) {
	snprintf(temp, PATH_MAX, "%s/tmp/%smail.jam", getenv("MBSE_ROOT"), (msgs.Type == ECHOMAIL) ? "echo" : "net");
	if ((fp = fopen(temp, "a")) != NULL) {
	    fprintf(fp, "%s %u\n", msgs.Base, Msg.Id);
	    fclose(fp);
	}
	CreateSema((char *)"mailout");
    }

    free(temp);
    Msg_Close();

    return 0;
}


