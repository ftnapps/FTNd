/*****************************************************************************
 *
 * $Id: packet.c,v 1.12 2005/08/28 13:34:43 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "users.h"
#include "mbsedb.h"



static FILE *pktfp=NULL;
static faddr pktroute = 
{
    NULL,0,0,0,0,NULL
};



FILE *openpkt(FILE *pkt, faddr *addr, char flavor, int session)
{
    off_t	    pos;
    struct flock    fl;
    struct stat	    st;
    char	    *Name;
    struct tm	    *ptm;
    time_t	    t;
    int		    i;
    faddr	    *bestaka;
    unsigned char   buffer[0x3a];
    char	    str[9];

    if (pkt == NULL) {
	if (pktfp) {
	    if (metric(addr,&pktroute) == 0) {
		if ((CFG.maxpktsize == 0L) || ((fstat(fileno(pktfp),&st) == 0) && (st.st_size < CFG.maxpktsize))) {
		    return pktfp;
		}
		closepkt();
	    } else {
		closepkt();
	    }
	}

	pktroute.zone   = addr->zone;
	pktroute.net    = addr->net;
	pktroute.node   = addr->node;
	pktroute.point  = addr->point;
	pktroute.domain = xstrcpy(addr->domain);
	pktroute.name   = NULL;
	Name = pktname(addr,flavor);
	mkdirs(Name, 0770);

	if ((pktfp = fopen(Name, "r+")) == NULL)
	    pktfp = fopen(Name,"w");
	if (pktfp == NULL) {
	    WriteError("$Unable to open packet %s",MBSE_SS(Name));
	    return NULL;
	}

	fl.l_type   = F_WRLCK;
	fl.l_whence = 0;
	fl.l_start  = 0L;
	fl.l_len    = 0L;
	if (fcntl(fileno(pktfp), F_SETLKW, &fl) < 0) {
	    WriteError("$Unable to lock packet %s", MBSE_SS(Name));
	    fclose(pktfp);
	    return NULL;
	}

	pkt = pktfp;
	pos = fseek(pkt, -2L, SEEK_END);
    }

    pos = ftell(pkt);
    if (pos <= 0L) {

	/*
	 * Write .PKT header, see FSC-0039 rev. 4
	 */
	memset(&buffer, 0, sizeof(buffer));
	t = time(NULL);
	ptm = localtime(&t);
	if (ptm->tm_sec > 59)
	    ptm->tm_sec = 59;

	bestaka = bestaka_s(addr);
	buffer[0x00] = (bestaka->node & 0x00ff);
        buffer[0x01] = (bestaka->node & 0xff00) >> 8;
        buffer[0x02] = (addr->node & 0x00ff);
        buffer[0x03] = (addr->node & 0xff00) >> 8;
        buffer[0x04] = ((ptm->tm_year + 1900) & 0x00ff);
        buffer[0x05] = ((ptm->tm_year + 1900) & 0xff00) >> 8;
        buffer[0x06] = ptm->tm_mon;
        buffer[0x08] = ptm->tm_mday;
        buffer[0x0a] = ptm->tm_hour;
        buffer[0x0c] = ptm->tm_min;
        buffer[0x0e] = ptm->tm_sec;
        buffer[0x12] = 2;
        buffer[0x14] = (bestaka->net & 0x00ff);
        buffer[0x15] = (bestaka->net & 0xff00) >> 8;
        buffer[0x16] = (addr->net & 0x00ff);
        buffer[0x17] = (addr->net & 0xff00) >> 8;
        buffer[0x18] = (PRODCODE & 0x00ff);
        buffer[0x19] = (VERSION_MAJOR & 0x00ff);

        memset(&str, 0, 8);
        if (session) {
	   if (noderecord(addr) && strlen(nodes.Spasswd))
	        snprintf(str, 9, "%s", nodes.Spasswd);
	} else {
	    if (noderecord(addr) && strlen(nodes.Epasswd))
	        snprintf(str, 9, "%s", nodes.Epasswd);
	}
	for (i = 0; i < 8; i++)
	    buffer[0x1a + i] = toupper(str[i]);	 /* FSC-0039 only talks about A-Z, 0-9, so force uppercase */

	buffer[0x22] = (bestaka->zone & 0x00ff);
        buffer[0x23] = (bestaka->zone & 0xff00) >> 8;
        buffer[0x24] = (addr->zone & 0x00ff);
        buffer[0x25] = (addr->zone & 0xff00) >> 8;
        buffer[0x29] = 1;
        buffer[0x2a] = (PRODCODE & 0xff00) >> 8;
        buffer[0x2b] = (VERSION_MINOR & 0x00ff);
        buffer[0x2c] = 1;
        buffer[0x2e] = buffer[0x22];
        buffer[0x2f] = buffer[0x23];
        buffer[0x30] = buffer[0x24];
        buffer[0x31] = buffer[0x25];
        buffer[0x32] = (bestaka->point & 0x00ff);
        buffer[0x33] = (bestaka->point & 0xff00) >> 8;
        buffer[0x34] = (addr->point & 0x00ff);
        buffer[0x35] = (addr->point & 0xff00) >> 8;
        buffer[0x36] = 'm';
        buffer[0x37] = 'b';
        buffer[0x38] = 's';
        buffer[0x39] = 'e';

        fseek(pkt, 0L, SEEK_SET);
        fwrite(buffer, 1, 0x3a, pkt);
    }

    return pkt;
}



void closepkt(void)
{
    unsigned char	buffer[2];

    memset(&buffer, 0, sizeof(buffer));

    if (pktfp) {
	fwrite(buffer, 1, 2, pktfp);
	fclose(pktfp);	/* close also discards lock */
    }
    pktfp = NULL;
    if (pktroute.domain) 
	free(pktroute.domain);
}


