/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Make Web statistics
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
#include "../lib/clcomm.h"
#include "../lib/diesel.h"
#include "makestat.h"




FILE *newpage(char *, char *);
FILE *newpage(char *Name, char *Title)
{
        char            linebuf[1024], outbuf[1024];
        static FILE*    fa;
	time_t		later;

	later = time(NULL) + 86400;
	sprintf(linebuf, "%s/stat/%s.temp", CFG.www_root, Name);
	mkdirs(linebuf, 0755);

        if ((fa = fopen(linebuf, "w")) == NULL) {
                WriteError("$Can't create %s", linebuf);
        } else {
                sprintf(linebuf, "%s", Title);
                html_massage(linebuf, outbuf);
                fprintf(fa, "<HTML>\n");
                fprintf(fa, "<META http-equiv=\"Expires\" content=\"%s\">\n", rfcdate(later));
                fprintf(fa, "<META http-equiv=\"Cache-Control\" content=\"no-cache, must-revalidate\">\n");
                fprintf(fa, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", CFG.www_charset);
                fprintf(fa, "<META name=\"%s\" lang=\"en\" content=\"%s\">\n", CFG.www_author, outbuf);
                fprintf(fa, "<HEAD>\n<TITLE>%s</TITLE>\n", outbuf);
                fprintf(fa, "<LINK rel=stylesheet HREF=\"%s/%s/css/files.css\">\n", CFG.www_url, CFG.www_link2ftp);
                fprintf(fa, "<STYLE TYPE=\"text/css\">\n");
                fprintf(fa, "</STYLE>\n</HEAD>\n<BODY>\n<A NAME=top></A>\n");
                fprintf(fa, "<H1 align=center>%s</H1><P>\n", Title);
		fprintf(fa, "<TABLE align=center cellspacing=0 cellpadding=2 border=1>\n");
                return fa;
        }
        return NULL;
}



void closepage(FILE *, char *);
void closepage(FILE *fa, char *Name)
{
        char    temp1[81], temp2[81];

        if (fa == NULL)
                return;

        fprintf(fa, "</TABLE><P>\n");
	fprintf(fa, "<DIV align=center>\n");
	fprintf(fa, "<A HREF=#top>Top of page</A>&nbsp;&nbsp;&nbsp;<A HREF=index.html>Statistic index</A>\n");
        fprintf(fa, "</DIV>\n</BODY>\n</HTML>\n");
        fclose(fa);
	sprintf(temp1, "%s/stat/%s.html", CFG.www_root, Name);
	sprintf(temp2, "%s/stat/%s.temp", CFG.www_root, Name);
        rename(temp2, temp1);
        fa = NULL;
}



char *adate(time_t);
char *adate(time_t now)
{
	static char	buf[40];
	struct tm	ptm;

	if (now == 0L) {
		sprintf(buf, "N/A");
	} else {
		ptm = *localtime(&now);
		sprintf(buf, "%02d-%02d-%04d %02d:%02d", ptm.tm_mday, ptm.tm_mon +1, ptm.tm_year + 1900,
			ptm.tm_hour, ptm.tm_min);
	}
	return buf;
}



void MakeStat(void)
{
	FILE		*fg, *fw;
	char		*name, *p;
	int		i, Total, Lm, Area;
	struct _history	hist;

	if (!strlen(CFG.www_root))
		return;

	Syslog('+', "Start making statistic HTML pages");
	name = calloc(128, sizeof(char));
	if (Miy == 0)
		Lm = 11;
	else
		Lm = Miy -1;

	sprintf(name, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((fg = fopen(name, "r")) == NULL) {
		WriteError("Can't open %s", name);
	} else {
		if ((fw = newpage((char *)"mgroups", (char *)"Message groups statistics")) != NULL) {
			fprintf(fw, "<TR><TH class=head colspan=4>Message group statistics</TH><TH colspan=2>Received last</TH><TH colspan=2>Sent last</TH></TR>\n");
			fprintf(fw, "<TR><TH>Group</TH><TH>Comment</TH><TH>Aka</TH><TH>Last date</TH><TH>week</TH><TH>month</TH><TH>week</TH><TH>month</TH></TR>\n");
			fread(&mgrouphdr, sizeof(mgrouphdr), 1, fg);
			while ((fread(&mgroup, mgrouphdr.recsize, 1, fg)) == 1) {
				if (mgroup.Active) {
					fprintf(fw, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD align=center>%s</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD></TR>\n",
						mgroup.Name, mgroup.Comment, aka2str(mgroup.UseAka), adate(mgroup.LastDate),
						mgroup.MsgsRcvd.lweek, mgroup.MsgsRcvd.month[Lm],
						mgroup.MsgsSent.lweek, mgroup.MsgsSent.month[Lm]);
				}
			}
			fprintf(fw, "</TABLE>\n");
			closepage(fw, (char *)"mgroups");
		} else {
			WriteError("Can't create mgroups.html");
		}
		fclose(fg);
	}

	
	sprintf(name, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fg = fopen(name, "r")) == NULL) {
		WriteError("$Can't open %s", name);
	} else {
		if ((fw = newpage((char *)"mareas", (char *)"Message areas statistics")) != NULL) {
			fprintf(fw, "<TR><TH class=head colspan=5>Message areas statistics</TH><TH colspan=2>Received last</TH><TH colspan=2>Posts last</TH></TR>\n");
			fprintf(fw, "<TR><TH>Nr</TH><TH>Area</TH><TH>Tag</TH><TH>Group</TH><TH>Last date</TH><TH>week</TH><TH>month</TH><TH>week</TH><TH>month</TH></TR>\n");
			fread(&msgshdr, sizeof(msgshdr), 1, fg);
			Area = 0;
			while ((fread(&msgs, msgshdr.recsize, 1, fg)) == 1) {
				Area++;
				if (msgs.Active) {
					fprintf(fw, "<TR><TD align=right>%d</TD><TD>%s</TD><TD>%s&nbsp;</TD><TD>%s&nbsp;</TD><TD align=center>%s</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD></TR>\n",
                                                Area, msgs.Name, msgs.Tag, msgs.Group, adate(msgs.LastRcvd),
                                                msgs.Received.lweek, msgs.Received.month[Lm],
                                                msgs.Posted.lweek, msgs.Posted.month[Lm]);
				}
				fseek(fg, msgshdr.syssize, SEEK_CUR);
			}
			fprintf(fw, "</TABLE>\n");
			closepage(fw, (char *)"mareas");
		} else {
			WriteError("Can't create mareas.html");
		}
		fclose(fg);
	}

	sprintf(name, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((fg = fopen(name, "r")) == NULL) {
		WriteError("$Can't open %s", name);
	} else {
		if ((fw = newpage((char *)"fgroups", (char *)"TIC file groups statistics")) != NULL) {
			fprintf(fw, "<TR><TH class=head colspan=4>TIC file group statistics</TH><TH colspan=2>Last week</TH><TH colspan=2>Last month</TH></TR>\n");
			fprintf(fw, "<TR><TH>Group</TH><TH>Comment</TH><TH>Aka</TH><TH>Last date</TH><TH>files</TH><TH>KBytes</TH><TH>files</TH><TH>KBytes</TH></TR>\n");
			fread(&fgrouphdr, sizeof(fgrouphdr), 1, fg);
			while ((fread(&fgroup, fgrouphdr.recsize, 1, fg)) == 1) {
				if (fgroup.Active) {
					fprintf(fw, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD align=center>%s</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD></TR>\n",
						fgroup.Name, fgroup.Comment, aka2str(fgroup.UseAka), adate(fgroup.LastDate),
						fgroup.Files.lweek, fgroup.KBytes.lweek,
						fgroup.Files.month[Lm], fgroup.KBytes.month[Lm]);
				}
			}
			fprintf(fw, "</TABLE>\n");
			closepage(fw, (char *)"fgroups");
		} else {
			WriteError("Can't create fgroups.html");
		}
		fclose(fg);
	}

	sprintf(name, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((fg = fopen(name, "r")) == NULL) {
		WriteError("$Can't open %s", name);
	} else {
		if ((fw = newpage((char *)"tic", (char *)"TIC file areas statistics")) != NULL) {
			fprintf(fw, "<TR><TH class=head colspan=4>TIC file areas statistics</TH><TH colspan=2>Last week</TH><TH colspan=2>Last month</TH></TR>\n");
			fprintf(fw, "<TR><TH>Area</TH><TH>Tag</TH><TH>Group</TH><TH>Last date</TH><TH>files</TH><TH>KBytes</TH><TH>files</TH><TH>KBytes</TH></TR>\n");
			fread(&tichdr, sizeof(tichdr), 1, fg);
			while ((fread(&tic, tichdr.recsize, 1, fg)) == 1) {
				if (tic.Active) {
					fprintf(fw, "<TR><TD>%s</TD><TD>%s&nbsp;</TD><TD>%s&nbsp;</TD><TD align=center>%s</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD><TD align=right>%ld</TD></TR>\n",
                                                tic.Comment, tic.Name, tic.Group, adate(tic.LastAction),
                                                tic.Files.lweek, tic.KBytes.lweek,
                                                tic.Files.month[Lm], tic.KBytes.month[Lm]);
				}
				fseek(fg, tichdr.syssize, SEEK_CUR);
			}
			fprintf(fw, "</TABLE>\n");
			closepage(fw, (char *)"tic");
		} else {
			WriteError("Can't create tic.html");
		}
		fclose(fg);
	}

        sprintf(name, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
        if ((fg = fopen(name, "r")) == NULL) {
                WriteError("$Can't open %s", name);
        } else {
                if ((fw = newpage((char *)"nodes", (char *)"Nodes statistics")) != NULL) {
                        fprintf(fw, "<TR><TH class=head colspan=7>Nodes statistics</TH></TR>\n");
                        fprintf(fw, "<TR><TH>Node</TH><TH>Sysop</TH><TH>Status</TH><TH>Start Date</TH><TH>Last date</TH><TH>Credit</TH><TH>Debet</TH></TR>\n");
                        fread(&nodeshdr, sizeof(nodeshdr), 1, fg);
                        while ((fread(&nodes, nodeshdr.recsize, 1, fg)) == 1) {
				fseek(fg, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
				p = xstrcpy(adate(nodes.StartDate));
				fprintf(fw, "<TR><TD>%s</TD><TD>%s</TD><TD>%s&nbsp;%s</TD><TD>%s</TD><TD>%s</TD><TD>%ld</TD><TD>%ld</TD></TR>\n",
					aka2str(nodes.Aka[0]), nodes.Sysop, nodes.Crash?"Crash":"", nodes.Hold?"Hold":"",
					p, adate(nodes.LastDate), nodes.Credit, nodes.Debet);
				free(p);
                        }
                        fprintf(fw, "</TABLE>\n");
                        closepage(fw, (char *)"nodes");
                } else {
                        WriteError("Can't create nodes.html");
                }
                fclose(fg);
        }

	sprintf(name, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
	if ((fg = fopen(name, "r")) == NULL) {
		WriteError("$Can't open %s", name);
	} else {
		if ((fw = newpage((char *)"mailhistory", (char *)"Mailer history")) != NULL) {
			fseek(fg, 0, SEEK_END);
			Total = (ftell(fg) / sizeof(hist)) -1;
			fseek(fg, 0, SEEK_SET);
			fread(&hist, sizeof(hist), 1, fg);
			fprintf(fw, "<TR><TH class=head colspan=8>Mailer history since %s</TH></TR>\n", adate(hist.online));
			fprintf(fw, "<TR><TH>Aka</TH><TH>System</TH><TH>Location</TH><TH>Time</TH><TH>Elapsed</TH><TH>Sent</TH><TH>Received</TH><TH>Mode</TH></TR>\n");

			for (i = Total; i > 0; i--) {
				fseek(fg, i * sizeof(hist), SEEK_SET);
				fread(&hist, sizeof(hist), 1, fg);
				fprintf(fw, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD align=right>%lu</TD><TD align=right>%lu</TD><TD>%s</TD></TR>", 
					aka2str(hist.aka), hist.system_name, hist.location,
					adate(hist.online), t_elapsed(hist.online, hist.offline), hist.sent_bytes,
					hist.rcvd_bytes, hist.inbound ? "In":"Out");
			}

			fprintf(fw, "</TABLE>\n");
			closepage(fw, (char *)"mailhistory");
		} else {
			WriteError("Can't create tic.html");
		}
		fclose(fg);
	}

	sprintf(name, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
	if ((fg = fopen(name, "r")) != NULL ) {
		fread(&SYSINFO, sizeof(SYSINFO), 1, fg);
		if ((fw = newpage((char *)"sysinfo", (char *)"BBS system information")) != NULL) {
			fprintf(fw, "<TR><TH class=head colspan=2>BBS system info</TH></TR>\n");
			fprintf(fw, "<TR><TH>Total calls</TH><TD align=right>%lu</TD></TR>\n", SYSINFO.SystemCalls);
			fprintf(fw, "<TR><TH>Pots calls</TH><TD align=right>%lu</TD></TR>\n", SYSINFO.Pots);
			fprintf(fw, "<TR><TH>ISDN calls</TH><TD align=right>%lu</TD></TR>\n", SYSINFO.ISDN);
			fprintf(fw, "<TR><TH>Network calls</TH><TD align=right>%lu</TD></TR>\n", SYSINFO.Network);
			fprintf(fw, "<TR><TH>Local calls</TH><TD align=right>%lu</TD></TR>\n", SYSINFO.Local);
			fprintf(fw, "<TR><TH>Start date</TH><TD align=right>%s</TD></TR>\n", adate(SYSINFO.StartDate));
			fprintf(fw, "<TR><TH>Last caller</TH><TD align=right>%s</TD></TR>\n", SYSINFO.LastCaller);
			fprintf(fw, "</TABLE>\n");
			closepage(fw, (char *)"sysinfo");
		}
		fclose(fg);
	}

	free(name);
	Syslog('+', "Finished making statistic HTML pages");
}


