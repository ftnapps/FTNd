/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - Scan mail outbound status
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "scanout.h"
#include "nodelist.h"
#include "callstat.h"
#include "outstat.h"


extern int		do_quiet;
extern struct sysconfig	CFG;



static struct _alist
{
	struct		_alist *next;	/* Next entry			*/
	faddr		addr;		/* Node address			*/
	int		flavors;	/* ORed flavors of mail/files	*/
	time_t		time;		/* Date/time of mail/files	*/
	off_t		size;		/* Total size of mail/files	*/
	callstat	cst;		/* Last call status		*/
	unsigned long	olflags;	/* Nodelist online flags	*/
	unsigned long	moflags;	/* Nodelist modem flags		*/
	unsigned long	diflags;	/* Nodelist ISDN flags		*/
	unsigned long	ipflags;	/* Nodelist TCP/IP flags	*/
	int		t1;		/* First Txx flag		*/
	int		t2;		/* Second Txx flag		*/
} *alist = NULL;


#define F_NORMAL 0x0001
#define F_CRASH  0x0002
#define	F_IMM	 0x0004
#define F_HOLD   0x0008
#define F_FREQ	 0x0010
#define	F_POLL   0x0020
#define	F_ISFLO	 0x0040
#define	F_ISPKT	 0x0080


int outstat()
{
	int		rc, first = TRUE;
	struct _alist	*tmp, *old;
	char		flstr[13];
	char		temp[81];

	tasklog('+', "Scanning outbound");
	/*
	 *  Clear current table
	 */
        for (tmp = alist; tmp; tmp = old) {
                old = tmp->next;
                free(tmp->addr.domain);
                free(tmp);
        }
        alist = NULL;

	if ((rc = scanout(each))) {
		tasklog('?', "Error scanning outbound, aborting");
		return rc;
	}

	for (tmp = alist; tmp; tmp = tmp->next) {
		if (first) {
			tasklog('+', "Flavor Out        Size   Online    Modem     ISDN   TCP/IP Calls Status Address");
			first = FALSE;
		}
		strcpy(flstr,"...... ... ..");
		if ((tmp->flavors) & F_IMM   ) flstr[0]='I';
		if ((tmp->flavors) & F_CRASH ) flstr[1]='C';
		if ((tmp->flavors) & F_NORMAL) flstr[2]='N';
		if ((tmp->flavors) & F_HOLD  ) flstr[3]='H';
		if ((tmp->flavors) & F_FREQ  ) flstr[4]='R';
		if ((tmp->flavors) & F_POLL  ) flstr[5]='P';
		if ((tmp->flavors) & F_ISPKT ) flstr[7]='M';
		if ((tmp->flavors) & F_ISFLO ) flstr[8]='F';
		if (tmp->t1) flstr[11] = tmp->t1;
		if (tmp->t2) flstr[12] = tmp->t2;

		sprintf(temp, "%s %8lu %08x %08x %08x %08x %5d %6d %s", flstr, (long)tmp->size, 
			(unsigned int)tmp->olflags, (unsigned int)tmp->moflags, 
			(unsigned int)tmp->diflags, (unsigned int)tmp->ipflags, 
			tmp->cst.tryno, tmp->cst.trystat, ascfnode(&(tmp->addr), 0x1f));
		tasklog('+', "%s", temp);
	}

	return 0;
}



int each(faddr *addr, char flavor, int isflo, char *fname)
{
	struct		_alist **tmp;
	struct		stat st;
	FILE		*fp;
	char		buf[256], *p;
	node		*nlent;
	callstat	*cst;

	if ((isflo != OUT_PKT) && (isflo != OUT_FLO) && (isflo != OUT_REQ) && (isflo != OUT_POL))
		return 0;

	for (tmp = &alist; *tmp; tmp = &((*tmp)->next))
		if (((*tmp)->addr.zone  == addr->zone) && ((*tmp)->addr.net   == addr->net) &&
		    ((*tmp)->addr.node  == addr->node) && ((*tmp)->addr.point == addr->point) &&
		    (((*tmp)->addr.domain == NULL) || (addr->domain == NULL) ||
		     (strcasecmp((*tmp)->addr.domain,addr->domain) == 0)))
			break;
	if (*tmp == NULL) {
		nlent = getnlent(addr);
		*tmp = (struct _alist *)malloc(sizeof(struct _alist));
		(*tmp)->next = NULL;
		(*tmp)->addr.name   = NULL;
		(*tmp)->addr.zone   = addr->zone;
		(*tmp)->addr.net    = addr->net;
		(*tmp)->addr.node   = addr->node;
		(*tmp)->addr.point  = addr->point;
		(*tmp)->addr.domain = xstrcpy(addr->domain);
		if (nlent->addr.domain)
			free(nlent->addr.domain);
		(*tmp)->flavors = 0;
		if (nlent->pflag != NL_DUMMY) {
			(*tmp)->olflags = nlent->oflags;
			(*tmp)->moflags = nlent->mflags;
			(*tmp)->diflags = nlent->dflags;
			(*tmp)->ipflags = nlent->iflags;
			(*tmp)->t1 = nlent->t1;
			(*tmp)->t2 = nlent->t2;
		} else {
			(*tmp)->olflags = 0L;
			(*tmp)->moflags = 0L;
			(*tmp)->diflags = 0L;
			(*tmp)->ipflags = 0L;
			(*tmp)->t1 = '\0';
			(*tmp)->t2 = '\0';
		}
		(*tmp)->time = time(NULL);
		(*tmp)->size = 0L;
	}

	cst = getstatus(addr);
	(*tmp)->cst.trytime = cst->trytime;
	(*tmp)->cst.tryno   = cst->tryno;
	(*tmp)->cst.trystat = cst->trystat;

	if ((isflo == OUT_FLO) || (isflo == OUT_PKT)) 
		switch (flavor) {
			case '?':	break;
			case 'i':	(*tmp)->flavors |= F_IMM; break;
			case 'o':	(*tmp)->flavors |= F_NORMAL; break;
			case 'c':	(*tmp)->flavors |= F_CRASH; break;
			case 'h':	(*tmp)->flavors |= F_HOLD; break;
			default:	tasklog('?', "Unknown flavor: '%c'\n",flavor); break;
		}

	if (stat(fname,&st) != 0) {
		tasklog('?', "$Can't stat %s", fname);
		st.st_size  = 0L;
		st.st_mtime = time(NULL);
	}

	/*
	 * Find the oldest time
	 */
	if (st.st_mtime < (*tmp)->time) 
		(*tmp)->time = st.st_mtime;

	if (isflo == OUT_FLO) {
		(*tmp)->flavors |= F_ISFLO;
		if ((fp = fopen(fname,"r"))) {
			while (fgets(buf, sizeof(buf) - 1, fp)) {
				if (*(p = buf + strlen(buf) - 1) == '\n') 
					*p-- = '\0';
				while (isspace(*p)) 
					*p-- = '\0';
				for (p = buf; *p; p++) 
					if (*p == '\\') 
						*p='/';
				for (p = buf; *p && isspace(*p); p++);
				if (*p == '~') continue;
				if ((*p == '#') || (*p == '-') || (*p == '^') || (*p == '@')) 
					p++;
				if (stat(p, &st) != 0) {
					if (strlen(CFG.dospath)) {
						if (stat(Dos2Unix(p), &st) != 0) {
							/*
							 * Fileattach dissapeared, maybe
							 * the node doesn't poll enough and
							 * is losing mail or files.
							 */
							st.st_size  = 0L;
							st.st_mtime = time(NULL);
						}
					} else {
						if (stat(p, &st) != 0) {
							st.st_size  = 0L;
							st.st_mtime = time(NULL);
						}
					}
				}

				if ((p = strrchr(fname,'/'))) 
					p++;
				else 
					p = fname;
				if ((strlen(p) == 12) && (strspn(p,"0123456789abcdefABCDEF") == 8) && (p[8] == '.')) {
					if (st.st_mtime < (*tmp)->time) 
						(*tmp)->time = st.st_mtime;
				}
				(*tmp)->size += st.st_size;
			}
			fclose(fp);
		} else 
			tasklog('?', "Can't open %s", fname);

	} else if (isflo == OUT_PKT) {
		(*tmp)->size += st.st_size;
		(*tmp)->flavors |= F_ISPKT;
	} else if (isflo == OUT_REQ) {
		(*tmp)->flavors |= F_FREQ;
	} else if (isflo == OUT_POL) {
		(*tmp)->flavors |= F_POLL;
	}

	return 0;
}


