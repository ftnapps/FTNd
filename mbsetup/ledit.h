/* $Id$ */

#ifndef _LEDIT_H
#define _LEDIT_H

int 		yes_no(char *);
void		errmsg(const char *, ...);
void		show_field(int, int, char *, int, int);
void		newinsert(int, int, int);
char		*edit_field(int, int, int, int, char *);
char		*select_record(int, int);
char		*select_filearea(int, int);
char		*select_area(int, int);
char		*select_pick(int, int);
char		*select_show(int);
int 		select_menu(int);
int		select_tag(int);
void		show_str(int, int, int, char *);
char		*edit_str(int, int, int, char *, char *);
char		*edit_pth(int, int, int, char *, char *);
void		test_jam(char *);
char		*edit_jam(int, int, int, char *, char *);
char		*edit_ups(int, int, int, char *, char *);
char		*getboolean(int val);
void		show_bool(int, int, int);
int 		edit_bool(int, int, int, char *);
char		*getloglevel(long val);
void		show_logl(int, int, long);
long		edit_logl(long, char *);
char		*getflag(unsigned long, unsigned long);
void		show_sec(int, int, securityrec);
securityrec	edit_sec(int, int, securityrec, char *);
securityrec	edit_usec(int, int, securityrec, char *);
char		*get_secstr(securityrec);
void		show_int(int, int, int);
int 		edit_int(int, int, int, char *);
void		show_ushort(int, int, unsigned short);
unsigned short	edit_ushort(int, int, unsigned short, char *);
void		show_sbit(int, int, unsigned short, unsigned short);
unsigned short	toggle_sbit(int, int, unsigned short, unsigned short, char *);
void		show_lbit(int, int, long, long);
long		toggle_lbit(int, int, long, long, char *);
char		*getmsgtype(int);
void		show_msgtype(int, int, int);
int 		edit_msgtype(int, int, int);
char		*getemailmode(int);
void		show_emailmode(int, int, int);
int		edit_emailmode(int, int, int);
char		*getmsgkinds(int);
void		show_msgkinds(int, int, int);
int 		edit_msgkinds(int, int, int);
char		*getservice(int);
void		show_service(int, int, int);
int		edit_service(int, int, int);
char            *getnewsmode(int);
void            show_newsmode(int, int, int);
int             edit_newsmode(int, int, int);
char		*getmsgeditor(int);
void		show_msgeditor(int, int, int);
int		edit_msgeditor(int, int, int);
char		*getlinetype(int);
void		show_linetype(int, int, int);
int 		edit_linetype(int, int, int);
char		*getmagictype(int);
void		show_magictype(int, int, int);
int		edit_magictype(int, int, int);
void		show_aka(int, int, fidoaddr);
void		edit_color(int *, int *, char *, char *);
char		*get_color(int);
char		*getmenutype(int);


/*
 * Macro's for the edit functions
 */
#define E_STR(y,x,l,str,help) strcpy(str, edit_str(y,x,l,str,(char *)help)); break;
#define E_PTH(y,x,l,str,help) strcpy(str, edit_pth(y,x,l,str,(char *)help)); break;
#define E_UPS(y,x,l,str,help) strcpy(str, edit_ups(y,x,l,str,(char *)help)); break;
#define E_JAM(y,x,l,str,help) strcpy(str, edit_jam(y,x,l,str,(char *)help)); break;
#define E_BOOL(y,x,bool,help) bool = edit_bool(y,x,bool,(char *)help); break;
#define E_INT(y,x,value,help) value = edit_int(y,x,value,(char *)help); break;
#define E_LOGL(grade,txt,af)  grade = edit_logl(grade,(char *)txt); af(); break;
#define S_COL(y,x,name,fg,bg) set_color(fg,bg); mvprintw(y,x,name);
#define E_SEC(y,x,sec,hdr,af) sec = edit_sec(y,x,sec,(char *)hdr); af(); break;
#define E_USEC(y,x,sec,hdr,af) sec = edit_usec(y,x,sec,(char *)hdr); af(); break;

#endif /* _LEDIT_H */

