/*****************************************************************************
 *
 * $Id: bounce.c,v 1.11 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: Bounce Netmail
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "../lib/mbsedb.h"
#include "sendmail.h"
#include "postnetmail.h"
#include "ping.h"



/*
 * External declarations
 */
extern	int	do_quiet;



/*
 * Global variables
 */
extern	int	net_in;			/* Netmails received		    */
extern	int	net_out;		/* Netmails forwarded		    */
extern	int	net_bad;		/* Bad netmails (tracking errors    */
extern	char	*msgid;			/* Original message id		    */



int Bounce(faddr *f, faddr *t, FILE *fp, char *reason)
{
    int	    rc = 0, count = 0;
    char    *Buf;
    FILE    *np;
    time_t  Now;
    faddr   *from;

    Now = time(NULL);
    if (SearchFidonet(f->zone))
	f->domain = xstrcpy(fidonet.domain);

    Syslog('+', "Bounce msg from %s", ascfnode(f, 0xff));
    Buf = calloc(MAX_LINE_LENGTH +1, sizeof(char));
    rewind(fp);

    np = tmpfile();
    from = bestaka_s(f);
    from->zone  = t->zone;
    from->net   = t->net;
    from->node  = t->node;
    from->point = t->point;
    from->name  = xstrcpy((char *)"Postmaster");

    if (f->point)
	fprintf(np, "\001TOPT %d\r", f->point);
    if (from->point)
	fprintf(np, "\001FMPT %d\r", from->point);
    fprintf(np, "\001INTL %d:%d/%d %d:%d/%d\r", f->zone, f->net, f->node, from->zone, from->net, from->node);

    /*
     * Add MSGID, REPLY and PID
     */
    fprintf(np, "\001MSGID: %s %08x\r", ascfnode(from, 0x1f), sequencer());
    while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {
	Striplf(Buf);
	if (strncmp(Buf, "\001MSGID:", 7) == 0) {
	    fprintf(np, "\001REPLY:%s\r", Buf+7);
	}
    }
    fprintf(np, "\001PID: MBSE-FIDO %s (%s-%s)\r", VERSION, OsName(), OsCPU());
    fprintf(np, "\001TZUTC: %s\r", gmtoffset(Now));

    fprintf(np, "     Dear %s\r\r", MBSE_SS(f->name));
    fprintf(np, "Your message could not be delevered, reason: %s\r\r", reason);
    fprintf(np, "Here are the first lines of the original message from you:\r\r");
    fprintf(np, "======================================================================\r");

    rewind(fp);
    while ((fgets(Buf, MAX_LINE_LENGTH, fp)) != NULL) {
	Striplf(Buf);
	if (Buf[0] == '\001') {
	    fprintf(np, "^a");
	    fwrite(Buf + 1, strlen(Buf) -1, 1, np);
	} else {
	    fwrite(Buf, strlen(Buf), 1, np);
	}
	fputc('\r', np);
	count++;
	if (count == 50)
	    break;
    }
    fprintf(np, "======================================================================\r");
    if (count == 50) {
	fprintf(np, "\rOnly the first 50 lines are displayed\r");
    }

    fprintf(np, "\rWith regards, %s\r\r", CFG.sysop_name);
    fprintf(np, "%s\r", TearLine());
    Now = time(NULL) - (gmt_offset((time_t)0) * 60);
    rc = postnetmail(np, from, f, NULL, (char *)"Bounced message", Now, 0x0000, FALSE, from->zone, f->zone);
    tidy_faddr(from);

    fclose(np);

    free(Buf);
    return rc;
}


