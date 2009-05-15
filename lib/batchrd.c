/*****************************************************************************
 *
 * $Id: batchrd.c,v 1.6 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: Batch reading
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "mbselib.h"


static int	counter = 0L;
static int	batchmode = -1;
int		usetmp = 0;

char *bgets(char *buf, int count, FILE *fp)
{
	if (usetmp) {
		return fgets(buf,count,fp);
	}

	if ((batchmode == 1) && (counter > 0L) && (counter < (count-1))) 
		count=(counter+1);
	if (fgets(buf,count,fp) == NULL) 
		return NULL;

	switch (batchmode) {
	case -1: if (!strncmp(buf,"#! rnews ",9) || !strncmp(buf,"#!rnews ",8)) {
			batchmode=1;
			sscanf(buf+8,"%d",&counter);
			Syslog('m', "first chunk of input batch: %d",counter);
			if (counter < (count-1)) 
				count=(counter+1);
			if (fgets(buf,count,fp) == NULL) 
				return NULL;
			else {
				counter -= strlen(buf);
				return(buf);
			}
		} else {
			batchmode=0;
			return buf;
		}

	case 0:	return buf;

	case 1:	if (counter <= 0L) {
			while (strncmp(buf,"#! rnews ",9) && strncmp(buf,"#!rnews ",8)) {
				Syslog('+', "batch out of sync: %s",buf);
				if (fgets(buf,count,fp) == NULL) 
					return NULL;
			}
			sscanf(buf+8,"%d",&counter);
			Syslog('m', "next chunk of input batch: %d",counter);
			return NULL;
		} else {
			counter -= strlen(buf);
			Syslog('m', "bread \"%s\", %d left of this chunk", buf,counter);
			return buf;
		}
	}
	return buf;
}


