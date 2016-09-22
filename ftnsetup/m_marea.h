#ifndef _MAREA_H
#define _MAREA_H

/* m_marea.h */

int	OpenMsgarea(void);
void	CloseMsgarea(int);
int	GroupInMarea(char *);
int	NodeInMarea(fidoaddr);
int	CountMsgarea(void);
void	EditMsgarea(void);
void	InitMsgarea(void);
void	gold_areas(FILE *);
void	msged_areas(FILE *);
int	mail_area_doc(FILE *, FILE *, int);
char	*PickMsgarea(char *);


#endif

