/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Give system information
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "taskinfo.h"



/*
 *  Get BBS System info.
 */
char *get_sysinfo(void)
{
    FILE	*fp;
    static char	buf[SS_BUFSIZE];
    char	*temp;
    time_t	startdate;

    snprintf(buf, SS_BUFSIZE, "201:1,16;");
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));

    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return buf;
    }
    free(temp);

    if (fread(&SYSINFO, sizeof(SYSINFO), 1, fp) == 1) {
	startdate = SYSINFO.StartDate;
	snprintf(buf, SS_BUFSIZE, "100:7,%ld,%ld,%ld,%ld,%ld,%s,%s;", SYSINFO.SystemCalls,
			SYSINFO.Pots, SYSINFO.ISDN, SYSINFO.Network, SYSINFO.Local,
			ctime(&startdate), SYSINFO.LastCaller);
    }

    fclose(fp);

    return buf;
}



char *get_lastcallercount(void)
{
    static char	buf[41];
    char	*temp;
    FILE	*fp;

    snprintf(buf, 41, "100:1,0;");
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/lastcall.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return buf;
    }
    
    fread(&LCALLhdr, sizeof(LCALLhdr), 1, fp);
    fseek(fp, 0, SEEK_END);
    snprintf(buf, 41, "100:1,%ld;", ((ftell(fp) - LCALLhdr.hdrsize) / LCALLhdr.recsize));
    fclose(fp);
    free(temp);
    return buf;
}



char *get_lastcallerrec(int Rec)
{
	static char		buf[SS_BUFSIZE];
	char                    *temp, action[9];
	FILE                    *fp;

	snprintf(buf, SS_BUFSIZE, "201:1,16;");
	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/etc/lastcall.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
		free(temp);
		return buf;
	}
	fread(&LCALLhdr, sizeof(LCALLhdr), 1, fp);
	fseek(fp, ((Rec -1) * LCALLhdr.recsize) + LCALLhdr.hdrsize, SEEK_SET);

	if (fread(&LCALL, LCALLhdr.recsize, 1, fp) == 1) {
		LCALL.UserName[15] = '\0';
		LCALL.Location[12] = '\0';
		strcpy(action, "--------");
		if (LCALL.Hidden)
			action[0] = 'H';
		if (LCALL.Download)
			action[1] = 'D';
		if (LCALL.Upload)
			action[2] = 'U';
		if (LCALL.Read)
			action[3] = 'R';
		if (LCALL.Wrote)
			action[4] = 'P';
		if (LCALL.Chat)
			action[5] = 'C';
		if (LCALL.Olr)
			action[6] = 'O';
		if (LCALL.Door)
			action[7] = 'E';
		action[8] = '\0';
		snprintf(buf, SS_BUFSIZE, "100:9,%s,%s,%d,%s,%s,%d,%d,%s,%s;", LCALL.UserName, LCALL.Location,
			LCALL.SecLevel, LCALL.Device, LCALL.TimeOn, 
			LCALL.CallTime, LCALL.Calls, LCALL.Speed, action);
	}

	free(temp);
	fclose(fp);
	return buf;
}


