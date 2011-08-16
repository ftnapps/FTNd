/*****************************************************************************
 *
 * $Id: attach.c,v 1.30 2005/11/19 20:22:41 mbse Exp $
 * Purpose ...............: Attach files to outbound
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



/*
 * Attach a file to the real outbound fo a given node.
 */
int attach(faddr noden, char *ofile, int mode, char flavor)
{
    FILE    *fp;
    char    *flofile, *thefile;
    int	    rc;

    if (ofile == NULL)
	return FALSE;

    if ((rc = file_exist(ofile, R_OK))) {
	WriteError("attach: file %s failed, %s", ofile, strerror(rc));
	return FALSE;
    }

    /*
     * Check if we attach a file with the same name
     */
    un_attach(&noden, ofile);

    flofile = calloc(PATH_MAX, sizeof(char));
    thefile = calloc(PATH_MAX, sizeof(char));
    snprintf(flofile, PATH_MAX -1, "%s", floname(&noden, flavor));

    /*
     * Check if outbound directory exists and 
     * create if it doesn't exist.
     */
    mkdirs(ofile, 0770);

    /*
     *  Attach file to .flo
     *
     *  Note that mbcico when connected to a node opens the file "r+",
     *  locks it with fcntl(F_SETLK), F_RDLCK, whence=0, start=0L, len=0L.
     *  It seems that this lock is released after the files in the .flo
     *  files are send. I don't know what will happen if we add entries
     *  to the .flo files, this must be tested!
     */
    if ((fp = fopen(flofile, "a+")) == NULL) {
	WriteError("$Can't open %s", flofile);
	WriteError("May be locked by mbcico");
	free(flofile);
	free(thefile);
	return FALSE;
    }

    switch (mode) {
	case LEAVE:
	    if (strlen(CFG.dospath)) {
		if (CFG.leavecase)
		    snprintf(thefile, PATH_MAX -1, "@%s", Unix2Dos(ofile));
		else
		    snprintf(thefile, PATH_MAX -1, "@%s", tu(Unix2Dos(ofile)));
	    } else {
		snprintf(thefile, PATH_MAX -1, "@%s", ofile);
	    }
	    break;

	case KFS:
	    if (strlen(CFG.dospath)) {
		if (CFG.leavecase)
		    snprintf(thefile, PATH_MAX -1, "^%s", Unix2Dos(ofile));
		else
		    snprintf(thefile, PATH_MAX -1, "^%s", tu(Unix2Dos(ofile)));
	    } else {
		snprintf(thefile, PATH_MAX -1, "^%s", ofile);
	    }
	    break;

	case TFS:
	    if (strlen(CFG.dospath)) {
		if (CFG.leavecase)
		    snprintf(thefile, PATH_MAX -1, "#%s", Unix2Dos(ofile));
		else
		    snprintf(thefile, PATH_MAX -1, "#%s", tu(Unix2Dos(ofile)));
	    } else {
		snprintf(thefile, PATH_MAX -1, "#%s", ofile);
	    }
	    break;
    }

    fseek(fp, 0, SEEK_END);
    fprintf(fp, "%s\r\n", thefile);
    fclose(fp);
    free(flofile);
    free(thefile);
    return TRUE;
}



int is_my_tic(char *, char *);
int is_my_tic(char *filename, char *ticfile)
{
    FILE    *fp;
    char    *buf;
    int	    Found = FALSE;

    buf = calloc(81, sizeof(char));

    if ((fp = fopen(ticfile, "r"))) {
	while (fgets(buf, 80, fp)) {
	    if (strstr(buf, filename)) {
		Found = TRUE;
		break;
	    }
	}
	fclose(fp);
    }

    free(buf);
    return Found;
}



/*
 * The real unatach function, return 1 if a file is removed.
 */
int check_flo(faddr *, char *, char);
int check_flo(faddr *node, char *filename, char flavor)
{
    char    *flofile, *ticfile, *buf;
    FILE    *fp;
    int	    filepos, newpos;
    char    tpl = '~';
    int	    rc = 0;

    buf = calloc(PATH_MAX+3, sizeof(char));
    flofile = calloc(PATH_MAX, sizeof(char));
    ticfile = calloc(PATH_MAX, sizeof(char));

    snprintf(flofile, PATH_MAX -1, "%s", floname(node, flavor));
    if ((fp = fopen(flofile, "r+"))) {
	filepos = 0;
	while (fgets(buf, PATH_MAX +2, fp)) {
	    newpos = ftell(fp);
	    Striplf(buf);
	    if (buf[strlen(buf)-1] == '\r')
		buf[strlen(buf)-1] = '\0';
	    if (((strcmp(buf, filename) == 0) || (strcmp(buf+1, filename) == 0)) && (buf[0] != '~')) {
		fseek(fp, filepos, SEEK_SET);
		fwrite(&tpl, 1, 1, fp);
		fflush(fp);
		fseek(fp, newpos, SEEK_SET);
		filepos = newpos;
		if (fgets(buf, PATH_MAX +2, fp)) {
		    Striplf(buf);
		    if (buf[strlen(buf)-1] == '\r')
			buf[strlen(buf)-1] = '\0';
		    if (strstr(buf, ".tic") && is_my_tic(basename(filename), buf+1)) {
			unlink(buf+1);
			fseek(fp, filepos, SEEK_SET);
			fwrite(&tpl, 1, 1, fp);
			fflush(fp);
			Syslog('+', "Removed old %s and %s for %s from outbound", 
				basename(filename), basename(buf+1), ascfnode(node, 0x1f));
		    } else {
			Syslog('+', "Removed old %s for %s from outbound", basename(filename), ascfnode(node, 0x1f));
		    }
		} else {
		    Syslog('+', "Removed old %s for %s from outbound", basename(filename), ascfnode(node, 0x1f));
		}
		rc = 1;
		break;
	    }
	    filepos = ftell(fp);
	}
	fclose(fp);
    }

    free(ticfile);
    free(flofile);
    free(buf);

    return rc;
}



/*
 * Remove a file from the flofile, also search for a .tic file.
 */
void un_attach(faddr *node, char *filename)
{
    char    *base, *allname;

    Syslog('p', "un_attach: %s %s", ascfnode(node, 0x1f), filename);
    allname = xstrcpy(filename);
    base = basename(allname);

    if ((strlen(base) == 12) && ((strncasecmp(base+8,".su",3) == 0) ||
	(strncasecmp(base+8,".mo",3) == 0) || (strncasecmp(base+8,".tu",3) == 0) ||
	(strncasecmp(base+8,".we",3) == 0) || (strncasecmp(base+8,".th",3) == 0) ||
	(strncasecmp(base+8,".fr",3) == 0) || (strncasecmp(base+8,".sa",3) == 0) ||
	(strncasecmp(base+8,".tic",4) == 0))) {
	free(allname);
	return;
    }
    free(allname);

    if (check_flo(node, filename, 'h') == 0)
	if (check_flo(node, filename, 'f') == 0)
	    check_flo(node, filename, 'c');
}

