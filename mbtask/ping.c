/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - ping functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "libs.h"
#include "../lib/structs.h"
#include "../lib/mberrors.h"
#include "taskstat.h"
#include "taskutil.h"
#include "ping.h"



/*
 *  Global variables
 */
extern struct taskrec	TCFG;			/* Task config record	*/
int     		ping_isocket;		/* Ping socket		*/
int     		icmp_errs = 0;		/* ICMP error counter	*/
extern int		internet;		/* Internet is down	*/
extern int		rescan;			/* Master rescan flag	*/
struct in_addr		paddr;			/* Current ping address	*/
extern int		T_Shutdown;		/* Program shutdown	*/
int			ping_run = FALSE;	/* Thread runnning	*/



/*
 * Internal prototypes
 */
static int      icmp4_errcmp(char *, int, struct in_addr *, char *, int, int);
unsigned short  get_rand16(void);
int             ping_send(struct in_addr);
int             ping_receive(struct in_addr);


/* 
 * different names, same thing... be careful, as these are macros... 
 */
#if defined(__FreeBSD__) || defined(__NetBSD__)
# define icmphdr   icmp
# define iphdr     ip
# define ip_saddr  ip_src.s_addr
# define ip_daddr  ip_dst.s_addr
#else
# define ip_saddr  saddr
# define ip_daddr  daddr
# define ip_hl     ihl
# define ip_p      protocol
#endif


#ifdef __linux__
# define icmp_type  type
# define icmp_code  code
# define icmp_cksum checksum
# ifdef icmp_id
#  undef icmp_id
# endif
# define icmp_id    un.echo.id
# ifdef icmp_seq
#  undef icmp_seq
# endif
# define icmp_seq   un.echo.sequence
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
# define ICMP_DEST_UNREACH   ICMP_UNREACH
# define ICMP_TIME_EXCEEDED ICMP_TIMXCEED    
#endif

#define ICMP_BASEHDR_LEN  8
#define ICMP4_ECHO_LEN    ICMP_BASEHDR_LEN


short			p_sequence = 10;
unsigned short          id;
struct icmphdr          icmpd;
struct sockaddr_in      to;



/*
 * Takes a packet as send out and a recieved ICMP packet and looks whether the ICMP packet is 
 * an error reply on the sent-out one. packet is only the packet (without IP header).
 * errmsg includes an IP header.
 * to is the destination address of the original packet (the only thing that is actually
 * compared of the IP header). The RFC sais that we get at least 8 bytes of the offending packet.
 * We do not compare more, as this is all we need.
 */
static int icmp4_errcmp(char *packet, int plen, struct in_addr *too, char *errmsg, int elen, int errtype)
{
    struct iphdr	iph;
    struct icmphdr	icmph;
    struct iphdr	eiph;
    char		*data;

    /* 
     * lots of memcpy to avoid unaligned accesses on alpha
     */
    if (elen < sizeof(struct iphdr))
	return 0;

    memcpy(&iph, errmsg, sizeof(iph));
    if (iph.ip_p != IPPROTO_ICMP || elen < iph.ip_hl * 4 + ICMP_BASEHDR_LEN + sizeof(eiph))
	return 0;

    memcpy(&icmph, errmsg + iph.ip_hl * 4, ICMP_BASEHDR_LEN);
    memcpy(&eiph, errmsg + iph.ip_hl * 4 + ICMP_BASEHDR_LEN, sizeof(eiph));
    if (elen < iph.ip_hl * 4 + ICMP_BASEHDR_LEN + eiph.ip_hl * 4 + 8)
	return 0;

    data = errmsg + iph.ip_hl * 4 + ICMP_BASEHDR_LEN + eiph.ip_hl * 4;
    return icmph.icmp_type == errtype && memcmp(&too->s_addr, &eiph.ip_daddr, sizeof(too->s_addr)) == 0 && 
	memcmp(data, packet, plen < 8 ?plen:8) == 0;
}



unsigned short get_rand16(void)
{
    return random()&0xffff;
}



/*
 * IPv4/ICMPv4 ping. Called from ping (see below)
 */
int ping_send(struct in_addr addr)
{
    int			len, sentlen;
#ifdef __linux__
    struct icmp_filter	f;
#else
    struct protoent	*pe;
    int			SOL_IP;
#endif
    unsigned long	sum;
    unsigned short	*ptr;

#ifndef __linux__
    if (!(pe = getprotobyname("ip"))) {
	Syslog('?', "icmp ping: getprotobyname() failed: %s", strerror(errno));
	return -1;
    }
    SOL_IP = pe->p_proto;
#endif

    id = (unsigned short)get_rand16(); /* randomize a ping id */

#ifdef __linux__
    /* 
     * Fancy ICMP filering -- only on Linux (as far is I know)
     *   
     * In fact, there should be macros for treating icmp_filter, but I haven't found them in Linux 2.2.15.
     * So, set it manually and unportable ;-)
     * This filter lets ECHO_REPLY (0), DEST_UNREACH(3) and TIME_EXCEEDED(11) pass.
     * !(0000 1000 0000 1001) = 0xff ff f7 f6 
     */
    f.data=0xfffff7f6;
    if (setsockopt(ping_isocket, SOL_RAW, ICMP_FILTER, &f, sizeof(f)) == -1) {
	if (icmp_errs < ICMP_MAX_ERRS)
	    Syslog('?', "$icmp ping: setsockopt() failed %d", ping_isocket);
	return -1;
    }
#endif

    icmpd.icmp_type  = ICMP_ECHO;
    icmpd.icmp_code  = 0;
    icmpd.icmp_cksum = 0;
    icmpd.icmp_id    = htons((short)id);
    icmpd.icmp_seq   = htons(p_sequence);

    /* Checksumming - Algorithm taken from nmap. Thanks... */

    ptr = (unsigned short *)&icmpd;
    sum = 0;

    for (len = 0; len < 4; len++) {
	sum += *ptr++;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    icmpd.icmp_cksum = ~sum;

    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port   = 0;
    to.sin_addr   = addr;
    SET_SOCKA_LEN4(to);

    sentlen = sendto(ping_isocket, &icmpd, ICMP4_ECHO_LEN, 0, (struct sockaddr *)&to, sizeof(to));
    if (sentlen != ICMP4_ECHO_LEN) {
	if (icmp_errs < ICMP_MAX_ERRS) {
	    if (sentlen == -1)
		Syslog('+', "ping: sent error: %s", strerror(errno));
	    else
		Syslog('+', "ping: sent %d octets, ret %d", ICMP4_ECHO_LEN, sentlen);
	}
	return -2;
    }
    return 0;
}



/*
 *  0 = reply received Ok.
 * -1 = reply packet not for us, this is Ok.
 * -2 = destination unreachable.
 * -3 = poll/select error.
 * -4 = time exceeded.
 * -5 = wrong packetlen received.
 * -6 = no data received, this is Ok.
 * -7 = icmp parameter problem.
 */
int ping_receive(struct in_addr addr)
{
    char                buf[1024]; 
    int                 rc, len;
    struct sockaddr_in	ffrom;
    struct icmphdr      icmpp;
    struct iphdr        iph;
    socklen_t           sl;        
    struct pollfd       pfd;

    pfd.fd = ping_isocket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    /*
     *  100 mSec is enough, this function is called at regular intervals.
     */
    if ((rc = poll(&pfd, 1, 100) < 0)) {
	if (icmp_errs < ICMP_MAX_ERRS)
	    Syslog('?', "$poll/select failed");
	return -3; 
    }
//    Syslog('p', "poll_thread: poll interrupted rc=%d events=%04x", rc, pfd.revents);

    if (pfd.revents & POLLIN || pfd.revents & POLLERR || pfd.revents & POLLHUP || pfd.revents & POLLNVAL) {
	sl = sizeof(ffrom);
	if ((len = recvfrom(ping_isocket, &buf, sizeof(buf)-1, 0,(struct sockaddr *)&ffrom, &sl)) != -1) {
	    if (len > sizeof(struct iphdr)) {
		memcpy(&iph, buf, sizeof(iph));
		if (len - iph.ip_hl * 4 >= ICMP_BASEHDR_LEN) {
		    memcpy(&icmpp, ((unsigned long int *)buf)+iph.ip_hl, sizeof(icmpp));
		    if (iph.ip_saddr == addr.s_addr && icmpp.icmp_type == ICMP_ECHOREPLY &&
			ntohs(icmpp.icmp_id) == id && ntohs(icmpp.icmp_seq) == p_sequence) {
			return 0;
		    } else {
			/* No regular echo reply. Maybe an error? */
			if (icmp4_errcmp((char *)&icmpd, ICMP4_ECHO_LEN, &to.sin_addr, buf, len, ICMP_DEST_UNREACH))
			    return -2;
			if (icmp4_errcmp((char *)&icmpd, ICMP4_ECHO_LEN, &to.sin_addr, buf, len, ICMP_TIME_EXCEEDED))
			    return -4;
#ifdef __linux__
			if (icmp4_errcmp((char *)&icmpd, ICMP4_ECHO_LEN, &to.sin_addr, buf, len, ICMP_PARAMETERPROB))
			    return -7;
#endif
			/*
			 * No fatal problem, the return code will be -1 caused by other
			 * icmp trafic on the network (packets not for us).
			 */
			return -1;
		    }
		}
	    }
	} else {
	    return -5; /* error */
	}
    }
    return -6; /* no answer */
}



/*
 *  Create the ping socket, called from main() during init as root.
 */
void init_pingsocket(void)
{
    if ((ping_isocket = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
	if (errno == EPERM) {
	    fprintf(stderr, "socket init failed, mbtask not installed setuid root\n");
	} else {
	    fprintf(stderr, "socket init failed\n");
	}
	exit(MBERR_INIT_ERROR);
    }

    /*
     * If someone's messing with us, bail.
     * It would be nice to issue an error message, but to where? 
     */
    if (ping_isocket == STDIN_FILENO || ping_isocket == STDOUT_FILENO || ping_isocket == STDERR_FILENO) {
	exit(MBERR_GENERAL);
    }
}



/*
 * Ping thread
 */
void *ping_thread(void)
{
    int		    rc = 0;
    static int      pingnr, pingresult[2];
    static char     pingaddress[41];
    static time_t   pingsend;
    time_t	    now;

    Syslog('+', "Starting ping thread");
    pingresult[1] = pingresult[2] = FALSE;
    pingnr = 2;
    internet = FALSE;
    ping_run = TRUE;

    while (! T_Shutdown) {

#ifdef USE_EXPERMIMENT
	Syslog('p', "ping_thread loop start");
#endif
	/*
	 * Select new address to ping
	 */
	if (pingnr == 1) {
	    pingnr = 2;
	    if (strlen(TCFG.isp_ping2)) {
		sprintf(pingaddress, "%s", TCFG.isp_ping2);
	    } else {
		pingresult[2] = FALSE;
	    }
	} else {
	    pingnr = 1;
	    if (strlen(TCFG.isp_ping1)) {
		sprintf(pingaddress, "%s", TCFG.isp_ping1);
	    } else {
		pingresult[1] = FALSE;
	    }
	}

	if (inet_aton(pingaddress, &paddr)) {
	    rc = ping_send(paddr);
#ifdef USE_EXPERIMENT
	    Syslog('p', "ping_send(%s) rc=%d", pingaddress, rc);
#endif
	    if (rc) {
		if (icmp_errs++ < ICMP_MAX_ERRS)
		    Syslog('?', "ping: to %s rc=%d", pingaddress, rc);
		pingresult[pingnr] = FALSE;
		now = time(NULL) + 10;

		while ((! T_Shutdown) && (time(NULL) < now)) {
		    sleep(1);
		}
		if (T_Shutdown)
		    break;
	    } else {
		pingsend = time(NULL);

		while (TRUE) {
		    if (T_Shutdown)
			break;

		    if (time(NULL) >= (pingsend + 20)) {
			pingresult[pingnr] = FALSE;
			if (icmp_errs < ICMP_MAX_ERRS)
			    Syslog('?', "ping: to %s timeout", pingaddress);
			break;
		    } else {
			/*
			 * Quickly eat all packets not for us, we only want our
			 * packets and empty results (packet still underway).
			 */
			while ((rc = ping_receive(paddr)) == -1);
#ifdef USE_EXPERIMENT
			Syslog('p', "ping_receive() rc=%d", rc);
#endif
			if (!rc) {
			    /*
			     * Reply received.
			     */
			    rc = time(NULL) - pingsend;
			    if (rc > 5)
				Syslog('+', "Ping: slow reply after %d seconds", rc);
			    pingresult[pingnr] = TRUE;
			    now = time(NULL) + 20 - rc;
			    while ((! T_Shutdown) && (time(NULL) < now)) {
				sleep(1);
			    }
			    break;
			} else {
			    if (rc != -6) {
				Syslog('p', "ping: recv %s id=%d rc=%d", pingaddress, id, rc);
				pingresult[pingnr] = FALSE;
			    }
			}
		    }
		} /* while TRUE */
	    }
	} else {
	    if (icmp_errs++ < ICMP_MAX_ERRS)
		Syslog('?', "Ping address %d is invalid \"%s\"", pingnr, pingaddress);
	    now = time(NULL) + 10;
	    while ((! T_Shutdown) && (time(NULL) < now)) {
		sleep(1);
	    }
	}

	if (T_Shutdown)
	    break;

	/* 
	 *  Evaluate the result of the ping test
	 */
	if (pingresult[1] == FALSE && pingresult[2] == FALSE) {
	    icmp_errs++;
	    if (internet) {
		Syslog('!', "Internet connection is down");
		internet = FALSE;
		sem_set((char *)"scanout", TRUE);
		RemoveSema((char *)"is_inet");
		rescan = TRUE;
	    }
	} else {
	    icmp_errs = 0;
	    if (!internet) {
		Syslog('!', "Internet connection is up");
		internet = TRUE;
		sem_set((char *)"scanout", TRUE);
		CreateSema((char *)"is_inet");
		rescan = TRUE;
	    }
	}
    }

    ping_run = FALSE;
    Syslog('+', "Ping thread stopped");
    pthread_exit(NULL);
}



