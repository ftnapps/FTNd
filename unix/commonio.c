/*****************************************************************************
 *
 * commonio.c
 * Purpose ...............: FTNd Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <utime.h>
#include <sys/time.h>
#ifdef SHADOW_PASSWORD
#include <shadow.h>
#endif
#include "commonio.h"

/* local function prototypes */
static int	check_link_count (const char *);
static int	do_lock_file (const char *, const char *);
static FILE	*fopen_set_perms (const char *, const char *, const struct stat *);
static int	create_backup (const char *, FILE *);
static void	free_linked_list (struct commonio_db *);
static void	add_one_entry (struct commonio_db *, struct commonio_entry *);
static int	name_is_nis (const char *);
static int	write_all (const struct commonio_db *);
static struct	commonio_entry *find_entry_by_name (struct commonio_db *, const char *);

#ifdef HAVE_LCKPWDF
static int	lock_count = 0;
#endif

static int check_link_count(const char *file)
{
	struct stat sb;

	if (stat(file, &sb) != 0)
		return 0;

	if (sb.st_nlink != 2)
		return 0;

	return 1;
}


static int do_lock_file(const char *file, const char *lock)
{
	int fd;
	int pid;
	int len;
	int retval;
	char buf[32];

	if ((fd = open(file, O_CREAT|O_EXCL|O_WRONLY, 0600)) == -1)
		return 0;

	pid = getpid();
	snprintf(buf, sizeof buf, "%d", pid);
	len = strlen(buf) + 1;
	if (write (fd, buf, len) != len) {
		close(fd);
		unlink(file);
		return 0;
	}
	close(fd);

	if (link(file, lock) == 0) {
		retval = check_link_count(file);
		unlink(file);
		return retval;
	}

	if ((fd = open(lock, O_RDWR)) == -1) {
		unlink(file);
		errno = EINVAL;
		return 0;
	}
	len = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (len <= 0) {
		unlink(file);
		errno = EINVAL;
		return 0;
	}
	buf[len] = '\0';
	if ((pid = strtol(buf, (char **) 0, 10)) == 0) {
		unlink(file);
		errno = EINVAL;
		return 0;
	}
	if (kill(pid, 0) == 0)  {
		unlink(file);
		errno = EEXIST;
		return 0;
	}
	if (unlink(lock) != 0) {
		unlink(file);
		return 0;
	}

	retval = 0;
	if (link(file, lock) == 0 && check_link_count(file))
		retval = 1;

	unlink(file);
	return retval;
}



static FILE *fopen_set_perms(const char *name, const char *mode, const struct stat *sb)
{
	FILE	*fp;
	mode_t	mask;

	mask = umask(0777);
	fp = fopen(name, mode);
	umask(mask);
	if (!fp)
		return NULL;

#ifdef HAVE_FCHOWN
	if (fchown(fileno(fp), sb->st_uid, sb->st_gid))
		goto fail;
#else
        if (chown(name, sb->st_mode))
                goto fail;
#endif

#ifdef HAVE_FCHMOD
        if (fchmod(fileno(fp), sb->st_mode & 0664))
                goto fail;
#else
        if (chmod(name, sb->st_mode & 0664))
                goto fail;
#endif
	return fp;

fail:
	fclose(fp);
	unlink(name);
	return NULL;
}



static int create_backup(const char *backup, FILE *fp)
{
	struct stat sb;
	struct utimbuf ub;
	FILE *bkfp;
	int c;
	mode_t mask;

	if (fstat(fileno(fp), &sb))
		return -1;

	mask = umask(077);
	bkfp = fopen(backup, "w");
	umask(mask);
	if (!bkfp)
		return -1;

	/* TODO: faster copy, not one-char-at-a-time.  --marekm */
	rewind(fp);
	while ((c = getc(fp)) != EOF) {
		if (putc(c, bkfp) == EOF)
			break;
	}
	if (c != EOF || fflush(bkfp)) {
		fclose(bkfp);
		return -1;
	}
	if (fclose(bkfp))
		return -1;

	ub.actime = sb.st_atime;
	ub.modtime = sb.st_mtime;
	utime(backup, &ub);
	return 0;
}



static void free_linked_list(struct commonio_db *db)
{
	struct commonio_entry *p;

	while (db->head) {
		p = db->head;
		db->head = p->next;

		if (p->line)
			free(p->line);

		if (p->entry)
			db->ops->free(p->entry);

		free(p);
	}
	db->tail = NULL;
}



int commonio_setname(struct commonio_db *db, const char *name)
{
	strcpy(db->filename, name);
	return 1;
}



int commonio_present(const struct commonio_db *db)
{
	return (access(db->filename, F_OK) == 0);
}



int commonio_lock_nowait(struct commonio_db *db)
{
        char file[1024];
        char lock[1024];

        if (db->locked)
                return 1;

        snprintf(file, sizeof file, "%s.%ld", db->filename, (long) getpid());
        snprintf(lock, sizeof lock, "%s.lock", db->filename);
        if (do_lock_file(file, lock)) {
                db->locked = 1;
                return 1;
        }
        return 0;
}


int commonio_lock(struct commonio_db *db)
{
        int i;

#ifdef HAVE_LCKPWDF
        /*
         * only if the system libc has a real lckpwdf() - the one from
         * lockpw.c calls us and would cause infinite recursion!
         */
        if (db->use_lckpwdf) {
                /*
                 * Call lckpwdf() on the first lock.
                 * If it succeeds, call *_lock() only once
                 * (no retries, it should always succeed).
                 */
                if (lock_count == 0) {
                        if (lckpwdf() == -1)
                                return 0;  /* failure */
                }
                if (!commonio_lock_nowait(db)) {
                        ulckpwdf();
                        return 0;  /* failure */
                }
                lock_count++;
                return 1;  /* success */
        }
#endif
        /*
         * lckpwdf() not used - do it the old way.
         */
#ifndef LOCK_TRIES
#define LOCK_TRIES 15
#endif

#ifndef LOCK_SLEEP
#define LOCK_SLEEP 1
#endif
        for (i = 0; i < LOCK_TRIES; i++) {
                if (i > 0)
                        sleep(LOCK_SLEEP);  /* delay between retries */
                if (commonio_lock_nowait(db))
                        return 1;  /* success */
                /* no unnecessary retries on "permission denied" errors */
                if (geteuid() != 0)
                        return 0;
        }
        return 0;  /* failure */
}



int commonio_unlock(struct commonio_db *db)
{
	char	lock[1024];

	if (db->isopen) {
		db->readonly = 1;
		if (!commonio_close(db))
			return 0;
	}
        if (db->locked) {
                /*
                 * Unlock in reverse order: remove the lock file,
                 * then call ulckpwdf() (if used) on last unlock.
                 */
                db->locked = 0;
                snprintf(lock, sizeof lock, "%s.lock", db->filename);
                unlink(lock);
#ifdef HAVE_LCKPWDF
                if (db->use_lckpwdf && lock_count > 0) {
                        lock_count--;
                        if (lock_count == 0)
                                ulckpwdf();
                }
#endif
                return 1;
        }
        return 0;
}



static void add_one_entry(struct commonio_db *db, struct commonio_entry *p)
{
	p->next = NULL;
	p->prev = db->tail;
	if (!db->head)
		db->head = p;
	if (db->tail)
		db->tail->next = p;
	db->tail = p;
}



static int name_is_nis(const char *n)
{
	return (n[0] == '+' || n[0] == '-');
}


/*
 * New entries are inserted before the first NIS entry.  Order is preserved
 * when db is written out.
 */
#ifndef KEEP_NIS_AT_END
#define KEEP_NIS_AT_END 1
#endif

#if KEEP_NIS_AT_END
/* prototype */
static void add_one_entry_nis (struct commonio_db *, struct commonio_entry *);

static void add_one_entry_nis(struct commonio_db *db, struct commonio_entry *new)
{
	struct commonio_entry *p;

	for (p = db->head; p; p = p->next) {
		if (name_is_nis(p->entry ? db->ops->getname(p->entry) : p->line)) {
			new->next = p;
			new->prev = p->prev;
			if (p->prev)
				p->prev->next = new;
			else
				db->head = new;
			p->prev = new;
			return;
		}
	}
	add_one_entry(db, new);
}
#endif /* KEEP_NIS_AT_END */



int commonio_open(struct commonio_db *db, int mode)
{
	char	buf[8192];
	char	*cp;
	char *line;
	struct commonio_entry *p;
	void *entry;
	int flags = mode;

	mode &= ~O_CREAT;

	if (db->isopen || (mode != O_RDONLY && mode != O_RDWR)) {
		errno = EINVAL;
		return 0;
	}
	db->readonly = (mode == O_RDONLY);
	if (!db->readonly && !db->locked) {
		errno = EACCES;
		return 0;
	}

	db->head = db->tail = db->cursor = NULL;
	db->changed = 0;

	db->fp = fopen(db->filename, db->readonly ? "r" : "r+");

	/*
	 * If O_CREAT was specified and the file didn't exist, it will be
	 * created by commonio_close().  We have no entries to read yet.  --marekm
	 */
	if (!db->fp) {
		if ((flags & O_CREAT) && errno == ENOENT) {
			db->isopen++;
			return 1;
		}
		return 0;
	}

	while (db->ops->fgets(buf, sizeof buf, db->fp)) {
		if ((cp = strrchr(buf, '\n')))
			*cp = '\0';

		if (!(line = strdup(buf)))
			goto cleanup;

		if (name_is_nis(line)) {
			entry = NULL;
		} else if ((entry = db->ops->parse(line))) {
			entry = db->ops->dup(entry);
			if (!entry)
				goto cleanup_line;
		}

		p = (struct commonio_entry *) malloc(sizeof *p);
		if (!p)
			goto cleanup_entry;

		p->entry = entry;
		p->line = line;
		p->changed = 0;

		add_one_entry(db, p);
	}

	db->isopen++;
	return 1;

cleanup_entry:
	if (entry)
		db->ops->free(entry);
cleanup_line:
	free(line);
cleanup:
	free_linked_list(db);
	fclose(db->fp);
	db->fp = NULL;
	errno = ENOMEM;
	return 0;
}



static int write_all(const struct commonio_db *db)
{
	const struct commonio_entry *p;
	void *entry;

	for (p = db->head; p; p = p->next) {
		if (p->changed) {
			entry = p->entry;
			if (db->ops->put(entry, db->fp))
				return -1;
		} else if (p->line) {
			if (db->ops->fputs(p->line, db->fp) == EOF)
				return -1;
			if (putc('\n', db->fp) == EOF)
				return -1;
		}
	}
	return 0;
}



int commonio_close(struct commonio_db *db)
{
	char buf[1024];
	int errors = 0;
	struct stat sb;

	if (!db->isopen) {
		errno = EINVAL;
		return 0;
	}
	db->isopen = 0;

	if (!db->changed || db->readonly) {
		fclose(db->fp);
		db->fp = NULL;
		goto success;
	}

	memset(&sb, 0, sizeof sb);
	if (db->fp) {
		if (fstat(fileno(db->fp), &sb)) {
			fclose(db->fp);
			db->fp = NULL;
			goto fail;
		}

		/*
		 * Create backup file.
		 */
		snprintf(buf, sizeof buf, "%s-", db->filename);

		if (create_backup(buf, db->fp))
			errors++;

		if (fclose(db->fp))
			errors++;

		if (errors) {
			db->fp = NULL;
			goto fail;
		}
	} else {
		/*
		 * Default permissions for new [g]shadow files.
		 * (passwd and group always exist...)
		 */
		sb.st_mode = 0400;
		sb.st_uid = 0;
		sb.st_gid = 0;
	}

	snprintf(buf, sizeof buf, "%s+", db->filename);

	db->fp = fopen_set_perms(buf, "w", &sb);
	if (!db->fp)
		goto fail;

	if (write_all(db))
		errors++;

	if (fflush(db->fp))
		errors++;
#ifdef HAVE_FSYNC
	if (fsync(fileno(db->fp)))
		errors++;
#else
        sync();
#endif
	if (fclose(db->fp))
		errors++;

	db->fp = NULL;

	if (errors) {
		unlink(buf);
		goto fail;
	}

	if (rename(buf, db->filename))
		goto fail;

success:
	free_linked_list(db);
	return 1;

fail:
	free_linked_list(db);
	return 0;
}



static struct commonio_entry * find_entry_by_name(struct commonio_db *db, const char *name)
{
	struct commonio_entry *p;
	void *ep;

	for (p = db->head; p; p = p->next) {
		ep = p->entry;
		if (ep && strcmp(db->ops->getname(ep), name) == 0)
			break;
	}
	return p;
}



int commonio_update(struct commonio_db *db, const void *entry)
{
	struct commonio_entry *p;
	void *nentry;

	if (!db->isopen || db->readonly) {
		errno = EINVAL;
		return 0;
	}
	if (!(nentry = db->ops->dup(entry))) {
		errno = ENOMEM;
		return 0;
	}
	p = find_entry_by_name(db, db->ops->getname(entry));
	if (p) {
		db->ops->free(p->entry);
		p->entry = nentry;
		p->changed = 1;
		db->cursor = p;

		db->changed = 1;
		return 1;
	}
	/* not found, new entry */
	p = (struct commonio_entry *) malloc(sizeof *p);
	if (!p) {
		db->ops->free(nentry);
		errno = ENOMEM;
		return 0;
	}

	p->entry = nentry;
	p->line = NULL;
	p->changed = 1;

#if KEEP_NIS_AT_END
	add_one_entry_nis(db, p);
#else
	add_one_entry(db, p);
#endif

	db->changed = 1;
	return 1;
}



void commonio_del_entry(struct commonio_db *db, const struct commonio_entry *p)
{
	if (p == db->cursor)
		db->cursor = p->next;

	if (p->prev)
		p->prev->next = p->next;
	else
		db->head = p->next;

	if (p->next)
		p->next->prev = p->prev;
	else
		db->tail = p->prev;

	db->changed = 1;
}



int commonio_remove(struct commonio_db *db, const char *name)
{
	struct commonio_entry *p;

	if (!db->isopen || db->readonly) {
		errno = EINVAL;
		return 0;
	}
	p = find_entry_by_name(db, name);
	if (!p) {
		errno = ENOENT;
		return 0;
	}

	commonio_del_entry(db, p);

	if (p->line)
		free(p->line);

	if (p->entry)
		db->ops->free(p->entry);

	return 1;
}



const void * commonio_locate(struct commonio_db *db, const char *name)
{
	struct commonio_entry *p;

	if (!db->isopen) {
		errno = EINVAL;
		return NULL;
	}
	p = find_entry_by_name(db, name);
	if (!p) {
		errno = ENOENT;
		return NULL;
	}
	db->cursor = p;
	return p->entry;
}



int commonio_rewind(struct commonio_db *db)
{
	if (!db->isopen) {
		errno = EINVAL;
		return 0;
	}
	db->cursor = NULL;
	return 1;
}



const void * commonio_next(struct commonio_db *db)
{
	void *entry;

	if (!db->isopen) {
		errno = EINVAL;
		return 0;
	}
	if (db->cursor == NULL)
		db->cursor = db->head;
	else
		db->cursor = db->cursor->next;

	while (db->cursor) {
		entry = db->cursor->entry;
		if (entry)
			return entry;

		db->cursor = db->cursor->next;
	}
	return NULL;
}

