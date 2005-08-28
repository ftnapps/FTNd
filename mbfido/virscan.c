/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Scan for virusses
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "virscan.h"


extern pid_t	mypid;

/*
 * Check for known viri, optional in a defined path.
 */
int VirScan(char *path)
{
    char    *pwd, *temp, *stdlog, *errlog, buf[256];
    FILE    *fp, *lp;
    int	    vrc, rc = FALSE, has_scan = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));
    stdlog = calloc(PATH_MAX, sizeof(char));
    errlog = calloc(PATH_MAX, sizeof(char));
    
    snprintf(temp,   PATH_MAX, "%s/etc/virscan.data", getenv("MBSE_ROOT"));
    snprintf(stdlog, PATH_MAX, "%s/tmp/stdlog%d", getenv("MBSE_ROOT"), mypid);
    snprintf(errlog, PATH_MAX, "%s/tmp/errlog%d", getenv("MBSE_ROOT"), mypid);

    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("No virus scanners defined");
	free(temp);
	return FALSE;
    }
    fread(&virscanhdr, sizeof(virscanhdr), 1, fp);

    while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
	if (virscan.available)
	    has_scan = TRUE;
    }
    if (!has_scan) {
	Syslog('+', "No active virus scanners, skipping scan");
	fclose(fp);
	free(temp);
	return FALSE;
    }
    
    pwd = calloc(PATH_MAX, sizeof(char));
    getcwd(pwd, PATH_MAX);
    if (path) {
	chdir(path);
	Syslog('+', "Start virusscan in %s", path);
    } else {
	Syslog('+', "Start virusscan in %s", pwd);
    }
    
    fseek(fp, virscanhdr.hdrsize, SEEK_SET);
    while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
        if (virscan.available) {
	    Altime(3600);
	    vrc = execute_str(virscan.scanner, virscan.options, (char *)NULL, (char *)"/dev/null", stdlog, errlog);
	    if (file_size(stdlog)) {
		if ((lp = fopen(stdlog, "r"))) {
		    while (fgets(buf, sizeof(buf) -1, lp)) {
			Striplf(buf);
			Syslog('+', "stdout: \"%s\"", printable(buf, 0));
		    }
		    fclose(lp);
		}
	    }
	    if (file_size(errlog)) {
		if ((lp = fopen(errlog, "r"))) {
		    while (fgets(buf, sizeof(buf) -1, lp)) {
			Striplf(buf);
			Syslog('+', "stderr: \"%s\"", printable(buf, 0));
		    }
		    fclose(lp);
		}
	    }
	    unlink(stdlog);
	    unlink(errlog);
            if (vrc != virscan.error) {
                Syslog('!', "Virus found by %s", virscan.comment);
		rc = TRUE;
            }
	    Altime(0);
	    Nopper();
        }
    }
    fclose(fp);

    if (path)
	chdir(pwd);

    free(pwd);
    free(temp);
    free(stdlog);
    free(errlog);
    return rc;
}


