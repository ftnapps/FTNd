/*****************************************************************************
 *
 * $Id: aliasdb.c,v 1.10 2008/11/26 22:12:28 mbse Exp $
 * Purpose ...............: Alias Database
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
 *   
 * Michiel Broek                FIDO:           2:280/2802
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
#include "aliasdb.h"



typedef struct	_aliasrec {
	char	freename[MAXNAME];	/* Internet address			*/
	char	address[128];		/* Fidonet address			*/
	time_t	dtime;			/* Time the record is added/updated	*/
} aliasrec;


static int	opened = 0;
FILE		*afp = NULL;


static int alias_db_init(void);
void close_alias_db(void);


static int alias_db_init(void)
{
	char		buf[PATH_MAX];
	struct stat	stbuf;
	int		tries = 0;
        struct flock    txflock;

        txflock.l_type   = F_WRLCK;
        txflock.l_whence = SEEK_SET;
        txflock.l_start  = 0L;
        txflock.l_len    = 0L;

	if (opened == -1) 
		return -1;
	if (opened) 
		return 0;

	snprintf(buf, PATH_MAX, "%s/var/aliases.data", getenv("MBSE_ROOT"));
	if (stat(buf, &stbuf) != 0) {
		afp = fopen(buf,"a");
		if (afp) 
			fclose(afp);
	}

	if ((afp = fopen(buf, "r+")) == NULL) {
		WriteError("$Can't open %s", buf);
		return -1;
	}

	/*
         * Now lock it.
         */
        while (fcntl(fileno(afp), F_SETLK, &txflock) != 0) {
                if (tries > 4)
                        Syslog('+', "Alias database locked %d errno=%d %s", tries +1, errno, strerror(errno));
                msleep(250);
                if (++tries >= 60) {
                        fclose(afp);
			afp = NULL;
                        WriteError("$Error locking alias database");
                        return -1;
                }
        }

	opened = 1;
	return 0;
}



int registrate(char *freename, char *address)
{
	char		buf[128], *p, *q;
	int		first;
	aliasrec	key;

	if (alias_db_init()) 
		return 1;

	if (strlen(freename) > MAXNAME)
		freename[MAXNAME] = '\0';
	strncpy(buf, freename, sizeof(buf)-1);
	first = TRUE;
	for (p = buf, q = buf; *p; p++) 
		switch (*p) {
		case '.':	*p=' '; /* fallthrough */
		case ' ':	if (first) {
					*(q++) = *p;
					first = FALSE;
				}
				break;
		default:	*(q++) = *p;
				first = 1;
				break;
		}

	*q = '\0';
	Syslog('m', "Registrate \"%s\" \"%s\"", buf, MBSE_SS(address));

	while (fread(&key, sizeof(key), 1, afp)) {
		if (!strcmp(key.freename, buf)) {
			/*
			 *  Already present, update date/time.
			 */
			key.dtime = time(NULL);
			fseek(afp, - sizeof(key), SEEK_CUR);
			fwrite(&key, sizeof(key), 1, afp);
			close_alias_db();
			return 1;
		}
	}
	
	snprintf(key.freename, MAXNAME, "%s", buf);
	snprintf(key.address, 128, "%s", address);
	key.dtime = time(NULL);

	if (fwrite(&key, sizeof(key), 1, afp) != 1) {
		WriteError("$Cannot store: \"%s\" \"%s\"", buf, MBSE_SS(address));
	} else {
		Syslog('m', "Registered \"%s\" as \"%s\"", buf, MBSE_SS(address));
	}
	close_alias_db();
	return 1;
}



char *lookup(char *freename)
{
	static	char	buf[128], *p, *q;
	int		first;
	aliasrec        key;

	if (alias_db_init()) 
		return NULL;

	strncpy(buf, freename, sizeof(buf) -1);
	first = TRUE;
	for (p = buf, q = buf; *p; p++) 
		switch (*p) {
		case '.':	*p=' '; /* fallthrough */
		case ' ':	if (first) {
					*(q++) = *p;
					first = FALSE;
				}
				break;
		default:	*(q++) = *p;
				first = TRUE;
				break;
		}

	*q = '\0';
	Syslog('m', "Lookup \"%s\"", MBSE_SS(freename));

        while (fread(&key, sizeof(key), 1, afp)) {
                if (!strcmp(key.freename, buf)) {
                        /*
                         *  Already present
                         */
                        close_alias_db();
			Syslog('m',"Found: \"%s\"",buf);
                        return buf;
                }
        }

	Syslog('m',"Not found: \"%s\"",buf);
	close_alias_db();
	return NULL;
}



void close_alias_db(void)
{
	if (opened != 1) 
		return;
	fclose(afp);
	afp = NULL;
	opened = 0;
}


