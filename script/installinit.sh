#
# $Id$
#
# Installation script to install bootscripts.
#
PATH=/bin:/sbin:/usr/bin:/usr/sbin:$MBSE_ROOT/bin
DISTNAME=
DISTVERS=
DISTINIT=
SU="su"
OSTYPE=`uname -s`

#------------------------------------------------------------------------
#
# Logging procedure, needs two parameters.
#
log() {
    /bin/echo `date +%d-%b-%y\ %X ` $1 $2 >> installinit.log
}



# Check one subdirectory
#
checkdir() {
    if [ ! -d $1 ]; then
	mkdir $1
	log "+" "[$?] created directory $1"
    fi
}



# Check /etc/rc.d subdirs
#
checkrcdir() {
    checkdir "/etc/rc.d/init.d"
    checkdir "/etc/rc.d/rc0.d"
    checkdir "/etc/rc.d/rc1.d"
    checkdir "/etc/rc.d/rc2.d"
    checkdir "/etc/rc.d/rc3.d"
    checkdir "/etc/rc.d/rc4.d"
    checkdir "/etc/rc.d/rc5.d"
    checkdir "/etc/rc.d/rc6.d"
}


#------------------------------------------------------------------------
#

log "+" "installinit started"

# Basic checks.
if [ `whoami` != "root" ]; then
cat << EOF
*** Run $0 as root only! ***

  Because some of the system files must be changed, you must be root
  to use this script.

*** SETUP aborted ***
EOF
        log "!" "Aborted, not root"
        exit 2
fi

if [ "$MBSE_ROOT" = "" ]; then
        echo "*** The MBSE_ROOT doesn't exist ***"
        log "!" "Aborted, MBSE_ROOT variable doesn't exist"
        exit 2
fi


# First do various tests to see which Linux distribution this is.
#
if [ "$OSTYPE" = "Linux" ]; then
    if [ -f /etc/slackware-version ]; then
    	# Slackware 7.0 and later
    	DISTNAME="Slackware"
    	DISTVERS=`cat /etc/slackware-version`
    else
    	if [ -f /etc/debian_version ]; then
	    # Debian, at least since version 2.2
	    DISTNAME="Debian"
	    DISTVERS=`cat /etc/debian_version`
    	else
	    if [ -f /etc/SuSE-release ]; then
	    	DISTNAME="SuSE"
	    	DISTVERS=`cat /etc/SuSE-release | grep VERSION | awk '{ print $3 }'`
	    else
		# Mandrake test before RedHat, Mandrake has a redhat-release
		# file also which is a symbolic link to mandrake-release.
		if [ -f /etc/mandrake-release ]; then
		    DISTNAME="Mandrake"
		    # Format: Linux Mandrake release 8.0 (Cooker) for i586
		    DISTVERS=`cat /etc/mandrake-release | awk '{ print $4 }'`
		else
		    if [ -f /etc/redhat-release ]; then
			DISTNAME="RedHat"
			DISTVERS=`cat /etc/redhat-release | awk '{ print $5 }'`
		    else
		    	if [ -f /etc/rc.d/rc.0 ] && [ -f /etc/rc.d/rc.local ]; then
		    	    # If Slackware wasn't detected yet it is version 4.0 or older.
		    	    DISTNAME="Slackware"
		    	    DISTVERS="Old"
		    	else
		    	    DISTNAME="Unknown"
		    	    log "!" "unknown distribution, collecting data"
                    	    log "-" "`uname -a`"
                    	    log "-" "`ls -la /etc`"
		    	    echo "Failed to install bootscripts, unknown Linux distribution."
		    	    echo "Please mail the file `pwd`/script/installinit.log to mbroek@users.sourceforge.net"
		    	    echo "or send it as file attach to Michiel Broek at 2:280/2802@Fidonet."
                    	    echo "Add information about the distribution you use in the message."
		    	    exit 1;
		    	fi
		    fi
	    	fi
	    fi
    	fi
    fi
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    DISTNAME="FreeBSD"
    DISTVERS=`uname -r`
    PW="pw "
fi
if [ "$OSTYPE" = "NetBSD" ]; then
    DISTNAME="NetBSD"
    DISTVERS=`uname -r`
fi



log "+" "Distribution $OSTYPE $DISTNAME $DISTVERS"


#--------------------------------------------------------------------------
#
#  Adding scripts for SuSE
#
if [ "$DISTNAME" = "SuSE" ]; then
    DISTINIT="/sbin/init.d/mbsed"
    echo "Installing SystemV init scripts for SuSE"
    log "+" "Installing SystemV init scripts for SuSE"
    echo "Adding $DISTINIT"

cat << EOF >$DISTINIT
#!/bin/bash
# Copyright (c) 2001 Michiel Broek
#
# Author: Michiel Broek <mbse@users.sourceforge.net>, 23-May-2001
#
# $DISTINIT for SuSE
#

# Find the MBSE_ROOT from the /etc/passwd file.
MBSE_ROOT=\`cat /etc/passwd | grep mbse: | awk -F ':' '{ print \$6}'\`

if [ "\$MBSE_ROOT" = "" ]
then
        echo "MBSE BBS: No 'mbse' user in the password file."
        exit 1
fi
 
if [ ! -d \$MBSE_ROOT ]
then
        echo "MBSE BBS: Home directory '\$MBSE_ROOT' not found."
        exit 1
fi
 
export MBSE_ROOT

case "\$1" in
    start|reload)
        echo -n "MBSE BBS starting:"
        rm -f \$MBSE_ROOT/sema/*
        rm -f \$MBSE_ROOT/var/*.LCK
	rm -f \$MBSE_ROOT/tmp/mb*
        $SU mbse -c '\$MBSE_ROOT/bin/mbtask' >/dev/null
        echo -n " mbtask"
	sleep 2
        if [ -f \$MBSE_ROOT/etc/config.data ]; then
                $SU mbse -c '\$MBSE_ROOT/bin/mbstat open -quiet'
                echo " and opened the bbs."
	else
		echo ""
        fi
	;;
    stop)
        echo -n "MBSE BBS shutdown:"
        if [ -f \$MBSE_ROOT/etc/config.data ]; then
                echo -n " logoff users "
                $SU mbse -c '\$MBSE_ROOT/bin/mbstat close wait -quiet' >/dev/null
                echo -n "done,"
        fi
        echo -n " stopping mbtask "
        killproc \$MBSE_ROOT/bin/mbtask -15
        echo "done."
	;;
    restart)
        echo "Restarting MBSE BBS: just kidding!"
	;;
    status)
        echo -n "MBSE BBS status: "
        if [ "\`/sbin/pidof mbtask\`" = "" ]; then
                echo "mbtask is NOT running"
        else
                echo "mbtask Ok"
        fi
	;;
    *)
	echo "Usage: \$0 {start|stop|status|reload|restart}"
	exit 1
esac
exit 0
EOF

    chmod 755 /etc/rc.d/init.d/mbsed
    echo "Making links for start/stop in runlevel 2"
    ln -s ../mbsed /sbin/init.d/rc2.d/K05mbsed
    ln -s ../mbsed /sbin/init.d/rc2.d/S99mbsed
    echo "Making links for start/stop in runlevel 3"
    ln -s ../mbsed /sbin/init.d/rc3.d/K05mbsed
    ln -s ../mbsed /sbin/init.d/rc3.d/S99mbsed
    echo "SuSE SystemV init configured"
    log "+" "SuSE SystemV init configured"
fi



#--------------------------------------------------------------------------
#
#  Adding scripts for Slackware
#
if [ "$DISTNAME" = "Slackware" ]; then
    if [ "$DISTVERS" = "Old" ] || [ "$DISTVERS" = "7.0.0" ]; then
	#
	# Slackware before version 7.1
	#
	DISTINIT="$MBSE_ROOT/etc/rc"
	echo "Adding old style Slackware MBSE BBS start/stop scripts"
        log "+" "Adding old style Slackware MBSE BBS start/stop scripts"
        if [ "`grep MBSE /etc/rc.d/rc.local`" = "" ]; then
            log "+" "Adding $MBSE_ROOT/etc/rc to /etc/rc.d/rc.local"
	    mv /etc/rc.d/rc.local /etc/rc.d/rc.local.mbse
	    cat /etc/rc.d/rc.local.mbse >/etc/rc.d/rc.local
	    echo "# Start MBSE BBS" >>/etc/rc.d/rc.local
	    echo "$MBSE_ROOT/etc/rc" >>/etc/rc.d/rc.local
	    chmod 755 /etc/rc.d/rc.local
	    echo "   Added $MBSE_ROOT/etc/rc to /etc/rc.d/rc.local"
            echo "   /etc/rc.d/rc.local.mbse is a backup file."
            echo ""
            echo "   You must manualy insert the lines '$MBSE_ROOT/etc/rc.shutdown'"
            echo "   into /etc/rc.d/rc.0 and /etc/rc.d/rc.K If you don't do it"
            echo "   everything will work also, but MBSE BBS isn't proper closed"
            echo "   if you halt or reboot your system."
        fi
        cp mbse.start   $MBSE_ROOT/bin
        cp mbse.stop    $MBSE_ROOT/bin
        cp rc           $MBSE_ROOT/etc
        cp rc.shutdown  $MBSE_ROOT/etc
        chown mbse.bbs  $MBSE_ROOT/bin/mbse.start $MBSE_ROOT/bin/mbse.stop
        chmod 755       $MBSE_ROOT/bin/mbse.start $MBSE_ROOT/bin/mbse.stop
        chown root.root $MBSE_ROOT/etc/rc $MBSE_ROOT/etc/rc.shutdown
        chmod 744       $MBSE_ROOT/etc/rc $MBSE_ROOT/etc/rc.shutdown
    else
	DISTINIT="/etc/rc.d/init.d/mbsed"
	echo "Adding SystemV Slackware MBSE BBS start/stop scripts"
        log "+" "Adding SystemV Slackware MBSE BBS start/stop scripts"
	checkrcdir

cat << EOF >$DISTINIT
#!/bin/sh
#
# description: Starts and stops MBSE BBS. 
#
# Author: Michiel Broek <mbse@users.sourceforge.net>, 23-May-2001
#
# $DISTINIT for Slackware
#

# Find the MBSE_ROOT from the /etc/passwd file.
MBSE_ROOT=\`cat /etc/passwd | grep mbse: | awk -F ':' '{ print \$6}'\`

if [ "\$MBSE_ROOT" = "" ]
then
        echo "MBSE BBS: No 'mbse' user in the password file."
        exit 1
fi

if [ ! -d \$MBSE_ROOT ]
then
        echo "MBSE BBS: Home directory '\$MBSE_ROOT' not found."
        exit 1
fi

export MBSE_ROOT

# See how we were called.
case "\$1" in
  start)
        echo -n "MBSE BBS starting:"
        rm -f \$MBSE_ROOT/sema/*
        rm -f \$MBSE_ROOT/var/*.LCK
	rm -f \$MBSE_ROOT/tmp/mb*
        $SU mbse -c '\$MBSE_ROOT/bin/mbtask' >/dev/null
        echo -n " mbtask"
	sleep 2
	if [ -f \$MBSE_ROOT/etc/config.data ]; then
        	$SU mbse -c '\$MBSE_ROOT/bin/mbstat open -quiet'
        	echo " and opened the bbs."
	fi
        ;;
  stop)
        echo -n "MBSE BBS shutdown:"
        if [ -f \$MBSE_ROOT/etc/config.data ]; then
                echo -n " logoff users "
                $SU mbse -c '\$MBSE_ROOT/bin/mbstat close wait -quiet' >/dev/null
                echo -n "done,"
	fi
        echo -n " stopping mbtask "
        kill -15 \`pidof \$MBSE_ROOT/bin/mbtask\`
        echo "done."
        ;;
  status)
	echo -n "MBSE BBS status: "
	if [ "\`/sbin/pidof mbtask\`" = "" ]; then
		echo "mbtask is NOT running"
	else
		echo "mbtask Ok"
	fi
        ;;
  restart)
        echo "Restarting MBSE BBS: just kidding!"
        ;;
  *)
        echo "Usage: mbsed {start|stop|restart|status}"
        exit 1
esac

exit 0
EOF
        chmod 755 $DISTINIT
        if [ -f $MBSE_ROOT/bin/mbse.start ]; then
            echo "Removing old startup scripts"
            rm $MBSE_ROOT/bin/mbse.start $MBSE_ROOT/bin/mbse.stop $MBSE_ROOT/etc/rc $MBSE_ROOT/etc/rc.shutdown
        fi
	echo "Making links for start/stop in runlevel 3"
        if [ -f /etc/rc.d/rc3.d/K05mbsed ]; then
            rm /etc/rc.d/rc3.d/K05mbsed
        fi
        ln -s ../init.d/mbsed /etc/rc.d/rc3.d/K05mbsed
        if [ -f /etc/rc.d/rc3.d/S95mbsed ]; then
            rm /etc/rc.d/rc3.d/S95mbsed
        fi
        ln -s ../init.d/mbsed /etc/rc.d/rc3.d/S95mbsed
	echo "Making links for start/stop in runlevel 4"
        if [ -f /etc/rc.d/rc4.d/K05mbsed ]; then
            rm /etc/rc.d/rc4.d/K05mbsed
        fi
        ln -s ../init.d/mbsed /etc/rc.d/rc4.d/K05mbsed
        if [ -f /etc/rc.d/rc4.d/S95mbsed ]; then
            rm /etc/rc.d/rc4.d/S95mbsed
        fi
        ln -s ../init.d/mbsed /etc/rc.d/rc4.d/S95mbsed
	echo "Slackware SystemV init configured"
        log "+" "Slackware SystemV init configured"
    fi
fi



#--------------------------------------------------------------------------
#
#  Adding scripts for RedHat and Mandrake
#  FIXME: some details unknown about Mandrake
#
if [ "$DISTNAME" = "RedHat" ] || [ "$DISTNAME" = "Mandrake" ]; then

    log "+" "Adding RedHat/Mandrake SystemV init scripts"
    DISTINIT="/etc/rc.d/init.d/mbsed"
    SU="su"
    #
    # From RedHat version 6.1 and up the behaviour of "su" has changed.
    # For Mandrake we follow the same behaviour.
    #
    if [ -f /etc/redhat-release ]; then
	RHR=`cat /etc/redhat-release | awk '{ print $5 }' | tr -d .`
	if [ $RHR -gt 60 ]; then
	    echo "You are running RedHat v6.1 or newer"
	    SU="su -"
	else
	    echo "You are running RedHat v6.0 or older"
	fi
    else
	if [ -f /etc/mandrake-release ]; then
	    RHR=`cat /etc/mandrake-release | awk '{ print $4 }' | tr -d .`
	    if [ $RHR -gt 60 ]; then
		echo "You are running Mandrake v6.1 or newer"
		SU="su -"
	    else
		echo "You are running Mandrake v6.0 or older"
	    fi
	else
	    echo "You are in big trouble."
	fi
    fi
    echo "Adding startup file $DISTINIT"

cat << EOF >$DISTINIT
#!/bin/sh
#
# chkconfig: 345 99 05
# description: Starts and stops MBSE BBS.
#
# For RedHat and Mandrake SYSV init style.
# 23-May-2001 M. Broek
#
# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
[ \${NETWORKING} = "no" ] && exit 1

# Find the MBSE_ROOT from the /etc/passwd file.
MBSE_ROOT=\`cat /etc/passwd | grep mbse: | awk -F ':' '{ print \$6}'\`

if [ "\$MBSE_ROOT" = "" ]
then
	echo "MBSE BBS: No 'mbse' user in the password file."
	exit 1
fi

if [ ! -d \$MBSE_ROOT ]
then
	echo "MBSE BBS: Home directory '\$MBSE_ROOT' not found."
	exit 1
fi

export MBSE_ROOT

# See how we were called.
case "\$1" in
  start)
	echo -n "Starting MBSE BBS: "
	rm -f \$MBSE_ROOT/sema/*
	rm -f \$MBSE_ROOT/var/*.LCK
	rm -f \$MBSE_ROOT/tmp/mb*
	$SU mbse -c '\$MBSE_ROOT/bin/mbtask' >/dev/null
	echo -n "mbtask "
	sleep 2
	if [ -f \$MBSE_ROOT/etc/config.data ]; then
		$SU mbse -c '\$MBSE_ROOT/bin/mbstat open -quiet'
		echo "opened"
	fi
	touch /var/lock/subsys/mbsed
	;;
  stop)
	echo -n "Shutting down MBSE BBS: "
	if [ -f \$MBSE_ROOT/etc/config.data ]; then
		echo -n "logoff users "
		$SU mbse -c '\$MBSE_ROOT/bin/mbstat close wait -quiet' >/dev/null
		echo -n "done, "
	fi
	echo -n "stop mbtask: "
	killproc mbtask -15
	rm -f /var/lock/subsys/mbsed
	echo "done."
	;;
  status)
	status mbsed
	;;
  restart)
	echo "Restarting MBSE BBS: just kidding!"
	;;
  *)
	echo "Usage: mbsed {start|stop|restart|status}"
	exit 1
esac

exit 0
EOF
	chmod 755 $DISTINIT
	echo "With the runlevel editor, 'tksysv' if you are running X,"
	echo "or 'ntsysv' if you are running virtual consoles, you must"
	echo "now add 'mbsed' start to the default runlevel, and 'mbsed'"
	echo "stop to runlevels 0 and 6"
fi



#--------------------------------------------------------------------------
#
#  Adding scripts for Debian
#
#
if [ "$DISTNAME" = "Debian" ]; then
	echo "You are running Debian Linux $DISTVERS"
        log "+" "Adding Debian SystemV init script"
	DISTINIT="/etc/init.d/mbsebbs"

cat << EOF >$DISTINIT
#!/bin/sh
#
# Note: this is not 100% Debian style, at least it works for now.
# 23-May-2001 Michiel Broek.
#  
# description: Starts and stops the MBSE BBS.

# For Debian SYSV init style.

# Find the MBSE_ROOT from the /etc/passwd file.
MBSE_ROOT=\`cat /etc/passwd | grep mbse: | awk -F ':' '{ print \$6}'\`

if [ "\$MBSE_ROOT" = "" ]; then
	echo "MBSE BBS: No 'mbse' user in the password file."
	exit 1
fi

if [ ! -d \$MBSE_ROOT ]; then 
	echo "MBSE BBS: Home directory '\$MBSE_ROOT' not found."
	exit 1
fi

PATH=/sbin:/bin:/usr/sbin:/usr/bin:\$MBSE_ROOT/bin
DAEMON=\$MBSE_ROOT/bin/mbtask
NAME=mbsebbs
DESC="MBSE BBS"

export MBSE_ROOT

# See how we were called.
case "\$1" in
  start)
	echo -n "Starting \$DESC: "
	rm -f \$MBSE_ROOT/sema/*
	rm -f \$MBSE_ROOT/var/*.LCK
	rm -f \$MBSE_ROOT/tmp/mb*
	su mbse -c '\$MBSE_ROOT/bin/mbtask' >/dev/null
	echo -n "mbtask "
	sleep 2
	if [ -f \$MBSE_ROOT/etc/config.data ]; then
		su mbse -c '\$MBSE_ROOT/bin/mbstat open -quiet'
		echo -n "opened "
	fi
	echo "done."
	;;
  stop)
	echo -n "Stopping \$DESC: "
	if [ -f \$MBSE_ROOT/etc/config.data ]; then
		echo -n "logoff users "
		su mbse -c '\$MBSE_ROOT/bin/mbstat close wait -quiet' >/dev/null
	fi
	start-stop-daemon --stop --signal 15 --user mbtask
	echo "\$NAME done."
	;;
  force-reload|restart)
	echo "Restarting \$DESC: is not possible, done."
	;;
  *)
	N=/etc/init.d/\$NAME
	echo "Usage: \$N {start|stop|restart|force-reload}" >&2
	exit 1
esac

exit 0
EOF
	chmod 755 $DISTINIT
	update-rc.d mbsebbs defaults
	echo "Debian install ready."
        log "+" "Debian SystemV init script installed"
fi


#--------------------------------------------------------------------------
#
#  Adding scripts for FreeBSD and NetBSD
#
#
if [ "$DISTNAME" = "FreeBSD" ] || [ "$DISTNAME" = "NetBSD" ]; then
    #
    # FreeBSD init
    #
    DISTINIT="$MBSE_ROOT/etc/rc"
    echo "Adding $DISTNAME style MBSE BBS start/stop scripts"
    log "+" "Adding $DISTNAME style MBSE BBS start/stop scripts"
    if [ -f /etc/rc.local ]; then
	if [ "`grep MBSE /etc/rc.local`" = "" ]; then
	    log "+" "Adding $MBSE_ROOT/etc/rc to existing /etc/rc.local"
	    mv /etc/rc.local /etc/rc.local.mbse
	    cat /etc/rc.local.mbse >/etc/rc.local
	    echo "# Start MBSE BBS" >>/etc/rc.local
	    echo "$MBSE_ROOT/etc/rc" >>/etc/rc.local
	    chmod 644 /etc/rc.local
	    echo "   Added $MBSE_ROOT/etc/rc to /etc/rc.local"
	    echo "   /etc/rc.local.mbse is a backup file."
	    echo ""
	fi
    else
	log "+" "Adding $MBSE_ROOT/etc/rc to new /etc/rc.local"
	echo "# Start MBSE BBS" >/etc/rc.local
	echo "$MBSE_ROOT/etc/rc" >>/etc/rc.local
	chmod 644 /etc/rc.local
	echo "   Added $MBSE_ROOT/etc/rc to /etc/rc.local"
	echo ""
    fi
    cp mbse.start   $MBSE_ROOT/bin
    cp mbse.stop    $MBSE_ROOT/bin
    cp rc           $MBSE_ROOT/etc
    cp rc.shutdown  $MBSE_ROOT/etc
    chown mbse.bbs  $MBSE_ROOT/bin/mbse.start $MBSE_ROOT/bin/mbse.stop
    chmod 755       $MBSE_ROOT/bin/mbse.start $MBSE_ROOT/bin/mbse.stop
    chown `id -un`.`id -gn` $MBSE_ROOT/etc/rc $MBSE_ROOT/etc/rc.shutdown
    chmod 744       $MBSE_ROOT/etc/rc $MBSE_ROOT/etc/rc.shutdown
fi



echo
echo "Please note, your MBSE BBS startup file is \"$DISTINIT\""
echo 
