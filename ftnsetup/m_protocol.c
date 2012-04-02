/*****************************************************************************
 *
 * $Id: m_protocol.c,v 1.19 2005/10/11 20:49:49 mbse Exp $
 * Purpose ...............: Setup Protocols.
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../paths.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_protocol.h"



int	ProtUpdated = 0;
int	ProtRecords = 0;



/*
 * Count nr of PROT records in the database.
 * Creates the database if it doesn't exist.
 */
int CountProtocol(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];
    int	    count;

    snprintf(ffile, PATH_MAX, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    PROThdr.hdrsize = sizeof(PROThdr);
	    PROThdr.recsize = sizeof(PROT);
	    fwrite(&PROThdr, sizeof(PROThdr), 1, fil);

	    /*
	     * Write default set of protocols
	     */
	    memset(&PROT, 0, sizeof(PROT));
	    snprintf(PROT.ProtKey,       2, "A");
	    snprintf(PROT.ProtName,     21, "Ymodem");
	    if (strlen(_PATH_SB) && strlen(_PATH_RB)) {
		snprintf(PROT.ProtUp,   51, "%s -v", _PATH_RB);
		snprintf(PROT.ProtDn,   51, "%s -v -u", _PATH_SB);
	    } else {
		snprintf(PROT.ProtUp,   51, "/usr/bin/rb -v");
		snprintf(PROT.ProtDn,   51, "/usr/bin/sb -v -u");
	    }
	    PROT.Available = FALSE;
	    snprintf(PROT.Advice,       31, "Press Ctrl-X to abort");
	    PROT.Efficiency = 75;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    snprintf(PROT.ProtKey,       2, "B");
	    snprintf(PROT.ProtName,     21, "Ymodem-1K");
	    if (strlen(_PATH_SB) && strlen(_PATH_RB)) {
		snprintf(PROT.ProtUp,   51, "%s -k -v", _PATH_RB);
		snprintf(PROT.ProtDn,   51, "%s -k -v -u", _PATH_SB);
	    } else {
		snprintf(PROT.ProtUp,   51, "/usr/bin/rb -k -v");
		snprintf(PROT.ProtDn,   51, "/usr/bin/sb -k -v -u");
	    }
	    PROT.Efficiency = 82;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    snprintf(PROT.ProtKey,       2, "C");
	    snprintf(PROT.ProtName,     21, "Zmodem");
	    if (strlen(_PATH_SZ) && strlen(_PATH_RZ)) {
		snprintf(PROT.ProtUp,   51, "%s -p -v", _PATH_RZ);
		snprintf(PROT.ProtDn,   51, "%s -b -q -r -u", _PATH_SZ);
	    } else {
		snprintf(PROT.ProtUp,   51, "/usr/bin/rz -p -v");
		snprintf(PROT.ProtDn,   51, "/usr/bin/sz -b -q -r -u");
	    }
	    PROT.Efficiency = 98;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

            snprintf(PROT.ProtKey,       2, "L");
	    snprintf(PROT.ProtName,     21, "Local disk");
	    snprintf(PROT.ProtUp,       51, "%s/bin/rf", getenv("MBSE_ROOT"));
	    snprintf(PROT.ProtDn,       51, "%s/bin/sf", getenv("MBSE_ROOT"));
	    snprintf(PROT.Advice,       31, "It goes before you know");
	    PROT.Level.level = 32000;
	    PROT.Efficiency = 100;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    memset(&PROT, 0, sizeof(PROT));
	    PROT.Internal = TRUE;
	    PROT.Available = TRUE;
	    snprintf(PROT.Advice,       31, "Press Ctrl-X to abort");
            snprintf(PROT.ProtKey,       2, "1");
	    snprintf(PROT.ProtName,     21, "Ymodem-1K");
	    PROT.Efficiency = 82;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    snprintf(PROT.ProtKey,       2, "8");
	    snprintf(PROT.ProtName,     21, "Zmodem-8K (ZedZap)");
	    PROT.Efficiency = 99;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    snprintf(PROT.ProtKey,       2, "G");
	    snprintf(PROT.ProtName,     21, "Ymodem-G");
	    PROT.Efficiency = 90;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    PROT.Available = FALSE;
	    snprintf(PROT.ProtKey,       2, "X");
	    snprintf(PROT.ProtName,     21, "Xmodem");
	    PROT.Efficiency = 75;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    PROT.Available = TRUE;
	    snprintf(PROT.ProtKey,       2, "Y");
	    snprintf(PROT.ProtName,     21, "Ymodem");
	    PROT.Efficiency = 75;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    snprintf(PROT.ProtKey,       2, "Z");
	    snprintf(PROT.ProtName,     21, "Zmodem");
	    PROT.Efficiency = 92;
	    fwrite(&PROT, sizeof(PROT), 1, fil);

	    fclose(fil);
	    chmod(ffile, 0640);
	    ProtUpdated = 1;
	    return 10;
	} else {
	    return -1;
	}
    } else {
	fread(&PROThdr, sizeof(PROThdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - PROThdr.hdrsize) / PROThdr.recsize;
	fclose(fil);
    }

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenProtocol(void);
int OpenProtocol(void)
{
    FILE    *fin, *fout;
    char    fnin[PATH_MAX], fnout[PATH_MAX], newkey = 'A', *usedkeys;
    int	    oldsize;
    int	    AddInt = TRUE;

    snprintf(fnin,  PATH_MAX, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&PROThdr, sizeof(PROThdr), 1, fin);
	    usedkeys = xstrcpy((char *)"18GXYZ");

	    /*
	     * In case we are automaic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize = PROThdr.recsize;
	    if (oldsize != sizeof(PROT)) {
		ProtUpdated = 1;
		Syslog('+', "Upgraded %s, format changed", fnin);
	    }

	    /*
	     * First check if the database needs upgrade with internal
	     * protocols.
	     */
	    while (fread(&PROT, oldsize, 1, fin) == 1) {
		if (PROT.Internal) {
		    AddInt = FALSE;
		    usedkeys = xstrcat(usedkeys, PROT.ProtKey);
		}
	    }
	    fseek(fin, PROThdr.hdrsize, SEEK_SET);
	    if (AddInt)
		Syslog('+', "Will add new internal transfer protocols");

	    PROThdr.hdrsize = sizeof(PROThdr);
	    PROThdr.recsize = sizeof(PROT);
	    fwrite(&PROThdr, sizeof(PROThdr), 1, fout);
				    
	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&PROT, 0, sizeof(PROT));
	    while (fread(&PROT, oldsize, 1, fin) == 1) {
		if (!PROT.Internal) {
		    /*
		     * Check selection key, if it is one of the new
		     * internal ones, change the key. Also disable
		     * the external protocols.
		     */
		    if (strstr(usedkeys, PROT.ProtKey)) {
			Syslog('+', "Change external protocol %s key %s to %c", PROT.ProtName, PROT.ProtKey, newkey);
			snprintf(PROT.ProtKey, 2, "%c", newkey);
			newkey++;
			ProtUpdated = 1;
		    }
		    if (AddInt && PROT.Available) {
			Syslog('+', "Disable external protocol %s", PROT.ProtName);
			PROT.Available = FALSE;
			ProtUpdated = 1;
		    }
		}
		fwrite(&PROT, sizeof(PROT), 1, fout);
		memset(&PROT, 0, sizeof(PROT));
	    }

	    if (AddInt) {
		Syslog('+', "Adding new internal protocols");
		memset(&PROT, 0, sizeof(PROT));
		PROT.Internal = TRUE;
		PROT.Available = TRUE;
		snprintf(PROT.Advice,      31, "Press Ctrl-X to abort");
		snprintf(PROT.ProtKey,      2,"1");
		snprintf(PROT.ProtName,     21,"Ymodem-1K");
		PROT.Efficiency = 82;
		fwrite(&PROT, sizeof(PROT), 1, fout);
												            
		snprintf(PROT.ProtKey,      2,"8");
		snprintf(PROT.ProtName,     21,"Zmodem-8K (ZedZap)");
		PROT.Efficiency = 99;
		fwrite(&PROT, sizeof(PROT), 1, fout);

		snprintf(PROT.ProtKey,      2,"G");
		snprintf(PROT.ProtName,     21,"Ymodem-G");
		PROT.Efficiency = 90;
		fwrite(&PROT, sizeof(PROT), 1, fout);

		PROT.Available = FALSE;
		snprintf(PROT.ProtKey,      2,"X");
		snprintf(PROT.ProtName,     21,"Xmodem");
		PROT.Efficiency = 75;
		fwrite(&PROT, sizeof(PROT), 1, fout);

		PROT.Available = TRUE;
		snprintf(PROT.ProtKey,      2,"Y");
		snprintf(PROT.ProtName,     21,"Ymodem");
		PROT.Efficiency = 75;
		fwrite(&PROT, sizeof(PROT), 1, fout);

		snprintf(PROT.ProtKey,      2,"Z");
		snprintf(PROT.ProtName,     21,"Zmodem");
		PROT.Efficiency = 92;
		fwrite(&PROT, sizeof(PROT), 1, fout);
		ProtRecords += 6;
		ProtUpdated = 1;
	    }

	    fclose(fin);
	    fclose(fout);
	    free(usedkeys);
	    return 0;
	} else
	    return -1;
    }
    return -1;
}



void CloseProtocol(int);
void CloseProtocol(int force)
{
    char    fin[PATH_MAX], fout[PATH_MAX];
    FILE    *fi, *fo;
    st_list *pro = NULL, *tmp;

    snprintf(fin,  PATH_MAX, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
    snprintf(fout, PATH_MAX, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));

    if (ProtUpdated == 1) {
	if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
	    working(1, 0, 0);
	    fi = fopen(fout, "r");
	    fo = fopen(fin,  "w");
	    fread(&PROThdr, PROThdr.hdrsize, 1, fi);
	    fwrite(&PROThdr, PROThdr.hdrsize, 1, fo);

	    while (fread(&PROT, PROThdr.recsize, 1, fi) == 1)
		if (!PROT.Deleted)
		    fill_stlist(&pro, PROT.ProtName, ftell(fi) - PROThdr.recsize);
	    sort_stlist(&pro);

	    for (tmp = pro; tmp; tmp = tmp->next) {
	        fseek(fi, tmp->pos, SEEK_SET);
	        fread(&PROT, PROThdr.recsize, 1, fi);
	        fwrite(&PROT, PROThdr.recsize, 1, fo);
	    }

	    fclose(fi);
	    fclose(fo);
	    unlink(fout);
	    chmod(fin, 0640);
	    tidy_stlist(&pro);
	    Syslog('+', "Updated \"protocol.data\"");
	    if (!force)
	        working(6, 0, 0);
	    ProtUpdated = 0;
	    return;
	}
    }
    chmod(fin, 0640);
    working(1, 0, 0);
    unlink(fout); 
}



int AppendProtocol(void)
{
    FILE	*fil;
    char	ffile[PATH_MAX];

    snprintf(ffile, PATH_MAX, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "a")) != NULL) {
	memset(&PROT, 0, sizeof(PROT));
	fwrite(&PROT, sizeof(PROT), 1, fil);
	fclose(fil);
	ProtUpdated = 1;
	return 0;
    } else
	return -1;
}



void s_protrec(void);
void s_protrec(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "8.5 EDIT PROTOCOL");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.  Select Key");
    mbse_mvprintw( 8, 6, "2.  Name");
    mbse_mvprintw( 9, 6, "3.  Upload");
    mbse_mvprintw(10, 6, "4.  Download");
    mbse_mvprintw(11, 6, "5.  Available");
    mbse_mvprintw(12, 6, "6.  Internal");
    mbse_mvprintw(13, 6, "7.  Advice");
    mbse_mvprintw(14, 6, "8.  Efficiency");
    mbse_mvprintw(15, 6, "9.  Deleted");
    mbse_mvprintw(16, 6, "10. Sec. level");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditProtRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX];
    int		    offset;
    int		    j;
    unsigned int    crc, crc1;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Protocol");

    snprintf(mfile, PATH_MAX, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    offset = sizeof(PROThdr) + ((Area -1) * sizeof(PROT));
    if (fseek(fil, offset, 0) != 0) {
    	working(2, 0, 0);
    	return -1;
    }

    fread(&PROT, sizeof(PROT), 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&PROT, crc, sizeof(PROT));

    s_protrec();
	
    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  7,21, 1, PROT.ProtKey);
	show_str(  8,21,20, PROT.ProtName);
	show_str(  9,21,50, PROT.ProtUp);
	show_str( 10,21,50, PROT.ProtDn);
	show_bool(11,21,    PROT.Available);
	show_bool(12,21,    PROT.Internal);
	show_str( 13,21,30, PROT.Advice);
	show_int( 14,21,    PROT.Efficiency);
	show_bool(15,21,    PROT.Deleted);
	show_sec( 16,21,    PROT.Level);

	if (PROT.Internal) {
	    set_color(BLUE, BLACK);
	    show_str(  8,21,20, PROT.ProtName);
	    show_bool(12,21,    PROT.Internal);
	    show_str( 13,21,30, PROT.Advice);
	    show_int( 14,21,    PROT.Efficiency);
	    show_bool(15,21,    PROT.Deleted);
	}
	
	j = select_menu(10);
	switch(j) {
	    case 0: crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&PROT, crc1, sizeof(PROT));
		    if (crc != crc1) {
			if (yes_no((char *)"Record is changed, save") == 1) {
			    working(1, 0, 0);
			    if ((fil = fopen(mfile, "r+")) == NULL) {
				working(2, 0, 0);
				return -1;
			    }
			    fseek(fil, offset, 0);
			    fwrite(&PROT, sizeof(PROT), 1, fil);
			    fclose(fil);
			    ProtUpdated = 1;
			    working(6, 0, 0);
			}
		    }
		    IsDoing("Browsing Menu");
		    return 0;
	    case 1: E_UPS(  7,21,1, PROT.ProtKey,   "The ^Key^ to select this protocol")
	    case 2: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_STR(  8,21,20,PROT.ProtName,  "The ^name^ of this protocol")
	    case 3: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_STR(  9,21,50,PROT.ProtUp,    "The ^Upload^ path, binary and parameters")
	    case 4: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_STR( 10,21,50,PROT.ProtDn,    "The ^Download^ path, binary and parameters")
	    case 5: E_BOOL(11,21,   PROT.Available, "Is this protocol ^available^")
	    case 6: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_BOOL(12,21,   PROT.Internal,  "Is this a ^internal^ transfer protocol")
	    case 7: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_STR( 13,21,30,PROT.Advice,    "A small ^advice^ to the user, eg \"Press Ctrl-X to abort\"")
	    case 8: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_INT( 14,21,   PROT.Efficiency,"The ^efficiency^ in % of this protocol")
	    case 9: if (PROT.Internal)
			errmsg((char *)"Editing not allowd with internal protocol");
		    else
			E_BOOL(15,21,   PROT.Deleted,   "Is this protocol ^Deleted^")
	    case 10:E_SEC( 16,21,   PROT.Level,     "8.5.11  PROTOCOL SECURITY LEVEL", s_protrec)
	}
    }

    return 0;
}



void EditProtocol(void)
{
    int	    i, o = 0, y;
    char    pick[12];
    FILE    *fil;
    char    temp[PATH_MAX];
    int	    offset;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    ProtRecords = CountProtocol();
    if (ProtRecords == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenProtocol() == -1) {
	working(2, 0, 0);
	return;
    }

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "8.5 PROTOCOL SETUP");
	set_color(CYAN, BLACK);
	if (ProtRecords != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/protocol.temp", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&PROThdr, sizeof(PROThdr), 1, fil);
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 10; i++) {
		    if ((o + i) <= ProtRecords) {
			offset = sizeof(PROThdr) + (((o + i) - 1) * PROThdr.recsize);
			fseek(fil, offset, 0);
			fread(&PROT, PROThdr.recsize, 1, fil);
			if (PROT.Deleted)
			    set_color(LIGHTRED, BLACK);
			else if (PROT.Available)
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			snprintf(temp, 81, "%3d.  %1s %-20s %s %3d %-30s %5d", i, PROT.ProtKey, PROT.ProtName, 
				PROT.Internal?"Int":"Ext", PROT.Efficiency, PROT.Advice, PROT.Level.level);
			mbse_mvprintw(y, 4, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	    /* Show records here */
	}
	strcpy(pick, select_record(ProtRecords, 10));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseProtocol(FALSE);
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    working(1, 0, 0);
	    if (AppendProtocol() == 0) {
		ProtRecords++;
		working(1, 0, 0);
	    } else
		working(2, 0, 0);
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 10) < ProtRecords) 
		o = o + 10;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 10) >= 0)
		o = o - 10;

	if ((atoi(pick) >= 1) && (atoi(pick) <= ProtRecords)) {
	    EditProtRec(atoi(pick));
	    o = ((atoi(pick) - 1) / 10) * 10;
	}
    }
}



void InitProtocol(void)
{
    CountProtocol();
    OpenProtocol();
    CloseProtocol(TRUE);
}



char *PickProtocol(int nr)
{
    static char	Prot[21] = "";
    int		o = 0, i, x, y;
    char	pick[12];
    FILE	*fil;
    char	temp[PATH_MAX];
    int		offset;

    clr_index();
    working(1, 0, 0);
    if (config_read() == -1) {
	working(2, 0, 0);
	return Prot;
    }

    ProtRecords = CountProtocol();
    if (ProtRecords == -1) {
	working(2, 0, 0);
	return Prot;
    }

    clr_index();
    set_color(WHITE, BLACK);
    snprintf(temp, 81, "%d.  PROTOCOL SELECT", nr);
    mbse_mvprintw( 5, 4, temp);
    set_color(CYAN, BLACK);
    if (ProtRecords) {
	snprintf(temp, PATH_MAX, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	working(1, 0, 0);
	if ((fil = fopen(temp, "r")) != NULL) {
	    fread(&PROThdr, sizeof(PROThdr), 1, fil);
	    x = 2;
	    y = 7;
	    set_color(CYAN, BLACK);
	    for (i = 1; i <= 20; i++) {
		if (i == 11) {
		    x = 42;
		    y = 7;
		}
		if ((o + i) <= ProtRecords) {
		    offset = sizeof(PROThdr) + (((o + i) - 1) * PROThdr.recsize);
		    fseek(fil, offset, 0);
		    fread(&PROT, PROThdr.recsize, 1, fil);
		    if (PROT.Available)
			set_color(CYAN, BLACK);
		    else
			set_color(LIGHTBLUE, BLACK);
		    snprintf(temp, 81, "%3d.  %s %-30s", i, PROT.ProtKey, PROT.ProtName);
		    temp[37] = '\0';
		    mbse_mvprintw(i + 6, x, temp);
		    y++;
		}
	    }
	    strcpy(pick, select_pick(ProtRecords, 20));

	    if ((atoi(pick) >= 1) && (atoi(pick) <= ProtRecords)) {
		offset = sizeof(PROThdr) + ((atoi(pick) - 1) * PROThdr.recsize);
		fseek(fil, offset, 0);
		fread(&PROT, PROThdr.recsize, 1, fil);
		if (PROT.Available)
		    strcpy(Prot, PROT.ProtName);
		else
		    errmsg((char *)"This protocol is not available");
	    }
	    fclose(fil);
	}
    }
    return Prot;
}



int bbs_prot_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *no;
    int	    j;

    snprintf(temp, PATH_MAX, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 8, 5, page, (char *)"BBS Transfer protocols");
    j = 0;
    fprintf(fp, "\n\n");
    fread(&PROThdr, sizeof(PROThdr), 1, no);

    ip = open_webdoc((char *)"protocol.html", (char *)"BBS Transfer Protocols", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Zone</TH><TH align='left'>Comment</TH><TH align='left'>Available</TH><TH align='left'>Type</TH></TR>\n");

    while ((fread(&PROT, PROThdr.recsize, 1, no)) == 1) {

	if (j == 4) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	snprintf(temp, 81, "protocol_%s.html", PROT.ProtKey);
	fprintf(ip, "<TR><TD><A HREF=\"%s\">%s</A></TD><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, PROT.ProtKey, PROT.ProtName, getboolean(PROT.Available), PROT.Internal ? "Internal":"External");
	if ((wp = open_webdoc(temp, (char *)"BBS Transfer Protocol", PROT.ProtName))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"protocol.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Selection key", PROT.ProtKey);
	    if (!PROT.Internal) {
		add_webtable(wp, (char *)"Protocol name", PROT.ProtName);
		add_webtable(wp, (char *)"Upload command", PROT.ProtUp);
	    }
	    add_webtable(wp, (char *)"Download command", PROT.ProtDn);
	    add_webtable(wp, (char *)"Available", getboolean(PROT.Available));
	    add_webtable(wp, (char *)"Internal protocol", getboolean(PROT.Internal));
	    add_webtable(wp, (char *)"User advice", PROT.Advice);
	    snprintf(temp, 81, "%d%%", PROT.Efficiency);
	    add_webtable(wp, (char *)"Efficiency", temp);
	    web_secflags(wp, (char *)"Security level", PROT.Level);
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}

	fprintf(fp, "   Selection key     %s\n", PROT.ProtKey);
        fprintf(fp, "   Protocol name     %s\n", PROT.ProtName);
	if (!PROT.Internal) {
	    fprintf(fp, "   Upload command    %s\n", PROT.ProtUp);
	    fprintf(fp, "   Download command  %s\n", PROT.ProtDn);
	}
        fprintf(fp, "   Available         %s\n", getboolean(PROT.Available));
        fprintf(fp, "   Internal protocol %s\n", getboolean(PROT.Internal));
        fprintf(fp, "   User advice       %s\n", PROT.Advice);
        fprintf(fp, "   Efficiency        %d%%\n", PROT.Efficiency);
        fprintf(fp, "   Security level    %s\n", get_secstr(PROT.Level));
        fprintf(fp, "\n\n");

        j++;
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	    
    fclose(no);
    return page;
}


