#ifndef _TIC_H
#define _TIC_H

/* $Id: tic.h,v 1.7 2005/11/12 12:52:30 mbse Exp $ */


typedef	struct	_tic_in {
	unsigned	Hatch		: 1;	/* Hatch keyword	    */
	unsigned	PathError	: 1;	/* Our system is in path    */
	char		Pth[PATH_MAX+1];	/* Path to hatched file	    */
	char		Area[21];		/* Area name		    */
	char		Origin[81];		/* Origin address	    */
	char		From[81];		/* From name		    */
	char		File[81];		/* File keyword		    */
	char		FullName[256];		/* Long filename	    */
	char		Replace[81];		/* File to replace	    */
	char		Created[81];		/* Created text		    */
	char		Path[25][81];		/* Travelled path	    */
	int		TotPath;		/* Nr of pathlines	    */
	char		Desc[1024];		/* Short description	    */
	char		Magic[21];		/* Magic alias		    */
	char		Crc[9];			/* CRC of file		    */
	char		Pw[21];			/* Password		    */
	char		AreaDesc[61];		/* Area description	    */
	char		Date[61];		/* Date field		    */
	int		Cost;	    		/* Uplink cost		    */
	off_t		Size;			/* Size of file		    */
	char		LDesc[25][81];		/* Long description	    */
	int		TotLDesc;		/* Total lines		    */
	char		Unknown[25][128];	/* Unknown (passthru) lines */
	int		Unknowns;		/* Total of above	    */
	int		MultiSeen;		/* Multi Seenby Lines	    */
} Tic_in;


typedef	struct	_TICrec {
	char		Inbound[PATH_MAX+1];	/* Inbound directory	    */
	char		TicName[13];		/* Name of .TIC file	    */
	Tic_in		TicIn;			/* Original TIC record	    */
	fidoaddr	OrgAka;			/* Origin address	    */
	fidoaddr	Aka;			/* An address ?		    */
	char		NewFile[81];		/* New 8.3 filename	    */
	char		NewFullName[256];	/* New LFN name		    */
	char		File_Id[25][49];	/* Description		    */
	int		File_Id_Ct;		/* Nr of lines		    */
	unsigned int	Crc_Int;		/* Crc value		    */
	int		KeepNum;		/* Keep number of files	    */
	off_t		FileSize;		/* Size of file		    */
	time_t		FileDate;		/* Date of file		    */
	time_t		UpLoadDate;		/* Upload date of file	    */
	unsigned	SendOrg		: 1;	/* Send original file	    */
	unsigned	Charge		: 1;	/* Charge for this file	    */
	unsigned	PassThru	: 1;	/* PassThru file	    */
	unsigned	NewAlias	: 1;	/* New alias is set	    */
	unsigned	Orphaned	: 1;	/* File is Orphaned	    */
	int		FileCost;		/* Cost for this file	    */
	char		BBSpath[PATH_MAX];	/* Path to import in	    */
	char		BBSdesc[55];		/* Area description	    */
} TICrec;


TICrec			TIC;			/* Global .tic record	    */
struct	_filerecord	T_File;			/* Global file handling rec.*/

int	CompileNL;


int	Tic(void);
int	LoadTic(char *, char *, orphans **);


#endif
