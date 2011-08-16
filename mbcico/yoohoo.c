/*****************************************************************************
 *
 * $Id: yoohoo.c,v 1.25 2007/08/25 18:32:08 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek		FIDO:		2:280/2802
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

/* Added modifications made on 9 Jun 1996 by P.Saratxaga
 * - added "Hello" structure (from FTS-0006) so we can more easily
 *   parse it.
 * - added domain support in hello packet (in Hello.name, reading after
 *   first \0 there is the domain)
 * - added support for 16 bit product code
 */

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "statetbl.h"
#include "ttyio.h"
#include "session.h"
#include "config.h"
#include "emsi.h"
#include "hydra.h"
#include "rdoptions.h"
#include "wazoo.h"
#include "dietifna.h"
#include "yoohoo.h"
#include "inbound.h"
#include "callstat.h"


/*------------------------------------------------------------------------*/
/* YOOHOO<tm> CAPABILITY VALUES                                           */
/*------------------------------------------------------------------------*/
#define Y_DIETIFNA 0x0001  /* Can do fast "FTS-0001"  0000 0000 0000 0001 */
#define FTB_USER   0x0002  /* Full-Tilt Boogie        0000 0000 0000 0010 */
#define ZED_ZIPPER 0x0004  /* Does ZModem, 1K blocks  0000 0000 0000 0100 */
#define ZED_ZAPPER 0x0008  /* Can do ZModem variant   0000 0000 0000 1000 */
#define DOES_IANUS 0x0010  /* Can do Janus            0000 0000 0001 0000 */
#define DOES_HYDRA 0x0020  /* Can do Hydra            0000 0000 0010 0000 */
#define Bit_6      0x0040  /* reserved by FTSC        0000 0000 0100 0000 */
#define Bit_7      0x0080  /* reserved by FTSC        0000 0000 1000 0000 */
#define Bit_8      0x0100  /* reserved by FTSC        0000 0001 0000 0000 */
#define Bit_9      0x0200  /* reserved by FTSC        0000 0010 0000 0000 */
#define Bit_a      0x0400  /* reserved by FTSC        0000 0100 0000 0000 */
#define Bit_b      0x0800  /* reserved by FTSC        0000 1000 0000 0000 */
#define Bit_c      0x1000  /* reserved by FTSC        0001 0000 0000 0000 */
#define Bit_d      0x2000  /* reserved by FTSC        0010 0000 0000 0000 */
#define DO_DOMAIN  0x4000  /* Packet contains domain  0100 0000 0000 0000 */
#define WZ_FREQ    0x8000  /* WZ file req. ok         1000 0000 0000 0000 */

#define LOCALCAPS (Y_DIETIFNA|ZED_ZIPPER|ZED_ZAPPER|DOES_HYDRA|DO_DOMAIN)


static int	rxyoohoo(void);
static int	txyoohoo(void);
static void	fillhello(unsigned short,char*);
static int	checkhello(void);

static int	y_akas;
static int	iscaller;

static struct _hello {
	unsigned char data[128];
	unsigned char crc[2];
} hello;

typedef struct   _Hello {
    unsigned short signal;         /* always 'o'     (0x6f)                  */
    unsigned short hello_version;  /* currently 1    (0x01)                  */
    unsigned short product;        /* product code                           */
    unsigned short product_maj;    /* major revision of the product          */
    unsigned short product_min;    /* minor revision of the product          */
    unsigned char  my_name[60];    /* Other end's name, will include domain  */
                                   /* if DO_DOMAIN is set in capabilities    */
    unsigned char  sysop[20];      /* sysop's name                           */
    unsigned short my_zone;        /* 0== not supported                      */
    unsigned short my_net;         /* out primary net number                 */
    unsigned short my_node;        /* our primary node number                */
    unsigned short my_point;       /* 0 == not supported                     */
    unsigned char  my_password[8]; /* This isn't necessarily null-terminated */
    unsigned char  reserved2[8];   /* reserved by Opus                       */
    unsigned short capabilities;   /* see below                              */
    unsigned char  reserved3[12];  /* for non-Opus systems with "approval"   */
                                   /*         total size 128 bytes           */
} Hello;


extern int	Loaded;
extern pid_t	mypid;
extern int	laststat;
extern int	session_state;


Hello hello2;
Hello gethello2(unsigned char[]);



int rx_yoohoo(void)
{
    int		    rc, protect = FALSE;
    unsigned short  capabilities,localcaps;
    char	    *pwd = NULL;
    callstat	    *cst;

    pwd = NULL;
    y_akas = 0;
    localcaps = LOCALCAPS;
    if (localoptions & NOZMODEM) localcaps &= ~(ZED_ZAPPER|ZED_ZIPPER);
    if (localoptions & NOZEDZAP) localcaps &= ~ZED_ZAPPER;
    if (localoptions & NOHYDRA)  localcaps &= ~DOES_HYDRA;
    emsi_local_opts = 0;
    emsi_remote_opts = 0;
    iscaller = 0;

    if ((rc = rxyoohoo()) == 0) {
	Loaded = checkhello();

	if (y_akas == 0) {
	    Syslog('+', "All akas busy, abort");
	    return MBERR_SESSION_ERROR;
	}

	capabilities = hello2.capabilities;
	if (capabilities & WZ_FREQ) 
	    session_flags |= SESSION_WAZOO;
	else 
	    session_flags &= ~SESSION_WAZOO;
	localcaps &= capabilities;
	if (localcaps & DOES_HYDRA) 
	    localcaps &= DOES_HYDRA;
	else if (localcaps & ZED_ZAPPER) 
	    localcaps &= ZED_ZAPPER;
	else if (localcaps & ZED_ZIPPER) 
	    localcaps &= ZED_ZIPPER;
	else if (localcaps & FTB_USER)   
	    localcaps &= FTB_USER;
	else if (localcaps & Y_DIETIFNA) 
	    localcaps &= Y_DIETIFNA;
	if ((localoptions & NOFREQS) == 0) 
	    localcaps |= WZ_FREQ;
	else 
	    emsi_local_opts |= OPT_NRQ;

	if (((nlent=getnlent(remote->addr))) && (nlent->pflag != NL_DUMMY)) {
	    Syslog('+', "Remote is a listed system");
	    strncpy(history.location, nlent->location, 35);
	    UserCity(mypid, nlent->sysop, nlent->location);
	}
	cst = getstatus(remote->addr);
	if (cst->trystat)
	    laststat = cst->trystat;
	Syslog('s', "Last session status %d", laststat);

	if (nlent) 
	    rdoptions(Loaded);

	if (strlen(nodes.Spasswd)) {
	    if ((strncasecmp((char*)hello2.my_password, nodes.Spasswd, strlen(nodes.Spasswd)) == 0) &&
		(strlen((char*)hello2.my_password) == strlen(nodes.Spasswd))) {
		Syslog('+', "Password correct, protected mail session");
		protect = TRUE;
		pwd = xstrcpy(nodes.Spasswd);
	    } else {
		if (pwd)
		    free(pwd);
		pwd = xstrcpy((char *)"BAD_PASS");
		Syslog('?', "Remote password \"%s\", expected \"%s\"", (char*)hello2.my_password, nodes.Spasswd);
		localcaps = 0;
	    }
	} else
	    Syslog('s', "No YooHoo password check");
	
	inbound_open(remote->addr, protect, FALSE);

	fillhello(localcaps,pwd);
	
	rc = txyoohoo();
	if (pwd)
	    free(pwd);
    }

    if ((rc == 0) && ((localcaps & LOCALCAPS) == 0)) {
	Syslog('+', "No common protocols or bad password");
	return 0;
    }
    if (rc) 
	return MBERR_YOOHOO;

    IsDoing("Inbound %s", ascfnode(remote->addr, 0x0f));

    if (protect)
	session_state = STATE_SECURE;
    else
	session_state = STATE_UNSECURE;

    session_flags |= SESSION_WAZOO;
    if (localcaps & DOES_HYDRA) 
	return hydra(0);
    else if ((localcaps & ZED_ZAPPER) || (localcaps & ZED_ZIPPER)) {
	if (localcaps & ZED_ZAPPER) 
	    emsi_local_protos = PROT_ZAP;
	else 
	    emsi_local_protos = PROT_ZMO;
	return rxwazoo();
    } else if (localcaps & Y_DIETIFNA) 
	return rxdietifna();
	
    WriteError("YooHoo internal error - no proto for 0x%04xh",localcaps);
    return MBERR_YOOHOO;
}



int tx_yoohoo(void)
{
    int		    rc;
    unsigned short  capabilities;
    char	    *pwd;

    y_akas = 0;
    if (strlen(nodes.Spasswd))
	pwd = xstrcpy(nodes.Spasswd);
    else
	pwd = NULL;

    capabilities = LOCALCAPS;
    if (localoptions & NOZMODEM) 
	capabilities &= ~(ZED_ZAPPER|ZED_ZIPPER);
    if (localoptions & NOZEDZAP) 
	capabilities &= ~ZED_ZAPPER;
    if (localoptions & NOHYDRA) 
	capabilities &= ~DOES_HYDRA;
    if ((localoptions & NOFREQS) == 0) 
	capabilities |= WZ_FREQ;
    else 
	emsi_local_opts |= OPT_NRQ;

    fillhello(capabilities,pwd);
    iscaller=1;

    if ((rc = txyoohoo()) == 0) {
	rc = rxyoohoo();
	checkhello();
	capabilities = hello2.capabilities;
	if (capabilities & WZ_FREQ) 
	    session_flags |= SESSION_WAZOO;
	else 
	    session_flags &= ~SESSION_WAZOO;
    }

    if (y_akas == 0) {
	Syslog('+', "All akas busy, abort");
	return MBERR_SESSION_ERROR;
    }
    if ((rc == 0) && ((capabilities & LOCALCAPS) == 0)) {
	Syslog('+', "No common protocols");
	return MBERR_SESSION_ERROR;
    }

    if (rc) 
	return MBERR_YOOHOO;

    IsDoing("Outbound %s", ascfnode(remote->addr, 0x0f));
    session_state = STATE_SECURE;

    session_flags |= SESSION_WAZOO;
    if (capabilities & DOES_HYDRA) 
	return hydra(1);
    else if ((capabilities & ZED_ZAPPER) || (capabilities & ZED_ZIPPER)) {
	if (capabilities & ZED_ZAPPER) 
	    emsi_local_protos = PROT_ZAP;
	else 
	    emsi_local_protos = PROT_ZMO;
	return txwazoo();
    } else if (capabilities & Y_DIETIFNA) 
	return txdietifna();
		
    WriteError("YooHoo internal error - no proto for 0x%04xh",capabilities);
    return MBERR_YOOHOO;
}



SM_DECL(rxyoohoo,(char *)"rxyoohoo")
SM_STATES
    sendenq,
    waitchar,
    getpacket,
    sendnak,
    sendack
SM_NAMES
    (char *)"sendenq",
    (char *)"waitchar",
    (char *)"getpacket",
    (char *)"sendnak",
    (char *)"sendack"
SM_EDECL

    int		    c, count=0;
    unsigned short  lcrc, rcrc;

SM_START(sendenq)

SM_STATE(sendenq)

    if (count++ > 12) {
	Syslog('+', "Too many tries to get YooHoo hello packet");
	SM_ERROR;
    }
    PUTCHAR(ENQ);
    SM_PROCEED(waitchar)

SM_STATE(waitchar)

    c=GETCHAR(10);
    if (c == TIMEOUT) {
	SM_PROCEED(sendenq);
    } else if (c < 0) {
	SM_ERROR;
    } else switch (c) {
	case 0x1f:	SM_PROCEED(getpacket); 
			break;
	case YOOHOO:	SM_PROCEED(sendenq); 
			break;
	default:	SM_PROCEED(waitchar);
			break;
    }

SM_STATE(getpacket)

    GET((char*)&hello, sizeof(hello), 30);
    if (STATUS) {
	SM_ERROR;
    }

    lcrc = crc16xmodem((char*)hello.data, sizeof(hello.data));
    rcrc = (hello.crc[0] << 8) + hello.crc[1];
    if (lcrc != rcrc) {
	Syslog('+',"crc does not match in YooHoo hello packet: %04xh/%04xh", rcrc,lcrc);
	SM_PROCEED(sendnak);
    } else {
	SM_PROCEED(sendack);
    }

SM_STATE(sendnak)

    if (count++ > 9) {
	Syslog('+', "Too many tries to get YooHoo hello packet");
	SM_ERROR;
    }
    PUTCHAR('?');
    SM_PROCEED(waitchar);

SM_STATE(sendack)

    PUTCHAR(ACK);
    SM_SUCCESS;

SM_END
SM_RETURN



SM_DECL(txyoohoo,(char *)"txyoohoo")
SM_STATES
    sendyoohoo,
    waitenq,
    sendpkt,
    waitchar
SM_NAMES
    (char *)"sendyoohoo",
    (char *)"waitenq",
    (char *)"sendpkt",
    (char *)"waitchar"
SM_EDECL

    int		    c, count = 0, startstate;
    unsigned short  lcrc;

    if (iscaller) 
	startstate = sendpkt;
    else 
	startstate = sendyoohoo;

    lcrc = crc16xmodem((char*)hello.data, sizeof(hello.data));
    hello.crc[0] = lcrc >> 8;
    hello.crc[1] = lcrc & 0xff;

SM_START(startstate)

SM_STATE(sendyoohoo)

    PUTCHAR(YOOHOO);
    SM_PROCEED(waitenq);

SM_STATE(waitenq)

    c=GETCHAR(10);
    if (c == TIMEOUT) {
	if (count++ > 9) {
	    Syslog('+', "Timeout waiting ENQ");
	    SM_ERROR;
	} else {
	    SM_PROCEED(sendyoohoo);
	}
    } else if (c < 0) {
	SM_ERROR;
    } else switch (c) {
	case ENQ:	SM_PROCEED(sendpkt);
	case YOOHOO:	SM_PROCEED(waitenq);
	default:	Syslog('+',"Got '%s' waiting hello ACK", printablec(c));
			SM_PROCEED(waitchar);
			break;
    }

SM_STATE(sendpkt)

    if (count++ > 9) {
	Syslog('+', "Too many tries to send YooHoo hello packet");
	SM_ERROR;
    }

    PUTCHAR(0x1f);
    PUT((char*)&hello, sizeof(hello));
    if (STATUS) {
	SM_ERROR;
    }
    SM_PROCEED(waitchar);

SM_STATE(waitchar)

    c=GETCHAR(30);
    if (c == TIMEOUT) {
	Syslog('+', "Timeout waiting hello ACK");
	SM_ERROR;
    } else if (c < 0) {
	SM_ERROR;
    } else switch (c) {
	case ACK:   SM_SUCCESS; 
		    break;
	case '?':   SM_PROCEED(sendpkt); 
		    break;
	case ENQ:   SM_PROCEED(sendpkt); 
		    break;
	default:    SM_PROCEED(waitchar);
		    break;
    }

SM_END
SM_RETURN



void fillhello(unsigned short capabilities, char *password)
{
    faddr	    *best;
    unsigned short  majver, minver;

    best = bestaka_s(remote->addr);
    sscanf(VERSION,"%hd.%hd", &majver, &minver);
    memset(&hello, 0, sizeof(hello));

    hello.data[0] = 'o';					    /* signal		*/
    hello.data[2] = 1;						    /* hello-version	*/
    hello.data[4] = (PRODCODE & 0x00ff);			    /* product code	*/
    hello.data[5] = (PRODCODE & 0xff00) >> 8;			    /* product code	*/
    hello.data[6] = (VERSION_MAJOR & 0x00ff);			    /* prod-ver-major	*/
    hello.data[7] = (VERSION_MAJOR & 0xff00) >> 8;		    /* prod-ver-major	*/
    hello.data[8] = (VERSION_MINOR & 0x00ff);			    /* prod-ver-minor	*/
    hello.data[9] = (VERSION_MINOR & 0xff00) >> 8;		    /* prod-ver-minor	*/
    strncpy((char*)hello.data+10, name?name:"Unavailable",59);	    /* name		*/
    if (name) {
	hello.data[10+strlen(name)] = '\0';
	strncpy((char*)hello.data+11 + strlen(name),
			best->domain, 58-strlen(name));		    /* domain		*/
    } else 
	strncpy((char*)hello.data + 22, best->domain, 47);	    /* domain		*/
    strncpy((char*)hello.data+70, CFG.sysop_name,19);		    /* sysop		*/
    hello.data[90] = best->zone&0xff;				    /* zone		*/
    hello.data[91] = best->zone>>8;				    /* zone		*/
    hello.data[92] = best->net&0xff;				    /* net		*/
    hello.data[93] = best->net>>8;				    /* net		*/
    hello.data[94] = best->node&0xff;				    /* node		*/
    hello.data[95] = best->node>>8;				    /* node		*/
    hello.data[96] = best->point&0xff;				    /* point		*/
    hello.data[97] = best->point>>8;				    /* point		*/

    if (password) 
	strncpy((char*)hello.data + 98, password, 8);

    hello.data[114] = capabilities & 0xff;			    /* capabilities	*/
    hello.data[115] = capabilities >> 8;			    /* capabilities	*/
    tidy_faddr(best);
}



int checkhello(void)
{
    unsigned short  i, majver = 0, minver = 0;
    fa_list	    **tmpl, *tmpa;
    faddr	    remaddr;
    char	    *prodnm, *q;
    int		    loaded = FALSE;

    Syslog('s',"check hello \"%s\"",printable((char*)hello.data,128));
    hello2 = gethello2(hello.data);

    if ((hello2.signal != 0x6f) || (hello2.hello_version != 0x01)) {
	Syslog('+', "Got \"%s\" instead of \"o\\000\\001\000\"", printable((char*)hello.data,3));
    }

    prodnm = xstrcpy((char *)"<unknown program>");
    for (i = 0; ftscprod[i].name; i++) {
	if (ftscprod[i].code == hello2.product) {
	    free(prodnm);
	    prodnm = xstrcpy(ftscprod[i].name);
	    majver = hello2.product_maj;
	    minver = hello2.product_min;
	    break;
	}
    }

    remaddr.zone  = hello2.my_zone;
    remaddr.net   = hello2.my_net;
    remaddr.node  = hello2.my_node;
    remaddr.point = hello2.my_point;
    remaddr.name = NULL;
    remaddr.domain = NULL;
    if (hello2.my_name[0])
	remaddr.domain = (char *)hello2.my_name + (strlen((char *)hello2.my_name)) + 1;
    if (remaddr.domain[0]) {
	if ((q = strchr(remaddr.domain, '.')))
	    *q = '\0';
    } else {
	remaddr.domain = NULL;
    }
    if (remote)
	Syslog('s',"Remote known     address: %s",ascfnode(remote->addr,0x1f));

    for (tmpl = &remote; *tmpl; tmpl = &((*tmpl)->next));
    if ((remote == NULL) || (metric(remote->addr, &remaddr) != 0)) {
	(*tmpl) = (fa_list*)malloc(sizeof(fa_list));
	(*tmpl)->next = NULL;
	(*tmpl)->addr = (faddr*)malloc(sizeof(faddr));
	(*tmpl)->addr->zone   = remaddr.zone;
	(*tmpl)->addr->net    = remaddr.net;
	(*tmpl)->addr->node   = remaddr.node;
	(*tmpl)->addr->point  = remaddr.point;
	(*tmpl)->addr->domain = xstrcpy(remaddr.domain);
	(*tmpl)->addr->name   = NULL; /* Added 15-Dec-1998 */
    } else {
	tmpl=&remote;
	Syslog('s',"Using single remote address");
    }

    tmpl = &remote;
    while (*tmpl) {
	if (nodelock((*tmpl)->addr, mypid)) {
	    Syslog('+', " address: %s is locked, removed from aka list",ascfnode((*tmpl)->addr,0x1f));
	    tmpa = *tmpl;
	    *tmpl = (*tmpl)->next;
	    tidy_faddr(tmpa->addr);
	    free(tmpa);
	} else {
	    Syslog('+', " address: %s",ascfnode((*tmpl)->addr,0x1f));
	    y_akas++;
	    /*
	     * With the loaded flag we prevent removing the noderecord
	     * when the remote presents us an address we don't know about.
	     */
	    if (!Loaded) {
		if (noderecord((*tmpl)->addr))
		    Loaded = TRUE;
	    }
	    tmpl = &((*tmpl)->next);
	}
    }

    if (y_akas) {   /* Only set if we have aka's left */
	history.aka.zone  = remote->addr->zone;
	history.aka.net   = remote->addr->net;
	history.aka.node  = remote->addr->node;
	history.aka.point = remote->addr->point;
    }

    if (hello2.product < 0x0100)
	Syslog('+', "    uses: %s [%02X] version %d.%d", prodnm, hello2.product, majver, minver);
    else	
	Syslog('+', "    uses: %s [%04X] version %d.%d", prodnm, hello2.product, majver, minver);
    Syslog('+', "  system: %s",(char*)hello2.my_name);
    strncpy(history.system_name, (char *)hello2.my_name, 35);
    Syslog('+', "   sysop: %s",(char*)hello2.sysop);
    strncpy(history.sysop, (char *)hello2.sysop, 35);
    snprintf(history.location, 10, "Somewhere");

    free(prodnm);
    return loaded;
}



Hello gethello2(unsigned char Hellop[])
{
    int	    i;
    Hello   p;

    p.signal=Hellop[0]+(Hellop[1]<<8);
    p.hello_version=Hellop[2]+(Hellop[3]<<8);
    p.product=Hellop[4]+(Hellop[5]<<8);
    p.product_maj=Hellop[6]+(Hellop[7]<<8);
    p.product_min=Hellop[8]+(Hellop[9]<<8);
    for (i=0;i<60;i++)
        p.my_name[i]=Hellop[10+i];
    for (i=0;i<20;i++)
        p.sysop[i]=Hellop[70+i];
    p.my_zone=Hellop[90]+(Hellop[91]<<8);
    p.my_net=Hellop[92]+(Hellop[93]<<8);
    p.my_node=Hellop[94]+(Hellop[95]<<8);
    p.my_point=Hellop[96]+(Hellop[97]<<8);
    for (i=0;i<8;i++)
        p.my_password[i]=Hellop[98+i];
    for (i=0;i<8;i++)
        p.reserved2[i]=Hellop[106+i];
    p.capabilities=Hellop[114]+(Hellop[115]<<8);
    for (i=0;i<12;i++)
        p.reserved3[i]=Hellop[116+i];

    return p;
}

