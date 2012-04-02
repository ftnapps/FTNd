/*****************************************************************************
 *
 * $Id: scanout.c,v 1.15 2007/09/24 18:54:05 mbse Exp $
 * Purpose ...............: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "config.h"
#include "scanout.h"
#include "lutil.h"


static faddr addr = {
	NULL,
	0,0,0,0,
	NULL
};


extern time_t	t_start;


static int scan_dir(int (*)(faddr*,char,int,char*),char*,int);
static int scan_dir(int (*fn)(faddr *, char, int, char *), char *dname, int ispoint)
{
    char	    fname[PATH_MAX];
    char	    flavor = '?';
    DIR		    *dp = NULL;
    struct dirent   *de;
    int		    rc = 0, isflo, fage;
    unsigned short  t_net, t_node, t_point;

    Syslog('o' ,"scan_dir \"%s\" (%s)",MBSE_SS(dname),ispoint?"point":"node");

    if ((dp = opendir(dname)) == NULL) {
	Syslog('+', "Creating directory %s", dname);
	/*
	 * Create a fake filename, mkdirs() likes that.
	 */
	snprintf(fname, PATH_MAX -1, "%s/foo", dname);
	(void)mkdirs(fname, 0770);
	if ((dp = opendir(dname)) == NULL) {
	    Syslog('o' ,"\"%s\" cannot be opened, proceed",MBSE_SS(dname));
	    return 0;
	}
    }

    while ((de=readdir(dp)))
    if ((strlen(de->d_name) == 12) && (de->d_name[8] == '.') && (strspn(de->d_name,"0123456789abcdefABCDEF") == 8)) {
	Syslog('o' ,"checking: \"%s\"",de->d_name);
	addr.point= 0;
	strncpy(fname,dname,PATH_MAX-2);
	strcat(fname,"/");
	strncat(fname,de->d_name,PATH_MAX-strlen(fname)-2);

	if ((strcasecmp(de->d_name+9,"pnt") == 0) && !ispoint) {
	    sscanf(de->d_name,"%04hx%04hx",&t_net,&t_node);
	    addr.net = t_net;
	    addr.node = t_node;
	    if ((rc = scan_dir(fn, fname, 1)))
		goto exout;
	} else if ((strcasecmp(de->d_name+8,".out") == 0) ||
		   (strcasecmp(de->d_name+8,".cut") == 0) ||
		   (strcasecmp(de->d_name+8,".hut") == 0) ||
		   (strcasecmp(de->d_name+8,".iut") == 0) ||
		   (strcasecmp(de->d_name+8,".dut") == 0) ||
		   (strcasecmp(de->d_name+8,".opk") == 0) ||
		   (strcasecmp(de->d_name+8,".cpk") == 0) ||
		   (strcasecmp(de->d_name+8,".hpk") == 0) ||
		   (strcasecmp(de->d_name+8,".ipk") == 0) ||
		   (strcasecmp(de->d_name+8,".flo") == 0) ||
		   (strcasecmp(de->d_name+8,".clo") == 0) ||
		   (strcasecmp(de->d_name+8,".hlo") == 0) ||
		   (strcasecmp(de->d_name+8,".ilo") == 0) ||
		   (strcasecmp(de->d_name+8,".dlo") == 0) ||
		   (strcasecmp(de->d_name+8,".req") == 0) ||
		   (strcasecmp(de->d_name+8,".pol") == 0)) {
	    if (ispoint) {
		sscanf(de->d_name,"%08hx", &t_point);
		addr.point = t_point;
	    } else {
		sscanf(de->d_name,"%04hx%04hx", &t_net,&t_node);
		addr.net = t_net;
		addr.node = t_node;
	    }
	    flavor = tolower(de->d_name[9]);
	    if (flavor == 'f') 
		flavor = 'o';
	    if (flavor == 'i')
		flavor = 'd';
	    if (strcasecmp(de->d_name+10,"ut") == 0)
		isflo=OUT_PKT;
	    else if (strcasecmp(de->d_name+10,"pk") == 0)
		isflo=OUT_DIR;
	    else if (strcasecmp(de->d_name+10,"lo") == 0)
		isflo=OUT_FLO;
	    else if (strcasecmp(de->d_name+9,"req") == 0)
		isflo=OUT_REQ;
	    else if (strcasecmp(de->d_name+9,"pol") == 0)
		isflo=OUT_POL;
	    else
		isflo=-1;
	    Syslog('o' ,"%s \"%s\"", (isflo == OUT_FLO) ? "flo file" : "packet", de->d_name);
	    if ((rc=fn(&addr,flavor,isflo,fname)))
		goto exout;
	} else if ((strncasecmp(de->d_name+9,"su",2) == 0) ||
	           (strncasecmp(de->d_name+9,"mo",2) == 0) ||
	           (strncasecmp(de->d_name+9,"tu",2) == 0) ||
	           (strncasecmp(de->d_name+9,"we",2) == 0) ||
	           (strncasecmp(de->d_name+9,"th",2) == 0) ||
	           (strncasecmp(de->d_name+9,"fr",2) == 0) ||
	           (strncasecmp(de->d_name+9,"sa",2) == 0)) {
	    isflo = OUT_ARC;
	    if ((rc = fn(&addr, flavor, isflo, fname)))
		goto exout;

	    Syslog('o' ,"arcmail file \"%s\"",de->d_name);
	    snprintf(fname, PATH_MAX -1, "%s/%s", dname, de->d_name);
	    fage = (int)((t_start - file_time(fname)) / 86400);

	    if (file_size(fname) == 0) {
		Syslog('o', "Age %d days", fage);
		/*
		 *  Remove truncated ARCmail that has a day extension
		 *  other then the current day or if the file is older
		 *  then 6 days.
		 */
		if ((strncasecmp(de->d_name+9, dayname(), 2)) || (fage > 6)) {
		    if (unlink(fname) == 0)
			Syslog('+', "Removed truncated ARCmail file %s", fname);
		}
	    }

	    if (CFG.toss_days && (fage > CFG.toss_days)) {
		/*
		 *  Remove ARCmail that is on hold too long.
		 */
		if (unlink(fname) == 0)
		    Syslog('+', "Removed ARCmail %s, %d days", fname, fage);
	    }
	} else {
	    Syslog('o' ,"skipping \"%s\"",de->d_name);
	}
    }

exout:
    closedir(dp);
    return rc;
}



int scanout(int (*fn)(faddr *, char, int, char *))
{
	int		i, j, rc = 0;
	unsigned short	zone = 0;
	char		fext[5];
	char		*p = NULL, *q = NULL;
	DIR		*dp;

	if ((dp = opendir(CFG.outbound)) == NULL) {
		WriteError("$Can't open outbound directory \"%s\" for reading", MBSE_SS(CFG.outbound));
		return 1;
	}
	closedir(dp);

	/*
	 * Build outbound directory names with zone numbers.
	 */
	for (i = 0; i < 40; i++) {
		if ((CFG.aka[i].zone) && (CFG.aka[i].zone != zone)) {
			zone = CFG.aka[i].zone;
			if (SearchFidonet(zone)) {
				for (j = 0; j < 6; j++) {
					/*
					 * Create outbound directory name for
					 * the primary aka of that zone.
					 */
					p = xstrcpy(CFG.outbound);
					if (zone != CFG.aka[0].zone) {
						if ((q = strrchr(p, '/')))
							*q = '\0';
						p = xstrcat(p, (char *)"/");
						p = xstrcat(p, fidonet.domain);
					}

					/*
					 * Not primary zones in the domain get
					 * a directory extension.
					 */
					if (fidonet.zone[j]) {
						if (j) {
							snprintf(fext, 5, ".%03x", fidonet.zone[j]);
							p = xstrcat(p, fext);
						}
						Syslog('o', "Zone %d Dir %s", fidonet.zone[j], p);
						addr.zone = fidonet.zone[j];
						addr.domain = fidonet.domain;

						if ((rc = scan_dir(fn, p, 0))) {
							if (p)
								free(p);
							p = NULL;
							return rc;
						}
					}

					if (p) 
						free(p);
					p = NULL;
				}
			}
		}
	}
	return rc;
}


