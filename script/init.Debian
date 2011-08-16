#!/bin/sh
#
# Note: this is not 100% Debian style, at least it works for now.
# $Id: init.Debian,v 1.10 2007/09/02 12:50:32 mbse Exp $
# 
# description: Starts and stops the MBSE BBS.
#
# For Debian SYSV init style.

# Find the MBSE_ROOT from the /etc/passwd file.
MBSE_ROOT=`cat /etc/passwd | grep ^mbse: | awk -F ':' '{ print $6}'`

if [ "$MBSE_ROOT" = "" ]; then
	echo "MBSE BBS: No 'mbse' user in the password file."
	exit 1
fi

if [ ! -d $MBSE_ROOT ]; then 
	echo "MBSE BBS: Home directory '$MBSE_ROOT' not found."
	exit 1
fi

PATH=/sbin:/bin:/usr/sbin:/usr/bin:$MBSE_ROOT/bin
DAEMON=$MBSE_ROOT/bin/mbtask
NAME=mbsebbs
DESC="MBSE BBS"

export MBSE_ROOT

# See how we were called.
case "$1" in
  start)
	echo -n "Starting $DESC: "
	rm -f $MBSE_ROOT/var/run/*
	rm -f $MBSE_ROOT/var/sema/*
	rm -f $MBSE_ROOT/var/*.LCK
	rm -f $MBSE_ROOT/tmp/mb*
	su mbse -c '$MBSE_ROOT/bin/mbtask' >/dev/null
	echo -n "mbtask "
	sleep 2
	if [ -f $MBSE_ROOT/etc/config.data ]; then
	    su mbse -c '$MBSE_ROOT/bin/mbstat open -quiet'
	    echo -n "opened "
	fi
	echo "done."
	;;
  stop)
	echo -n "Stopping $DESC: "
	if [ -f $MBSE_ROOT/var/run/mbtask ]; then
	    echo -n "logoff users "
	    su mbse -c '$MBSE_ROOT/bin/mbstat close wait -quiet' >/dev/null
	    echo -n " stopping mbtask"
	    pid=$( cat $MBSE_ROOT/var/run/mbtask )
	    kill $pid
	    i=10 
	    doit=1
	    while [ $i -gt 0 ] && [ $doit = 1 ]
	    do
		if [ -f $MBSE_ROOT/var/run/mbtask ]; then
		    echo -n "."
		    sleep 1
		    i=`expr $i - 1`
		else
		    doit=0
		fi
	    done
	    if [ -f $MBSE_ROOT/var/run/mbtask ]; then
		kill -9 `cat $MBSE_ROOT/var/run/mbtask`
	    fi
	    echo " done."
	else
	    echo "already stopped."
	fi
	;;
  force-reload|restart)
	$0 stop
	$0 start
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
esac
exit 0
