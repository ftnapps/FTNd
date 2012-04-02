/*****************************************************************************
 *
 * $Id: taskinfo.c,v 1.12 2006/01/30 22:27:03 mbse Exp $
 * Purpose ...............: Give system information
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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
void get_sysinfo_r(char *buf)
{
    FILE	*fp;
    char	*temp;
    time_t	startdate;

    snprintf(buf, SS_BUFSIZE, "201:1,16;");
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));

    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return;
    }

    if (fread(&SYSINFO, sizeof(SYSINFO), 1, fp) == 1) {
	startdate = SYSINFO.StartDate;
	ctime_r(&startdate, temp);
	snprintf(buf, SS_BUFSIZE, "100:7,%d,%d,%d,%d,%d,%s,%s;", SYSINFO.SystemCalls,
			SYSINFO.Pots, SYSINFO.ISDN, SYSINFO.Network, SYSINFO.Local,
			temp, clencode(SYSINFO.LastCaller));
    }

    free(temp);
    fclose(fp);

    return;
}



void get_lastcallercount_r(char *buf)
{
    char	*temp;
    FILE	*fp;

    snprintf(buf, SS_BUFSIZE, "100:1,0;");
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/lastcall.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return;
    }
    
    fread(&LCALLhdr, sizeof(LCALLhdr), 1, fp);
    fseek(fp, 0, SEEK_END);
    snprintf(buf, SS_BUFSIZE, "100:1,%ld;", ((ftell(fp) - LCALLhdr.hdrsize) / LCALLhdr.recsize));
    fclose(fp);
    free(temp);
    return;
}



void get_lastcallerrec_r(int Rec, char *buf)
{
    char        *temp, action[9], *name, *city;
    FILE        *fp;

    snprintf(buf, SS_BUFSIZE, "201:1,16;");
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/lastcall.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	return;
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
	name = xstrcpy(clencode(LCALL.UserName));
	city = xstrcpy(clencode(LCALL.Location));
	snprintf(buf, SS_BUFSIZE, "100:9,%s,%s,%d,%s,%s,%d,%d,%s,%s;", name, city,
			LCALL.SecLevel, LCALL.Device, LCALL.TimeOn, 
			(int)LCALL.CallTime, LCALL.Calls, LCALL.Speed, action);
	free(name);
	free(city);
    }

    free(temp);
    fclose(fp);
    return;
}


