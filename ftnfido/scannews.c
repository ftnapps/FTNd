/*****************************************************************************
 *
 * $Id: scannews.c,v 1.38 2007/08/26 12:21:16 mbse Exp $
 * Purpose ...............: Scan for new News
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
#include "../lib/mbinet.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "mkftnhdr.h"
#include "hash.h"
#include "rollover.h"
#include "storeecho.h"
#include "rfc2ftn.h"
#include "scannews.h"


#define MAXHDRSIZE 2048
#define	MAXSEEN 70
#define	MAXPATH 73


/*
 *  Global variables
 */
POverview	xoverview = NULL;
int		marker = 0;



/*
 *  External variables
 */
extern	int	do_quiet;
extern	int	do_learn;
extern	int	news_in;
extern	int	news_imp;
extern	int	news_dupe;
extern	int	echo_out;
extern	int	echo_in;
extern	int	do_flush;
extern	char	*replyaddr;



/*
 *  Internal functions
 */
int		do_one_group(List **, char *, char *, int);
int		get_xover(char *, int, int, List **);
int		get_xoverview(void);
void		tidy_artlist(List **);
void		fill_artlist(List **, char *, int, int);
void		Marker(void);
int		get_article(char *, char *);



void tidy_artlist(List **fdp)
{
	List	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add article to the list
 */
void fill_artlist(List **fdp, char *id, int nr, int dupe)
{
	List	**tmp;

	for (tmp = fdp; *tmp; tmp = &((*tmp)->next));
	*tmp = (List *)malloc(sizeof(List));
	(*tmp)->next = NULL;
	snprintf((*tmp)->msgid, MAX_MSGID_LEN, "%s", id);
	(*tmp)->nr = nr;
	(*tmp)->isdupe = dupe;
}



void Marker(void)
{
        if (do_quiet)
                return;

        switch (marker) {
                case 0: printf(">---");
                        break;

                case 1: printf(">>--");
                        break;

                case 2: printf(">>>-");
                        break;

                case 3: printf(">>>>");
                        break;

                case 4: printf("<>>>");
                        break;

                case 5: printf("<<>>");
                        break;

                case 6: printf("<<<>");
                        break;

                case 7: printf("<<<<");
                        break;

                case 8: printf("-<<<");
                        break;

                case 9: printf("--<<");
                        break;

                case 10:printf("---<");
                        break;

                case 11:printf("----");
                        break;
        }
        printf("\b\b\b\b");
        fflush(stdout);

        if (marker < 11)
                marker++;
        else
                marker = 0;
}



/*
 *  Scan for new news available at the nntp server.
 */
void ScanNews(void)
{
    List		*art = NULL;
    POverview		tmp, old;
    FILE		*pAreas;
    char		*sAreas;
    struct msgareashdr	Msgshdr;
    struct msgareas	Msgs;

    IsDoing((char *)"Scan News");
    if (nntp_connect() == -1) {
	WriteError("Can't connect to newsserver");
	return;
    }
    if (get_xoverview()) {
	return;
    }

    if (!do_quiet) {
	mbse_colour(LIGHTGREEN, BLACK);
	printf("Scan for new news articles\n");
    }

    sAreas = calloc(PATH_MAX, sizeof(char));
    snprintf(sAreas, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if(( pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open Messages Areas File.");
	return;
    }
    fread(&Msgshdr, sizeof(Msgshdr), 1, pAreas);

    while (fread(&Msgs, Msgshdr.recsize, 1, pAreas) == 1) {
	fseek(pAreas, Msgshdr.syssize, SEEK_CUR);
#ifdef USE_NEWSGATE
	if ((Msgs.Active) && strlen(Msgs.Newsgroup)) {
#else
	if ((Msgs.Active) && strlen(Msgs.Newsgroup) && (Msgs.Type == NEWS)) {
#endif
	    if (IsSema((char *)"upsalarm")) {
		Syslog('+', "Detected upsalarm semafore, aborting newsscan");
		break;
	    }
	    Syslog('m', "Scan newsgroup: %s", Msgs.Newsgroup);
	    if (!do_quiet) {
		mbse_colour(CYAN, BLACK);
		printf("\r%-40s", Msgs.Newsgroup);
		fflush(stdout);
	    }
	    Nopper();
	    if (do_one_group(&art, Msgs.Newsgroup, Msgs.Tag, Msgs.MaxArticles) == RETVAL_ERROR)
		break;
	    /*
	     * To be safe, update the dupes database after each area.
	     */
	    CloseDupes();
	}
    }
    fclose(pAreas);
    free(sAreas);

    for (tmp = xoverview; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->header)
	    free(tmp->header);
	if (tmp->field)
	    free(tmp->field);
	free(tmp);
    }

    nntp_close();

    do_flush = TRUE;
    if (!do_quiet)
	printf("\r                                                    \r");
}



int do_one_group(List **art, char *grpname, char *ftntag, int maxarticles)
{
    List    *tmp;
    char    temp[128], *resp;
    int	    retval, fetched = 0;
    int	    total, start, end;

    Syslog('m', "do_one_group(%s, %s)", grpname, ftntag);
    IsDoing((char *)"Scan %s", grpname);
    snprintf(temp, 128, "GROUP %s\r\n", grpname);
    nntp_send(temp);
    resp = nntp_receive();
    retval = atoi(strtok(resp, " "));
    if (retval == 480) {
	/*
	 * We must login
	 */
	if (nntp_auth() == FALSE) {
	    WriteError("Authorisation failure");
	    nntp_close();
	    return RETVAL_NOAUTH;
	}
	nntp_send(temp);
	resp = nntp_receive();
	retval = atoi(strtok(resp, " "));
    }
    if (retval != 211) {
	if (retval == 411) {
	    WriteError("No such newsgroup: %s", grpname);
	    return RETVAL_UNEXPECTEDANS;
	}
	WriteError("Unknown response %d to GROUP command", retval);
	return RETVAL_ERROR;
    }

    total = atol(strtok(NULL, " "));
    start = atol(strtok(NULL, " "));
    end   = atol(strtok(NULL, " '\0'"));
    Syslog('m', "GROUP total %d, start %d, end %d, max %d", total, start, end, maxarticles);
    if ((maxarticles) && (total > maxarticles)) {
	start = end - maxarticles;
	total = maxarticles;
	Syslog('m', "NEW:  total %d, start %d, end %d", total, start, end);
    }
    if (!total) {
	Syslog('+', "Fetched 0 articles from %s", grpname);
	return RETVAL_NOARTICLES;
    }

    retval = get_xover(grpname, start, end, art);
    if (retval != RETVAL_OK) {
	tidy_artlist(art);
	return retval;
    }

    if (!do_learn) {
	for (tmp = *art; tmp; tmp = tmp->next) {
	    if (!tmp->isdupe) {
		/*
		 *  If the message isn't a dupe, it must be new for us.
		 */
		get_article(tmp->msgid, ftntag);
		fetched++;
	    }
	}
    }

    tidy_artlist(art);

    if ((maxarticles) && (fetched == maxarticles))
	Syslog('!', "Warning: the max. articles value in newsgroup %s might be to low", grpname);

    Syslog('+', "Fetched %d article%s from %s", fetched, (fetched == 1) ? "":"s", grpname);
    return RETVAL_OK;
}



int get_article(char *msgid, char *ftntag)
{
    char    cmd[81], *resp;
    int	    retval, done = FALSE;
    FILE    *fp = NULL, *dp;
    char    dpath[PATH_MAX];

    Syslog('m', "Get article %s, %s", msgid, ftntag);
    if (!SearchMsgs(ftntag)) {
	WriteError("Search message area %s failed", ftntag);
	return RETVAL_ERROR;
    }

    snprintf(dpath, PATH_MAX, "%s/tmp/scannews.last", getenv("MBSE_ROOT"));
    dp = fopen(dpath, "w");

    IsDoing("Article %d", (news_in + 1));
    snprintf(cmd, 81, "ARTICLE %s\r\n", msgid);
    fprintf(dp, "ARTICLE %s\n", msgid);
    nntp_send(cmd);
    resp = nntp_receive();
    fprintf(dp, "%s\n", resp);
    retval = atoi(strtok(resp, " "));
    switch (retval) {
	case 412:   WriteError("No newsgroup selected");
		    return RETVAL_UNEXPECTEDANS;
	case 420:   WriteError("No current article has been selected");
		    return RETVAL_UNEXPECTEDANS;
	case 423:   WriteError("No such article in this group");
		    return RETVAL_UNEXPECTEDANS;
	case 430:   WriteError("No such article found");
		    return RETVAL_UNEXPECTEDANS;
	case 220:   if ((fp = tmpfile()) == NULL) {
			WriteError("$Can't open tmpfile");
			return RETVAL_UNEXPECTEDANS;
		    }
		    while (done == FALSE) {
			resp = nntp_receive();
			fwrite(resp, strlen(resp), 1, dp);
			fprintf(dp, "\n");
			fflush(dp);
			if ((strlen(resp) == 1) && (strcmp(resp, ".") == 0)) {
			    done = TRUE;
			} else {
			    fwrite(resp, strlen(resp), 1, fp);
			    fputc('\n', fp);
			}
		    }
		    break;
    }

    IsDoing("Article %d", (news_in));
    retval = rfc2ftn(fp, NULL);
    fclose(fp);
    fclose(dp);
    return retval;
}



int get_xover(char *grpname, int startnr, int endnr, List **art)
{
    char	    cmd[81], *ptr, *ptr2, *resp, *p;
    int		    retval, dupe, done = FALSE;
    int		    nr;
    unsigned int    crc;
    POverview	    pov;

    snprintf(cmd, 81, "XOVER %d-%d\r\n", startnr, endnr);
    if ((retval = nntp_cmd(cmd, 224))) {
	switch (retval) {
	    case 412:	WriteError("No newsgroup selected");
			return RETVAL_NOXOVER;
	    case 502:	WriteError("Permission denied");
			return RETVAL_NOXOVER;
	    case 420:	Syslog('m', "No articles in group %s", grpname);
			return RETVAL_OK;
	}
    }

    while (done == FALSE) {
	resp = nntp_receive();
	if ((strlen(resp) == 1) && (strcmp(resp, ".") == 0)) {
	    done = TRUE;
	} else {
	    Marker();
	    Nopper();
	    pov = xoverview;
	    ptr = resp;
	    ptr2 = ptr;

	    /*
	     * First item is the message number.
	     */
	    while (*ptr2 != '\0' && *ptr2 != '\t')
		ptr2++;
	    if (*ptr2 != '\0')
		*(ptr2) = '\0';
	    nr = atol(ptr);
	    ptr = ptr2;
	    ptr++;

	    /*
	     * Search the message-id
	     */
	    while (*ptr != '\0' && pov != NULL && strcmp(pov->header, "Message-ID:") != 0) {
		/*
		 * goto the next field, past the tab.
		 */
		pov = pov->next;

		while (*ptr != '\t' && *ptr != '\0')
		    ptr++;
		if (*ptr != '\0')
		    ptr++;
	    }
	    if (*ptr != '\0' && pov != NULL) {
		/*
		 * Found it, now find start of msgid
		 */
		while (*ptr != '\0' && *ptr != '<')
		    ptr++;
		if(ptr != '\0') {
		    ptr2 = ptr;
		    while(*ptr2 != '\0' && *ptr2 != '>')
			ptr2++;
		    if (*ptr2 != '\0') {
			*(ptr2+1) = '\0';
			p = xstrcpy(ptr);
			p = xstrcat(p, grpname);
			crc = str_crc32(p);
			dupe = CheckDupe(crc, D_NEWS, CFG.nntpdupes);
			fill_artlist(art, ptr, nr, dupe);
			free(p);
			if (CFG.slow_util && do_quiet)
			    msleep(1);
		    }
		}
	    }
	}
    }

    return RETVAL_OK;
}



int get_xoverview(void)
{
    int		retval, len, full, done = FALSE;
    char	*resp;
    POverview	tmp, curptr = NULL;

    Syslog('m', "Getting overview format list");
    if ((retval = nntp_cmd((char *)"LIST overview.fmt\r\n", 215)) == 0) {
	while (done == FALSE) {
	    resp = nntp_receive();
	    if ((strcmp(resp, ".") == 0) && (strlen(resp) == 1)) {
		done = TRUE;
	    } else {
		len = strlen(resp);
		/*
		 * Check for the full flag, which means the field name
		 * is in the xover string.
		 */
		full = (strstr(resp, ":full") == NULL) ? FALSE : TRUE;
		/*
		 * Now get rid of everything back to :
		 */
		while (resp[len] != ':')
		    resp[len--] = '\0';
		len++;

		tmp = malloc(sizeof(Overview));
		tmp->header = calloc(len + 1, sizeof(char));
		strncpy(tmp->header, resp, len);
		tmp->header[len] = '\0';
		tmp->next = NULL;
		tmp->field = NULL;
		tmp->fieldlen = 0;
		tmp->full = full;

		if (curptr == NULL) {
		    /* at head of list */
		    curptr = tmp;
		    xoverview = tmp;
		} else {
		    /* add to linked list */
		    curptr->next = tmp;
		    curptr = tmp;
		}
	    }
	}
    } else {
	return 1;
    }
    return 0;
}


