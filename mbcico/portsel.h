#ifndef	_PORTSEL_H
#define	_PORTSEL_H


typedef struct _pp_list {
	struct _pp_list	*next;
	char		tty[7];			/* tty name of the port	*/
	unsigned long	mflags;			/* Analogue Modem flags	*/
	unsigned long	dflags;			/* ISDN flags		*/
	unsigned long	iflags;			/* TCP-IP flags		*/
	char		*uflags[MAXUFLAGS];	/* User flags		*/
	int		match;			/* Does port match	*/
} pp_list;


void tidy_pplist(pp_list **);
void fill_pplist(pp_list **, pp_list *);
int  make_portlist(node *, pp_list **);
int  load_port(char *);
int  load_modem(char *);


#endif

