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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/diesel.h"
#include "../lib/msg.h"
#include "mgrutil.h"
#include "makestat.h"




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
    FILE	    *fi, *fg, *fw;
    char	    *name, *p, *q;
    int		    i, Total, Lm, Area;
    struct _history hist;
    long	    fileptr = 0;

    if (!strlen(CFG.www_root)) {
	Syslog('!', "Warning, WWW root not defined, skip statistical html creation");
	return;
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
			MacroVars("bcdefghi", "ssssdddd", mgroup.Name, mgroup.Comment, aka2str(mgroup.UseAka), 
				adate(mgroup.LastDate), mgroup.MsgsRcvd.lweek, mgroup.MsgsRcvd.month[Lm], 
				mgroup.MsgsSent.lweek, mgroup.MsgsSent.month[Lm]);
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
			MacroVars("bcdefghij", "dssssdddd", Area, msgs.Name, msgs.Tag, msgs.Group, adate(msgs.LastRcvd),
			    msgs.Received.lweek, msgs.Received.month[Lm], msgs.Posted.lweek, msgs.Posted.month[Lm]);
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
			MacroVars("bcdefghi", "ssssdddd", fgroup.Name, fgroup.Comment, aka2str(fgroup.UseAka), 
				adate(fgroup.LastDate), fgroup.Files.lweek, fgroup.KBytes.lweek, 
				fgroup.Files.month[Lm], fgroup.KBytes.month[Lm]);
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
			MacroVars("bcdefghi", "ssssdddd", tic.Comment, tic.Name, tic.Group, adate(tic.LastAction),
                                 tic.Files.lweek, tic.KBytes.lweek, tic.Files.month[Lm], tic.KBytes.month[Lm]);
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
		    MacroVars("bcdefghi", "sssssddd", aka2str(nodes.Aka[0]), nodes.Sysop, q, p, 
			    adate(nodes.LastDate), nodes.Billing, nodes.Credit, nodes.Debet);
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
		    MacroVars("cdefghijklm", "sssssssddds", hist.aka.zone?aka2str(hist.aka):"N/A", hist.system_name, 
					hist.sysop, hist.location, hist.tty, adate(hist.online), 
					t_elapsed(hist.online, hist.offline), hist.sent_bytes,
					hist.rcvd_bytes, hist.cost, hist.inbound ? "In":"Out");
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

    sprintf(name, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
    if ((fg = fopen(name, "r")) != NULL ) {
	if ((fi = OpenMacro("html.sysinfo", 'E', TRUE)) == NULL) {
	    Syslog('+', "Can't open macro file, skipping html pages creation");
	} else {
	    fread(&SYSINFO, sizeof(SYSINFO), 1, fg);
	    MacroVars("bcdefgh", "dddddss", SYSINFO.SystemCalls, SYSINFO.Pots, SYSINFO.ISDN, SYSINFO.Network,
		    SYSINFO.Local, adate(SYSINFO.StartDate), SYSINFO.LastCaller);
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
}


