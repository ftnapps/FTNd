/*****************************************************************************
 *
 * File ..................: mbmail/mbmail.c
 * Purpose ...............: MBSE BBS Mail Gate
 * Last modification date : 28-May-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbftn.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbmsgs.h"
#include "../lib/dbuser.h"
#include "hash.h"
#include "mkftnhdr.h"
#include "message.h"


/* ### Modified by P.Saratxaga on 9 Aug 1995 ###
 * - added a line so if fmsg->to->name is greater than MAXNAME (35),
 *   and "@" or "%" or "!" is in it; then UUCP is used instead. So we can
 *   send messages to old style fido->email gates (that is, using another 
 *   soft than mbmail ;-) ) with long adresses.
 */

extern int	most_debug;
extern int	addrerror;
extern int	defaultrfcchar;
extern int	defaultftnchar;
extern int	do_quiet;		/* Quiet flag			*/
extern int	e_pid;			/* Pid of child process		*/
extern int	show_log;		/* Show logging on screen	*/
time_t		t_start;		/* Start time			*/
time_t		t_end;			/* End time			*/
extern int	usetmp;
int		pgpsigned;
extern char	*configname;
extern char	passwd[];
int             net_in = 0, net_imp = 0, net_out = 0, net_bad = 0;
int		dirtyoutcode = CHRS_NOTSET;


void ProgName(void)
{
	if (do_quiet)
		return;

	colour(15, 0);
	printf("\nMBMAIL: MBSE BBS %s - Internet Fidonet mail gate\n", VERSION);
	colour(14, 0);
	printf("	%s\n", Copyright);
}



void Help(void)
{
	do_quiet = FALSE;
	ProgName();

	colour(11, 0);
	printf("\nUsage: mbmail -r<addr> -g<grade> <recip> ...\n\n");
	colour(9, 0);
	printf("	Commands are:\n\n");
	colour(3, 0);
	printf("	-r<addr>	address to route packet\n");
	printf("	-g<grade>	[ n | c | h ] \"flavor\" of packet\n");
	printf("	-c<charset>	force the given charset\n");
	printf("	<recip>		list of receipient addresses\n");
	colour(7, 0);
	ExitClient(0);
}



void die(int onsig)
{
	/*
	 * First check if there is a child running, if so, kill it.
	 */
	if (e_pid) {
		if ((kill(e_pid, SIGTERM)) == 0)
			Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
		else {
			if ((kill(e_pid, SIGKILL)) == 0)
				Syslog('+', "SIGKILL to pid %d succeeded", e_pid);
			else
				WriteError("Failed to kill pid %d", e_pid);
		}

		/*
		 * In case the child had the tty in raw mode, reset the tty.
		 */
		system("stty sane");
	}

	signal(onsig, SIG_IGN);

	if (!do_quiet) {
		show_log = TRUE;
		colour(3, 0);
	}

	if (onsig) {
		if (onsig <= NSIG)
			WriteError("$Terminated on signal %d (%s)", onsig, SigName[onsig]);
		else
			WriteError("Terminated with error %d", onsig);
	}

	Syslog('+', "Msgs in [%4d] imp [%4d] out [%4d] bad [%4d]", net_in, net_imp, net_out, net_bad);

        if (net_out)
                CreateSema((char *)"scanout");


	time(&t_end);
	Syslog(' ', "MBMAIL finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet)
		colour(7, 0);
	ExitClient(onsig);
}



int main(int argc, char *argv[])
{
	char		*cmd, *envptr;
	struct passwd	*pw;
	int		i, c, rc;
	char		*routec = NULL;
	faddr		*route = NULL;
	char		*p;
	char		buf[BUFSIZ];
	FILE		*fp;
	fa_list		**envrecip, *envrecip_start = NULL;
	int		envrecip_count = 0;
	rfcmsg		*msg=NULL;
	ftnmsg		*fmsg=NULL;
	faddr		*taddr;
	char		cflavor = '\0', flavor;
	fa_list		*sbl = NULL;
	int		incode, outcode;

#ifdef MEMWATCH
	mwInit();
#endif

        /*
         * The next trick is to supply a fake environment variable
         * MBSE_ROOT in case we are started from the MTA.
	 * Some MTA's can't change uid to mbse, so if that's the
	 * case we will try it ourself.
         * This will setup the variable so InitConfig() will work.
         * The /etc/passwd must point to the correct homedirectory.
         */
        pw = getpwuid(getuid());
        if (getenv("MBSE_ROOT") == NULL) {
		pw = getpwuid(getuid());
		if (strcmp(pw->pw_name, "mbse")) {
			/*
			 *  We are not running as user mbse.
			 */
			pw = getpwnam("mbse");
			if (setuid(pw->pw_uid)) {
				printf("Fatal error: can't set uid to user mbse\n");
			}
                }
                envptr = xstrcpy((char *)"MBSE_ROOT=");
                envptr = xstrcat(envptr, pw->pw_dir);
                putenv(envptr);
        }

	do_quiet = TRUE;
	InitConfig();
	InitFidonet();
	InitNode();
	InitMsgs();
	InitUser();
	TermInit(1);
	time(&t_start);
	umask(002);

	/*
	 * Catch all the signals we can, and ignore the rest.
	 */
	for(i = 0; i < NSIG; i++) {

		if ((i == SIGINT) || (i == SIGBUS) || (i == SIGILL) ||
		    (i == SIGSEGV) || (i == SIGTERM) || (i == SIGKILL))
			signal(i, (void (*))die);
		else
			signal(i, SIG_IGN);
	}

	if (argc < 2)
		Help();

	cmd = xstrcpy((char *)"Cmd line: mbmail");

	while ((c = getopt(argc, argv, "g:r:c:h")) != -1) {
		cmd = xstrcat(cmd, (char *)" - ");
		cmd[strlen(cmd) -1] = c;
		switch (c) {
			case 'h':	Help();
			case 'g':	if (optarg && ((*optarg == 'n') || (*optarg == 'c') || (*optarg == 'h'))) {
						cflavor = *optarg;
						cmd = xstrcat(cmd, optarg);
					} else {
						Help(); 
					}
					break;
			case 'r':	routec = optarg; cmd = xstrcat(cmd, optarg); break;
			case 'c':	dirtyoutcode = readchrs(optarg);
					cmd = xstrcat(cmd, optarg);
					if (dirtyoutcode == CHRS_NOTSET)
						dirtyoutcode = getcode(optarg);
					break;
			default:	Help();
		}
	}

	ProgName();
	InitClient(pw->pw_name, (char *)"mbmail", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);

	Syslog(' ', " ");
	Syslog(' ', "MBMAIL v%s", VERSION);
	most_debug = TRUE;

	if (!diskfree(CFG.freespace))
		die(101);


	if (cflavor == 'n') 
		cflavor='o';

	if ((routec) && ((route=parsefaddr(routec)) == NULL))
		WriteError("unparsable route address \"%s\" (%s)", MBSE_SS(routec),addrerrstr(addrerror));

	if ((p=strrchr(argv[0],'/'))) 
		p++;
	else 
		p=argv[0];

	envrecip = &envrecip_start;
	while (argv[optind]) {
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[optind]);
		if ((taddr = parsefaddr(argv[optind++]))) {
			(*envrecip)=(fa_list*)malloc(sizeof(fa_list));
			(*envrecip)->next=NULL;
			(*envrecip)->addr=taddr;
			envrecip=&((*envrecip)->next);
			envrecip_count++;
		} else 
			WriteError("unparsable recipient \"%s\" (%s), ignored", argv[optind-1], addrerrstr(addrerror));
	}
	Syslog(' ', cmd);
	free(cmd);

	if (!envrecip_count) {
		WriteError("No valid receipients specified, aborting");
		die(105);
	}

	for (envrecip = &envrecip_start; *envrecip; envrecip = &((*envrecip)->next))
		Syslog('+', "envrecip: %s",ascfnode((*envrecip)->addr,0x7f));
	Syslog('+', "route: %s",ascfnode(route,0x1f));

	umask(066); /* packets may contain confidential information */

	while (!feof(stdin)) {
		usetmp = 0;
		tidyrfc(msg);
		msg = parsrfc(stdin);

		incode = outcode = CHRS_NOTSET;
		pgpsigned = 0;

		p = hdr((char *)"Content-Type",msg);
		if (p) 
			incode = readcharset(p);
		if (incode == CHRS_NOTSET) {
                	p = hdr((char *)"X-FTN-CHRS",msg);
			if (p == NULL) 
				p = hdr((char *)"X-FTN-CHARSET",msg);
			if (p == NULL) 
				p = hdr((char *)"X-FTN-CODEPAGE",msg);
			if (p) 
				incode = readchrs(p);
			if ((p = hdr((char *)"Message-ID",msg)) && (chkftnmsgid(p)))
				incode = defaultftnchar;
			else 
				incode = defaultrfcchar;
		}
		if ((p = hdr((char *)"Content-Type",msg)) && ((strcasestr(p, (char *)"multipart/signed")) ||
			 (strcasestr(p,(char *)"application/pgp")))) { 
			pgpsigned = 1; 
			outcode = incode; 
		} else if ((p=hdr((char *)"X-FTN-ORIGCHRS",msg))) 
			outcode = readchrs(p);
		else if (dirtyoutcode != CHRS_NOTSET)
			outcode = dirtyoutcode;
		else 
			outcode = getoutcode(incode);

		flavor = cflavor;

		if ((p = hdr((char *)"X-FTN-FLAGS",msg))) {
			if (!flavor) {
				if (flag_on((char *)"CRS",p)) 
					flavor = 'c';
				else if (flag_on((char *)"HLD",p)) 
					flavor = 'h';
				else 
					flavor = 'o';
			}
			if (flag_on((char *)"ATT",p))
				attach(*route, hdr((char *)"Subject",msg), 0, flavor);
			if (flag_on((char *)"TFS",p))
				attach(*route, hdr((char *)"Subject",msg), 1, flavor);
			if (flag_on((char *)"KFS",p))
				attach(*route, hdr((char *)"Subject",msg), 2, flavor);
		}
		if ((!flavor) && ((p = hdr((char *)"Priority",msg)) || 
		    (p = hdr((char *)"Precedence",msg)) || (p = hdr((char *)"X-Class",msg)))) {
			while (isspace(*p)) 
				p++;
			if ((strncasecmp(p,"fast",4) == 0) || (strncasecmp(p,"high",4) == 0) ||
			    (strncasecmp(p,"crash",5) == 0) || (strncasecmp(p,"airmail",5) == 0) ||
			    (strncasecmp(p,"special-delivery",5) == 0) || (strncasecmp(p,"first-class",5) == 0))
				flavor='c';
			else if ((strncasecmp(p,"slow",4) == 0) || (strncasecmp(p,"low",3) == 0) ||
			         (strncasecmp(p,"hold",4) == 0) || (strncasecmp(p,"news",4) == 0) ||
			         (strncasecmp(p,"bulk",4) == 0) || (strncasecmp(p,"junk",4) == 0))
      				flavor='h';
		}
		if (!flavor)
			flavor='o';

		if (envrecip_count > 1) {
			if ((fp = tmpfile()) == NULL) {
				WriteError("$Cannot open temporary file");
				die(102);
			}
			while (bgets(buf, sizeof(buf)-1, stdin))
				fputs(buf, fp);
			rewind(fp);
			usetmp = 1;
		} else {
			fp = stdin;
			usetmp = 0;
		}

		tidy_ftnmsg(fmsg);
		if ((fmsg = mkftnhdr(msg, incode, outcode, FALSE)) == NULL) {
			WriteError("Unable to create FTN headers from RFC ones, aborting");
			die(103);
		}

		for (envrecip = &envrecip_start; *envrecip; envrecip = &((*envrecip)->next)) {
			fmsg->to = (*envrecip)->addr;
			if ((!fmsg->to->name) || ((strlen(fmsg->to->name) > MAXNAME)
			    && ((strstr(fmsg->to->name,"@")) ||  (strstr(fmsg->to->name,"%")) 
			    ||  (strstr(fmsg->to->name,"!"))))) 
				fmsg->to->name = (char *)"UUCP";
			rc = putmessage(msg, fmsg, fp, route, flavor, &sbl, incode, outcode);
			if (rc == 1) {
				WriteError("Unable to put netmail message into the packet, aborting");
				die(104);
			}
			if (rc == 2) {
				WriteError("Unable to import netmail messages into the messabe base");
				die(105);
			}
			if (usetmp) 
				rewind(fp);
			fmsg->to = NULL;
		}
		net_in++;
		if (usetmp) 
			fclose(fp);
	}

	closepkt();
	die(0);
	return 0;
}

