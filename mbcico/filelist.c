/*****************************************************************************
 *
 * $Id: filelist.c,v 1.32 2008/11/26 22:01:01 mbse Exp $
 * Purpose ...............: fidonet mailer 
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
#include "../lib/nodelist.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "config.h"
#include "session.h"
#include "filelist.h"


#ifndef PATH_MAX
#define PATH_MAX 512
#endif

extern int  master;
extern int  Loaded;
int	    made_request;



static char *tmpkname(void);
static char *tmpkname(void)
{
    static char buf[16];

    snprintf(buf,16,"%08x.pkt", sequencer());
    return buf;
}



char *xtodos(char *orig)
{
    char	buf[13], *copy, *p;

    if (orig == NULL) 
	return NULL;

    if ((remote_flags & SESSION_FNC) == 0) {
	Syslog('o', "No filename conversion for \"%s\"", MBSE_SS(orig));
	return xstrcpy(orig);
    }

    copy = xstrcpy(orig);
    if ((p = strrchr(copy,'/'))) 
	p++;
    else 
	p = copy;

    name_mangle(p);
    memset(&buf, 0, sizeof(buf));
    strncpy(buf, p, 12);
    Syslog('o', "name \"%s\" converted to \"%s\"", MBSE_SS(orig), buf);
    free(copy);
    return xstrcpy(buf);
}



/*
 * Add the specified entry to the list of files which should be sent
 * to the remote system.
 * 1. lst		file list to add entry to
 * 2. local		local filename
 * 3. remote		remote filename
 * 4. disposition	disposition
 * 5. floff		offset of entry in flo-file (-1 if this is a flo file)
 * 6. flofp		FILE * of flo file
 * 7. toend		append to end of list
 */
void add_list(file_list **lst, char *local, char *Remote, int disposition, off_t floff, FILE *flofp, int toend)
{
    file_list **tmpl, *tmp;

    Syslog('o', "add_list(\"%s\",\"%s\",%d,%s)", MBSE_SS(local),MBSE_SS(Remote),disposition,toend?"to end":"to beg");

    if (toend) 
	for (tmpl = lst; *tmpl; tmpl =&((*tmpl)->next));
    else 
	tmpl = &tmp;
    *tmpl = (file_list*)malloc(sizeof(file_list));
    if (toend) {
	(*tmpl)->next = NULL;
    } else {
	(*tmpl)->next = *lst;
	*lst = *tmpl;
    }

    (*tmpl)->remote      = xtodos(Remote);
    (*tmpl)->local       = xstrcpy(local);
    (*tmpl)->disposition = disposition;
    (*tmpl)->floff       = floff;
    (*tmpl)->flofp       = flofp;
    return;
}



static void check_flo(file_list **, char *);
static void check_flo(file_list **lst, char *nm)
{
    FILE	*fp;
    off_t	off;
    struct	flock fl;
    char	buf[PATH_MAX], buf2[PATH_MAX], *p, *q;
    int		disposition;
    struct stat stbuf;

    if ((fp = fopen(nm,"r+")) == NULL) {
	/*
	 * If no flo file, return.
	 */
	return;
    }

    Syslog('o', "check_flo(\"%s\")",MBSE_SS(nm));

    fl.l_type   = F_RDLCK;
    fl.l_whence = 0;
    fl.l_start  = 0L;
    fl.l_len    = 0L;

    if (fcntl(fileno(fp), F_SETLK, &fl) != 0) {
	if (errno != EAGAIN)
	    WriteError("$Can't read-lock \"%s\"", MBSE_SS(nm));
	else 
	    Syslog('o',"flo file busy");
	fclose(fp);
	return;
    }

    if (stat(nm, &stbuf) != 0) {
	WriteError("$Can't access \"%s\"", MBSE_SS(nm));
	fclose(fp);
	return;
    }

    while (!feof(fp) && !ferror(fp)) {
	off = ftell(fp);
	if (fgets(buf, sizeof(buf)-1, fp) == NULL) 
	    continue;
	if (buf[0] == '~') 
	    continue; /* skip sent files */
	if (*(p=buf + strlen(buf) -1) == '\n') 
	    *p-- = '\0';
	if (*p == '\r') 
	    *p = '\0';

	switch (buf[0]) {
	    case '#':	p=buf+1; disposition=TFS; break;
	    case '-':
	    case '^':	p=buf+1; disposition=KFS; break;
	    case '@':	p=buf+1; disposition=LEAVE; break;
	    case 0:	continue;
	    default:	p=buf; disposition=LEAVE; break;
	}

	memset(&buf2, 0, sizeof(buf2));
	if (strlen(CFG.dospath)) {
	    if (strncasecmp(p, CFG.dospath, strlen(CFG.dospath)) == 0) {
		strcpy(buf2,uxoutbound);
		for (p+=strlen(CFG.dospath), q=buf2+strlen(buf2); *p; p++, q++)
		    *q = ((*p) == '\\')?'/':tolower(*p);
		*q = '\0';
		p = buf2;
	    }
	}

	if ((q=strrchr(p,'/')))
	    q++;
	else
	    q = p;
	add_list(lst, p, q, disposition, off, fp, 1);
    }

    /*
     * Add flo file to file list
     */
    add_list(lst, nm, NULL, KFS, -1L, fp, 1);

    /* here, we leave .flo file open, it will be closed by */
    /* execute_disposition */

    return;
}



void check_filebox(char *, file_list **);
void check_filebox(char *boxpath, file_list **st)
{
    char	    *temp;
    DIR             *dp;
    struct dirent   *de;
    struct passwd   *pw;
    struct stat     stbuf;

    if ((dp = opendir(boxpath)) == NULL) {
	Syslog('o', "\"%s\" cannot be opened, proceed", MBSE_SS(boxpath));
    } else {
	Syslog('o', "checking filebox \"%s\"", boxpath);
        temp = calloc(PATH_MAX, sizeof(char));
	pw = getpwnam((char *)"mbse");
        while ((de = readdir(dp))) {
            if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
		snprintf(temp, PATH_MAX -1, "%s/%s", boxpath, de->d_name);
                if (stat(temp, &stbuf) == 0) {
                    Syslog('o' ,"checking file \"%s\"", de->d_name);
                    if (S_ISREG(stbuf.st_mode)) {
                        if (pw->pw_uid == stbuf.st_uid) {
                            /*
                             * We own the file
                             */
                            if ((stbuf.st_mode & S_IRUSR) && (stbuf.st_mode & S_IWUSR)) {
                                add_list(st, temp, de->d_name, KFS, 0L, NULL, 1);
                            } else {
                                Syslog('+', "No R/W permission on %s", temp);
                            }
                        } else if (pw->pw_gid == stbuf.st_gid) {
                            /*
                             * We own the file group
                             */
                            if ((stbuf.st_mode & S_IRGRP) && (stbuf.st_mode & S_IWGRP)) {
                                add_list(st, temp, de->d_name, KFS, 0L, NULL, 1);
                            } else {
                                Syslog('+', "No R/W permission on %s", temp);
                            }
                        } else {
                            /*
                             * No owner of file
                             */
                            if ((stbuf.st_mode & S_IROTH) && (stbuf.st_mode & S_IWOTH)) {
                                add_list(st, temp, de->d_name, KFS, 0L, NULL, 1);
                            } else {
                                Syslog('+', "No R/W permission on %s", temp);
                            }
                        }
                    } else {
                        Syslog('+', "Not a regular file %s", temp);
                    }
                } else {
                    WriteError("Can't stat %s", temp);
                }
            }
	}
	closedir(dp);
	free(temp);
    }
}



file_list *create_filelist(fa_list *al, char *fl, int create)
{
    file_list	    *st = NULL, *tmpf;
    fa_list	    *tmpa;
    char	    flavor, *tmpfl, *nm, *temp, tmpreq[13], digit[6], *temp2;
    struct stat	    stbuf;
    int		    packets = 0;
    FILE	    *fp;
    unsigned char   buffer[2];
    faddr	    *fa;
    DIR		    *dp = NULL;
    struct dirent   *de;
    struct stat     sb;

    Syslog('o', "Create_filelist(%s,\"%s\",%d)", al?ascfnode(al->addr,0x1f):"<none>", MBSE_SS(fl), create);
    made_request = 0;

    for (tmpa = al; tmpa; tmpa = tmpa->next) {
	Syslog('o', "Check address %s", ascfnode(tmpa->addr, 0x1f));

	/*
	 * For the main aka, check the outbox.
	 */
	if ((tmpa->addr) && Loaded && strlen(nodes.OutBox) &&
	    (tmpa->addr->zone == nodes.Aka[0].zone) && (tmpa->addr->net == nodes.Aka[0].net) &&
	    (tmpa->addr->node == nodes.Aka[0].node) && (tmpa->addr->point == nodes.Aka[0].point)) {
	    check_filebox(nodes.OutBox, &st);
	}

	/*
	 * Check spool files, these are more or less useless but they
	 * create a filelist of files to send which are never send.
	 * It gives the transfer protocols something "to do".
	 */
	if (strchr(fl, 'o')) {
	    nm = splname(tmpa->addr);
	    if ((fp = fopen(nm, "w"))) 
		fclose(fp);
	    if ((nm != NULL) && (stat(nm, &stbuf) == 0))
		add_list(&st, nm, NULL, DSF, 0L, NULL, 1);
	}

	/*
	 * Check .pol files
	 */
	nm = polname(tmpa->addr);
	if ((nm != NULL) && (stat(nm,&stbuf) == 0))
	    add_list(&st,nm,NULL,DSF,0L,NULL,1);

	/*
	 * Check other files, all flavors
	 */
	tmpfl = fl;
	while ((flavor = *tmpfl++)) {

	    Syslog('o', "Check flavor %c", flavor);
	    /*
	     * Check normal mail packets
	     */
	    nm = pktname(tmpa->addr,flavor);
	    if ((nm != NULL) && (stat(nm,&stbuf) == 0)) {
		Syslog('o', "found %s", nm);
		packets++;
		add_list(&st, nm, tmpkname(), KFS, 0L, NULL, 1);
	    }

	    /*
	     * Check .flo files for file attaches
	     */
	    nm = floname(tmpa->addr,flavor);
	    check_flo(&st, nm);
	}

	if ((session_flags & SESSION_WAZOO) &&
	    ((session_flags & SESSION_HYDRA) == 0) && (master || ((session_flags & SESSION_IFNA) == 0))) {
	    /*
	     * we don't distinguish flavoured reqs
	     */
	    nm = reqname(tmpa->addr);
	    if ((nm != NULL) && (stat(nm, &stbuf) == 0)) {
		snprintf(tmpreq, 13, "%04X%04X.REQ", tmpa->addr->net, tmpa->addr->node);
		add_list(&st, nm, tmpreq, DSF, 0L, NULL, 1);
		made_request = 1;
	    }
	}
    }

    /*
     * Check T-Mail style fileboxes
     */
    temp = calloc(PATH_MAX, sizeof(char));
    if (strlen(CFG.tmailshort) && (dp = opendir(CFG.tmailshort))) {
       Syslog('o', "Checking T-Mail short box \"%s\"", CFG.tmailshort);
       while ((de = readdir(dp))) {
           if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
               snprintf(temp, PATH_MAX -1, "%s/%s", CFG.tmailshort, de->d_name);
               if (stat(temp, &sb) == 0) {
                   Syslog('o' ,"checking \"%s\"", de->d_name);
                   if (S_ISDIR(sb.st_mode)) {
                       int i;
                       char b=0;
                       for (i=0; (i<8) && (!b); ++i) {
                           char c = tolower(de->d_name[i]);
                           if ( (c<'0') || (c>'v') || ((c>'9') && (c<'a')) ) 
			       b=1;
                       }
                       if (de->d_name[8]!='.') 
			   b=1;
                       for (i=9; (i<11) && (!b); ++i) {
                           char c = tolower(de->d_name[i]);
                           if ( (c<'0') || (c>'v') || ((c>'9') && (c<'a')) ) 
			       b=1;
                       }
                       if (b) 
			   continue;
                       if (de->d_name[11]==0) 
			   flavor='o';
                       else if ((tolower(de->d_name[11])=='h') && (de->d_name[12]==0)) 
			   flavor='h';
                       else 
			   continue;
                       fa = (faddr*)malloc(sizeof(faddr));
                       fa->name = NULL;
                       fa->domain = NULL;
                       memset(&digit, 0, sizeof(digit));
                       digit[0] = de->d_name[0];
                       digit[1] = de->d_name[1];
                       fa->zone = strtol(digit, NULL, 32);
                       memset(&digit, 0, sizeof(digit));
                       digit[0] = de->d_name[2];
                       digit[1] = de->d_name[3];
                       digit[2] = de->d_name[4];
                       fa->net = strtol(digit, NULL, 32);
                       memset(&digit, 0, sizeof(digit));
                       digit[0] = de->d_name[5];
                       digit[1] = de->d_name[6];
                       digit[2] = de->d_name[7];
                       fa->node = strtol(digit, NULL, 32);
                       memset(&digit, 0, sizeof(digit));
                       digit[0] = de->d_name[9];
                       digit[1] = de->d_name[10];
                       fa->point = strtol(digit, NULL, 32);
                       for (tmpa = al; tmpa; tmpa = tmpa->next) {
                           if ((fa->zone==tmpa->addr->zone) && (fa->net==tmpa->addr->net) && 
			       (fa->node==tmpa->addr->node) && (fa->point==tmpa->addr->point) && strchr(fl, flavor))
				if (SearchFidonet(tmpa->addr->zone))
				    check_filebox(temp, &st);
                       }
                       tidy_faddr(fa);
                   }
               }
           }
       }
       closedir(dp);
    }

    if (strlen(CFG.tmaillong) && (dp = opendir(CFG.tmaillong))) {
       temp2 = calloc(PATH_MAX, sizeof(char));
       Syslog('o', "Checking T-Mail long box \"%s\"", CFG.tmaillong);
       while ((de = readdir(dp))) {
           if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
               snprintf(temp, PATH_MAX -1, "%s/%s", CFG.tmaillong, de->d_name);
               if (stat(temp, &sb) == 0) {
                   Syslog('o' ,"checking \"%s\"", de->d_name);
                   if (S_ISDIR(sb.st_mode)) {
                       char c, d;
                       int n;
                       snprintf(temp2, PATH_MAX -1, "%s", de->d_name);
                       fa = (faddr*)malloc(sizeof(faddr));
                       fa->name = NULL;
                       fa->domain = NULL;
                       n = sscanf(temp2, "%u.%u.%u.%u.%c%c", &(fa->zone), &(fa->net), &(fa->node), &(fa->point), &c, &d);
                       if ((n==4) || ((n==5) && (tolower(c)=='h'))) {
                           if (n==4) 
			       flavor = 'o';
                           else 
			       flavor = 'h';
                           for (tmpa = al; tmpa; tmpa = tmpa->next) {
                               if ((fa->zone==tmpa->addr->zone) && (fa->net==tmpa->addr->net) && 
				    (fa->node==tmpa->addr->node) && (fa->point==tmpa->addr->point) && 
				    strchr(fl, flavor))
				  if (SearchFidonet(tmpa->addr->zone)) 
					check_filebox(temp, &st);
                           }
                       }
                       tidy_faddr(fa);
                   }   
               }
           }
       }
       closedir(dp);
       free(temp2);
    }
    free(temp);


    /*
     * For FTS-0001 we need to create at least one packet.
     */
    if (((st == NULL) && (create > 1)) || ((st != NULL) && (packets == 0) && (create > 0))) {
	Syslog('o', "Create packet for %s", ascfnode(al->addr,0x1f));
	if ((fp = openpkt(NULL, al->addr, 'o', TRUE))) {
	    memset(&buffer, 0, sizeof(buffer));
	    fwrite(buffer, 1, 2, fp);
	    fclose(fp);
	}
	add_list(&st, pktname(al->addr,'o'), tmpkname(), KFS, 0L, NULL, 0);
    }

    for (tmpf = st; tmpf; tmpf = tmpf->next)
	Syslog('o',"flist: \"%s\" -> \"%s\" dsp:%d flofp:%u floff:%u",
		MBSE_SS(tmpf->local), MBSE_SS(tmpf->remote), tmpf->disposition,
		(unsigned int)tmpf->flofp, (unsigned int)tmpf->floff);

    return st;
}



/*
 * Create file request list for the Hydra or Binkp protocol.
 */
file_list *create_freqlist(fa_list *al)
{
    file_list	*st = NULL, *tmpf;
    fa_list	*tmpa;
    char	*nm, tmpreq[13];
    struct stat	stbuf;

    Syslog('o', "create_freqlist(%s)", al?ascfnode(al->addr, 0x1f):"<none>");
    made_request = 0;

    for (tmpa = al; tmpa; tmpa = tmpa->next) {
	nm = reqname(tmpa->addr);
	if ((nm != NULL) && (stat(nm, &stbuf) == 0)) {
	    snprintf(tmpreq, 13, "%04X%04X.REQ", tmpa->addr->net, tmpa->addr->node);
	    add_list(&st, nm, tmpreq, DSF, 0L, NULL, 1);
	    made_request = 1;
	}
    }

    if (made_request) {
	for (tmpf = st; tmpf; tmpf = tmpf->next)
	    Syslog('o', "flist: \"%s\" -> \"%s\" dsp:%d flofp:%lu floff:%lu",
			MBSE_SS(tmpf->local), MBSE_SS(tmpf->remote), tmpf->disposition, tmpf->flofp, tmpf->floff);
    }

    return st;
}



void tidy_filelist(file_list *fl, int dsf)
{
    file_list *tmp;

    if (fl == NULL)
	return;

    for (tmp=fl;fl;fl=tmp) {
	tmp=fl->next;
	if (dsf && (fl->disposition == DSF)) {
	    Syslog('o',"Removing sent file \"%s\"",MBSE_SS(fl->local));
	    if (unlink(fl->local) != 0) {
		if (errno == ENOENT)
		    Syslog('o',"Cannot unlink nonexistent file \"%s\"", MBSE_SS(fl->local));
		else
		    WriteError("$Cannot unlink file \"%s\"", MBSE_SS(fl->local));
	    }
	}
	if (fl->local) 
	    free(fl->local);
	if (fl->remote) 
	    free(fl->remote);
	else if (fl->flofp) 
	    fclose(fl->flofp);
	free(fl);
    }
    return;
}



void execute_disposition(file_list *fl)
{
    FILE    *fp=NULL;
    char    *nm, tpl='~';

    Syslog('o', "execute_disposition(%s)", fl->local);
    nm = fl->local;
    if (fl->flofp) {
	/*
	 * Check for special case: flo-file 
	 */
	if (fl->floff == -1) {
	    /*
	     * We check if there are any files left for transmission
	     * in the flo-file to decide whether to remove or leave
	     * it on disk.
	     */
	    char    buf[PATH_MAX];
	    int	    files_remain = 0;

	    if (fseek(fl->flofp, 0L, 0) == 0) {
		while (!feof(fl->flofp) && !ferror(fl->flofp)) {
		    if (fgets(buf, sizeof(buf)-1, fl->flofp) == NULL)
			continue;

		    /*
		     * Count nr of files which haven't been sent yet
		     */
		    if (buf[0] != '~')
			files_remain++;
		}

	    } else {
		WriteError("$Error seeking in .flo to 0");
		files_remain = -1; /* Keep flo-file */
	    }

	    if (files_remain) {
		Syslog('o', "Leaving flo-file \"%s\", %d files remaining", MBSE_SS(nm), files_remain);
		fl->disposition = LEAVE;
	    } else {
		fl->disposition = KFS;
	    }
	} else {
	    /*
	     * Mark files as sent in flo-file
	     */
	    if (fseek(fl->flofp, fl->floff, 0) == 0) {
		if (fwrite(&tpl,1,1,fl->flofp) != 1) {
		    WriteError("$Error writing '~' to .flo at %u", (unsigned int)fl->floff);
		} else {
		    Syslog('o', "Marked file \"%s\" as sent", MBSE_SS(nm));
		}
		fflush(fl->flofp);
#ifdef HAVE_FDATASYNC
		fdatasync(fileno(fl->flofp));
#else
#ifdef HAVE_FSYNC		
		fsync(fileno(fl->flofp));
#endif
#endif
	    } else 
		WriteError("$error seeking in .flo to %u", (unsigned int)fl->floff);
	}
    }

    switch (fl->disposition) {
	case DSF:
	case LEAVE: break;

	case TFS:   Syslog('o', "Truncating sent file \"%s\"",MBSE_SS(nm));
		    if ((fp=fopen(nm,"w"))) 
			fclose(fp);
		    else 
			WriteError("$Cannot truncate file \"%s\"",MBSE_SS(nm));
		    break;

	case KFS:   Syslog('o', "Removing sent file \"%s\"",MBSE_SS(nm));
		    if (unlink(nm) != 0) {
			if (errno == ENOENT)
			    Syslog('o', "Cannot unlink nonexistent file \"%s\"", MBSE_SS(nm));
			else
			    WriteError("$Cannot unlink file \"%s\"", MBSE_SS(nm));
		    }
		    break;

	default:    WriteError("execute_disposition: unknown disp %d for \"%s\"", fl->disposition,MBSE_SS(nm));
		    break;
    }

    return;
}



char *transfertime(struct timeval start, struct timeval end, int bytes, int sent)
{
    static char	    resp[81];
    double long	    startms, endms, elapsed;

    startms = (start.tv_sec * 1000) + (start.tv_usec / 1000);
    endms = (end.tv_sec * 1000) + (end.tv_usec / 1000);
    elapsed = endms - startms;
    memset(&resp, 0, sizeof(resp));
    if (!elapsed)
	elapsed = 1L;
    if (bytes > 1000000)
	snprintf(resp, 81, "%d bytes %s in %0.3Lf seconds (%0.3Lf Kb/s)",
	    bytes, sent?"sent":"received", elapsed / 1000.000, ((bytes / elapsed) * 1000) / 1024);
    else
	snprintf(resp, 81, "%d bytes %s in %0.3Lf seconds (%0.3Lf Kb/s)", 
	    bytes, sent?"sent":"received", elapsed / 1000.000, ((bytes * 1000) / elapsed) / 1024);   
    return resp;
}



char *compress_stat(int original, int saved)
{
    static char	    resp[81];

    snprintf(resp, 81, "compressed %d bytes, compression %0.1f%%", saved, ((saved * 100.0) / original));
    return resp;
}


