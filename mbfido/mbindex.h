#ifndef	_MBINDEX_H
#define	_MBINDEX_H

/* $Id: mbindex.h,v 1.4 2004/07/10 15:03:49 mbse Exp $ */

typedef	struct	_fd_list {
    struct _fd_list	*next;
    char		fname[65];
    time_t		fdate;
} fd_list;


int	lockindex(void);
void	ulockindex(void);
void	Help(void);
void	ProgName(void);
void	die(int);
char	*fullpath(char *);
int	compile(char *, unsigned short, unsigned short, unsigned short);
void	tidy_fdlist(fd_list **);
void	fill_fdlist(fd_list **, char *, time_t);
void	sort_fdlist(fd_list **);
char	*pull_fdlist(fd_list **);
int	makelist(char *, unsigned short, unsigned short, unsigned short);
void	tidy_nllist(nl_list **);
int	in_nllist(struct _nlidx, nl_list **, int);
void	fill_nllist(struct _nlidx, nl_list **);
void    tidy_nluser(nl_user **);
void    fill_nluser(struct _nlusr, nl_user **);
int	comp_user(nl_user **, nl_user **);
int	comp_node(nl_list **, nl_list **);
int	nodebld(void);

#endif

