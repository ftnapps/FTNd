/*****************************************************************************
 *
 * $Id: auth.c,v 1.5 2005/09/07 20:44:37 mbse Exp $
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

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "mbnntp.h"
#include "auth.h"

#ifndef	USE_NEWSGATE

int	authorized = FALSE;	    /* Authentication status	*/
int	got_username = FALSE;	    /* Did we get a username?	*/
int	grecno = 0;		    /* User record number	*/
char	username[9];		    /* Cached username		*/

extern pid_t	mypid;


int check_auth(char *cmd)
{
    if (authorized)
	return TRUE;

    send_nntp("480 Authentication required");
    return FALSE;
}



void auth_user(char *cmd)
{
    char    *p;
    
    p = strtok(cmd, " \0");
    p = strtok(NULL, " \0");
    p = strtok(NULL, " \0");
    if (strlen(p) > 8) {
	/*
	 * Username too long
	 */
	WriteError("Got a username of %d characters", sizeof(p));
	send_nntp("482 Authentication rejected");
	return;
    }
    memset(&username, 0, sizeof(username));
    strncpy(username, p, 8);
    send_nntp("381 More authentication information required");
    got_username = TRUE;
}



void auth_pass(char *cmd)
{
    char    *p, *temp;
    FILE    *fp;
    int	    FoundName = FALSE;

    if (! got_username) {
	WriteError("Got AUTHINFO PASS before AUTHINFO USER");
	send_nntp("482 Authentication rejected");
	return;
    }

    p = strtok(cmd, " \0");
    p = strtok(NULL, " \0");
    p = strtok(NULL, " \0");

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp,"r+")) == NULL) {
	/*
	 * This should not happen
	 */
	WriteError("$Can't open %s", temp);
	send_nntp("482 Authentication rejected");
	free(temp);
	got_username = FALSE;
	return;
    }
    free(temp);

    grecno = 0;
    fread(&usrconfighdr, sizeof(usrconfighdr), 1, fp);
    while (fread(&usrconfig, usrconfighdr.recsize, 1, fp) == 1) {
	if (strcmp(usrconfig.Name, username) == 0) {
	    FoundName = TRUE;
	    usercharset=usrconfig.Charset;
	    break;
	}
	grecno++;
    }

    if (!FoundName) {
	fclose(fp);
	Syslog('+', "Unknown user \"%s\", reject", username);
	send_nntp("482 Authentication rejected");
	got_username = FALSE;
	memset(&usrconfig, 0, sizeof(usrconfig));
	return;
    }
    
    if (strcasecmp(usrconfig.Password, p)) {
	Syslog('+', "Password error, reject user \"%s\"", username);
	send_nntp("482 Authentication rejected");
	fclose(fp);
	got_username = FALSE;
	memset(&usrconfig, 0, sizeof(usrconfig));
	return;
    }

    /*
     * Update user record
     */
    usrconfig.tLastLoginDate = time(NULL);  /* Login date is current date   */
    usrconfig.iTotalCalls++;		    /* Increase calls		    */
	
    /*
     * Update user record.
     */
    if (fseek(fp, usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize), 0) != 0) {
	WriteError("Can't seek in %s/etc/users.data", getenv("MBSE_ROOT"));
    } else {
	fwrite(&usrconfig, sizeof(usrconfig), 1, fp);
    }
    fclose(fp);

    /*
     * User logged in, tell it to the server. Check if a location is
     * set, if Ask User location for new users is off, this field is
     * empty but we have to send something to the server.
     */
    if (strlen(usrconfig.sLocation))
	UserCity(mypid, usrconfig.Name, usrconfig.sLocation);
    else
	UserCity(mypid, usrconfig.Name, (char *)"N/A");

    IsDoing("Logged in");
    Syslog('+', "User %s logged in", username);
    authorized = TRUE;
    got_username = FALSE;
    send_nntp("281 Authentication accepted");
}

#endif
