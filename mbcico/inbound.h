/*****************************************************************************
 *
 * $Id: inbound.h,v 1.3 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Fidonet mailer, inbound functions 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:	2:280/2802
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

#ifndef	_INBOUND_H
#define	_INBOUND_H


int	inbound_open(faddr *, int, int);    /* Open temp inbound	*/
int	inbound_close(int);		    /* Close temp inbound	*/
int	inbound_space(void);		    /* Get free inbound space   */

#endif
