/*****************************************************************************
 *
 * $Id: m_ngroup.c,v 1.19 2005/10/11 20:49:49 mbse Exp $
 * Purpose ...............: Setup NGroups.
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
#include "m_ngroup.h"



int	NGrpUpdated = 0;


/*
 * Count nr of ngroup records in the database.
 * Creates the database if it doesn't exist.
 */
int CountNGroup(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			ngrouphdr.hdrsize = sizeof(ngrouphdr);
			ngrouphdr.recsize = sizeof(ngroup);
			fwrite(&ngrouphdr, sizeof(ngrouphdr), 1, fil);
			memset(&ngroup, 0, sizeof(ngroup));
			snprintf(ngroup.Name, 13, "DONT");
			snprintf(ngroup.Comment, 56, "Do NOT announce");
			ngroup.Active = TRUE;
			fwrite(&ngroup, sizeof(ngroup), 1, fil);
			memset(&ngroup, 0, sizeof(ngroup));
			snprintf(ngroup.Name, 13, "LOCAL");
			snprintf(ngroup.Comment, 56, "Local file areas");
			ngroup.Active = TRUE;
			fwrite(&ngroup, sizeof(ngroup), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 2;
		} else
			return -1;
	}

	fread(&ngrouphdr, sizeof(ngrouphdr), 1, fil);
	fseek(fil, 0, SEEK_SET);
	fread(&ngrouphdr, ngrouphdr.hdrsize, 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - ngrouphdr.hdrsize) / ngrouphdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenNGroup(void);
int OpenNGroup(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/ngroups.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			NGrpUpdated = 0;
			fread(&ngrouphdr, sizeof(ngrouphdr), 1, fin);
			fseek(fin, 0, SEEK_SET);
			fread(&ngrouphdr, ngrouphdr.hdrsize, 1, fin);
			if (ngrouphdr.hdrsize != sizeof(ngrouphdr)) {
				ngrouphdr.hdrsize = sizeof(ngrouphdr);
				NGrpUpdated = 1;
			}

			/*
			 * In case we are automaitc upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = ngrouphdr.recsize;
			if (oldsize != sizeof(ngroup)) {
				NGrpUpdated = 1;
				Syslog('+', "Upgraded %s, format changed", fnin);
			}
			ngrouphdr.hdrsize = sizeof(ngrouphdr);
			ngrouphdr.recsize = sizeof(ngroup);
			fwrite(&ngrouphdr, sizeof(ngrouphdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&ngroup, 0, sizeof(ngroup));
			while (fread(&ngroup, oldsize, 1, fin) == 1) {
				fwrite(&ngroup, sizeof(ngroup), 1, fout);
				memset(&ngroup, 0, sizeof(ngroup));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseNGroup(int);
void CloseNGroup(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*mgr = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/ngroups.temp", getenv("MBSE_ROOT"));

	if (NGrpUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&ngrouphdr, ngrouphdr.hdrsize, 1, fi);
			fwrite(&ngrouphdr, ngrouphdr.hdrsize, 1, fo);

			while (fread(&ngroup, ngrouphdr.recsize, 1, fi) == 1)
				if (!ngroup.Deleted)
					fill_stlist(&mgr, ngroup.Name, ftell(fi) - ngrouphdr.recsize);
			sort_stlist(&mgr);

			for (tmp = mgr; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&ngroup, ngrouphdr.recsize, 1, fi);
				fwrite(&ngroup, ngrouphdr.recsize, 1, fo);
			}

			tidy_stlist(&mgr);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"ngroups.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendNGroup(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/ngroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&ngroup, 0, sizeof(ngroup));
		fwrite(&ngroup, sizeof(ngroup), 1, fil);
		fclose(fil);
		NGrpUpdated = 1;
		return 0;
	} else
		return -1;
}



void NgScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "9.1 EDIT NEWFILES GROUP");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 6, "1.  Name");
	mbse_mvprintw( 8, 6, "2.  Comment");
	mbse_mvprintw( 9, 6, "3.  Active");
	mbse_mvprintw(10, 6, "4.  Deleted");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditNGrpRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	int	offset;
	int	j;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit NewfileGroup");

	snprintf(mfile, PATH_MAX, "%s/etc/ngroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(ngrouphdr) + ((Area -1) * sizeof(ngroup));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&ngroup, sizeof(ngroup), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&ngroup, crc, sizeof(ngroup));
	NgScreen();
	
	for (;;) {
		set_color(WHITE, BLACK);
		show_str(  7,18,12, ngroup.Name);
		show_str(  8,18,55, ngroup.Comment);
		show_bool( 9,18,    ngroup.Active);
		show_bool(10,18,    ngroup.Deleted);

		j = select_menu(4);
		switch(j) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&ngroup, crc1, sizeof(ngroup));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&ngroup, sizeof(ngroup), 1, fil);
					fclose(fil);
					NGrpUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1: E_UPS( 7,18,12,ngroup.Name,"The ^name^ for this message group")
		case 2: E_STR( 8,18,55,ngroup.Comment,"The ^desription^ for this message group")
		case 3: E_BOOL(9,18,   ngroup.Active, "Is this message group ^active^")
		case 4: E_BOOL(10,18,  ngroup.Deleted, "Is this group ^Deleted^")
		}
	}

	return 0;
}



void EditNGroup(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountNGroup();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenNGroup() == -1) {
		working(2, 0, 0);
		return;
	}
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "11. NEWFILES GROUPS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/ngroups.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&ngrouphdr, sizeof(ngrouphdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11 ) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(ngrouphdr) + (((o + i) - 1) * ngrouphdr.recsize);
						fseek(fil, offset, 0);
						fread(&ngroup, ngrouphdr.recsize, 1, fil);
						if (ngroup.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						snprintf(temp, 81, "%3d.  %-12s %-18s", o + i, ngroup.Name, ngroup.Comment);
						temp[38] = '\0';
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseNGroup(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendNGroup() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditNGrpRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitNGroup(void)
{
    CountNGroup();
    OpenNGroup();
    CloseNGroup(TRUE);
}



char *PickNGroup(char *shdr)
{
	static	char MGrp[21] = "";
	int	records, i, o = 0, y, x;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return MGrp;
	}

	records = CountNGroup();
	if (records == -1) {
		working(2, 0, 0);
		return MGrp;
	}


	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.  NEWFILES GROUP SELECT", shdr);
		mbse_mvprintw( 5, 4, temp);
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&ngrouphdr, sizeof(ngrouphdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(ngrouphdr) + (((o + i) - 1) * ngrouphdr.recsize);
						fseek(fil, offset, 0);
						fread(&ngroup, ngrouphdr.recsize, 1, fil);
						if (ngroup.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						snprintf(temp, 81, "%3d.  %-12s %-18s", o + i, ngroup.Name, ngroup.Comment);
						temp[38] = '\0';
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_pick(records, 20));

		if (strncmp(pick, "-", 1) == 0) 
			return MGrp;

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			snprintf(temp, PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
			fil = fopen(temp, "r");
			offset = sizeof(ngrouphdr) + ((atoi(pick) - 1) * ngrouphdr.recsize);
			fseek(fil, offset, 0);
			fread(&ngroup, ngrouphdr.recsize, 1, fil);
			fclose(fil);
			strcpy(MGrp, ngroup.Name);
			return MGrp;
		}
	}
}



int newf_group_doc(FILE *fp, FILE *toc, int page)
{
    char    *temp, group[13];
    FILE    *ip, *wp, *no;
    int	    i, groups, refs, nr;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL) {
	free(temp);
	return page;
    }

    wp = open_webdoc((char *)"newgroup.html", (char *)"Newfiles groups", NULL);
    fprintf(wp, "<A HREF=\"index.html\">Main</A>\n");
    
    page = newpage(fp, page);
    addtoc(fp, toc, 11, 0, page, (char *)"Newfiles announce groups");
    fprintf(fp, "\n");
    fprintf(fp, "   Name         Act Comment\n");
    fprintf(fp, "   ------------ --- --------------------------------------------------\n");

    fread(&ngrouphdr, sizeof(ngrouphdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&ngrouphdr, ngrouphdr.hdrsize, 1, no);

    fprintf(wp, "<P>\n");
    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<COL width='20%%'><COL width='5%%'><COL width='75%%'>\n");
    fprintf(wp, "<TBODY>\n");
    fprintf(wp, "<TR><TH align='left'>Name</TH><TH align='left'>Act</TH><TH align='left'>Comment</TH></TR>\n");

    while ((fread(&ngroup, ngrouphdr.recsize, 1, no)) == 1) {
	fprintf(fp, "   %-12s %s %s\n", ngroup.Name, getboolean(ngroup.Active), ngroup.Comment);
	fprintf(wp, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n", ngroup.Name, getboolean(ngroup.Active), ngroup.Comment);
    }
    
    fprintf(wp, "</TBODY>\n");
    fprintf(wp, "</TABLE>\n");

    fseek(no, ngrouphdr.hdrsize, SEEK_SET);
    while ((fread(&ngroup, ngrouphdr.recsize, 1, no)) == 1) {
	refs = 0;
	snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((ip = fopen(temp, "r"))) {
	    fread(&areahdr, sizeof(areahdr), 1, ip);
	    nr = 0;
	    while ((fread(&area, areahdr.recsize, 1, ip)) == 1) {
		nr++;
		if (area.Available && (strcmp(ngroup.Name, area.NewGroup) == 0)) {
		    if (refs == 0) {
			fprintf(wp, "<HR>\n");
			fprintf(wp, "<H3>References for group %s</H3>\n", ngroup.Name);
			fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
			fprintf(wp, "<TBODY>\n");
		    }
		    refs++;
		    fprintf(wp, "<TR><TD><A HREF=\"filearea_%d.html\">File area %d</A></TD><TD>%s</TD></TR>\n", nr, nr, area.Name);
		}
	    }
	    fclose(ip);
	}
	snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((ip = fopen(temp, "r"))) {
	    fread(&fgrouphdr, fgrouphdr.hdrsize, 1, ip);
	    while ((fread(&fgroup, fgrouphdr.recsize, 1, ip)) == 1) {
		if (strcmp(ngroup.Name, fgroup.AnnGroup) == 0) {
		    if (refs == 0) {
			fprintf(wp, "<HR>\n");
			fprintf(wp, "<H3>References for group %s</H3>\n", ngroup.Name);
			fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
			fprintf(wp, "<TBODY>\n");
		    }
		    refs++;
		    fprintf(wp, "<TR><TD><A HREF=\"filegroup_%s.html\">File group %s</A></TD><TD>%s</TD></TR>\n", 
			    fgroup.Name, fgroup.Name, fgroup.Comment);
		}
	    }
	    fclose(ip);
	}
	snprintf(temp, PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
	if ((ip = fopen(temp, "r"))) {
	    fread(&newfileshdr, sizeof(newfileshdr), 1, ip);
	    nr = 0;
	    while ((fread(&newfiles, newfileshdr.recsize, 1, ip)) == 1) {
		nr++;
		groups = newfileshdr.grpsize / sizeof(group);
		for (i = 0; i < groups; i++) {
		    fread(&group, sizeof(group), 1, ip);
		    if (newfiles.Active && strlen(group) && (strcmp(ngroup.Name, group) == 0)) {
			if (refs == 0) {
			    fprintf(wp, "<HR>\n");
			    fprintf(wp, "<H3>References for group %s</H3>\n", ngroup.Name);
			    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			    fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
			    fprintf(wp, "<TBODY>\n");
			}
			refs++;
			fprintf(wp, "<TR><TD><A HREF=\"newfiles_%d.html\">Report %d</A></TD><TD>%s</TD></TR>\n",
			    nr, nr, newfiles.Comment);
		    }
		}
	    }
	    fclose(ip);
	}
	if (refs) {
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	}
    }

    close_webdoc(wp);
	
    free(temp);
    fclose(no);
    return page;
}


