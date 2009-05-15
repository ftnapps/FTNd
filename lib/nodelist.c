/*****************************************************************************
 * 
 * $Id: nodelist.c,v 1.37 2007/07/09 18:43:53 mbse Exp $
 * Purpose ...............: Read nodelists information
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
#include "mbselib.h"
#include "nodelist.h"


#define NULLDOMAIN "nulldomain"


static char		*k, *v;
static char		*nlpath = NULL;
static int		nlinitdone = FALSE;
static int		linecnt = 0;
static unsigned int	mypots = 0, myisdn = 0, mytcpip =0;


static int		getkwd(char**);
static int		getmdm(char**);
static int		getarr(char**);
static int		getdom(char**);
static int		getsrv(char**);



/*
 * Table to parse the ~/etc/nodelist.conf file
 */
static struct _keytab {
    char    *key;
    int	    (*prc)(char **);
    char**  dest;
} keytab[] = {
    {(char *)"online",	    getkwd,	    (char **)&nl_online},
    {(char *)"request",	    getkwd,	    (char **)&nl_request},
    {(char *)"reqbits",	    getkwd,	    (char **)&nl_reqbits},
    {(char *)"pots",	    getmdm,	    (char **)&nl_pots},
    {(char *)"isdn",	    getmdm,	    (char **)&nl_isdn},
    {(char *)"tcpip",	    getmdm,	    (char **)&nl_tcpip},
    {(char *)"search",	    getarr,	    (char **)&nl_search},
    {(char *)"dialer",	    getarr,	    (char **)&nl_dialer},
    {(char *)"ipprefix",    getarr,	    (char **)&nl_ipprefix},
    {(char *)"domsuffix",   getdom,	    (char **)&nl_domsuffix},
    {(char *)"service",	    getsrv,	    (char **)&nl_service},
    {NULL,		    NULL,	    NULL}
};



/*
 * Get a keyword, string, unsigned int
 */
static int getkwd(char **dest)
{
    char	    *p;
    unsigned int    tmp;
    nodelist_flag   **tmpm;
    
    for (p = v; *p && !isspace(*p); p++);
    if (*p)
	*p++ = '\0';
    while (*p && isspace(*p))
	p++;
    if (*p == '\0') {
	WriteError("%s(%s): less then two tokens", nlpath, linecnt);
	return MBERR_INIT_ERROR;
    }

    for (tmpm = (nodelist_flag**)dest; *tmpm; tmpm=&((*tmpm)->next));
    (*tmpm) = (nodelist_flag *) xmalloc(sizeof(nodelist_flag));
    (*tmpm)->next = NULL;
    (*tmpm)->name = xstrcpy(v);
    tmp = strtoul(p, NULL, 0);
    (*tmpm)->value = tmp;
    
    return 0;
}



/*
 * Get a keyword, string, unsigned int, unsigned int
 */
static int getmdm(char **dest)
{
    char            *p, *q;
    unsigned int    tmp1, tmp2;
    nodelist_modem  **tmpm;

    for (p = v; *p && !isspace(*p); p++);
    if (*p)
	*p++ = '\0';
    while (*p && isspace(*p))
	p++;
    if (*p == '\0') {
	WriteError("%s(%s): less then two tokens", nlpath, linecnt);
	return MBERR_INIT_ERROR;
    }

    for (q = p; *q && !isspace(*q); q++);
    if (*q)
	*q++ = '\0';
    while (*q && isspace(*q))
	q++;
    if (*q == '\0') {
	WriteError("%s(%s): less then three tokens", nlpath, linecnt);
	return MBERR_INIT_ERROR;
    }

    for (tmpm = (nodelist_modem**)dest; *tmpm; tmpm=&((*tmpm)->next));
    (*tmpm) = (nodelist_modem *) xmalloc(sizeof(nodelist_modem));
    (*tmpm)->next = NULL;
    (*tmpm)->name = xstrcpy(v);
    tmp1 = strtoul(p, NULL, 0);
    tmp2 = strtoul(q, NULL, 0);
    (*tmpm)->mask = tmp1;
    (*tmpm)->value = tmp2;

    return 0;
}



/*
 * Get a keyword, string array
 */
static int getarr(char **dest)
{
    char            *p;
    nodelist_array  **tmpm;

    for (p = v; *p && !isspace(*p); p++);
    if (*p)
	*p++ = '\0';

    for (tmpm = (nodelist_array**)dest; *tmpm; tmpm=&((*tmpm)->next));
    (*tmpm) = (nodelist_array *) xmalloc(sizeof(nodelist_array));
    (*tmpm)->next = NULL;
    (*tmpm)->name = xstrcpy(v);
    return 0;
}



/*
 * Get a keyword, unsigned short, string
 */
static int getdom(char **dest)
{
    char            *p;
    unsigned short  tmp;
    nodelist_domsuf **tmpm;
		    
    for (p = v; *p && !isspace(*p); p++);
    if (*p)
	*p++ = '\0';
    while (*p && isspace(*p))
	p++;
    if (*p == '\0') {
	WriteError("%s(%s): less then two tokens", nlpath, linecnt);
	return MBERR_INIT_ERROR;
    }

    for (tmpm = (nodelist_domsuf**)dest; *tmpm; tmpm=&((*tmpm)->next));
    (*tmpm) = (nodelist_domsuf *) xmalloc(sizeof(nodelist_domsuf));
    (*tmpm)->next = NULL;
    tmp = strtod(v, NULL);
    (*tmpm)->zone = tmp;
    (*tmpm)->name = xstrcpy(p);

    return 0;
}



/*
 * Get a keyword, string, string, unsigned int
 */
static int getsrv(char **dest)
{
    char		*p, *q;
    unsigned int	tmp;
    nodelist_service	**tmpm;

    for (p = v; *p && !isspace(*p); p++);
    if (*p)
	*p++ = '\0';
    while (*p && isspace(*p))
	p++;
    if (*p == '\0') {
	WriteError("%s(%s): less then two tokens", nlpath, linecnt);
	return MBERR_INIT_ERROR;
    }

    for (q = p; *q && !isspace(*q); q++);
    if (*q)
	*q++ = '\0';
    while (*q && isspace(*q))
	q++;
    if (*q == '\0') {
	WriteError("%s(%s): less then three tokens", nlpath, linecnt);
	return MBERR_INIT_ERROR;
    }

    for (tmpm = (nodelist_service**)dest; *tmpm; tmpm=&((*tmpm)->next));
    (*tmpm) = (nodelist_service *) xmalloc(sizeof(nodelist_service));
    (*tmpm)->next = NULL;
    (*tmpm)->flag = xstrcpy(v);
    (*tmpm)->service = xstrcpy(p);
    tmp = strtoul(q, NULL, 0);
    (*tmpm)->defport = (*tmpm)->tmpport = tmp;
    return 0;
}



void tidy_nl_flag(nodelist_flag **);
void tidy_nl_flag(nodelist_flag **fap)
{
    nodelist_flag   *tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->name)
	    free(tmp->name);
	free(tmp);
    }
    *fap = NULL;
}



void tidy_nl_modem(nodelist_modem **);
void tidy_nl_modem(nodelist_modem **fap)
{
    nodelist_modem  *tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->name)
	    free(tmp->name);
	free(tmp);
    }
    *fap = NULL;
}



void tidy_nl_array(nodelist_array **);
void tidy_nl_array(nodelist_array **fap)
{
    nodelist_array	*tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->name)
	    free(tmp->name);
	free(tmp);
    }
    *fap = NULL;
}



void tidy_nl_domsuf(nodelist_domsuf **);
void tidy_nl_domsuf(nodelist_domsuf **fap)
{
    nodelist_domsuf *tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->name)
	    free(tmp->name);
	free(tmp);
    }
    *fap = NULL;
}



void tidy_nl_service(nodelist_service **);
void tidy_nl_service(nodelist_service **fap)
{
    nodelist_service	*tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->flag)
	    free(tmp->flag);
	if (tmp->service)
	    free(tmp->service);
	free(tmp);
    }
    *fap = NULL;
}



/*
 * De-init nodelists, free all allocated memory
 */
void deinitnl(void)
{
    if (!nlinitdone)
	return;

    tidy_nl_flag(&nl_online);
    tidy_nl_flag(&nl_request);
    tidy_nl_flag(&nl_reqbits);
    tidy_nl_modem(&nl_pots);
    tidy_nl_modem(&nl_isdn);
    tidy_nl_modem(&nl_tcpip);
    tidy_nl_array(&nl_search);
    tidy_nl_array(&nl_dialer);
    tidy_nl_array(&nl_ipprefix);
    tidy_nl_domsuf(&nl_domsuffix);
    tidy_nl_service(&nl_service);

    nlinitdone = FALSE;
}



/*
 *  Init nodelists.
 */
int initnl(void)
{
    int		    i, rc = 0, Found;
    FILE	    *dbf, *fp;
    char	    *filexnm, buf[256], *p, *q;
    struct _nlfil   fdx;
    struct taskrec  TCFG;
    nodelist_modem  **tmpm;

    if (nlinitdone == TRUE)
	return 0;

    nl_online = NULL;
    nl_pots = NULL;
    nl_request = NULL;
    nl_reqbits = NULL;
    nl_isdn = NULL;
    nl_tcpip = NULL;
    nl_search = NULL;
    nl_domsuffix = NULL;
    nl_ipprefix = NULL;
    nl_dialer = NULL;
    nl_service = NULL;

    filexnm = xstrcpy(CFG.nodelists);
    filexnm = xstrcat(filexnm,(char *)"/node.files");
    nlpath = calloc(PATH_MAX, sizeof(char));

    /*
     * Check if all installed nodelists are present.
     */
    if ((dbf = fopen(filexnm, "r")) == NULL) {
	WriteError("$Can't open %s", filexnm);
	rc = MBERR_INIT_ERROR;
    } else {
	while (fread(&fdx, sizeof(fdx), 1, dbf) == 1) {
	    snprintf(nlpath, PATH_MAX -1, "%s/%s", CFG.nodelists, fdx.filename);
	    if ((fp = fopen(nlpath, "r")) == NULL) {
		WriteError("$Can't open %s", nlpath);
		rc = MBERR_INIT_ERROR;
	    } else {
		fclose(fp);
	    }
	}

	fclose(dbf);
    }
    free(filexnm);

    /*
     * Read and parse ~/etc/nodelist.conf
     */
    snprintf(nlpath, PATH_MAX -1, "%s/etc/nodelist.conf", getenv("MBSE_ROOT"));
    if ((dbf = fopen(nlpath, "r")) == NULL) {
	WriteError("$Can't open %s", nlpath);
	rc = MBERR_INIT_ERROR;
    } else {
	while (fgets(buf, sizeof(buf) -1, dbf)) {
	    linecnt++;
	    if (*(p = buf + strlen(buf) -1) != '\n') {
		WriteError("%s(%d): \"%s\" - line too long", nlpath, linecnt, buf);
		rc = MBERR_INIT_ERROR;
		break;
	    }
	    *p-- = '\0';
	    while ((p >= buf) && isspace(*p))
		*p-- = '\0';
	    k = buf;
	    while (*k && isspace(*k))
		k++;
	    p = k;
	    while (*p && !isspace(*p))
		p++;
	    *p++='\0';
	    v = p;
	    while (*v && isspace(*v)) 
		v++;

	    if ((*k == '\0') || (*k == '#')) {
		continue;
	    }
	    
	    for (i = 0; keytab[i].key; i++)
		if (strcasecmp(k,keytab[i].key) == 0)
		    break;

	    if (keytab[i].key == NULL) {
		WriteError("%s(%d): %s %s - unknown keyword", nlpath, linecnt, MBSE_SS(k), MBSE_SS(v));
		rc = MBERR_INIT_ERROR;
		break;
	    } else if ((keytab[i].prc(keytab[i].dest))) {
		rc = MBERR_INIT_ERROR;
		break;
	    }
	}
	fclose(dbf);
    }

    Found = FALSE;

    /*
     * Howmany TCP sessions are allowd
     */
    snprintf(nlpath, PATH_MAX -1, "%s/etc/task.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(nlpath, "r"))) {
	fread(&TCFG, sizeof(TCFG), 1, fp);
	fclose(fp);
    } else {
	TCFG.max_tcp = 0;
    }

    /*
     *  Read all our TCP/IP capabilities and set the global flag.
     */
    if (TCFG.max_tcp) {
	snprintf(buf, 256, "%s", CFG.IP_Flags);
	q = buf;
	for (p = q; p; p = q) {
	    if ((q = strchr(p, ',')))
		*q++ = '\0';
	    for (tmpm = &nl_tcpip; *tmpm; tmpm=&((*tmpm)->next))
		if (strncasecmp((*tmpm)->name, p, strlen((*tmpm)->name)) == 0)
		    mytcpip |= (*tmpm)->value;
	}
    }
 
    /*
     * Read the ports configuration for all pots and isdn lines.
     * All lines are ORed so we have a global and total lines
     * capability.
     */
    snprintf(nlpath, PATH_MAX -1, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(nlpath, "r"))) {
	fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fp);

	while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fp) == 1) {
	    if (((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) && (ttyinfo.available) && (ttyinfo.callout)) {

		snprintf(buf, 256, "%s", ttyinfo.flags);
		q = buf;
		for (p = q; p; p = q) {
		    if ((q = strchr(p, ',')))
			*q++ = '\0';
		    for (tmpm = &nl_pots; *tmpm; tmpm=&((*tmpm)->next))
			if (strcasecmp((*tmpm)->name, p) == 0)
			    mypots |= (*tmpm)->value;
		    for (tmpm = &nl_isdn; *tmpm; tmpm=&((*tmpm)->next))
			if (strcasecmp((*tmpm)->name, p) == 0)
			    myisdn |= (*tmpm)->value;
		}
	    }
	}
	fclose(fp);
    }

    free(nlpath);
    nlinitdone = TRUE;
    return rc;
}



int comp_node(struct _nlidx, struct _ixentry);
int comp_node(struct _nlidx fap1, struct _ixentry fap2)
{
    if (fap1.zone != fap2.zone)
	return (fap1.zone - fap2.zone);
    else if (fap1.net != fap2.net)
	return (fap1.net - fap2.net);
    else if (fap1.node != fap2.node)
	return (fap1.node - fap2.node);
    else
	return (fap1.point - fap2.point);
}



node *getnlent(faddr *addr)
{
    FILE		    *fp, *np;
    static node		    nodebuf;
    static char		    buf[MAXNLLINELEN], ebuf[MAXNLLINELEN], *p, *q, tbuf[256];
    struct _ixentry	    xaddr;
    int			    i, Found = FALSE, ixflag, stdflag, ndrecord = FALSE;
    char		    *mydomain, *path, *r;
    struct _nlfil	    fdx;
    struct _nlidx	    ndx;
    int			    lowest, highest, current;
    struct _nodeshdr	    ndhdr;
    static struct _nodes    nd;
    nodelist_modem	    **tmpm;
    nodelist_flag	    **tmpf;
    nodelist_service	    **tmps;
    nodelist_array	    **tmpa, **tmpaa;
    nodelist_domsuf	    **tmpd;
    unsigned int	    tport = 0;
    
    Syslog('n', "getnlent: %s", ascfnode(addr,0xff));

    mydomain = xstrcpy(CFG.aka[0].domain);
    if (mydomain == NULL) 
	mydomain = (char *)NULLDOMAIN;

    nodebuf.addr.domain = NULL;
    nodebuf.addr.zone   = 0;
    nodebuf.addr.net    = 0;
    nodebuf.addr.node   = 0;
    nodebuf.addr.point  = 0;
    nodebuf.addr.name   = NULL;
    nodebuf.upnet	= 0;
    nodebuf.upnode      = 0;
    nodebuf.region      = 0;
    nodebuf.type        = 0;
    nodebuf.pflag       = 0;
    nodebuf.name        = NULL;
    nodebuf.location    = NULL;
    nodebuf.sysop       = NULL;
    nodebuf.phone       = NULL;
    nodebuf.speed       = 0;
    nodebuf.mflags      = 0L;
    nodebuf.oflags      = 0L;
    nodebuf.xflags      = 0L;
    nodebuf.iflags      = 0L;
    nodebuf.dflags      = 0L;
    nodebuf.uflags[0]   = NULL;
    nodebuf.t1		= '\0';
    nodebuf.t2		= '\0';
    nodebuf.url		= NULL;
    
    if (addr == NULL) 
	goto retdummy;

    if (addr->zone == 0)
	addr->zone = CFG.aka[0].zone;
    xaddr.zone = addr->zone;
    nodebuf.addr.zone = addr->zone;
    xaddr.net = addr->net;
    nodebuf.addr.net = addr->net;
    xaddr.node = addr->node;
    nodebuf.addr.node = addr->node;
    xaddr.point = addr->point;
    nodebuf.addr.point = addr->point;

    if (initnl())
	goto retdummy;

    /*
     * Search domainname for the requested aka, should not fail.
     */
    path = calloc(PATH_MAX, sizeof(char));
    snprintf(path, PATH_MAX -1, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(path, "r"))) {
	fread(&fidonethdr, sizeof(fidonethdr), 1, fp);
	while (fread(&fidonet, fidonethdr.recsize, 1, fp) == 1) {
	    for (i = 0; i < 6; i++) {
		if (addr->zone == fidonet.zone[i]) {
		    nodebuf.addr.domain = xstrcpy(fidonet.domain);
		    Found = TRUE;
		    break;
		}
	    }
	    if (Found)
		break;
	}
	fclose(fp);
    }
    Found = FALSE;

    /*
     *  First, lookup node in index. NOTE -- NOT 5D YET
     */
    snprintf(path, PATH_MAX -1, "%s/%s", CFG.nodelists, "node.index");
    if ((fp = fopen(path, "r")) == NULL) {
	WriteError("$Can't open %s", path);
	free(path);
	goto retdummy;
    }

    fseek(fp, 0, SEEK_END);
    highest = ftell(fp) / sizeof(ndx);
    lowest = 0;

    while (TRUE) {
	current = ((highest - lowest) / 2) + lowest;
	fseek(fp, current * sizeof(ndx), SEEK_SET);
	if (fread(&ndx, sizeof(ndx), 1, fp) != 1)
	    break;

	if (comp_node(ndx, xaddr) == 0) {
	    Found = TRUE;
	    break;
	}
	if (comp_node(ndx, xaddr) < 0)
	    lowest = current;
	else
	    highest = current;
	if ((highest - lowest) <= 1)
	    break;
    }
    fclose(fp);

    if (!Found) {
	free(path);
	goto retdummy;
    }

    snprintf(path, PATH_MAX -1, "%s/%s", CFG.nodelists, "node.files");
    if ((fp = fopen(path, "r")) == NULL) {
	WriteError("$Can't open %s", path);
	free(path);
	goto retdummy;
    }

    /*
     *  Get filename from node.files
     */
    for (i = 0; i < (ndx.fileno +1); i++)
	fread(&fdx, sizeof(fdx), 1, fp);
    fclose(fp);

    /* CHECK DOMAIN HERE */

    /*
     *  Open and read in real nodelist
     */
    snprintf(path, PATH_MAX -1, "%s/%s", CFG.nodelists, fdx.filename);
    if ((fp = fopen(path, "r")) == NULL) {
	WriteError("$Can't open %s", path);
	free(path);
	goto retdummy;
    }

    if (fseek(fp, ndx.offset, SEEK_SET) != 0) {
	WriteError("$Seek failed for nodelist entry");
	fclose(fp);
	goto retdummy;
    }

    if (fgets(buf, sizeof(buf)-1, fp) == NULL) {
	WriteError("$fgets failed for nodelist entry");
	fclose(fp);
	goto retdummy;
    }
    Syslog('n', "getnlent: %s", buf);

    /*
     * Load noderecord if this node has one, if there is one then
     * nodelist overrides in this record will be used instead of 
     * the nodelist entries.
     */
    snprintf(path, PATH_MAX -1, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((np = fopen(path, "r")) != NULL) {
	fread(&ndhdr, sizeof(nodeshdr), 1, np);

	while (fread(&nd, ndhdr.recsize, 1, np) == 1) {
	    fseek(np, ndhdr.filegrp + ndhdr.mailgrp, SEEK_CUR);
	    for (i = 0; i < 20; i++) {
		if ((addr->zone == nd.Aka[i].zone) && (addr->net == nd.Aka[i].net) &&
		    (addr->node == nd.Aka[i].node) && (addr->point == nd.Aka[i].point)) {
		    ndrecord = TRUE;
		    break;
		}
	    }
	    if (ndrecord)
		break;
	}

	fclose(np);
    }
    free(path);
    
    nodebuf.type = ndx.type;
    nodebuf.pflag = ndx.pflag;

    /*
     * If an override exists on Hold and Down status,
     * clear these bits.
     */
    if (ndrecord && nd.IgnHold && (nodebuf.pflag & (NL_HOLD + NL_DOWN))) {
	nodebuf.pflag &= ~NL_DOWN;
	nodebuf.pflag &= ~NL_HOLD;
	Syslog('+', "getnlent: %s override node Down/Hold status", ascfnode(addr,0xff));
    }

    if (*(p = buf + strlen(buf) -1) == '\n') 
	*p = '\0';
    if (*(p = buf + strlen(buf) -1) == '\r') 
	*p = '\0';
    for (p = buf; *p; p++) 
	if (*p == '_') 
	    *p = ' ';

    p = buf;

    if ((q = strchr(p,','))) 
	*q++ = '\0';

    p = q;
    if (p == NULL) {
	fclose(fp);
	goto badsyntax;
    }
    
    if ((q=strchr(p,','))) 
	*q++ = '\0';
    p = q;
    if (p == NULL) {
	fclose(fp);
	goto badsyntax;
    }
    
    /*
     * Get system name
     */
    if ((q=strchr(p,','))) 
	*q++ = '\0';
    if (ndrecord && strlen(nd.Nl_hostname)) {
	Syslog('+', "getnlent: %s system name override with %s", ascfnode(addr,0xff), nd.Nl_hostname);
	nodebuf.name = nd.Nl_hostname;
    } else
	nodebuf.name = p;
    p = q;
    if (p == NULL) {
	fclose(fp);
	goto badsyntax;
    }
    
    /*
     * Get location
     */
    if ((q=strchr(p,','))) 
	*q++ = '\0';
    nodebuf.location = p;
    p = q;
    if (p == NULL) {
	fclose(fp);
	goto badsyntax;
    }
    
    /*
     * Get sysop name
     */
    if ((q=strchr(p,','))) 
	*q++ = '\0';
    nodebuf.sysop = p;
    p = q;
    if (p == NULL) {
	fclose(fp);
	goto badsyntax;
    }
    
    /*
     * Get phone number
     */
    if ((q=strchr(p,','))) 
	*q++ = '\0';
    if (strcasecmp(p, "-Unpublished-") == 0)
	nodebuf.phone = NULL;
    else
	nodebuf.phone = p;
    p = q;
    if (p == NULL) {
	fclose(fp);
	goto badsyntax;
    }
    
    /*
     * Get modem speed
     */
    if ((q=strchr(p,','))) 
	*q++ = '\0';
    nodebuf.speed = atoi(p);

    /*
     * Reset all possible overridden portnumbers to the default from nodelist.conf
     */
    for (tmps = &nl_service; *tmps; tmps=&((*tmps)->next))
	(*tmps)->tmpport = (*tmps)->defport;
    
    /*
     * Process the nodelist flags.
     */
    if (ndrecord && strlen(nd.Nl_flags)) {
	Syslog('+', "getnlent: %s flags override %s", ascfnode(addr,0xff), nd.Nl_flags);
	q = nd.Nl_flags;
    }
    ixflag = 0;
    stdflag = TRUE;
    for (p = q; p; p = q) {
	if ((q = strchr(p, ','))) 
	    *q++ = '\0';
	if ((strncasecmp(p, "U", 1) == 0) && (strlen(p) == 1)) {
	    stdflag = FALSE;
	} else {
	    /*
	     * Process authorized flags and user flags both as authorized.
	     */
	    for (tmpf = &nl_online; *tmpf; tmpf=&((*tmpf)->next))
		if (strcasecmp(p, (*tmpf)->name) == 0)
		    nodebuf.oflags |= (*tmpf)->value;
	    for (tmpm = &nl_pots; *tmpm; tmpm=&((*tmpm)->next))
		if (strcasecmp(p, (*tmpm)->name) == 0)
		    nodebuf.mflags |= (*tmpm)->value;
	    for (tmpm = &nl_isdn; *tmpm; tmpm=&((*tmpm)->next))
		if (strcasecmp(p, (*tmpm)->name) == 0)
		    nodebuf.dflags |= (*tmpm)->value;
	    for (tmpm = &nl_tcpip; *tmpm; tmpm=&((*tmpm)->next))
		if (strncasecmp(p, (*tmpm)->name, strlen((*tmpm)->name)) == 0) {
		    nodebuf.iflags |= (*tmpm)->value;
		    /*
		     * Parse the IP flag for a optional port number.
		     */
		    if ((r = strrchr(p, ':'))) {
			*r++;
			for (tmps = &nl_service; *tmps; tmps=&((*tmps)->next)) {
			    if (strncmp(p, (*tmps)->flag, 3) == 0) {
				/*
				 * Check for format IBN:70.66.52.252 because this is not
				 * a port number but a dotted IP quad with default port number.
				 */
				if ((strchr(r, '.') == NULL) && atoi(r)) {
				    (*tmps)->tmpport = atoi(r);
				    Syslog('n', "getnlent: port override %s %d to %d", 
					    (*tmpm)->name, (*tmps)->defport, (*tmps)->tmpport);
				}
			    }
			}
		    }
		}
	    for (tmpf = &nl_request; *tmpf; tmpf=&((*tmpf)->next))
		if (strcasecmp(p, (*tmpf)->name) == 0)
		    nodebuf.xflags = (*tmpf)->value;
	    if ((p[0] == 'T') && (strlen(p) == 3)) {
		/*
		 * System open hours flag
		 */
		nodebuf.t1 = p[1];
		nodebuf.t2 = p[2];
	    }
	    if (!stdflag) {
		if (ixflag < MAXUFLAGS) {
		    nodebuf.uflags[ixflag++] = p;
		    if (ixflag < MAXUFLAGS) 
			nodebuf.uflags[ixflag] = NULL;
		}
	    }
	}
    }

    /*
     * Now we read the next line from the nodelist and see if this
     * is a ESLF (Extended St. Louis Format) line. This is for test
     * and nothing is defined yet. For now, debug logging only.
     */
    while (TRUE) {
	if (fgets(ebuf, sizeof(ebuf)-1, fp) == NULL) {
//	    WriteError("$fgets failed for nodelist entry");	Errors are allowed here.
	    break;
	}
	/*
	 * Lines starting with ;E space are real errors.
	 */
	if (strncmp(ebuf, (char *)";E ", 3) == 0)
	    break;
	if (strncmp(ebuf, (char *)";E", 2))
	    break;
	Syslog('n', "ESLF: \"%s\"", printable(ebuf, 0));
    }

    /*
     * Build the connection URL
     *
     * If the node has some IP flags and we allow TCP, then search the best protocol.
     */
    if (nodebuf.iflags & mytcpip) {
	memset(&tbuf, 0, sizeof(tbuf));
	for (tmpm = &nl_tcpip; *tmpm; tmpm=&((*tmpm)->next)) {
	    if ((*tmpm)->mask & nodebuf.iflags) {
		for (tmps = &nl_service; *tmps; tmps=&((*tmps)->next)) {
		    if (strcmp((*tmps)->flag, (*tmpm)->name) == 0) {
			snprintf(tbuf, 256, "%s", (*tmps)->service);
			tport = (*tmps)->tmpport;
		    }
		}
	    }
	}

	/*
	 * The last setting is the best
	 */
	nodebuf.url = xstrcpy(tbuf);
	nodebuf.url = xstrcat(nodebuf.url, (char *)"://");

	/*
	 * Next, try to find out the FQDN for this node, we have a search
	 * preference list in the nodelist.conf file.
	 */
	memset(&tbuf, 0, sizeof(tbuf));
	if (ndrecord && strlen(nd.Nl_hostname)) {
	    Syslog('n', "getnlent: using override %s for FQDN", nd.Nl_hostname);
	    snprintf(tbuf, 256, nodebuf.name);
	    nodebuf.url = xstrcat(nodebuf.url, tbuf);
	} else {
	    for (tmpa = &nl_search; *tmpa; tmpa=&((*tmpa)->next)) {
		Syslog('n', "getnlent: search FQDN method %s", (*tmpa)->name);
		if (strcasecmp((*tmpa)->name, "field3") == 0) {
		    snprintf(tbuf, 256, nodebuf.name);
		    if (strchr(tbuf, '.')) {
			/*
			 * Okay, there are dots, this can be a FQDN or IP address.
			 */
			Syslog('n', "getnlent: using field3 \"%s\"", tbuf);
			nodebuf.url = xstrcat(nodebuf.url, tbuf);
			break;
		    } else {
			memset(&tbuf, 0, sizeof(tbuf));
		    }
		} else if (strcasecmp((*tmpa)->name, "field6") == 0) {
		    memset(&tbuf, 0, sizeof(tbuf));
		    for (tmpaa = &nl_ipprefix; *tmpaa; tmpaa=&((*tmpaa)->next)) {
			if (nodebuf.phone && strncmp(nodebuf.phone, (*tmpaa)->name, strlen((*tmpaa)->name)) == 0) {
			    Syslog('n', "getnlent: found %s prefix", (*tmpaa)->name);
			    snprintf(tbuf, 256, "%s", nodebuf.phone+strlen((*tmpaa)->name));
			    for (i = 0; i < strlen(tbuf); i++)
				if (tbuf[i] == '-')
				    tbuf[i] = '.';
			    Syslog('n', "getnlent: using field6 \"%s\"", tbuf);
			    nodebuf.url = xstrcat(nodebuf.url, tbuf);
			    break;
			}
		    }
		    if (strlen(tbuf))
			break;
		} else if (strcasecmp((*tmpa)->name, "field8") == 0) {
		    /*
		     * Read nodelist line again in another buffer, the original
		     * buffer is divided into pieces by all previous actions.
		     */
		    memset(&tbuf, 0, sizeof(tbuf));
		    if (fseek(fp, ndx.offset, SEEK_SET) != 0) {
			WriteError("$Seek failed for nodelist entry");
			fclose(fp);
			goto retdummy;
		    }
		    if (fgets(ebuf, sizeof(ebuf)-1, fp) == NULL) {
			WriteError("$fgets failed for nodelist entry");
			fclose(fp);
			goto retdummy;
		    }
		    if (*(p = ebuf + strlen(ebuf) -1) == '\n')
			*p = '\0';
		    if (*(p = ebuf + strlen(ebuf) -1) == '\r')
			*p = '\0';
		    p = ebuf;
		    /*
		     * Shift to field 8
		     */
		    for (i = 0; i < 7; i++) {
			if ((q = strchr(p,',')))
			    *q++ = '\0';
			p = q;
			if (p == NULL) {
			    fclose(fp);
			    goto badsyntax;
			}
		    }
		    for (p = q; p; p = q) {
			if ((q = strchr(p, ',')))
		            *q++ = '\0';
			if ((r = strchr(p, ':'))) {
			    r++;
			    /*
			     * If there is a user@domain then strip the userpart.
			     */
			    if (strchr(r, '@')) {
				r = strchr(r, '@');
				r++;
			    }
			    if (*r == '*') {
				Syslog('n', "getnlent: possible default domain marking \"%s\"", MBSE_SS(r));
				for (tmpd = &nl_domsuffix; *tmpd; tmpd=&((*tmpd)->next)) {
				    if ((*tmpd)->zone == nodebuf.addr.zone) {
					if (*r++ == '\0')
					    snprintf(tbuf, 256, "f%d.n%d.z%d.%s.%s", nodebuf.addr.node, nodebuf.addr.net,
						    nodebuf.addr.zone, nodebuf.addr.domain, (*tmpd)->name);
					else
					    snprintf(tbuf, 256, "f%d.n%d.z%d.%s.%s%s", nodebuf.addr.node, nodebuf.addr.net,
						    nodebuf.addr.zone, nodebuf.addr.domain, (*tmpd)->name, r);
					Syslog('n', "getnlent: will try default domain \"%s\"", tbuf);
					nodebuf.url = xstrcat(nodebuf.url, tbuf);
					break;
				    }
				}
				if (strlen(tbuf))
				    break;
				Syslog('n', "getnlent: no matching default domain found for zone %d", nodebuf.addr.zone);
			    }
			    if (strchr(r, '.')) {
				Syslog('n', "getnlent: found a FQDN \"%s\"", MBSE_SS(r));
				snprintf(tbuf, 256, "%s", r);
				nodebuf.url = xstrcat(nodebuf.url, tbuf);
				break;
			    }
			}
		    }
		    if (strlen(tbuf))
			break;
		    memset(&tbuf, 0, sizeof(tbuf));
		} else if (strcasecmp((*tmpa)->name, "defdomain") == 0) {
		    Syslog('n', "getnlent: trying default domain");
		    if (nodebuf.addr.domain) {
			for (tmpd = &nl_domsuffix; *tmpd; tmpd=&((*tmpd)->next)) {
			    if ((*tmpd)->zone == nodebuf.addr.zone) {
				snprintf(tbuf, 256, "f%d.n%d.z%d.%s.%s", nodebuf.addr.node, nodebuf.addr.net,
					nodebuf.addr.zone, nodebuf.addr.domain, (*tmpd)->name);
				Syslog('n', "getnlent: will try default domain \"%s\"", tbuf);
				nodebuf.url = xstrcat(nodebuf.url, tbuf);
				break;
			    }
			}
		    } else {
			Syslog('+', "getnlent: no domain name for zone %d, check setup", nodebuf.addr.zone);
		    }
		    if (strlen(tbuf))
			break;
		    Syslog('n', "getnlent: no matching default domain found for zone %d", nodebuf.addr.zone);
		    memset(&tbuf, 0, sizeof(tbuf));
		}
	    }
	}

	if (strlen(tbuf) == 0) {
	    Syslog('+', "getnlent: no FQDN found, cannot call");
	    if (nodebuf.url)
		free(nodebuf.url);
	    nodebuf.url = NULL;
	} else if (strchr(tbuf, ':') == NULL) {
	    /*
	     * No optional port number, add one from the default
	     * for this protocol.
	     */
	    snprintf(tbuf, 256, ":%u", tport);
	    nodebuf.url = xstrcat(nodebuf.url, tbuf);
	}

    } else if (nodebuf.dflags & myisdn) {
	nodebuf.url = xstrcpy((char *)"isdn://");
	nodebuf.url = xstrcat(nodebuf.url, nodebuf.phone);
    } else if (nodebuf.mflags & mypots) {
	nodebuf.url = xstrcpy((char *)"pots://");
	nodebuf.url = xstrcat(nodebuf.url, nodebuf.phone);
    }
    fclose(fp);

    nodebuf.addr.name = nodebuf.sysop;
    if (nodebuf.addr.domain)
	free(nodebuf.addr.domain);
    nodebuf.addr.domain = xstrcpy(fdx.domain);
    nodebuf.upnet  = ndx.upnet;
    nodebuf.upnode = ndx.upnode;
    nodebuf.region = ndx.region;
    if (addr->domain == NULL) 
	addr->domain = xstrcpy(nodebuf.addr.domain);

    /*
     * FSP-1033, the ICM (IP Continuous Mail) flag.
     */
    nodebuf.can_pots = (nodebuf.mflags || nodebuf.dflags)   ? TRUE : FALSE;
    nodebuf.can_ip   = (nodebuf.iflags)			    ? TRUE : FALSE;
    nodebuf.is_cm    = (nodebuf.oflags & 0x00000001)	    ? TRUE : FALSE;
    nodebuf.is_icm   = (nodebuf.oflags & 0x00000002)	    ? TRUE : FALSE; 
    /*
     * Now correct if node is ION with a CM flag instead of ICM flag.
     * This is the case the first months after approval of the ICM flag
     * and with nodes who won't change flags.
     */
    if (!nodebuf.can_pots && nodebuf.can_ip && nodebuf.is_cm && !nodebuf.is_icm) {
	Syslog('n', "getnlent: correct CM into ICM flag");
	nodebuf.is_cm  = FALSE;
	nodebuf.is_icm = TRUE;
    }

    Syslog('n', "getnlent: system  %s, %s", nodebuf.name, nodebuf.location);
    Syslog('n', "getnlent: sysop   %s, %s", nodebuf.sysop, nodebuf.phone);
    Syslog('n', "getnlent: URL     %s", printable(nodebuf.url, 0));
    Syslog('n', "getnlent: online  POTS %s CM %s, IP %s ICM %s", nodebuf.can_pots ?"Yes":"No", 
				nodebuf.is_cm ?"Yes":"No", nodebuf.can_ip ?"Yes":"No", nodebuf.is_icm ?"Yes":"No");
    free(mydomain);
    
    return &nodebuf;

badsyntax:
    WriteError("nodelist %d offset +%lu: bad syntax in line \"%s\"", ndx.fileno, (unsigned int)ndx.offset, buf);
    /* fallthrough */

retdummy:
    memset(&nodebuf, 0, sizeof(nodebuf));
    nodebuf.pflag = NL_DUMMY;
    nodebuf.name = (char *)"Unknown";
    nodebuf.location = (char *)"Nowhere";
    nodebuf.sysop = (char *)"Sysop";
    nodebuf.phone = NULL;
    nodebuf.speed = 2400;
    nodebuf.url = NULL;
    free(mydomain);

    return &nodebuf;
}


