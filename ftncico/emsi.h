#ifndef EMSI_H
#define EMSI_H

/* $Id: emsi.h,v 1.2 2003/02/23 21:00:49 mbroek Exp $ */

#define LCODE_PUA 0x0001
#define LCODE_PUP 0x0002
#define LCODE_NPU 0x0004
#define LCODE_HAT 0x0008
#define LCODE_HXT 0x0010
#define LCODE_HRQ 0x0020
#define	LCODE_FNC 0x0040
#define	LCODE_RMA 0x0080
#define	LCODE_RH1 0x0100

extern int emsi_local_lcodes;
extern int emsi_remote_lcodes;

#define	PROT_DZA 0x0001
#define PROT_ZAP 0x0002
#define PROT_ZMO 0x0004
#define	PROT_JAN 0x0008
#define PROT_KER 0x0010
#define PROT_HYD 0x0020
#define PROT_TCP 0x0040

extern int emsi_local_protos;
extern int emsi_remote_protos;

#define OPT_NRQ 0x0002
#define OPT_ARC 0x0004
#define OPT_XMA 0x0008
#define OPT_FNC 0x0010
#define OPT_CHT 0x0020
#define OPT_SLK 0x0040
#define	OPT_EII	0x0080
#define	OPT_DFB	0x0100
#define	OPT_FRQ	0x0200

extern int emsi_local_opts;
extern int emsi_remote_opts;

extern char *emsi_local_password;
extern char *emsi_remote_password;
extern char emsi_remote_comm[];


int rx_emsi(char *);
int tx_emsi(char *);


#endif
