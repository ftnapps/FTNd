/*****************************************************************************
 *
 * File ..................: mbfido/cookie.h
 * Purpose ...............: Create a cookie
 * Last modification date : 19-Mar-1999
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "cookie.h"



char *Cookie()
{
	FILE	*olf;
	char	fname[81];
	int	i, j, in, id;
	int	recno = 0;
	long	offset;
	int	nrecno;
	static	char temp[81];

	sprintf(fname, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if ((olf = fopen(fname, "r")) == NULL) {
		WriteError("$Can't open %s", fname);
		return '\0';
	}

	fread(&olhdr, sizeof(olhdr), 1, olf);
	while (fread(&ol, olhdr.recsize, 1, olf) == 1) {
		recno++;
	}
	nrecno = recno;

	/*
	 * Generate random record number
	 */
	while (TRUE) {
		in = nrecno;
		id = getpid();

		i = rand();
		j = i % id;
		if ((j <= in))
			break;
	}

	offset = olhdr.hdrsize + (j * olhdr.recsize);
	if (fseek(olf, offset, SEEK_SET) != 0) {
		WriteError("$Can't move pointer in %s", fname);
		return '\0';
	}
	fread(&ol, olhdr.recsize, 1, olf);
	fclose(olf);

	memset(&temp, 0, sizeof(temp));
	strcpy(temp, ol.Oneline);
	return temp;
}


