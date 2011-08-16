/*****************************************************************************
 *
 * $Id: m_new.c,v 1.24 2007/03/02 13:23:36 mbse Exp $
 * Purpose ...............: Newfiles Setup
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "m_new.h"
#include "m_lang.h"
#include "m_marea.h"
#include "m_ngroup.h"


int	NewUpdated = 0;


/*
 * Count nr of newfiles records in the database.
 * Creates the database if it doesn't exist.
 */
int CountNewfiles(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX], group[13];
	int	count, i;

	snprintf(ffile, PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			newfileshdr.hdrsize = sizeof(newfileshdr);
			newfileshdr.recsize = sizeof(newfiles);
			newfileshdr.grpsize = CFG.new_groups * 13;
			fwrite(&newfileshdr, sizeof(newfileshdr), 1, fil);
			memset(&newfiles, 0, sizeof(newfiles));

			snprintf(newfiles.Comment, 56, "General newfiles announce");
			snprintf(newfiles.Area, 51, "%s/var/mail/local/users", getenv("MBSE_ROOT"));
			snprintf(newfiles.Origin, 51, "%s", CFG.origin);
			snprintf(newfiles.From, 36, "Sysop");
			snprintf(newfiles.Too, 36, "All");
			snprintf(newfiles.Subject, 61, "New files found");
			snprintf(newfiles.Template, 15, "newfiles");
			newfiles.Language = 'E';
			newfiles.Active = TRUE;
			newfiles.charset = FTNC_CP437;
			fwrite(&newfiles, sizeof(newfiles), 1, fil);
			snprintf(group, 13, "LOCAL");
			fwrite(&group, 13, 1, fil);
			memset(&group, 0, sizeof(group));
			for (i = 1; i < CFG.new_groups; i++)
			    fwrite(&group, 13, 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 1;
		} else
			return -1;
	}

	fread(&newfileshdr, sizeof(newfileshdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - newfileshdr.hdrsize) / (newfileshdr.recsize + newfileshdr.grpsize);
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenNewfiles(void)
{
    FILE	*fin, *fout;
    char	*fnin, *fnout, group[13];
    int		oldsize, i, old_groups, oldgroup;

    fnin  = calloc(PATH_MAX, sizeof(char));
    fnout = calloc(PATH_MAX, sizeof(char));
    snprintf(fnin,  PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/newfiles.temp", getenv("MBSE_ROOT"));
	
    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&newfileshdr, sizeof(newfileshdr), 1, fin);
	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize    = newfileshdr.recsize;
	    oldgroup   = newfileshdr.grpsize;
	    old_groups = oldgroup / 13;
	    if ((oldsize != sizeof(newfiles)) || (CFG.new_groups != old_groups))
		NewUpdated = 1;
	    else
		NewUpdated = 0;
	    if (oldsize != sizeof(newfiles))
		Syslog('+', "Updated %s, format changed", fnin);
	    else if (CFG.new_groups != old_groups)
		Syslog('+', "Updated %s, nr of groups now %d", fnin, CFG.new_groups);
	    
	    newfileshdr.hdrsize = sizeof(newfileshdr);
	    newfileshdr.recsize = sizeof(newfiles);
	    newfileshdr.grpsize = CFG.new_groups * 13;
	    fwrite(&newfileshdr, sizeof(newfileshdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&newfiles, 0, sizeof(newfiles));
	    while (fread(&newfiles, oldsize, 1, fin) == 1) {
		if (!strlen(newfiles.Template)) {
		    snprintf(newfiles.Template, 15, "newfiles");
		    NewUpdated = 1;
		}
		if (newfiles.charset == FTNC_NONE) {
		    newfiles.charset = FTNC_CP437;
		    NewUpdated = 1;
		}
		fwrite(&newfiles, sizeof(newfiles), 1, fout);
		memset(&newfiles, 0, sizeof(newfiles));
		/*
		 * Copy the existing groups
		 */
		for (i = 1; i <= old_groups; i++) {
		    fread(&group, 13, 1, fin);
		    if (i <= CFG.new_groups)
			fwrite(&group, 13, 1, fout);
		}
		if (old_groups < CFG.new_groups) {
		    /*
		     * The size increased, fill with blank records.
		     */
		    memset(&group, 0, 13);
		    for (i = (old_groups + 1); i <= CFG.new_groups; i++)
			fwrite(&group, 13, 1, fout);
		}
	    }
	    fclose(fin);
	    fclose(fout);
	    free(fnin);
	    free(fnout);
	    return 0;
	}
    }

    free(fnin);
    free(fnout);
    return -1;
}



void CloseNewfiles(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX], group[13];
	FILE	*fi, *fo;
	st_list	*new = NULL, *tmp;
	int	i;

	snprintf(fin,  PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/newfiles.temp", getenv("MBSE_ROOT"));

	if (NewUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&newfileshdr, newfileshdr.hdrsize, 1, fi);
			fwrite(&newfileshdr, newfileshdr.hdrsize, 1, fo);

			while (fread(&newfiles, newfileshdr.recsize, 1, fi) == 1) {
				if (!newfiles.Deleted)
					fill_stlist(&new, newfiles.Comment, ftell(fi) - newfileshdr.recsize);
				fseek(fi, newfileshdr.grpsize, SEEK_CUR);
			}
			sort_stlist(&new);

			for (tmp = new; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&newfiles, newfileshdr.recsize, 1, fi);
				fwrite(&newfiles, newfileshdr.recsize, 1, fo);
				for (i = 0; i < (newfileshdr.grpsize / sizeof(group)); i++) {
					fread(&group, sizeof(group), 1, fi);
					fwrite(&group, sizeof(group), 1, fo);
				}
			}

			fclose(fi);
			fclose(fo);
			tidy_stlist(&new);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"newfiles.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendNewfiles(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX], group[13];
	int	i;

	snprintf(ffile, PATH_MAX, "%s/etc/newfiles.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&newfiles, 0, sizeof(newfiles));
		/*
		 * Fill in default values
		 */
		snprintf(newfiles.From, 36, "%s", CFG.sysop_name);
		newfiles.Language = 'E';
		snprintf(newfiles.Template, 15, "newfiles");
		newfiles.charset = FTNC_CP437;
		strncpy(newfiles.Origin, CFG.origin, 50);
		fwrite(&newfiles, sizeof(newfiles), 1, fil);
		memset(&group, 0, 13);
		for (i = 1; i <= CFG.new_groups; i++)
			fwrite(&group, 13, 1, fil);
		fclose(fil);
		NewUpdated = 1;
		return 0;
	} else
		return -1;
}



void NewScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "12. EDIT NEW FILES REPORTS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.  Comment");
	mbse_mvprintw( 8, 2, "2.  Msg area");
	mbse_mvprintw( 9, 2, "3.  Origin line");
	mbse_mvprintw(10, 2, "4.  From name");
	mbse_mvprintw(11, 2, "5.  To name");
	mbse_mvprintw(12, 2, "6.  Subject");
	mbse_mvprintw(13, 2, "7.  Language");
	mbse_mvprintw(14, 2, "8.  Template");
	mbse_mvprintw(15, 2, "9.  Aka to use");
	mbse_mvprintw(16, 2, "10. Active");
	mbse_mvprintw(17, 2, "11. Deleted");
	mbse_mvprintw(16,42, "12. New groups");
	mbse_mvprintw(17,42, "13. CHRS kludge");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditNewRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX], temp1[2];
	int		offset;
	unsigned int	crc, crc1;
	gr_list		*fgr = NULL, *tmp;
	char		group[13];
	int		groups, i, j, GrpChanged = FALSE;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Newfiles");

	snprintf(mfile, PATH_MAX, "%s/etc/ngroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) != NULL) {
		fread(&ngrouphdr, sizeof(ngrouphdr), 1, fil);

		while (fread(&ngroup, ngrouphdr.recsize, 1, fil) == 1)
			fill_grlist(&fgr, ngroup.Name);

		fclose(fil);
		sort_grlist(&fgr);
	}

	snprintf(mfile, PATH_MAX, "%s/etc/newfiles.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		tidy_grlist(&fgr);
		return -1;
	}

	fread(&newfileshdr, sizeof(newfileshdr), 1, fil);
	offset = newfileshdr.hdrsize + ((Area -1) * (newfileshdr.recsize + newfileshdr.grpsize));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		tidy_grlist(&fgr);
		return -1;
	}

	fread(&newfiles, newfileshdr.recsize, 1, fil);
	groups = newfileshdr.grpsize / 13;

	for (i = 0; i < groups; i++) {
		fread(&group, sizeof(group), 1, fil);
		if (strlen(group)) {
			for (tmp = fgr; tmp; tmp = tmp->next)
				if (!strcmp(tmp->group, group))
					tmp->tagged = TRUE;
		}
	}

	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&newfiles, crc, newfileshdr.recsize);

	for (;;) {
		NewScreen();
		set_color(WHITE, BLACK);
		show_str(  7,18,55, newfiles.Comment);
		show_str(  8,18,50, newfiles.Area);
		show_str(  9,18,50, newfiles.Origin);
		show_str( 10,18,35, newfiles.From);
		show_str( 11,18,35, newfiles.Too);
		show_str( 12,18,60, newfiles.Subject);
		snprintf(temp1, 2, "%c", newfiles.Language);
		show_str( 13,18,2,  temp1);
		show_str( 14,18,14, newfiles.Template);
		show_str( 15,18,35, aka2str(newfiles.UseAka));
		show_bool(16,18,    newfiles.Active);
		show_bool(17,18,    newfiles.Deleted);
		i = 0;
		for (tmp = fgr; tmp; tmp = tmp->next)
			if (tmp->tagged)
				i++;
		show_int( 16,58, i);
		show_charset(17,58, newfiles.charset);

		switch(select_menu(13)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&newfiles, crc1, newfileshdr.recsize);
			if ((crc != crc1) || GrpChanged) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&newfiles, newfileshdr.recsize, 1, fil);

					groups = newfileshdr.grpsize / 13;
					i = 0;
					for (tmp = fgr; tmp; tmp = tmp->next)
						if (tmp->tagged) {
							i++;
							memset(&group, 0, 13);
							snprintf(group, 13, "%s", tmp->group);
							fwrite(&group, 13, 1, fil);
						}

					memset(&group, 0, 13);
					for (j = i; j < groups; j++)
						fwrite(&group, 13, 1, fil);

					fclose(fil);
					NewUpdated = 1;
					working(6, 0, 0);
				}
			}
			tidy_grlist(&fgr);
			IsDoing("Browsing Menu");
			return 0;

		case 1:	E_STR(  7,18,55, newfiles.Comment,   "The ^comment^ for this area")
		case 2: strcpy(newfiles.Area, PickMsgarea((char *)"12.2"));
			break;
		case 3: E_STR(  9,18,50, newfiles.Origin,    "The ^origin line^ to append, leave blank for random lines")
		case 4: E_STR( 10,18,35, newfiles.From,      "The ^From^ name to appear above the messages")
		case 5: E_STR( 11,18,35, newfiles.Too,       "The ^To^ name to appear above the messages")
		case 6: E_STR( 12,18,60, newfiles.Subject,   "The ^Subject^ of the messages")
		case 7: newfiles.Language = PickLanguage((char *)"12.7");
			break;
		case 8: E_STR( 14,18,14, newfiles.Template,  "The ^template^ file to use for the report")
		case 9: i = PickAka((char *)"12.9", TRUE);
			if (i != -1)
				newfiles.UseAka = CFG.aka[i];
			break;
		case 10:E_BOOL(16,18,    newfiles.Active,    "If this report is ^active^")
		case 11:E_BOOL(17,18,    newfiles.Deleted,   "Is this record ^deleted^")
		case 12:if (E_Group(&fgr, (char *)"12.13 NEWFILE GROUPS"))
				GrpChanged = TRUE;
			break;
		case 13:newfiles.charset = edit_charset(17,58, newfiles.charset);
			break;
		}
	}
}



void EditNewfiles(void)
{
    int	    records, i, o, x, y;
    char    pick[12], temp[PATH_MAX];
    FILE    *fil;
    int	    offset;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    records = CountNewfiles();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenNewfiles() == -1) {
	working(2, 0, 0);
	return;
    }
    o = 0;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 4, "12. NEWFILES REPORTS");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/newfiles.temp", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&newfileshdr, sizeof(newfileshdr), 1, fil);
		x = 2;
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 20; i++) {
		    if (i == 11) {
			x = 42;
			y = 7;
		    }
		    if ((o + i) <= records) {
			offset = sizeof(newfileshdr) + (((o + i) - 1) * (newfileshdr.recsize + newfileshdr.grpsize));
			fseek(fil, offset, 0);
			fread(&newfiles, newfileshdr.recsize, 1, fil);
			if (newfiles.Active)
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			snprintf(temp, 81, "%3d.  %-32s", o + i, newfiles.Comment);
			temp[37] = 0;
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_record(records, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseNewfiles(FALSE);
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    if (records < CFG.new_groups) {
		working(1, 0, 0);
		if (AppendNewfiles() == 0) {
		    records++;
		    working(1, 0, 0);
		} else
		    working(2, 0, 0);
	    } else {
		errmsg("Cannot add, change global setting in menu 1.14.3");
	    }
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 20) < records) 
		o = o + 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o = o - 20;

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    EditNewRec(atoi(pick));
	    o = ((atoi(pick) - 1) / 20) * 20;
	}
    }
}



void InitNewfiles(void)
{
    CountNewfiles();
    OpenNewfiles();
    CloseNewfiles(TRUE);
}



int new_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX], group[13];
    FILE    *wp, *ip, *no;
    int	    groups, i, j, nr = 0;

    snprintf(temp, PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 12, 0, page, (char *)"Newfiles reports");
    j = 0;

    fprintf(fp, "\n\n");
    fread(&newfileshdr, sizeof(newfileshdr), 1, no);

    ip = open_webdoc((char *)"newfiles.html", (char *)"Newfiles Reports", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<UL>\n");

    while ((fread(&newfiles, newfileshdr.recsize, 1, no)) == 1) {

	if (j == 3) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}
	nr++;
	snprintf(temp, 81, "newfiles_%d.html", nr);
	fprintf(ip, " <LI><A HREF=\"%s\">Report %d</A> %s</LI>\n", temp, nr, newfiles.Comment);
	if ((wp = open_webdoc(temp, (char *)"Newfiles report", newfiles.Comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"newfiles.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Area comment", newfiles.Comment);
	    add_webtable(wp, (char *)"Message area", newfiles.Area);
	    add_webtable(wp, (char *)"Origin line", newfiles.Origin);
	    add_webtable(wp, (char *)"From name", newfiles.From);
	    add_webtable(wp, (char *)"To name", newfiles.Too);
	    add_webtable(wp, (char *)"Subject", newfiles.Subject);
	    snprintf(temp, 81, "%c", newfiles.Language);
	    add_webtable(wp, (char *)"Language", temp);
	    add_webtable(wp, (char *)"Aka to use", aka2str(newfiles.UseAka));
	    add_webtable(wp, (char *)"Active", getboolean(newfiles.Active));
	    add_webtable(wp, (char *)"CHRS kludge", getftnchrs(newfiles.charset));
	    fprintf(fp, "     Area comment      %s\n", newfiles.Comment);
	    fprintf(fp, "     Message area      %s\n", newfiles.Area);
	    fprintf(fp, "     Origin line       %s\n", newfiles.Origin);
	    fprintf(fp, "     From name         %s\n", newfiles.From);
	    fprintf(fp, "     To name           %s\n", newfiles.Too);
	    fprintf(fp, "     Subject           %s\n", newfiles.Subject);
	    fprintf(fp, "     Language          %c\n", newfiles.Language);
	    fprintf(fp, "     Aka to use        %s\n", aka2str(newfiles.UseAka));
	    fprintf(fp, "     Active            %s\n", getboolean(newfiles.Active));
	    fprintf(fp, "     CHRS kludge       %s\n", getftnchrs(newfiles.charset));
	    fprintf(fp, "\n     File groups:\n     ");
	    groups = newfileshdr.grpsize / sizeof(group);
	    for (i = 0; i < groups; i++) {
		fread(&group, sizeof(group), 1, no);
		if (strlen(group)) {
		    if (i)
			fprintf(wp, "<TR><TH>&nbsp;</TH><TD><A HREF=\"newgroup.html\">%s</A></TD></TR>\n", group);
		    else
			fprintf(wp, "<TR><TH align='left'>New groups</TH><TD><A HREF=\"newgroup.html\">%s</A></TD></TR>\n", 
				group);
		    fprintf(fp, "%-12s ", group);
		    if (((i+1) %5) == 0)
			fprintf(fp, "\n     ");
		}
	    }
	    if ((i+1) % 5)
		fprintf(fp, "\n");
	    fprintf(fp, "\n\n\n");
	    j++;
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}
    }

    fprintf(ip, "</UL>\n");
    close_webdoc(ip);
	
    fclose(no);
    return page;
}


