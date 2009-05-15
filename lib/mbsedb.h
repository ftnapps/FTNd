/*****************************************************************************
 *
 & $Id: mbsedb.h,v 1.10 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: MBSE BBS database library header
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek                FIDO:           2:280/2802
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

#ifndef _MBSEDB_H
#define _MBSEDB_H


void	InitConfig(void);		/* Initialize and load config	     */
void	LoadConfig(void);		/* Only load config file	     */
int	IsOurAka(fidoaddr);		/* Check if our aka		     */


/*
 * Dupes database type
 */
typedef enum {D_ECHOMAIL, D_FILEECHO, D_NEWS} DUPETYPE;

void	InitDupes(void);
int	CheckDupe(unsigned int, int, int);
void	CloseDupes(void);



/*
 * Fidonet database
 */
struct _fidonethdr  fidonethdr;		    /* Header record		    */
struct _fidonet	    fidonet;		    /* Fidonet datarecord	    */
int		    fidonet_cnt;	    /* Fidonet records in database  */
char		    fidonet_fil[PATH_MAX];  /* Fidonet database filename    */

int	InitFidonet(void);		/* Initialize fidonet database	    */
int	TestFidonet(unsigned short);	/* Test if zone is in memory	    */
int	SearchFidonet(unsigned short);	/* Search specified zone and load   */
char	*GetFidoDomain(unsigned short);	/* Search Fidonet domain name	    */



/*
 * Nodes database
 */
struct	_nodeshdr	nodeshdr;	/* Header record		    */
struct	_nodes		nodes;		/* Nodes datarecord		    */
int			nodes_cnt;	/* Node records in database	    */

int	InitNode(void);			/* Initialize nodes database	    */
int	TestNode(fidoaddr);		/* Check if noderecord is loaded    */
int	SearchNodeFaddr(faddr *);	/* Search specified node and load   */
int	SearchNode(fidoaddr);		/* Search specified node and load   */
int	UpdateNode(void);		/* Update record if changed.	    */
char	*GetNodeMailGrp(int);		/* Get nodes mailgroup record	    */
char	*GetNodeFileGrp(int);		/* Get nodes filegroup record	    */




/*
 * TIC area database
 */
struct	_tichdr		tichdr;		/* Header record		    */
struct	_tic		tic;		/* Tics datarecord		    */
struct	_fgrouphdr	fgrouphdr;	/* Group header record		    */
struct	_fgroup		fgroup;		/* Group record			    */
int			tic_cnt;	/* Tic records in database	    */

int	InitTic(void);			/* Initialize tic database	    */
int	SearchTic(char *);		/* Search specified msg are	    */
int	TicSystemConnected(sysconnect);	/* Is system connected		    */
int	TicSystemConnect(sysconnect *, int); /* Connect/change/delete system*/
int	GetTicSystem(sysconnect *, int);/* Get connected system		    */
void	UpdateTic(void);		/* Update current messages record   */



/*
 * User records
 */
struct	userhdr	    usrhdr;		/* Header record		    */
struct	userrec	    usr;		/* User datarecord		    */
int		    usr_cnt;		/* User records in database	    */
char		    usr_fil[PATH_MAX];	/* User database filename	    */

int	InitUser(void);			/* Initialize user database	    */
int	TestUser(char *);		/* Test if user is in memory	    */
int	SearchUser(char *);		/* Search specified user and load   */



/*
 * Message areas database
 */
struct  msgareashdr     msgshdr;        /* Header record                    */
struct  msgareas        msgs;           /* Msgss datarecord                 */
struct  _mgrouphdr      mgrouphdr;      /* Group header record              */
struct  _mgroup         mgroup;         /* Group record                     */
int                     msgs_cnt;       /* Msgs records in database         */

int     InitMsgs(void);                 /* Initialize msgs database         */
int     SearchMsgs(char *);             /* Search specified msg area        */
int     SearchMsgsNews(char *);         /* Search specified msg area        */
int	SearchBadBoard(void);		/* Search system badboard	    */
int     MsgSystemConnected(sysconnect); /* Is system connected              */
int     MsgSystemConnect(sysconnect *, int); /* Connect/change/delete system*/
int     GetMsgSystem(sysconnect *, int);/* Get connected system             */
int     SearchNetBoard(unsigned short, unsigned short); /* Search netmail   */
void    UpdateMsgs(void);               /* Update current messages record   */



/*
 * Structure of current open file area
 */
struct _fdbarea {
    int		area;           /* Area number          */
    int         locked;         /* Is area locked       */
    FILE        *fp;            /* File pointer         */
};


struct _fdbarea *mbsedb_OpenFDB(int, int);
int mbsedb_CloseFDB(struct _fdbarea *);
int mbsedb_LockFDB(struct _fdbarea *, int);
int mbsedb_UnlockFDB(struct _fdbarea *);
int mbsedb_InsertFDB(struct _fdbarea *, struct FILE_record, int);
int mbsedb_PackFDB(struct _fdbarea *);
int mbsedb_SortFDB(struct _fdbarea *);


#endif
