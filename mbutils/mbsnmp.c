/*****************************************************************************
 *
 * $Id: mbsnmp.c,v 1.3 2008/02/23 21:42:17 mbse Exp $
 * Purpose ...............: SNMP passthru support.
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
#include "mbsnmp.h"



// mbsnmp [-n|-g] .1.3.6.1.4.1.2021.256.group.item
// mbsnmp <base oid> <request type> <requested oid>
// <base oid>	     Base oid for this tree.
// <request type>    -n for SNMP getNext, or -g for SNMP get.
// <request oid>     Requested OID
// outputs: "new oid\n" . "integer\n" . data . "\n"


/*	
 * group	command		reply
 * -----	-------		----------------------------------------------------------------------------------------------
 * 0		MGMS:0;		100:12,kbrcvd,kbsent,sessin,sessout,sess_sec,sess_unseq,sess_bad,ftsc,yoohoo,emsi,binkp,freqs;
 * 1		MGTN:0; 	100:3,in,out,bad;
 * 2		MGTI:0; 	100:3,in,out,bad;
 * 3		MGTE:0; 	100:4,in,out,bad,dupe;
 * 4		MGTR:0;		100:4,in,out,bad,dupe;
 * 5		MGTT:0; 	100:4,in,out,bad,dupe;
 * 6		MGTF:0; 	100:6,in,out,bad,dupe,magics,hatched;
 * 7		MGBB:0; 	100:9,sessions,minutes,posted,uploads,kbupload,downloads,kbdownload,chats,chatminutes;
 * 8		MGOB:0; 	100:1,size;
 */


void usage(void);
void die(int);



void usage(void)
{
    fprintf(stderr, "\nMBSNMP: MBSE BBS %s SNMP subagent\n", VERSION);
    fprintf(stderr, "        %s\n", COPYRIGHT);
    fprintf(stderr, "\nUsage:    mbsnmp <base oid> <request type> <request oid>\n\n");
    exit(MBERR_COMMANDLINE);
}



void die(int onsig)
{
    signal(onsig, SIG_IGN);

    if (onsig)
	Syslog('+', "Terminated on signal %d", onsig);

    ExitClient(onsig);
}



int main(int argc, char **argv)
{
    int		    i, getnext = FALSE, group = 0, sub = 0, params, val = 0;
    char	    *t1, *t2, *saveptr1 = NULL, *saveptr2 = NULL, *token1, *token2; 
    char	    *base_save, *req_save, *envptr = NULL, *req_oid, *base_oid, *resp, *type;
    struct passwd   *pw;

    /*
     * First, check the syntax and parameters.
     */
    if (argc != 4)
	usage();
    if (strcmp(argv[2],"-n") && strcmp(argv[2],"-g"))
	usage();
    base_oid = argv[1];
    req_oid  = argv[3];
    getnext  = strcmp(argv[2], "-g");
    if (strncmp(base_oid, req_oid, strlen(base_oid)))
	usage();

    /*
     * The next trick is to supply a fake environment variable
     * MBSE_ROOT because most likely we are started from snmpd.
     * This will setup the variable so InitConfig() will work.
     * The /etc/passwd must point to the correct homedirectory.
     */
    pw = getpwnam((char *)"mbse");
    if (getenv("MBSE_ROOT") == NULL) {
	envptr = xstrcpy((char *)"MBSE_ROOT=");
	envptr = xstrcat(envptr, pw->pw_dir);
	putenv(envptr);
    }

    InitConfig();

    /*
     * Catch or ignore signals
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbsnmp", CFG.location, CFG.logfile, 
		CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    base_save = xstrcpy(base_oid);
    req_save  = xstrcpy(req_oid);
    for (i = 1, t1 = base_oid, t2 = req_oid; ; i++, t1 = NULL, t2 = NULL) {
	token1 = strtok_r(t1, ".", &saveptr1);
	token2 = strtok_r(t2, ".", &saveptr2);
	if ((token1 == NULL) || (token2 == NULL))
	    break;
    }

    /*
     * Complete the requested OID if needed.
     */
    if (token2) {
	group = atoi(token2);
	t2 = NULL;
	token2 = strtok_r(t2, ".", &saveptr2);
	if (token2) {
	    sub = atoi(token2);
	}
    }

    if ((group < 0) || (group > 8)) {
	die(MBERR_OK);
    }

    switch (group) {
	case 0:	resp = SockR("MGMS:0;"); break;
	case 1: resp = SockR("MGTN:0;"); break;
	case 2: resp = SockR("MGTI:0;"); break;
	case 3: resp = SockR("MGTE:0;"); break;
	case 4: resp = SockR("MGTR:0;"); break;
	case 5: resp = SockR("MGTT:0;"); break;
	case 6: resp = SockR("MGTF:0;"); break;
	case 7: resp = SockR("MGBB:0;"); break;
	case 8: resp = SockR("MGOB:0;"); break;
	default: resp = (char *)"200:0;";
    }

    t1 = strtok(resp, ":");
    t1 = strtok(NULL, ",");
    params = atoi(t1);

    for (i = 0; i < params; i++) {
	t1 = strtok(NULL, ",;");
	val = atoi(t1);
	if (i == sub)
	    break;
    }

    /*
     * The value type defaults to counter.
     */
    type = (char *)"counter";
    if ((group == 8) && (sub == 0))
	type = (char *)"gauge";

    if ((group < 9) && (sub < params)) {
	if (getnext) {
	    sub++;
	    if ((sub >= params) && (group < 8)) {
		sub = 0;
		group++;
	    }
	}
    	printf("%s.%d.%d\n", base_save, group, sub);
    	printf("%s\n", type);
    	printf("%d\n", val);
    }

    die(MBERR_OK);
    return 0;
}



