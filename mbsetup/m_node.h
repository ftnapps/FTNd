#ifndef _NODE_H
#define _NODE_H

/* $Id: m_node.h,v 1.3 2003/01/31 21:49:30 mbroek Exp $ */

int	    CountNoderec(void);
int	    OpenNoderec(void);
void	    CloseNoderec(int);
int	    GroupInNode(char *, int);
void	    EditNodes(void);
void	    InitNodes(void);
int	    node_doc(FILE *, FILE *, int);
fidoaddr    PullUplink(char *);

#endif
