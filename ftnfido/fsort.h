#ifndef	_FSORT_H
#define	_FSORT_H



typedef	struct	_fd_list {
	struct _fd_list	*next;
	char		fname[65];
	time_t		fdate;
} fd_list;


void	tidy_fdlist(fd_list **);
void	fill_fdlist(fd_list **, char *, time_t);
void	sort_fdlist(fd_list **);
char	*pull_fdlist(fd_list **);


#endif

