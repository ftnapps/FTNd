/*****************************************************************************
 *
 * $Id: m_ibc.c,v 1.9 2008/02/28 22:05:14 mbse Exp $
 * Purpose ...............: Setup Internet BBS Chat
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
#include "m_ibc.h"



int	IBCUpdated = 0;


/*
 * Count nr of IBC records in the database.
 * Creates the database if it doesn't exist.
 */
int CountIBC(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];
    int	    count;

    snprintf(ffile, PATH_MAX, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    ibcsrvhdr.hdrsize = sizeof(ibcsrvhdr);
	    ibcsrvhdr.recsize = sizeof(ibcsrv);
	    fwrite(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fil);

	    return 0;
	} else
	    return -1;
    }

    fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - ibcsrvhdr.hdrsize) / ibcsrvhdr.recsize;
    fclose(fil);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenIBC(void);
int OpenIBC(void)
{
    FILE    *fin, *fout;
    char    fnin[PATH_MAX], fnout[PATH_MAX];
    int	    oldsize;

    snprintf(fnin,  PATH_MAX, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/ibcsrv.temp", getenv("MBSE_ROOT"));
    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fin);
	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize = ibcsrvhdr.recsize;
	    if (oldsize != sizeof(ibcsrv)) {
		IBCUpdated = 1;
		Syslog('+', "Upgraded %s, format changed", fnin);
	    } else
		IBCUpdated = 0;
	    ibcsrvhdr.hdrsize = sizeof(ibcsrvhdr);
	    ibcsrvhdr.recsize = sizeof(ibcsrv);
	    fwrite(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&ibcsrv, 0, sizeof(ibcsrv));
	    while (fread(&ibcsrv, oldsize, 1, fin) == 1) {
		fwrite(&ibcsrv, sizeof(ibcsrv), 1, fout);
		memset(&ibcsrv, 0, sizeof(ibcsrv));
	    }

	    fclose(fin);
	    fclose(fout);
	    return 0;
	} else
	    return -1;
    }
    return -1;
}



void CloseIBC(int);
void CloseIBC(int force)
{
    char	fin[PATH_MAX], fout[PATH_MAX];
    FILE	*fi, *fo;
    st_list	*vir = NULL, *tmp;

    snprintf(fin,  PATH_MAX, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    snprintf(fout, PATH_MAX, "%s/etc/ibcsrv.temp", getenv("MBSE_ROOT"));

    if (IBCUpdated == 1) {
	if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
	    working(1, 0, 0);
	    fi = fopen(fout, "r");
	    fo = fopen(fin,  "w");
	    fread(&ibcsrvhdr, ibcsrvhdr.hdrsize, 1, fi);
	    fwrite(&ibcsrvhdr, ibcsrvhdr.hdrsize, 1, fo);

	    while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fi) == 1)
		if (!ibcsrv.Deleted)
		    fill_stlist(&vir, ibcsrv.comment, ftell(fi) - ibcsrvhdr.recsize);
	    sort_stlist(&vir);

	    for (tmp = vir; tmp; tmp = tmp->next) {
		fseek(fi, tmp->pos, SEEK_SET);
		fread(&ibcsrv, ibcsrvhdr.recsize, 1, fi);
		fwrite(&ibcsrv, ibcsrvhdr.recsize, 1, fo);
	    }

	    tidy_stlist(&vir);
	    fclose(fi);
	    fclose(fo);
	    unlink(fout);
	    chmod(fin, 0640);
	    Syslog('+', "Updated \"ibcsrv.data\"");
	    if (!force)
		working(6, 0, 0);
	    return;
	}
    }
    chmod(fin, 0640);
    unlink(fout); 
}



int AppendIBC(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];

    snprintf(ffile, PATH_MAX, "%s/etc/ibcsrv.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "a")) != NULL) {
	memset(&ibcsrv, 0, sizeof(ibcsrv));
	strcpy(ibcsrv.myname, CFG.myfqdn);
	fwrite(&ibcsrv, sizeof(ibcsrv), 1, fil);
	fclose(fil);
	IBCUpdated = 1;
	return 0;
    } else
	return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditIBCRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX];
    int		    offset;
    int		    j;
    unsigned int    crc, crc1;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit ibcsrv");

    snprintf(mfile, PATH_MAX, "%s/etc/ibcsrv.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    offset = sizeof(ibcsrvhdr) + ((Area -1) * sizeof(ibcsrv));
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	return -1;
    }

    fread(&ibcsrv, sizeof(ibcsrv), 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&ibcsrv, crc, sizeof(ibcsrv));

    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "20. EDIT INTERNET BBS CHAT SERVER");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.  Comment");
    mbse_mvprintw( 8, 2, "2.  Server");
    mbse_mvprintw( 9, 2, "3.  Dyn. DNS");
    mbse_mvprintw(10, 2, "4.  Myname");
    mbse_mvprintw(11, 2, "5.  Password");
    mbse_mvprintw(12, 2, "6.  Active");
    mbse_mvprintw(13, 2, "7.  Deleted");
    mbse_mvprintw(14, 2, "8.  Compress");

    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  7,16,40, ibcsrv.comment);
	show_str(  8,16,63, ibcsrv.server);
	show_bool( 9,16,    ibcsrv.Dyndns);
	show_str( 10,16,63, ibcsrv.myname);
	show_str( 11,16,15, ibcsrv.passwd);
	show_bool(12,16,    ibcsrv.Active);
	show_bool(13,16,    ibcsrv.Deleted);
	show_bool(14,16,    ibcsrv.Compress);

	j = select_menu(8);
	switch(j) {
	case 0:	crc1 = 0xffffffff;
		crc1 = upd_crc32((char *)&ibcsrv, crc1, sizeof(ibcsrv));
		if (crc != crc1) {
		    if (yes_no((char *)"Record is changed, save") == 1) {
			working(1, 0, 0);
			if ((fil = fopen(mfile, "r+")) == NULL) {
			    working(2, 0, 0);
			    return -1;
			}
			fseek(fil, offset, 0);
			fwrite(&ibcsrv, sizeof(ibcsrv), 1, fil);
			fclose(fil);
			IBCUpdated = 1;
			working(6, 0, 0);
		    }
		}
		IsDoing("Browsing Menu");
		return 0;
	case 1:	E_STR(  7,16,40,ibcsrv.comment,  "The ^Comment^ for this record")
	case 2:	E_STR(  8,16,63,ibcsrv.server,   "The known internet ^name^ or ^IP^ address of the remote server")
	case 3: E_BOOL( 9,16,   ibcsrv.Dyndns,   "Set to Yes if the remote server uses a ^dynamic dns^ service")
	case 4: E_STR( 10,16,63,ibcsrv.myname,   "The known internet ^name^ or ^IP^ address of this server")
	case 5:	E_STR( 11,16,64,ibcsrv.passwd,   "The ^password^ for this server")
	case 6:	E_BOOL(12,16,   ibcsrv.Active,   "Switch if this server is ^Active^ for chat")
	case 7:	E_BOOL(13,16,   ibcsrv.Deleted,  "Is this server to be ^Deleted^")
	case 8:	E_BOOL(14,16,   ibcsrv.Compress, "Use ^zlib compression^ with this server")
	}
    }

    return 0;
}



void EditIBC(void)
{
    int	    records, i, x, y;
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

    records = CountIBC();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenIBC() == -1) {
	working(2, 0, 0);
	return;
    }

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 4, "20. INTERNET BBS CHAT SETUP");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/ibcsrv.temp", getenv("MBSE_ROOT"));
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fil);
		x = 2;
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= records; i++) {
		    offset = sizeof(ibcsrvhdr) + ((i - 1) * ibcsrvhdr.recsize);
		    fseek(fil, offset, 0);
		    fread(&ibcsrv, ibcsrvhdr.recsize, 1, fil);
		    if (i == 11) {
			x = 42;
			y = 7;
		    }
		    if (ibcsrv.Deleted)
			set_color(LIGHTRED, BLACK);
		    else if (ibcsrv.Active)
			set_color(CYAN, BLACK);
		    else
			set_color(LIGHTBLUE, BLACK);
		    snprintf(temp, 81, "%3d.  %s (%s)", i, ibcsrv.server, ibcsrv.comment);
		    temp[37] = 0;
		    mbse_mvprintw(y, x, temp);
		    y++;
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_record(records, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseIBC(FALSE);
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    working(1, 0, 0);
	    if (AppendIBC() == 0) {
		records++;
		working(3, 0, 0);
	    } else
		working(2, 0, 0);
	}

	if ((atoi(pick) >= 1) && (atoi(pick) <= records))
	    EditIBCRec(atoi(pick));
    }
}



void InitIBC(void)
{
    CountIBC();
    OpenIBC();
    CloseIBC(TRUE);
}



int ibc_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *vir;
    int	    nr = 0, j;

    snprintf(temp, PATH_MAX, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    if ((vir = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 20, 0, page, (char *)"Internet BBS Chatservers");
    j = 0;
    fprintf(fp, "\n\n");
    fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, vir);

    ip = open_webdoc((char *)"ibcsrv.html", (char *)"Internet BBS Chatservers", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<UL>\n");
		    
    while ((fread(&ibcsrv, ibcsrvhdr.recsize, 1, vir)) == 1) {

	if (j == 5) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	nr++;
	snprintf(temp, 81, "ibcsrv_%d.html", nr);
	fprintf(ip, "<LI><A HREF=\"%s\">%s</A></LI>\n", temp, ibcsrv.comment);
	if ((wp = open_webdoc(temp, (char *)"Internet BBS Chatserver", ibcsrv.comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"ibcsrv.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Server comment", ibcsrv.comment);
	    add_webtable(wp, (char *)"Server address", ibcsrv.server);
	    add_webtable(wp, (char *)"Uses dynamic dns", getboolean(ibcsrv.Dyndns));
	    add_webtable(wp, (char *)"My address", ibcsrv.myname);
	    add_webtable(wp, (char *)"Active", getboolean(ibcsrv.Active));
	    add_webtable(wp, (char *)"Compresion", getboolean(ibcsrv.Compress));
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}

	fprintf(fp, "      Comment     %s\n", ibcsrv.comment);
	fprintf(fp, "      Server      %s\n", ibcsrv.server);
	fprintf(fp, "      Dynamic dns %s\n", getboolean(ibcsrv.Dyndns));
	fprintf(fp, "      My name     %s\n", ibcsrv.myname);
	fprintf(fp, "      Active      %s\n", getboolean(ibcsrv.Active));
	fprintf(fp, "      Compression %s\n", getboolean(ibcsrv.Compress)); 
	fprintf(fp, "\n\n\n");
	j++;
    }

    fprintf(ip, "</UL>\n");
    close_webdoc(ip);
	    
    fclose(vir);
    return page;
}


