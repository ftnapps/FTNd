#!/bin/bash
#
# $Id$
# 
# Basic setup script for MBSE BBS
#
# (C) Michiel Broek
#
# Customisation section, change the next variables to your need.
# However, all docs refer to the setup below.
#
# Basic bbs root directory.
clear
MHOME=/opt/mbse
PATH=/bin:/sbin:/usr/bin:/usr/sbin:
DISTNAME=
DISTVERS=
OSTYPE=`uname -s`

#------------------------------------------------------------------------
#
# Logging procedure, needs two parameters.
#
log() {
    /bin/echo `date +%d-%b-%y\ %X ` $1 $2 >> SETUP.log
}


#------------------------------------------------------------------------
#
cat << EOF
MBSE BBS for Linux first time setup. Checking your system..."

If anything goes wrong with this script, look at the output of
the file SETUP.log that is created by this script in this
directory. If you can't get this script to run on your system,
mail this logfile to Michiel Broek at 2:280/2802 or email it
to mbroek@users.sourceforge.net

EOF

echo -n "Press ENTER to start the basic checks "
read junk

log "+" "MBSE BBS $0 started by `whoami`"
log "+" "Current directory is `pwd`"

# Check the OS type, only Linux for now.
#
if [ "$OSTYPE" != "Linux" ] && [ "$OSTYPE" != "FreeBSD" ] && [ "$OSTYPE" != "NetBSD" ]; then

    cat << EOF

Your are trying to install MBSE BBS on a $OSTYPE system, however
at this time only Linux, FreeBSD or NetBSD is supported.


EOF
    log "!" "Aborted, OS is $OSTYPE"
    exit 2
fi


#
# First do various tests to see which Linux distribution this is.
#
if [ "$OSTYPE" = "Linux" ]; then
    PW=
    if [ -f /etc/slackware-version ]; then
    	# Slackware 7.0 and later
    	DISTNAME="Slackware"
    	DISTVERS=`cat /etc/slackware-version`
	DISTVERS=${DISTVERS:0:3}
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
		    	fi
		    fi
	    	fi
	    fi
    	fi
    fi
fi # Linux
if [ "$OSTYPE" = "FreeBSD" ]; then
    DISTNAME="FreeBSD"
    DISTVERS=`uname -r`
    DISTVERS=${DISTVERS:0:3}
    PW="pw "
fi
if [ "$OSTYPE" = "NetBSD" ]; then
    DISTNAME="NetBSD"
    DISTVERS=`uname -r`
fi

log "+" "Detected \"${OSTYPE}\" (${HOSTTYPE}) \"${DISTNAME}\" version \"${DISTVERS}\""


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

if [ "$MBSE_ROOT" != "" ]; then
	echo "*** The MBSE_ROOT variable exists: $MBSE_ROOT ***"
	echo "*** SETUP aborted ***"
	log "!" "Aborted, MBSE_ROOT variable exists: ${MBSE_ROOT}"
	exit 2
fi

if [ "`grep -w mbse: /etc/passwd`" != "" ]; then
	echo "*** User 'mbse' already exists on this system ***"
	echo "*** SETUP aborted ***"
	log "!" "Aborted, user 'mbse' already exists on this system"
	exit 2
fi

if [ "`grep -w bbs: /etc/group`" != "" ]; then
	echo "*** Group 'bbs' already exists on this system ***"
	echo "*** SETUP aborted ***"
	log "!" "Aborted, group 'bbs' already exists on this system"
	exit 2
fi

if [ -f /etc/passwd.lock ]; then
	echo "*** The password file is locked, make sure that nobody"
	echo "    is using any password utilities. ***"
	echo "*** SETUP aborted ***"
	log "!" "Aborted, password file is locked"
	exit 2
fi
clear

if [ "$OSTYPE" = "Linux" ]; then
    if [ -d /opt ]; then
    	log "+" "Directory /opt already present"
    else
    	mkdir /opt
    	log "+" "[$?] Directory /opt created"
	echo "Directory /opt created."
    fi
fi

if [ "$OSTYPE" = "FreeBSD" ] || [ "$OSTYPE" = "NetBSD" ]; then
    #
    #  FreeBSD/NetBSD uses /usr/local for extra packages and doesn't use /opt
    #  Also using /opt means that we are in the root partition which
    #  by default is very small. We put everything in /usr/local/opt
    #  and create symlinks to it.
    #
    if [ -d /opt ]; then
	log "+" "Directory /opt already present"
    else
    	if [ -d /usr/local/opt ]; then
	    log "+" "Directory /usr/local/opt already present"
    	else
	    mkdir -p /usr/local/opt
	    log "+" "[$?] Directory /usr/local/opt created"
	    echo "Directory /usr/local/opt created."
        fi
    	ln -s /usr/local/opt /opt
	log "+" "[$?] Link /opt to /usr/local/opt created"
	echo "Link /opt to /usr/local/opt created."
    fi
fi


cat << EOF
    Basic checks done.

    The detected $OSTYPE distribution is $DISTNAME $DISTVERS

    Everything looks allright to start the installation now.
    Next the script will install a new group 'bbs' and two new
    users, 'mbse' which is the bbs system account, and 'bbs' which
    is the login account for bbs users. This account will have no
    password! The shell for this account is the main bbs program.

    One final important note: This script will make changes to some
    of your system files. Because I don't have access to all kinds of
    distributions and configurations there is no garantee that this
    script is perfect. Please make sure you have a recent system backup.
    Also make sure you have resque boot disks and know how to repair
    your system. It might also be wise to login as root on another
    virtual console incase something goes wrong with system login.

    If you are not sure, or forgot something, hit Control-C now or
EOF

echo -n "    press Enter to start the installation "
read junk
clear

#------------------------------------------------------------------------
#
#  The real work starts here
#
log "+" "Starting installation"
echo "Installing MBSE BBS for the first time..."
echo ""
echo -n "Adding group 'bbs'"
$PW groupadd bbs
log "+" "[$?] Added group bbs"
echo -n ", user 'mbse'"
if [ "$OSTYPE" = "Linux" ]; then
    useradd -c "MBSE BBS Admin" -d $MHOME -g bbs -G uucp -m -s /bin/bash mbse
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    pw useradd mbse -c "MBSE BBS Admin" -d $MHOME -g bbs -G wheel,dialer -m -s /usr/local/bin/bash
fi
if [ "$OSTYPE" = "NetBSD" ]; then
    useradd -c "MBSE BBS Admin" -d $MHOME -g bbs -G wheel,dialer -m -s /usr/pkg/bin/bash mbse
fi
log "+" "[$?] Added user mbse"
chmod 770 $MHOME
log "+" "[$?] chmod 770 $MHOME"

echo -n " writing '$MHOME/.profile'"
cat << EOF >$MHOME/.profile
# profile for mbse
#
export PATH=\$HOME/bin:\$PATH
export MBSE_ROOT=\$HOME
export GOLDED=\$HOME/etc
# For xterm on the Gnome desktop:
cd \$HOME
EOF
echo ""
log "+" "Created $MHOME/.profile"

# On some systems there is a .bashrc file in the users homedir.
# It must be removed.
if [ -f $MHOME/.bashrc ] || [ -f $MHOME/.bash_profile ]; then
      echo "Removing '$MHOME/.bash*'"
      rm -f $MHOME/.bash*
      log "+" "Removed $MHOME/.bash* files"
fi

echo ""
echo "Now set the login password for user 'mbse'"
passwd mbse
log "+" "[$?] Password is set for user mbse"


echo -n "Adding user 'bbs'"
if [ ! -d $MHOME/home ]; then
    mkdir $MHOME/home
    log "+" "[$?] Created directory $MHOME/home"
fi
chown mbse.bbs $MHOME/home
log "+" "[$?] chown mbse.bbs $MHOME/home"
chmod 770 $MHOME/home
log "+" "[$?] chmod 775 $MHOME/home"
if [ "$OSTYPE" = "Linux" ]; then
    useradd -c "MBSE BBS Login" -d $MHOME/home/bbs -g bbs -s $MHOME/bin/mbnewusr bbs
    log "+" "[$?] Added user bbs"
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    pw useradd bbs -c "MBSE BBS Login" -d $MHOME/home/bbs -g bbs -s $MHOME/bin/mbnewusr
    log "+" "[$?] Added user bbs"
fi
if [ "$OSTYPE" = "NetBSD" ]; then
    useradd -c "MBSE BBS Login" -d $MHOME/home/bbs -m -g bbs -s $MHOME/bin/mbnewusr bbs
    log "+" "[$?] Added user bbs"
fi
# Some systems (RedHat and Mandrake) insist on creating a users homedir.
# NetBSD gives errormessages when not creating a homedir, so we let it create.
# These are full of garbage we don't need. Kill it first.
if [ -d $MHOME/home/bbs ]; then
    rm -Rf $MHOME/home/bbs
    log "+" "[$?] Removed $MHOME/home/bbs"
fi
mkdir -m 0770 $MHOME/home/bbs
log "+" "[$?] mkdir $MHOME/home/bbs"
chown mbse.bbs $MHOME/home/bbs
log "+" "[$?] chown mbse.bbs $MHOME/home/bbs"


echo ", removing password:"
if [ "$OSTYPE" = "Linux" ]; then
   echo -n "$$" >/etc/passwd.lock
   if [ -f /etc/shadow ]; then
	log "+" "Standard shadow password system"
	# Not all systems are the same...
	if [ "`grep bbs:\!\!: /etc/shadow`" != "" ]; then
		sed /bbs:\!\!:/s/bbs:\!\!:/bbs::/ /etc/shadow >/etc/shadow.bbs
	else
		sed /bbs:\!:/s/bbs:\!:/bbs::/ /etc/shadow >/etc/shadow.bbs
	fi
	log "+" "[$?] removed password from user bbs"
	mv /etc/shadow /etc/shadow.mbse
	log "+" "[$?] made backup of /etc/shadow"
	mv /etc/shadow.bbs /etc/shadow
	log "+" "[$?] moved new /etc/shadow in place"
	if [ "$DISTNAME" = "Debian" ] || [ "$DISTNAME" = "SuSE" ]; then
		# Debian and SuSE use other ownership of /etc/shadow
		chmod 640 /etc/shadow
		chgrp shadow /etc/shadow
		log "+" "[$?] Debian/SuSE style owner of /etc/shadow (0640 root.shadow)"
	else
		chmod 600 /etc/shadow
		log "+" "[$?] Default style owner of /etc/shadow (0600 root.root)"
	fi
	echo " File /etc/shadow.mbse is your backup of /etc/shadow"
    else
	log "+" "Not a shadow password system"
        if [ "`grep -w bbs:\!\!: /etc/passwd`" != "" ]; then
                sed /bbs:\!\!:/s/bbs:\!\!:/bbs::/ /etc/passwd >/etc/passwd.bbs
        else
                sed /bbs:\!:/s/bbs:\!:/bbs::/ /etc/passwd >/etc/passwd.bbs
        fi
	log "+" "[$?] Removed password of user bbs"
	mv /etc/passwd /etc/passwd.mbse
	log "+" "[$?] Made backup of /etc/passwd"
	mv /etc/passwd.bbs /etc/passwd
	log "+" "[$?] Moved new /etc/passwd in place"
        chmod 644 /etc/passwd
	log "+" "[$?] Changed owner of /etc/passwd"
	echo " File /etc/passwd.mbse is your backup of /etc/passwd"
    fi
    rm /etc/passwd.lock
fi
if [ "$OSTYPE" = "NetBSD" ]; then
cat << EOF

READ THIS CAREFULLY NOW   READ THIS CAREFULLY NOW

I don't know how to automatic remove the password for the "bbs"
user account in NetBSD. You have to do this for me!
Next I start the editor you need to use, remove all the stars"
after the word Password, then save the file with "wq!"

EOF
    echo -n "Press Enter when ready "
    read junk
    chpass bbs
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    #
    #  FreeBSD has a util to remove a password
    #
    chpass -p "" bbs
    log "+" "[$?] Removed password of user bbs"
fi
echo ""


if [ "`grep binkp /etc/services`" = "" ]; then
        BINKD=TRUE
else
        BINKD=FALSE
fi
if [ "`grep fido /etc/services`" = "" ]; then
	FIDO=TRUE
else
	FIDO=FALSE
fi

log "+" "Services: binkp=$BINKD fido=$FIDO"

if [ "$FIDO" = "TRUE" ] || [ "$BINKD" = "TRUE" ]; then
	echo -n "Modifying /etc/services"
	log "+" "Modifying /etc/services"
	mv /etc/services /etc/services.mbse
	cat /etc/services.mbse >/etc/services
	echo "#" >>/etc/services
	echo "# Unofficial for MBSE BBS" >>/etc/services
	echo "#" >>/etc/services
	if [ "$BINKD" = "TRUE" ]; then
		echo -n ", binkp at port 24554"
		echo "binkp		24554/tcp		# mbcico IBN mode">>/etc/services
	fi
	if [ "$FIDO" = "TRUE" ]; then
		echo -n ", fido at port 60179"
		echo "tfido		60177/tcp		# mbcico ITN mode">>/etc/services
		echo "fido		60179/tcp		# mbcico IFC mode">>/etc/services
	fi
	chmod 644 /etc/services
	echo ", done."
fi


if [ -f /etc/inetd.conf ]; then
    log "+" "/etc/inetd.conf found, inetd system"
    if [ "`grep mbcico /etc/inetd.conf`" = "" ]; then
	echo -n "Modifying /etc/inetd.conf"
	log "+" "Modifying /etc/inetd.conf"
	mv /etc/inetd.conf /etc/inetd.conf.mbse
	cat /etc/inetd.conf.mbse >/etc/inetd.conf
cat << EOF >>/etc/inetd.conf

#:MBSE-BBS: bbs service
binkp	stream	tcp	nowait	mbse	$MHOME/bin/mbcico	mbcico -t ibn
tfido	stream	tcp	nowait	mbse	$MHOME/bin/mbcico	mbcico -t itn
fido	stream	tcp	nowait	mbse	$MHOME/bin/mbcico	mbcico -t ifc

EOF
	chmod 644 /etc/inetd.conf
	if [ -f /var/run/inetd.pid ]; then
		echo -n ", restarting inetd"
		kill -HUP `cat /var/run/inetd.pid`
		log "+" "[$?] restarted inetd"
	else
		log "!" "Warning: no inetd.pid file found"
	fi
	echo ", done."
    fi
fi

if [ -f /etc/xinetd.conf ]; then
    log "+" "/etc/xinetd.conf found, xinetd system"
    if [ -d /etc/xinetd.d ]; then
	log "+" "has xinetd.d subdir, writing files"
	XINET="/etc/xinetd.d/mbsebbs"
    else
	log "+" "appending to xinetd.conf"
	XINET="/etc/xinetd.conf"
    fi
cat << EOF >> $XINET
#:MBSE BBS services are defined here.

service binkp
{
	socket_type	= stream
	protocol	= tcp
	wait		= no
	user		= mbse
	instances	= 10
	server		= $MHOME/bin/mbcico
	server-args	= -t ibn
}

service tfido
{
	socket_type	= stream
	protocol	= tcp
	wait		= no
	user		= mbse
	instances	= 10
	server		= $MHOME/bin/mbcico
	server-args	= -t itn
}

service fido
{
	socket_type	= stream
	protocol	= tcp
	wait		= no
	user		= mbse
	instances	= 10
	server		= $MHOME/bin/mbcico
	server-args	= -t ifc
}
EOF

fi

echo ""
echo -n "Press Enter to continue"
read junk
clear

cat << EOF
     The script made it to the end, that looks good. Before you logout do some
     sanity checks;

     1. Can you still login as a normal user.

     2. Login on another virtual console, network or whatever as user 'mbse'.
        Then type 'echo \$MBSE_ROOT'. Does this show the path to
        '$MHOME' or nothing. 

     3. Login on another virtual console as user 'bbs'. It should not ask for
        a password, but should direct try to start the bbs. This is not
        installed yet but you should see error messages and then be logged out.

     If these three tests weren't successfull, restore /etc/passwd and
     or /etc/shadow, the backup copies have the extension '.mbse'.
     Then issue (as root of course) the following commands:

EOF
if [ "$OSTYPE" = "Linux" ] || [ "$OSTYPE" = "NetBSD" ]; then
    echo "     userdel bbs"
    echo "     userdel -r mbse"
    echo "     groupdel bbs"
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    echo "     pw userdel bbs -r"
    echo "     pw userdel mbse -r"
    echo "     pw groupdel bbs"
fi

