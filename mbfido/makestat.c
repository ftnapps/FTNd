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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/diesel.h"
#include "../lib/msg.h"
#include "mgrutil.h"
#include "makestat.h"


extern int  do_quiet;



FILE *newpage(char *, FILE *);
FILE *newpage(char *Name, FILE *fi)
{
    char            *temp;
    static FILE*    fa;
    time_t	    later;

    later = time(NULL) + 86400;
    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/stat/%s.temp", CFG.www_root, Name);
    mkdirs(temp, 0755);

    if ((fa = fopen(temp, "w")) == NULL) {
	WriteError("$Can't create %s", temp);
    } else {
	MacroVars("a", "s", rfcdate(later));
	MacroRead(fi, fa);
	free(temp);
	return fa;
    }

    free(temp);
    return NULL;
}



void closepage(FILE *, char *, FILE *);
void closepage(FILE *fa, char *Name, FILE *fi)
{
    char    *temp1, *temp2;

    if (fa == NULL)
	return;

    temp1 = calloc(PATH_MAX, sizeof(char));
    temp2 = calloc(PATH_MAX, sizeof(char));
    MacroRead(fi, fa);
    fclose(fa);
    sprintf(temp1, "%s/stat/%s.html", CFG.www_root, Name);
    sprintf(temp2, "%s/stat/%s.temp", CFG.www_root, Name);
    rename(temp2, temp1);
    chmod(temp1, 0644);
    free(temp2);
    free(temp1);
    fa = NULL;
}



char *adate(time_t);
char *adate(time_t now)
{
	static char	buf[40];
	struct tm	ptm;

	if (now == 0L) {
		sprintf(buf, "&nbsp;");
	} else {
		ptm = *localtime(&now);
		sprintf(buf, "%02d-%02d-%04d %02d:%02d", ptm.tm_mday, ptm.tm_mon +1, ptm.tm_year + 1900,
			ptm.tm_hour, ptm.tm_min);
	}
	return buf;
}



void MakeStat(void)
{
    FILE	    *fi, *fg, *fw;
    char	    *name, *p, *q;
    int		    i, Total, Lm, Area;
    struct _history hist;
    long	    fileptr = 0;

    if (!strlen(CFG.www_root)) {
	Syslog('!', "Warning, WWW root not defined, skip statistical html creation");
	return;
    }

    if (!do_quiet) {
	printf("\rMaking statistical HTML pages");
	fflush(stdout);
    }
    
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
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, fg);

	if ((fi = OpenMacro("html.egroups", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    if ((fw = newpage((char *)"mgroups", fi)) != NULL) {
		fileptr = ftell(fi);
		while ((fread(&mgroup, mgrouphdr.recsize, 1, fg)) == 1) {
		    if (mgroup.Active) {
			fseek(fi, fileptr, SEEK_SET);
			MacroVars("b", "s", mgroup.Name);
			MacroVars("c", "s", mgroup.Comment);
			MacroVars("d", "s", mgroup.UseAka.zone ? aka2str(mgroup.UseAka):"&nbsp;");
			MacroVars("e", "s", adate(mgroup.LastDate));
			MacroVars("f", "d", mgroup.MsgsRcvd.lweek);
			MacroVars("g", "d", mgroup.MsgsRcvd.month[Lm]);
			MacroVars("h", "d", mgroup.MsgsSent.lweek);
			MacroVars("i", "d", mgroup.MsgsSent.month[Lm]);
			MacroRead(fi, fw);
		    }
		}
		closepage(fw, (char *)"mgroups", fi);
	    } else {
		WriteError("Can't create mgroups.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    if (!do_quiet) {
	printf(".");
	fflush(stdout);
    }
    sprintf(name, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) == NULL) {
	WriteError("$Can't open %s", name);
    } else {
	if ((fi = OpenMacro("html.mareas", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    if ((fw = newpage((char *)"mareas", fi)) != NULL) {
		fileptr = ftell(fi);
		fread(&msgshdr, sizeof(msgshdr), 1, fg);
		Area = 0;
		while ((fread(&msgs, msgshdr.recsize, 1, fg)) == 1) {
		    Area++;
		    if (msgs.Active) {
			if (Msg_Open(msgs.Base)) {
			    MacroVars("k", "d", Msg_Number());
			    Msg_Close();
			} else {
			    MacroVars("k", "d", 0);
			}
			fseek(fi, fileptr, SEEK_SET);
			MacroVars("b", "d", Area);
			MacroVars("c", "s", msgs.Name);
			MacroVars("d", "s", msgs.Tag);
			MacroVars("e", "s", msgs.Group);
			MacroVars("f", "s", adate(msgs.LastRcvd));
			MacroVars("g", "d", msgs.Received.lweek);
			MacroVars("h", "d", msgs.Received.month[Lm]);
			MacroVars("i", "d", msgs.Posted.lweek);
			MacroVars("j", "d", msgs.Posted.month[Lm]);
			MacroRead(fi, fw);
		    }
		    fseek(fg, msgshdr.syssize, SEEK_CUR);
		}
		closepage(fw, (char *)"mareas", fi);
	    } else {
		WriteError("Can't create mareas.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    if (!do_quiet) {
	printf(".");
	fflush(stdout);
    }
    sprintf(name, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) == NULL) {
	WriteError("$Can't open %s", name);
    } else {
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, fg);
	if ((fi = OpenMacro("html.fgroups", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    if ((fw = newpage((char *)"fgroups", fi)) != NULL) {
		fileptr = ftell(fi);
		while ((fread(&fgroup, fgrouphdr.recsize, 1, fg)) == 1) {
		    if (fgroup.Active) {
			fseek(fi, fileptr, SEEK_SET);
			MacroVars("b", "s", fgroup.Name);
			MacroVars("c", "s", fgroup.Comment);
			MacroVars("d", "s", fgroup.UseAka.zone ? aka2str(fgroup.UseAka):"&nbsp;");
			MacroVars("e", "s", adate(fgroup.LastDate));
			MacroVars("f", "d", fgroup.Files.lweek);
			MacroVars("g", "d", fgroup.KBytes.lweek);
			MacroVars("h", "d", fgroup.Files.month[Lm]);
			MacroVars("i", "d", fgroup.KBytes.month[Lm]);
			MacroRead(fi, fw);
		    }
		}
		closepage(fw, (char *)"fgroups", fi);
	    } else {
		WriteError("Can't create fgroups.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    if (!do_quiet) {
	printf(".");
	fflush(stdout);
    }
    sprintf(name, "%s/etc/tic.data", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) == NULL) {
	WriteError("$Can't open %s", name);
    } else {
	fread(&tichdr, sizeof(tichdr), 1, fg);
	if ((fi = OpenMacro("html.tic", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    if ((fw = newpage((char *)"tic", fi)) != NULL) {
		fileptr = ftell(fi);
		while ((fread(&tic, tichdr.recsize, 1, fg)) == 1) {
		    if (tic.Active) {
			fseek(fi, fileptr, SEEK_SET);
			MacroVars("b", "s", tic.Comment);
			MacroVars("c", "s", tic.Name);
			MacroVars("d", "s", tic.Group);
			MacroVars("e", "s", adate(tic.LastAction));
			MacroVars("f", "d", tic.Files.lweek);
			MacroVars("g", "d", tic.KBytes.lweek);
			MacroVars("h", "d", tic.Files.month[Lm]);
			MacroVars("i", "d", tic.KBytes.month[Lm]);
			MacroRead(fi, fw);
		    }
		    fseek(fg, tichdr.syssize, SEEK_CUR);
		}
		closepage(fw, (char *)"tic", fi);
	    } else {
		WriteError("Can't create tic.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    if (!do_quiet) {
	printf(".");
	fflush(stdout);
    }
    sprintf(name, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) == NULL) {
	WriteError("$Can't open %s", name);
    } else {
	fread(&nodeshdr, sizeof(nodeshdr), 1, fg);
	if ((fi = OpenMacro("html.nodes", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    if ((fw = newpage((char *)"nodes", fi)) != NULL) {
		fileptr = ftell(fi);
                while ((fread(&nodes, nodeshdr.recsize, 1, fg)) == 1) {
		    fseek(fg, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
		    p = xstrcpy(adate(nodes.StartDate));
		    fseek(fi, fileptr, SEEK_SET);
		    if (nodes.Crash)
			q = xstrcpy((char *)"Crash");
		    else if (nodes.Hold)
			q = xstrcpy((char *)"Hold");
		    else
			q = xstrcpy((char *)"Normal");
		    MacroVars("b", "s", aka2str(nodes.Aka[0]));
		    MacroVars("c", "s", nodes.Sysop);
		    MacroVars("d", "s", q);
		    MacroVars("e", "s", p);
		    MacroVars("f", "s", adate(nodes.LastDate));
		    MacroVars("g", "d", nodes.Billing);
		    MacroVars("h", "d", nodes.Credit);
		    MacroVars("i", "d", nodes.Debet);
		    MacroVars("j", "d", nodes.F_KbSent.total);
		    MacroVars("k", "d", nodes.F_KbRcvd.total);
		    MacroVars("l", "d", nodes.MailSent.total);
		    MacroVars("m", "d", nodes.MailRcvd.total);
		    MacroRead(fi, fw);
		    free(p);
		    free(q);
		}
                closepage(fw, (char *)"nodes", fi);
	    } else {
		WriteError("Can't create nodes.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    if (!do_quiet) {
	printf(".");
	fflush(stdout);
    }
    sprintf(name, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) == NULL) {
	WriteError("$Can't open %s", name);
    } else {
	if ((fi = OpenMacro("html.mailer", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    fseek(fg, 0, SEEK_END);
	    Total = (ftell(fg) / sizeof(hist)) -1;
	    fseek(fg, 0, SEEK_SET);
	    fread(&hist, sizeof(hist), 1, fg);
	    MacroVars("b", "s", adate(hist.online));
	    if ((fw = newpage((char *)"mailhistory", fi)) != NULL) {
		fileptr = ftell(fi);
		for (i = Total; i > 0; i--) {
		    fseek(fg, i * sizeof(hist), SEEK_SET);
		    fread(&hist, sizeof(hist), 1, fg);
		    fseek(fi, fileptr, SEEK_SET);
		    if (!strcmp(hist.aka.domain, "(null)"))
			hist.aka.domain[0] = '\0';
		    MacroVars("c", "s", hist.aka.zone ? aka2str(hist.aka):"&nbsp;");
		    MacroVars("d", "s", hist.system_name);
		    MacroVars("e", "s", hist.sysop);
		    MacroVars("f", "s", hist.location);
		    MacroVars("g", "s", strlen(hist.tty) ? hist.tty:"&nbsp;");
		    MacroVars("h", "s", adate(hist.online));
		    MacroVars("i", "s", t_elapsed(hist.online, hist.offline));
		    MacroVars("j", "d", hist.sent_bytes);
		    MacroVars("k", "d", hist.rcvd_bytes);
		    MacroVars("l", "d", hist.cost);
		    MacroVars("m", "s", hist.inbound ? "In":"Out");
		    MacroRead(fi, fw);
		}
		closepage(fw, (char *)"mailhistory", fi);
	    } else {
		WriteError("Can't create mailhistory.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    if (!do_quiet) {
	printf(".");
	fflush(stdout);
    }
    sprintf(name, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) != NULL ) {
	if ((fi = OpenMacro("html.sysinfo", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    fread(&SYSINFO, sizeof(SYSINFO), 1, fg);
	    MacroVars("b", "d", SYSINFO.SystemCalls);
	    MacroVars("c", "d", SYSINFO.Pots);
	    MacroVars("d", "d", SYSINFO.ISDN);
	    MacroVars("e", "d", SYSINFO.Network);
	    MacroVars("f", "d", SYSINFO.Local);
	    MacroVars("g", "s", adate(SYSINFO.StartDate));
	    MacroVars("h", "s", SYSINFO.LastCaller);
	    MacroVars("i", "s", adate(SYSINFO.LastTime));
	    if ((fw = newpage((char *)"sysinfo", fi)) != NULL) {
		closepage(fw, (char *)"sysinfo", fi);
	    } else {
		WriteError("Can't create sysinfo.html");
	    }
	    fclose(fi);
	    MacroClear();
	}
	fclose(fg);
    }

    free(name);
    Syslog('+', "Finished making statistic HTML pages");

    if (!do_quiet) {
	printf("\r                                    \r");
	fflush(stdout);
    }
}


