#ifndef	_TIC_H
#define	_TIC_H


typedef	struct	_tic_in {
	char		Area[21];		/* Area name		    */
	char		Origin[81];		/* Origin address	    */
	char		From[81];		/* From name		    */
	char		OrgName[81];		/* Original filename	    */
	char		LName[81];		/* Long filename	    */
	char		Replace[81];		/* File to replace	    */
	char		Created[81];		/* Created text		    */
	char		Path[25][81];		/* Travelled path	    */
	int		TotPath;		/* Nr of pathlines	    */
	char		Desc[256];		/* Short description	    */
	char		Magic[21];		/* Magic alias		    */
	char		Crc[9];			/* CRC of file		    */
	char		Pw[21];			/* Password		    */
	char		AreaDesc[61];		/* Area description	    */
	char		Date[61];		/* Date field		    */
	long		UplinkCost;		/* Uplink cost		    */
	off_t		Size;			/* Size of file		    */
	char		LDesc[25][81];		/* Long description	    */
	int		TotLDesc;		/* Total lines		    */
	char		Unknown[25][128];	/* Unknown (passthru) lines */
	int		Unknowns;		/* Total of above	    */
	int		MultiSeen;		/* Multi Seenby Lines	    */
} Tic_in;


typedef	struct	_TICrec {
	char		Inbound[PATH_MAX];	/* Inbound directory	    */
	char		TicName[13];		/* Name of .TIC file	    */
	Tic_in		TicIn;			/* Original TIC record	    */
	fidoaddr	OrgAka;			/* Origin address	    */
	fidoaddr	Aka;			/* An address ?		    */
	char		NewName[81];		/* New name of file	    */
	char		File_Id[25][49];	/* Description		    */
	int		File_Id_Ct;		/* Nr of lines		    */
	unsigned long	Crc_Int;		/* Crc value		    */
	int		KeepNum;		/* Keep number of files	    */
	off_t		FileSize;		/* Size of file		    */
	time_t		FileDate;		/* Date of file		    */
	time_t		UpLoadDate;		/* Upload date of file	    */
	char		FilePath[PATH_MAX];	/* Path to the file	    */
	unsigned        PathErr         : 1;    /* If path error            */
	unsigned        OtherPath       : 1;    /* If otherpath is true     */
	unsigned        Hatch           : 1;    /* If internal hatched      */
	unsigned	NoMove		: 1;	/* No move magic	    */
	unsigned	HatchNew	: 1;	/* Hatch in new areas	    */
	unsigned	SendOrg		: 1;	/* Send original file	    */
	unsigned	Charge		: 1;	/* Charge for this file	    */
	unsigned	PassThru	: 1;	/* PassThru file	    */
	unsigned	NewAlias	: 1;	/* New alias is set	    */
	long		FileCost;		/* Cost for this file	    */
	char		BBSpath[PATH_MAX];	/* Path to import in	    */
	char		BBSdesc[55];		/* Area description	    */
} TICrec;


TICrec			TIC;			/* Global .tic record	    */
struct	_filerecord	T_File;			/* Global file handling rec.*/

int	CompileNL;


int	Tic(void);
int	LoadTic(char *, char *);


#endif

