/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Characterset functions
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
#include "mbselib.h"


char *getchrs(int val)
{
    switch (val) {
        case FTNC_NONE:     return (char *)"Undefined";
        case FTNC_CP437:    return (char *)"CP437 2";
        case FTNC_CP850:    return (char *)"CP850 2";
        case FTNC_CP865:    return (char *)"CP865 2";
        case FTNC_CP866:    return (char *)"CP866 2";
        case FTNC_LATIN_1:  return (char *)"LATIN-1 2";
        case FTNC_LATIN_2:  return (char *)"LATIN-2 2";
        case FTNC_LATIN_5:  return (char *)"LATIN-5 2";
        case FTNC_MAC:      return (char *)"MAC 2";
        default:            return (char *)"LATIN-1 2";
    }
}


char *getchrsdesc(int val)
{
    switch (val) {
        case FTNC_NONE:     return (char *)"Undefined";
        case FTNC_CP437:    return (char *)"IBM codepage 437 (Western European) (ANSI terminal)";
        case FTNC_CP850:    return (char *)"IBM codepage 850 (Latin-1)";
        case FTNC_CP865:    return (char *)"IBM codepage 865 (Nordic)";
        case FTNC_CP866:    return (char *)"IBM codepage 866 (Russian)";
        case FTNC_LATIN_1:  return (char *)"ISO 8859-1 (Western European)";
        case FTNC_LATIN_2:  return (char *)"ISO 8859-2 (Eastern European)";
        case FTNC_LATIN_5:  return (char *)"ISO 8859-5 (Turkish)";
        case FTNC_MAC:      return (char *)"MacIntosh character set";
        default:            return (char *)"ERROR";
    }
}


