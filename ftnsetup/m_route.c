/*****************************************************************************
 *
 * $Id: m_route.c,v 1.13 2008/02/28 22:05:14 mbse Exp $
 * Purpose ...............: Routing Setup
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
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "grlist.h"
#include "m_node.h"
#include "m_route.h"


int	RouteUpdated = 0;


/*
 * Count nr of route records in the database.
 * Creates the database if it doesn't exist.
 */
int CountRoute(void)
{
    FILE    *fil;
    char    *ffile;
    int	    count;

    ffile = calloc(PATH_MAX, sizeof(char));
    snprintf(ffile, PATH_MAX, "%s/etc/route.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    routehdr.hdrsize = sizeof(routehdr);
	    routehdr.recsize = sizeof(route);
	    fwrite(&routehdr, sizeof(routehdr), 1, fil);
	    fclose(fil);
	    chmod(ffile, 0640);
	    free(ffile);
	    return 0;
	} else {
	    free(ffile);
	    return -1;
	}
    }

    fread(&routehdr, sizeof(routehdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - routehdr.hdrsize) / routehdr.recsize;
    fclose(fil);
    free(ffile);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenRoute(void)
{
    FILE    *fin, *fout;
    char    *fnin, *fnout;
    int	    oldsize;

    fnin  = calloc(PATH_MAX, sizeof(char));
    fnout = calloc(PATH_MAX, sizeof(char));
    snprintf(fnin,  PATH_MAX, "%s/etc/route.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/route.temp", getenv("MBSE_ROOT"));

    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&routehdr, sizeof(routehdr), 1, fin);
	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize    = routehdr.recsize;
	    if (oldsize != sizeof(route)) {
		Syslog('+', "Updated %s, format changed", fnin);
		RouteUpdated = 1;
	    } else
		RouteUpdated = 0;

	    routehdr.hdrsize = sizeof(routehdr);
	    routehdr.recsize = sizeof(route);
	    fwrite(&routehdr, sizeof(routehdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&route, 0, sizeof(route));
	    while (fread(&route, oldsize, 1, fin) == 1) {
		fwrite(&route, sizeof(route), 1, fout);
		memset(&route, 0, sizeof(route));
	    }
	    fclose(fin);
	    fclose(fout);
	    free(fnin);
	    free(fnout);
	    return 0;
	} else
	    return -1;
    }
    return -1;
}



/*
 * Update route database and sort it. Sorting is important
 * because the routelist is processed top-down, the first
 * match counts.
 */
void CloseRoute(int force)
{
    char    *fin, *fout;
    FILE    *fi, *fo;
    st_list *new = NULL, *tmp;

    fin  = calloc(PATH_MAX, sizeof(char));
    fout = calloc(PATH_MAX, sizeof(char));
    snprintf(fin,  PATH_MAX, "%s/etc/route.data", getenv("MBSE_ROOT"));
    snprintf(fout, PATH_MAX, "%s/etc/route.temp", getenv("MBSE_ROOT"));

    if (RouteUpdated == 1) {
	if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
	    working(1, 0, 0);
	    fi = fopen(fout, "r");
	    fo = fopen(fin,  "w");
	    fread(&routehdr, routehdr.hdrsize, 1, fi);
	    fwrite(&routehdr, routehdr.hdrsize, 1, fo);

	    while (fread(&route, routehdr.recsize, 1, fi) == 1) {
		if (!route.Deleted) {
		    fill_stlist(&new, route.mask, ftell(fi) - routehdr.recsize);
		}
	    }
	    sort_stlist(&new);

	    for (tmp = new; tmp; tmp = tmp->next) {
		fseek(fi, tmp->pos, SEEK_SET);
		fread(&route, routehdr.recsize, 1, fi);
		fwrite(&route, routehdr.recsize, 1, fo);
	    }
	    fclose(fi);
	    fclose(fo);
	    tidy_stlist(&new);
	    unlink(fout);
	    chmod(fin, 0640);
	    Syslog('+', "Updated \"route.data\"");
	    free(fin);
	    free(fout);
	    if (!force)
		working(6, 0, 0);
	    return;
	}
    }
    chmod(fin, 0640);
    unlink(fout); 
    free(fin);
    free(fout);
}



int AppendRoute(void)
{
    FILE    *fil;
    char    *ffile;

    ffile = calloc(PATH_MAX, sizeof(char));
    snprintf(ffile, PATH_MAX, "%s/etc/route.temp", getenv("MBSE_ROOT"));

    if ((fil = fopen(ffile, "a")) != NULL) {
	memset(&route, 0, sizeof(route));
	/*
	 * Fill in default values
	 */
	route.routetype = R_NOROUTE;
	fwrite(&route, sizeof(route), 1, fil);
	fclose(fil);
	RouteUpdated = 1;
	free(ffile);
	return 0;
    }

    free(ffile);
    return -1;
}


#define	RCHARS "All:/.0123456789"


/*
 * Check syntax of entered routing mask.
 */
int RouteSyntax(char *);
int RouteSyntax(char *s)
{
    char    *str, *p;

    if (strspn(s, RCHARS) != strlen(s))
	return FALSE;
    
    str = xstrcpy(s);

    if ((p = strchr(str, ':'))) {
	*(p++) = '\0';
	if (strspn(str, "0123456789") != strlen(str)) {
	    if (strcmp(str, "All"))
		return FALSE;
	} else if (strlen(p) == 0)
	    return FALSE;
	str = p;
    } else {
	if (strcmp(str, "All"))
	    return FALSE;
    }

    if ((p = strchr(str, '/'))) {
	*(p++) = '\0';
	if (strspn(str, "0123456789") != strlen(str)) {
	    if (strcmp(str, "All"))
		return FALSE;
	} else if (strlen(p) == 0)
	    return FALSE;
	str = p;
    } else {
	if (strcmp(str, "All"))
	    return FALSE;
    }

    if ((p = strchr(str, '.'))) {
	*(p++) = '\0';
	if (strspn(str, "0123456789") != strlen(str)) {
	    if (strcmp(str, "All"))
		return FALSE;
	} if (strlen(p) == 0)
	    return FALSE;
	str = p;
    } else {
	if (strspn(str, "0123456789") != strlen(str)) {
	    if (strcmp(str, "All"))
		return FALSE;
	}
	str = NULL;
    }

    return TRUE;
}



void RouteScreen(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 5, "19. EDIT ROUTING TABLE");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 5, "1.  Mask");
//  mbse_mvprintw( 8, 5, "2.  Src name");
    mbse_mvprintw( 9, 5, "3.  Action");
    mbse_mvprintw(10, 5, "4.  Dest addr");
//  mbse_mvprintw(11, 5, "5.  Dest name");
    mbse_mvprintw(12, 5, "6.  Active");
    mbse_mvprintw(13, 5, "7.  Deleted");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditRouteRec(int Area)
{
    FILE	    *fil;
    char	    *mfile, newmask[25];
    int		    offset;
    unsigned int    crc, crc1;
    int		    i;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Route");

    mfile = calloc(PATH_MAX, sizeof(char));
    snprintf(mfile, PATH_MAX, "%s/etc/route.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	free(mfile);
	return -1;
    }

    fread(&routehdr, sizeof(routehdr), 1, fil);
    offset = routehdr.hdrsize + ((Area -1) * routehdr.recsize);
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	free(mfile);
	return -1;
    }

    fread(&route, routehdr.recsize, 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&route, crc, routehdr.recsize);
    RouteScreen();

    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  7,20,24, route.mask);
	show_str(  8,20,36, route.sname);
	show_routetype(9,20, route.routetype);
	show_str( 10,20,35, aka2str(route.dest));
	show_str( 11,20,35, route.dname);
	show_bool(12,20,    route.Active);
	show_bool(13,20,    route.Deleted);

	switch(select_menu(7)) {
	    case 0: crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&route, crc1, routehdr.recsize);
		    if (crc != crc1) {
			if (yes_no((char *)"Record is changed, save") == 1) {
			    working(1, 0, 0);
			    if ((fil = fopen(mfile, "r+")) == NULL) {
				working(2, 0, 0);
				free(mfile);
				return -1;
			    }
			    fseek(fil, offset, 0);
			    fwrite(&route, routehdr.recsize, 1, fil);
			    fclose(fil);
			    RouteUpdated = 1;
			    working(6, 0, 0);
			}
		    }
		    free(mfile);
		    IsDoing("Browsing Menu");
		    return 0;
	    case 1: strcpy(newmask, edit_str( 7,20,24, route.mask, 
				(char *)"The ^routing mask^ to match, ie: ^1:All^, ^2:280/All^, ^2:280/28^"));
		    /*
		     * First make all chars in the right case, the only allowed word is "All".
		     */
		    for (i = 0; i < strlen(newmask); i++) {
			if (newmask[i] == 'a')
			    newmask[i] = 'A';
			if (newmask[i] == 'L')
			    newmask[i] = 'l';
		    }
		    if (RouteSyntax(newmask)) {
			if (strlen(newmask) && !strlen(route.mask))
			    route.Active = TRUE;
			strcpy(route.mask, newmask);
		    } else
			errmsg("Syntax error");
		    break;
//	    case 2: E_STR(  8,20,36, route.sname,     "The additional ^source username^ to add to the match");
	    case 3: route.routetype = edit_routetype(9,20, route.routetype);
		    break;
	    case 4: route.dest = PullUplink((char *)"19.4");
		    RouteScreen();
		    break;
//	    case 5: E_STR( 11,20,36, route.dname,     "The ^destination name^ if needed");
	    case 6: E_BOOL(12,20,    route.Active,    "If this route is ^active^");
	    case 7: E_BOOL(13,20,    route.Deleted,   "If this route must be ^deleted^");
	}
    }
}



void EditRoute(void)
{
    int	    records, i, o, x, y;
    char    pick[12], *temp;
    FILE    *fil;
    int	    offset;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    records = CountRoute();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenRoute() == -1) {
	working(2, 0, 0);
	return;
    }
    o = 0;

    temp = calloc(PATH_MAX, sizeof(char));

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 4, "19. ROUTING TABLE");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/route.temp", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&routehdr, sizeof(routehdr), 1, fil);
		x = 2;
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 10; i++) {
		    if ((o + i) <= records) {
			offset = sizeof(routehdr) + (((o + i) - 1) * routehdr.recsize);
			fseek(fil, offset, 0);
			fread(&route, routehdr.recsize, 1, fil);
			if (route.Deleted)
			    set_color(LIGHTRED, BLACK);
			else if (route.Active)
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			snprintf(temp, 81, "%3d.  %-25s %s %s", o + i, route.mask, get_routetype(route.routetype), aka2str(route.dest));
//			temp[37] = 0;
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_record(records, 10));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseRoute(FALSE);
	    free(temp);
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    working(1, 0, 0);
	    if (AppendRoute() == 0) {
		records++;
		working(1, 0, 0);
	    } else
		working(2, 0, 0);
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 10) < records) 
		o = o + 10;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 10) >= 0)
		o = o - 10;

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    EditRouteRec(atoi(pick));
	    o = ((atoi(pick) - 1) / 10) * 10;
	}
    }
}



void InitRoute(void)
{
    CountRoute();
    OpenRoute();
    CloseRoute(TRUE);
}



int route_doc(FILE *fp, FILE *toc, int page)
{
    char    *temp;
    FILE    *wp, *no;
    int	    j;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/route.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL) {
	free(temp);
	return page;
    }

    page = newpage(fp, page);
    addtoc(fp, toc, 19, 0, page, (char *)"Routing table");
    j = 1;

    fprintf(fp, "\n\n");
    fread(&routehdr, sizeof(routehdr), 1, no);

    wp = open_webdoc((char *)"route.html", (char *)"Netmail Routing", NULL);
    fprintf(wp, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(wp, "<UL>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<TBODY>\n");
    fprintf(wp, "<TR><TH align='left'>Route mask</TH><TH align='left'>Src username</TH><TH align='left'>Action</TH>");
    fprintf(wp, "<TH align='left'>Destination</TH><TH align='left'>Dest username</TH><TH align='left'>Active</TH></TR>\n");
					    
    while ((fread(&route, routehdr.recsize, 1, no)) == 1) {

	if (j == 7) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}
	
	fprintf(wp, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n",
		route.mask, route.sname, get_routetype(route.routetype), 
		aka2str(route.dest), route.dname, getboolean(route.Active));
	fprintf(fp, "     Route mask        %s\n", route.mask);
	fprintf(fp, "     Src username      %s\n", route.sname);
	fprintf(fp, "     Route action      %s\n", get_routetype(route.routetype));
	fprintf(fp, "     Destination       %s\n", aka2str(route.dest));
	fprintf(fp, "     Dest username     %s\n", route.dname);
	fprintf(fp, "     Active            %s\n", getboolean(route.Active));
	fprintf(fp, "\n\n");
	j++;
    }
    
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");
    close_webdoc(wp);
    fclose(no);
    free(temp);
    return page;
}


