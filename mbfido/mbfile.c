/*****************************************************************************
 *
 * File ..................: mbfile/mbfile.c
 * Purpose ...............: File Database Maintenance
 * Last modification date : 25-May-2001
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
#include "mbfile.h"



extern int	do_quiet;		/* Supress screen output	    */
int		do_pack  = FALSE;	/* Pack filebase		    */
int		do_check = FALSE;	/* Check filebase		    */
int		do_kill  = FALSE;	/* Kill/move old files		    */
int		do_index = FALSE;	/* Create request index		    */
extern	int	e_pid;			/* Pid of external process	    */
extern	int	show_log;		/* Show logging			    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */
int		marker = 0;		/* Marker counter		    */


typedef struct _Index {
	struct _Index		*next;
	struct FILEIndex	idx;
} Findex;


void ProgName(void)
{
	if (do_quiet)
		return;

	colour(15, 0);
	printf("\nMBFILE: MBSE BBS %s File maintenance utility\n", VERSION);
	colour(14, 0);
	printf("        %s\n", Copyright);
}



void die(int onsig)
{
	/*
	 * First check if a child is running, if so, kill it.
	 */
	if (e_pid) {
		if ((kill(e_pid, SIGTERM)) == 0)
			Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
		else {
			if ((kill(e_pid, SIGKILL)) == 0)
				Syslog('+', "SIGKILL to pid %d succeded", e_pid);
			else
				WriteError("$Failed to kill pid %d", e_pid);
		}

		/*
		 * In case the child had the tty in raw mode...
		 */
		if (!do_quiet)
			system("stty sane");
	}

	signal(onsig, SIG_IGN);

	if (onsig) {
		if (onsig <= NSIG)
			WriteError("$Terminated on signal %d (%s)", onsig, SigName[onsig]);
		else
			WriteError("Terminated with error %d", onsig);
	}

	time(&t_end);
	Syslog(' ', "MBFILE finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet) {
		colour(7, 0);
		printf("\n");
	}
	ExitClient(onsig);
}



int main(int argc, char **argv)
{
	int	i;
	char	*cmd;
	struct	passwd *pw;

#ifdef MEMWATCH
	mwInit();
#endif
	InitConfig();
	TermInit(1);
	time(&t_start);
	umask(002);

	/*
	 * Catch all signals we can, and ignore the rest.
	 */
	for (i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGKILL) ||
		    (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
			signal(i, (void (*))die);
		else
			signal(i, SIG_IGN);
	}

	if(argc < 2)
		Help();

	cmd = xstrcpy((char *)"Command line: mbfile");

	for (i = 1; i < argc; i++) {

		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, tl(argv[i]));

		if (!strncmp(argv[i], "i", 1))
			do_index = TRUE;
		if (!strncmp(argv[i], "p", 1))
			do_pack = TRUE;
		if (!strncmp(argv[i], "c", 1))
			do_check = TRUE;
		if (!strncmp(argv[i], "k", 1))
			do_kill = TRUE;
		if (!strncmp(argv[i], "-q", 2))
			do_quiet = TRUE;
	}

	if (!(do_pack || do_check || do_kill || do_index))
		Help();

	ProgName();
	pw = getpwuid(getuid());
	InitClient(pw->pw_name, (char *)"mbfile", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);

	Syslog(' ', " ");
	Syslog(' ', "MBFILE v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet)
		printf("\n");

	if (!diskfree(CFG.freespace))
		die(101);

	if (do_kill)
		Kill();

	if (do_check)
		Check();

	if (do_pack)
		PackFileBase();

	if (do_index)
		Index();

	die(0);
	return 0;
}



void Help(void)
{
	do_quiet = FALSE;
	ProgName();

	colour(11, 0);
	printf("\nUsage:	mbfile [command] <options>\n\n");
	colour(9, 0);
	printf("	Commands are:\n\n");
	colour(3, 0);
	printf("	c  check	Check filebase\n");
	printf("	i  index	Create filerequest index\n");
	printf("	k  kill		Kill/move old files\n");
	printf("	p  pack		Pack filebase\n");
	colour(9, 0);
	printf("\n	Options are:\n\n");
	colour(3, 0);
	printf("	-q -quiet	Quiet mode\n");
	colour(7, 0);
	printf("\n");
	die(0);
}



void Marker(void)
{
	/*
	 * Keep the connection with the server alive
	 */
	Nopper();

	/*
	 * Release system resources when running in the background
	 */
	if (CFG.slow_util && do_quiet)
		usleep(1);

	if (do_quiet)
		return;

	switch (marker) {
		case 0:	printf(">---");
			break;

		case 1:	printf(">>--");
			break;

		case 2:	printf(">>>-");
			break;

		case 3:	printf(">>>>");
			break;

		case 4:	printf("<>>>");
			break;

		case 5:	printf("<<>>");
			break;

		case 6:	printf("<<<>");
			break;

		case 7:	printf("<<<<");
			break;

		case 8: printf("-<<<");
			break;

		case 9: printf("--<<");
			break;

		case 10:printf("---<");
			break;

		case 11:printf("----");
			break;
	}
	printf("\b\b\b\b");
	fflush(stdout);

	if (marker < 11)
		marker++;
	else
		marker = 0;
}



/*
 *  Check files for age, and not downloaded for x days. If they match
 *  one of these criteria (setable in areas setup), the file will be
 *  move to some retire area or deleted, depending on the setup.
 *  If they are moved, the upload date is reset to the current date,
 *  so you can set new removal criteria again.
 */
void Kill(void)
{
	FILE	*pAreas, *pFile, *pDest, *pTemp;
	int	i, iAreas, iAreasNew = 0;
	int	iTotal = 0, iKilled =  0, iMoved = 0;
	char	*sAreas, *fAreas, *newdir = NULL, *sTemp;
	time_t	Now;
	int	rc, Killit, FilesLeft;
	struct	fileareas darea;
	char	from[128], to[128];

	sAreas = calloc(81, sizeof(char));
	fAreas = calloc(81, sizeof(char));
	sTemp  = calloc(81, sizeof(char));

	IsDoing("Kill files");
	if (!do_quiet) {
		colour(3, 0);
		printf("Kill/move files...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("Can't open %s", sAreas);
		die(0);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	fseek(pAreas, 0, SEEK_END);
	iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;
	Now = time(NULL);

	for (i = 1; i <= iAreas; i++) {

		fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
		fread(&area, areahdr.recsize, 1, pAreas);

		if ((area.Available) && (area.DLdays || area.FDdays) && (!area.CDrom)) {

			if (!diskfree(CFG.freespace))
				die(101);

			if (!do_quiet) {
				printf("\r%4d => %-44s    \b\b\b\b", i, area.Name);
				fflush(stdout);
			}

			/*
			 * Check if download directory exists, 
			 * if not, create the directory.
			 */
			if (access(area.Path, R_OK) == -1) {
				Syslog('!', "Create dir: %s", area.Path);
				newdir = xstrcpy(area.Path);
				newdir = xstrcat(newdir, (char *)"/");
				mkdirs(newdir);
				free(newdir);
				newdir = NULL;
			}

			sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);

			/*
			 * Open the file database, if it doesn't exist,
			 * create an empty one.
			 */
			if ((pFile = fopen(fAreas, "r+")) == NULL) {
				Syslog('!', "Creating new %s", fAreas);
				if ((pFile = fopen(fAreas, "a+")) == NULL) {
					WriteError("$Can't create %s", fAreas);
					die(0);
				}
			} 

			/*
			 * Now start checking the files in the filedatabase
			 * against the contents of the directory.
			 */
			while (fread(&file, sizeof(file), 1, pFile) == 1) {
				iTotal++;
				Marker();

				Killit = FALSE;
				if (area.DLdays) {
					/*
					 * Test last download date or never downloaded and the
					 * file is more then n days available for download.
					 */
					if ((file.LastDL) && 
					    (((Now - file.LastDL) / 84400) > area.DLdays)) {
						Killit = TRUE;
					}
					if ((!file.LastDL) && 
					    (((Now - file.UploadDate) / 84400) > area.DLdays)) {
						Killit = TRUE;
					}
				}

				if (area.FDdays) {
					/*
					 * Check filedate
					 */
					if (((Now - file.UploadDate) / 84400) > area.FDdays) {
						Killit = TRUE;
					}
				}

				if (Killit) {
					do_pack = TRUE;
					if (area.MoveArea) {
						fseek(pAreas, ((area.MoveArea -1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
						fread(&darea, areahdr.recsize, 1, pAreas);
						sprintf(from, "%s/%s", area.Path, file.Name);
						sprintf(to,   "%s/%s", darea.Path, file.Name);
						if ((rc = file_mv(from, to)) == 0) {
							Syslog('+', "Move %s, area %d => %d", file.Name, i, area.MoveArea);
							sprintf(to, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), area.MoveArea);
							if ((pDest = fopen(to, "a+")) != NULL) {
								file.UploadDate = time(NULL);
								file.LastDL = time(NULL);
								fwrite(&file, sizeof(file), 1, pDest);
								fclose(pDest);
							}
							/*
							 * Now again if there is a dotted version (thumbnail) of this file.
							 */
							sprintf(from, "%s/.%s", area.Path, file.Name);
							sprintf(to,   "%s/.%s", darea.Path, file.Name);
							if (file_exist(from, R_OK) == 0)
								file_mv(from, to);
							file.Deleted = TRUE;
							fseek(pFile, - sizeof(file), SEEK_CUR);
							fwrite(&file, sizeof(file), 1, pFile);
							iMoved++;
						} else {
							WriteError("Move %s failed rc = %d", file.Name, rc);
						}
					} else {
						Syslog('+', "Delete %s, area %d", file.Name, i);
						file.Deleted = TRUE;
						fseek(pFile, - sizeof(file), SEEK_CUR);
						fwrite(&file, sizeof(file), 1, pFile);
						iKilled++;
						sprintf(from, "%s/%s", area.Path, file.Name);
						unlink(from);
					}
				}
			}

			/*
			 * Now we must pack this area database otherwise
			 * we run into trouble later on.
			 */
			fseek(pFile, 0, SEEK_SET);
			sprintf(sTemp, "%s/fdb/fdbtmp.data", getenv("MBSE_ROOT"));

			if ((pTemp = fopen(sTemp, "a+")) != NULL) {
				FilesLeft = FALSE;
				while (fread(&file, sizeof(file), 1, pFile) == 1) {
					if ((!file.Deleted) && strcmp(file.Name, "") != 0) {
						fwrite(&file, sizeof(file), 1, pTemp);
						FilesLeft = TRUE;
					}
				}

				fclose(pFile);
				fclose(pTemp);
				if ((rename(sTemp, fAreas)) == 0) {
					unlink(sTemp);
					chmod(fAreas, 006600);
				}
				if (!FilesLeft) {
					Syslog('+', "Warning: area %d (%s) is empty", i, area.Name);
				}
			} else 
				fclose(pFile);

			iAreasNew++;

		} /* if area.Available */
	}

	fclose(pAreas);

	Syslog('+', "Kill  Areas [%5d] Files [%5d] Deleted [%5d] Moved [%5d]", iAreasNew, iTotal, iKilled, iMoved);

	if (!do_quiet) {
		printf("\r                                                          \r");
		fflush(stdout);
	}

	free(sTemp);
	free(sAreas);
	free(fAreas);
}



void tidy_index(Findex **);
void tidy_index(Findex **fap)
{
	Findex	*tmp, *old;

	for (tmp = *fap; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fap = NULL;
}



void fill_index(struct FILEIndex, Findex **);
void fill_index(struct FILEIndex idx, Findex **fap)
{
	Findex	*tmp;

	tmp = (Findex *)malloc(sizeof(Findex));
	tmp->next = *fap;
	tmp->idx  = idx;
	*fap = tmp;
}


int comp_index(Findex **, Findex **);

void sort_index(Findex **);
void sort_index(Findex **fap)
{
	Findex	*ta, **vector;
	size_t	n = 0, i;

	if (*fap == NULL)
		return;

	for (ta = *fap; ta; ta = ta->next)
		n++;

	vector = (Findex **)malloc(n * sizeof(Findex *));

	i = 0;
	for (ta = *fap; ta; ta = ta->next)
		vector[i++] = ta;

	qsort(vector, n, sizeof(Findex *), 
		(int(*)(const void*, const void *))comp_index);

	(*fap) = vector[0];
	i = 1;

	for (ta = *fap; ta; ta = ta->next) {
		if (i < n)
			ta->next = vector[i++];
		else
			ta->next = NULL;
	}

	free(vector);
	return;
}



int comp_index(Findex **fap1, Findex **fap2)
{
	return strcasecmp((*fap1)->idx.LName, (*fap2)->idx.LName);
}



/*
 * Build a sorted index for the file request processor.
 */
void Index(void)
{
	FILE	*pAreas, *pFile, *pIndex;
	long	i, iAreas, iAreasNew = 0, record;
	int	iTotal = 0;
	char	*sAreas, *fAreas, *newdir = NULL, *sIndex;
	Findex	*fdx = NULL;
	Findex	*tmp;
	struct FILEIndex	idx;

	sAreas = calloc(81, sizeof(char));
	fAreas = calloc(81, sizeof(char));
	sIndex = calloc(81, sizeof(char));

	IsDoing("Kill files");
	if (!do_quiet) {
		colour(3, 0);
		printf("Create filerequest index...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("$Can't open %s", sAreas);
		die(0);
	}

	sprintf(sIndex, "%s/etc/request.index", getenv("MBSE_ROOT"));
	if ((pIndex = fopen(sIndex, "w")) == NULL) {
		WriteError("$Can't create %s", sIndex);
		die(0);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	fseek(pAreas, 0, SEEK_END);
	iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

	for (i = 1; i <= iAreas; i++) {

		fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
		fread(&area, areahdr.recsize, 1, pAreas);

		if ((area.Available) && (area.FileReq)) {

			if (!diskfree(CFG.freespace))
				die(101);

			if (!do_quiet) {
				printf("\r%4ld => %-44s    \b\b\b\b", i, area.Name);
				fflush(stdout);
			}

			/*
			 * Check if download directory exists, 
			 * if not, create the directory.
			 */
			if (access(area.Path, R_OK) == -1) {
				Syslog('!', "Create dir: %s", area.Path);
				newdir = xstrcpy(area.Path);
				newdir = xstrcat(newdir, (char *)"/");
				mkdirs(newdir);
				free(newdir);
				newdir = NULL;
			}

			sprintf(fAreas, "%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), i);

			/*
			 * Open the file database, if it doesn't exist,
			 * create an empty one.
			 */
			if ((pFile = fopen(fAreas, "r+")) == NULL) {
				Syslog('!', "Creating new %s", fAreas);
				if ((pFile = fopen(fAreas, "a+")) == NULL) {
					WriteError("$Can't create %s", fAreas);
					die(0);
				}
			} 

			/*
			 * Now start creating the unsorted index.
			 */
			record = 0;
			while (fread(&file, sizeof(file), 1, pFile) == 1) {
				iTotal++;
				if ((iTotal % 10) == 0)
					Marker();
				memset(&idx, 0, sizeof(idx));
				sprintf(idx.Name, "%s", tu(file.Name));
				sprintf(idx.LName, "%s", tu(file.LName));
				idx.AreaNum = i;
				idx.Record = record;
				fill_index(idx, &fdx);
				record++;
			}

			fclose(pFile);
			iAreasNew++;

		} /* if area.Available */
	}

	fclose(pAreas);

	sort_index(&fdx);
	for (tmp = fdx; tmp; tmp = tmp->next)
		fwrite(&tmp->idx, sizeof(struct FILEIndex), 1, pIndex);
	fclose(pIndex);
	tidy_index(&fdx);

	Syslog('+', "Index Areas [%5d] Files [%5d]", iAreasNew, iTotal);

	if (!do_quiet) {
		printf("\r                                                          \r");
		fflush(stdout);
	}

	free(sIndex);
	free(sAreas);
	free(fAreas);
	RemoveSema((char *)"reqindex");
}



/*
 *  Check file database integrity, all files in the file database must
 *  exist in real, the size and date/time must match, the files crc is
 *  checked, and if anything is wrong, the file database is updated.
 *  If the file is missing the entry is marked as deleted. With the
 *  pack option that record will be removed.
 *  After these checks, de database is checked for missing records, if
 *  there are files on disk but not in the directory these files are 
 *  deleted. System files (beginning with a dot) are left alone and
 *  the files 'files.bbs', 'files.bak', '00index', 'header' 'readme' 
 *  and 'index.html' too.
 *
 *  Remarks:  Maybe if the crc check fails, and the date and time are
 *            ok, the file is damaged and must be made unavailable.
 */
void Check(void)
{
	FILE	*pAreas, *pFile;
	int	i, iAreas, iAreasNew = 0;
	int	iTotal = 0, iErrors =  0;
	char	*sAreas, *fAreas, *newdir;
	DIR	*dp;
	struct	dirent	*de;
	int	Found, Update;
	char	fn[128];
	struct	stat stb;

	sAreas = calloc(81, sizeof(char));
	fAreas = calloc(81, sizeof(char));
	newdir = calloc(81, sizeof(char));

	if (!do_quiet) {
		colour(3, 0);
		printf("Checking file database...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("Can't open %s", sAreas);
		die(0);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	fseek(pAreas, 0, SEEK_END);
	iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

	for (i = 1; i <= iAreas; i++) {

		fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
		fread(&area, areahdr.recsize, 1, pAreas);

		if (area.Available) {

			IsDoing("Check area %d", i);

			if (!do_quiet) {
				printf("\r%4d => %-44s    \b\b\b\b", i, area.Name);
				fflush(stdout);
			}

			/*
			 * Check if download directory exists, 
			 * if not, create the directory.
			 */
			if (access(area.Path, R_OK) == -1) {
				Syslog('!', "No dir: %s", area.Path);
				sprintf(newdir, "%s/foobar", area.Path);
				mkdirs(newdir);
			}

			sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);

			/*
			 * Open the file database, if it doesn't exist,
			 * create an empty one.
			 */
			if ((pFile = fopen(fAreas, "r+")) == NULL) {
				Syslog('!', "Creating new %s", fAreas);
				if ((pFile = fopen(fAreas, "a+")) == NULL) {
					WriteError("$Can't create %s", fAreas);
					die(0);
				}
			} 

			/*
			 * Now start checking the files in the filedatabase
			 * against the contents of the directory.
			 */
			while (fread(&file, sizeof(file), 1, pFile) == 1) {

				iTotal++;
				sprintf(newdir, "%s/%s", area.Path, file.Name);

				if (file_exist(newdir, R_OK)) {
					Syslog('+', "File %s area %d not on disk.", newdir, i);
					if (!file.NoKill) {
						file.Deleted = TRUE;
						do_pack = TRUE;
					}
					iErrors++;
					file.Missing = TRUE;
					fseek(pFile, - sizeof(file), SEEK_CUR);
					fwrite(&file, sizeof(file), 1, pFile);
				} else {
					/*
					 * File exists, now check the file.
					 */
					Marker();
					Update = FALSE;
					if (file_time(newdir) != file.FileDate) {
						Syslog('!', "Date mismatch area %d file %s", i, file.Name);
						file.FileDate = file_time(newdir);
						iErrors++;
						Update = TRUE;
					}
					if (file_size(newdir) != file.Size) {
						Syslog('!', "Size mismatch area %d file %s", i, file.Name);
						file.Size = file_size(newdir);
						iErrors++;
						Update = TRUE;
					}
					if (file_crc(newdir, CFG.slow_util && do_quiet) != file.Crc32) {
						Syslog('!', "CRC error area %d, file %s", i, file.Name);
						file.Crc32 = file_crc(newdir, CFG.slow_util && do_quiet);
						iErrors++;
						Update = TRUE;
					}
					Marker();
					if (Update) {
						fseek(pFile, - sizeof(file), SEEK_CUR);
						fwrite(&file, sizeof(file), 1, pFile);
					}
				}
			}

			/*
			 * Check files in the directory against the database.
			 * This test is skipped for CD-rom.
			 */
			if (!area.CDrom) {
				if ((dp = opendir(area.Path)) != NULL) {
					while ((de = readdir(dp)) != NULL) {
						if (de->d_name[0] != '.') {
							Marker();
							Found = FALSE;
							rewind(pFile);
							while (fread(&file, sizeof(file), 1, pFile) == 1) {
								if (strcmp(file.Name, de->d_name) == 0) {
									Found = TRUE;
									break;
								}
							}
							if ((!Found) && 
							    (strncmp(de->d_name, "files.bbs", 9)) &&
							    (strncmp(de->d_name, "files.bak", 9)) &&
							    (strncmp(de->d_name, "00index", 7)) &&
							    (strncmp(de->d_name, "header", 6)) &&
							    (strncmp(de->d_name, "index", 5)) &&
							    (strncmp(de->d_name, "readme", 6))) {
								sprintf(fn, "%s/%s", area.Path, de->d_name);
								if (stat(fn, &stb) == 0)
									if (S_ISREG(stb.st_mode)) {
										if (unlink(fn) == 0) {
											Syslog('!', "%s not in fdb, deleted from disk", fn);
											iErrors++;
										} else {
											WriteError("$%s not in fdb, cannot delete", fn);
										}
									}
							}
						}
					}
					closedir(dp);
				} else {
					WriteError("Can't open %s", area.Path);
				}
			}

			fclose(pFile);
			iAreasNew++;

		} /* if area.Available */
	}

	fclose(pAreas);
	if (!do_quiet) {
		printf("\r                                                                  \r");
		fflush(stdout);
	}

	free(newdir);
	free(sAreas);
	free(fAreas);

	Syslog('+', "Check Areas [%5d] Files [%5d]  Errors [%5d]", iAreasNew, iTotal, iErrors);
}



/*
 *  Removes records who are marked for deletion. If there is still a file
 *  on disk, it will be removed too.
 */
void PackFileBase(void)
{
	FILE	*fp, *pAreas, *pFile;
	int	i, iAreas, iAreasNew = 0;
	int	iTotal = 0, iRemoved = 0;
	char	*sAreas, *fAreas, *fTmp, fn[128];

	sAreas = calloc(81, sizeof(char));
	fAreas = calloc(81, sizeof(char));
	fTmp   = calloc(81, sizeof(char));

	IsDoing("Pack filebase");
	if (!do_quiet) {
		colour(3, 0);
		printf("Packing file database...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

	if ((pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("Can't open %s", sAreas);
		die(0);
	}

	fread(&areahdr, sizeof(areahdr), 1, pAreas);
	fseek(pAreas, 0, SEEK_END);
	iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

	for (i = 1; i <= iAreas; i++) {

		fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
		fread(&area, areahdr.recsize, 1, pAreas);

		if (area.Available && !area.CDrom) {

			if (!diskfree(CFG.freespace))
				die(101);

			if (!do_quiet) {
				printf("\r%4d => %-44s", i, area.Name);
				fflush(stdout);
			}
			Marker();

			sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);
			sprintf(fTmp,   "%s/fdb/fdbtmp.data", getenv("MBSE_ROOT"));

			if ((pFile = fopen(fAreas, "r")) == NULL) {
				Syslog('!', "Creating new %s", fAreas);
				if ((pFile = fopen(fAreas, "a+")) == NULL) {
					WriteError("$Can't create %s", fAreas);
					die(0);
				}
			} 

			if ((fp = fopen(fTmp, "a+")) == NULL) {
				WriteError("$Can't create %s", fTmp);
				die(0);
			}

			while (fread(&file, sizeof(file), 1, pFile) == 1) {

				iTotal++;

				if ((!file.Deleted) && (strcmp(file.Name, "") != 0)) {
					fwrite(&file, sizeof(file), 1, fp);
				} else {
					iRemoved++;
					Syslog('+', "Removed file \"%s\" from area %d", file.Name, i);
					sprintf(fn, "%s/%s", area.Path, file.Name);
					Syslog('+', "Unlink %s result %d", fn, unlink(fn));
					/*
					 * If a dotted version (thumbnail) exists, remove it silently
					 */
					sprintf(fn, "%s/.%s", area.Path, file.Name);
					unlink(fn);
				}
			}

			fclose(fp);
			fclose(pFile);

			if ((rename(fTmp, fAreas)) == 0) {
				unlink(fTmp);
				chmod(fAreas, 00660);
			}
			iAreasNew++;

		} /* if area.Available */
	}

	fclose(pAreas);
	Syslog('+', "Pack  Areas [%5d] Files [%5d] Removed [%5d]", iAreasNew, iTotal, iRemoved);

	if (!do_quiet) {
		printf("\r                                                              \r");
		fflush(stdout);
	}

	free(fTmp);
	free(sAreas);
	free(fAreas);
}


