/*****************************************************************************
 *
 * $Id: client.c,v 1.17 2007/08/25 12:19:44 mbse Exp $
 * Purpose ...............: MBSE Deamon Client
 *
 *****************************************************************************
 * Copyright (C) 1993-2006
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
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "mbselib.h"


static int		sock = -1;	/* Unix Datagram socket		*/
struct sockaddr_un      clntaddr;       /* Client socket address        */
struct sockaddr_un      servaddr;       /* Server socket address        */
struct sockaddr_un      from;           /* From socket address          */
socklen_t		fromlen;
static char		*myname='\0';	/* my program name		*/
char			spath[108];     /* Server socket path           */
char			cpath[108];     /* Client socket path           */


/************************************************************************
 *
 * Connect to Unix Datagram socket, return -1 if error or socket no. 
 */

int socket_connect(char *user, char *prg, char *city)
{
    int 	s;
    static char	buf[SS_BUFSIZE], tty[18];
    char	*tmp, *u, *p, *c;

    myname = prg;

    /*
     * Create Unix Datagram socket for the client.
     */
    s = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s == -1) {
	perror(myname);
	printf("Unable to create Unix Datagram socket\n");
	return -1;
    }

    /*
     * Client will bind to an address so the server will get
     * an address in its recvfrom call and use it to send
     * data back to the client.
     */
    memset(&clntaddr, 0, sizeof(clntaddr));
    clntaddr.sun_family = AF_UNIX;
    strcpy(clntaddr.sun_path, cpath);

    if (bind(s, (struct sockaddr *)&clntaddr, sizeof(clntaddr)) < 0) {
	close(s);
	perror(myname);
	printf("Can't bind socket %s\n", cpath);
	return -1;
    }

    /*
     * If running seteuid as another user, chown to mbse.bbs
     */
    if (getuid() != geteuid()) {
	chown(cpath, getuid(), getgid());
    } else {
	chmod(cpath, 0775);
    }

    /*
     * Setup address structure for the server socket.
     */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, spath);
	
    /*
     * Now that we have an connection, we gather 
     * information to tell the server who we are.
     */
    if (isatty(1) && (ttyname(1) != NULL)) {
	strncpy(tty, ttyname(1), sizeof(tty)-1);
	if (strchr(tty, 'p'))
	    memccpy(tty, index(tty, 'p'), '\0', strlen(tty));
	else if (strchr(tty, 't'))
	    memccpy(tty, index(tty, 't'), '\0', strlen(tty));
	else if (strchr(tty, 'c'))
	    memccpy(tty, index(tty, 'c'), '\0', strlen(tty));
    } else {
	strcpy(tty, "-");
    }
    sock = s;
	
    /*
     * Send the information to the server. 
     */
    u = xstrcpy(clencode(user));
    p = xstrcpy(clencode(prg));
    c = xstrcpy(clencode(city));
    snprintf(buf, SS_BUFSIZE, "AINI:5,%d,%s,%s,%s,%s;", getpid(), tty, u, p, c);
    free(c);
    free(p);
    free(u);
    if (socket_send(buf) != 0) {
	sock = -1;
	return -1;
    }

    strcpy(buf, socket_receive());
    if (strncmp(buf, "100:1,", 6) != 0) {
	printf("AINI not acknowledged by the server\n");
	sock = -1;
	return -1;
    }

    /*
     * Extract nodenumber from the reply.
     */
    tmp = strtok(buf, ",");
    tmp = strtok(NULL, ";");
    iNode = atoi(tmp);

    return s;
}



/*
 * Send data via internet domain socket
 */
int socket_send(char *buf)
{
    if (sock == -1)
	return -1;

    if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) != strlen(buf)) {
	printf("Socket send failed error %d\n", errno);
	if (errno == 2) {
	    abort();
	}
	return -1;
    }
    return 0;
}



/*
 * Return an empty buffer if somthing went wrong, else the complete
 * dataline is returned.
 */
char *socket_receive(void)
{
    static char	buf[SS_BUFSIZE];
    ssize_t	rlen;

    memset((char *)&buf, 0, SS_BUFSIZE);
    fromlen = sizeof(from);
    rlen = recvfrom(sock, buf, SS_BUFSIZE, 0, (struct sockaddr *)&from, &fromlen);
    if (rlen == -1) {
        perror("recv");
        printf("Error reading socket\n");
        memset((char *)&buf, 0, SS_BUFSIZE);
        return buf;
    }
    return buf;
}



/***************************************************************************
 *
 *  Shutdown the socket, first send the server the close command so this
 *  application will be removed from the servers active clients list.
 *  There must be a parameter with the pid so that client applications
 *  where the shutdown will be done by a child process is able to give
 *  the parent pid as an identifier.
 */

int socket_shutdown(pid_t pid)
{
    static char	buf[SS_BUFSIZE];

    if (sock == -1)
	return 0;

    snprintf(buf, SS_BUFSIZE, "ACLO:1,%d;", pid);
    if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
	if (strncmp(buf, "107:0;", 6) != 0) {
	    printf("Shutdown not acknowledged by the server\n");
	    printf("Got \"%s\"\n", buf);
	}
    }
	
    if (shutdown(sock, 1) == -1) {
	perror(myname);
	printf("Cannot shutdown socket\n");
	return -1;
    }

    sock = -1;
    return 0;
}


