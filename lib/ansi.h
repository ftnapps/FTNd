/*****************************************************************************
 *
 * File ..................: mbse.h
 * Purpose ...............: ANSI Screen definitions
 * Last modification date : 10-Feb-1999
 *
 *****************************************************************************
 * Copyright (C) 1997-1999
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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

#ifndef _ANSI_H
#define _ANSI_H


#define ANSI_RED	"\x1B[31;1m"
#define ANSI_YELLOW	"\x1B[33;1m"
#define ANSI_BLUE	"\x1B[34;1m"
#define ANSI_GREEN	"\x1B[32;1m"
#define ANSI_WHITE	"\x1B[37;1m"
#define ANSI_CYAN	"\x1B[36;1m"
#define ANSI_MAGENTA	"\x1B[35m"

#define	ANSI_HOME	"\x1B[H"
#define ANSI_UP		"\x1B[A"
#define ANSI_DOWN	"\x1B[B"
#define	ANSI_RIGHT	"\x1B[C"
#define ANSI_LEFT	"\x1B[D"

#define ANSI_BOLD	"\x1B[1m"
#define ANSI_NORMAL	"\x1B[0m"
#define ANSI_CLEAR	"\x1B[2J"
#define	ANSI_CLREOL	"\x1B[K"


#endif
