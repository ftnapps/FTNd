/*****************************************************************************
 *
 * $Id: rnews.c,v 1.15 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: rnews function
 * Remarks ...............: Most of these functions are borrowed from inn.
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
#include "rfc2ftn.h"
#include "mbfido.h"
#include "../paths.h"
#include "rnews.h"


#define	UUCPBUF 10240


typedef struct _HEADER {
    char	*Name;
    int         size;
} HEADER;
typedef void *POINTER;

static char     UNPACK[] = "gzip";
static HEADER   RequiredHeaders[] = {
    { (char *)"Message-ID",     10 },
#define _messageid      0
    { (char *)"Newsgroups",     10 },
#define _newsgroups     1
    { (char *)"From",            4 },
#define _from           2
    { (char *)"Date",            4 },
#define _date           3
    { (char *)"Subject",         7 },
#define _subject        4
    { (char *)"Path",            4 },
#define _path           5
};
#define IS_MESGID(hp)   ((hp) == &RequiredHeaders[_messageid])
#define IS_PATH(hp)     ((hp) == &RequiredHeaders[_path])
#define	IS_NG(hp)	((hp) == &RequiredHeaders[_newsgroups])


/*
 *  Some macro's
 */
#define NEW(T, c)		((T *)xmalloc((unsigned int)(sizeof (T) * (c))))
#define RENEW(p, T, c)		(p = (T *)realloc((char *)(p), (unsigned int)(sizeof (T) * (c))))
#define DISPOSE(p)		free((void *)p)
#define SIZEOF(array)		((int)(sizeof array / sizeof array[0]))
#define ENDOF(array)		(&array[SIZEOF(array)])
#define ISWHITE(c)		((c) == ' ' || (c) == '\t')
#define caseEQn(a, b, n)	(strncasecmp((a), (b), (size_t)(n)) == 0)



/*
 *  External variables
 */
extern	int	do_quiet;
extern	int	news_in;
extern	int	news_dupe;
extern	int	check_dupe;
extern	int	do_flush;


void ProcessOne(FILE *);


/*
 *  Find a header in an article.
 */
const char *HeaderFindMem(const char *, const int, const char *, const int);
const char *HeaderFindMem(const char *Article, const int ArtLen, const char *Header, const int HeaderLen)
{
	const char	*p;

	for (p = Article; ; ) {
		/*
		 * Match first character, then colon, then whitespace (don't
		 * delete that line -- meet the RFC!) then compare the rest
		 * of the word.
		 */
		if (HeaderLen + 1 < Article + ArtLen - p && p[HeaderLen] == ':'
			&& ISWHITE(p[HeaderLen + 1]) && caseEQn(p, Header, (size_t)HeaderLen)) {
			p += HeaderLen + 2;
			while (1) {
				for (; p < Article + ArtLen && ISWHITE(*p); p++)
					continue;
				if (p == Article+ArtLen)
					return NULL;
				else {
					if (*p != '\r' && *p != '\n')
						return p;
					else {
						/* handle multi-lined header */
						if (++p == Article + ArtLen)
							return NULL;
						if (ISWHITE(*p))
							continue;
						if (p[-1] == '\r' && *p== '\n') {
							if (++p == Article + ArtLen)
								return NULL;
							if (ISWHITE(*p))
								continue;
							return NULL;
						}
						return NULL;
					}
				}
			}
		}
		if ((p = memchr(p, '\n', ArtLen - (p - Article))) == NULL ||
			(++p >= Article + ArtLen) || (*p == '\n') || 
			(((*p == '\r') && (++p >= Article + ArtLen)) || (*p == '\n')))
			return NULL;
	}
}



/*
 *  Open up a pipe to a process with fd tied to its stdin.  Return a
 *  descriptor tied to its stdout or -1 on error.
 */
static int StartChild(int, char *, char *[]);
static int StartChild(int fd, char *path, char *argv[])
{
    int	pan[2], i;
    pid_t	pid;

    /* Create a pipe. */
    if (pipe(pan) < 0) {
	WriteError("%Cant pipe for %s", path);
	die(MBERR_EXEC_FAILED);
    }

    /* Get a child. */
    for (i = 0; (pid = fork()) < 0; i++) {
	if (i == MAX_FORKS) {
	    WriteError("$Cant fork %s -- spooling", path);
	    return -1;
	}
	Syslog('m', "Cant fork %s -- waiting", path);
	(void)sleep(60);
    }

    /* Run the child, with redirection. */
    if (pid == 0) {
	(void)close(pan[PIPE_READ]);

	/* Stdin comes from our old input. */
	if (fd != STDIN) {
	    if ((i = dup2(fd, STDIN)) != STDIN) {
		WriteError("$Cant dup2 %d to 0 got %d", fd, i);
		_exit(MBERR_EXEC_FAILED);
	    }
	    (void)close(fd);
	}

	/* Stdout goes down the pipe. */
	if (pan[PIPE_WRITE] != STDOUT) {
	    if ((i = dup2(pan[PIPE_WRITE], STDOUT)) != STDOUT) {
		WriteError("$Cant dup2 %d to 1 got %d", pan[PIPE_WRITE], i);
		_exit(MBERR_EXEC_FAILED);
	    }
	    (void)close(pan[PIPE_WRITE]);
	}

	Syslog('m', "execv %s %s", MBSE_SS(path), MBSE_SS(argv[1]));
	(void)execv(path, argv);
	WriteError("$Cant execv %s", path);
	_exit(MBERR_EXEC_FAILED);
    }

    (void)close(pan[PIPE_WRITE]);
    (void)close(fd);
    return pan[PIPE_READ];
}



/*
 *  Wait for the specified number of children.
 */
void WaitForChildren(int i)
{
	pid_t	pid;
	int	status;

	while (--i >= 0) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid < 0) {
			if (errno != ECHILD)
				WriteError("$Cant wait");
			break;
		}
	}
}



/*
 *  Write an article to the rejected directory.
 */
static void Reject(const char *, const char *, const char *);
static void Reject(const char *article, const char *reason, const char *arg)
{
#if     defined(DO_RNEWS_SAVE_BAD)
	char	buff[SMBUF];
	FILE	*F;
	int	i;
#endif  /* defined(DO_RNEWS_SAVE_BAD) */

	WriteError(reason, arg);
#if     defined(DO_RNEWS_SAVE_BAD)
//    TempName(PATHBADNEWS, buff);
//    if ((F = fopen(buff, "w")) == NULL) {
//        syslog(L_ERROR, "cant fopen %s %m", buff);
//        return;
//    }
//    i = strlen(article);
//    if (fwrite((POINTER)article, (size_t)1, (size_t)i, F) != i)
//        syslog(L_ERROR, "cant fwrite %s %m", buff);
//    if (fclose(F) == EOF)
//        syslog(L_ERROR, "cant close %s %m", buff);
#endif  /* defined(DO_RNEWS_SAVE_BAD) */
}



/*
 *  Process one article.  Return TRUE if the article was okay; FALSE if the
 *  whole batch needs to be saved (such as when the server goes down or if
 *  the file is corrupted).
 */
static int Process(char *);
static int Process(char *article)
{
	HEADER	*hp;
	char	*p;
	char	*id = NULL;
	FILE	*fp;

	/*
	 * Empty article?
	 */
	if (*article == '\0')
		return TRUE;

	/*
	 * Make sure that all the headers are there.
	 */
	for (hp = RequiredHeaders; hp < ENDOF(RequiredHeaders); hp++) {
		if ((p = (char *)HeaderFindMem(article, strlen(article), hp->Name, hp->size)) == NULL) {
			Reject(article, "bad_article missing %s", hp->Name);
			return FALSE;
		}
		if (IS_MESGID(hp)) {
			id = p;
			continue;
		}
	}

	/*
	 *  Put the article in a temp file, all other existing functions
	 *  did already work with tempfiles.
	 */
	fp = tmpfile();
	fwrite(article, 1, strlen(article), fp);
	ProcessOne(fp);
	fclose(fp);

	return TRUE;
}



/*
 *  Read the rest of the input as an article.  Just punt to stdio in
 *  this case and let it do the buffering.
 */
static int ReadRemainder(register int, char, char);
static int ReadRemainder(register int fd, char first, char second)
{
    register FILE   *F;
    register char   *article;
    register int    size;
    register int    used;
    register int    left;
    register int    i;
    int		    ok;

    /* Turn the descriptor into a stream. */
    if ((F = fdopen(fd, "r")) == NULL) {
	WriteError("$Can't fdopen %d", fd);
	die(MBERR_GENERAL);
    }

    /* Get an initial allocation, leaving space for the \0. */
    size = BUFSIZ + 1;
    article = NEW(char, size + 2);
    article[0] = first;
    article[1] = second;
    used = second ? 2 : 1;
    left = size - used;

    /* Read the input. */
    while ((i = fread((POINTER)&article[used], (size_t)1, (size_t)left, F)) != 0) {
	if (i < 0) {
	    WriteError("$Cant fread after %d bytes", used);
	    die(MBERR_GENERAL);
	}
	used += i;
	left -= i;
	if (left < SMBUF) {
	    size += BUFSIZ;
	    left += BUFSIZ;
	    RENEW(article, char, size);
	}
    }
    if (article[used - 1] != '\n')
	article[used++] = '\n';
    article[used] = '\0';
    (void)fclose(F);

    ok = Process(article);
    DISPOSE(article);
    return ok;
}



/*
 *  Read an article from the input stream that is artsize bytes long.
 */
static int ReadBytecount(register int, int);
static int ReadBytecount(register int fd, int artsize)
{
	static char	*article;
	static int	oldsize;
	register char	*p;
	register int	left;
	register int	i;

	/* If we haven't gotten any memory before, or we didn't get enough,
	 * then get some. */
	if (article == NULL) {
		oldsize = artsize;
		article = NEW(char, oldsize + 1 + 1);
	} else if (artsize > oldsize) {
		oldsize = artsize;
		RENEW(article, char, oldsize + 1 + 1);
	}

	/* Read in the article. */
	for (p = article, left = artsize; left; p += i, left -= i)
		if ((i = read(fd, p, left)) <= 0) {
			i = errno;
			WriteError("$Cant read wanted %d got %d", artsize, artsize - left);
#if     0
			/* Don't do this -- if the article gets re-processed we
			 * will end up accepting the truncated version. */
			artsize = p - article;
			article[artsize] = '\0';
			Reject(article, "short read (%s?)", strerror(i));
#endif  /* 0 */
			return TRUE;
		}
	if (p[-1] != '\n')
		*p++ = '\n';
	*p = '\0';

	return Process(article);
}



/*
 *  Read a single text line; not unlike fgets().  Just more inefficient.
 */
static int ReadLine(char *, int, int);
static int ReadLine(char *p, int size, int fd)
{
    char    *save;

    /* Fill the buffer, a byte at a time. */
    for (save = p; size > 0; p++, size--) {
	if (read(fd, p, 1) != 1) {
	    *p = '\0';
	    WriteError("$Cant read first line got %s", save);
	    die(MBERR_GENERAL);
	}
	if (*p == '\n') {
	    *p = '\0';
	    return TRUE;
	}
    }
    *p = '\0';
    WriteError("bad_line too long %s", save);
    return FALSE;
}



/*
 *  Unpack a single batch.
 */
static int UnpackOne(int *, int *);
static int UnpackOne(int *fdp, int *countp)
{
	char	buff[SMBUF];
	char	*cargv[4];
	int	artsize;
	int	i;
	int	gzip = 0;
	int	HadCount;
	int	SawCunbatch;

	*countp = 0;
	for (SawCunbatch = FALSE, HadCount = FALSE; ; ) {
		/* Get the first character. */
		if ((i = read(*fdp, &buff[0], 1)) < 0) {
			WriteError("$cant read first character");
			return FALSE;
		}
		if (i == 0)
			break;

		if (buff[0] == 0x1f)
			gzip = 1;
		else if (buff[0] != RNEWS_MAGIC1)
			/* Not a batch file.  If we already got one count, the batch
			 * is corrupted, else read rest of input as an article. */
			return HadCount ? FALSE : ReadRemainder(*fdp, buff[0], '\0');

		/* Get the second character. */
		if ((i = read(*fdp, &buff[1], 1)) < 0) {
			WriteError("$Cant read second character");
			return FALSE;
		}
		if (i == 0)
			/* A one-byte batch? */
			return FALSE;

		/* Check second magic character. */
		/* gzipped ($1f$8b) or compressed ($1f$9d) */
		if (gzip && ((buff[1] == (char)0x8b) || (buff[1] == (char)0x9d))) {
			cargv[0] = (char *)"gzip";
			cargv[1] = (char *)"-d";
			cargv[2] = NULL;
			lseek(*fdp, (int) 0, 0); /* Back to the beginning */
			*fdp = StartChild(*fdp, (char*)_PATH_GZIP, cargv);
			if (*fdp < 0)
				return FALSE;
			(*countp)++;
			SawCunbatch = TRUE;
			continue;
		}
		if (buff[1] != RNEWS_MAGIC2)
			return HadCount ? FALSE : ReadRemainder(*fdp, buff[0], buff[1]);

		/* Some kind of batch -- get the command. */
		if (!ReadLine(&buff[2], (int)(sizeof buff - 3), *fdp))
			return FALSE;

		if (strncmp(buff, "#! rnews ", 9) == 0) {
			artsize = atoi(&buff[9]);
			if (artsize <= 0) {
				WriteError("Bad_line bad count %s", buff);
				return FALSE;
			}
			HadCount = TRUE;
			if (ReadBytecount(*fdp, artsize))
				continue;
			return FALSE;
		}

		if (HadCount)
			/* Already saw a bytecount -- probably corrupted. */
			return FALSE;

		if (strcmp(buff, "#! cunbatch") == 0) {
			Syslog('m', "Compressed newsbatch");
			if (SawCunbatch) {
				WriteError("Nested_cunbatch");
				return FALSE;
			}
			cargv[0] = UNPACK;
			cargv[1] = (char *)"-d";
			cargv[2] = NULL;
			*fdp = StartChild(*fdp, (char *)_PATH_GZIP, cargv);
			if (*fdp < 0)
				return FALSE;
			(*countp)++;
			SawCunbatch = TRUE;
			continue;
		}

		WriteError("bad_format unknown command %s", buff);
		return FALSE;
	}
	return TRUE;
}



void NewsUUCP(void)
{
	int	fd = STDIN, i, rc;

	Syslog('+', "Processing UUCP newsbatch");
	IsDoing((char *)"UUCP Batch");

	if (!do_quiet) {
		mbse_colour(LIGHTGREEN, BLACK);
		printf("Process UUCP Newsbatch\n");
	}

	rc = UnpackOne(&fd, &i);
	WaitForChildren(i);
	Syslog('+', "End of UUCP batch, rc=%d", rc);
	do_flush = TRUE;

	if (!do_quiet)
		printf("\r                                                    \r");
}



/*
 *  Process one newsarticle.
 */
void ProcessOne(FILE *fp)
{
	char		*p, *fn, *buf, *gbuf, *mbuf, *group, *groups[25];
	int		i, nrofgroups;
	unsigned int	crc;

	buf    = calloc(UUCPBUF, sizeof(char));
	fn     = calloc(PATH_MAX, sizeof(char));

	/*
	 *  Find newsgroups names in article.
	 */
	rewind(fp);
	nrofgroups = 0;
	mbuf = NULL;
	gbuf = NULL;
	while (fgets(buf, UUCPBUF, fp)) {
		if (!strncasecmp(buf, "Newsgroups: ", 12)) {
			gbuf = xstrcpy(buf+12);
			Striplf(gbuf);
			strtok(buf, " ");
			while ((group = strtok(NULL, ",\n"))) {
				if (SearchMsgsNews(group)) {
					Syslog('m', "Add group %s (%s)", msgs.Newsgroup, msgs.Tag);
					groups[nrofgroups] = xstrcpy(group);
					nrofgroups++;
				} else {
					Syslog('-', "Newsgroup %s doesn't exist", group);
				}
			}
		}
		if (!strncasecmp(buf, "Message-ID: ", 12)) {
			/*
			 *  Store the Message-ID without the < > characters.
			 */
			mbuf = xstrcpy(buf+13);
			mbuf[strlen(mbuf)-2] = '\0';
			Syslog('m', "Message ID \"%s\"", printable(mbuf, 0));
		}
	}

	if (nrofgroups == 0) {
		WriteError("No newsgroups found: %s", gbuf);
	} else if (mbuf == NULL) {
		WriteError("No valid Message-ID found");
	} else {
		IsDoing("Article %d", (news_in + 1));
		for (i = 0; i < nrofgroups; i++) {
			Syslog('m', "Process %s", groups[i]);
			p = xstrcpy(mbuf);
			p = xstrcat(p, groups[i]);
			crc = str_crc32(p);
			if (check_dupe && CheckDupe(crc, D_NEWS, CFG.nntpdupes)) {
				news_dupe++;
				news_in++;
				Syslog('+', "Duplicate article \"%s\" in group %s", mbuf, groups[i]);
			} else {
				if (SearchMsgsNews(groups[i])) {
					rfc2ftn(fp, NULL);
				}
			}
			free(groups[i]);
		}
	}

	if (mbuf)
		free(mbuf);
	if (gbuf)
		free(gbuf);

	free(buf);
	free(fn);
}



