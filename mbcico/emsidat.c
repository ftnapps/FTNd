/*****************************************************************************
 *
 * $Id: emsidat.c,v 1.26 2007/11/25 15:49:46 mbse Exp $
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
#include "../lib/nodelist.h"
#include "../lib/mbsedb.h"
#include "emsi.h"
#include "session.h"
#include "lutil.h"
#include "config.h"
#include "emsidat.h"
#include "filetime.h"


extern int	Loaded;
extern int	mypid;
extern int	emsi_akas;



/*
 * Encode EMSI string. If called with a NULL pointer memory is freed.
 */
char *emsiencode(char *s)
{
    char	Base16Code[]="0123456789ABCDEF";
    static char	*buf;
    char	*p, *q;

    if (buf)
	free(buf);
    buf = NULL;
    if (s == NULL)
	return NULL;
    
    if ((buf = malloc(2 * strlen(s) + 1 * sizeof(char))) == NULL) {
	Syslog('+', "emsiencode:out of memory:string too long:\"%s\"", s);
	return s;
    }
    for (p = s, q = buf; *p != '\0';) {
	switch (*p) {
	    case '\\':      *q++ = '\\'; *q++ = *p++; break;
	    case '[':
	    case ']':
	    case '{':
	    case '}':       *q++ = '\\';
			    *q++ = Base16Code[(*p >> 4) & 0x0f];
			    *q++ = Base16Code[*p & 0x0f];
			    p++; break;
	    default:        *q++ = *p++; break;
	}
    }
    *q = '\0';

    return buf;
}



char *mkemsidat(int caller)
{
    time_t  tt;
    char    cbuf[16], *p;
    faddr   *primary;
    int	    i;

    p = xstrcpy((char *)"EMSI_DAT0000{EMSI}{");

    primary = bestaka_s(remote->addr);
    p = xstrcat(p, ascfnode(primary, 0x1f));

    for (i = 0; i < 40; i++)
	if ((CFG.aka[i].zone) && (CFG.akavalid[i]) &&
		    ((CFG.aka[i].zone != primary->zone) || (CFG.aka[i].net  != primary->net) ||
		     (CFG.aka[i].node != primary->node) || (CFG.aka[i].point!= primary->point))) {
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, aka2str(CFG.aka[i]));
	}

    p=xstrcat(p,(char *)"}{");
    tidy_faddr(primary);

    if (emsi_local_password) 
	p=xstrcat(p,emsi_local_password);
    else if (strlen(nodes.Spasswd)) {
	    p = xstrcat(p, nodes.Spasswd);
    }

    if (emsi_local_opts & OPT_EII) {
	p = xstrcat(p, (char *)"}{");

	if (emsi_local_lcodes & LCODE_FNC) 
	    p = xstrcat(p, (char *)"FNC,");
	if (emsi_local_lcodes & LCODE_RMA) 
	    p = xstrcat(p, (char *)"RMA,");
	if (emsi_local_lcodes & LCODE_RH1) 
	    p = xstrcat(p, (char *)"RH1,");

	if (emsi_local_lcodes & LCODE_PUA) 
	    p=xstrcat(p,(char *)"PUA,");
	else if (emsi_local_lcodes & LCODE_PUP) 
	    p=xstrcat(p,(char *)"PUP,");
	else if (emsi_local_lcodes & LCODE_NPU) 
	    p=xstrcat(p,(char *)"NPU,");
	if (emsi_local_lcodes & LCODE_HAT) 
	    p=xstrcat(p,(char *)"HAT,");
	if (emsi_local_lcodes & LCODE_HXT) 
	    p=xstrcat(p,(char *)"HXT,");
	if (emsi_local_lcodes & LCODE_HRQ) 
	    p=xstrcat(p,(char *)"HRQ,");
	if (*(p+strlen(p)-1) == ',') 
	    *(p+strlen(p)-1) = '}';
	else 
	    p=xstrcat(p,(char *)"}");
    } else {
	p=xstrcat(p,(char *)"}{8N1");
	if (emsi_local_lcodes & LCODE_RH1)
	    p = xstrcat(p, (char *)",RH1");
	if (caller) {
	    if (emsi_local_lcodes & LCODE_PUA) 
		p=xstrcat(p,(char *)",PUA");
	    else if (emsi_local_lcodes & LCODE_PUP) 
		p=xstrcat(p,(char *)",PUP");
	    else if (emsi_local_lcodes & LCODE_NPU) 
		p=xstrcat(p,(char *)",NPU");
	} else {
	    if (emsi_local_lcodes & LCODE_HAT) 
		p=xstrcat(p,(char *)",HAT");
	    if (emsi_local_lcodes & LCODE_HXT) 
		p=xstrcat(p,(char *)",HXT");
	    if (emsi_local_lcodes & LCODE_HRQ) 
		p=xstrcat(p,(char *)",HRQ");
	}

	p = xstrcat(p, (char *)"}");
    }

    p=xstrcat(p,(char *)"{");
    if (emsi_local_protos & PROT_TCP)
	p=xstrcat(p,(char *)"TCP,");
    if (emsi_local_protos & PROT_HYD)
	p=xstrcat(p,(char *)"HYD,");
    if (emsi_local_protos & PROT_JAN)
	p=xstrcat(p,(char *)"JAN,");
    if (emsi_local_protos & PROT_ZAP) 
	p=xstrcat(p,(char *)"ZAP,");
    if (emsi_local_protos & PROT_ZMO) 
	p=xstrcat(p,(char *)"ZMO,");
    if (emsi_local_protos & PROT_DZA);
	p=xstrcat(p,(char *)"DZA,");
    if (emsi_local_protos & PROT_KER) 
	p=xstrcat(p,(char *)"KER,");
    if (emsi_local_protos ==  0) 
	p=xstrcat(p,(char *)"NCP,");
    if (emsi_local_opts & OPT_NRQ) 
	p=xstrcat(p,(char *)"NRQ,");
    if (emsi_local_opts & OPT_ARC) 
	p=xstrcat(p,(char *)"ARC,");
    if (emsi_local_opts & OPT_XMA) 
	p=xstrcat(p,(char *)"XMA,");
    if (emsi_local_opts & OPT_FNC) 
	p=xstrcat(p,(char *)"FNC,");
    if (emsi_local_opts & OPT_CHT) 
	p=xstrcat(p,(char *)"CHT,");
    if (emsi_local_opts & OPT_SLK) 
	p=xstrcat(p,(char *)"SLK,");
    if (emsi_local_opts & OPT_EII) 
	p=xstrcat(p,(char *)"EII,");
    if (emsi_local_opts & OPT_DFB) 
	p=xstrcat(p,(char *)"DFB,");
    if (emsi_local_opts & OPT_FRQ) 
	p=xstrcat(p,(char *)"FRQ,");
    if (*(p+strlen(p)-1) == ',') 
	*(p+strlen(p)-1) = '}';
    else 
	p=xstrcat(p,(char *)"}");

    snprintf(cbuf,16,"{%X}",PRODCODE);
    p=xstrcat(p,cbuf);
    p=xstrcat(p,(char *)"{mbcico}{");
    p=xstrcat(p,(char *)VERSION);
    p=xstrcat(p,(char *)"}{");
    p=xstrcat(p,(char *)__DATE__);
    p=xstrcat(p,(char *)"}{IDENT}{[");
    p=xstrcat(p,name?emsiencode(name):(char *)"Unknown");
    p=xstrcat(p,(char *)"][");
    p=xstrcat(p,emsiencode(CFG.location));
    p=xstrcat(p,(char *)"][");
    p=xstrcat(p,emsiencode(CFG.sysop_name));
    p=xstrcat(p,(char *)"][");
    p=xstrcat(p,phone?emsiencode(phone):(char *)"-Unpublished-");
    p=xstrcat(p,(char *)"][");
    if ((CFG.IP_Speed) && (emsi_local_protos & PROT_TCP))
	snprintf(cbuf,16,"%u",CFG.IP_Speed);
    else 
	strcpy(cbuf,"9600");
    p=xstrcat(p,cbuf);
    p=xstrcat(p,(char *)"][");
    p=xstrcat(p,flags?emsiencode(flags):(char *)"");
    p=xstrcat(p,(char *)"]}{TRX#}{[");
    tt = time(NULL);
    snprintf(cbuf,16,"%08X", (unsigned int)mtime2sl(tt));
    p=xstrcat(p,cbuf);
    p=xstrcat(p,(char *)"]}{TZUTC}{[");
    p=xstrcat(p,gmtoffset(tt));
    p=xstrcat(p,(char *)"]}");

    snprintf(cbuf,16,"%04X",(unsigned int)strlen(p+12));
    memcpy(p+8,cbuf,4);
    emsiencode(NULL);   /* Free memory */
    return p;
}



char *sel_brace(char*);
char *sel_brace(char *s)
{
    static char *save;
    char	*p,*q;
    int		i;

    if (s == NULL) 
	s=save;
    for (;*s && (*s != '{');s++);
    if (*s == '\0') {
	save=s;
	return NULL;
    } else
	s++;

    for (p=s,q=s;*p;p++) 
	switch (*p) {
	    case '}':	if (*(p+1) == '}') 
			    *q++=*p++;
			else {
			    *q='\0';
			    save=p+1;
			    goto exit;
			}
			break;
	    case '\\':	if (*(p+1) == '\\') 
			    *q++=*p++;
			else {
			    sscanf(p+1,"%02x",&i);
			    *q++=i;
			    p+=2;
			}
			break;
	    default:	*q++=*p;
			break;
	}
exit:
    return s;
}



char *sel_bracket(char*);
char *sel_bracket(char *s)
{
    static char	*save;
    char	*p,*q;
    int		i;

    if (s == NULL) 
	s=save;
    for (;*s && (*s != '[');s++);
    if (*s == '\0') {
	save=s;
	return NULL;
    } else
	s++;

    for (p=s,q=s;*p;p++) 
	switch (*p) {
	    case ']':	if (*(p+1) == ']') 
			    *q++=*p++;
			else {
			    *q='\0';
			    save=p+1;
			    goto exit;
			}
			break;
	    case '\\':	if (*(p+1) == '\\') 
			    *q++=*p++;
			else {
			    sscanf(p+1,"%02x",&i);
			    *q++=i;
			    p+=2;
			}
			break;
	    default:	*q++=*p;
			break;
	}
exit:
    return s;
}



int scanemsidat(char *buf)
{
    fa_list **tmp, *tmpa;
    faddr   *fa;
    char    *p, *q, *mailer_prod, *mailer_name, *mailer_version, *mailer_serial;
    int	    dupe, ttt;

    p = sel_brace(buf);
    if (strcasecmp(p,"EMSI") != 0) {
	Syslog('?', "This can never occur. Got \"%s\" instead of \"EMSI\"",p);
	return 1;
    }
    p = sel_brace(NULL);

    /*
     * Clear remote address list, and build a new one from EMSI data
     */
    tidy_falist(&remote);
    remote = NULL;
    tmp = &remote;
    for (q = strtok(p," "); q; q = strtok(NULL," ")) {
	if ((fa = parsefnode(q))) {
	    dupe = FALSE;
	    for (tmpa = remote; tmpa; tmpa = tmpa->next) {
		if ((tmpa->addr->zone == fa->zone) && (tmpa->addr->net == fa->net) &&
		    (tmpa->addr->node == fa->node) && (tmpa->addr->point == fa->point) &&
		    (strcmp(tmpa->addr->domain, fa->domain) == 0)) {
		    dupe = TRUE;
		    Syslog('i', "Double address %s", ascfnode(tmpa->addr, 0x1f));
		    break;
		}
	    }
	    if (!dupe) {
		*tmp = (fa_list*)malloc(sizeof(fa_list));
		(*tmp)->next = NULL;
		(*tmp)->addr = fa;
		tmp = &((*tmp)->next);
	    }
	}
    }

    tmp = &remote;
    while (*tmp) {
	if (nodelock((*tmp)->addr, mypid)) {
	    Syslog('+', "address : %s is locked, removed from aka list",ascfnode((*tmp)->addr,0x1f));
	    tmpa=*tmp;
	    *tmp=(*tmp)->next;
	    free(tmpa);
	} else {
	    /*
	     * With the loaded flag we prevent removing the noderecord
	     * when the remote presents us an address we don't know about.
	     */
	    emsi_akas++;
	    Syslog('+', "address : %s",ascfnode((*tmp)->addr,0x1f));
	    if (!Loaded) {
		if (noderecord((*tmp)->addr))
		    Loaded = TRUE;
	    }
	    tmp = &((*tmp)->next);
	}
    }

    if (emsi_akas) {	/* Only if any aka's left */
	history.aka.zone  = remote->addr->zone;
	history.aka.net   = remote->addr->net;
	history.aka.node  = remote->addr->node;
	history.aka.point = remote->addr->point;
	snprintf(history.aka.domain, 13, "%s", printable(remote->addr->domain, 0));
    }

    if (emsi_remote_password) 
	free(emsi_remote_password);
    emsi_remote_password=xstrcpy(sel_brace(NULL));

    p=sel_brace(NULL);
    Syslog('+', "link    : %s", MBSE_SS(p));
    for (q=strtok(p,",");q;q=strtok(NULL,",")) {
	if (((q[0] >= '5') && (q[0] <= '8')) && ((toupper(q[1]) == 'N') || (toupper(q[1]) == 'O') ||
	    (toupper(q[1]) == 'E') || (toupper(q[1]) == 'S') || (toupper(q[1]) == 'M')) && ((q[2] == '1') || (q[2] == '2'))) {
		strncpy(emsi_remote_comm,q,3);
	}
	else if (strcasecmp(q,"PUA") == 0) emsi_remote_lcodes |= LCODE_PUA;
	else if (strcasecmp(q,"PUP") == 0) emsi_remote_lcodes |= LCODE_PUP;
	else if (strcasecmp(q,"NPU") == 0) emsi_remote_lcodes |= LCODE_NPU;
	else if (strcasecmp(q,"HAT") == 0) emsi_remote_lcodes |= LCODE_HAT;
	else if (strcasecmp(q,"HXT") == 0) emsi_remote_lcodes |= LCODE_HXT;
	else if (strcasecmp(q,"HRQ") == 0) emsi_remote_lcodes |= LCODE_HRQ;
	else if (strcasecmp(q,"FNC") == 0) emsi_remote_lcodes |= LCODE_FNC;
	else if (strcasecmp(q,"RMA") == 0) emsi_remote_lcodes |= LCODE_RMA;
	else if (strcasecmp(q,"RH1") == 0) emsi_remote_lcodes |= LCODE_RH1;
	else Syslog('+', "unrecognized EMSI link code: \"%s\"",q);
    }

    p=sel_brace(NULL);
    Syslog('+', "comp    : %s", p);
    for (q=strtok(p,",");q;q=strtok(NULL,",")) {
	if (strcasecmp(q,"DZA") == 0) emsi_remote_protos |= PROT_DZA;
	else if (strcasecmp(q,"ZAP") == 0) emsi_remote_protos |= PROT_ZAP;
	else if (strcasecmp(q,"ZMO") == 0) emsi_remote_protos |= PROT_ZMO;
	else if (strcasecmp(q,"JAN") == 0) emsi_remote_protos |= PROT_JAN;
	else if (strcasecmp(q,"HYD") == 0) emsi_remote_protos |= PROT_HYD;
	else if (strcasecmp(q,"KER") == 0) emsi_remote_protos |= PROT_KER;
	else if (strcasecmp(q,"TCP") == 0) emsi_remote_protos |= PROT_TCP;
	else if (strcasecmp(q,"NCP") == 0) emsi_remote_protos = 0;
	else if (strcasecmp(q,"NRQ") == 0) emsi_remote_opts |= OPT_NRQ;
	else if (strcasecmp(q,"ARC") == 0) emsi_remote_opts |= OPT_ARC;
	else if (strcasecmp(q,"XMA") == 0) emsi_remote_opts |= OPT_XMA;
	else if (strcasecmp(q,"FNC") == 0) emsi_remote_opts |= OPT_FNC;
	else if (strcasecmp(q,"CHT") == 0) emsi_remote_opts |= OPT_CHT;
	else if (strcasecmp(q,"SLK") == 0) emsi_remote_opts |= OPT_SLK;
	else if (strcasecmp(q,"EII") == 0) emsi_remote_opts |= OPT_EII;
	else if (strcasecmp(q,"DFB") == 0) emsi_remote_opts |= OPT_DFB;
	else if (strcasecmp(q,"FRQ") == 0) emsi_remote_opts |= OPT_FRQ;
	else if (strcasecmp(q,"BBS") == 0) Syslog('+', "remote has BBS activity now");
	else Syslog('+', "unrecognized EMSI proto/option code: \"%s\"",q);
    }
    if ((emsi_remote_opts & OPT_FNC) == 0) 
	remote_flags &= ~SESSION_FNC;

    mailer_prod=sel_brace(NULL);
    mailer_name=sel_brace(NULL);
    mailer_version=sel_brace(NULL);
    mailer_serial=sel_brace(NULL);
    Syslog('+', "uses    : %s [%s] version %s/%s", mailer_name, mailer_prod, mailer_version, mailer_serial);

    while ((p=sel_brace(NULL))) {
	if (strcasecmp(p,"IDENT") == 0) {
	    p=sel_brace(NULL);
	    Syslog('+', "system  : %s",(p=sel_bracket(p)));
	    strncpy(history.system_name, p, 35);
	    Syslog('+', "location: %s",(p=sel_bracket(NULL)));
	    strncpy(history.location, p, 35);
	    Syslog('+', "operator: %s",(p=sel_bracket(NULL)));
	    strncpy(history.sysop, p, 35);
	    if (remote && remote->addr)
		remote->addr->name=xstrcpy(p);
	    Syslog('+', "phone   : %s",sel_bracket(NULL));
	    Syslog('+', "baud    : %s",sel_bracket(NULL));
	    Syslog('+', "flags   : %s",sel_bracket(NULL));
	} else if (strcasecmp(p, "TZUTC") == 0) {
	    p = sel_brace(NULL);
	    p = sel_bracket(p);
	    if ((strlen(p) == 4) || (strlen(p) == 5))
		Syslog('+', "timezone: %s", p);
	    else
		Syslog('+', "TZUTC   : %s", p);
	} else if (strcasecmp(p,"TRX#") == 0) {
	    time_t tt, now;
	    char ctt[32];

	    now = time(NULL);
	    p=sel_brace(NULL);
	    p=sel_bracket(p);
	    if (sscanf(p,"%08x",&ttt) == 1) {
		tt = (time_t)ttt;
		strcpy(ctt,date(sl2mtime(tt)));
		Syslog('+', "time    : %s",ctt);
		Syslog('+', "tranx   : %08lX/%08lX [%ld]", now, sl2mtime(tt), now - sl2mtime(tt));
	    } else
		Syslog('+', "remote     TRX#: %s",p);
	} else if (strcasecmp(p, "TRAF") == 0) {
	    unsigned int tt, tt1;

	    p = sel_brace(NULL);
	    if (sscanf(p, "%08x %08x", &tt, &tt1) == 2) {
		Syslog('+', "netmail : %u byte(s)", tt);
		Syslog('+', "echomail: %u byte(s)", tt1);
	    } else {
		Syslog('+', "TRAF    : %s", p);
	    }
	} else if (strcasecmp(p, "MOH#") == 0) {
	    unsigned int tt;

	    p = sel_brace(NULL);
	    p = sel_bracket(p);
	    if (sscanf(p, "%08x", &tt) == 1)
		Syslog('+', "on hold : %u byte(s)", tt);
	    else
		Syslog('+', "MOH#    : %s", p);
	} else {
	    q=sel_brace(NULL);
	    Syslog('+', "extra   : \"%s\" value: \"%s\"",p,q);
	}
    }

    return 0;
}


