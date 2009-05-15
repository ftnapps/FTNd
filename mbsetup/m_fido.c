/*****************************************************************************
 *
 * $Id: m_fido.c,v 1.25 2005/10/11 20:49:48 mbse Exp $
 * Purpose ...............: Setup Fidonet structure.
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
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_fido.h"



int	    FidoUpdated = 0;
extern int  exp_golded;


/*
 * Count nr of fidonet records in the database.
 * Creates the database if it doesn't exist.
 */
int CountFidonet(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];
    int	    count;

    snprintf(ffile, PATH_MAX, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    fidonethdr.hdrsize = sizeof(fidonethdr);
	    fidonethdr.recsize = sizeof(fidonet);
	    fwrite(&fidonethdr, sizeof(fidonethdr), 1, fil);
	    /*
	     * Fill in the defaults
	     */
	    memset(&fidonet, 0, sizeof(fidonet));
	    snprintf(fidonet.comment,  41, "Fidonet network");
	    snprintf(fidonet.domain,   13, "fidonet");
	    snprintf(fidonet.nodelist,  9, "NODELIST");
	    snprintf(fidonet.seclist[0].nodelist, 9, "REGION28");
	    fidonet.seclist[0].zone = 2;
	    fidonet.seclist[0].net = 28;
	    fidonet.zone[0] = 2;
	    fidonet.zone[1] = 1;
	    fidonet.zone[2] = 3;
	    fidonet.zone[3] = 4;
	    fidonet.zone[4] = 5;
	    fidonet.zone[5] = 6;
	    fidonet.available = TRUE;
	    fwrite(&fidonet, sizeof(fidonet), 1, fil);
	    fclose(fil);
	    exp_golded = TRUE;
	    chmod(ffile, 0640);
	    return 1;
	} else
	    return -1;
    }

    fread(&fidonethdr, sizeof(fidonethdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - fidonethdr.hdrsize) / fidonethdr.recsize;
    fclose(fil);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenFidonet(void);
int OpenFidonet(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/fidonet.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&fidonethdr, sizeof(fidonethdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = fidonethdr.recsize;
			if (oldsize != sizeof(fidonet)) {
				FidoUpdated = 1;
				Syslog('+', "Updated %s, format changed");
			} else
				FidoUpdated = 0;
			fidonethdr.hdrsize = sizeof(fidonethdr);
			fidonethdr.recsize = sizeof(fidonet);
			fwrite(&fidonethdr, sizeof(fidonethdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&fidonet, 0, sizeof(fidonet));
			while (fread(&fidonet, oldsize, 1, fin) == 1) {
				fwrite(&fidonet, sizeof(fidonet), 1, fout);
				memset(&fidonet, 0, sizeof(fidonet));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseFidonet(int);
void CloseFidonet(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX], temp[10];
	FILE	*fi, *fo;
	st_list	*fid = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/fidonet.temp", getenv("MBSE_ROOT"));

	if (FidoUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			exp_golded = TRUE;
			fi = fopen(fout, "r");
			fo = fopen(fin, "w");
			fread(&fidonethdr, fidonethdr.hdrsize, 1, fi);
			fwrite(&fidonethdr, fidonethdr.hdrsize, 1, fo);

			while (fread(&fidonet, fidonethdr.recsize, 1, fi) == 1)
				if (!fidonet.deleted) {
					snprintf(temp, 10, "%05d", fidonet.zone[0]);
					fill_stlist(&fid, temp, ftell(fi) - fidonethdr.recsize);
				}
			sort_stlist(&fid);

			for (tmp = fid; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&fidonet, fidonethdr.recsize, 1, fi);
				fwrite(&fidonet, fidonethdr.recsize, 1, fo);
			}

			fclose(fi);
			fclose(fo);
			unlink(fout);
			tidy_stlist(&fid);
			chmod(fin, 0640);

			Syslog('+', "Updated \"fidonet.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendFidonet(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/fidonet.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&fidonet, 0, sizeof(fidonet));
		fwrite(&fidonet, sizeof(fidonet), 1, fil);
		fclose(fil);
		FidoUpdated = 1;
		return 0;
	} else
		return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditFidoRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX], *temp;
    int		    offset;
    int		    i, j = 0;
    unsigned int    crc, crc1;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Fidonet");

    snprintf(mfile, PATH_MAX, "%s/etc/fidonet.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    offset = sizeof(fidonethdr) + ((Area -1) * sizeof(fidonet));
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	return -1;
    }

    fread(&fidonet, sizeof(fidonet), 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&fidonet, crc, sizeof(fidonet));

    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 6, "2.  EDIT FIDONET NETWORK");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.  Comment");
    mbse_mvprintw( 8, 6, "2.  Domain name");
    mbse_mvprintw( 9, 6, "3.  Available");
    mbse_mvprintw(10, 6, "4.  Deleted");
    mbse_mvprintw(11, 6, "5.  Main Nodelist");
    mbse_mvprintw(12, 6, "6.  Merge list #1");
    mbse_mvprintw(13, 6, "7.  Merge list #2");
    mbse_mvprintw(14, 6, "8.  Merge list #3");
    mbse_mvprintw(15, 6, "9.  Merge list #4");
    mbse_mvprintw(16, 6, "10. Merge list #5");
    mbse_mvprintw(17, 6, "11. Merge list #6");
    mbse_mvprintw(12,55, "12. Primary zone");
    mbse_mvprintw(13,55, "13. Zone number #2");
    mbse_mvprintw(14,55, "14. Zone number #3");
    mbse_mvprintw(15,55, "15. Zone number #4");
    mbse_mvprintw(16,55, "16. Zone number #5");
    mbse_mvprintw(17,55, "17. Zone number #6");
    temp = calloc(18, sizeof(char));

    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,26,40, fidonet.comment);
	show_str( 8,26,8,  fidonet.domain);
	show_bool(9,26,    fidonet.available);
	show_bool(10,26,   fidonet.deleted);
	show_str(11,26,8,  fidonet.nodelist);
	for (i = 0; i < 6; i++) {
	    if ((fidonet.seclist[i].zone) || strlen(fidonet.seclist[i].nodelist)) {
		show_str(i + 12,26,8, fidonet.seclist[i].nodelist);
		snprintf(temp, 18, "%d:%d/%d", fidonet.seclist[i].zone, fidonet.seclist[i].net, fidonet.seclist[i].node);
		show_str(i + 12, 36,17, temp);
	    } else 
		show_str(i + 12,26,27, (char *)"                           ");
	    show_int(i + 12,74, fidonet.zone[i]);
	}

	j = select_menu(17);
	switch(j) {
	    case 0: if (fidonet.available && fidonet.deleted)
			fidonet.available = FALSE;
		    if (fidonet.available && (strlen(fidonet.domain) == 0)) {
			errmsg("You must fill in a valid domain name");
			break;
		    }
		    if (fidonet.available && (fidonet.zone[0] == 0)) {
			errmsg("The network must have a main zone number");
			break;
		    }
		    if (fidonet.available && (strlen(fidonet.nodelist) == 0)) {
			errmsg("You must fill in a nodelist for this network");
			break;
		    }
		    crc1 = 0xffffffff;
		    crc1 = upd_crc32((char *)&fidonet, crc1, sizeof(fidonet));
		    if (crc != crc1) {
			if (yes_no((char *)"Record is changed, save") == 1) {
			    working(1, 0, 0);
			    if ((fil = fopen(mfile, "r+")) == NULL) {
				working(2, 0, 0);
				free(temp);
				return -1;
			    }
			    fseek(fil, offset, 0);
			    fwrite(&fidonet, sizeof(fidonet), 1, fil);
			    fclose(fil);
			    FidoUpdated = 1;
			    working(6, 0, 0);
			}
		    }
		    IsDoing("Browsing Menu");
		    free(temp);
		    return 0;
	    case 1: E_STR(7,26,40, fidonet.comment, "The ^Comment^ for this network name")
	    case 2: E_STR(8, 26,8, fidonet.domain, "The ^Name^ of the network without dots")
	    case 3: E_BOOL(9,26, fidonet.available, "Is this network ^Available^ for use")
	    case 4: E_BOOL(10,26, fidonet.deleted,   "Is this netword ^Deleted^")
	    case 5: E_STR(11,26,8, fidonet.nodelist, "The name of the ^Primary Nodelist^ for this network")
	    case 6:
	    case 7:
	    case 8:
	    case 9:
	    case 10:
	    case 11: strcpy(fidonet.seclist[j-6].nodelist, 
			    edit_str(j+6,26,8, fidonet.seclist[j-6].nodelist, 
				(char *)"The secondary ^nodelist^ or ^pointlist^ name for this domain"));
		    if (strlen(fidonet.seclist[j-6].nodelist)) {
			do {
			    snprintf(temp, 18, "%d:%d/%d", fidonet.seclist[j-6].zone, 
				    fidonet.seclist[j-6].net, fidonet.seclist[j-6].node);
			    strcpy(temp, edit_str(j+6,36,17, temp, 
					(char *)"The top ^fidonet aka^ for this nodelist (zone:net/node)"));
			    if ((strstr(temp, ":") == NULL) || (strstr(temp, "/") == NULL)) {
				working(2, 0, 0);
			    }
			} while ((strstr(temp, ":") == NULL) || (strstr(temp, "/") == NULL));
			fidonet.seclist[j-6].zone = atoi(strtok(temp, ":"));
			fidonet.seclist[j-6].net  = atoi(strtok(NULL, "/"));
			fidonet.seclist[j-6].node = atoi(strtok(NULL, ""));
		    } else {
			fidonet.seclist[j-6].zone = 0;
			fidonet.seclist[j-6].net  = 0;
			fidonet.seclist[j-6].node = 0;
		    }
		    break;
	    case 12:
	    case 13:
	    case 14:
	    case 15:
	    case 16:
	    case 17:E_IRC(j,74, fidonet.zone[j-12], 0, 32767, "A ^Zone number^ which belongs to this domain (1..32767)")
	}
    }

    return 0;
}



void EditFidonet(void)
{
	int	records, i, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountFidonet();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenFidonet() == -1) {
		working(2, 0, 0);
		return;
	}
	IsDoing("Browsing Menu");

        if (! check_free())
	    return;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "2.  FIDONET SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/fidonet.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&fidonethdr, sizeof(fidonethdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(fidonethdr) + ((i - 1) * fidonethdr.recsize);
					fseek(fil, offset, 0);
					fread(&fidonet, fidonethdr.recsize, 1, fil);
					if (fidonet.available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					if (i == 11) {
						x = 42;
						y = 7;
					}
					snprintf(temp, 81, "%3d.  z%d: %-32s", i, fidonet.zone[0], fidonet.comment);
					temp[38] = 0;
					mbse_mvprintw(y, x, temp);
					y++;
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseFidonet(FALSE);
			open_bbs();
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendFidonet() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditFidoRec(atoi(pick));
	}
}



void InitFidonetdb(void)
{
    CountFidonet();
    OpenFidonet();
    CloseFidonet(TRUE);
}



void gold_akamatch(FILE *fp)
{
    char    temp[PATH_MAX];
    FILE    *fido;
    faddr   *want, *ta;
    int     i;

    snprintf(temp, PATH_MAX, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
    if ((fido = fopen(temp, "r")) == NULL)
	return;

    fprintf(fp, "; AKA Matching\n;\n");
    want = (faddr *)malloc(sizeof(faddr));

    fread(&fidonethdr, sizeof(fidonethdr), 1, fido);
    while ((fread(&fidonet, fidonethdr.recsize, 1, fido)) == 1) {

	if (fidonet.available) {
	    for (i = 0; i < 6; i++) {
		if (fidonet.zone[i]) {
		    want->zone   = fidonet.zone[0];
		    want->net    = 0;
		    want->node   = 0;
		    want->point  = 0;
		    want->name   = NULL;
		    want->domain = NULL;
		    ta = bestaka_s(want);
		    fprintf(fp, "AKAMATCH %d:* %s\n", fidonet.zone[i], ascfnode(ta, 0xf));
		    tidy_faddr(ta);
		}
	    }
	}
    }

    free(want);
    fprintf(fp, ";\n");
    fprintf(fp, "AKAMATCHNET   YES\n");
    fprintf(fp, "AKAMATCHECHO  YES\n");
    fprintf(fp, "AKAMATCHLOCAL NO\n\n");

    fprintf(fp, "; NODELISTS\n;\n");
    fprintf(fp, "NODEPATH %s/\n", CFG.nodelists);
    fseek(fido, fidonethdr.hdrsize, SEEK_SET);
    while ((fread(&fidonet, fidonethdr.recsize, 1, fido)) == 1) {
	if (fidonet.available) {
	    fprintf(fp, "NODELIST %s.*\n", fidonet.nodelist);
	    for (i = 0; i < 6; i++)
		if (strlen(fidonet.seclist[i].nodelist) || fidonet.seclist[i].zone)
		    fprintf(fp, "NODELIST %s.*\n", fidonet.seclist[i].nodelist);
	}
    }
//  fprintf(fp, "USERLIST golded.lst\n");
    fprintf(fp, "LOOKUPNET   YES\n");
    fprintf(fp, "LOOKUPECHO  NO\n");
    fprintf(fp, "LOOKUPLOCAL NO\n\n");
    fclose(fido);
}



int fido_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *fido;
    int	    i, j;

    snprintf(temp, PATH_MAX, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
    if ((fido = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 2, 0, page, (char *)"Fidonet networks");
    j = 0;

    fprintf(fp, "\n\n");
    fread(&fidonethdr, sizeof(fidonethdr), 1, fido);

    ip = open_webdoc((char *)"fidonet.html", (char *)"Fidonet networks", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Zone</TH><TH align='left'>Comment</TH><TH align='left'>Available</TH></TR>\n");

    while ((fread(&fidonet, fidonethdr.recsize, 1, fido)) == 1) {

	if (j == 6) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	snprintf(temp, 81, "fidonet_%d.html", fidonet.zone[0]);
	fprintf(ip, " <TR><TD><A HREF=\"%s\">%d</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, fidonet.zone[0], fidonet.comment, getboolean(fidonet.available));

	if ((wp = open_webdoc(temp, (char *)"Fidonet network", fidonet.comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"fidonet.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='400' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='50%%'><COL width='50%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Comment", fidonet.comment);
	    add_webtable(wp, (char *)"Domain", fidonet.domain);
	    add_webtable(wp, (char *)"Available", getboolean(fidonet.available));
	    add_webtable(wp, (char *)"Nodelist", fidonet.nodelist);
	    for (i = 0; i < 6; i++)
		if (strlen(fidonet.seclist[i].nodelist) || fidonet.seclist[i].zone) {
		    snprintf(temp, 81, "%d %-8s %d:%d/%d", i+1, fidonet.seclist[i].nodelist, fidonet.seclist[i].zone,
			fidonet.seclist[i].net, fidonet.seclist[i].node);
		    add_webtable(wp, (char *)"Merge list", temp);
		}
	    snprintf(temp, 81, "%d", fidonet.zone[0]);
	    for (i = 1; i < 6; i++)
		if (fidonet.zone[i])
		    snprintf(temp, 81, "%s %d", temp, fidonet.zone[i]);
	    add_webtable(wp, (char *)"Zone(s)", temp);
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}

	fprintf(fp, "     Comment      %s\n", fidonet.comment);
	fprintf(fp, "     Domain       %s\n", fidonet.domain);
	fprintf(fp, "     Available    %s\n", getboolean(fidonet.available));
	fprintf(fp, "     Nodelist     %s\n", fidonet.nodelist);
	for (i = 0; i < 6; i++)
	    if (strlen(fidonet.seclist[i].nodelist) || fidonet.seclist[i].zone) {
		fprintf(fp, "     Merge list %d %-8s %d:%d/%d\n", i+1, 
						fidonet.seclist[i].nodelist, fidonet.seclist[i].zone,
						fidonet.seclist[i].net, fidonet.seclist[i].node);
	    }
	fprintf(fp, "     Zone(s)      ");
	for (i = 0; i < 6; i++)
	    if (fidonet.zone[i])
		fprintf(fp, "%d ", fidonet.zone[i]);
	fprintf(fp, "\n\n\n");
	j++;
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	
    fclose(fido);
    return page;
}



