/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Scan for outgoing mail.
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/msg.h"
#include "../lib/clcomm.h"
#include "../lib/msgtext.h"
#include "../lib/dbnode.h"
#include "../lib/dbmsgs.h"
#include "addpkt.h"
#include "pack.h"
#include "tracker.h"
#include "ftn2rfc.h"
#include "rfc2ftn.h"
#include "postemail.h"
#include "scan.h"


extern	int	do_quiet;
extern	int	net_out;
extern	int	net_bad;
extern	int	echo_in;
extern	int	email_out;
extern	int	echo_out;
extern	int	most_debug;
int		scanned;

#define	MAXSEEN 70


/*
 * Internal prototypes
 */
void ScanFull(void);
void ScanOne(char *, unsigned long);
void ExportEcho(sysconnect, unsigned long, fa_list **);
void ExportNews( unsigned long, fa_list **);
void ExportNet(unsigned long, int);
void ExportEmail(unsigned long);



/*
 *  Scan for outgoing mail. If using the files $MBSE_ROOT/tmp/echomail.jam
 *  or netmail.jam not all mail is scanned a full mailscan will be 
 *  performed.
 */
void ScanMail(int DoAll)
{
	int		DoFull = FALSE, i = 0;
	unsigned long	msg;
	char		*Fname = NULL, *temp, *path;
	FILE		*fp;

	if (DoAll) {
		DoFull = TRUE;
	} else {
		scanned = 0;
		Fname = calloc(PATH_MAX, sizeof(char));
		temp  = calloc(128, sizeof(char));

		sprintf(Fname, "%s/tmp/echomail.jam", getenv("MBSE_ROOT"));
		if ((fp = fopen(Fname, "r")) != NULL) {
			while ((fgets(temp, 128, fp)) != NULL) {
				path = strtok(temp, " ");
				msg = atol(strtok(NULL, "\n"));
				Syslog('+', "Export message %lu from %s", msg, path);
				ScanOne(path, msg);
				i++;
				Nopper();
			}
			fclose(fp);
			unlink(Fname); 
		}

		sprintf(Fname, "%s/tmp/netmail.jam", getenv("MBSE_ROOT"));
		if ((fp = fopen(Fname, "r")) != NULL) {
			while ((fgets(temp, 128, fp)) != NULL) {
				path = strtok(temp, " ");
				msg = atol(strtok(NULL, "\n"));
				Syslog('+', "Export message %lu from %s", msg, path);
				ScanOne(path, msg);
				i++;
				Nopper();
			}
			fclose(fp);
			unlink(Fname);
		}

		if ((i != scanned) || (i == 0)) {
			Syslog('+', "Not all messages exported, forcing full mail scan");
			Syslog('+', "i=%d scanned=%d", i, scanned);
			DoFull = TRUE;
		}
		free(Fname);
		free(temp);
	}

	if (DoFull)
		ScanFull();

	if (echo_out || net_out)
		packmail();
	RemoveSema((char *)"mailout");
}



void ScanFull()
{
	char		*sAreas, sbe[128];
	FILE		*pAreas;
	long		arearec = 0, sysstart, nextstart;
	unsigned long	Total, Number;
	int		i;
	sysconnect	Link;
	fa_list		*sbl = NULL;

	Syslog('+', "Full mailscan");
	IsDoing("Scanning mail");

	if (!do_quiet) {
		colour(9, 0);
		printf("Scanning mail\n");
		colour(3, 0);
		fflush(stdout);
	}

	sAreas = calloc(PATH_MAX, sizeof(char));
	sprintf(sAreas, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((pAreas = fopen(sAreas, "r")) != NULL) {
		fread(&usrconfighdr, sizeof(usrconfighdr), 1, pAreas);

		while (fread(&usrconfig, usrconfighdr.recsize, 1, pAreas) == 1) {
			if (usrconfig.Email && strlen(usrconfig.Name)) {

				Nopper();
				if (!do_quiet) {
					colour(3, 0);
					printf("\r%8s %-40s", usrconfig.Name, usrconfig.sUserName);
					colour(13, 0);
					fflush(stdout);
				}

				sprintf(sAreas, "%s/%s/mailbox", CFG.bbs_usersdir, usrconfig.Name);
				if (Msg_Open(sAreas)) {
					if ((Total = Msg_Number()) != 0L) {
						Number = Msg_Lowest();

						do {
							if (CFG.slow_util && do_quiet)
								usleep(1);

							if (((Number % 10) == 0) && (!do_quiet)) {
								printf("%6lu\b\b\b\b\b\b", Number);
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

	sprintf(sAreas, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
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
				colour(3, 0);
				printf("\r%5ld .. %-40s", arearec, msgs.Name);
				colour(13, 0);
				fflush(stdout);
			}

			if (Msg_Open(msgs.Base)) {
				if ((Total = Msg_Number()) != 0L) {
					Number = Msg_Lowest();

					do {
						if (CFG.slow_util && do_quiet)
							usleep(1);

						if (((Number % 10) == 0) && (!do_quiet)) {
							printf("%6lu\b\b\b\b\b\b", Number);
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
										if (CFG.akavalid[i] &&
										    (msgs.Aka.zone == CFG.aka[i].zone) &&
										    (CFG.aka[i].point == 0) &&
										    !((msgs.Aka.net == CFG.aka[i].net) &&
										    (msgs.Aka.node == CFG.aka[i].node))) {
											sprintf(sbe, "%u/%u", CFG.aka[i].net,
												CFG.aka[i].node);
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
									if (strlen(msgs.Newsgroup))
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



void ScanOne(char *path, unsigned long MsgNum)
{
	char		*sAreas, sbe[128];
	FILE		*pAreas;
	long		sysstart;
	unsigned long	Total, Area = 0;
	int		i;
	sysconnect	Link;
	fa_list		*sbl = NULL;

	IsDoing("Scanning mail");

	if (!do_quiet) {
		colour(9, 0);
		printf("Scanning mail\n");
		colour(3, 0);
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
	sprintf(sAreas, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
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
			colour(3, 0);
			printf("\r%5ld .. %-40s", Area, msgs.Name);
			colour(13, 0);
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
									if (CFG.akavalid[i] && 
									    (msgs.Aka.zone == CFG.aka[i].zone) &&
									    (CFG.aka[i].point == 0) &&
									    !((msgs.Aka.net == CFG.aka[i].net) && 
									      (msgs.Aka.node == CFG.aka[i].node))) {
										sprintf(sbe, "%u/%u", CFG.aka[i].net, 
											CFG.aka[i].node);
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
								if (strlen(msgs.Newsgroup))
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



int RescanOne(faddr *L, char *marea, unsigned long Num)
// Return:    0 -> Ok
//            1 -> Unknown area
//            2 -> Node cant rescan this area
{
        unsigned long   Total, MsgNum, Area = 0;
        fa_list         *sbl = NULL;
        fidoaddr        *l;
        int             First, Found;
        unsigned long   rescanned;
        sysconnect      Link;

        IsDoing("ReScan mail");

        if (!do_quiet) {
                colour(9, 0);
                printf("ReScan mail\n");
                colour(3, 0);
                fflush(stdout);
        }

        l = faddr2fido( L );
        rescanned = 0L;
        if (!SearchMsgs(marea)) {
                syslog('+',"ReScan of unknown echo area %s", marea);
                return 1;
        }

        First = TRUE;
        Found = FALSE;
        while (GetMsgSystem(&Link, First)) {
                First = FALSE;
                if ((l->zone == Link.aka.zone) &&
                    (l->net  == Link.aka.net) &&
                    (l->node == Link.aka.node) &&
                    (l->point == Link.aka.point)) {
                        Found = TRUE;
                        break;
                }
        }
        if (!Found) {
                Syslog('+',"Node %s can't Rescan area %s", L, marea);
                return 2;
        }

        if ((msgs.Active) && (msgs.Type == ECHOMAIL)) {
                if (!do_quiet) {
                        colour(3, 0);
                        printf("\r%5ld .. %-40s", Area, msgs.Name);
                        colour(13, 0);
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
        Syslog('+',"Rescan OK. %ul messages rescanned", rescanned);
        return 0;
}



/*
 *  Export message to downlink. The messagebase is locked.
 */
void ExportEcho(sysconnect L, unsigned long MsgNum, fa_list **sbl)
{
	char	*p;
	int	seenlen, oldnet, flags = 0;
	char	sbe[16];
	fa_list	*tmpl;
	FILE	*qp;
	faddr	*from, *dest;
	int	is_pid = FALSE;

	if ((!L.sendto) || L.pause || L.cutoff)
		return;

	Syslog('M', "Export to %s", aka2str(L.aka));

	if ((qp = OpenPkt(msgs.Aka, L.aka, (char *)"qqq")) == NULL)
		return;

	flags |= (Msg.Private)		? M_PVT : 0;
	from = fido2faddr(msgs.Aka);
	dest = fido2faddr(L.aka);
	AddMsgHdr(qp, from, dest, flags, 0, Msg.Written, Msg.To, Msg.From, Msg.Subject);
	tidy_faddr(from);
	tidy_faddr(dest);
	fprintf(qp, "AREA:%s\r", msgs.Tag);

	if (Msg_Read(MsgNum, 78)) {
		if ((p = (char *)MsgText_First()) != NULL) {
			do {
				if ((strncmp(p, " * Origin:", 10) == 0) && !is_pid) {
					/*
					 * If there was no PID kludge, insert the TID
					 * kludge anyway.
					 */
					fprintf(qp, "\001TID: MBSE-FIDO %s\r", VERSION);
				}
				fprintf(qp, "%s", p);
				if (strncmp(p, " * Origin:", 10) == 0)
					break;

				/*
				 * Only append CR if not the last line
				 */
				fprintf(qp, "\r");

				/*
				 * Append ^aTID line behind the PID.
				 */
				if (strncmp(p, "\001PID", 4) == 0) {
					fprintf(qp, "\001TID: MBSE-FIDO %s\r", VERSION);
					is_pid = TRUE;
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
			sprintf(sbe, " %u", tmpl->addr->node);
		else
			sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
		oldnet = tmpl->addr->net;
		seenlen += strlen(sbe);
		if (seenlen > MAXSEEN) {
			seenlen = 0;
			fprintf(qp, "\rSEEN-BY:");
			sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
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
void ExportNews(unsigned long MsgNum, fa_list **sbl)
{
	char    *p;
	int     seenlen, oldnet, flags = 0;
	char    sbe[16];
	fa_list *tmpl;
	FILE    *qp;
	faddr   *from, *dest;
	int     is_pid = FALSE, kludges = TRUE;

	qp = tmpfile();

	Syslog('m', "Msg.From %s", Msg.From);
	Syslog('m', "Msg.FromAddress %s", Msg.FromAddress);
	Syslog('m', "Msg.To %s", Msg.To);
	Syslog('m', "Msg.ToAdrress", Msg.ToAddress);

	flags |= (Msg.Private)          ? M_PVT : 0;
	from = fido2faddr(msgs.Aka);

	/*
	 * Add name with echo to news gate.
	 */
	from->name = xstrcpy(Msg.From);
	Syslog('m', "from %s", ascinode(from, 0xff));
	dest = NULL;

	fprintf(qp, "AREA:%s\n", msgs.Tag);
	Syslog('m', "AREA:%s", msgs.Tag);

	if (Msg_Read(MsgNum, 78)) {
		if ((p = (char *)MsgText_First()) != NULL) {
			do {
				if (kludges) {
					if (p[0] != '\001') {
						/*
						 * After the first kludges, send RFC headers
						 */
						kludges = FALSE;
						fprintf(qp, "Subject: %s\n", Msg.Subject);
						Syslog('m', "Subject: %s", Msg.Subject);
						fprintf(qp, "\n");
						Syslog('m', "\n");
						fprintf(qp, "%s\n", p);
						Syslog('m', "%s", p);
					} else {
						fprintf(qp, "%s\n", p+1);
						Syslog('m', "%s", p+1);
					}
				} else {
					if ((strncmp(p, " * Origin:", 10) == 0) && !is_pid) {
						/*
						 * If there was no PID kludge, insert the TID
						 * kludge anyway.
						 */
						fprintf(qp, "\001TID: MBSE-FIDO %s\n", VERSION);
						Syslog('m', "\\001TID: MBSE-FIDO %s", VERSION);
					}
					fprintf(qp, "%s", p);
					Syslog('m', "%s", printable(p, 0));
					if (strncmp(p, " * Origin:", 10) == 0)
						break;

					/*
					 * Only append NL if not the last line
					 */
					fprintf(qp, "\n");

					/*
					 * Append ^aTID line
					 */
					if (strncmp(p, "\001PID", 4) == 0) {
						fprintf(qp, "\001TID: MBSE-FIDO %s\n", VERSION);
						Syslog('m', "\\001TID: MBSE-FIDO %s", VERSION);
						is_pid = TRUE;
					}
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
			sprintf(sbe, " %u", tmpl->addr->node);
		else
			sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
		oldnet = tmpl->addr->net;
		seenlen += strlen(sbe);
		if (seenlen > MAXSEEN) {
			seenlen = 0;
			fprintf(qp, "\nSEEN-BY:");
			sprintf(sbe, " %u/%u", tmpl->addr->net, tmpl->addr->node);
			seenlen = strlen(sbe);
		}
		fprintf(qp, "%s", sbe);
	}
	fprintf(qp, "\n\001PATH: %u/%u\n", msgs.Aka.net, msgs.Aka.node);
	Syslog('m', "\\001PATH: %u/%u", msgs.Aka.net, msgs.Aka.node);

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
void ExportNet(unsigned long MsgNum, int UUCPgate)
{
	char		*p, *q, ext[4], fromname[37];
	int		i, rc, flags = 0, first;
	FILE		*qp, *fp;
	fidoaddr	Dest, Route, *dest;
	time_t		now;
	struct tm	*tm;
	char		flavor;
	faddr		*from, *too, *ta;
	int		is_fmpt = FALSE, is_topt = FALSE, is_intl = FALSE;
	unsigned short	point;
	char		MailFrom[128], MailTo[128];

	Syslog('m', "Export netmail to %s of %s (%s) %s mode", Msg.To, Msg.ToAddress, 
		(Msg.Crash || Msg.Direct || Msg.FileAttach) ? "Direct" : "Routed", UUCPgate ? "UUCP" : "Netmail");

        /*
         *  Analyze this message if it contains INTL, FMPT and TOPT kludges 
         *  and check if we need them. If they are missing they are inserted.
         *  GoldED doesn't insert them but MBSE does.
         */
        if (Msg_Read(MsgNum, 78)) {
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
                        } while ((p = (char *)MsgText_Next()) != NULL);
                }
        }

	/*
	 *  Check if this a netmail to our own local UUCP gate.
	 */
	if ((!strcmp(Msg.To, "UUCP")) && (is_local(parsefnode(Msg.ToAddress)))) {
		most_debug = TRUE;
		Syslog('m', "We are the UUCP gate");
		Syslog('m', "From %s FromAddress %s", Msg.From, Msg.FromAddress);
		if ((fp = tmpfile()) == NULL) {
			WriteError("$Can't open tempfile");
			return;
		}
		from = fido2faddr(msgs.Aka);
		sprintf(fromname, "%s", Msg.From);
		for (i = 0; i < strlen(fromname); i++)
			if (fromname[i] == ' ')
				fromname[i] = '_';
		sprintf(MailFrom, "%s@%s", fromname, ascinode(from, 0x2f));

        	if (Msg_Read(MsgNum, 78)) {
                	if ((p = (char *)MsgText_First()) != NULL) {
                        	do {
					if (strncmp(p, "To: ", 4) == 0) {
						q = strtok(p, "<");
						q = strtok(NULL, ">");
						sprintf(MailTo, "%s", q);
						break;
					}
				} while ((p = (char *)MsgText_Next()) != NULL);
			}
		}

		/*
		 *  First send all headers
		 */
		fprintf(fp, "Date: %s\n", rfcdate(Msg.Written));
		fprintf(fp, "From: %s@%s\n", fromname, ascinode(from, 0x2f));
		tidy_faddr(from);
		fprintf(fp, "Subject: %s\n", Msg.Subject);
		fprintf(fp, "MIME-Version: 1.0\n");
		fprintf(fp, "Content-Type: text/plain\n");
		fprintf(fp, "Content-Transfer-Encoding: 8bit\n");
		fprintf(fp, "X-Mailreader: MBSE BBS %s\r\n", VERSION);

		if (msgs.Aka.point && !is_fmpt)
			fprintf(fp, "X-FTN-FMPT: %d\r", msgs.Aka.point);
		if (Dest.point && !is_topt)
			fprintf(fp, "X-FTN-TOPT: %d\r", Dest.point);
		if (!is_intl)
			fprintf(fp, "X-FTN-INTL: %d:%d/%d %d:%d/%d\r", Dest.zone, Dest.net, Dest.node,
				msgs.Aka.zone, msgs.Aka.net, msgs.Aka.node);

                if (Msg_Read(MsgNum, 78)) {
                        if ((p = (char *)MsgText_First()) != NULL) {
                                do {
					if (p[0] == '\001') {
						if (strncmp(p, "\001INTL", 5) == 0)
							fprintf(fp, "X-FTN-INTL: %s\n", p+4);
						else
							fprintf(fp, "X-FTN-%s\n", p+1);
					}
                                } while ((p = (char *)MsgText_Next()) != NULL);
                        }
                }

                if (Msg_Read(MsgNum, 78)) {
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
			sprintf(ext, (char *)"iii");
		else if (Msg.Crash)
			sprintf(ext, (char *)"ccc");
		else
			sprintf(ext, (char *)"nnn");
		point = Dest.point;
		Dest.point = 0;
		if (point)
			Syslog('+', "Routing via Boss %s", aka2str(Dest));
		if ((qp = OpenPkt(msgs.Aka, Dest, (char *)ext)) == NULL) {
			net_bad++;
			return;
		}
		Dest.point = point;

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
			sprintf(ext, (char *)"qqq");
		else if (nodes.Crash)
			sprintf(ext, (char *)"ccc");
		else if (nodes.Hold)
			sprintf(ext, (char *)"hhh");
		else
			sprintf(ext, (char *)"nnn");
		if ((qp = OpenPkt(msgs.Aka, Route, (char *)ext)) == NULL) {
			net_bad++;
			return;
		}
	}

	flags |= (Msg.Private)		? M_PVT   : 0;
	flags |= (Msg.Crash)		? M_CRASH : 0;
	flags |= (Msg.Hold)		? M_HOLD  : 0;
	flags |= (Msg.Immediate)	? M_CRASH : 0;
	flags |= (Msg.FileRequest)	? M_REQ   : 0;
	flags |= (Msg.FileAttach)	? M_FILE  : 0;
	flags |= (Msg.ReceiptRequest)	? M_RRQ   : 0;
	flags |= (Msg.ConfirmRequest)	? M_AUDIT : 0;

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

	if (Msg_Read(MsgNum, 78)) {
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
		if (strlen(CFG.dospath)) {
			rc = attach(*ta, Dos2Unix(Msg.Subject), LEAVE, flavor);
			Syslog('+', "FileAttach %s %s", Dos2Unix(Msg.Subject), rc ? "Success":"Failed");
		} else {
			rc = attach(*ta, Msg.Subject, LEAVE, flavor);
			Syslog('+', "FileAttach %s %s", Msg.Subject, rc ? "Success":"Failed");
		}
		tidy_faddr(ta);
	}

	net_out++;
}



/*
 *  Export Email message, the messagebase is locked.
 */
void ExportEmail(unsigned long MsgNum)
{
        char	*p;
	FILE	*qp;
	int	retval, flags = 0;
	faddr	*from, *too;
	int	kludges = TRUE;

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
	if (Msg_Read(MsgNum, 78)) {
		if ((p = (char *)MsgText_First()) != NULL) {
			do {
				Syslog('m', "%s", printable(p, 0));
				/*
				 *  GoldED places ^A characters in front of the RFC headers, 
				 *  so does mbsebbs as well.
				 */
				if (p[0] == '\001') {
					fprintf(qp, "%s\n", p+1);
					if (!strncmp(p, "\001PID:", 5)) {
						fprintf(qp, "TID: MBSE-FIDO %s\n", VERSION);
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
		 */
		retval = postemail(qp, Msg.From, Msg.To);
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

