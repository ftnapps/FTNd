/* filelist.h, */

#ifndef	_FILELIST_H
#define	_FILELIST_H

char *xtodos(char *);
void add_list(file_list **, char *, char *, int, off_t, FILE *, int);
file_list *create_filelist(fa_list *, char *, int);
file_list *create_freqlist(fa_list *);
void tidy_filelist(file_list *, int);
void execute_disposition(file_list *);
char *transfertime(struct timeval, struct timeval, int, int);
char *compress_stat(int, int);

#endif

