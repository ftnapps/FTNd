/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS functions for TURBODIESEL
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "users.h"
#include "records.h"
#include "clcomm.h"
#include "diesel.h"



FILE *OpenMacro(const char *filename, int Language)
{
    FILE	*pLang, *fi = NULL;
    char	*temp;
		            
    temp = calloc(PATH_MAX, sizeof(char));
    temp[0] = '\0';

    if (Language != '\0') {
	/*
	 * Maybe a valid language character, try to load the language
	 */
	sprintf(temp, "%s/etc/language.data", getenv("MBSE_ROOT"));
	if ((pLang = fopen(temp, "rb")) == NULL) {
	    WriteError("mbdiesel: Can't open language file: %s", temp);
	} else {
	    fread(&langhdr, sizeof(langhdr), 1, pLang);
    
	    while (fread(&lang, langhdr.recsize, 1, pLang) == 1) {
		if ((lang.LangKey[0] == Language) && (lang.Available)) {
		    sprintf(temp,"%s/%s", lang.MacroPath, filename);
		    break;
		}
	    }
	    fclose(pLang);
	}
    }
    
    /*
     * Try to open the selected language
     */
    if (temp[0] != '\0')
	fi = fopen(temp, "r");

    /*
     * If no selected language is loaded, try default language
     */
    if (fi == NULL) {
	sprintf(temp, "%s/%s", CFG.bbs_macros, filename);
	fi = fopen(temp,"r");
    }

    if (fi == NULL)
	Syslog('d', "OpenMacro(%s, %c): not found, using hardcoded", filename, Language);
    else
	Syslog('d', "OpenMacro(%s, %c): using %s", filename, Language, temp);

    free(temp);
    return fi;
}


