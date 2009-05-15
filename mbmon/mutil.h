#ifndef	_MUTIL_H
#define	_MUTIL_H

unsigned char	readkey(int, int, int, int);
unsigned char	testkey(int, int);
void		show_field(int, int, char *, int, int);
void		newinsert(int, int, int);
char		*edit_field(int, int, int, int, char *);
int 		select_menu(int);
void		clrtoeol(void);
void		hor_lin(int, int, int);
void		set_color(int, int);
void		show_date(int, int, int, int);
void		center_addstr(int y, char *s);
void		screen_start(char *);
void		screen_stop(void);
void		working(int, int, int);
void		clr_index(void);
void		showhelp(char *);

#endif

