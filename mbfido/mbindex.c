/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Nodelist Compiler
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
#include "../lib/mbsedb.h"


typedef struct _nl_list {
    struct _nl_list	*next;
    struct _nlidx	idx;
} nl_list;


#include "mbindex.h"


FILE		*ifp, *ufp, *ffp;
long		total = 0, entries = 0;
int		filenr = 0;
unsigned short	regio;
nl_list		*nll = NULL;


extern		int do_quiet;		/* Quiet flag			    */
extern		int show_log;		/* Show logging on screen	    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */



/*
 * If we don't know what to type
 */
void Help(void)
{
    do_quiet = FALSE;
    ProgName();

    colour(LIGHTCYAN, BLACK);
    printf("\nUsage: mbindex <options>\n\n");
    colour(LIGHTBLUE, BLACK);
    printf("	Options are:\n\n");
    colour(CYAN, BLACK);
    printf("	-quiet		Quiet mode\n");
    colour(LIGHTGRAY, BLACK);
    printf("\n");
    die(MBERR_COMMANDLINE);
}



/*
 * Header, only if not quiet.
 */
void ProgName(void)
{
    if (do_quiet)
	return;

    colour(WHITE, BLACK);
    printf("\nMBINDEX: MBSE BBS %s Nodelist Index Compiler\n", VERSION);
    colour(YELLOW, BLACK);
    printf("         %s\n", COPYRIGHT);
    colour(CYAN, BLACK);
}



void die(int onsig)
{
    if (onsig && (onsig < NSIG))
	signal(onsig, SIG_IGN);

    ulockprogram((char *)"mbindex");

    if (!do_quiet) {
	colour(CYAN, BLACK);
	show_log = TRUE;
    }

    if (IsSema((char *)"mbindex"))
	RemoveSema((char *)"mbindex");

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    t_end = time(NULL);
    Syslog(' ', "MBINDEX finished in %s", t_elapsed(t_start, t_end));
	
    if (!do_quiet)
	colour(LIGHTGRAY, BLACK);

    ExitClient(onsig);
}



int main(int argc,char *argv[])
{
    int		    i;
    char	    *cmd;
    struct passwd   *pw;

    InitConfig();
    InitFidonet();
    TermInit(1, 80, 25);
    t_start = time(NULL);
    umask(002);

    /*
     *  Catch all the signals we can, and ignore the rest.
     *  Don't listen to SIGTERM.
     */
    for (i = 0; i < NSIG; i++) {

	if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV))
	    signal(i, (void (*))die);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    cmd = xstrcpy((char *)"Command line: mbindex");

    if (argc > 2)
	Help();

    if (argc == 2) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[1]);

	if (strncasecmp(argv[1], "-q", 2) == 0)
	    do_quiet = TRUE;
	else
	    Help();
    }

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbindex", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBINDEX v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (enoughspace(CFG.freespace) == 0)
	die(MBERR_DISK_FULL);

    if (lockprogram((char *)"mbindex")) {
	if (!do_quiet)
	    printf("Can't lock mbindex, abort.\n");
	die(MBERR_NO_PROGLOCK);
    }

    if (nodebld())
	die(MBERR_GENERAL);
    else
	die(MBERR_OK);
    return 0;
}



void tidy_nllist(nl_list **fap)
{
    nl_list	*tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *fap = NULL;
}



int in_nllist(struct _nlidx idx, nl_list **fap, int replace)
{
    nl_list	*tmp;

    for (tmp = *fap; tmp; tmp = tmp->next) {
	if ((tmp->idx.zone == idx.zone) && (tmp->idx.net == idx.net) &&
	       	(tmp->idx.node == idx.node) && (tmp->idx.point == idx.point)) {
	    if (replace) {
		tmp->idx = idx;
		entries++;
	    }
	    regio = tmp->idx.region;
	    return TRUE;
	}
    }
    return FALSE;
}



void fill_nllist(struct _nlidx idx, nl_list **fap)
{
    nl_list	*tmp;

    tmp = (nl_list *)malloc(sizeof(nl_list));
    tmp->next = *fap;
    tmp->idx  = idx;
    *fap = tmp;
    total++;
    entries++;
}



char *fullpath(char *fname)
{
    static char path[PATH_MAX];

    sprintf(path, "%s/%s", CFG.nodelists, fname);
    return path;
}



void sort_nllist(nl_list **fap)
{
    nl_list	*ta, **vector;
    size_t	n = 0, i;

    if (*fap == NULL)
	return;

    for (ta = *fap; ta; ta = ta->next)
	n++;

    vector = (nl_list **)malloc(n * sizeof(nl_list *));

    i = 0;
    for (ta = *fap; ta; ta = ta->next) {
	vector[i++] = ta;
    }

    qsort(vector, n, sizeof(nl_list *), (int(*)(const void*, const void *))comp_node);

    (*fap) = vector[0];
    i = 1;

    for (ta = *fap; ta; ta = ta->next) {
	if (i < n)
	    ta->next = vector[i++];
	else
	    ta->next = NULL;
    }

    free(vector);
    return;
}



int comp_node(nl_list **fap1, nl_list **fap2)
{
    if ((*fap1)->idx.zone != (*fap2)->idx.zone)
	return ((*fap1)->idx.zone - (*fap2)->idx.zone);
    else if ((*fap1)->idx.net != (*fap2)->idx.net)
	return ((*fap1)->idx.net - (*fap2)->idx.net);
    else if ((*fap1)->idx.node != (*fap2)->idx.node)
	return ((*fap1)->idx.node - (*fap2)->idx.node);
    else
	return ((*fap1)->idx.point - (*fap2)->idx.point);
}



int compile(char *nlname, unsigned short zo, unsigned short ne, unsigned short no)
{
    int		    num, i, rc = 0, lineno, boss = FALSE, bossvalid = FALSE;
    unsigned short  upnet, upnode;
    char	    buf[MAXNLLINELEN], *p, *q;
    faddr	    *tmpa;
    FILE	    *nl;
    struct _nlidx   ndx;
    struct _nlusr   udx;

    rc = 0;
    if ((nl = fopen(fullpath(nlname), "r")) == NULL) {
	WriteError("$Can't open %s", fullpath(nlname));
	return 102;
    }

    Syslog('+', "Compiling \"%s\" (%d)", nlname, filenr);
    IsDoing("Compile NL %d", filenr +1);

    memset(&ndx, 0, sizeof(ndx));
    ndx.type = NL_NODE;
    ndx.fileno = filenr;
    upnet = 0;
    upnode = 0;

    /*
     *  If zone is set, it is a overlay segment
     */
    if (zo) {
	ndx.zone  = zo;
	ndx.net   = ne;
	ndx.node  = no;
	ndx.point = 0;
    }
    entries = 0;
    lineno  = 0;

    while (!feof(nl)) {

	Nopper();
	ndx.offset = ftell(nl);
	lineno++;
	if (fgets(buf, sizeof(buf)-1, nl) == NULL)
	    continue;

	/*
	 * Next check at <lf> and <eof> characters
	 */
	if ((*(buf+strlen(buf) -1) != '\n') && (*(buf + strlen(buf) -1) != '\012')) {
	    while (fgets(buf, sizeof(buf) -1, nl) && (*(buf + strlen(buf) -1) != '\n')) /*void*/;
		if (strlen(buf) > 1) /* Suppress EOF character */
		    Syslog('-', "Nodelist: too long line junked (%d)", lineno);
	    continue;
	}

		if (*(p=buf+strlen(buf) -1) == '\n') 
			*p-- = '\0';
		if (*p == '\r') 
			*p = '\0';
		if ((buf[0] == ';') || (buf[0] == '\032') || (buf[0] == '\0'))
			continue;

		if (CFG.slow_util && do_quiet) {
			if (zo) {
				msleep(1);
			} else {
				if ((lineno % 40) == 0)
					msleep(1);
			}
		}

		if ((p = strchr(buf, ','))) 
			*p++ = '\0';
		if ((q = strchr(p, ','))) 
			*q++ = '\0';

		ndx.type = NL_NONE;
		ndx.pflag = 0;

		if (buf[0] == '\0') {
			if (boss)
				ndx.type = NL_POINT;
			else
				ndx.type = NL_NODE;
		} else 
		if (strcasecmp(buf,"Boss") == 0) {
			ndx.type = NL_POINT;
			bossvalid = FALSE;
			if ((tmpa=parsefnode(p)) == NULL) {
				WriteError("%s(%u): unparsable Boss addr \"%s\"", nlname,lineno,p);
				continue;
			}
			boss = TRUE;
			if (tmpa->zone) 
				ndx.zone = tmpa->zone;
			ndx.net   = tmpa->net;
			ndx.node  = tmpa->node;
			ndx.point = 0;
			tidy_faddr(tmpa);
			ndx.type = NL_NONE;

			if (in_nllist(ndx, &nll, FALSE)) {
				bossvalid = TRUE;
			}
			continue; /* no further processing */
		} else {
			boss = FALSE;
			ndx.type = NL_NONE;
			if (!strcasecmp(buf, "Down")) {
				ndx.pflag |= NL_DOWN;
				ndx.type = NL_NODE;
			}
			if (!strcasecmp(buf, "Hold")) {
				ndx.pflag |= NL_HOLD;
				ndx.type = NL_NODE;
			}
			if (!strcasecmp(buf, "Pvt")) {
				ndx.pflag |= NL_PVT;
				ndx.type = NL_NODE;
			}

			if (!strcasecmp(buf, "Zone"))
				ndx.type = NL_ZONE;
			if (!strcasecmp(buf, "Region"))
				ndx.type = NL_REGION;
			if (!strcasecmp(buf, "Host"))
				ndx.type = NL_HOST;
			if (!strcasecmp(buf, "Hub"))
				ndx.type = NL_HUB;
			if (!strcasecmp(buf, "Point")) {
				ndx.type = NL_POINT;
				bossvalid = TRUE;
			}
		}

		if (ndx.type == NL_NONE) {
			for (q = buf; *q; q++) 
				if (*q < ' ') 
					*q='.';
			WriteError("%s(%u): unidentified entry \"%s\"", nlname, lineno, buf);
			continue;
		}

		if ((num=atoi(p)) == 0) {
			WriteError("%s(%u): bad numeric \"%s\"", nlname,lineno,p);
			continue;
		}

		/*
		 * now update the current address
		 */
		switch (ndx.type) {
		case NL_REGION:	ndx.net   = num;
				ndx.node  = 0;
				ndx.point = 0;
				ndx.upnet = ndx.zone;
				ndx.upnode= 0;
				ndx.region= num;
				upnet     = num;
				upnode    = 0;
				break;
		case NL_ZONE:	ndx.zone  = num;
				ndx.net   = num;
				ndx.node  = 0;
				ndx.point = 0;
				ndx.upnet = 0;
				ndx.upnode= 0;
				ndx.region= 0;
				upnet     = num;
				upnode    = 0;
				break;
		case NL_HOST:	ndx.net   = num;
				ndx.node  = 0;
				ndx.point = 0;
				ndx.upnet = ndx.region;
				ndx.upnode= 0;
				upnet     = num;
				upnode    = 0;
				break;
		case NL_HUB:	ndx.node  = num;
				ndx.point = 0;
				ndx.upnet = ndx.net;
				ndx.upnode= 0;
				upnet     = ndx.net;
				upnode    = num;
				break;
		case NL_NODE:	ndx.node  = num;
				ndx.point = 0;
				ndx.upnet = upnet;
				ndx.upnode= upnode;
				break;
		case NL_POINT:	ndx.point = num;
				ndx.upnet = ndx.net;
				ndx.upnode= ndx.node;
				if ((!ndx.region) && bossvalid)
					ndx.region = regio;
				break;
		}
		if (!do_quiet) {
			printf("\rZone %-6uRegion %-6uNet %-6uNode %-6uPoint %-6u", 
				ndx.zone, ndx.region, ndx.net, ndx.node, ndx.point);
			fflush(stdout);
		}

		memset(&udx, 0, sizeof(udx));
		udx.record = total;

		/*
		 *  Read nodelist line and extract username.
		 */
		for (i = 0; i < 3; i++) {
			p = q;
			if (p == NULL)
				continue;
			if ((q = strchr(p, ',')))
				*q++ = '\0';
			if (q == NULL)
			    q = p;
		}
		if (strlen(p) > 35)
			p[35] = '\0';
		sprintf(udx.user, "%s", p);

		/*
		 *  Now search for the baudrate field, 300 means it's
		 *  and ISDN or TCP/IP only node which is a special case.
		 */
		for (i = 0; i < 2; i++) {
			p = q;
			if (p == NULL)
				continue;
			if ((q = strchr(p, ',')))
				*q++ = '\0';
			if (q == NULL)
				q = p;
		}
		if ((strlen(p) == 3) && (!strcmp(p, "300")) && (q != NULL)) {
			if ((strstr(q, (char *)"X75")) ||
			    (strstr(q, (char *)"V110L")) ||
			    (strstr(q, (char *)"V110H")) ||
			    (strstr(q, (char *)"V120L")) ||
			    (strstr(q, (char *)"V120H")) ||
			    (strstr(q, (char *)"ISDN")))
				ndx.pflag |= NL_ISDN;
			if ((strstr(q, (char *)"IFC")) ||
			    (strstr(q, (char *)"IBN")) ||
			    (strstr(q, (char *)"ITN")) ||
			    (strstr(q, (char *)"IVM")) ||
			    (strstr(q, (char *)"IFT")) ||
			    (strstr(q, (char *)"IP")))
				ndx.pflag |= NL_TCPIP;
		}

		/*
		 *  If zone, net and node given, then this list is an
		 *  overlay so we will call in_list() to replace the
		 *  existing records, or append them if they don't exist.
		 *  Also, only points with a valid boss will be added.
		 */
		if (zo) {
			if (!(in_nllist(ndx, &nll, TRUE))) {
				if (ndx.point && bossvalid) {
					fill_nllist(ndx, &nll);
				}
				if (!ndx.point)
					fill_nllist(ndx, &nll);
			}
		} else
			fill_nllist(ndx, &nll);
	}

	fclose(nl);
	Syslog('+', "%d entries", entries);

	if (!do_quiet) {
		printf(" %ld entries\n", entries);
		fflush(stdout);
	}

	return rc;
}



/*
 * Tidy the filearray
 */
void tidy_fdlist(fd_list **fdp)
{
	fd_list	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add a file on the array.
 */
void fill_fdlist(fd_list **fdp, char *filename, time_t filedate)
{
	fd_list	*tmp;

	tmp = (fd_list *)malloc(sizeof(fd_list));
	tmp->next = *fdp;
	sprintf(tmp->fname, "%s", filename);
	tmp->fdate = filedate;
	*fdp = tmp;
}



int compfdate(fd_list **, fd_list **);


/*
 * Sort the array of files by filedate
 */
void sort_fdlist(fd_list **fdp)
{
	fd_list	*ta, **vector;
	size_t	n = 0, i;

	if (*fdp == NULL) {
		return;
	}

	for (ta = *fdp; ta; ta = ta->next)
		n++;

	vector = (fd_list **)malloc(n * sizeof(fd_list *));

	i = 0;
	for (ta = *fdp; ta; ta = ta->next) {
		vector[i++] = ta;
	}

	qsort(vector, n, sizeof(fd_list*), (int(*)(const void*, const void*))compfdate);

	(*fdp) = vector[0];
	i = 1;

	for (ta = *fdp; ta; ta = ta->next) {
		
		if (i < n)
			ta->next = vector[i++];
		else
			ta->next = NULL;
	}

	free(vector);
	return;
}



int compfdate(fd_list **fdp1, fd_list **fdp2)
{
	return ((*fdp1)->fdate - (*fdp2)->fdate);
}



/*
 * Return the name of the oldest file in the array
 */
char *pull_fdlist(fd_list **fdp)
{
	static char	buf[65];
	fd_list		*ta;

	if (*fdp == NULL)
		return NULL;

	ta = *fdp;
	memset(&buf, 0, sizeof(buf));
	sprintf(buf, "%s", ta->fname);

	if (ta->next != NULL)
		*fdp = ta->next;
	else
		*fdp = NULL;

	free(ta);
	return buf;
}



int makelist(char *base, unsigned short zo, unsigned short ne, unsigned short no)
{
	int		rc = 0, files = 0;
	struct dirent	*de;
	DIR		*dp;
	char		*p = NULL, *q;
	struct _nlfil	fdx;
	fd_list		*fdl = NULL;

	if (!strlen(base)) {
		WriteError("Error, no nodelist defined for %d:%d/%d", zo, ne, no);
		return 0;
	}

	if ((dp = opendir(CFG.nodelists)) == NULL) {
		WriteError("$Can't open dir %s", CFG.nodelists);
		rc = 103;
	} else {
		while ((de = readdir(dp)))
			if (strncmp(de->d_name, base, strlen(base)) == 0) {
				/*
				 *  Extension must be at least 2 digits
				 */
				q = (de->d_name) + strlen(base);
				if ((*q == '.') && (strlen(q) > 2) &&
				    (strspn(q+1,"0123456789") == (strlen(q)-1))) {
					/*
					 * Add matched filenames to the array
					 */
					fill_fdlist(&fdl, de->d_name, file_time(fullpath(de->d_name)));
					files++;
				}
			}
		closedir(dp);

		if (files == 0) {
			Syslog('+', "No nodelist found for %s", base);
			return 103;
		}

		/*
		 * Sort found nodelists by age and kill all but the newest 2.
		 */
		sort_fdlist(&fdl);
		while (files) {
			p = pull_fdlist(&fdl);
			if (files > 2) {
				Syslog('+', "Remove old \"%s\"", p);
				unlink(fullpath(p));
			}
			files--;
		}
		tidy_fdlist(&fdl); 

		memset(&fdx, 0, sizeof(fdx));
		sprintf(fdx.filename, "%s", p);
		sprintf(fdx.domain, "%s", fidonet.domain);
		fdx.number = filenr;
		fwrite(&fdx, sizeof(fdx), 1, ffp);

		rc = compile(p, zo, ne, no);
		filenr++;
	}

	return rc;
}



int nodebld(void)
{
	int		rc = 0, i;
	char		*im, *fm, *um, *old, *new;
	struct _nlfil	fdx;
	FILE		*fp;
	nl_list		*tmp;

	memset(&fdx, 0, sizeof(fdx));
	im = xstrcpy(fullpath((char *)"temp.index"));
	fm = xstrcpy(fullpath((char *)"temp.files"));
	um = xstrcpy(fullpath((char *)"temp.users"));

	if ((ifp = fopen(im, "w+")) == NULL) {
		WriteError("$Can't create %s",MBSE_SS(im));
		return 101;
	}
	if ((ufp = fopen(um, "w+")) == NULL) {
		WriteError("$Can't create %s", MBSE_SS(um));
		fclose(ifp);
		unlink(im);
		return 101;
	}
	if ((ffp = fopen(fm, "w+")) == NULL) {
		WriteError("$Can't create %s", MBSE_SS(fm));
		fclose(ifp);
		unlink(im);
		fclose(ufp);
		unlink(um);
		return 101;
	}

	if (!do_quiet) {
		colour(3, 0);
		printf("\n");
	}

	if ((fp = fopen(fidonet_fil, "r")) == 0)
		rc = 102;
	else {
		fread(&fidonethdr, sizeof(fidonethdr), 1, fp);

		while (fread(&fidonet, fidonethdr.recsize, 1, fp) == 1) {
			if (fidonet.available) {
				rc = makelist(fidonet.nodelist, 0, 0, 0);
				if (rc)
					break;

				for (i = 0; i < 6; i++) {
					if (fidonet.seclist[i].zone) {
						rc = makelist(fidonet.seclist[i].nodelist,
							fidonet.seclist[i].zone,
							fidonet.seclist[i].net, 
							fidonet.seclist[i].node);
						if (rc)
							break;
					}
				}
				if (rc)
					break;
			}
		}

		fclose(fp);
	}

	fclose(ufp);
	fclose(ffp);

	if (rc == 0) {
		IsDoing("Sorting nodes");
		sort_nllist(&nll);

		IsDoing("Writing files");
		for (tmp = nll; tmp; tmp = tmp->next)
			fwrite(&tmp->idx, sizeof(struct _nlidx), 1, ifp);
		fclose(ifp);

		Syslog('+', "Compiled %d entries", total);

		/*
		 *  Rename existing files to old.*, they stay on disk in
		 *  case they are open by some program. The temp.* files
		 *  are then renamed to node.*
		 */
		old = xstrcpy(fullpath((char *)"old.index"));
		new = xstrcpy(fullpath((char *)"node.index"));
		unlink(old);
		rename(new, old);
		rename(im, new);
		free(old);
		free(new);
		old = xstrcpy(fullpath((char *)"old.users"));
		new = xstrcpy(fullpath((char *)"node.users"));
		unlink(old);
		rename(new, old);
		rename(um, new);
		free(old);
		free(new);
		old = xstrcpy(fullpath((char *)"old.files"));
		new = xstrcpy(fullpath((char *)"node.files"));
		unlink(old);
		rename(new, old);
		rename(fm, new);
		free(old);
		free(new);
	} else {
		fclose(ifp);
		Syslog('+', "Compile failed, rc=%d", rc);
		unlink(im);
		unlink(fm);
		unlink(um);
	}

	free(im);
	free(fm);
	free(um);
	tidy_nllist(&nll);

	return rc;
}



