#ifndef	_DBNODE_H
#define	_DBNODE_H


struct	_nodeshdr	nodeshdr;	/* Header record		    */
struct	_nodes		nodes;		/* Nodes datarecord		    */
int			nodes_cnt;	/* Node records in database	    */

int	InitNode(void);			/* Initialize nodes database	    */
int	TestNode(fidoaddr);		/* Check if noderecord is loaded    */
int	SearchNode(fidoaddr);		/* Search specified node and load   */
int	UpdateNode(void);		/* Update record if changed.	    */
char	*GetNodeMailGrp(int);		/* Get nodes mailgroup record	    */
char	*GetNodeFileGrp(int);		/* Get nodes filegroup record	    */


#endif

