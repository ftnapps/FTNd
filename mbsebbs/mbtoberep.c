/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Show contents of toberep.data
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


int main(int argc, char **argv)
{
	char			*BBSpath;
	char			*temp;
	FILE			*fp;
	struct _filerecord	rep;
	int			i;

	if ((BBSpath = getenv("MBSE_ROOT")) == NULL) {
		printf("MBSE_ROOT variable not set\n");
		exit(MBERR_INIT_ERROR);
	}

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/toberep.data", BBSpath);

	if ((fp = fopen(temp, "r")) == NULL) {
		printf("File %s not found\n", temp);
		free(temp);
		exit(MBERR_INIT_ERROR);
	}

	while (fread(&rep, sizeof(rep), 1, fp) == 1) {
		printf("File echo     %s\n", rep.Echo);
		printf("Comment       %s\n", rep.Comment);
		printf("Group         %s\n", rep.Group);
		printf("Short name    %s\n", rep.Name);
		printf("Long name     %s\n", rep.LName);
		printf("File size     %lu\n", (long)(rep.Size));
		printf("File size Kb  %lu\n", rep.SizeKb);
		printf("File date     %s", ctime(&rep.Fdate));
		printf("File CRC      %s\n", rep.Crc);
		printf("Origin system %s\n", rep.Origin);
		printf("From system   %s\n", rep.From);
		printf("Replace       %s\n", rep.Replace);
		printf("Magic         %s\n", rep.Magic);
		printf("Cost          %ld\n", rep.Cost);
		printf("Announce      ");
		if (rep.Announce)
			printf("yes\n");
		else
			printf("no\n");
		printf("Description   %s\n", rep.Desc);
		for (i = 0; i < rep.TotLdesc; i++) {
			printf("   %2d         %s\n", (int)strlen(rep.LDesc[i]), rep.LDesc[i]);
		}
		printf("\n\n");
	}

	fclose(fp);
	chmod(temp, 0640);
	free(temp);
	return 0;
}



