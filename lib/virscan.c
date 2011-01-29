/*****************************************************************************
 *
 * Purpose ...............: Scan a file for virusses
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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

#include "../config.h"
#include "mbselib.h"


extern pid_t	mypid;


/*
 * ClamAV stream check.
 * Original written bu Laurent Wacrenier as part of the
 * clamd-stream-client.
 * Returns: 0 = Ok, no virus found.
 *          1 = Virus found.
 *          2 = Internal error.
 */
int clam_stream_check(char *servname, char *servport, char *filename)
{
    struct sockaddr_in	sa_in;
    struct addrinfo	hints, *res = NULL, *p;
    int			s, ss, buf_len = 0, err;
    char		buf[1024], *buf_c, *port_s, *ipver = NULL, ipstr[INET6_ADDRSTRLEN];
    FILE		*fp;

    Syslog('f', "clam_stream_check(%s, %s, %s)", servname, servport, filename);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((err = getaddrinfo(servname, servport, &hints, &res)) != 0) {
	WriteError("getaddrinfo(%s:%s): %s\n", servname, servport, gai_strerror(err));
	return 2;
    }

    for (p = res; p != NULL; p = p->ai_next) {
	void	*addr;

	if (p->ai_family == AF_INET) {
	    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
	    addr = &(ipv4->sin_addr);
	    ipver = (char *)"IPv4";
	} else {
	    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
	    addr = &(ipv6->sin6_addr);
	    ipver = (char *)"IPv6";
	}
	inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
	Syslog('+', "Trying %s %s port %s", ipver, ipstr, servport);

	if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
	    WriteError("$socket()");
	    return 2;
	}

	if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
	    WriteError("$connect %s port %s", ipstr, servport);
	    p = p->ai_next;
	    close(s);
	} else {
	    break;
	}
    }

    if (p == NULL) {
	WriteError("unable to connect to %s", servname);
	return 2;
    }

#define COMMAND "STREAM\r\n"

    Syslog('f', "snd: %s", printable((char *)COMMAND, 0));
    if (write(s, COMMAND, sizeof(COMMAND)-1) == -1) {
	WriteError("$write()");
	return 2;
    }
    if ((buf_len = read(s, buf, sizeof(buf)-1)) == -1) {
	WriteError("$read()");
	return 2;
    }
    Syslog('f', "got: %s", printable(buf, 0));
	  
    buf[buf_len] = 0;
    if (strncasecmp(buf, "PORT ", sizeof("PORT ") -1) != 0) {
	return 2;
    }
	      
    port_s = buf + sizeof("PORT ") -1;
    while(*port_s == ' ') port_s ++;

    memcpy(&sa_in, res->ai_addr, sizeof(sa_in));
    sa_in.sin_port = htons(strtoul(port_s, NULL, 10));
    sa_in.sin_family = AF_INET;
    ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
	WriteError("$socket()");
	return 2;
    }

    sa_in.sin_port = htons(strtoul(port_s, NULL, 10));
    if (connect(ss, (struct sockaddr *)&sa_in, sizeof(struct sockaddr_in)) == -1) {
	WriteError("$connect2()");
	return 2;
    }

    if ((fp = fopen(filename, "r")) == NULL) {
	WriteError("$can't open %s", filename);
	return 2;
    }

    while ((buf_len = fread(buf, 1, sizeof(buf), fp)) > 0) {
	if (write(ss, buf, buf_len) == -1) {
	    if (errno == EPIPE)
		break;
	    WriteError("$write2()");
	    fclose(fp);
	    return 2;
	}
    }
    if (buf_len == 0 && ferror(fp)) {
	WriteError("$read2()");
	return 2;
    }
    close(ss);
    fclose(fp);

    if ((buf_len = read(s, buf, sizeof(buf)-1)) == -1) { 
	WriteError("$read3()");
	return 2;
    }
    buf[buf_len] = 0;
    Syslog('f', "res: %s", printable(buf, 0));

    /*  fwrite(buf, 1, buf_len, stderr); */
    buf_c = buf + buf_len;
    while(*buf_c=='\r' || *buf_c == '\r' || *buf_c == ' ')
	*buf_c --;
    if (buf_c - buf >= sizeof(" FOUND") && strncasecmp(buf_c - sizeof(" FOUND"), " FOUND", sizeof(" FOUND")-1) == 0) {
	char *buf_s = buf;
	buf_c -= sizeof(" FOUND");
	if (strncasecmp(buf_s, "stream:", sizeof("stream:")-1) == 0) {
	    buf_s += sizeof("stream:")-1;
	    while(*buf_s == ' ') 
		buf_s ++;
	    WriteError("ClamAV stream check, virus found: %.*s", (int)(buf_c - buf_s), buf_s);
	}
	return 1;
    }

    close(s);
    Syslog('f', "clam_stream_check(): no virus found");
    return 0;
}



/*
 * F-Prot stream check.
 * Returns: 0 = Ok, no virus found.
 *          1 = Virus found.
 *          2 = Internal error.
 *
 * telnet localhost 10200
 * SCAN FILE /path/to/file SIZE n
 * stream data
 *   --
 * receive:
 * n <descriptive text> <name of the scanned object>
 * n -= result code, 0 s clean.
 */
int fp_stream_check(char *server, char *port, char *filename)
{
    struct addrinfo	hints, *res;
    int			filesize = 0, buf_len = 0, s, err;
    char		*cmd, buf[1024], *str1, *str2;
    FILE		*fp;

    Syslog('f', "fp_stream_check(%s, %s, %s)", server, port, filename);
    filesize = file_size(filename);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((err = getaddrinfo(server, port, &hints, &res)) != 0) {
	WriteError("getaddrinfo(%s:%s): %s\n", server, port, gai_strerror(err));
	return 2;
    }

    while (res) {
	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1) {
	    WriteError("$socket()");
	    return 2;
	}

	if (connect(s, res->ai_addr, sizeof(struct sockaddr)) == -1) {
	    struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
	    WriteError("$connect(%s:%d)", inet_ntoa(sa->sin_addr), (int)ntohs(sa->sin_port));
	    res = res->ai_next;
	    close(s);
	} else {
	    break;
	}
    }

    if (res == NULL) {
	WriteError("unable to connect to %s", server);
	return 2;
    }

    cmd = calloc(PATH_MAX, sizeof(char));
    snprintf(cmd, PATH_MAX-1, "SCAN STREAM %s SIZE %d\n", filename, filesize);
    Syslog('f', "snd: %s", printable(cmd, 0));
    if (write(s, cmd, strlen(cmd)) == -1) {
	WriteError("$write()");
	return 2;
    }
    free(cmd);

    /*
     * Stream the data
     */
    if ((fp = fopen(filename, "r")) == NULL) {
	WriteError("$can't open %s", filename);
	return 2;
    }

    while ((buf_len = fread(buf, 1, sizeof(buf), fp)) > 0) {
	if (write(s, buf, buf_len) == -1) {
	    if (errno == EPIPE)
		break;
	    WriteError("$write2()");
	    fclose(fp);
	    return 2;
	}
    }
    fclose(fp);

    if ((buf_len = read(s, buf, sizeof(buf)-1)) == -1) {
	WriteError("$read()");
	return 2;
    }
    buf[buf_len] = '\0';
    Syslog('f', "got: %s", printable(buf, 0));
    cmd = xstrcpy(buf);
    str1 = strtok(cmd, "<");
    str1[strlen(str1)-1] = '\0';
    str2 = strtok(NULL, ">");
    err = atoi(str1);
    if (err) {
	// Message looks like: 'contains infected objects: EICAR_Test_File'
	WriteError("F-Prot stream check %s, rc=%d", str2, err);
	return 1;
    }

    close(s);
    Syslog('f', "fp_stream_check(): no virus found");
    return 0;
}



/*
 *  Virusscan on a file.
 */
int VirScanFile(char *filename)
{
    char    		*temp, *stdlog, *errlog, buf[256], *port;
    FILE    		*fp, *lp;
    int	    		vrc, rc = FALSE, has_scan = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/virscan.data", getenv("MBSE_ROOT"));

    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("No virus scanners defined");
	free(temp);
	return FALSE;
    }
    fread(&virscanhdr, sizeof(virscanhdr), 1, fp);

    while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
	if (virscan.available && (virscan.scantype == SCAN_EXTERN)) {
	    if (file_exist(virscan.scanner, X_OK) == 0) {
	    	has_scan = TRUE;
	    } else {
		Syslog('+', "Warning: virusscanner %s marked active but not present", virscan.comment);
	    }
	}
	if (virscan.available && (virscan.scantype != SCAN_EXTERN)) {
	    has_scan = TRUE;
	}
    }
    if (!has_scan) {
	Syslog('+', "No active virus scanners, skipping scan");
	fclose(fp);
	free(temp);
	return FALSE;
    }
    
    stdlog = calloc(PATH_MAX, sizeof(char));
    errlog = calloc(PATH_MAX, sizeof(char));
    snprintf(stdlog, PATH_MAX, "%s/tmp/stdlog%d", getenv("MBSE_ROOT"), mypid);
    snprintf(errlog, PATH_MAX, "%s/tmp/errlog%d", getenv("MBSE_ROOT"), mypid);

    fseek(fp, virscanhdr.hdrsize, SEEK_SET);
    while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
        if (virscan.available) {
	    Syslog('+', "Scanning %s with %s", filename, virscan.comment);
	    Altime(3600);
	    switch (virscan.scantype) {
		case SCAN_EXTERN:	if (file_exist(virscan.scanner, X_OK) ==0) {
					    vrc = execute_str(virscan.scanner, virscan.options, filename, (char *)"/dev/null", stdlog, errlog);
					    if (file_size(stdlog)) {
						if ((lp = fopen(stdlog, "r"))) {
						    while (fgets(buf, sizeof(buf) -1, lp)) {
							Striplf(buf);
							Syslog('+', "stdout: \"%s\"", printable(buf, 0));
						    }
						    fclose(lp);
						}
					    }
					    if (file_size(errlog)) {
						if ((lp = fopen(errlog, "r"))) {
						    while (fgets(buf, sizeof(buf) -1, lp)) {
							Striplf(buf);
							Syslog('+', "stderr: \"%s\"", printable(buf, 0));
						    }
						    fclose(lp);
						}
					    }
					    unlink(stdlog);
					    unlink(errlog);
					    if (vrc != virscan.error) {
						WriteError("Virus found by %s", virscan.comment);
						rc = TRUE;
					    }
					}
					break;
		case CLAM_STREAM:	port = calloc(21, sizeof(char));
					snprintf(port, 20, "%d", virscan.port);
					if ((clam_stream_check(virscan.host, port, filename) == 1)) {
					    rc = TRUE;
					}
					free(port);
					break;
		case FP_STREAM:		port = calloc(21, sizeof(char));
					snprintf(port, 20, "%d", virscan.port);
					if ((fp_stream_check(virscan.host, port, filename) == 1)) {
					    rc = TRUE;
					}
					free(port);
					break;
	    }

	    Altime(0);
	    Nopper();
        }
    }
    fclose(fp);

    free(temp);
    free(stdlog);
    free(errlog);
    return rc;
}


