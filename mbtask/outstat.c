/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - Scan mail outbound status
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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

#include "../config.h"
#include "libs.h"
#include "../lib/structs.h"
#include "../lib/mberrors.h"
#include "taskutil.h"
#include "taskstat.h"
#include "scanout.h"
#include "../lib/nodelist.h"
#include "callstat.h"
#include "ports.h"
#include "outstat.h"


extern int		do_quiet;
extern int		internet;	    /* Internet status		*/
extern struct sysconfig	CFG;		    /* Main configuration	*/
extern struct taskrec	TCFG;		    /* Task configuration	*/
int			nxt_hour, nxt_min;  /* Time of next event	*/
int			inet_calls;	    /* Internet calls to make	*/
int			isdn_calls;	    /* ISDN calls to make	*/
int			pots_calls;	    /* POTS calls to make	*/
_alist_l		*alist = NULL;	    /* Nodes to call list	*/
extern int		s_do_inet;	    /* Internet wanted		*/
extern int		pots_lines;	    /* POTS lines available	*/
extern int		isdn_lines;	    /* ISDN lines available	*/
extern pp_list		*pl;		    /* Available ports		*/
extern char		waitmsg[];	    /* Waiting message		*/



/*
 * Load noderecord if the node is in our setup.
 */
int load_node(fidoaddr);
int load_node(fidoaddr n)
{
    char    *temp;
    FILE    *fp;
    int	    i, j = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	memset(&nodes, 0, sizeof(nodes));
	return FALSE;
    }
 
    fread(&nodeshdr, sizeof(nodeshdr), 1, fp);
    while (fread(&nodes, nodeshdr.recsize, 1, fp) == 1) {
	fseek(fp, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
	j++;
	for (i = 0; i < 20; i++) {
	    if ((n.zone == nodes.Aka[i].zone) && (n.net == nodes.Aka[i].net) &&
		(n.node == nodes.Aka[i].node) && (n.point == nodes.Aka[i].point)) {
		fclose(fp);
		free(temp);
		return TRUE;
	    }
	}
    }

    fclose(fp);
    memset(&nodes, 0, sizeof(nodes));
    free(temp);
    return FALSE;    
}



char *size_str(long);
char *size_str(long size)
{
    static char	fmt[25];
    
    if (size > 1048575) {
	sprintf(fmt, "%ldK", size / 1024);
    } else {
	sprintf(fmt, "%ld ", size);
    }
    return fmt;
}



void set_next(int, int);
void set_next(int hour, int min)
{
    time_t	now;
    struct tm	*etm;
    int		uhour, umin;

    now = time(NULL);
    etm = gmtime(&now);
    uhour = etm->tm_hour; /* For some reason, these intermediate integers are needed */
    umin  = etm->tm_min;

    if ((hour > uhour) || ((hour == uhour) && (min > umin))) {
	if (hour < nxt_hour) {
	    nxt_hour = hour;
	    nxt_min  = min;
	} else if ((hour == nxt_hour) && (min < nxt_min)) {
	    nxt_hour = hour;
	    nxt_min  = min;
	}
    }
}



char *callstatus(int status)
{
    switch (status) {
	case MBERR_OK:			return (char *)"Ok     ";
	case MBERR_TTYIO_ERROR:		return (char *)"tty err";
	case MBERR_NO_CONNECTION:	return (char *)"No conn";
	case MBERR_MODEM_ERROR:		return (char *)"Mdm err";
	case MBERR_NODE_LOCKED:		return (char *)"Locked ";
	case MBERR_UNKNOWN_SESSION:	return (char *)"unknown";
	case MBERR_NODE_NOT_IN_LIST:	return (char *)"Unlist ";
	case MBERR_NODE_MAY_NOT_CALL:	return (char *)"Forbid ";
	case MBERR_FTRANSFER:		return (char *)"Transf.";
	case MBERR_NO_PORT_AVAILABLE:	return (char *)"No tty ";
	case MBERR_NOT_ZMH:		return (char *)"No ZMH ";
	case MBERR_SESSION_ERROR:	return (char *)"Badsess";
	case MBERR_NO_IP_ADDRESS:	return (char *)"No IP  ";
	default:			Syslog('-', "callstatus(%d), unknown", status);
					return (char *)"ERROR  ";
    }
}



char *callmode(int mode)
{
    switch (mode) {
	case CM_NONE:	return (char *)"None   ";
	case CM_INET:	return (char *)"Inet   ";
	case CM_ISDN:	return (char *)"ISDN   ";
	case CM_POTS:	return (char *)"POTS   ";
	case MBFIDO:	return (char *)"mbfido ";
	case MBINDEX:	return (char *)"mbindex";
	case MBFILE:	return (char *)"mbfile ";
	case MBINIT:	return (char *)"mbinit ";
	default:	return (char *)"None   ";
    }
}



/*
 * Returns name of T-Mail filebox in Dos format (8+3).
 * I know, this code looks ungly, but it works. - MiCHA :-)
 */
const char* shortboxname(const faddr *fa) {
    static char	dirname[12];
    unsigned	z=fa->zone, n=fa->net, f=fa->node, p=fa->point;
    unsigned	u,v;

    u=z%32; z/=32; if (z>=32) return NULL;
    dirname[0]=z<10?z+'0':z-10+'a';
    dirname[1]=u<10?u+'0':u-10+'a';
    u=n%32; n/=32; v=n%32; n/=32; if (n>=32) return NULL;
    dirname[2]=n<10?n+'0':n-10+'a';
    dirname[3]=v<10?v+'0':v-10+'a';
    dirname[4]=u<10?u+'0':u-10+'a';
    u=f%32; f/=32; v=f%32; f/=32; if (f>=32) return NULL;
    dirname[5]=f<10?f+'0':f-10+'a';
    dirname[6]=v<10?v+'0':v-10+'a';
    dirname[7]=u<10?u+'0':u-10+'a';
    dirname[8]='.';
    u=p%32; p/=32; if (p>=32) return NULL;
    dirname[9]=p<10?p+'0':p-10+'a';
    dirname[10]=u<10?u+'0':u-10+'a';
    dirname[11]=0;
    return dirname;
}



/*
 * Scan outbound, the call status is set in three counters: internet,
 * ISDN and POTS (analogue modems).
 * For all systems the CM and Txx flags are checked and for official
 * FidoNet nodes the Zone Mail Hour wich belongs to the destination
 * zone.
 * All nodes are qualified, if there is a way to call them or not on
 * this moment.
 * The method how to call a node is decided as well.
 */
int outstat()
{
    int		    rc, first = TRUE, T_window, iszmh = FALSE;
    struct _alist   *tmp, *old;
    char	    flstr[13], *temp, as[6], be[6], utc[6], flavor;
    time_t	    now;
    struct tm	    *tm;
    int		    uhour, umin, thour, tmin;
    pp_list	    *tpl;
    faddr	    *fa;
    FILE	    *fp;
    DIR		    *dp = NULL;
    struct dirent   *de;
    struct stat	    sb;
    struct passwd   *pw;
    unsigned long   cmmask, ibnmask = 0, ifcmask = 0, itnmask = 0;
    nodelist_modem  **tmpm;

    cmmask = getCMmask();
    for (tmpm = &nl_tcpip; *tmpm; tmpm=&((*tmpm)->next)) {
	if (strcmp((*tmpm)->name, "IBN") == 0)
	    ibnmask = (*tmpm)->mask;
	if (strcmp((*tmpm)->name, "IFC") == 0)
	    ifcmask = (*tmpm)->mask;
	if (strcmp((*tmpm)->name, "ITN") == 0)
	    itnmask = (*tmpm)->mask;
    }
    now = time(NULL);
    tm = gmtime(&now); /* UTC time */
    uhour = tm->tm_hour;
    umin  = tm->tm_min;
    sprintf(utc, "%02d:%02d", uhour, umin);
    Syslog('+', "Scanning outbound at %s UTC.", utc);
    nxt_hour = 24;
    nxt_min  = 0;
    inet_calls = isdn_calls = pots_calls = 0;

    /*
     *  Clear current table
     */
    for (tmp = alist; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    alist = NULL;

    if ((rc = scanout(each))) {
	Syslog('?', "Error scanning outbound, aborting");
	return rc;
    }

    /*
     * Check private outbound box for nodes in the setup.
     */
    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	Syslog('?', "Error open %s, aborting", temp);
	free(temp);
	return 1;
    }
    fread(&nodeshdr, sizeof(nodeshdr), 1, fp);
    fseek(fp, 0, SEEK_SET);
    fread(&nodeshdr, nodeshdr.hdrsize, 1, fp);
    pw = getpwnam((char *)"mbse");

    while ((fread(&nodes, nodeshdr.recsize, 1, fp)) == 1) {
	if (strlen(nodes.OutBox)) {
	    if (nodes.Crash)
		flavor = 'c';
	    else if (nodes.Hold)
		flavor = 'h';
	    else
		flavor = 'o';

	    fa = (faddr *)malloc(sizeof(faddr));
	    fa->name   = NULL;
	    fa->domain = xstrcpy(nodes.Aka[0].domain);
	    fa->zone   = nodes.Aka[0].zone;
	    fa->net    = nodes.Aka[0].net;
	    fa->node   = nodes.Aka[0].node;
	    fa->point  = nodes.Aka[0].point;

	    if ((dp = opendir(nodes.OutBox)) != NULL) {
		while ((de = readdir(dp))) {
		    if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			sprintf(temp, "%s/%s", nodes.OutBox, de->d_name);
			if (stat(temp, &sb) == 0) {
			    if (S_ISREG(sb.st_mode)) {
				if (pw->pw_uid == sb.st_uid) {
				    /*
				     * We own the file
				     */
				    if ((sb.st_mode & S_IRUSR) && (sb.st_mode & S_IWUSR)) {
					each(fa, flavor, OUT_FIL, temp);
				    } else {
					Syslog('+', "No R/W permission on %s", temp);
				    }
				} else if (pw->pw_gid == sb.st_gid) {
				    /*
				     * We own the file group
				     */
				    if ((sb.st_mode & S_IRGRP) && (sb.st_mode & S_IWGRP)) {
					each(fa, flavor, OUT_FIL, temp);
				    } else {
					Syslog('+', "No R/W permission on %s", temp);
				    }
				} else {
				    /*
				     * No owner of file
				     */
				    if ((sb.st_mode & S_IROTH) && (sb.st_mode & S_IWOTH)) {
					each(fa, flavor, OUT_FIL, temp);
				    } else {
					Syslog('+', "No R/W permission on %s", temp);
				    }
				}
			    } else {
				Syslog('+', "Not a regular file: %s", temp);
			    }
			} else {
			    Syslog('?', "Can't stat %s", temp);
			}
		    }
		}
		closedir(dp);
	    }
	    if (fa->domain)
		free(fa->domain);
	    free(fa);
	}
	fseek(fp, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
    }
    fclose(fp);

    /*
     * During processing the outbound list, determine when the next event will occur,
     * ie. the time when the callout status of a node changes because of starting a
     * ZMH, or changeing the time window for Txx flags.
     */
    for (tmp = alist; tmp; tmp = tmp->next) {
	if (first) {
	    Syslog('+', "Flavor Out        Size    Online    Modem     ISDN   TCP/IP Calls Status  Mode    Address");
	    first = FALSE;
	}

	rc = load_node(tmp->addr);
//	Syslog('o', "Load node %s rc=%s, NoCall=%s, NoTCP=%s", fido2str(tmp->addr, 0x0f), rc?"true":"false",
//		    nodes.NoCall?"True":"False", (itnmask + ibnmask + ifcmask)?"False":"True");

	/*
	 * Zone Mail Hours, only use Fidonet Hours.
	 * Other nets use your default ZMH.
	 */
	T_window = iszmh = FALSE;
	switch (tmp->addr.zone) {
	    case 1:	if (uhour == 9)
			    iszmh = TRUE;
			set_next(9, 0);
			set_next(10, 0);
			break;
	    case 2:	if (((uhour == 2) && (umin >= 30)) || ((uhour == 3) && (umin < 30)))
			    iszmh = TRUE;
			set_next(2, 30);
			set_next(3, 30);
			break;
	    case 3:	if (uhour == 18)
			    iszmh = TRUE;
			set_next(18, 0);
			set_next(19, 0);
			break;
	    case 4:	if (uhour == 8)
			    iszmh = TRUE;
			set_next(8, 0);
			set_next(9, 0);
			break;
	    case 5:	if (uhour == 1)
			    iszmh = TRUE;
			set_next(1, 0);
			set_next(2, 0);
			break;
	    case 6:	if (uhour == 20)
			    iszmh = TRUE;
			set_next(20, 0);
			set_next(21, 0);
			break;
	    default:	if (get_zmh())
			    iszmh = TRUE;
			break;
	}

	if (tmp->t1 && tmp->t2) {
	    /*
	     * Txx flags, check callwindow
	     */
	    thour = toupper(tmp->t1) - 'A';
	    if (isupper(tmp->t1))
		tmin = 0;
	    else
		tmin = 30;
	    sprintf(as, "%02d:%02d", thour, tmin);
	    set_next(thour, tmin);
	    thour = toupper(tmp->t2) - 'A';
	    if (isupper(tmp->t2))
		tmin = 0;
	    else
		tmin = 30;
	    sprintf(be, "%02d:%02d", thour, tmin);
	    set_next(thour, tmin);
	    if (strcmp(as, be) > 0) {
		/*
		 * Time window is passing midnight
		 */
		if ((strcmp(utc, as) >= 0) || (strcmp(utc, be) < 0))
		    T_window = TRUE;
	    } else {
		/*
		 * Time window is not passing midnight
		 */
		if ((strcmp(utc, as) >= 0) && (strcmp(utc, be) < 0))
		    T_window = TRUE;
	    }
	}
//	Syslog('o', "T_window=%s, iszmh=%s", T_window?"true":"false", iszmh?"true":"false");
	strcpy(flstr,"...... .... ..");

	/*
	 * If the node has internet and we have internet configured, 
	 * check if we can send immediatly.
	 */
	if (TCFG.max_tcp && (tmp->olflags & cmmask) &&
		(((tmp->flavors) & F_IMM) || ((tmp->flavors) & F_CRASH) || ((tmp->flavors) & F_NORMAL)) &&
		((tmp->ipflags & ibnmask) || (tmp->ipflags & ifcmask) || (tmp->ipflags & itnmask))) {
	    tmp->flavors |= F_CALL;
	}

	if ((tmp->flavors) & F_IMM) {
	    flstr[0]='I';
	    /*
	     * Immediate mail, send if node is CM or is in a Txx window or is in ZMH.
	     */
	    if ((tmp->olflags & cmmask) || T_window || iszmh) {
		tmp->flavors |= F_CALL;
	    }
	}

	if ((tmp->flavors) & F_CRASH ) {
	    flstr[1]='C';
	    /*
	     * Crash mail, send if node is CM or is in a Txx window or is in ZMH.
	     */
	    if ((tmp->olflags & cmmask) || T_window || iszmh) {
		tmp->flavors |= F_CALL;
	    }
	}

	if ((tmp->flavors) & F_NORMAL)
	    flstr[2]='N';
	if ((tmp->flavors) & F_HOLD  ) 
	    flstr[3]='H';
	if ((tmp->flavors) & F_FREQ  ) 
	    flstr[4]='R';
	if ((tmp->flavors) & F_POLL  ) {
	    flstr[5]='P';
	    tmp->flavors |= F_CALL;
	}

	if ((tmp->flavors) & F_ISFIL ) {
	    flstr[7]='A';
	    /*
	     * Arcmail and maybe file attaches, send during ZMH or if node has a Txx window.
	     */
	    if (iszmh || T_window) {
		tmp->flavors |= F_CALL;
	    }
	}
	if ((tmp->flavors) & F_ISPKT ) { 
	    flstr[8]='M';
	    /*
	     * Normal mail, send during ZMH or if node has a Txx window.
	     */
	    if (iszmh || T_window) {
		tmp->flavors |= F_CALL;
	    }
	}
	if ((tmp->flavors) & F_ISFLO ) 
	    flstr[9]='F';

	if (tmp->cst.tryno >= 30) {
	    /*
	     * Node is undialable, clear callflag
	     */
	    tmp->flavors &= ~F_CALL;
	}
	if (tmp->t1) 
	    flstr[12] = tmp->t1;
	if (tmp->t2) 
	    flstr[13] = tmp->t2;

	/*
	 * If forbidden to call from setup, clear callflag.
	 */
	if (nodes.NoCall)
	    tmp->flavors &= ~F_CALL;

	/*
	 * If we must call this node, figure out how to call this node.
	 */
	if ((tmp->flavors) & F_CALL) {
	    tmp->callmode = CM_NONE;

	    if (TCFG.max_tcp && ((tmp->ipflags & ibnmask) || (tmp->ipflags & ifcmask) || (tmp->ipflags & itnmask))) {
		inet_calls++;
		tmp->callmode = CM_INET;
	    }

	    if ((tmp->callmode == CM_NONE) && isdn_lines) {
		/*
		 * If any matching port found, mark node ISDN
		 */
		for (tpl = pl; tpl; tpl = tpl->next) {
		    if (tmp->diflags & tpl->dflags) {
			isdn_calls++;
			tmp->callmode = CM_ISDN;
			break;
		    }
		}
	    }

	    if ((tmp->callmode == CM_NONE) && pots_lines) {
		/*
		 * If any matching ports found, mark node POTS
		 */
		for (tpl = pl; tpl; tpl = tpl->next) {
		    if (tmp->moflags & tpl->mflags) {
			pots_calls++;
			tmp->callmode = CM_POTS;
			break;
		    }
		}
	    }

	    /*
	     * Here we are out of options, clear callflag.
	     */
	    if (tmp->callmode == CM_NONE) {
		Syslog('!', "No method to call %s available", fido2str(tmp->addr, 0x0f));
		tmp->flavors &= ~F_CALL;
	    }
	}

	if ((tmp->flavors) & F_CALL)
	    flstr[10]='C';
	else
	    /*
	     * Safety, clear callmode.
	     */
	    tmp->callmode = CM_NONE;
	
	/*
	 * Show callresult for this node.
	 */
	sprintf(temp, "%s %8s %08x %08x %08x %08x %5d %s %s %s", flstr, size_str(tmp->size),
		(unsigned int)tmp->olflags, (unsigned int)tmp->moflags,
		(unsigned int)tmp->diflags, (unsigned int)tmp->ipflags,
		tmp->cst.tryno, callstatus(tmp->cst.trystat), callmode(tmp->callmode), fido2str(tmp->addr, 0x0f));
	Syslog('+', "%s", temp);

    } /* All nodes scanned. */
	
    if (nxt_hour == 24) {
	/*
	 * 24:00 hours doesn't exist
	 */
	nxt_hour = 0;
	nxt_min  = 0;
    }

    /*
     * Always set/reset semafore do_inet if internet is needed.
     */
    if (!IsSema((char *)"do_inet") && inet_calls) {
	CreateSema((char *)"do_inet");
	s_do_inet = TRUE;
	Syslog('c', "Created semafore do_inet");
    } else if (IsSema((char *)"do_inet") && !inet_calls) {
	RemoveSema((char *)"do_inet");
	s_do_inet = FALSE;
	Syslog('c', "Removed semafore do_inet");
    }

    /*
     * Log results
     */
    sprintf(waitmsg, "Next event at %02d:%02d UTC", nxt_hour, nxt_min);
    Syslog('+', "Systems to call: Inet=%d, ISDN=%d, POTS=%d, Next event at %02d:%02d UTC", 
	    inet_calls, isdn_calls, pots_calls, nxt_hour, nxt_min);
    free(temp);
    return 0;
}



int each(faddr *addr, char flavor, int isflo, char *fname)
{
    struct _alist   **tmp;
    struct stat	    st;
    FILE	    *fp;
    char	    buf[256], *p;
    node	    *nlent;
    callstat	    *cst;

    if ((isflo != OUT_PKT) && (isflo != OUT_FLO) && (isflo != OUT_REQ) && (isflo != OUT_POL) && (isflo != OUT_FIL))
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
	(*tmp)->addr.zone   = addr->zone;
	(*tmp)->addr.net    = addr->net;
	(*tmp)->addr.node   = addr->node;
	(*tmp)->addr.point  = addr->point;
	sprintf((*tmp)->addr.domain, "%s", addr->domain);
	if (nlent->addr.domain)
	    free(nlent->addr.domain);
	if (nlent->url)
	    free(nlent->url);
	nlent->url = NULL;
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

    if ((isflo == OUT_FLO) || (isflo == OUT_PKT) || (isflo == OUT_FIL)) 
	switch (flavor) {
	    case '?':	break;
	    case 'i':	(*tmp)->flavors |= F_IMM; break;
	    case 'o':	(*tmp)->flavors |= F_NORMAL; break;
	    case 'c':	(*tmp)->flavors |= F_CRASH; break;
	    case 'h':	(*tmp)->flavors |= F_HOLD; break;
	    default:	Syslog('?', "Unknown flavor: '%c'\n",flavor); break;
	}

    if (stat(fname,&st) != 0) {
	Syslog('?', "$Can't stat %s", fname);
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
		    if (*p == '~') 
			continue;
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
	    Syslog('?', "Can't open %s", fname);

    } else if (isflo == OUT_PKT) {
	(*tmp)->size += st.st_size;
	(*tmp)->flavors |= F_ISPKT;
    } else if (isflo == OUT_REQ) {
	(*tmp)->flavors |= F_FREQ;
    } else if (isflo == OUT_POL) {
	(*tmp)->flavors |= F_POLL;
    } else if (isflo == OUT_FIL) {
	(*tmp)->size += st.st_size;
	(*tmp)->flavors |= F_ISFIL;
    }

    return 0;
}


