#ifndef _MBINET_H
#define	_MBINET_H

/* $Id$ */

int	smtp_connect(void);
int	smtp_send(char *);
char	*smtp_receive(void);
int	smtp_close(void);
int	smtp_cmd(char *, int);

int     nntp_connect(void);
int     nntp_send(char *);
char    *nntp_receive(void);
int     nntp_close(void);
int     nntp_cmd(char *, int);
int	nntp_auth(void);

int	pop3_connect(void);
int	pop3_send(char *);
char	*pop3_receive(void);
int	pop3_close(void);
int	pop3_cmd(char *);

#endif
