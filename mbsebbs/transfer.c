/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: File Transfers
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "transfer.h"


/*
 
   Design remarks.

   This entry must accept up and downloads at the same time in case a users
   wants to download but uses a bidirectional protocol and has something to
   upload.

   First design fase: support the now used external protocols and link it to
   the rest of the bbs.

   Second design fase: add build-in zmodem and ymodem-1k.

   To think of:
    - Drop bidirectional support, is this still in use. No bimodem specs and
      Hydra seems not implemented in any terminal emulator.
    - Add sliding kermit? Can still be used externally.
    - Add special internet protocol and make a module for minicom. No, this
      would mean only a few users will use it.

*/

int download(down_list **download_list)
{
    return 0;
}


int upload(up_list **upload_list)
{
    return 0;
}



