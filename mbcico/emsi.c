/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:	2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/nodelist.h"
#include "../lib/dbnode.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
#include "ttyio.h"
#include "session.h"
#include "statetbl.h"
#include "config.h"
#include "emsi.h"
#include "emsidat.h"
#include "hydra.h"
#include "rdoptions.h"
#include "tcp.h"
#include "wazoo.h"


#define LOCAL_PROTOS (PROT_ZMO | PROT_ZAP | PROT_HYD | PROT_TCP)

static int rxemsi(void);
static int txemsi(void);
static char *intro;
static int caller;

extern int	most_debug;
extern pid_t	mypid;

int	emsi_local_lcodes;
int	emsi_remote_lcodes;
int	emsi_local_protos;
int	emsi_remote_protos;
int	emsi_local_opts;
int	emsi_remote_opts;
char	*emsi_local_password = NULL;
char	*emsi_remote_password = NULL;
char	emsi_remote_comm[4]="8N1";



int rx_emsi(char *data)
{
    int	    rc;
    fa_list *tmr;
    int	    denypw=0;

    Syslog('+', "Start inbound EMSI session");

    emsi_local_lcodes = LCODE_RH1;
    emsi_remote_lcodes=0;

    emsi_local_protos=LOCAL_PROTOS;
    if (localoptions & NOZMODEM) 
	emsi_local_protos &= ~(PROT_ZMO | PROT_ZAP | PROT_DZA);
    if (localoptions & NOZEDZAP) 
	emsi_local_protos &= ~PROT_ZAP;
    if (localoptions & NOJANUS) 
	emsi_local_protos &= ~PROT_JAN;
    if (localoptions & NOHYDRA) 
	emsi_local_protos &= ~PROT_HYD;
    if ((localoptions & NOITN) || (localoptions & NOIFC) || ((session_flags & SESSION_TCP) == 0)) {
	emsi_local_protos &= ~PROT_TCP;
    }

    emsi_remote_protos=0;
    emsi_local_opts = OPT_XMA;
    emsi_remote_opts=0;
    emsi_local_password = NULL;
    emsi_remote_password = NULL;
    intro=data+2;
    caller=0;

    if ((rc=rxemsi())) 
	return MBERR_EMSI;

    Syslog('i', "local  lcodes 0x%04x, protos 0x%04x, opts 0x%04x", emsi_local_lcodes,emsi_local_protos,emsi_local_opts);
    Syslog('i', "remote lcodes 0x%04x, protos 0x%04x, opts 0x%04x", emsi_remote_lcodes,emsi_remote_protos,emsi_remote_opts);

    if (emsi_remote_opts & OPT_EII) {
	emsi_local_opts |= OPT_EII;
    }

    emsi_local_protos &= emsi_remote_protos;
    if (emsi_local_protos & PROT_TCP) 
	emsi_local_protos &= PROT_TCP;
    else if (emsi_local_protos & PROT_HYD) 
	emsi_local_protos &= PROT_HYD;
    else if (emsi_local_protos & PROT_JAN)
	emsi_local_protos &= PROT_JAN;
    else if (emsi_local_protos & PROT_ZAP) 
	emsi_local_protos &= PROT_ZAP;
    else if (emsi_local_protos & PROT_ZMO) 
	emsi_local_protos &= PROT_ZMO;
    else if (emsi_local_protos & PROT_DZA) 
	emsi_local_protos &= PROT_DZA;
    else if (emsi_local_protos & PROT_KER) 
	emsi_local_protos &= PROT_KER;

    emsi_local_password = NULL;

    for (tmr = remote; tmr; tmr = tmr->next)
	if (((nlent = getnlent(tmr->addr))) && (nlent->pflag != NL_DUMMY)) {
	    Syslog('+', "Remote is a listed system");
	    if (inbound)
		free(inbound);
	    inbound = xstrcpy(CFG.inbound);
	    UserCity(mypid, nlent->sysop, nlent->location);
	    break;
	}
    if (nlent) 
	rdoptions(TRUE);

    /*
     * Added these options, if they are in the setup for this
     * calling node, then disable these options.
     */
    if (localoptions & NOHYDRA)
	emsi_local_opts &= ~PROT_HYD;
    if (localoptions & NOZEDZAP)
	emsi_local_opts &= ~PROT_ZAP;
    if (localoptions & NOZMODEM)
	emsi_local_opts &= ~(PROT_ZMO | PROT_ZAP | PROT_DZA);

    if (localoptions & NOFREQS)
	emsi_local_opts |= OPT_NRQ;

    if (strlen(nodes.Spasswd)) {
	if ((strncasecmp(emsi_remote_password, nodes.Spasswd, strlen(nodes.Spasswd)) == 0) &&
	    (strlen(emsi_remote_password) == strlen(nodes.Spasswd))) {
	    emsi_local_password = xstrcpy(nodes.Spasswd);
	    if (inbound)
		free(inbound);
	    inbound = xstrcpy(CFG.pinbound);
	    Syslog('+', "Password correct, protected EMSI session");
	} else {
	    denypw = 1;
	    Syslog('?', "Remote password \"%s\", expected \"%s\"", MBSE_SS(emsi_remote_password), nodes.Spasswd);
	    emsi_local_password = xstrcpy((char *)"BAD_PASS");
	    emsi_local_lcodes = LCODE_HAT;
	}
    } else {
	Syslog('i', "No EMSI password check");
	Syslog('?', "Unexpected remote password \"%s\"", MBSE_SS(emsi_local_password));
    }

    Syslog('i', "local  lcodes 0x%04x, protos 0x%04x, opts 0x%04x", emsi_local_lcodes,emsi_local_protos,emsi_local_opts);

    if ((rc=txemsi())) 
	return MBERR_EMSI;

    if (denypw || (emsi_local_protos == 0)) {
	Syslog('+', "Refusing remote: %s", emsi_local_protos?"bad password presented": "no common protocols");
	return 0;
    }

    IsDoing("EMSI %s inb", ascfnode(remote->addr, 0x0f));

    if ((emsi_remote_opts & OPT_NRQ) == 0) 
	session_flags |= SESSION_WAZOO;
    else 
	session_flags &= ~SESSION_WAZOO;

    if (emsi_local_protos & PROT_TCP) 
	return rxtcp();
    else if (emsi_local_protos & PROT_HYD)
	return hydra(0);
//  else if (emsi_local_protos & PROT_JAN)
//	return janus();
    else 
	return rxwazoo();
}



int tx_emsi(char *data)
{
    int	rc;

    Syslog('+', "Start outbound EMSI session");
    emsi_local_lcodes = LCODE_PUA | LCODE_RH1;
    emsi_remote_lcodes = 0;

    emsi_local_protos=LOCAL_PROTOS;
    if (localoptions & NOZMODEM) 
	emsi_local_protos &= ~(PROT_ZMO | PROT_ZAP | PROT_DZA);
    if (localoptions & NOZEDZAP) 
	emsi_local_protos &= ~PROT_ZAP;
    if (localoptions & NOJANUS)
	emsi_local_protos &= ~PROT_JAN;
    if (localoptions & NOHYDRA) 
	emsi_local_protos &= ~PROT_HYD;
    if ((localoptions & NOIFC) || (localoptions & NOITN) || ((session_flags & SESSION_TCP) == 0)) {
	emsi_local_protos &= ~PROT_TCP;
    }
    emsi_remote_protos=0;
    emsi_local_opts=OPT_XMA | OPT_EII | OPT_NRQ;
    emsi_remote_opts=0;
    emsi_local_password=NULL;
    emsi_remote_password=NULL;
    intro=data+2;
    caller=1;
    emsi_local_password=NULL;

    Syslog('i', "local  lcodes 0x%04x, protos 0x%04x, opts 0x%04x", emsi_local_lcodes,emsi_local_protos,emsi_local_opts);

    if ((rc=txemsi())) 
	return MBERR_EMSI;
    else {
	if ((rc=rxemsi())) 
	    return MBERR_EMSI;
    }

    if ((emsi_remote_opts & OPT_EII) == 0) {
	emsi_local_opts &= ~OPT_EII;
    }

    Syslog('i', "remote lcodes 0x%04x, protos 0x%04x, opts 0x%04x", emsi_remote_lcodes,emsi_remote_protos,emsi_remote_opts);

    if ((emsi_remote_protos == 0) || (emsi_remote_lcodes & LCODE_HAT)) {
	Syslog('+', "Remote refused us: %s", emsi_remote_protos?"traffic held":"no common protos");
	return MBERR_SESSION_ERROR;
    }

    IsDoing("EMSI %s out", ascfnode(remote->addr, 0x0f));

    emsi_local_protos &= emsi_remote_protos;
    if ((emsi_remote_opts & OPT_NRQ) == 0) 
	session_flags |= SESSION_WAZOO;
    else 
	session_flags &= ~SESSION_WAZOO;

    if (emsi_local_protos & PROT_TCP) 
	return txtcp();
    else if (emsi_local_protos & PROT_HYD)
	return hydra(1);
//  else if (emsi_local_protos & PROT_JAN)
//	return janus();
    else 
	return txwazoo();
}



SM_DECL(rxemsi,(char *)"rxemsi")
SM_STATES
	waitpkt,
	waitchar,
	checkemsi,
	getdat,
	checkpkt,
	checkdat,
	sendnak,
	sendack
SM_NAMES
	(char *)"waitpkt",
	(char *)"waitchar",
	(char *)"checkemsi",
	(char *)"getdat",
	(char *)"checkpkt",
	(char *)"checkdat",
	(char *)"sendnak",
	(char *)"sendack"
SM_EDECL

	int		c = 0;
	unsigned short	lcrc, rcrc;
	int		len;
	int		standby = 0, tries = 0;
	char		buf[13], *p;
	char		*databuf = NULL;

	p = buf;
	databuf = xstrcpy(intro);

SM_START(checkpkt)
	Syslog('i', "RXEMSI: start");

SM_STATE(waitpkt)

	standby = 0;
	SM_PROCEED(waitchar);

SM_STATE(waitchar)

	c = GETCHAR(5);
	if (c == TIMEOUT) {
		if (++tries > 9) {
			Syslog('+', "Too many tries waiting EMSI handshake");
			SM_ERROR;
		} else {
			SM_PROCEED(sendnak);
		}
	} else if (c < 0) {
		SM_ERROR;
	} else if ((c >= ' ') && (c <= '~')) {
		if (c == '*') {
			standby = 1;
			p = buf;
			*p = '\0';
		} else if (standby) {
			if ((p - buf) < (sizeof(buf) - 1)) {
				*p++ = c;
				*p = '\0';
			} if ((p - buf) >= (sizeof(buf) - 1)) {
				standby = 0;
				SM_PROCEED(checkemsi);
			}
		}
	} else switch(c) {
		case DC1:	break;
		case '\n':
		case '\r':	standby = 0;
				break;
		default:	standby = 0;
				break;
	}

	SM_PROCEED(waitchar);

SM_STATE(checkemsi)

	Syslog('i', "RXEMSI: rcvd %s", printable(buf, 0));

	if (strncasecmp(buf, "EMSI_DAT",8) == 0) {
		SM_PROCEED(getdat);
	} else if (strncasecmp(buf, "EMSI_",5) == 0) {
		if (databuf) 
			free(databuf);
		databuf = xstrcpy(buf);
		SM_PROCEED(checkpkt);
	} else {
		SM_PROCEED(waitpkt);
	}

SM_STATE(getdat)

	if (sscanf(buf+8,"%04x",&len) != 1) {
		SM_PROCEED(sendnak);
	}

	len += 16; /* strlen("EMSI_DATxxxxyyyy"), include CRC */
	if (databuf) 
		free(databuf);
	databuf = malloc(len + 1);
	strcpy(databuf, buf);
	p = databuf + strlen(databuf);

	while (((p-databuf) < len) && ((c=GETCHAR(8)) >= 0)) {
		*p++ = c;
		*p = '\0';
	}

	Syslog('i', "RXEMSI: rcvd %s (%d bytes)", databuf, len);

	if (c == TIMEOUT) {
		SM_PROCEED(sendnak);
	} else if (c < 0) {
		Syslog('+', "Error while reading EMSI_DAT packet");
		SM_ERROR;
	}

	SM_PROCEED(checkdat);

SM_STATE(checkpkt)

	if (strncasecmp(databuf,"EMSI_DAT",8) == 0) {
		SM_PROCEED(checkdat);
	}

	lcrc = crc16xmodem(databuf, 8);
	sscanf(databuf + 8, "%04hx", &rcrc);
	if (lcrc != rcrc) {
		Syslog('+', "Got EMSI packet \"%s\" with bad crc: %04x/%04x", printable(databuf, 0), lcrc, rcrc);
		SM_PROCEED(sendnak);
	} if (strncasecmp(databuf, "EMSI_HBT", 8) == 0) {
		tries = 0;
		SM_PROCEED(waitpkt);
	} else if (strncasecmp(databuf, "EMSI_INQ", 8) == 0) {
		SM_PROCEED(sendnak);
	} else {
		SM_PROCEED(waitpkt);
	}

SM_STATE(checkdat)

	sscanf(databuf + 8, "%04x", &len);
	if (len != (strlen(databuf) - 16)) {
		Syslog('+', "Bad EMSI_DAT length: %d/%d", len, strlen(databuf));
		SM_PROCEED(sendnak);
	}
	/* Some FD versions send length of the packet including the
	   trailing CR.  Arrrgh!  Dirty overwork follows: */
	if (*(p = databuf + strlen(databuf) - 1) == '\r') 
		*p='\0';
	sscanf(databuf + strlen(databuf) - 4, "%04hx", &rcrc);
	*(databuf + strlen(databuf) - 4) = '\0';
	lcrc = crc16xmodem(databuf, strlen(databuf));
	if (lcrc != rcrc) {
		Syslog('+', "Got EMSI_DAT packet \"%s\" with bad crc: %04x/%04x", printable(databuf, 0), lcrc, rcrc);
		SM_PROCEED(sendnak);
	} if (scanemsidat(databuf + 12) == 0) {
		SM_PROCEED(sendack);
	} else {
		Syslog('+', "Could not parse EMSI_DAT packet \"%s\"",databuf);
		SM_ERROR;
	}

SM_STATE(sendnak)

	if (++tries > 9) {
		Syslog('+', "Too many tries getting EMSI_DAT");
		SM_ERROR;
	} if (caller) {
		PUTSTR((char *)"**EMSI_NAKEEC3\r\021");
		Syslog('i', "RXEMSI: send **EMSI_NAKEEC3");
	} else {
		PUTSTR((char *)"**EMSI_REQA77E\r\021");
		Syslog('i', "RXEMSI: send **EMSI_REQA77E");
		if (tries > 1) {
			PUTSTR((char *)"**EMSI_NAKEEC3\r\021");
			Syslog('i', "RXEMSI: send **EMSI_NAKEEC3");
		}
	}
	SM_PROCEED(waitpkt);

SM_STATE(sendack)

	Syslog('i', "RXEMSI: send **EMSI_ACKA490 (2 times)"); 
	PUTSTR((char *)"**EMSI_ACKA490\r\021");
	PUTSTR((char *)"**EMSI_ACKA490\r\021");
	SM_SUCCESS;

SM_END
	Syslog('i', "RXEMSI: end");
	free(databuf);

SM_RETURN




SM_DECL(txemsi,(char *)"txemsi")
SM_STATES
	senddata,
	waitpkt,
	waitchar,
	checkpkt,
	sendack
SM_NAMES
	(char *)"senddata",
	(char *)"waitpkt",
	(char *)"waitchar",
	(char *)"checkpkt",
	(char *)"sendack"
SM_EDECL

	int		c;
	unsigned short	lcrc, rcrc;
	int		standby = 0, tries = 0;
	char		buf[13], *p;
	char		trailer[8];

	p = buf;
	memset(&buf, 0, sizeof(buf));
	strncpy(buf, intro, sizeof(buf) - 1);

SM_START(senddata)
	Syslog('i', "TXEMSI: start");

SM_STATE(senddata)

	p = mkemsidat(caller);
	PUTCHAR('*');
	PUTCHAR('*');
	PUTSTR(p);
	sprintf(trailer, "%04X\r\021", crc16xmodem(p, strlen(p)));
	PUTSTR(trailer);
	Syslog('i', "TXEMSI: send **%s%04X", p, crc16xmodem(p, strlen(p)));
	free(p);
	SM_PROCEED(waitpkt);

SM_STATE(waitpkt)

	standby = 0;
	SM_PROCEED(waitchar);

SM_STATE(waitchar)

	c = GETCHAR(8);
	if (c == TIMEOUT) {
		if (++tries > 9) {
			Syslog('+', "too many tries sending EMSI");
			SM_ERROR;
		} else {
			SM_PROCEED(senddata);
		}
	} else if (c < 0) {
		SM_ERROR;
	} else if ((c >= ' ') && (c <= '~')) {
		if (c == '*') {
			standby = 1;
			p = buf;
			*p = '\0';
		} else if (standby) {
			if ((p - buf) < (sizeof(buf) - 1)) {
				*p++ = c;
				*p = '\0';
			} if ((p - buf) >= (sizeof(buf) - 1)) {
				standby = 0;
				SM_PROCEED(checkpkt);
			}
		}
	} else switch(c) {
		case DC1:	SM_PROCEED(waitchar);
				break;
		case '\n':
		case '\r':	standby = 0;
				break;
		default:	standby = 0;
				break;
	}
	SM_PROCEED(waitchar);

SM_STATE(checkpkt)

	Syslog('i', "TXEMSI: rcvd %s", buf);
	if (strncasecmp(buf, "EMSI_DAT", 8) == 0) {
		SM_PROCEED(sendack);
	} else if (strncasecmp(buf, "EMSI_", 5) == 0) {
		lcrc = crc16xmodem(buf, 8);
		sscanf(buf + 8, "%04hx", &rcrc);
		if (lcrc != rcrc) {
			Syslog('+', "Got EMSI packet \"%s\" with bad crc: %04x/%04x", printable(buf, 0), lcrc, rcrc);
			SM_PROCEED(senddata);
		} if (strncasecmp(buf, "EMSI_REQ", 8) == 0) {
			SM_PROCEED(waitpkt);
		} if (strncasecmp(buf, "EMSI_ACK", 8) == 0) {
			SM_SUCCESS;
		} else {
			SM_PROCEED(senddata);
		}
	} else {
		SM_PROCEED(waitpkt);
	}

SM_STATE(sendack)

	Syslog('i', "TXEMSI: send **EMSI_ACKA490 (2 times)");
	PUTSTR((char *)"**EMSI_ACKA490\r\021");
	PUTSTR((char *)"**EMSI_ACKA490\r\021");
	SM_PROCEED(waitpkt);

SM_END
	Syslog('i', "TXEMSI: end");

SM_RETURN


