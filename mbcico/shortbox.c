/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "shortbox.h"


/*
 * Returns name of T-Mail filebox in Dos format (8+3).
 * I know, this code looks ungly, but it works. - MiCHA :-)
 */
char* shortboxname(faddr *fa) {
    static char dirname[12];
    unsigned    z=fa->zone, n=fa->net, f=fa->node, p=fa->point;
    unsigned    u,v;

    u=z%32; z/=32; if (z>=32) return NULL;
    dirname[0]=z<10?z+'0':z-10+'a';
    dirname[1]=u<10?u+'0':u-10+'a';
    u=n%32; n/=32; v=n%32; n/=32; if (n>=32) return NULL;
    dirname[2]=n<10?n+'0':n-10+'a';
    dirname[3]=v<10?v+'0':v-10+'a';
    dirname[4]=u<10?u+'0':u-10+'a';
    u=f%32; f/=32; v=f%32; f/=32; if (f>=32) return NULL;
    dirname[5]=f<10?f+'0':f-10+'a';
    dirname[6]=v<10?v+'0':v-10+'a';
    dirname[7]=u<10?u+'0':u-10+'a';
    dirname[8]='.';
    u=p%32; p/=32; if (p>=32) return NULL;
    dirname[9]=p<10?p+'0':p-10+'a';
    dirname[10]=u<10?u+'0':u-10+'a';
    dirname[11]=0;
    return dirname;
}


