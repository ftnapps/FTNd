/*****************************************************************************
 *
 * ftnlang.c
 * Purpose ...............: Language Compiler
 *
 *****************************************************************************
 * Copyright (C)    2012   Robert James Clay <jame@rocasa.us>
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"


int main(int argc, char **argv)
{
	FILE	*fp, *fp1;
	int	i, j, lines;
	char	*temp, *temp1;

	temp  = calloc(PATH_MAX, sizeof(char));
	temp1 = calloc(PATH_MAX, sizeof(char));  

	printf("\nFTNLANG: FTNd %s Quick Language Data File Creator\n", VERSION);
	printf("        %s\n", COPYRIGHT);

	if (argc < 3) {
		printf("\nUsage: %s [language data file] [language text file]\n\n", *(argv));
		exit(FTNERR_COMMANDLINE);
	}

	snprintf(temp1, PATH_MAX, "%s", *(argv + 1));
	unlink(temp1);

	snprintf(temp, PATH_MAX, "%s", *(argv + 2));
	if ((fp1 = fopen(temp, "r")) == NULL) {
		printf("\nUnable to open %s\n", temp);
		exit(FTNERR_COMMANDLINE);
	}
	snprintf(temp1, PATH_MAX, "%s", *(argv + 1));
	if ((fp = fopen(temp1, "a+")) == NULL) {
		printf("\nUnable to open %s\n", temp1);
		exit(FTNERR_COMMANDLINE);
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
			exit(FTNERR_GENERAL);
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
			exit(FTNERR_GENERAL);
		}

		fwrite(&ldata, sizeof(ldata), 1, fp);
		lines++;
	}

	fclose(fp);
	fclose(fp1);
	free(temp);
	free(temp1);

	printf("\nCompiled %d language lines\n", lines);

	return 0;
}


