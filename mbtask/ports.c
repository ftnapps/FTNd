/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - mode portlists
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "../lib/structs.h"
#include "taskutil.h"
#include "nodelist.h"
#include "ports.h"


#ifndef LOCKDIR
#if defined(__FreeBSD__) || defined(__NetBSD__)
#define LOCKDIR "/var/spool/lock"
#else
#define LOCKDIR "/var/lock"
#endif
#endif

#define LCKPREFIX LOCKDIR"/LCK.."
#define LCKTMP LOCKDIR"/TMP."


extern char		ttyfn[];	    /* TTY file name		*/
extern time_t		tty_time;	    /* TTY update time		*/
extern int		rescan;		    /* Master rescan flag	*/
pp_list			*pl = NULL;	    /* Portlist			*/

int			pots_lines = 0;	    /* POTS (Modem) lines	*/
int			isdn_lines = 0;	    /* ISDN lines		*/
int			pots_free  = 0;	    /* POTS (Modem) lines free	*/
int			isdn_free  = 0;	    /* ISDN lines free		*/



/*
 * Tidy the portlist
 */
void tidy_portlist(pp_list **);
void tidy_portlist(pp_list ** fdp)
{
    pp_list *tmp, *old;

    tasklog('p', "tidy_portlist");
    for (tmp = *fdp; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *fdp = NULL;
}



/*
 * Add a port to the portlist
 */
void fill_portlist(pp_list **, pp_list *);
void fill_portlist(pp_list **fdp, pp_list *new)
{
    pp_list *tmp, *ta;

    tmp = (pp_list *)malloc(sizeof(pp_list));
    tmp->next = NULL;
    strncpy(tmp->tty, new->tty, 6);
    tmp->mflags = new->mflags;
    tmp->dflags = new->dflags;

    if (*fdp == NULL) {
	*fdp = tmp;
    } else {
	for (ta = *fdp; ta; ta = ta->next)
	    if (ta->next == NULL) {
		ta->next = (pp_list *)tmp;
		break;
	    }
    }
}



/*
 * Build a list of available dialout ports.
 */
void load_ports()
{
    FILE    *fp;
    pp_list new;
    int	    j, stdflag;
    char    *p, *q;

    tidy_portlist(&pl);
    if ((fp = fopen(ttyfn, "r")) == NULL) {
	tasklog('?', "$Can't open %s", ttyfn);
	return;
    }
    fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fp);
    
    tasklog('p', "Building portlist...");
    pots_lines = isdn_lines = 0;

    while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fp) == 1) {
	if (((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) && (ttyinfo.available) && (ttyinfo.callout)) {

	    if (ttyinfo.type == POTS)
		pots_lines++;
	    if (ttyinfo.type == ISDN)
		isdn_lines++;

	    memset(&new, 0, sizeof(new));
	    strncpy(new.tty, ttyinfo.tty, 6);

	    stdflag = TRUE;
	    q = xstrcpy(ttyinfo.flags);
	    for (p = q; p; p = q) {
		if ((q = strchr(p, ',')))
		    *q++ = '\0';
		if ((strncasecmp(p, "U", 1) == 0) && (strlen(p) == 1)) {
		    stdflag = FALSE;
		} else {
		    for (j = 0; fkey[j].key; j++)
			if (strcasecmp(p, fkey[j].key) == 0)
			    new.mflags |= fkey[j].flag;
		    for (j = 0; dkey[j].key; j++)
			if (strcasecmp(p, dkey[j].key) == 0)
			    new.dflags |= dkey[j].flag;
		}
	    }

	    tasklog('p', "port %s modem %08lx ISDN %08lx", new.tty, new.mflags, new.dflags);
	    fill_portlist(&pl, &new);
	}
    }

    fclose(fp);
    tty_time = file_time(ttyfn);
    tasklog('+', "Detected %d modem ports and %d ISDN ports", pots_lines, isdn_lines);
}



/*
 * Check status of all modem/ISDN ports. 
 * If something is changed set the master rescan flag.
 */
void check_ports(void)
{
    pp_list	*tpl;
    char	lckname[256];
    FILE	*lf;
    int		tmppid, changed = FALSE;
    pid_t	rempid = 0;

    pots_free = isdn_free = 0;

    for (tpl = pl; tpl; tpl = tpl->next) {
	sprintf(lckname, "%s%s", LCKPREFIX, tpl->tty);
	if ((lf = fopen(lckname, "r")) == NULL) {
	    if (tpl->locked) {
		tpl->locked = 0;
		tasklog('+', "Port %s is now free after %d seconds", tpl->tty, tpl->locktime);
		if (tpl->locktime > 4)
		    /*
		     * Good, set master rescan flag if longer then 4 seconds locked.
		     */
		    changed = TRUE;
	    }
	} else {
	    fscanf(lf, "%d", &tmppid);
	    rempid = tmppid;
	    fclose(lf);
	    if (kill(rempid, 0) && (errno == ESRCH)) {
		tasklog('+', "Stale lockfile for %s, unlink", tpl->tty);
		unlink(lckname);
		changed = TRUE;
	    } else {
		if (!tpl->locked) {
		    tpl->locked = rempid;
		    tpl->locktime = 0;
		    tasklog('+', "Port %s locked, pid %d", tpl->tty, rempid);
		} else {
		    /*
		     * Count locktime
		     */
		    tpl->locktime++;
		    if (tpl->locktime == 5) {
			changed = TRUE;
			tasklog('+', "Port %s locked for 5 seconds, forcing scan", tpl->tty);
		    }
		}
	    }
	}

	/*
	 * Now count free ports
	 */
	if (tpl->mflags && !tpl->locked)
	    pots_free++;
	if (tpl->dflags && !tpl->locked)
	    isdn_free++;

	if (changed) {
	    rescan = TRUE;
	    tasklog('p', "Free ports: pots=%d isdn=%d", pots_free, isdn_free);
	}
    }
}


