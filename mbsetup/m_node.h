#ifndef _NODE_H
#define _NODE_H

int		OpenNoderec(void);
void		CloseNoderec(int);
int		GroupInNode(char *, int);
void		EditNodes(void);
int		node_doc(FILE *, FILE *, int);
fidoaddr	PullUplink(char *);

#endif

