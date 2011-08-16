/*****************************************************************************
 *
 * $Id: getheader.c,v 1.14 2006/03/20 12:36:21 mbse Exp $
 * Purpose ...............: Read fidonet .pkt header
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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


faddr	pktfrom;
char	pktpwd[9];


/*
 * Return codes:
 *  0 - All Seems Well
 *  1 - Invalid type (not 2 or 2+)
 *  2 - Read header error
 *  3 - Not for me
 *  4 - Password error
 *  5 - Unsecure session
 *
 *  If session is TRUE, the password is checked as being the session password,
 *  otherwise it is checked as the mail password.
 */
int getheader(faddr *f, faddr *t, FILE *pkt, char *pname, int session)
{
    unsigned char   buffer[0x3a];
    int		    i, capword, prodx, major, minor = 0, tome = FALSE;
    char	    *p, *prodn = NULL, *fa, *ta, buf[5];
    int		    year, month, day, hour, min, sec;

    f->domain = NULL;
    f->name   = NULL;
    t->domain = NULL;
    t->name   = NULL;

    /*
     * Read type 2+ packet header, see FSC-0039 version 4 and FTS-0001
     */
    if (fread(buffer, 1, 0x3a, pkt) != 0x3a) {
	WriteError("Could not read header (%s)", pname);
	return 2;
    }
    if ((buffer[0x12] + (buffer[0x13] << 8)) != 2) {
	WriteError("Not a type 2 packet (%s)", pname);
	return 1;
    }

    f->node = (buffer[0x01] << 8) + buffer[0x00];
    t->node = (buffer[0x03] << 8) + buffer[0x02];
    f->net  = (buffer[0x15] << 8) + buffer[0x14];
    t->net  = (buffer[0x17] << 8) + buffer[0x16];
    f->zone = (buffer[0x23] << 8) + buffer[0x22];
    t->zone = (buffer[0x25] << 8) + buffer[0x24];

    year    = (buffer[0x05] << 8) + buffer[0x04];
    /*
     * Check for Y2K bugs, if there are any this is not important,
     * it is just for logging!
     */
    if (year < 50)
	year = year + 2000;
    else if (year < 1900)
	year = year + 1900;
    month   = (buffer[0x07] << 8) + buffer[0x06] + 1;
    day     = (buffer[0x09] << 8) + buffer[0x08];
    hour    = (buffer[0x0b] << 8) + buffer[0x0a];
    min     = (buffer[0x0d] << 8) + buffer[0x0c];
    sec     = (buffer[0x0f] << 8) + buffer[0x0e];
    prodx   =  buffer[0x18];
    major   =  buffer[0x19];

    capword = (buffer[0x2d] << 8) + buffer[0x2c];
    if (capword != ((buffer[0x28] << 8) + buffer[0x29]))
	capword = 0;

    if (capword & 0x0001) {
	/*
	 * FSC-0039 packet type 2+
	 */
	prodx     = prodx + (buffer[0x2a] << 8);
	minor     = buffer[0x2b];
	f->zone   = buffer[0x2e] + (buffer[0x2f] << 8);
	t->zone   = buffer[0x30] + (buffer[0x31] << 8);
	f->point  = buffer[0x32] + (buffer[0x33] << 8);
	t->point  = buffer[0x34] + (buffer[0x35] << 8);
    } else {
	/*
	 * Stone age @%#$@
	 */
	f->zone   = buffer[0x22] + (buffer[0x23] << 8);
	t->zone   = buffer[0x24] + (buffer[0x25] << 8);
	if ((f->zone == 0) && (t->zone == 0)) {
	    /*
	     * No zone info, since the packet should be for us, guess the zone
	     * against our aka's from the setup using a 2d test.
	     */
	    for (i = 0; i < 40; i++) {
		if ((CFG.akavalid[i]) && (t->net  == CFG.aka[i].net) && (t->node == CFG.aka[i].node)) {
		    t->zone = CFG.aka[i].zone;
		    f->zone = CFG.aka[i].zone;
		    Syslog('!', "Warning, zone %d assumed", CFG.aka[i].zone);
		    break;
		}
	    }
	}
    }
    
    for (i = 0; i < 8; i++) 
	pktpwd[i] = buffer[0x1a + i];
    pktpwd[8]='\0';
    for (p = pktpwd + 7; (p >= pktpwd) && (*p == ' '); p--) *p='\0';
    if (pktpwd[0]) 
	f->name = pktpwd;

    /*
     * Fill in a default product code in case it doesn't exist
     */
    snprintf(buf, 5, "%04x", prodx);
    prodn = xstrcpy((char *)"Unknown 0x");
    prodn = xstrcat(prodn, buf);
    for (i = 0; ftscprod[i].name; i++)
	if (ftscprod[i].code == prodx) {
	    free(prodn);
	    prodn = xstrcpy(ftscprod[i].name);
	    break;
	}

    pktfrom.name   = NULL;
    pktfrom.domain = NULL;
    pktfrom.zone   = f->zone;
    pktfrom.net    = f->net;
    pktfrom.node   = f->node;
    if (capword & 0x0001) 
	pktfrom.point = f->point;
    else 
	pktfrom.point = 0;

    for (i = 0; i < 40; i++) {
	if ((CFG.akavalid[i]) && ((t->zone == 0) || (t->zone == CFG.aka[i].zone)) &&
	    (t->net  == CFG.aka[i].net) && (t->node == CFG.aka[i].node) &&
	    ((!(capword & 0x0001)) || (t->point == CFG.aka[i].point) || (t->point && !CFG.aka[i].point)))
	    tome = TRUE;
    }

    fa = xstrcpy(ascfnode(f, 0x1f));
    ta = xstrcpy(ascfnode(t, 0x1f));
    Syslog('+', "Packet   : %s type %s", pname, (capword & 0x0001) ? "2+":"stone-age");
    Syslog('+', "From     : %s to %s", fa, ta);
    Syslog('+', "Dated    : %02u-%02u-%u %02u:%02u:%02u", day, month, year, hour, min, sec);
    Syslog('+', "Program  : %s %d.%d", prodn, major, minor);
    free(ta);
    free(fa);

    if (capword & 0x0001) {
	buf[0] = buffer[0x36];
	buf[1] = buffer[0x37];
	buf[2] = buffer[0x38];
	buf[3] = buffer[0x39];
	buf[4] = '\0';
    }

    if (prodn)
	free(prodn);

    if (!tome)
	return 3;

    if (session) {
	/*
	 * FTS-0001 session setup mode.
	 */
	if (noderecord(f) && strlen(nodes.Spasswd)) {
	    if (strcasecmp(nodes.Spasswd, pktpwd) == 0) {
		return 0; /* Secure session */
	    } else {
		Syslog('!', "Password : got \"%s\", expected \"%s\"", pktpwd, nodes.Spasswd);
		return 4; /* Bad password */
	    }
	} else {
	    Syslog('+', "Node not in setup or no password set");
	    return 5; /* Unsecure session */
	}
    } else {
	/*
	 * Mail password check
	 */
	if (noderecord(f) && nodes.MailPwdCheck && strlen(nodes.Epasswd)) {
	    if (strcasecmp(nodes.Epasswd, pktpwd) == 0) {   
		return 0; /* Password Ok */
	    } else {
		Syslog('!', "Password : got \"%s\", expected \"%s\"", pktpwd, nodes.Epasswd);
		return 4; /* Bad password */
	    }
	} else {
	    return 0; /* Not checked, still Ok */
	}
    }

    return 0;
}


