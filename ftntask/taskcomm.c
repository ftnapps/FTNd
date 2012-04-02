/*****************************************************************************
 *
 * Purpose ...............: MBSE BBS Daemon
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "taskstat.h"
#include "taskregs.h"
#include "taskdisk.h"
#include "taskinfo.h"
#include "taskutil.h"
#include "taskchat.h"
#include "taskcomm.h"
#include "taskibc.h"


extern int			oserr;		/* Copy of Unix error	*/
extern int			sock;		/* Server socket	*/
extern struct sockaddr_un	from;		/* From socket address	*/
extern int			fromlen;	/* From address length	*/
extern int			logtrans;	/* Log transactions	*/



/************************************************************************
 *
 *  Logging procedures.
 */

int userlog(char *);
int userlog(char *param)
{
    char	*prname, *prpid, *grade, *msg;
    static char	lfn[PATH_MAX], token[14];
    int		rc;

    lfn[0] = '\0';
    strncpy(token, strtok(param, ","), 14);
    strncpy(token, strtok(NULL, ","), 14);
    snprintf(lfn, PATH_MAX, "%s/log/%s", getenv("MBSE_ROOT"), token);
    prname = strtok(NULL, ",");
    prpid  = strtok(NULL, ",");
    grade  = strtok(NULL, ",");
    msg    = xstrcpy(cldecode(strtok(NULL, ";")));
    rc = ulog(lfn, grade, prname, prpid, msg);
    free(msg);
    return rc;
}



/*
 * Process command received from the client.
 */
char *exe_cmd(char *);
char *exe_cmd(char *in)
{
    static char	obuf[SS_BUFSIZE];
    static char	ibuf[SS_BUFSIZE];
    static char	cmd[4];
    static char	token[SS_BUFSIZE];
    static char	ebuf[19];
    static char	var1[16];
    int		result;
    char	*buf;

    strncpy(ibuf, in, SS_BUFSIZE);
    strncpy(cmd, ibuf, 4);
    strncpy(ebuf, "200:1,Syntax error;", 19);

    /*
     * Split the commandline after the colon so we can give the
     * options directly to the actual functions. Also set a default
     * and most used answer.
     */
    strncpy(token, &ibuf[5], SS_BUFSIZE);
    strncpy(obuf, "100:0;", SS_BUFSIZE);


    /*
     * The A(counting) commands.
     *
     *  AINI:5,pid,tty,user,program,city;
     *  100:1,linenr;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "AINI", 4) == 0) {
	if ((result = reg_newcon(token)) != -1) {
	    snprintf(obuf, SS_BUFSIZE, "100:1,%d;", result);
	    return obuf;
	} else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ACLO:1,pid;
     *  107:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd ,"ACLO", 4) == 0) {
	if (reg_closecon(token) == 0) {
	    strcpy(obuf, "107:0;");
	    return obuf;
	} else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ADOI:2,pid,doing;
     *  100:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "ADOI", 4) == 0) {
	if (reg_doing(token) == 0)
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ATCP:1,pid;
     *  100:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "ATCP", 4) == 0) {
	if (reg_ip(token) == 0)
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ATTY:2,pid,tty;
     *  100:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "ATTY", 4) == 0) {
	if (reg_tty(token) == 0)
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ALOG:5,file,program,pid,grade,text;
     *  100:0;
     *  201:1,errno;
     */
    if (strncmp(cmd, "ALOG", 4) == 0) {
	if (userlog(token) != 0) 
	    snprintf(obuf, SS_BUFSIZE, "201:1,%d;", oserr);
	return obuf;
    }

    /*
     *  AUSR:3,pid,user,city;
     *  100:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "AUSR", 4) == 0) {
	if (reg_user(token) == 0) 
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ADIS:2,pid,flag; (set Do Not Disturb).
     *  100:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "ADIS", 4) == 0) {
	if (reg_silent(token) == 0)
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ATIM:2,pid,seconds;
     *  100:0;
     *  200:1,Syntax Error;
     */
    if (strncmp(cmd, "ATIM", 4) == 0) {
	if (reg_timer(TRUE, token) == 0)
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  ADEF:1,pid;
     *  100:0;
     */
    if (strncmp(cmd, "ADEF", 4) == 0) {
	if (reg_timer(FALSE, token) == 0)
	    return obuf;
	else {
	    stat_inc_serr();
	    return ebuf;
	}
    }

    /*
     *  Check for personal message
     *
     *  CIPM:1,pid;  (Is personal message present)
     *  100:2,fromname,message;
     *  100:0;
     */
    if (strncmp(cmd, "CIPM", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	reg_ipm_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * CSPM:3,fromuser,touser,text; (Send personal message).
     * 100:1,n;  n: 1=donotdisturb 2=buffer full 3=error
     * 100:0;
     */
    if (strncmp(cmd, "CSPM", 4) == 0) {
	if ((result = reg_spm(token)))
	    snprintf(obuf, SS_BUFSIZE, "100:1,%d;", result);
	return obuf;
    }

    /*
     * CSYS:2,pid,1;    Sysop available for chat (from mbmon)
     * CSYS:2,pid,0;    Sysop goes away (from mbmon)
     * 100:0;		Allways Ok.
     */
    if (strncmp(cmd, "CSYS", 4) == 0) {
	reg_sysop(token);
	return obuf;
    }

    /*
     * CPAG:2,pid,reason;   Page sysop for a chat
     * 100:1,n;		    1=busy, 2=sysop not available, 3=error
     * 100:0;		    Ok
     */
    if (strncmp(cmd, "CPAG", 4) == 0) {
	if ((result = reg_page(token))) {
	    snprintf(obuf, SS_BUFSIZE, "100:1,%d;", result);
	}
	return obuf;
    }

    /*
     * CCAN:1,pid;	    Cancel sysop page
     * 100:0;		    Always Ok
     */
    if (strncmp(cmd, "CCAN", 4) == 0) {
	reg_cancel(token);
	return obuf;
    }

    /*
     * Check for sysop page (from mbmon)
     *
     * CCKP:0;
     * 100:3,pid,1,reason;  Page is active
     * 100:3,pid,0,reason;  Page is canceled, but user still online
     * 100:0;		    No page active
     */
    if (strncmp(cmd, "CCKP", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	reg_checkpage_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * Check for sysop in chatmode for forced sysop chat
     *
     * CISC:1,pid;
     * 100:1,1;		    Yes (and drop into chatmode)
     * 100:1,0;		    No
     */
    if (strncmp(cmd, "CISC", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	chat_checksysop_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * Connect to chatserver
     *
     * CCON:4,pid,username,unixname,n;	Connect to chatserver with username, n=1 user is the sysop
     * 100:1,error;		If error
     * 100:0;			Ok
     */
    if (strncmp(cmd, "CCON", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
        chat_connect_r(token, buf);
        snprintf(obuf, SS_BUFSIZE, "%s", buf);
        free(buf);
        return obuf;
    }
    
    /*
     * Close chat session
     *
     * CCLO:1,pid;	    Leave chatserver
     * 100:1,error;	    Error
     * 100:0;		    Ok
     */
    if (strncmp(cmd, "CCLO", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	chat_close_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * Put message on server
     *
     * CPUT:2,pid,message;  Put message on server
     * 100:2,0,error;	    Error, not fatal and continue chat
     * 100:2,1,error;	    Error, fatal and disconnect
     * 100:0;		    Ok
     */
    if (strncmp(cmd, "CPUT", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	chat_put_r(token, buf);
	strncpy(obuf, buf, SS_BUFSIZE);
	free(buf);
	return obuf;
    }

    /*
     * Get message from server
     *
     * CGET:1,pid;	    Get message from server
     * 100:2,0,message;	    If message present
     * 100:2,1,error;	    Error and disconnect
     * 100:0;		    No message
     */
    if (strncmp(cmd, "CGET", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	chat_get_r(token, buf);
	strncpy(obuf, buf, SS_BUFSIZE);
	free(buf);
	return obuf;
    }

    /*
     * The D(isk) commands.
     */

    /*
     *  DRES:0;             Reset and reread disk tables.
     *  100:0;		    Always Ok.
     */
    if (strncmp(cmd, "DRES", 4) == 0) {
	return disk_reset();
    }
    
    /*
     *  DSPC:1,n;           Check free space in MBytes
     *  100:2,0,n;          No, n = lowest size in MBytes
     *  100:2,1,n;          Yes, n = lowest size in MBytes
     *  100:1,2;            Unknown
     *  100:1,3;            Error
     */
    if (strncmp(cmd, "DSPC", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	disk_check_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }
    
    /*
     *  DGFS:0;		    Get filesystem status.
     *  100:n,data1,..,data10;
     */
    if (strncmp(cmd, "DGFS", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	disk_getfs_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }
    
    /*
     * The G(lobal) commands.
     *
     *  GNOP:1,pid;
     *  100:0;
     */
    if (strncmp(cmd ,"GNOP", 4) == 0) {
	reg_nop(token);
	return obuf;
    }

    /*
     *  GPNG:n,data;
     *  100:n,data;
     */
    if (strncmp(cmd, "GPNG", 4) == 0) {
	snprintf(obuf, SS_BUFSIZE, "100:%s", token);
	return obuf;
    }

    /*
     *  GVER:0;
     *  100:1,Version ...;
     */
    if (strncmp(cmd, "GVER", 4) == 0) {
	snprintf(obuf, SS_BUFSIZE, "100:1,Version %s;", VERSION);
	return obuf;
    }

    /*
     *  GSTA:0;
     *  100:19,start,laststart,daily,startups,clients,tot_clients,tot_peak,tot_syntax,tot_comerr,
     *         today_clients,today_peak,today_syntax,today_comerr,!BBSopen,ZMH,internet,Processing,Load,sequence;
     *  201:1,16;
     */
    if (strncmp(cmd, "GSTA", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	stat_status_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  GMON:1,n;  n=1 First time
     *  100:7,pid,tty,user,program,city,isdoing,starttime;
     *  100:0;
     */
    if (strncmp(cmd, "GMON", 4) == 0) {
	strtok(token, ",");
	strcpy(var1, strtok(NULL, ";"));
	buf = calloc(SS_BUFSIZE, sizeof(char));
	get_reginfo_r(atoi(var1), buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  GSYS:0;
     *  100:7,calls,pots_calls,isdn_calls,network_calls,local_calls,startdate,last_caller;
     *  201:1,16;
     */
    if (strncmp(cmd, "GSYS", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	get_sysinfo_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  GLCC:0;
     *  100:1,n;
     */
    if (strncmp(cmd, "GLCC", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	get_lastcallercount_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  GLCR:1,recno;
     *  100:9,user,location,level,device,time,mins,calls,speed,actions;
     *  201:1,16;
     */
    if (strncmp(cmd, "GLCR", 4) == 0) {
        strtok(token, ",");
        strcpy(var1, strtok(NULL, ";"));
	buf = calloc(SS_BUFSIZE, sizeof(char));
	get_lastcallerrec_r(atoi(var1), buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }


    /*
     * The (M)IB commands, SNMP counters.
     */

    /* 
     * MIB Set Mailer Session
     *
     * MSMS:6,kbrcvd,kbsent,direction,state,freqs;
     * 100:0;
     *
     * kbrcvd	  Kbytes received
     * kbsent	  KBytes sent
     * direction  0=inbount, 1=outbound
     * state	  0=secure, 1=unsecure, 2=bad session
     * type	  0=unknown, 1=ftsc, 2=yoohoo, 3=emsi, 4=binkp
     * freqs	  nr of file requests
     */
    if (strncmp(cmd, "MSMS", 4) == 0) {
	mib_set_mailer(token);
	return obuf;
    }

    /*
     * MIB Get Mailer Session
     *
     * MGMS:0;
     * 100:12,kbrcvd,kbsent,sessin,sessout,sess_sec,sess_unseq,sess_bad,ftsc,yoohoo,emsi,binkp,freqs;
     */
    if (strncmp(cmd, "MGMS", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_mailer_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Set Tosser Netmail
     *
     * MSTN:3,in,out,bad;
     * 100:0;
     */
    if (strncmp(cmd, "MSTN", 4) == 0) {
	mib_set_netmail(token);
	return obuf;
    }

    /*
     * MIB Get Tosser Netmail
     *
     * MGTN:0;
     * 100:3,in,out,bad;
     */
    if (strncmp(cmd, "MGTN", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_netmail_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Set Tosser Internet-email
     *
     * MSTI:3,in,out,bad;
     * 100:0;
     */
    if (strncmp(cmd, "MSTI", 4) == 0) {
	mib_set_email(token);
	return obuf;
    }

    /*
     * MIB Get Tosser Internet-email
     *
     * MGTI:0;
     * 100:3,in,out,bad;
     */
    if (strncmp(cmd, "MGTI", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_email_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Set Tosser Echomail
     *
     * MSTE:4,in,out,bad,dupe;
     * 100:0;
     */
    if (strncmp(cmd, "MSTE", 4) == 0) {
	mib_set_echo(token);
	return obuf;
    }

    /*
     * MIB Get Tosser Echomail
     *
     * MGTE:0;
     * 100:4,in,out,bad,dupe;
     */
    if (strncmp(cmd, "MGTE", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_echo_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Set Tosser RFC-news
     *
     * MSTR:4,in,out,bad,dupe;
     * 100:0;
     */
    if (strncmp(cmd, "MSTR", 4) == 0) {
	mib_set_news(token);
	return obuf;
    }

    /*
     * MIB Get Tosser RFC-news
     *
     * MGTR:0;
     * 100:4,in,out,bad,dupe;
     */
    if (strncmp(cmd, "MGTR", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_news_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Get Tosser Totals
     *
     * MGTT:0;
     * 100:4,in,out,bad,dupe;
     */
    if (strncmp(cmd, "MGTT", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_tosser_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Set Tosser Files
     *
     * MSFF:6,in,out,bad,dupe,magics,hatched;
     * 100:0;
     */
    if (strncmp(cmd, "MSTF", 4) == 0) {
	mib_set_files(token);
	return obuf;
    }

    /*
     * MIB Get Tosser Files
     *
     * MGFF:0;
     * 100:6,in,out,bad,dupe,magics,hatched;
     */
    if (strncmp(cmd, "MGTF", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_files_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * MIB Set BBS
     *
     * MSBB:9,sessions,minutes,posted,uploads,kbupload,downloads,kbdownload,chats,chatminutes;
     * 100:0;
     */
    if (strncmp(cmd, "MSBB", 4) == 0) {
	mib_set_bbs(token);
	return obuf;
    }

    /*
     * MIB Get BBS
     *
     * MGBB:0;
     * 100:9,sessions,minutes,posted,uploads,kbupload,downloads,kbdownload,chats,chatminutes;
     */
    if (strncmp(cmd, "MGBB", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_bbs_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }
	
    /*
     * MIB Get Outbound Size
     *
     * MGOB:0;
     * 100:1,size;
     */
    if (strncmp(cmd, "MGOB", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	mib_get_outsize_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     * The (S)tatus commands.
     *
     *  SBBS:0;
     *  100:2,n,status message;
     */
    if (strncmp(cmd, "SBBS", 4) == 0) {
	switch(stat_bbs_stat()) {
	    case 0: snprintf(obuf, SS_BUFSIZE, "100:2,0,The system is open for use;");
		    break;
	    case 1: snprintf(obuf, SS_BUFSIZE, "100:2,1,The system is closed right now!;");
		    break;
	    case 2: snprintf(obuf, SS_BUFSIZE, "100:2,2,The system is closed for Zone Mail Hour!;");
		    break;
	}
	return obuf;
    }

    /*
     *  SOPE:0;
     *  100:0;
     */
    if (strncmp(cmd, "SOPE", 4) == 0) {
	stat_set_open(1);
	return obuf;
    }

    /*
     *  SCLO:0;
     *  100:0;
     */
    if (strncmp(cmd, "SCLO", 4) == 0) {
	stat_set_open(0);
	return obuf;
    }

    /*
     *  SFRE:0;
     *  100:1,Running utilities: n  Active users: n;
     *  100:0;
     *  201:1,16;
     */
    if (strncmp(cmd, "SFRE", 4) == 0) {
	buf = calloc(SS_BUFSIZE, sizeof(char));
	reg_fre_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  SSEQ:0;
     *  100:1,number;
     *  200:1,16;
     */
    if (strncmp(cmd, "SSEQ", 4) == 0) {
        buf = calloc(SS_BUFSIZE, sizeof(char));
	getseq_r(buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  SEST:1,semafore;   Get status of semafore
     *  100:1,n;           1 = set, 0 = not set
     *  200:1,16;
     */
    if (strncmp(cmd, "SEST", 4) == 0) {
        buf = calloc(SS_BUFSIZE, sizeof(char));
	sem_status_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  SECR:1,semafore;   Set semafore
     *  100:0;
     *  200:1,16;
     */
    if (strncmp(cmd, "SECR", 4) == 0) {
        buf = calloc(SS_BUFSIZE, sizeof(char));
	sem_create_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }

    /*
     *  SERM:1,semafore;   Remove semafore
     *  100:0;
     *  200:1,16;
     */
    if (strncmp(cmd, "SERM", 4) == 0) {
        buf = calloc(SS_BUFSIZE, sizeof(char));
	sem_remove_r(token, buf);
	snprintf(obuf, SS_BUFSIZE, "%s", buf);
	free(buf);
	return obuf;
    }


    /*
     * If we got this far, there must be an error.
     */
    stat_inc_serr();
    Syslog('!', "Comm systax error: \"%s:%s\"", cmd, printable(token, 0));
    return ebuf;
}



void do_cmd(char *cmd)
{
    char    buf[SS_BUFSIZE];
    int	    slen, tries = 0;

    if (logtrans)
	Syslog('-', "< %s", cmd);
    snprintf(buf, SS_BUFSIZE, "%s", exe_cmd(cmd));
    if (logtrans)
	Syslog('-', "> %s", buf);

    for (;;) {
	slen = sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&from, fromlen);
	if (slen == -1)
	    Syslog('?', "$do_cmd(): sendto error %d %s", tries, from.sun_path);
	else if (slen != strlen(buf))
	    Syslog('?', "do_cmd(): send %d of %d bytes, try=%d", slen, strlen(buf), tries);
	else
	    return;
	tries++;
	if (tries == 3)
	    return;
	sleep(1);
    }
}


