#ifndef	_MBERRORS_H
#define	_MBERRORS_H

/* $Id$ */

/*
 * Exit status values
 */
#define	MBERR_OK		0	/* No errors			    */
#define	MBERR_COMMANDLINE	100	/* Commandline error		    */
#define	MBERR_CONFIG_ERROR	101	/* Configuration error		    */
#define	MBERR_INIT_ERROR	102	/* Initialisation error		    */
#define	MBERR_DISK_FULL		103	/* Some disk partition full	    */
#define	MBERR_UPS_ALARM		104	/* UPS alarm detected		    */
#define	MBERR_NO_RECIPIENTS	105	/* No valid recipients		    */
#define	MBERR_EXEC_FAILED	106	/* Execute external prog failed	    */
#define	MBERR_TTYIO_ERROR	107	/* Set tty failed		    */
#define	MBERR_FTRANSFER		108	/* File transfer error		    */
#define	MBERR_ATTACH_FAILED	109	/* File attach failed		    */
#define	MBERR_NO_PROGLOCK	110	/* Cannot lock program, retry later */
#define MBERR_NODE_NOT_IN_LIST	111	/* Node not in nodelist		    */
#define	MBERR_NODE_MAY_NOT_CALL	112	/* Node may not be called	    */
#define	MBERR_NO_CONNECTION	113	/* Cannot make connection	    */
#define	MBERR_PORTERROR		114	/* Cannot open tty port		    */
#define	MBERR_NODE_LOCKED	115	/* Node is locked		    */
#define	MBERR_NO_IP_ADDRESS	116	/* Node IP address not found	    */
#define	MBERR_UNKNOWN_SESSION	117	/* Unknown session		    */
#define	MBERR_NOT_ZMH		118	/* Not Zone Mail Hour		    */
#define	MBERR_MODEM_ERROR	119	/* Modem error			    */
#define	MBERR_NO_PORT_AVAILABLE	120	/* No modemport available	    */
#define	MBERR_SESSION_ERROR	121	/* Session error (password)	    */
#define	MBERR_EMSI		122	/* EMSI session error		    */
#define	MBERR_FTSC		123	/* FTSC session error		    */
#define	MBERR_WAZOO		124	/* WAZOO session error		    */
#define	MBERR_YOOHOO		125	/* YOOHOO session error		    */
#define	MBERR_OUTBOUND_SCAN	126	/* Outbound scan error		    */
#define	MBERR_CANNOT_MAKE_POLL	127	/* Cannot make poll		    */
#define	MBERR_REQUEST		128	/* File request error		    */
#define MBERR_DIFF_ERROR	129	/* Error processing nodediff	    */
#define	MBERR_VIRUS_FOUND	130	/* Virus found			    */
#define	MBERR_GENERAL		131	/* General error		    */
#define	MBERR_TIMEOUT		132	/* Timeout error		    */
#define	MBERR_TTYIO		200	/* Base for ttyio errors	    */
#define	MBERR_MEMWATCH		255	/* Memwatch error		    */
#define	MBERR_EXTERNAL		256	/* Status external prog + 256	    */

#endif
