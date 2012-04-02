#ifndef	_SCREEN_H
#define	_SCREEN_H


void clrtoeol(void);
void hor_lin(int y, int x, int len);
void set_color(int f, int b);
void show_date(int f, int b, int y, int x);
void center_addstr(int y, char *s);
void screen_start(char *);
void screen_stop(void);
void working(int txno, int y, int x);
void clr_index(void);
void showhelp(char *T);

// int menu_prompt(void);

#endif

