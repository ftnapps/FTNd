/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: fidonet mailer 
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "config.h"
#include "session.h"
#include "filelist.h"


#ifndef PATH_MAX
#define PATH_MAX 512
#endif

extern int master;
int made_request;



static char *tmpkname(void);
static char *tmpkname(void)
{
	static char buf[16];

	sprintf(buf,"%08lx.pkt", sequencer());
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
	Syslog('o', "name \"%s\" converted to \"%s\"", MBSE_SS(orig), MBSE_SS(buf));
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
	file_list **tmpl;
	file_list *tmp;

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
	char	buf[PATH_MAX],buf2[PATH_MAX],*p,*q;
	int	disposition;
	struct	stat stbuf;

	Syslog('O', "check_flo(\"%s\")",MBSE_SS(nm));

	if ((fp = fopen(nm,"r+")) == NULL) {
		Syslog('O',"no flo file");
		return;
	}
	fl.l_type   = F_RDLCK;
	fl.l_whence = 0;
	fl.l_start  = 0L;
	fl.l_len    = 0L;
	if (fcntl(fileno(fp), F_SETLK, &fl) != 0) {
		if (errno != EAGAIN)
			WriteError("$cannot read-lock \"%s\"",MBSE_SS(nm));
		else 
			Syslog('O',"flo file busy");
		fclose(fp);
		return;
	}

	if (stat(nm, &stbuf) != 0) {
		WriteError("$cannot access \"%s\"",MBSE_SS(nm));
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
			case 0:		continue;
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
		} else {
			if (strncasecmp(p, CFG.uxpath, strlen(CFG.uxpath)) == 0) {
				for (p=p, q=buf2+strlen(buf2); *p; p++, q++)
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



file_list *create_filelist(fa_list *al, char *fl, int create)
{
	file_list	*st=NULL;
	file_list	*tmpf;
	fa_list		*tmpa;
	char		flavor, *tmpfl;
	char		*nm;
	char		tmpreq[13];
	struct stat	stbuf;
	int		packets = 0;
	FILE		*fp;
	unsigned char	buffer[2];

	Syslog('o', "Create_filelist(%s,\"%s\",%d)", al?ascfnode(al->addr,0x1f):"<none>", MBSE_SS(fl), create);
	made_request = 0;

	for (tmpa = al; tmpa; tmpa = tmpa->next) {
		Syslog('o', "Check address %s", ascfnode(tmpa->addr, 0x1f));

		/*
		 * Check spool files.
		 */
		if (strchr(fl, 'o')) {
			nm=splname(tmpa->addr);
			if ((fp=fopen(nm,"w"))) 
				fclose(fp);
			if ((nm != NULL) && (stat(nm,&stbuf) == 0))
				add_list(&st,nm,NULL,DSF,0L,NULL,1);
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
			/*
			 * Check normal mail packets
			 */
			nm=pktname(tmpa->addr,flavor);
			if ((nm != NULL) && (stat(nm,&stbuf) == 0)) {
				packets++;
				add_list(&st,nm,tmpkname(),KFS,0L,NULL,1);
			}

			/*
			 * Check .flo files for file attaches
			 */
			nm=floname(tmpa->addr,flavor);
			check_flo(&st,nm);
		}

		if ((session_flags & SESSION_WAZOO) &&
		    ((session_flags & SESSION_HYDRA) == 0) &&
		    (master || ((session_flags & SESSION_IFNA) == 0))) {
			/*
			 * we don't distinguish flavoured reqs
			 */
			nm=reqname(tmpa->addr);
			if ((nm != NULL) && (stat(nm,&stbuf) == 0)) {
				sprintf(tmpreq,"%04X%04X.REQ", tmpa->addr->net,tmpa->addr->node);
				add_list(&st,nm,tmpreq,DSF,0L,NULL,1);
				made_request = 1;
			}
		}
	}

	if (((st == NULL) && (create > 1)) || ((st != NULL) && (packets == 0) && (create > 0))) {
		Syslog('o',"Create packet for %s",ascfnode(al->addr,0x1f));
		if ((fp = openpkt(NULL, al->addr, 'o'))) {
			memset(&buffer, 0, sizeof(buffer));
			fwrite(buffer, 1, 2, fp);
			fclose(fp);
		}
		add_list(&st,pktname(al->addr,'o'),tmpkname(),KFS,0L,NULL,0);
	}

	for (tmpf = st; tmpf; tmpf = tmpf->next)
		Syslog('O',"flist: \"%s\" -> \"%s\" dsp:%d flofp:%lu floff:%lu",
			MBSE_SS(tmpf->local), MBSE_SS(tmpf->remote), tmpf->disposition,
			(unsigned long)tmpf->flofp, (unsigned long)tmpf->floff);

	return st;
}



/*
 * Create file request list for the Hydra or Binkp protocol.
 */
file_list *create_freqlist(fa_list *al)
{
	file_list	*st = NULL, *tmpf;
	fa_list		*tmpa;
	char		*nm;
	char		tmpreq[13];
	struct stat	stbuf;

	Syslog('o', "create_freqlist(%s)", al?ascfnode(al->addr, 0x1f):"<none>");
	made_request = 0;

	for (tmpa = al; tmpa; tmpa = tmpa->next) {
		nm = reqname(tmpa->addr);
		if ((nm != NULL) && (stat(nm, &stbuf) == 0)) {
			sprintf(tmpreq, "%04X%04X.REQ", tmpa->addr->net, tmpa->addr->node);
			add_list(&st, nm, tmpreq, DSF, 0L, NULL, 1);
			made_request = 1;
		}
	}

	for (tmpf = st; tmpf; tmpf = tmpf->next)
		Syslog('O', "flist: \"%s\" -> \"%s\" dsp:%d flofp:%lu floff:%lu",
			MBSE_SS(tmpf->local), MBSE_SS(tmpf->remote), tmpf->disposition,
			tmpf->flofp, tmpf->floff);

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
	FILE	*fp=NULL;
	char	*nm;
	char	tpl='~';

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
			char buf[PATH_MAX];
			int files_remain = 0;

			if (fseek(fl->flofp, 0L, 0) == 0) {
				while (!feof(fl->flofp) && !ferror(fl->flofp)) {
					if (fgets(buf, sizeof(buf)-1, fl->flofp) == NULL)
						continue;

					/*
					 * Count nr of files which haven't been
					 * sent yet
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
					WriteError("$Error writing '~' to .flo at %lu", (unsigned long)fl->floff);
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
				WriteError("$error seeking in .flo to %lu", (unsigned long)fl->floff);
		}
	}

	switch (fl->disposition) {
	case DSF:
	case LEAVE:	
			break;
	case TFS:	
			Syslog('o', "Truncating sent file \"%s\"",MBSE_SS(nm));
			if ((fp=fopen(nm,"w"))) 
				fclose(fp);
			else 
				WriteError("$Cannot truncate file \"%s\"",MBSE_SS(nm));
			break;
	case KFS:	
			Syslog('o', "Removing sent file \"%s\"",MBSE_SS(nm));
			if (unlink(nm) != 0) {
				if (errno == ENOENT)
					Syslog('o', "Cannot unlink nonexistent file \"%s\"", MBSE_SS(nm));
				else
					WriteError("$Cannot unlink file \"%s\"", MBSE_SS(nm));
			}
			break;
	default:	WriteError("execute_disposition: unknown disp %d for \"%s\"",
				fl->disposition,MBSE_SS(nm));
			break;
	}

	return;
}


