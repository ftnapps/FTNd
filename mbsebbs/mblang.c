/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Language Compiler
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/mberrors.h"


int main(int argc, char **argv)
{
	FILE	*fp, *fp1;
	int	i, j, lines;
	char	*temp, *temp1;

#ifdef MEMWATCH
        mwInit();
#endif
	temp  = calloc(PATH_MAX, sizeof(char));
	temp1 = calloc(PATH_MAX, sizeof(char));  

	printf("\nMBLANG: MBSE BBS %s Quick Language Data File Creator\n", VERSION);
	printf("        %s\n", COPYRIGHT);

	if (argc < 3) {
		printf("\nUsage: %s [language data file] [language text file]\n\n", *(argv));
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(MBERR_COMMANDLINE);
	}

	sprintf(temp1, "%s", *(argv + 1));
	unlink(temp1);

	sprintf(temp, "%s", *(argv + 2));
	if ((fp1 = fopen(temp, "r")) == NULL) {
		printf("\nUnable to open %s\n", temp);
#ifdef MEMWATCH
                mwTerm();
#endif
		exit(MBERR_COMMANDLINE);
	}
	sprintf(temp1, "%s", *(argv + 1));
	if ((fp = fopen(temp1, "a+")) == NULL) {
		printf("\nUnable to open %s\n", temp1);
#ifdef MEMWATCH
                mwTerm();
#endif
		exit(MBERR_COMMANDLINE);
	}

	lines = 0;
	while (fgets(temp, 115, fp1) != NULL) {

		memset(&ldata, 0, sizeof(ldata));

		/*
		 * Take the response keys part
		 */
		for(i = 0; i < strlen(temp); i++) {
			if(temp[i] == '|')
				break;
			ldata.sKey[i] = temp[i];
		}
		if (i > 29) {
			printf("\nKey part in line %d too long (%d chars)", lines, i);
#ifdef MEMWATCH
                	mwTerm();
#endif
			exit(MBERR_GENERAL);
		}

		/*
		 * Take the prompt string part
		 */
		j = 0;
		for (i = i+1; i < strlen(temp); i++) {
			if (temp[i] == '\n')
				break;
			ldata.sString[j] = temp[i];
			j++;
		}
		if (j > 84) {
			printf("\nLanguage string in line %d too long (%d chars)", lines, j);
#ifdef MEMWATCH
                	mwTerm();
#endif
			exit(MBERR_GENERAL);
		}

		fwrite(&ldata, sizeof(ldata), 1, fp);
		lines++;
	}

	fclose(fp);
	fclose(fp1);
	free(temp);
	free(temp1);

	printf("\nCompiled %d language lines\n", lines);

#ifdef MEMWATCH
        mwTerm();
#endif
	return 0;
}


