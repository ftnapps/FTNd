/*****************************************************************************
 *
 * File ..................: mbfbgen.c
 * Purpose ...............: mbfbgen generates file databases from the old 
 *                          style files.bbs.
 * Last modification date : 28-Jun-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"


struct	stat statfile; 
struct	FILERecord file;


int main(void)
{
	FILE	*pAreas, *pFilesBBS, *pDataBase;
	int	i, j = 0, k = 0, x;
	int	Append = FALSE, Doit, Files = 0, TotalFiles = 0;
	long 	offset;
	long	TotalAreas = 0, StartArea, EndArea, recno = 0;
	char	sFileName[PATH_MAX];
	char	temp[81];
	char	sDataBase[PATH_MAX];
	char	sString1[256];
	char	sUploader[36];
	char	*token = NULL;
	struct	passwd *pw;

#ifdef MEMWATCH
	mwInit();
#endif

	InitConfig();
	TermInit(1);
	umask(002);
	system("clear");
	colour(15, 0);
	printf("\nMBFBGEN: MBSE BBS %s FileBase Generator Utility\n", VERSION);
	colour(14, 0);
	printf("         %s\n\n", Copyright);
	colour(7, 0);

	pw = getpwuid(getuid());
	InitClient(pw->pw_name, (char *)"fbgen", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);

	Syslog(' ', " ");
	Syslog(' ', "MBFBGEN v%s", VERSION);

	sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((pAreas = fopen(temp,"r")) == NULL) {
		WriteError("$Can't open %s", temp);
		printf("Can't open %s\n", temp);
		ExitClient(1);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {
		TotalAreas++;
	}
	fclose(pAreas);

	recno = 0;

	colour(3, 0);
	printf("Total File Areas : %ld\n", TotalAreas);
	printf("File Record Size : %d\n", (int)sizeof(file));

	printf("\nStart at area [1]: ");
	colour(10, 0);
	fgets(temp, 10, stdin);
	if ((strcmp(temp, "")) == 0)
		StartArea = 1L;
	else
		StartArea = atoi(temp);

	colour(3, 0);
	printf("\nStop at area [%ld]: ", TotalAreas);
	colour(10, 0);
	fgets(temp, 10, stdin);
	if ((strcmp(temp, "")) == 0)
		EndArea = TotalAreas;
	else
		EndArea = atoi(temp);

	if ((StartArea < 1L) || (StartArea > TotalAreas)) {
		colour(12, 0);
		printf("\007\nIllegal \"Start\" area %ld\n", StartArea);
		colour(7, 0);
		ExitClient(0);
	}
	if ((EndArea < StartArea) || (EndArea > TotalAreas)) {
		colour(12, 0);
		printf("\007\nIllegal \"End\" area %ld\n", EndArea);
		colour(7, 0);
		ExitClient(0);
	}

	colour(3, 0);
	printf("\nDefault is [Sysop]\n");
	printf("Name of Uploader for files: ");
	colour(10, 0);
	fgets(sUploader, 35, stdin);
	for (i = 0; i < strlen(sUploader); i++)
		if (sUploader[i] == '\n')
			sUploader[i] = '\0';
	if ((strcmp(sUploader, "")) == 0)
		sprintf(sUploader, "Sysop");

	colour(7, 0);
	sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((pAreas = fopen(temp,"r")) == NULL) {
		WriteError("$Can't open %s", temp);
		printf("Can't open %s\n", temp);
		ExitClient(1);
	}
	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	recno = StartArea;

	Syslog('+', "From %ld to %ld, Uploader: \"%s\"", StartArea, EndArea, sUploader);
	colour(14, 0);
	printf("\n\n");

	while (TRUE) {

		offset = areahdr.hdrsize + ((recno -1) * areahdr.recsize);
		if (fseek(pAreas, offset, 0) != 0) {
			WriteError("$Can't seek in fareas.data"); 
			printf("Can't seek in fareas.data\n");
			ExitClient(1); 
		}
		fread(&area, areahdr.recsize, 1, pAreas);

		sprintf(sDataBase,"%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), recno);

		/*
		 * Try to find files.bbs at 2 places, in the subdirectory
		 * with the files, and at the path given in the setup.
		 */
		sprintf(sFileName,"%s/files.bbs", area.Path);
		if ((pFilesBBS = fopen(sFileName,"r")) == NULL) {
			sprintf(sFileName, "%s", area.FilesBbs);
			if ((pFilesBBS = fopen(sFileName, "r")) == NULL)
				WriteError("$Can't process area: %ld %s", recno, area.Name);
		}

		if (pFilesBBS != NULL) {
			Files = 0;

			while (fgets(sString1,255,pFilesBBS) != NULL) {

				if ((sString1[0] != ' ') && (sString1[0] != '\t')) {
					/*
					 * New file entry, check if there hase been a file
					 * that is not saved yet.
					 */
					if (Append) {
						if ((pDataBase = fopen(sDataBase,"a+")) == NULL) {
							WriteError("$Can't open %s", sDataBase);
							ExitClient(1);
						} else {
							fwrite(&file, sizeof(file), 1, pDataBase);
							fclose(pDataBase);
						}
						Append = FALSE;
					}

					Files++;
					TotalFiles++;
					printf("\rArea: %-6ld Fileno: %-6d Total: %-6d", recno, Files, TotalFiles);
					fflush(stdout);

					memset(&file, 0, sizeof(file));

					token = tl(strtok(sString1, " ,\t"));
					strcpy(file.Name, token);
					strcpy(file.LName, token);
					token = strtok(NULL,"\0");
					i = strlen(token);
					j = k = 0;
					for (x = 0; x < i; x++) {
						if ((token[x] == '\n') || (token[x] == '\r'))
							token[x] = '\0';
					}
					Doit = FALSE;
					for (x = 0; x < i; x++) {
						if (!Doit) {
							if (isalnum(token[x]))
								Doit = TRUE;
						}
						if (Doit) {
							if (k > 42) {
								if (token[x] == ' ') {
									file.Desc[j][k] = '\0';
									j++;
									k = 0;
								} else {
									if (k == 49) {
										file.Desc[j][k] = '\0';
										k = 0;
										j++;
									}
									file.Desc[j][k] = token[x];
									k++;
								}
							} else {
								file.Desc[j][k] = token[x];
								k++;
							}
						}
					}

					sprintf(temp,"%s/%s", area.Path, file.Name);
					if (stat(temp,&statfile) != 0) {
						WriteError("Cannot locate file on disk! Skipping... -> %s\n",temp);
						Append = FALSE;
					}

					Append = TRUE;
					file.Size = statfile.st_size;
					file.FileDate = statfile.st_mtime;
					file.Crc32 = file_crc(temp, FALSE);

					strcpy(file.Uploader,sUploader);
					time(&file.UploadDate);
				} else {
					/*
					 * Add multiple description lines
					 */
					token = strtok(sString1, "\0");
					i = strlen(token);
					j++;
					k = 0;
					Doit = FALSE;
					for (x = 0; x < i; x++) {
						if ((token[x] == '\n') || (token[x] == '\r'))
							token[x] = '\0';
					}
					for (x = 0; x < i; x++) {
						if (Doit) {
							if (k > 42) {
								if (token[x] == ' ') {
									file.Desc[j][k] = '\0';
									j++;
									k = 0;
								} else {
									if (k == 49) {
										file.Desc[j][k] = '\0';
										k = 0;
										j++;
									}
									file.Desc[j][k] = token[x];
									k++;
								}
							} else {
								file.Desc[j][k] = token[x];
								k++;
							}
						} else {
							/*
							 * Skip until + or | is found
							 */
							if ((token[x] == '+') || (token[x] == '|'))
								Doit = TRUE;
						}
					}
				}
			} /* End of files.bbs */

			/*
			 * Flush the last file to the database
			 */
			if (Append) {
				if ((pDataBase = fopen(sDataBase, "a+")) == NULL) {
					WriteError("$Can't open %s", sDataBase);
					ExitClient(1);
				} else {
					fwrite(&file, sizeof(file), 1, pDataBase);
					fclose(pDataBase);
				}
				Append = FALSE;
			}
			fclose(pFilesBBS);
		}
		Syslog('+', "Area %ld added %ld files", recno, Files);
		recno++;
		if (recno > EndArea)
			break;
	}
	fclose(pAreas);

	colour(3, 0);
	printf("\r                                            \rAdded total %d files\n", TotalFiles);
	colour(7, 0);

	Syslog('+', "Added total %ld files", TotalFiles);
	Syslog(' ', "MBFBGEN Finished");
	ExitClient(0);
	return 0;
}


