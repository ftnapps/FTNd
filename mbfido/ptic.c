/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Process 1 .tic file
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
#include "../lib/dbtic.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "../lib/dbdupe.h"
#include "ulock.h"
#include "mover.h"
#include "toberep.h"
#include "tic.h"
#include "utic.h"
#include "addbbs.h"
#include "magic.h"
#include "forward.h"
#include "rollover.h"
#include "ptic.h"
#include "magic.h"
#include "virscan.h"


#define	UNPACK_FACTOR	300

extern	int	tic_bad;
extern	int	tic_dup;
extern	int	tic_out;
extern	int	do_quiet;
extern	int	check_crc;
extern	int	check_dupe;


/*
 * Return values:
 * 0 - Success
 * 1 - Some error
 * 2 - Orphaned tic
 */
int ProcessTic(fa_list *sbl, char *Realname)
{
	time_t		Now, Fdate;
	int		Age, First, Listed = FALSE;
	int		DownLinks = 0;
	int		MustRearc = FALSE, MustVirus = FALSE;
	int		IsVirus = FALSE, UnPacked = FALSE, IsArchive = FALSE;
	int		i, j, k, File_Id = FALSE;
	char		*Temp, *Temp2, *unarc = NULL, *cmd = NULL;
	char		temp1[PATH_MAX], temp2[PATH_MAX], sbe[24], TDesc[256];
	unsigned long	crc, crc2, Kb;
	sysconnect	Link;
	FILE		*fp;
	long		FwdCost = 0, FwdSize = 0;
	struct utimbuf	ut;
	int		BBS_Imp = FALSE, DidBanner = FALSE;


	time(&Now);

	if (TIC.PathErr) {
		WriteError("Our Aka is in the path");
		return 1;
	}

	Temp = calloc(PATH_MAX, sizeof(char));
	sprintf(Temp, "%s%s", TIC.FilePath, TIC.TicIn.OrgName);

	if (!do_quiet) {
		colour(10, 0);
		printf("Checking  \b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}

	/*
	 * A hack to work around received uppercase or mixed case filenames.
	 */
	Temp2 = calloc(PATH_MAX, sizeof(char));
	sprintf(Temp2, "%s%s", TIC.FilePath, Realname);
	Syslog('f', "Realname with path \"%s\"", Temp2);
	if (file_exist(Temp, R_OK) && !file_exist(Temp2, R_OK)) {
		if (rename(Temp2, Temp))
			WriteError("$Rename %s to %s failed", Temp2, Temp);
		else
			Syslog('f', "File %s renamed to %s", Temp2, Temp);
	}
	free(Temp2);

	crc = file_crc(Temp, CFG.slow_util && do_quiet);
	if (crc == -1) {
		WriteError("File not in inbound: %s", Temp);
		/*
		 * Now check the age of the .tic file.
		 */
		sprintf(Temp, "%s%s", TIC.Inbound, TIC.TicName);
		Fdate = file_time(Temp);
		Age = (Now - Fdate) / 84400;
		Syslog('+', "Orphaned tic age %d days", Age);

		if (Age > 21) {
			tic_bad++;
			mover(TIC.Inbound, TIC.TicName);
		}
	
		free(Temp);
		return 2;
	}

	TIC.FileSize = file_size(Temp);
	TIC.FileDate = file_time(Temp);

	if (TIC.TicIn.Size) {
		if (TIC.TicIn.Size != TIC.FileSize)
			WriteError("Size is %ld, expected %ld", TIC.FileSize, TIC.TicIn.Size);
	}

	if (TIC.Crc_Int) {
		if (crc != TIC.Crc_Int) {
			Syslog('!', "CRC: expected %08lX, the file is %08lX", TIC.Crc_Int, crc);
			if (check_crc) {
				Bad((char *)"CRC: error, %s may be damaged", TIC.TicIn.OrgName);
				free(Temp);
				return 1;
			} else {
				Syslog('!', "CRC: error, recalculating crc");
				ReCalcCrc(Temp);
			}
		}
	} else {
		Syslog('+', "CRC: missing, calculating CRC");
		ReCalcCrc(Temp);
	}

	/*
	 * Load and check the .TIC area.
	 */
	if (!SearchTic(TIC.TicIn.Area)) {
		Bad((char *)"Unknown file area %s", TIC.TicIn.Area);
		free(Temp);
		return 1;
	}

	if ((tic.Secure) && (!TIC.Hatch)) {
		First = TRUE;
		while (GetTicSystem(&Link, First)) {
			First = FALSE;
			if (Link.aka.zone) {
				if ((Link.aka.zone == TIC.Aka.zone) &&
				    (Link.aka.net  == TIC.Aka.net) &&
				    (Link.aka.node == TIC.Aka.node) &&
				    (Link.aka.point== TIC.Aka.point) &&
				    (Link.receivefrom)) 
					Listed = TRUE;
			}
		}
		if (!Listed) {
			Bad((char *)"%s NOT connected to %s", aka2str(TIC.Aka), TIC.TicIn.Area);
			free(Temp);
			return 1;
		}
	}

	if ((!SearchNode(TIC.Aka)) && (!TIC.Hatch)) {
		Bad((char *)"%s NOT known", aka2str(TIC.Aka));
		free(Temp);
		return 1;
	}

	if (!TIC.Hatch) {
		if (strcasecmp(TIC.TicIn.Pw, nodes.Fpasswd)) {
			Bad((char *)"Pwd error, got %s, expected %s", TIC.TicIn.Pw, nodes.Fpasswd);
			free(Temp);
			return 1;
		}
	} else {
		if (strcasecmp(TIC.TicIn.Pw, CFG.hatchpasswd)) {
			Bad((char *)"Password error in local Hatch");
			WriteError("WARNING: it might be a Trojan in your inbound");
			free(Temp);
			return 1;
		}
	}

	if (Magic_DeleteFile()) {
		sprintf(temp1, "%s%s", TIC.Inbound, TIC.TicName);
		file_rm(temp1);
		Syslog('+', "Deleted file %s", temp1);
		free(Temp);
		return 0;
	}


	if (Magic_MoveFile()) {
		if (!SearchTic(TIC.TicIn.Area)) {
			Bad((char *)"Unknown Area: %s", TIC.TicIn.Area);
			free(Temp);
			return 1;
		}
	}

	strcpy(T_File.Echo, tic.Name);
	strcpy(T_File.Group, tic.Group);
	TIC.KeepNum = tic.KeepLatest;

	Magic_Keepnum();

	if (!tic.FileArea) {
		Syslog('f', "Passthru area!");
		strcpy(TIC.BBSpath, CFG.ticout);
		strcpy(TIC.BBSdesc, tic.Comment);
	} else {
		sprintf(Temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
		if ((fp = fopen(Temp, "r")) == NULL) {
			WriteError("Can't access fareas.data area: %ld", tic.FileArea);
			free(Temp);
			return 1;
		}
		fread(&areahdr, sizeof(areahdr), 1, fp);
		if (fseek(fp, ((tic.FileArea -1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET)) {
			fclose(fp);
			WriteError("Can't seek area %ld in fareas.data", tic.FileArea);
			free(Temp);
			return 1;
		}
		if (fread(&area, areahdr.recsize, 1, fp) != 1) {
			fclose(fp);
			WriteError("Can't read area %ld in fareas.data", tic.FileArea);
			free(Temp);
			return 1;
		}
		fclose(fp);
		strcpy(TIC.BBSpath, area.Path);
		strcpy(TIC.BBSdesc, area.Name);

		/*
		 * If the File area has a special announce group, change
		 * the group to that name.
		 */
		if (strlen(area.NewGroup))
			strcpy(T_File.Group, area.NewGroup);
	}
	strcpy(T_File.Comment, tic.Comment);

	/*
	 * The destination may be changed now.
	 */
	Magic_OtherPath();

	/*
	 * Check if the destination area really exists, it may be that
	 * the area is not linked to an existing BBS area.
	 */
	if (tic.FileArea && access(TIC.BBSpath, W_OK)) {
		WriteError("$No write access to \"%s\"", TIC.BBSpath);
		Bad((char *)"Dest directory not available");
		free(Temp);
		return 1;
	}

	if ((tic.DupCheck) && (check_dupe)) {
		sprintf(Temp, "%s%s", TIC.TicIn.Area, TIC.TicIn.Crc);
		crc2 = 0xffffffff;
		crc2 = upd_crc32(Temp, crc2, strlen(Temp));
		if (CheckDupe(crc2, D_FILEECHO, CFG.tic_dupes)) {
			Bad((char *)"Duplicate file");
			tic_dup++;
			free(Temp);
			return 1;
		}
	}

	/*
	 * Count the actual downlinks for this area
	 */
	First = TRUE;
	while (GetTicSystem(&Link, First)) {
		First = FALSE;
		if ((Link.aka.zone) && (Link.sendto))
			DownLinks++;
	}

	/*
	 * Calculate the cost of this file
	 */
	T_File.Size = TIC.FileSize;
	T_File.SizeKb = TIC.FileSize / 1024;
	if ((fgroup.UnitCost) || (TIC.TicIn.UplinkCost))
		TIC.Charge = TRUE;
	else
		TIC.Charge = FALSE;

	if (TIC.Charge) {
		/*
		 * Calculate our filetransfer cost.
		 */
		FwdCost = fgroup.UnitCost;
		FwdSize = fgroup.UnitSize;

		/*
		 * If FwdSize <> 0 then calculate per size, else
		 * charge for each file.
		 */
		if (FwdSize)
			TIC.FileCost = ((TIC.FileSize / 1024) / FwdSize) * FwdCost;
		else
			TIC.FileCost = FwdCost;

		if (TIC.TicIn.UplinkCost)
			TIC.FileCost += TIC.TicIn.UplinkCost;

		if (fgroup.AddProm)
			TIC.FileCost += (TIC.FileCost * fgroup.AddProm / 1000);

		if (fgroup.DivideCost) {
			/*
			 * If not a passthru area, we are a link too.
			 */
			if (DownLinks)
				TIC.FileCost = TIC.FileCost / (DownLinks + 1);
		}

		/*
		 * At least charge one unit.
		 */
		if (!TIC.FileCost)
			TIC.FileCost = 1;

		Syslog('+', "The file cost will be %ld", TIC.FileCost);
	}

	/*
	 * Update the uplink's counters.
	 */
	Kb = TIC.FileSize / 1024;
	if (SearchNode(TIC.Aka)) {
		nodes.Debet -= TIC.TicIn.UplinkCost;
		if (TIC.TicIn.UplinkCost)
			Syslog('f', "Uplink cost %ld, debet %ld", TIC.TicIn.UplinkCost, nodes.Debet);
		StatAdd(&nodes.FilesRcvd, 1L);
		StatAdd(&nodes.F_KbRcvd, Kb);
		UpdateNode();
		SearchNode(TIC.Aka);
	}

	/*
	 * Update the fileecho and group counters.
	 */
	StatAdd(&fgroup.Files, 1L);
	StatAdd(&fgroup.KBytes, Kb);
	fgroup.LastDate = time(NULL);
	StatAdd(&tic.Files, 1L);
	StatAdd(&tic.KBytes, Kb);
	tic.LastAction = time(NULL);
	UpdateTic();

	if (!TIC.HatchNew) {

		if (!do_quiet) {
			printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		}

		/*
		 * Move the file to the inbound directory if it isn't
		 * already there (needed by unpacker).
		 */
		if (strcmp(TIC.FilePath,TIC.Inbound)) {
			sprintf(temp1, "%s%s", TIC.FilePath, TIC.TicIn.OrgName);
			sprintf(temp2, "%s%s", TIC.Inbound, TIC.TicIn.OrgName);
			if (!file_mv(temp1, temp2)) {
				sprintf(TIC.FilePath, "%s/", TIC.Inbound);
			} else {
				WriteError("Can't move %s to inbound", temp1);
			}
		}

		/*
		 * Check if this is an archive, and if so, compression method
		 * is used for this file.
		 */
		if (strlen(tic.Convert) || tic.VirScan || tic.FileId || tic.ConvertAll || strlen(tic.Banner)) {
			if ((unarc = unpacker(TIC.TicIn.OrgName)) == NULL)
				Syslog('+', "Unknown archive format %s", TIC.TicIn.OrgName);
			else {
				IsArchive = TRUE;
				if ((strlen(tic.Convert) && (strcmp(unarc, tic.Convert) == 0)) || (tic.ConvertAll))
					MustRearc = TRUE;
			}
		}

		/*
		 * Copy the file if there are downlinks and we send the 
		 * original file, but want to rearc it for ourself, or if
		 * it's a passthru area.
		 */
		if (((tic.SendOrg) && (MustRearc || strlen(tic.Banner))) || (!tic.FileArea)) {
			sprintf(temp1, "%s%s", TIC.FilePath, TIC.TicIn.OrgName);
			sprintf(temp2, "%s/%s", CFG.ticout, TIC.TicIn.OrgName);
			if (file_cp(temp1, temp2) == 0) {
				TIC.SendOrg = TRUE;
			} else {
				WriteError("$Copy %s to %s failed", temp1, temp2);
			}
		}

		if ((tic.VirScan) && (IsArchive))
			MustVirus = TRUE;

		if (MustVirus || MustRearc) {

			/*
			 * Check if there is a temp directory for the archive
			 * conversion.
			 */
			sprintf(temp2, "%s/tmp/arc", getenv("MBSE_ROOT"));
			if ((access(temp2, R_OK)) != 0) {
				if (mkdir(temp2, 0777)) {
					WriteError("$Can't create %s", temp2);
					free(Temp);
					return 1;
				}
			}

			/*
			 * Check for stale FILE_ID.DIZ files
			 */
			sprintf(temp1, "%s/tmp/arc/FILE_ID.DIZ", getenv("MBSE_ROOT"));
			if (!unlink(temp1))
				Syslog('+', "Removed stale %s", temp1);
			sprintf(temp1, "%s/tmp/FILE_ID.DIZ", getenv("MBSE_ROOT"));
			if (!unlink(temp1))
				Syslog('+', "Removed stale %s", temp1);

			if (!checkspace(temp2, TIC.TicIn.OrgName, UNPACK_FACTOR)) {
				Bad((char *)"Not enough free diskspace left");
				free(Temp);
				return 1;
			}

			if (chdir(temp2) != 0) {
				WriteError("$Can't change to %s", temp2);
				free(Temp);
				return 1;
			}

			if (!getarchiver(unarc)) {
				chdir(TIC.Inbound);
				free(Temp);
				return 1;
			}

			cmd = xstrcpy(archiver.funarc);

			if ((cmd == NULL) || (cmd == "")) {
				Syslog('!', "No unarc command available");
			} else {
				sprintf(temp1, "%s%s", TIC.Inbound, TIC.TicIn.OrgName);
				if (execute(cmd, temp1, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
					UnPacked = TRUE;
				} else {
					chdir(TIC.Inbound);
					Bad((char *)"Archive maybe corrupt");
					free(Temp);
					DeleteVirusWork();
					return 1;
				}
				free(cmd);
			}
		}

		if (MustVirus && UnPacked) {

			if (!do_quiet) {
				printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
				fflush(stdout);
			}

			IsVirus = VirScan();
			if (IsVirus) {
				DeleteVirusWork();
				chdir(TIC.Inbound);
				Bad((char *)"Possible virus found!");
				free(Temp);
				return 1;
			}

			if (!do_quiet) {
				printf("Checking  \b\b\b\b\b\b\b\b\b\b");
				fflush(stdout);
			}

		} /* MustVirus and Unpacked */

		if (tic.FileId && tic.FileArea && IsArchive) {
			if (UnPacked) {
				sprintf(temp1, "%s/tmp/arc/FILE_ID.DIZ", getenv("MBSE_ROOT"));
				sprintf(temp2, "%s/tmp/FILE_ID.DIZ", getenv("MBSE_ROOT"));
				if (file_cp(temp1, temp2) == 0)
					File_Id = TRUE;
			} else {
				if (!getarchiver(unarc)) {
					chdir(TIC.Inbound);
				} else {
					cmd = xstrcpy(archiver.iunarc);

					if (cmd == NULL) {
						WriteError("No unarc command available");
					} else {
						sprintf(temp1, "%s/tmp", getenv("MBSE_ROOT"));
						chdir(temp1);
						sprintf(temp1, "%s%s FILE_ID.DIZ", TIC.Inbound, TIC.TicIn.OrgName);
						if (execute(cmd, temp1, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
							File_Id = TRUE;
						}
						free(cmd);
					}
				} /* if getarchiver */
			} /* if not unpacked */
		} /* if need FILE_ID.DIZ and not passthru */

		/*
		 * Create internal file description, priority is FILE_ID.DIZ,
		 * 2nd LDesc, and finally the standard description.
		 */
		if (!Get_File_Id()) {
			if (TIC.TicIn.TotLDesc > 2) {
				for (i = 0; i < TIC.TicIn.TotLDesc; i++) {
					strcpy(temp1, TIC.TicIn.LDesc[i]);
					temp1[48] = '\0';
					strcpy(TIC.File_Id[i], temp1);
				}
				TIC.File_Id_Ct = TIC.TicIn.TotLDesc;
			} else {
				/*
				 * Format the description line (max 255 chars)
				 * in parts of 48 characters.
				 */
			 	if (strlen(TIC.TicIn.Desc) <= 48) {
					strcpy(TIC.File_Id[0], TIC.TicIn.Desc);
					TIC.File_Id_Ct++;
				} else {
					memset(&TDesc, 0, sizeof(TDesc));
					strcpy(TDesc, TIC.TicIn.Desc);
					while (strlen(TDesc) > 48) {
						j = 48;
						while (TDesc[j] != ' ')
							j--;
						strncat(TIC.File_Id[TIC.File_Id_Ct], TDesc, j);
						TIC.File_Id_Ct++;
						k = strlen(TDesc);
						j++; /* Correct space */
						for (i = 0; i <= k; i++, j++)
							TDesc[i] = TDesc[j];
					}
					strcpy(TIC.File_Id[TIC.File_Id_Ct], TDesc);
					TIC.File_Id_Ct++;
				}
			}
		} /* not get FILE_ID.DIZ */

		if ((MustRearc) && (UnPacked) && (tic.FileArea)) {
			if (Rearc(tic.Convert)) {
				if (strlen(tic.Banner)) {
					Syslog('f', "Must replace banner 1");
				}

				/*
				 * Get new filesize for import and announce
				 */
				sprintf(temp1, "%s%s", TIC.FilePath, TIC.NewName);
				TIC.FileSize = file_size(temp1);
				T_File.Size = TIC.FileSize;
				T_File.SizeKb = TIC.FileSize / 1024;
				/*
				 * Calculate the CRC if we must send the new
				 * archived file.
				*/
				if (!TIC.SendOrg) {
					ReCalcCrc(temp1);
				}
			} else {
				WriteError("Rearc failed");
			} /* if Rearc() */
		} else {
			/*
			 * If the file is not unpacked, change the banner
			 * direct if this is needed.
			 */
			if ((strlen(tic.Banner)) && IsArchive) {
				cmd = xstrcpy(archiver.barc);
				if ((cmd == NULL) || (!strlen(cmd))) {
					Syslog('!', "No banner command for %s", archiver.name);
				} else {
					sprintf(temp1, "%s%s", TIC.Inbound, TIC.TicIn.OrgName);
					sprintf(Temp, "%s/etc/%s", getenv("MBSE_ROOT"), tic.Banner);
					if (execute(cmd, temp1, (char *)NULL, Temp, (char *)"/dev/null", (char *)"/dev/null")) {
						WriteError("$Changing the banner failed");
					} else {
						Syslog('+', "New banner %s", tic.Banner);
						TIC.FileSize = file_size(temp1);
						T_File.Size = TIC.FileSize;
						T_File.SizeKb = TIC.FileSize / 1024;
						ReCalcCrc(temp1);
						DidBanner = TRUE;
					}
				}
			}
		} /* if MustRearc and Unpacked and not Passthtru */

		if (UnPacked)
			DeleteVirusWork();

		chdir(TIC.Inbound);

		/*
		 * If the file is converted, we set the date of the original
		 * received file as the file creation date.
		 */
		if (MustRearc || DidBanner) {
			if ((!tic.NoTouch) && (tic.FileArea)) {
				ut.actime = mktime(localtime(&TIC.FileDate));
				ut.modtime = mktime(localtime(&TIC.FileDate));
				sprintf(Temp, "%s%s", TIC.FilePath, TIC.NewName);
				utime(Temp, &ut);
			}
		}

		if (tic.FileArea) {
			Syslog('+', "Import: %s Area: %s", TIC.NewName, TIC.TicIn.Area);

			if (TIC.OtherPath) {
				/* BBS_Imp = Add_DOS */
			} else {
				BBS_Imp = Add_BBS();
			}

			if (!BBS_Imp) {
				Bad((char *)"File Import Error");
				free(Temp);
				return 1;
			}
		}

	} /* Not TIC.HatchNew */

	chdir(TIC.Inbound);

	if ((!TIC.HatchNew) && (tic.FileArea)) {
		if (strlen(TIC.TicIn.Magic))
			UpDateAlias(TIC.TicIn.Magic);
		else
			Magic_UpDateAlias();

		for (i = 0; i <= TIC.File_Id_Ct; i++)
			strcpy(T_File.LDesc[i], TIC.File_Id[i]);
		T_File.TotLdesc = TIC.File_Id_Ct;
		T_File.Announce = tic.Announce;
		strcpy(T_File.Name, TIC.NewName);
		T_File.Fdate = TIC.FileDate;
		T_File.Cost = TIC.TicIn.UplinkCost;
		Add_ToBeRep();
	}

	if (TIC.SendOrg && !tic.FileArea) {
		/*
		 * If it's a passthru area we don't need the
		 * file in the inbound anymore so it can be
		 * deleted.
		 */
		sprintf(temp1, "%s%s", TIC.FilePath, TIC.TicIn.OrgName);
		if (file_rm(temp1) == 0)
			Syslog('f', "Deleted %s", temp1);
	}

	if (DownLinks) {
		First = TRUE;

		/*
		 * Add all our system aka's to the seenby lines in the same zone
		 */
		for (i = 0; i < 40; i++) {
			if (CFG.akavalid[i] && (tic.Aka.zone == CFG.aka[i].zone) &&
			    !((tic.Aka.net == CFG.aka[i].net) && (tic.Aka.node == CFG.aka[i].node))) {
				sprintf(sbe, "%u:%u/%u", CFG.aka[i].zone, CFG.aka[i].net, CFG.aka[i].node);
				fill_list(&sbl, sbe, NULL);
			}
		}

		/*
		 * Add downlinks to seenby lines
		 */
		while (GetTicSystem(&Link, First)) {
			First = FALSE;
			if ((Link.aka.zone) && (Link.sendto) && (!Link.pause) && (!Link.aka.point)) {
				if (!((TIC.Aka.zone == Link.aka.zone) && (TIC.Aka.net == Link.aka.net) &&
				      (TIC.Aka.node == Link.aka.node) && (TIC.Aka.point == Link.aka.point))) {
					sprintf(sbe, "%u:%u/%u", Link.aka.zone, Link.aka.net, Link.aka.node);
					fill_list(&sbl, sbe, NULL);
				}
			}
		}
		uniq_list(&sbl);
		sort_list(&sbl);

		/*
		 * Now start forwarding files
		 */
		First = TRUE;
		while (GetTicSystem(&Link, First)) {
			First = FALSE;
			if ((Link.aka.zone) && (Link.sendto) && (!Link.pause)) {
				if (!((TIC.Aka.zone == Link.aka.zone) && (TIC.Aka.net == Link.aka.net) &&
				      (TIC.Aka.node == Link.aka.node) && (TIC.Aka.point == Link.aka.point))) {
					tic_out++;
					ForwardFile(Link.aka, sbl);
				}			
			}
		}
	}

	if (!TIC.HatchNew) {
		Magic_ExecCommand();
		Magic_CopyFile();
		Magic_UnpackFile();
		Magic_AdoptFile();
	}

	sprintf(Temp, "%s%s", TIC.Inbound, TIC.TicName);
	unlink(Temp);
	free(Temp);
	return 0;
}


