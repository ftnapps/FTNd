/*****************************************************************************
 *
 * $Id: m_virus.c,v 1.22 2008/02/17 16:10:18 mbse Exp $
 * Purpose ...............: Setup Virus structure.
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
#include "m_virus.h"



int	VirUpdated = 0;


/*
 * Count nr of virscan records in the database.
 * Creates the database if it doesn't exist.
 */
int CountVirus(void)
{
    FILE	*fil;
    char	ffile[PATH_MAX];
    int		count;

    snprintf(ffile, PATH_MAX, "%s/etc/virscan.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    virscanhdr.hdrsize = sizeof(virscanhdr);
	    virscanhdr.recsize = sizeof(virscan);
	    fwrite(&virscanhdr, sizeof(virscanhdr), 1, fil);
	    
	    /*
	     *  Create some default records and enable them if they exist.
	     */
	    memset(&virscan, 0, sizeof(virscan));
	    snprintf(virscan.comment, 41, "AntiVir/Linux Scanner");
	    if (strlen(_PATH_ANTIVIR)) {
		snprintf(virscan.scanner, 65, "%s", _PATH_ANTIVIR);
		virscan.available = TRUE;
	    } else {
		snprintf(virscan.scanner, 65, "/usr/bin/antivir");
		virscan.available = FALSE;
	    }
	    snprintf(virscan.options, 65, "-allfiles -s -q");
	    virscan.scantype = SCAN_EXTERN;
	    fwrite(&virscan, sizeof(virscan), 1, fil);

	    memset(&virscan, 0, sizeof(virscan));
	    snprintf(virscan.comment, 41, "F-Prot commandline scanner");
	    if (strlen(_PATH_FPROT)) {
		snprintf(virscan.scanner, 65, "%s .", _PATH_FPROT);
		virscan.available = TRUE;
	    } else {
		snprintf(virscan.scanner, 65, "/usr/local/bin/fpscan .");
		virscan.available = FALSE;
	    }
	    snprintf(virscan.options, 65, "-archive -silent");
	    virscan.scantype = SCAN_EXTERN;
	    fwrite(&virscan, sizeof(virscan), 1, fil);

	    memset(&virscan, 0, sizeof(virscan));
	    snprintf(virscan.comment, 41, "McAfee VirusScan for Linux");
	    if (strlen(_PATH_UVSCAN)) {
		snprintf(virscan.scanner, 65, "%s", _PATH_UVSCAN);
		virscan.available = TRUE;
	    } else {
		snprintf(virscan.scanner, 65, "/usr/local/bin/uvscan");
		virscan.available = FALSE;
	    }
            snprintf(virscan.options, 65, "--noboot --noexpire -r --secure -");
	    virscan.scantype = SCAN_EXTERN;
            fwrite(&virscan, sizeof(virscan), 1, fil);

	    memset(&virscan, 0, sizeof(virscan));
	    snprintf(virscan.comment, 41, "Clam AntiVirus commandline scanner");
	    if (strlen(_PATH_CLAMAV)) {
		snprintf(virscan.scanner, 65, "%s", _PATH_CLAMAV);
		virscan.available = TRUE;
	    } else {
		snprintf(virscan.scanner, 65, "/usr/local/bin/clamscan");
		virscan.available = FALSE;
	    }
	    snprintf(virscan.options, 65, "--quiet --recursive");
	    virscan.scantype = SCAN_EXTERN;
	    fwrite(&virscan, sizeof(virscan), 1, fil);

	    memset(&virscan, 0, sizeof(virscan));
	    snprintf(virscan.comment, 41, "Clam AntiVirus stream scanner");
	    virscan.available = FALSE;
	    virscan.scantype = CLAM_STREAM;
	    snprintf(virscan.host, 65, "localhost");
	    virscan.port = 3310;
	    fwrite(&virscan, sizeof(virscan), 1, fil);

	    memset(&virscan, 0, sizeof(virscan));
	    snprintf(virscan.comment, 41, "F-Prot stream scanner");
	    virscan.available = FALSE;
	    virscan.scantype = FP_STREAM;
	    snprintf(virscan.host, 65, "localhost");
	    virscan.port = 10200;
	    fwrite(&virscan, sizeof(virscan), 1, fil);
			
	    fclose(fil);
	    chmod(ffile, 0640);
	    return 6;
	} else
	    return -1;
    }

    fread(&virscanhdr, sizeof(virscanhdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - virscanhdr.hdrsize) / virscanhdr.recsize;
    fclose(fil);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenVirus(void);
int OpenVirus(void)
{
    FILE	*fin, *fout;
    char	fnin[PATH_MAX], fnout[PATH_MAX];
    int		oldsize, has_stream = FALSE;

    snprintf(fnin,  PATH_MAX, "%s/etc/virscan.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/virscan.temp", getenv("MBSE_ROOT"));
    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&virscanhdr, sizeof(virscanhdr), 1, fin);
	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize = virscanhdr.recsize;
	    if (oldsize != sizeof(virscan)) {
		VirUpdated = 1;
		Syslog('+', "Upgraded %s, format changed", fnin);
	    } else
		VirUpdated = 0;
	    virscanhdr.hdrsize = sizeof(virscanhdr);
	    virscanhdr.recsize = sizeof(virscan);
	    fwrite(&virscanhdr, sizeof(virscanhdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&virscan, 0, sizeof(virscan));
	    while (fread(&virscan, oldsize, 1, fin) == 1) {
		/*
		 * Check if we already have defined stream scanners
		 */
		if (virscan.scantype != SCAN_EXTERN)
		    has_stream = TRUE;
		fwrite(&virscan, sizeof(virscan), 1, fout);
		memset(&virscan, 0, sizeof(virscan));
	    }

	    fclose(fin);
	    if ((oldsize != sizeof(virscan)) && (! has_stream)) {
		memset(&virscan, 0, sizeof(virscan));
		snprintf(virscan.comment, 41, "Clam AntiVirus stream scanner");
		virscan.available = FALSE;
		virscan.scantype = CLAM_STREAM;
		snprintf(virscan.host, 65, "localhost");
		virscan.port = 3310;
		fwrite(&virscan, sizeof(virscan), 1, fout);

		memset(&virscan, 0, sizeof(virscan));
		snprintf(virscan.comment, 41, "F-Prot stream scanner");
		virscan.available = FALSE;
		virscan.scantype = FP_STREAM;
		snprintf(virscan.host, 65, "localhost");
		virscan.port = 10200;
		fwrite(&virscan, sizeof(virscan), 1, fout);
	    }

	    fclose(fout);
	    return 0;
	} else
	    return -1;
    }
    return -1;
}



void CloseVirus(int);
void CloseVirus(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*vir = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/virscan.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/virscan.temp", getenv("MBSE_ROOT"));

	if (VirUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&virscanhdr, virscanhdr.hdrsize, 1, fi);
			fwrite(&virscanhdr, virscanhdr.hdrsize, 1, fo);

			while (fread(&virscan, virscanhdr.recsize, 1, fi) == 1)
				if (!virscan.deleted)
					fill_stlist(&vir, virscan.comment, ftell(fi) - virscanhdr.recsize);
			sort_stlist(&vir);

			for (tmp = vir; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&virscan, virscanhdr.recsize, 1, fi);
				fwrite(&virscan, virscanhdr.recsize, 1, fo);
			}

			tidy_stlist(&vir);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"virscan.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	unlink(fout); 
}



int AppendVirus(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/virscan.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&virscan, 0, sizeof(virscan));
		fwrite(&virscan, sizeof(virscan), 1, fil);
		fclose(fil);
		VirUpdated = 1;
		return 0;
	} else
		return -1;
}



void ScanScreen(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "4.  EDIT VIRUS SCANNER");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.  Comment");
    mbse_mvprintw( 8, 2, "2.  Type");
    mbse_mvprintw( 9, 2, "3.  Available");
    mbse_mvprintw(10, 2, "4.  Deleted");
    mbse_mvprintw(11, 2, "5.  Error lvl");

    if (virscan.scantype == SCAN_EXTERN) {
    	mbse_mvprintw(12, 2, "6.  Command");
    	mbse_mvprintw(13, 2, "7.  Options");
    } else {
	mbse_mvprintw(12, 2, "6.  Host");
	mbse_mvprintw(13, 2, "7.  Port");
    }
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditVirRec(int Area)
{
    FILE		*fil;
    char		mfile[PATH_MAX];
    int			offset, j;
    unsigned int	crc, crc1;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit VirScan");

    snprintf(mfile, PATH_MAX, "%s/etc/virscan.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    offset = sizeof(virscanhdr) + ((Area -1) * sizeof(virscan));
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	return -1;
    }

    fread(&virscan, sizeof(virscan), 1, fil);
    fclose(fil);
    crc = 0xffffffff;
    crc = upd_crc32((char *)&virscan, crc, sizeof(virscan));

    ScanScreen();
	
    for (;;) {
	set_color(WHITE, BLACK);
	show_str(  7,16,40, virscan.comment);
	show_scannertype(8, 16, virscan.scantype);
	show_bool( 9,16,    virscan.available);
	show_bool(10,16,    virscan.deleted);
	show_int( 11,16,    virscan.error);

	if (virscan.scantype == SCAN_EXTERN) {
	    show_str( 12,16,64, virscan.scanner);
	    show_str( 13,16,64, virscan.options);
	} else {
	    show_str( 12,16,64, virscan.host);
	    show_int( 13,16,    virscan.port);
	}

	j = select_menu(7);
	switch(j) {
	case 0:	crc1 = 0xffffffff;
		crc1 = upd_crc32((char *)&virscan, crc1, sizeof(virscan));
		if (crc != crc1) {
		    if (yes_no((char *)"Record is changed, save") == 1) {
			working(1, 0, 0);
			if ((fil = fopen(mfile, "r+")) == NULL) {
			    working(2, 0, 0);
			    return -1;
			}
			fseek(fil, offset, 0);
			fwrite(&virscan, sizeof(virscan), 1, fil);
			fclose(fil);
			VirUpdated = 1;
			working(6, 0, 0);
		    }
		}
		IsDoing("Browsing Menu");
		return 0;
	case 1: E_STR(  7,16,40,virscan.comment,  "The ^Comment^ for this record")
	case 2: virscan.scantype = edit_scannertype( 8,16,virscan.scantype);
		ScanScreen();
		break;
	case 3: E_BOOL( 9,16,   virscan.available,"Switch if this virus scanner is ^Available^ for use.")
	case 4: E_BOOL(10,16,   virscan.deleted,  "Is this scanner ^deleted^")
	case 5: E_INT( 11,16,   virscan.error,    "The ^Error Level^ the scanner returns when no virus is found")

	case 6:	if (virscan.scantype == SCAN_EXTERN) {
		    E_STR( 12,16,64,virscan.scanner,  "The full ^name and path^ to the binary of this scanner")
		} else {
		    E_STR( 12,16,64,virscan.host,  "The ^FQDN hostname^ or ^IP address^ where the stream scanner is running")
		}
	case 7:	if (virscan.scantype == SCAN_EXTERN) {
		    E_STR( 13,16,64,virscan.options,  "The ^commandline options^ for this scanner")
		} else {
		    E_INT( 13,16,   virscan.port,     "The ^port number^ for this stream scanner")
		}
	}
    }
    return 0;
}



void EditVirus(void)
{
	int	records, i, x, y;
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

	records = CountVirus();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenVirus() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "4.  VIRUS SCANNERS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/virscan.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&virscanhdr, sizeof(virscanhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(virscanhdr) + ((i - 1) * virscanhdr.recsize);
					fseek(fil, offset, 0);
					fread(&virscan, virscanhdr.recsize, 1, fil);
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if (virscan.available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					snprintf(temp, 81, "%3d.  %-32s", i, virscan.comment);
					temp[37] = 0;
					mbse_mvprintw(y, x, temp);
					y++;
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseVirus(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendVirus() == 0) {
				records++;
				working(3, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditVirRec(atoi(pick));
	}
}



void InitVirus(void)
{
    CountVirus();
    OpenVirus();
    CloseVirus(TRUE);
}



int virus_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *vir;
    int	    nr = 0, j;

    snprintf(temp, PATH_MAX, "%s/etc/virscan.data", getenv("MBSE_ROOT"));
    if ((vir = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 4, 0, page, (char *)"Virus scanners");
    j = 0;
    fprintf(fp, "\n\n");
    fread(&virscanhdr, sizeof(virscanhdr), 1, vir);

    ip = open_webdoc((char *)"virscan.html", (char *)"Virus Scanners", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Nr</TH><TH align='left'>Comment</TH><TH align='left'>Available</TH></TR>\n");
		    
    while ((fread(&virscan, virscanhdr.recsize, 1, vir)) == 1) {

	if (j == 5) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	nr++;
	snprintf(temp, 81, "virscan_%d.html", nr);
	fprintf(ip, "<TR><TD><A HREF=\"%s\">%d</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, nr, virscan.comment, getboolean(virscan.available));
	if ((wp = open_webdoc(temp, (char *)"Virus Scanner", virscan.comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"virscan.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Scanner name", virscan.comment);
	    add_webtable(wp, (char *)"Scanner type", get_scannertype(virscan.scantype));
	    add_webtable(wp, (char *)"Available", getboolean(virscan.available));
	    add_webdigit(wp, (char *)"Errorlevel OK", virscan.error);
	    if (virscan.scantype == SCAN_EXTERN) {
		add_webtable(wp, (char *)"Command line", virscan.scanner);
		add_webtable(wp, (char *)"Options", virscan.options);
	    } else {
		add_webtable(wp, (char *)"Host", virscan.host);
		add_webdigit(wp, (char *)"Port", virscan.port);
	    }
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}

	fprintf(fp, "      Scanner name   %s\n", virscan.comment);
	fprintf(fp, "      Scanner type   %s\n", get_scannertype(virscan.scantype));
	fprintf(fp, "      Available      %s\n", getboolean(virscan.available));
	fprintf(fp, "      Errorlevel OK  %d\n", virscan.error); 
	if (virscan.scantype == SCAN_EXTERN) {
	    fprintf(fp, "      Command line   %s\n", virscan.scanner);
	    fprintf(fp, "      Options        %s\n", virscan.options);
	} else {
	    fprintf(fp, "      Host           %s\n", virscan.host);
	    fprintf(fp, "      Port           %d\n", virscan.port);
	}
	fprintf(fp, "\n\n\n");
	j++;
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	    
    fclose(vir);
    return page;
}


