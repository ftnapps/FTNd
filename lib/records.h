/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Global records structure
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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


#ifndef _RECORDS_H
#define _RECORDS_H

struct	userhdr		usrconfighdr;		/* Users database	   */
struct	userrec		usrconfig;
struct	userrec		exitinfo;		/* Users online data	   */

struct	servicehdr	servhdr;		/* Services database	   */
struct	servicerec	servrec;

struct	sysrec		SYSINFO;		/* System info statistics  */

struct	prothdr		PROThdr;		/* Transfer protocols	   */
struct	prot		PROT;

struct	onelinehdr	olhdr;			/* Oneliner database	   */
struct	oneline		ol;

struct	fileareashdr	areahdr;		/* File areas		   */
struct	fileareas	area;
struct	FILERecord	file;
struct	_fgrouphdr	fgrouphdr;		/* File groups		   */
struct	_fgroup		fgroup;

struct	_ngrouphdr	ngrouphdr;		/* Newfiles groups	   */
struct	_ngroup		ngroup;	

struct	bbslisthdr	bbshdr;			/* BBS list		   */
struct	bbslist		bbs;

struct	lastcallershdr	LCALLhdr;		/* Lastcallers info	   */
struct	lastcallers	LCALL;

struct	sysconfig	CFG;			/* System configuration	   */

struct	limitshdr	LIMIThdr;		/* User limits		   */
struct	limits		LIMIT;

struct	menufile	menus;

struct	msgareashdr	msgshdr;		/* Messages configuration  */
struct	msgareas	msgs;
struct	_mgrouphdr	mgrouphdr;		/* Message groups	   */
struct	_mgroup		mgroup;

struct	languagehdr	langhdr;		/* Language data	   */
struct	language	lang;			  			  
struct	langdata	ldata;

struct	_fidonethdr	fidonethdr;		/* Fidonet structure	   */
struct	_fidonet	fidonet;
struct  domhdr		domainhdr;
struct  domrec		domtrans;

struct	_archiverhdr	archiverhdr;		/* Archivers		   */
struct	_archiver	archiver;

struct	_virscanhdr	virscanhdr;		/* Virus scanners	   */
struct	_virscan	virscan;

struct	_ttyinfohdr	ttyinfohdr;		/* TTY lines		   */
struct	_ttyinfo	ttyinfo;
struct	_modemhdr	modemhdr;		/* Modem models		   */
struct	_modem		modem;

struct	_tichdr		tichdr;			/* TIC areas		   */
struct	_tic		tic;
struct	_hatchhdr	hatchhdr;		/* Hatch areas		   */
struct	_hatch		hatch;
struct	_magichdr	magichdr;		/* Magic areas		   */
struct	_magic		magic;

struct	_nodeshdr	nodeshdr;		/* Fidonet nodes	   */
struct	_nodes		nodes;

struct	_bill		bill;			/* Unsent bills		   */

struct	_newfileshdr	newfileshdr;		/* New file reports	   */
struct	_newfiles	newfiles;

struct	_scanmgrhdr	scanmgrhdr;		/* Filefind areas	   */
struct	_scanmgr	scanmgr;

struct	_routehdr	routehdr;		/* Routing file		    */
struct	_route		route;

#endif

