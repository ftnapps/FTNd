/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Give status of all filesystems
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10		Internet:	mbse@user.sourceforge.net
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "../lib/structs.h"
#include "taskdisk.h"
#include "taskutil.h"



/*
 * This function returns the information of all mounted filesystems,
 * but no more then 10 filesystems.
 */
char *get_diskstat()
{
	char		*mtab, *dev, *fs, *type, *tmp = NULL;
	FILE		*fp;
	int		i = 0;
	static char	buf[SS_BUFSIZE];
	struct statfs	sfs;
	char		tt[80];
	unsigned long	temp;

	buf[0] = '\0';
	mtab = calloc(PATH_MAX, sizeof(char));
#ifdef __linux__
	if ((fp = fopen((char *)"/etc/mtab", "r")) == 0) {
#elif __FreeBSD__ || __NetBSD__
	if ((fp = fopen((char *)"/etc/fstab", "r")) == 0) {
#endif
		sprintf(buf, "100:0;");
		return buf;
	}

	while (fgets(mtab, 255, fp)) {
		dev  = strtok(mtab, " \t");
		fs   = strtok(NULL, " \t");
		type = strtok(NULL, " \t");
		if (strncmp((char *)"/dev/", dev, 5) == 0) {
			if (statfs(fs, &sfs) == 0) {
				i++;
				if (tmp == NULL)
					tmp = xstrcpy((char *)",");
				else
					tmp = xstrcat(tmp, (char *)",");
				tt[0] = '\0';
				temp = (unsigned long)(sfs.f_bsize / 512L);
				sprintf(tt, "%lu %lu %s %s", 
					(unsigned long)(sfs.f_blocks * temp) / 2048L,
					(unsigned long)(sfs.f_bavail * temp) / 2048L,
					fs, type);
				tmp = xstrcat(tmp, tt);
			}
			if (i == 10) /* No more then 10 filesystems */
				break;
		}
	}
	fclose(fp);

	if (strlen(tmp) > (SS_BUFSIZE - 8))
		sprintf(buf, "100:0;");
	else
		sprintf(buf, "100:%d%s;", i, tmp);

	return buf;
}



