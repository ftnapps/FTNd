/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Hangup functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "../lib/mberrors.h"
#include "dispfile.h"
#include "misc.h"
#include "language.h"
#include "bye.h"


extern	pid_t		mypid;
extern	time_t		t_start;
int			do_mailout = FALSE;


void Good_Bye(int onsig)
{
    FILE    *pUsrConfig, *pExitinfo;
    char    *temp;
    long    offset;
    time_t  t_end;

    IsDoing("Hangup");
    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('+', "Good_Bye()");

    if (onsig != SIGHUP)
	DisplayFile((char *)"goodbye");

    if (do_mailout)
	CreateSema((char *)"mailout");

    SaveLastCallers();

    /*
     * Update the users database record.
     */
    sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((pUsrConfig = fopen(temp,"r+b")) != NULL) {
	sprintf(temp, "%s/%s/exitinfo", CFG.bbs_usersdir, exitinfo.Name);
	if ((pExitinfo = fopen(temp,"rb")) != NULL) {
	    fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
	    offset = usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize);
	    fread(&exitinfo, sizeof(exitinfo), 1, pExitinfo);

	    usrconfig = exitinfo;
	    fclose(pExitinfo);

	    usrconfig.iLastFileArea = iAreaNumber;
	    if (!iAreaNumber)
		WriteError("Setting filearea to zero");

	    /* If time expired, do not say say successful logoff */
	    if (!iExpired)
		Syslog('+', "User successfully logged off BBS");

	    usrconfig.iLastMsgArea = iMsgAreaNumber;

	    offset = usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize);
	    Syslog('b', "Good_Bye: write users.data at offset %ld", offset);
	    if (fseek(pUsrConfig, offset, 0) != 0) {
		WriteError("Can't move pointer in file %s", temp);
		ExitClient(MBERR_GENERAL);
	    }

	    fwrite(&usrconfig, sizeof(usrconfig), 1, pUsrConfig);
	    fclose(pUsrConfig);
	}
    }

    t_end = time(NULL);
    Syslog(' ', "MBSEBBS finished in %s", t_elapsed(t_start, t_end));

    /*
     * Start shutting down this session
     */
    socket_shutdown(mypid);
    sprintf(temp, "%s/tmp/mbsebbs%d", getenv("MBSE_ROOT"), getpid());
    unlink(temp);

    sprintf(temp, "%s/%s/exitinfo", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp);
    free(temp);
    unlink("taglist");

    /*
     * Flush all data to the user, wait 5 seconds to
     * be sure the user received all data, this program
     * and parent are also finished.
     */
    colour(7, 0);
    fflush(stdout);	
    fflush(stdin);
    sleep(5);

    Unsetraw();
    Free_Language();
    free(pTTY);
#ifdef MEMWATCH
    mwTerm();
#endif
    exit(onsig);
}



void Quick_Bye(int onsig)
{
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('+', "Quick_Bye");
    socket_shutdown(mypid);
    sprintf(temp, "%s/tmp/mbsebbs%d", getenv("MBSE_ROOT"), getpid());
    unlink(temp);
    free(temp);

    colour(7, 0);
    fflush(stdout);
    fflush(stdin);
    sleep(3);

    Free_Language();
    free(pTTY);
#ifdef MEMWATCH
    mwTerm();
#endif
    exit(MBERR_OK);
}



