/*****************************************************************************
 *
 * $Id: scan.c,v 1.47 2007/09/05 18:40:02 mbse Exp $
 * Purpose ...............: Scan for outgoing mail.
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
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/mbsedb.h"
#include "addpkt.h"
#include "tracker.h"
#include "ftn2rfc.h"
#include "rfc2ftn.h"
#include "rollover.h"
#include "postemail.h"
#include "scan.h"


extern	int	do_quiet;
extern	int	net_out;
extern	int	net_bad;
extern	int	echo_in;
extern	int	email_out;
extern	int	echo_out;
extern	int	most_debug;
extern	int	do_flush;
int		scanned;

#define	MAXSEEN 70


/*
 * Internal prototypes
 */
void ScanFull(void);
void ScanOne(char *, unsigned int);
void ExportEcho(sysconnect, unsigned int, fa_list **);
void ExportNews( unsigned int, fa_list **);
void ExportNet(unsigned int, int);
void ExportEmail(unsigned int);



/*
 *  Scan for outgoing mail. If using the files $MBSE_ROOT/tmp/echomail.jam
 *  or netmail.jam not all mail is scanned a full mailscan will be 
 *  performed.
 */
void ScanMail(int DoAll)
{
    int		    DoFull = FALSE, i = 0;
    unsigned int    msg;
    char	    *Fname = NULL, *temp, *msgstr, *path;
    FILE	    *fp;

    if (DoAll) {
	DoFull = TRUE;
    } else {
	scanned = 0;
	Fname = calloc(PATH_MAX, sizeof(char));
	temp  = calloc(PATH_MAX, sizeof(char));

	snprintf(Fname, PATH_MAX, "%s/tmp/echomail.jam", getenv("MBSE_ROOT"));
	if ((fp = fopen(Fname, "r")) != NULL) {
	    while ((fgets(temp, PATH_MAX - 1, fp)) != NULL) {
		path = strtok(temp, " \n\0");
		msgstr = strtok(NULL, "\n\0");
		if (path && msgstr) {
		    msg = atol(msgstr);
		    Syslog('+', "Export message %lu from %s", msg, path);
		    ScanOne(path, msg);
		    i++;
		} else {
		    Syslog('!', "Ignored garbage line in %s", Fname);
		}
		Nopper();
	    }
	    fclose(fp);
	    unlink(Fname); 
	}

	snprintf(Fname, PATH_MAX, "%s/tmp/netmail.jam", getenv("MBSE_ROOT"));
	if ((fp = fopen(Fname, "r")) != NULL) {
	    while ((fgets(temp, PATH_MAX - 1, fp)) != NULL) {
		path = strtok(temp, " \n\0");
		msgstr = strtok(NULL, "\n\0");
		if (path && msgstr) {
		    msg = atol(msgstr);
		    Syslog('+', "Export message %lu from %s", msg, path);
		    ScanOne(path, msg);
		    i++;
		} else {
		    Syslog('!', "Ignored garbage line in %s", Fname);
		}
		Nopper();
	    }
	    fclose(fp);
	    unlink(Fname);
	}

	if ((i != scanned) || (i == 0)) {
	    Syslog('+', "Not all messages exported, forcing full mail scan to fix this");
	    DoFull = TRUE;
	}
	free(Fname);
	free(temp);
    }

    if (DoFull)
	ScanFull();

    if (echo_out || net_out)
	do_flush = TRUE;
    RemoveSema((char *)"mailout");
}



void ScanFull()
{
    char	    *sAreas, sbe[128];
    FILE	    *pAreas;
    int		    arearec = 0, sysstart, nextstart;
    unsigned int    Total, Number;
    int		    i;
    sysconnect	    Link;
    fa_list	    *sbl = NULL;

    Syslog('+', "Full mailscan");
    IsDoing("Scanning mail");

    if (!do_quiet) {
	mbse_colour(LIGHTBLUE, BLACK);
	printf("Scanning mail\n");
	mbse_colour(CYAN, BLACK);
	fflush(stdout);
    }

    sAreas = calloc(PATH_MAX, sizeof(char));
    snprintf(sAreas, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen(sAreas, "r")) != NULL) {
	fread(&usrconfighdr, sizeof(usrconfighdr), 1, pAreas);

	while (fread(&usrconfig, usrconfighdr.recsize, 1, pAreas) == 1) {
	    if (usrconfig.Email && strlen(usrconfig.Name)) {

		Nopper();
		if (!do_quiet) {
		    mbse_colour(CYAN, BLACK);
		    printf("\r%8s %-40s", usrconfig.Name, usrconfig.sUserName);
		    mbse_colour(LIGHTMAGENTA, BLACK);
		    fflush(stdout);
		}

		snprintf(sAreas, PATH_MAX, "%s/%s/mailbox", CFG.bbs_usersdir, usrconfig.Name);
		if (Msg_Open(sAreas)) {
		    if ((Total = Msg_Number()) != 0L) {
			Number = Msg_Lowest();

			do {
			    if (CFG.slow_util && do_quiet)
				msleep(1);

			    if (((Number % 10) == 0) && (!do_quiet)) {
				printf("%6u\b\b\b\b\b\b", Number);
				fflush(stdout);
			    }

			    Msg_ReadHeader(Number);
			    if (Msg.Local) {
				if (Msg_Lock(15L)) {
				    Syslog('m', "Export %lu email from %s", Number, usrconfig.Name);
				    ExportEmail(Number);
				    Msg.Local = FALSE;
				    Msg.Arrived = time(NULL);
				    Msg_WriteHeader(Number);
				    Msg_UnLock();
				}
			    }

			} while (Msg_Next(&Number) == TRUE);
		    }
		    Msg_Close();
		    if (!do_quiet) {
			printf("      \b\b\b\b\b\b");
			fflush(stdout);
		    }
		}
	    }
	}
	fclose(pAreas);
    }

    snprintf(sAreas, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen(sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	free(sAreas);
	return;
    }
    free(sAreas);
    fread(&msgshdr, sizeof(msgshdr), 1, pAreas);

    while (fread(&msgs, msgshdr.recsize, 1, pAreas) == 1) {
	sysstart = ftell(pAreas);
	fseek(pAreas, msgshdr.syssize, SEEK_CUR);
	nextstart = ftell(pAreas);
	arearec++;

	if ((msgs.Active) && (msgs.Type == ECHOMAIL || msgs.Type == NETMAIL || msgs.Type == NEWS)) {

	    Nopper();
	    if (!do_quiet) {
		mbse_colour(CYAN, BLACK);
		printf("\r%5d .. %-40s", arearec, msgs.Name);
		mbse_colour(LIGHTMAGENTA, BLACK);
		fflush(stdout);
	    }

	    if (Msg_Open(msgs.Base)) {
		if ((Total = Msg_Number()) != 0L) {
		    Number = Msg_Lowest();

		    do {
			if (CFG.slow_util && do_quiet)
			    msleep(1);

			if (((Number % 10) == 0) && (!do_quiet)) {
			    printf("%6u\b\b\b\b\b\b", Number);
			    fflush(stdout);
			}

			Msg_ReadHeader(Number);
			if (Msg.Local) {
			    if (Msg_Lock(15L)) {
				Syslog('m', "Export %lu from area %ld", Number, arearec);

			        /*
			         * Setup SEEN-BY lines
			         */
			        if ((msgs.Type == ECHOMAIL) || (msgs.Type == NEWS)) {
				    echo_in++;
				    fill_list(&sbl, aka2str(msgs.Aka), NULL);
				    for (i = 0; i < 40; i++) {
					if (CFG.akavalid[i] && (msgs.Aka.zone == CFG.aka[i].zone) &&
						(CFG.aka[i].point == 0) && !((msgs.Aka.net == CFG.aka[i].net) &&
						(msgs.Aka.node == CFG.aka[i].node))) {
					    snprintf(sbe, 128, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
					    fill_list(&sbl, sbe, NULL);
					}
				    }
				    fseek(pAreas, sysstart, SEEK_SET);
				    for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
					fread(&Link, sizeof(sysconnect), 1, pAreas);
				        if ((Link.aka.zone) && (Link.sendto) && (!Link.pause)) {
					    fill_list(&sbl, aka2str(Link.aka), NULL);
					}
				    }
				    uniq_list(&sbl);
				    sort_list(&sbl);

				    fseek(pAreas, sysstart, SEEK_SET);
				    for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
				        fread(&Link, sizeof(sysconnect), 1, pAreas);
					if (Link.aka.zone)
					    ExportEcho(Link, Number, &sbl);
				    }
#ifdef USE_NEWSGATE
				    if (strlen(msgs.Newsgroup))
#else
				    if (strlen(msgs.Newsgroup) && (msgs.Type == NEWS))
#endif
					ExportNews(Number, &sbl);

				    tidy_falist(&sbl);
				}
			        if (msgs.Type == NETMAIL) {
				    ExportNet(Number, FALSE);
				    most_debug = FALSE;
				}
			        Msg.Local = FALSE;
			        Msg.Arrived = time(NULL);
			        Msg_WriteHeader(Number);
			        Msg_UnLock();
			    }
			}

		    } while (Msg_Next(&Number) == TRUE);
		}

		Msg_Close();

		if (!do_quiet) {
		    printf("      \b\b\b\b\b\b");
		    fflush(stdout);
		}
	    }

	    /*
	     * Make sure to start at the next area.
	     */
	    fseek(pAreas, nextstart, SEEK_SET);
	}
    }

    fclose(pAreas);

    if (!do_quiet) {
	printf("\r                                                        \r");
	fflush(stdout);
    }
}



void ScanOne(char *path, unsigned int MsgNum)
{
    char	    *sAreas, sbe[128];
    FILE	    *pAreas;
    int		    sysstart;
    unsigned int    Total, Area = 0;
    int		    i;
    sysconnect	    Link;
    fa_list	    *sbl = NULL;

    IsDoing("Scanning mail");

    if (!do_quiet) {
	mbse_colour(LIGHTBLUE, BLACK);
	printf("Scanning mail\n");
	mbse_colour(CYAN, BLACK);
	fflush(stdout);
    }

    if (strncmp(CFG.bbs_usersdir, path, strlen(CFG.bbs_usersdir)) == 0) {
	if (Msg_Open(path)) {
	    if (((Total = Msg_Number()) != 0L) && (Msg_ReadHeader(MsgNum)) && Msg.Local) {
		if (Msg_Lock(15L)) {
		    scanned++;
		    ExportEmail(MsgNum);
		    Msg.Local = FALSE;
		    Msg.Arrived = time(NULL);
		    Msg_WriteHeader(MsgNum);
		    Msg_UnLock();
		}
	    }
	    Msg_Close();
	} else {
	    WriteError("Can't open %s", path);
	}
	return;
    }

    sAreas = calloc(PATH_MAX, sizeof(char));
    snprintf(sAreas, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen(sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	free(sAreas);
	return;
    }
    free(sAreas);
    fread(&msgshdr, sizeof(msgshdr), 1, pAreas);

    /*
     * Seek the path we want
     */
    while (TRUE) {
	if (fread(&msgs, msgshdr.recsize, 1, pAreas) != 1) {
	    fclose(pAreas);
	    Syslog('m', "ScanOne() reached end of areas");
	    return;
	}
	Area++;
	sysstart = ftell(pAreas);
	fseek(pAreas, msgshdr.syssize, SEEK_CUR);
	if (strcmp(msgs.Base, path) == 0)
	    break;
    }

    if ((msgs.Active) && (msgs.Type == ECHOMAIL || msgs.Type == NETMAIL || msgs.Type == NEWS)) {
	if (!do_quiet) {
	    mbse_colour(CYAN, BLACK);
	    printf("\r%5d .. %-40s", Area, msgs.Name);
	    mbse_colour(LIGHTMAGENTA, BLACK);
	    fflush(stdout);
	}

	if (Msg_Open(msgs.Base)) {
	    if ((Total = Msg_Number()) != 0L) {
		if (Msg_ReadHeader(MsgNum)) {
		    if (Msg.Local) {
			if (Msg_Lock(15L)) {
			    scanned++;
			    /*
			     * Setup SEEN-BY lines
			     */
			    if (msgs.Type == ECHOMAIL || msgs.Type == NEWS) {
				echo_in++;
				fill_list(&sbl, aka2str(msgs.Aka), NULL);
				for (i = 0; i < 40; i++) {
				    if (CFG.akavalid[i] && (msgs.Aka.zone == CFG.aka[i].zone) && (CFG.aka[i].point == 0) &&
					    !((msgs.Aka.net == CFG.aka[i].net) && (msgs.Aka.node == CFG.aka[i].node))) {
					snprintf(sbe, 128, "%u/%u", CFG.aka[i].net, CFG.aka[i].node);
					fill_list(&sbl, sbe, NULL);
				    }
				}
				fseek(pAreas, sysstart, SEEK_SET);
				for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
				    fread(&Link, sizeof(sysconnect), 1, pAreas);
				    if ((Link.aka.zone) && (Link.sendto) && (!Link.pause)) {
					fill_list(&sbl, aka2str(Link.aka), NULL);
				    }
				}
				uniq_list(&sbl);
				sort_list(&sbl);

				fseek(pAreas, sysstart, SEEK_SET);
				for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++) {
				    fread(&Link, sizeof(sysconnect), 1, pAreas);
				    if (Link.aka.zone) {
					ExportEcho(Link, MsgNum, &sbl);
				    }
				}
#ifdef USE_NEWSGATE
				if (strlen(msgs.Newsgroup))
#else
				if (strlen(msgs.Newsgroup) && (msgs.Type == NEWS))
#endif
				     ExportNews(MsgNum, &sbl);

				tidy_falist(&sbl);
			    }
			    if (msgs.Type == NETMAIL) {
				ExportNet(MsgNum, FALSE);
				most_debug = FALSE;
			    }

			    Msg.Local = FALSE;
			    Msg.Arrived = time(NULL);
			    Msg_WriteHeader(MsgNum);
			    Msg_UnLock();
			}
		    }
		}
	    }
	    Msg_Close();
	}
    } else {
	WriteError("Config error: area %d not active or not Echo/Netmail area", Area);
    }

    fclose(pAreas);

    if (!do_quiet) {
	printf("\r                                                        \r");
	fflush(stdout);
    }
}



int RescanOne(faddr *L, char *marea, unsigned int Num)
// Return:    0 -> Ok
//            1 -> Unknown area
//            2 -> Node cant rescan this area
{
    unsigned int    Total, MsgNum, Area = 0;
    fa_list         *sbl = NULL;
    fidoaddr        *l;
    int             First, Found;
    unsigned int    rescanned;
    sysconnect      Link;

    IsDoing("ReScan mail");

    if (!do_quiet) {
        mbse_colour(LIGHTBLUE, BLACK);
        printf("ReScan mail\n");
        mbse_colour(CYAN, BLACK);
        fflush(stdout);
    }

    l = faddr2fido( L );
    rescanned = 0L;
    if (!SearchMsgs(marea)) {
        Mgrlog("ReScan of unknown echo area %s", marea);
        return 1;
    }

    First = TRUE;
    Found = FALSE;
    while (GetMsgSystem(&Link, First)) {
        First = FALSE;
        if ((l->zone == Link.aka.zone) && (l->net  == Link.aka.net) && (l->node == Link.aka.node) && (l->point == Link.aka.point)) {
            Found = TRUE;
            break;
        }
    }
    if (!Found) {
        Mgrlog("Node %s can't Rescan area %s", L, marea);
        return 2;
    }

    if ((msgs.Active) && ((msgs.Type == ECHOMAIL) || (msgs.Type == NEWS) || (msgs.Type == LIST))) {
        if (!do_quiet) {
            mbse_colour(CYAN, BLACK);
            printf("\r%5d .. %-40s", Area, msgs.Name);
            mbse_colour(LIGHTMAGENTA, BLACK);
	    fflush(stdout);
	}

        if (Msg_Open(msgs.Base)) {
            Total = Msg_Number();
            MsgNum = 1;
            if (Num!=0 && Num<Total)
		MsgNum = (Total + 1 - Num);

            while (MsgNum<=Total){
                if (Msg_ReadHeader(MsgNum)) {
                    if (Msg_Lock(15L)) {
                        fill_list(&sbl, aka2str(msgs.Aka), NULL);
                        fill_list(&sbl, aka2str(Link.aka), NULL);
                        sort_list(&sbl);
                        ExportEcho(Link, MsgNum, &sbl);
                        tidy_falist(&sbl);
                        Msg_UnLock();
                        rescanned++;
                    }
                }
                MsgNum++;
            }
            Msg_Close();
        }
    }

    if (!do_quiet) {
        printf("\r                                                        \r");
        fflush(stdout);
    }

    Mgrlog("Rescan OK. %ul messages rescanned", rescanned);
    return 0;
}



/*
 *  Export message to downlink. The messagebase is locked.
 */
void ExportEcho(sysconnect L, unsigned int MsgNum, fa_list **sbl)
{
    int	    rc, seenlen, oldnet, flags = 0, kludges = TRUE;
    char    *p, sbe[128], ext[4];
    fa_list *tmpl;
    FILE    *qp;
    faddr   *from, *dest;

    if ((!L.sendto) || L.pause || L.cutoff)
	return;

    if (!SearchNode(L.aka)) {
	WriteError("Can't send to %s, noderecord not found", aka2str(L.aka));
	return;
    }

    /*
     * Add statistics count
     */
    StatAdd(&nodes.MailSent, 1L);
    UpdateNode();
    SearchNode(L.aka);
    
    memset(&ext, 0, sizeof(ext));
    if (nodes.PackNetmail)
	snprintf(ext, 4, (char *)"qqq");
    else if (nodes.Crash)
	snprintf(ext, 4, (char *)"ccc");
    else if (nodes.Hold)
	snprintf(ext, 4, (char *)"hhh");
    else
	snprintf(ext, 4, (char *)"nnn");

    if ((qp = OpenPkt(msgs.Aka, L.aka, (char *)ext)) == NULL)
	return;

    flags |= (Msg.Private)		? M_PVT : 0;
    from = fido2faddr(msgs.Aka);
    dest = fido2faddr(L.aka);
    rc = AddMsgHdr(qp, from, dest, flags, 0, Msg.Written, Msg.To, Msg.From, Msg.Subject);
    tidy_faddr(from);
    tidy_faddr(dest);

    if (rc) {
	Syslog('+', "Cannot export message");
	fclose(qp);
	return;
    }

    fprintf(qp, "AREA:%s\r", msgs.Tag);

    if (Msg_Read(MsgNum, 79)) {
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if (kludges && (p[0] != '\001')) {
		    /*
		     * At the end of the kludges, add the TID kludge.
		     */
		    kludges = FALSE;
		    fprintf(qp, "\001TID: MBSE-FIDO %s (%s-%s)\r", VERSION, OsName(), OsCPU());
		}
		fprintf(qp, "%s", p);
		if (strncmp(p, " * Origin:", 10) == 0)
		    break;

		/*
		 * Only append CR if not the last line
		 */
		fprintf(qp, "\r");
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }

    seenlen = MAXSEEN + 1;
    /*
     * Ensure that it will not match the first entry.
     */
    oldnet = (*sbl)->addr->net - 1;
    for (tmpl = *sbl; tmpl; tmpl = tmpl->next) {
	if (tmpl->addr->net == oldnet)
	    snprintf(sbe, 128, " %u", tmpl->addr->node);
	else
	    snprintf(sbe, 128, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	oldnet = tmpl->addr->net;
	seenlen += strlen(sbe);
	if (seenlen > MAXSEEN) {
	    seenlen = 0;
	    fprintf(qp, "\rSEEN-BY:");
	    snprintf(sbe, 128, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	    seenlen = strlen(sbe);
	}
	fprintf(qp, "%s", sbe);
    }
    fprintf(qp, "\r\001PATH: %u/%u\r", msgs.Aka.net, msgs.Aka.node);
    putc(0, qp);
    fclose(qp);

    echo_out++;
}



/*
 *  Export message to the newsserver. The messagebase is locked.
 */
void ExportNews(unsigned int MsgNum, fa_list **sbl)
{
    char    *p;
    int     i, seenlen, oldnet, flags = 0;
    char    sbe[128];
    fa_list *tmpl;
    FILE    *qp;
    faddr   *from, *dest;
    int     kludges = TRUE;

    qp = tmpfile();

    Syslog('m', "Msg.From %s", Msg.From);
    Syslog('m', "Msg.FromAddress %s", Msg.FromAddress);
    Syslog('m', "Msg.To %s", Msg.To);
    Syslog('m', "Msg.ToAddress %s", Msg.ToAddress);

    flags |= (Msg.Private)          ? M_PVT : 0;
    from = fido2faddr(msgs.Aka);

    /*
     * Add name with echo to news gate.
     */
    p = calloc(256, sizeof(char));
    strcpy(p, Msg.From);
    for (i = 0; i < strlen(p); i++) {
	if (p[i] == '@') {
	    p[i] = '\0';
	    break;
	}
    }
    from->name = xstrcpy(p);
    free(p);
    Syslog('m', "from %s", ascinode(from, 0xff));
    dest = NULL;

    fprintf(qp, "AREA:%s\n", msgs.Tag);

    if (Msg_Read(MsgNum, 79)) {
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if (kludges) {
		    if (p[0] != '\001') {
			/*
			 * After the first kludges, send RFC headers
			 */
			kludges = FALSE;
			fprintf(qp, "\001TID: MBSE-FIDO %s (%s-%s)\n", VERSION, OsName(), OsCPU());
			fprintf(qp, "Subject: %s\n", Msg.Subject);
			fprintf(qp, "\n");
			fprintf(qp, "%s\n", p);
		    } else {
			fprintf(qp, "%s\n", p+1);
		    }
		} else {
		    fprintf(qp, "%s", p);
		    if (strncmp(p, " * Origin:", 10) == 0)
			break;

		    /*
		     * Only append NL if not the last line
		     */
		    fprintf(qp, "\n");
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }

    seenlen = MAXSEEN + 1;
    /*
     * Ensure that it will not match the first entry.
     */
    oldnet = (*sbl)->addr->net - 1;
    for (tmpl = *sbl; tmpl; tmpl = tmpl->next) {
	if (tmpl->addr->net == oldnet)
	    snprintf(sbe, 128, " %u", tmpl->addr->node);
	else
	    snprintf(sbe, 128, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	oldnet = tmpl->addr->net;
	seenlen += strlen(sbe);
	if (seenlen > MAXSEEN) {
	    seenlen = 0;
	    fprintf(qp, "\nSEEN-BY:");
	    snprintf(sbe, 128, " %u/%u", tmpl->addr->net, tmpl->addr->node);
	    seenlen = strlen(sbe);
	}
	fprintf(qp, "%s", sbe);
    }
    fprintf(qp, "\n\001PATH: %u/%u\n", msgs.Aka.net, msgs.Aka.node);

    rewind(qp);
    most_debug = TRUE;
    ftn2rfc(from, dest, NULL, NULL, Msg.Written + (gmt_offset((time_t)0) * 60), flags, qp);
    most_debug = FALSE;
    tidy_faddr(from);
    fclose(qp);
}



/*
 *  Export Netmail message, the messagebase is locked.
 */
void ExportNet(unsigned int MsgNum, int UUCPgate)
{
    char	    *p, *q, ext[4], fromname[37], flavor, MailFrom[128], MailTo[128];
    int		    i, rc, flags = 0, first, is_fmpt = FALSE, is_topt = FALSE, is_intl = FALSE, mypoint = FALSE, empty = TRUE;
    FILE	    *qp, *fp, *fl;
    fidoaddr	    Dest, Route, *dest;
    time_t	    now;
    struct tm	    *tm;
    faddr	    *from, *too, *ta;
    unsigned short  point;

    Syslog('m', "Export netmail to %s of %s (%s) %s mode", Msg.To, Msg.ToAddress, 
		(Msg.Crash || Msg.Direct || Msg.FileAttach) ? "Direct" : "Routed", UUCPgate ? "UUCP" : "Netmail");

    /*
     *  Analyze this message if it contains INTL, FMPT and TOPT kludges 
     *  and check if we need them. If they are missing they are inserted.
     *  GoldED doesn't insert them but MBSE does.
     */
    if (Msg_Read(MsgNum, 79)) {
        if ((p = (char *)MsgText_First()) != NULL) {
            do {
                if (strncmp(p, "\001FMPT", 5) == 0)
                    is_fmpt = TRUE;
                if (strncmp(p, "\001TOPT", 5) == 0)
                    is_topt = TRUE;
                if (strncmp(p, "\001INTL", 5) == 0)
                    is_intl = TRUE;
		if (strncmp(p, "--- ", 4) == 0)
                    break;
		if ((p[0] != '\001') && (p[0] != '\0')) {
		    empty = FALSE;
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
        }
    }
    if (empty)
    	Syslog('m', " netmail is empty");
    
    /*
     *  Check if this a netmail to our own local UUCP gate.
     */
    ta = parsefnode(Msg.ToAddress);
    if ((!strcmp(Msg.To, "UUCP")) && (is_local(ta))) {
	tidy_faddr(ta);
	most_debug = TRUE;
	if ((fp = tmpfile()) == NULL) {
	    WriteError("$Can't open tempfile");
	    return;
	}
	from = fido2faddr(msgs.Aka);
	strncpy(fromname, Msg.From, 36);
	for (i = 0; i < strlen(fromname); i++)
	    if (fromname[i] == ' ')
		    fromname[i] = '_';
	snprintf(MailFrom, 128, "%s@%s", fromname, ascinode(from, 0x2f));

        if (Msg_Read(MsgNum, 79)) {
            if ((p = (char *)MsgText_First()) != NULL) {
                do {
		    if (strncmp(p, "To: ", 4) == 0) {
			Syslog('m', "%s", MBSE_SS(p));
			if ((strchr(p, '<') != NULL) && (strchr(p, '>') != NULL)) {
			    q = strtok(p, "<");
			    q = strtok(NULL, ">");
			} else {
			    q = strtok(p, " ");
			    q = strtok(NULL, " \n\r\t");
			}
			snprintf(MailTo, 128, "%s", q);
			Syslog('m', "Final MailTo \"%s\"", MailTo);
			break;
						
		    }
		} while ((p = (char *)MsgText_Next()) != NULL);
	    }
	}
	Syslog('+', "UUCP gate From: %s at %s To: %s", Msg.From, Msg.FromAddress, MailTo);
	
	/*
	 *  First send all headers
	 */
	fprintf(fp, "Date: %s\n", rfcdate(Msg.Written + (gmt_offset((time_t)0) * 60)));
	fprintf(fp, "From: %s@%s\n", fromname, ascinode(from, 0x2f));
	tidy_faddr(from);
	fprintf(fp, "Subject: %s\n", Msg.Subject);
	fprintf(fp, "MIME-Version: 1.0\n");
	fprintf(fp, "Content-Type: text/plain\n");
	fprintf(fp, "Content-Transfer-Encoding: 8bit\n");
	fprintf(fp, "X-Mailreader: MBSE BBS %s\r\n", VERSION);

	if (msgs.Aka.point && !is_fmpt)
	    fprintf(fp, "X-FTN-FMPT: %d\n", msgs.Aka.point);

        if (Msg_Read(MsgNum, 79)) {
            if ((p = (char *)MsgText_First()) != NULL) {
                do {
		    if (p[0] == '\001') {
			if (strncmp(p, "\001INTL", 5) == 0)
			    fprintf(fp, "X-FTN-INTL: %s\n", p+6);
			else
			    fprintf(fp, "X-FTN-%s\n", p+1);
		    }
                } while ((p = (char *)MsgText_Next()) != NULL);
            }
        }

	if (Msg_Read(MsgNum, 79)) {
            if ((p = (char *)MsgText_First()) != NULL) {
                do {
                    if (p[0] != '\001') {
                        fprintf(fp, "%s\n", p);
                    }
                } while ((p = (char *)MsgText_Next()) != NULL);
            }
	}

	postemail(fp, MailFrom, MailTo);
	fclose(fp);
	return;
    }
    tidy_faddr(ta);

    if (UUCPgate) {
	memcpy(&Dest, &CFG.UUCPgate, sizeof(fidoaddr));
	memset(&msgs, 0, sizeof(msgs));
	memcpy(&msgs.Aka, &CFG.EmailFidoAka, sizeof(fidoaddr));
    } else {
	ta = parsefnode(Msg.ToAddress);
	dest = faddr2fido(ta);
	tidy_faddr(ta);
	memcpy(&Dest, dest, sizeof(fidoaddr));
    }
    Dest.domain[0] = '\0';

    if (!(Msg.Crash || Msg.Immediate || Msg.Direct || Msg.FileAttach)) {
	if (!TrackMail(Dest, &Route)) {
	    Syslog('!', "No route to %s, message orphaned", Msg.ToAddress);
	    Msg.Orphan = TRUE;
	    net_bad++;
	    return;
	}
    }

    Msg.Sent = TRUE;
    if (Msg.KillSent)
	Msg.Deleted = TRUE;

    if (Msg.Crash || Msg.Direct || Msg.FileAttach || Msg.Immediate) {
	memset(&ext, 0, sizeof(ext));
	if (Msg.Immediate)
	    snprintf(ext, 4, (char *)"ddd");
	else if (Msg.Crash)
	    snprintf(ext, 4, (char *)"ccc");
	else
	    snprintf(ext, 4, (char *)"nnn");

	/*
	 * If the destination is a point, check if it is our point
	 */
	for (i = 0; i < 40; i++) {
	    if (CFG.akavalid[i] &&
			(CFG.aka[i].zone == Dest.zone) && (CFG.aka[i].net == Dest.net) && (CFG.aka[i].node == Dest.node)) {
		if (Dest.point && !CFG.aka[i].point) {
		    mypoint = TRUE;
		}
	    }
	}
	if (mypoint) {
	    if ((qp = OpenPkt(msgs.Aka, Dest, (char *)ext)) == NULL) {
		 net_bad++;
		 return;
	    }
	} else {
	    point = Dest.point;
	    Dest.point = 0;
	    if (point)
		Syslog('+', "Routing via Boss %s", aka2str(Dest));
	    if ((qp = OpenPkt(msgs.Aka, Dest, (char *)ext)) == NULL) {
		net_bad++;
		return;
	    }
	    Dest.point = point;
	}

    } else {
	Syslog('m', "Route via %s", aka2str(Route));
	if (!SearchNode(Route)) {
	    WriteError("Routing node %s not in setup, aborting", aka2str(Route));
	    return;
	}

	/*
	 *  Note that even if the exported netmail is not crash, that if
	 *  the routing node has crash status, this netmail will be send
	 *  crash.
	 */
	memset(&ext, 0, sizeof(ext));
	if (nodes.PackNetmail)
	    snprintf(ext, 4, (char *)"qqq");
	else if (nodes.Crash)
	    snprintf(ext, 4, (char *)"ccc");
	else if (nodes.Hold)
	    snprintf(ext, 4, (char *)"hhh");
	else
	    snprintf(ext, 4, (char *)"nnn");
	if ((qp = OpenPkt(msgs.Aka, Route, (char *)ext)) == NULL) {
	    net_bad++;
	    return;
	}
    }

    flags |= (Msg.Private)	    ? M_PVT   : 0;
    flags |= (Msg.Crash)	    ? M_CRASH : 0;
    flags |= (Msg.Hold)		    ? M_HOLD  : 0;
    flags |= (Msg.Immediate)	    ? M_CRASH : 0;
    flags |= (Msg.FileRequest)	    ? M_REQ   : 0;
    flags |= (Msg.FileAttach)	    ? M_FILE  : 0;
    flags |= (Msg.ReceiptRequest)   ? M_RRQ   : 0;
    flags |= (Msg.ConfirmRequest)   ? M_AUDIT : 0;

    too  = fido2faddr(Dest);
    from = fido2faddr(msgs.Aka);
    if (UUCPgate) {
	Syslog('m', "AddMsgHdr(%s, %s, %s)", (char *)"UUCP", Msg.From, Msg.Subject);
	rc = AddMsgHdr(qp, from, too, flags, 0, Msg.Written, (char *)"UUCP", Msg.From, Msg.Subject);
    } else {
	rc = AddMsgHdr(qp, from, too, flags, 0, Msg.Written, Msg.To, Msg.From, Msg.Subject);
    }
    tidy_faddr(from);
    tidy_faddr(too);

    if (rc) {
	WriteError("Create message failed");
	return;
    }

    if (msgs.Aka.point && !is_fmpt)
	fprintf(qp, "\001FMPT %d\r", msgs.Aka.point);
    if (Dest.point && !is_topt)
	fprintf(qp, "\001TOPT %d\r", Dest.point);
    if (!is_intl)
	fprintf(qp, "\001INTL %d:%d/%d %d:%d/%d\r", Dest.zone, Dest.net, Dest.node, 
		    msgs.Aka.zone, msgs.Aka.net, msgs.Aka.node);

    if (Msg_Read(MsgNum, 79)) {
	first = TRUE;
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		if (UUCPgate && first && (p[0] != '\001')) {
		    /*
		     * Past the kludges at the message start.
		     * Add the To: name@dom.com and a blank line.
		     */
		    fprintf(qp, "To: %s\r", Msg.To);
		    fprintf(qp, "\r");
		    first = FALSE;
		}
		fprintf(qp, "%s\r", p);
		if (strncmp(p, "--- ", 4) == 0)
		    break;
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }

    now = time(NULL);
    tm = gmtime(&now);
    fprintf(qp, "\001Via %s @%d%02d%02d.%02d%02d%02d.01.UTC MBSE BBS %s\r",
		aka2str(msgs.Aka), tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, VERSION);

    putc(0, qp);
    fclose(qp);

    if (Msg.FileAttach) {
	if (Msg.Crash)
	    flavor = 'c';
	else
	    flavor = 'f';

	ta = parsefnode(Msg.ToAddress);
	p = calloc(PATH_MAX, sizeof(char));
	snprintf(p, PATH_MAX, "%s/%d.%d.%d.%d/.filelist", CFG.out_queue, ta->zone, ta->net, ta->node, ta->point);
	mkdirs(p, 0750);

	if ((fl = fopen(p, "a+")) == NULL) {
	    WriteError("$Can't open %s", p);
	} else {
	    if (strlen(CFG.dospath)) {
		fprintf(fl, "%c LEAVE NOR %s\n", flavor, Dos2Unix(Msg.Subject));
		Syslog('+', "FileAttach %s", Dos2Unix(Msg.Subject));
	    } else {
		fprintf(fl, "%c LEAVE NOR %s\n", flavor, Msg.Subject);
		Syslog('+', "FileAttach %s", Msg.Subject);
	    }
	    fsync(fileno(fl));
	    fclose(fl);
	}
	tidy_faddr(ta);
    }

    net_out++;
}



/*
 *  Export Email message, the messagebase is locked.
 */
void ExportEmail(unsigned int MsgNum)
{
    char    *p, *q, MailFrom[128], MailTo[128];
    FILE    *qp;
    int	    retval, flags = 0, kludges = TRUE;
    faddr   *from, *too;

    Syslog('m', "Export email to %s", Msg.To);
    Syslog('m', "           from %s", Msg.From);
    Msg.Sent = TRUE;
    if (Msg.KillSent)
	Msg.Deleted = TRUE;

    /*
     *  For local scanned messages both addresses are the same.
     */
    from = fido2faddr(CFG.EmailFidoAka);
    too  = fido2faddr(CFG.EmailFidoAka);
    qp = tmpfile();

    flags |= (Msg.Private)          ? M_PVT   : 0;
    flags |= (Msg.Crash)            ? M_CRASH : 0;
    flags |= (Msg.Hold)             ? M_HOLD  : 0;
    flags |= (Msg.Immediate)        ? M_CRASH : 0;
    flags |= (Msg.FileRequest)      ? M_REQ   : 0;
    flags |= (Msg.FileAttach)       ? M_FILE  : 0;
    flags |= (Msg.ReceiptRequest)   ? M_RRQ   : 0;
    flags |= (Msg.ConfirmRequest)   ? M_AUDIT : 0;

    Syslog('m', "------------ Scanned message start");
    if (Msg_Read(MsgNum, 79)) {
	if ((p = (char *)MsgText_First()) != NULL) {
	    do {
		Syslogp('m', printable(p, 0));
		/*
		 *  GoldED places ^A characters in front of the RFC headers, 
		 *  so does mbsebbs as well.
		 */
		if (p[0] == '\001') {
		    fprintf(qp, "%s\n", p+1);
		    if (!strncmp(p, "\001PID:", 5)) {
			fprintf(qp, "TID: MBSE-FIDO %s (%s-%s)\n", VERSION, OsName(), OsCPU());
		    }
		} else {
		    if (kludges) {
			kludges = FALSE;
			fprintf(qp, "\n");
		    }
		    fprintf(qp, "%s\n", p);
		}
	    } while ((p = (char *)MsgText_Next()) != NULL);
	}
    }
    Syslog('m', "------------ Scanned message end");
    rewind(qp);
    most_debug = TRUE;

    /*
     *  At this point the message is RFC formatted.
     */
    if (CFG.EmailMode != E_NOISP) {
	/*
	 *  Dialup or direct internet connection, send message via MTA.
	 *  Reformat the addresses for SMTP.
	 */
	p = Msg.From;
	if ((strchr(p, '<') != NULL) && (strchr(p, '>') != NULL)) {
	    q = strtok(p, "<");
	    q = strtok(NULL, ">");
	    snprintf(MailFrom, 128, "%s", q);
	} else if (Msg.From[0] == ' ') {
	    q = strtok(p, " ");
	    q = strtok(NULL, " \n\r\t");
	    snprintf(MailFrom, 128, "%s", q);
	} else {
	    snprintf(MailFrom, 128, "%s", Msg.From);
	}

        p = Msg.To;
	if ((strchr(p, '<') != NULL) && (strchr(p, '>') != NULL)) {
	    q = strtok(p, "<");
	    q = strtok(NULL, ">");
	    snprintf(MailTo, 128, "%s", q);
	} else if (Msg.To[0] == ' ') {
	    q = strtok(p, " ");
	    q = strtok(NULL, " \n\r\t");
	    snprintf(MailTo, 128, "%s", q);
	} else {
	    snprintf(MailTo, 128, "%s", Msg.To);
	}

	retval = postemail(qp, MailFrom, MailTo);
    } else {
	/*
	 *  Message goes to UUCP gateway.
	 */
	retval = rfc2ftn(qp, too);
    }

    most_debug = FALSE;
    tidy_faddr(from);
    tidy_faddr(too);
    Syslog('m', "posted email rc=%d", retval);
    email_out++;
}

