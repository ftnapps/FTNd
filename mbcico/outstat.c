/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Show mail outbound status
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbftn.h"
#include "../lib/mberrors.h"
#include "scanout.h"
#include "callstat.h"
#include "outstat.h"


extern int	do_quiet;


static struct _alist
{
	struct	_alist *next;
	faddr	addr;
	int	flavors;
	time_t	time;
	off_t	size;
} *alist = NULL;


#define F_NORMAL 1
#define F_CRASH  2
#define	F_IMM	 4
#define F_HOLD   8
#define F_FREQ  16
#define	F_POLL  32



int outstat()
{
    int		    rc;
    struct _alist   *tmp, *old;
    char	    flstr[6], *temp, flavor;
    time_t	    age;
    faddr	    *fa;
    callstat        *cst;
    FILE	    *fp;
    DIR		    *dp = NULL;
    struct dirent   *de;
    struct stat	    sb;
    struct passwd   *pw;

    if ((rc = scanout(each))) {
	WriteError("Error scanning outbound, aborting");
	return MBERR_OUTBOUND_SCAN;
    }

    /*
     * Check private outbound box for nodes in the setup.
     * Also, check directory outbounds for FTP nodes.
     */
    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("Error open %s, aborting", temp);
	free(temp);
	return MBERR_OUTBOUND_SCAN;
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
	    fa = fido2faddr(nodes.Aka[0]);
	    Syslog('o', "checking outbox %s (%s)", nodes.OutBox, ascfnode(fa, 0x2f));
	    if ((dp = opendir(nodes.OutBox)) == NULL) {
		Syslog('o', "\"%s\" cannot be opened, proceed", MBSE_SS(nodes.OutBox));
	    } else {
		while ((de = readdir(dp))) {
		    if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			sprintf(temp, "%s/%s", nodes.OutBox, de->d_name);
			if (stat(temp, &sb) == 0) {
			    Syslog('o' ,"checking: \"%s\"", de->d_name);
			    if (S_ISREG(sb.st_mode)) {
				if (pw->pw_uid == sb.st_uid) {
				    /*
				     * We own the file
				     */
				    if ((sb.st_mode & S_IRUSR) && (sb.st_mode & S_IWUSR)) {
					each(fa, flavor, 0, temp);
				    } else {
					Syslog('o', "no R/W permission on %s", temp);
				    }
				} else if (pw->pw_gid == sb.st_gid) {
				    /*
				     * We own the file group
				     */
				    if ((sb.st_mode & S_IRGRP) && (sb.st_mode & S_IWGRP)) {
					each(fa, flavor, 0, temp);
				    } else {
					Syslog('o', "no R/W permission on %s", temp);
				    }
				} else {
				    /*
				     * No owner of file
				     */
				    if ((sb.st_mode & S_IROTH) && (sb.st_mode & S_IWOTH)) {
					each(fa, flavor, 0, temp);
				    } else {
					Syslog('o', "no R/W permission on %s", temp);
				    }
				}
			    } else {
				Syslog('o', "not a regular file");
			    }
			} else {
			    WriteError("Can't stat %s", temp);
			}
		    }
		}
		closedir(dp);
	    }
	    tidy_faddr(fa);
	}
	if ((nodes.Session_out == S_DIR) && strlen(nodes.Dir_out_path)) {
	    fa = fido2faddr(nodes.Aka[0]);
	    flavor = 'h';   /* Directory outbound files are always on hold */
	    Syslog('o', "checking directory path %s (%s)", nodes.Dir_out_path, ascfnode(fa, 0x2f));
	    if ((dp = opendir(nodes.Dir_out_path)) == NULL) {
		Syslog('o', "\"%s\" cannot be opened, proceed", MBSE_SS(nodes.Dir_out_path));
	    } else {
		while ((de = readdir(dp))) {
		    if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			sprintf(temp, "%s/%s", nodes.Dir_out_path, de->d_name);
			if (stat(temp, &sb) == 0) {
			    if (S_ISREG(sb.st_mode)) {
				each(fa, flavor, 0, temp);
			    } else {
				Syslog('o', "not a regular file");
			    }
			} else {
			    WriteError("Can't stat %s", temp);
			}
		    }
		}
		closedir(dp);
	    }
	    tidy_faddr(fa);
	}
	fseek(fp, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
    }
    fclose(fp);

    if (!do_quiet) {
	colour(10, 0);
	printf("flavor try      size age    address\n");
	colour(3, 0);
    }

    Syslog('+', "Flavor Try      Size Age    Address");
    for (tmp = alist; tmp; tmp = tmp->next) {
	if ((tmp->flavors & F_FREQ) || (tmp->size) || 1) {
	    strcpy(flstr,"......");
	    if ((tmp->flavors) & F_IMM   ) flstr[0]='I';
	    if ((tmp->flavors) & F_CRASH ) flstr[1]='C';
	    if ((tmp->flavors) & F_NORMAL) flstr[2]='N';
	    if ((tmp->flavors) & F_HOLD  ) flstr[3]='H';
	    if ((tmp->flavors) & F_FREQ  ) flstr[4]='R';
	    if ((tmp->flavors) & F_POLL  ) flstr[5]='P';

	    cst = getstatus(&(tmp->addr));
	    age = time(NULL);
	    age -= tmp->time;
	    sprintf(temp, "%s %3d %9lu %s %s", flstr, cst->tryno, (long)tmp->size, str_time(age), ascfnode(&(tmp->addr), 0x1f));

	    if (!do_quiet)
		printf("%s\n", temp);
	    Syslog('+', "%s", temp);
	}
    }

    free(temp);
    for (tmp = alist; tmp; tmp = old) {
	old = tmp->next;
	free(tmp->addr.domain);
	free(tmp);
    }
    alist = NULL;

    return 0;
}



int each(faddr *addr, char flavor, int isflo, char *fname)
{
	struct	_alist **tmp;
	struct	stat st;
	FILE	*fp;
	char	buf[256], *p;

	Syslog('o', "each(%s, %c, %s, %s)", ascfnode(addr, 0x2f), flavor, isflo?"isflo":"noflo", fname);

	if ((isflo != OUT_PKT) && (isflo != OUT_FLO) && (isflo != OUT_REQ) && (isflo != OUT_POL))
		return 0;

	for (tmp = &alist; *tmp; tmp = &((*tmp)->next))
		if (((*tmp)->addr.zone  == addr->zone) && ((*tmp)->addr.net   == addr->net) &&
		    ((*tmp)->addr.node  == addr->node) && ((*tmp)->addr.point == addr->point) &&
		    (((*tmp)->addr.domain == NULL) || (addr->domain == NULL) ||
		     (strcasecmp((*tmp)->addr.domain,addr->domain) == 0)))
			break;
	if (*tmp == NULL) {
		*tmp = (struct _alist *)malloc(sizeof(struct _alist));
		(*tmp)->next = NULL;
		(*tmp)->addr.name   = NULL;
		(*tmp)->addr.zone   = addr->zone;
		(*tmp)->addr.net    = addr->net;
		(*tmp)->addr.node   = addr->node;
		(*tmp)->addr.point  = addr->point;
		(*tmp)->addr.domain = xstrcpy(addr->domain);
		(*tmp)->flavors = 0;
		(*tmp)->time = time(NULL);
		(*tmp)->size = 0L;
	}

	if ((isflo == OUT_FLO) || (isflo == OUT_PKT)) 
		switch (flavor) {
			case '?':	break;
			case 'i':	(*tmp)->flavors |= F_IMM; break;
			case 'o':	(*tmp)->flavors |= F_NORMAL; break;
			case 'c':	(*tmp)->flavors |= F_CRASH; break;
			case 'h':	(*tmp)->flavors |= F_HOLD; break;
			default:	WriteError("Unknown flavor: '%c'\n",flavor); break;
		}

	if (stat(fname,&st) != 0) {
		WriteError("$Can't stat %s", fname);
		st.st_size = 0L;
		st.st_mtime = time(NULL);
	}

	/*
	 * Find the oldest time
	 */
	if (st.st_mtime < (*tmp)->time) 
		(*tmp)->time = st.st_mtime;

	if (isflo == OUT_FLO) {
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
							st.st_size = 0L;
							st.st_mtime = time(NULL);
						}
					} else {
						if (stat(p, &st) != 0) {
							st.st_size = 0L;
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
			WriteError("Can't open %s", fname);

	} else if (isflo == OUT_PKT) {
		(*tmp)->size += st.st_size;
	} else if (isflo == OUT_REQ) {
		(*tmp)->flavors |= F_FREQ;
	} else if (isflo == OUT_POL) {
		(*tmp)->flavors |= F_POLL;
	}

	return 0;
}



int IsZMH(void);
int IsZMH()
{
	static  char buf[81];

	sprintf(buf, "SBBS:0;");
	if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "100:2,2", 7) == 0)
			return TRUE;
	}
	return FALSE;
}



int poll(faddr *addr, int stop)
{
	char		*pol;
	int		rc = 0;
	FILE		*fp;
	callstat	*cst;
	node		*nlent;

	if (addr == NULL)
		return 0;

	pol = polname(addr);

	if (stop) {
		if (access(pol, R_OK) == 0) {
			rc = unlink(pol);
			if (rc == 0) {
				Syslog('+', "Removed poll for %s", ascfnode(addr, 0x1f));
				if (!do_quiet)
					printf("Removed poll for %s\n", ascfnode(addr, 0x1f));
			}
		} else {
			Syslog('+', "No poll found for %s", ascfnode(addr, 0x1f));
		}
	} else {
		nlent = getnlent(addr);
		if (nlent->pflag == NL_DUMMY) {
			Syslog('+', "Node %s not in nodelist", ascfnode(addr, 0x1f));
			if (!do_quiet)
				printf("Node %s not in nodelist", ascfnode(addr, 0x1f));
			return MBERR_NODE_NOT_IN_LIST;
		}
		if (nlent->pflag == NL_DOWN) {
			Syslog('+', "Node %s has status Down", ascfnode(addr, 0x1f));
			if (!do_quiet)
				printf("Node %s has status Down", ascfnode(addr, 0x1f));
			return MBERR_NODE_MAY_NOT_CALL;
		}
		if (nlent->pflag == NL_HOLD) {
			Syslog('+', "Node %s has status Hold", ascfnode(addr, 0x1f));
			if (!do_quiet)
				printf("Node %s has status Hold", ascfnode(addr, 0x1f));
			return MBERR_NODE_MAY_NOT_CALL;
		}
	
		if ((fp = fopen(pol, "w+")) == NULL) {
			WriteError("$Can't create poll for %s", ascfnode(addr, 0x1f));
			rc = MBERR_CANNOT_MAKE_POLL;
		} else {
			fclose(fp);
			if (((nlent->oflags & OL_CM) == 0) && (!IsZMH())) {
				Syslog('+', "Created poll for %s, non-CM node outside ZMH", ascfnode(addr, 0x1f));
				if (!do_quiet)
					printf("Created poll for %s, non-CM node outside ZMH\n", ascfnode(addr, 0x1f));
			} else {
				Syslog('+', "Created poll for %s", ascfnode(addr, 0x1f));
				if (!do_quiet)
					printf("Created poll for %s\n", ascfnode(addr, 0x1f));
			}
			cst = getstatus(addr);
			if ((cst->trystat == MBERR_NODE_LOCKED) || 
			    (cst->trystat == MBERR_NOT_ZMH) ||
			    (cst->trystat == MBERR_NO_CONNECTION) ||
			    (cst->trystat == MBERR_SESSION_ERROR) ||
			    (cst->trystat == MBERR_UNKNOWN_SESSION) ||
			    (cst->trystat == MBERR_NO_PORT_AVAILABLE) ||
			    (cst->trystat == MBERR_MODEM_ERROR)) {
				putstatus(addr, 0, 0);
			}
			CreateSema((char *)"scanout");
		}
	}

	return 0;
}



int reset(faddr *addr)
{
    if (addr == NULL)
	return 0;

    putstatus(addr, 0, 0);
    Syslog('+', "Reset try-counter for %s", ascfnode(addr, 0x1f));
    if (!do_quiet)
	printf("Reset try-counter for %s\n", ascfnode(addr, 0x1f));
    CreateSema((char *)"scanout");

    return 0;
}



int freq(faddr *addr, char *fname)
{
	char	*req;
	FILE	*fp;

	Syslog('o', "Freq %s %s", ascfnode(addr, 0x1f), fname);

	/*
	 * Append filename to .req file
	 */
	req = xstrcpy(reqname(addr));
	if ((fp = fopen(req, "a")) == NULL) {
		WriteError("$Can't append to %s", req);
		if (!do_quiet)
			printf("File request failed\n");
		free(req);
		return MBERR_REQUEST;
	}
	fprintf(fp, "%s\r\n", fname);
	fclose(fp);

	Syslog('+', "File request \"%s\" from %s", fname, ascfnode(addr, 0x1f));
	if (!do_quiet)
		printf("File request \"%s\" from %s\n", fname, ascfnode(addr, 0x1f));

	free(req);
	return 0;
}



