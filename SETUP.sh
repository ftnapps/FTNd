#!/bin/bash
#
# SETUP.sh
# 
# Basic setup script for FTNd
#
# (C) Robert James Clay and Michiel Broek
#
# Customisation section, change the next variables to your need.
# However, all docs refer to the setup below.
#
# Basic bbs root directory.
clear
FTNHOME=/opt/ftnd
PATH=/bin:/sbin:/usr/bin:/usr/sbin:
DISTNAME=
DISTVERS=
OSTYPE=$( uname -s )

#------------------------------------------------------------------------
#
# Logging procedure, needs two parameters.
#
log() {
    /bin/echo $( date +%d-%b-%y\ %X ) $1 $2 >> SETUP.log
}


#------------------------------------------------------------------------
#
cat << EOF
FTNd for Unix, first time setup. Checking your system..."

If anything goes wrong with this script, look at the output of
the file SETUP.log that is created by this script in this
directory. If you can't get this script to run on your system,
mail this logfile to RJ Clay at 1:120/544 or email it
to jame@rocasa.us

EOF

echo -n "Press ENTER to start the basic checks "
read junk

log "+" "FTNd $0 started by $(whoami)"
log "+" "Current directory is $(pwd)"

# Check the OS type, only Linux for now.
#
if [ "$OSTYPE" != "Linux" ] && [ "$OSTYPE" != "FreeBSD" ] && [ "$OSTYPE" != "NetBSD" ] && [ "$OSTYPE" != "OpenBSD" ] && [ "$OSTYPE" != "Darwin" ]; then

    cat << EOF

Your are trying to install FTNd on a $OSTYPE system, however
at this time only Linux, FreeBSD, NetBSD, OpenBSD and Darwin (OS X)
are supported.

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
    # There are two styles, newer releases are like "Slackware 12.0.0"
    if grep -q Slackware /etc/slackware-version ; then
        DISTVERS=$( cat /etc/slackware-version | awk '{ print $2 }' )
    else
        DISTVERS=$( cat /etc/slackware-version )
    fi
    elif [ -f /etc/slamd64-version ]; then
    # Slamd64
    DISTNAME="Slamd64"
    DISTVERS=`cat /etc/slamd64-version`
    elif [ -f /etc/zenwalk-version ]; then
    DISTNAME="Zenwalk"
    DISTVERS=$( cat /etc/zenwalk-version | awk '{ print $2 }' )
    elif [ -f /etc/debian_version ]; then
    # Debian, at least since version 2.2
    DISTNAME="Debian"
    DISTVERS=$( cat /etc/debian_version )
    # Ubuntu is based on Debian
    if grep -q "Ubuntu" /etc/issue ; then
        DISTNAME="Ubuntu"
        DISTVERS=$( cat /etc/issue | awk '{ print $2 }' )
    fi
    elif [ -f /etc/SuSE-release ]; then
    DISTNAME="SuSE"
    DISTVERS=$( cat /etc/SuSE-release | grep VERSION | awk '{ print $3 }' )
    # Mandrake test before RedHat, Mandrake has a redhat-release
    # file also which is a symbolic link to mandrake-release.
    elif [ -f /etc/mandrake-release ]; then
    DISTNAME="Mandrake"
    # Format: Linux Mandrake release 8.0 (Cooker) for i586
    DISTVERS=$( cat /etc/mandrake-release | awk '{ print $4 }' )
    elif [ -f /etc/redhat-release ]; then
    DISTNAME="RedHat"
    if grep -q e-smith /etc/redhat-release ; then
        DISTVERS=$( cat /etc/redhat-release | awk '{ print $13 }' | tr -d \) )
    else
        DISTVERS=$( cat /etc/redhat-release | awk '{ print $5 }' )
    fi
    elif [ -f /etc/gentoo-release ]; then
        DISTNAME="Gentoo"
        DISTVERS=$( cat /etc/gentoo-release | awk '{ print $5 }' )
    elif [ -f /etc/arch-release ]; then
    DISTNAME="Arch Linux"
    DISTVERS="N/A"
    else
        DISTNAME="Unknown"
    fi
elif [ "$OSTYPE" = "FreeBSD" ]; then
    DISTNAME="FreeBSD"
    DISTVERS=$(uname -r)
    PW="pw "
elif [ "$OSTYPE" = "NetBSD" ]; then
    DISTNAME="NetBSD"
    DISTVERS=$(uname -r)
elif [ "$OSTYPE" = "OpenBSD" ]; then
    DISTNAME="OpenBSD"
    DISTVERS=$(uname -r)
elif [ "$OSTYPE" = "Darwin" ]; then
    DISTNAME="Darwin"
    DISTVERS=$(uname -r)
else
    DISTNAME="Unknown"
fi
log "+" "Detected \"${OSTYPE}\" (${HOSTTYPE}) \"${DISTNAME}\" version \"${DISTVERS}\""


# Basic checks.
if [ "$DISTNAME" = "Unknown" ]; then
    cat << EOF

    Your are trying to install FTNd on a $OSTYPE system, however
    that distribution is unknown.

EOF
    log "!" "Aborted, OS is $OSTYPE, distribution is unknown"
    exit 2
fi

if [ $( whoami ) != "root" ]; then
cat << EOF
*** Run $0 as root only! ***

  Because some of the system files must be changed, you must be root
  to use this script.

*** SETUP aborted ***
EOF
    log "!" "Aborted, not root"
    exit 2
fi

if [ "$FTND_ROOT" != "" ]; then
    echo "*** The FTND_ROOT variable exists: $FTND_ROOT ***"
    echo "*** SETUP aborted ***"
    log "!" "Aborted, FTND_ROOT variable exists: ${FTND_ROOT}"
    exit 2
fi

if grep -q ^ftnd: /etc/passwd ; then
    echo "*** User 'ftnd' already exists on this system ***"
    echo "*** SETUP aborted ***"
    log "!" "Aborted, user 'ftnd' already exists on this system"
    exit 2
fi

if grep -q ^ftnbbs: /etc/group ; then
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

#
# Check if this is Debian. Debian by default has no xinetd installed.
#
if [ "$DISTNAME" = "Debian" ]; then
    if [ ! -f /etc/xinetd.d/echo ]; then
        echo "*** You seem to be using Debian but have not yet installed xinetd."
        echo "    'sudo apt-get install xinetd' will install that for you. ***"
        echo "*** SETUP aborted ***"
        log "!" "Aborted, Ubuntu without xinetd package"
        exit 2
    fi
fi

#
# Check if this is Ubuntu. Ubuntu by default has no xinetd installed.
#
if [ "$DISTNAME" = "Ubuntu" ]; then
    if [ ! -f /etc/xinetd.d/echo ]; then
        echo "*** You seem to be using Ubuntu but have not yet installed xinetd."
        echo "    'sudo apt-get install xinetd' will install that for you. ***"
        echo "*** SETUP aborted ***"
        log "!" "Aborted, Ubuntu without xinetd package"
        exit 2
    fi
fi

if [ "$DISTNAME" = "Arch Linux" ]; then
    if [ ! -f /etc/xinetd.d/servers ]; then
        echo "*** You seem to be using Arch Linux but have not yet installed xinetd."
        echo "    'pacman -S xinetd' will install that for you. ***"
        echo "*** SETUP aborted ***"
        log "!" "Aborted, Arch Linux without xinetd package"
        exit 2
    fi
fi

clear

if [ "$OSTYPE" = "Linux" ]; then
    if [ -d /opt ]; then
        log "+" "Directory /opt already present"
    else
        mkdir /opt
        log "+" "Directory /opt created"
        echo "Directory /opt created."
    fi
fi

if [ "$OSTYPE" = "FreeBSD" ] || [ "$OSTYPE" = "NetBSD" ] || [ "$OSTYPE" = "OpenBSD" ] || [ "$OSTYPE" = "Darwin" ]; then
    #
    #  FreeBSD/NetBSD/OpenBSD/Darwin uses /usr/local for extra packages
    #  and doesn't use /opt.
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
            log "+" "Directory /usr/local/opt created"
            echo "Directory /usr/local/opt created."
        fi
        ln -s /usr/local/opt /opt
        log "+" "Link /opt to /usr/local/opt created"
        echo "Link /opt to /usr/local/opt created."
    fi
fi


cat << EOF
    Basic checks done.

    The detected $OSTYPE distribution is $DISTNAME $DISTVERS

    Everything looks allright to start the installation now.
    Next the script will install a new group 'ftnbbs' and two new
    users, 'ftnd' which is the FTNd system account, and 'ftnbbs' which
    is the login account for ftnbbs users. This account will have no
    password! The shell for this account is the main ftnbbs program.

    One final important note: This script will make changes to some
    of your system files. Because I don't have access to all kinds of
    distributions and configurations there is no garantee that this
    script is perfect. Please make sure you have a recent system backup.
    Also make sure you have resque boot disks and know how to repair
    your system. It might also be wise to login as root on another
    virtual console incase something goes wrong with system login.

    Darwin (OS X) Users must install the .dmg image of user utils
    available on Version Tracker and within this archive prior
    to continuing the installation.

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
echo "Installing FTNd for the first time..."
echo ""
echo -n "Adding group 'ftnbbs'"
$PW groupadd ftnbbs
log "+" "[$?] Added group ftnbbs"

echo -n ", user 'ftnd' $OSTYPE "
if [ "$OSTYPE" = "Linux" ]; then
    # Different distros have different needs...
    GRPS="uucp"
    if grep -q ^wheel /etc/group ; then
        GRPS=${GRPS}",wheel"
    fi
    if [ "$DISTNAME" = "Ubuntu" ]; then
        GRPS=${GRPS}",adm,admin"
    fi
    if grep -q ^dialout /etc/group ; then
        GRPS=${GRPS}",dialout"
    fi
    if grep -q ^dip /etc/group ; then
        GRPS=${GRPS}",dip"
    fi
    log "+" "useradd -c \"FTNd Admin\" -d $FTNHOME -g ftnbbs -G $GRPS -m -s /bin/bash ftnd"
    useradd -c "FTNd Admin" -d $FTNHOME -g ftnbbs -G $GRPS -m -s /bin/bash ftnd
elif [ "$OSTYPE" = "FreeBSD" ]; then
    pw useradd ftnd -c "FTNd Admin" -d $FTNHOME -g ftnbbs -G wheel,dialer -m -s /usr/local/bin/bash
elif [ "$OSTYPE" = "NetBSD" ]; then
    useradd -c "FTNd Admin" -d $FTNHOME -g ftnbbs -G wheel,dialer -m -s /usr/pkg/bin/bash ftnd
elif [ "$OSTYPE" = "OpenBSD" ]; then
    useradd -c "FTNd Admin" -d $FTNHOME -g ftnbbs -G wheel,dialer -m -s /usr/local/bin/bash ftnd
elif [ "$OSTYPE" = "Darwin" ]; then
    useradd ftnd -c "FTNd Admin" -d $FTNHOME -g ftnbbs -s /bin/bash
fi
log "+" "[$?] Added user ftnd"
chmod 755 $FTNHOME
log "+" "[$?] chmod 755 $FTNHOME"

echo -n " writing '$FTNHOME/.profile'"
cat << EOF >$FTNHOME/.profile
# profile for ftnd
#
export PATH=\$HOME/bin:\$PATH
export FTND_ROOT=\$HOME
export GOLDED=\$HOME/etc
# For xterm on the Gnome desktop:
cd \$HOME
#
export COLUMNS LINES
EOF
chown ftnd $FTNHOME/.profile
chgrp ftnbbs $FTNHOME/.profile
echo ""
log "+" "Created $FTNHOME/.profile"

# On some systems there is a .bashrc file in the users homedir.
# It must be removed.
if [ -f $FTNHOME/.bashrc ] || [ -f $FTNHOME/.bash_profile ]; then
      echo "Removing '$FTNHOME/.bash*'"
      rm -f $FTNHOME/.bash*
      log "+" "Removed $FTNHOME/.bash* files"
fi

echo ""
echo "Now set the login password for user 'ftnd'"
passwd ftnd
log "+" "[$?] Password is set for user ftnd"


echo -n "Adding user 'ftnbbs'"
if [ ! -d $FTNHOME/home ]; then
    mkdir $FTNHOME/home
    log "+" "[$?] Created directory $FTNHOME/home"
fi
chown ftnd $FTNHOME/home
log "+" "[$?] chown ftnd $FTNHOME/home"
chgrp ftnbbs $FTNHOME/home
log "+" "[$?] chgrp ftnbbs $FTNHOME/home"
chmod 770 $FTNHOME/home
log "+" "[$?] chmod 770 $FTNHOME/home"
if [ "$OSTYPE" = "Linux" ]; then
    useradd -c "FTNd Login" -d $FTNHOME/home/ftnbbs -g ftnbbs -s $FTNHOME/bin/ftnnewusr ftnbbs
    log "+" "[$?] Added user ftnbbs"
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    pw useradd ftnbbs -c "FTNd Login" -d $FTNHOME/home/ftnbbs -g ftnbbs -s $FTNHOME/bin/ftnnewusr
    log "+" "[$?] Added user ftnbbs"
fi
if [ "$OSTYPE" = "NetBSD" ]; then
    useradd -c "FTNd Login" -d $FTNHOME/home/ftnbbs -m -g ftnbbs -s $FTNHOME/bin/ftnnewusr ftnbbs
    log "+" "[$?] Added user ftnbbs"
fi
if [ "$OSTYPE" = "OpenBSD" ]; then
    useradd -c "FTNd Login" -d $FTNHOME/home/ftnbbs -m -g ftnbbs -s $FTNHOME/bin/ftnnewusr ftnbbs
    log "+" "[$?] Added user ftnbbs"
fi
if [ "$OSTYPE" = "Darwin" ]; then
    useradd ftnbbs -c "FTNd Login" -d $FTNHOME/home/ftnbbs -g ftnbbs -s $FTNHOME/bin/ftnnewuser
    log "+" "[$?] Added user ftnbbs"
fi
# Some systems (RedHat and Mandrake) insist on creating a users homedir.
# NetBSD gives errormessages when not creating a homedir, so we let it create.
# These are full of garbage we don't need. Kill it first.
if [ -d $FTNHOME/home/ftnbbs ]; then
    rm -Rf $FTNHOME/home/ftnbbs
    log "+" "[$?] Removed $FTNHOME/home/ftnbbs"
fi
mkdir -m 0770 $FTNHOME/home/ftnbbs
log "+" "[$?] mkdir $FTNHOME/home/ftnbbs"
chown ftnd $FTNHOME/home/ftnbbs
log "+" "[$?] chown ftnd $FTNHOME/home/ftnbbs"
chgrp ftnbbs $FTNHOME/home/ftnbbs
log "+" "[$?] chgrp ftnbbs $FTNHOME/home/ftnbbs"

echo ", removing password:"
if [ "$OSTYPE" = "Linux" ]; then
   echo -n "$$" >/etc/passwd.lock
   if [ -f /etc/shadow ]; then
    log "+" "Standard shadow password system"
    # Not all systems are the same...
    if grep -q ^ftnbbs:\!\!: /etc/shadow ; then
        sed /ftnbbs:\!\!:/s/ftnbbs:\!\!:/ftnbbs::/ /etc/shadow >/etc/shadow.ftnbbs
    else
        sed /ftnbbs:\!:/s/ftnbbs:\!:/ftnbbs::/ /etc/shadow >/etc/shadow.ftnbbs
    fi
    log "+" "[$?] removed password from user ftnbbs"
    mv /etc/shadow /etc/shadow.ftnd
    log "+" "[$?] made backup of /etc/shadow"
    mv /etc/shadow.ftnbbs /etc/shadow
    log "+" "[$?] moved new /etc/shadow in place"
    if [ "$DISTNAME" = "Debian" ] || [ "$DISTNAME" = "Ubuntu" ] || [ "$DISTNAME" = "SuSE" ]; then
        # Debian, Ubuntu and SuSE use other ownership of /etc/shadow
        chmod 640 /etc/shadow
        chgrp shadow /etc/shadow
        log "+" "[$?] Debian/Ubuntu/SuSE style owner of /etc/shadow (0640 root.shadow)"
    else
        chmod 600 /etc/shadow
        log "+" "[$?] Default style owner of /etc/shadow (0600 root.root)"
    fi
    echo " File /etc/shadow.ftnd is your backup of /etc/shadow"
    else
    log "+" "Not a shadow password system"
        if grep -q ^ftnbbs:\!\!: /etc/passwd ; then
            sed /ftnbbs:\!\!:/s/ftnbbs:\!\!:/ftnbbs::/ /etc/passwd >/etc/passwd.ftnbbs
        else
            sed /ftnbbs:\!:/s/ftnbbs:\!:/ftnbbs::/ /etc/passwd >/etc/passwd.ftnbbs
        fi
        log "+" "[$?] Removed password of user ftnbbs"
        mv /etc/passwd /etc/passwd.ftnd
        log "+" "[$?] Made backup of /etc/passwd"
        mv /etc/passwd.ftnbbs /etc/passwd
        log "+" "[$?] Moved new /etc/passwd in place"
        chmod 644 /etc/passwd
        log "+" "[$?] Changed owner of /etc/passwd"
        echo " File /etc/passwd.ftnd is your backup of /etc/passwd"
    fi
    rm /etc/passwd.lock
fi
if [ "$OSTYPE" = "NetBSD" ] || [ "$OSTYPE" = "OpenBSD" ] || [ "$OSTYPE" = "Darwin" ]; then
cat << EOF

READ THIS CAREFULLY NOW   READ THIS CAREFULLY NOW

I don't know how to automatic remove the password for the "ftnbbs"
user account in NetBSD/Darwin. You have to do this for me!
Next I start the editor you need to use, remove all the stars"
after the word Password, then save the file with "wq!"

EOF
    echo -n "Press Enter when ready "
    read junk
    chpass ftnbbs
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    #
    #  FreeBSD has a util to remove a password
    #
    chpass -p "" ftnbbs
    log "+" "[$?] Removed password of user ftnbbs"
fi
echo ""


if grep -q ^binkp /etc/services ; then
    BINKD=FALSE
else
    BINKD=TRUE
fi
if grep -q 60179\/tcp /etc/services ; then
    FIDO_TCP=FALSE
else
    FIDO_TCP=TRUE
fi
if grep -q 60179\/udp /etc/services ; then
    FIDO_UDP=FALSE
else
    FIDO_UDP=TRUE
fi
if grep -q ^tfido /etc/services ; then
    TFIDO=FALSE
else
    TFIDO=TRUE
fi

log "+" "Services: binkp=$BINKD fido_tcp=$FIDO_TCP fido_udp=$FIDO_UDP tfido=$TFIDO"

if [ "$FIDO_TCP" = "TRUE" ] || [ "$FIDO_UDP" = "TRUE" ] || [ "$TFIDO" = "TRUE" ] || [ "$BINKD" = "TRUE" ]; then
    echo -n "Modifying /etc/services"
    log "+" "Modifying /etc/services"
    mv /etc/services /etc/services.ftnd
    cat /etc/services.ftnd >/etc/services
    echo "#" >>/etc/services
    echo "# Unofficial for FTNd" >>/etc/services
    echo "#" >>/etc/services
    if [ "$BINKD" = "TRUE" ]; then
        echo -n ", binkp at port 24554"
        echo "binkp     24554/tcp       # ftncico IBN mode">>/etc/services
    fi
    if [ "$TFIDO" = "TRUE" ]; then
        echo -n ", tfido at port 60177"
        echo "tfido     60177/tcp       # ftncico ITN mode (alternate port)">>/etc/services
    fi
    if [ "$FIDO_TCP" = "TRUE" ]; then
        echo -n ", fido tcp at port 60179"
        echo "fido      60179/tcp       # ftncico IFC mode">>/etc/services
    fi
    if [ "$FIDO_UDP" = "TRUE" ]; then
        echo -n ", fido udp at port 60179"
        echo "fido      60179/udp       # Chatserver">>/etc/services
    fi
    chmod 644 /etc/services
    echo ", done."
fi


if [ -f /etc/inetd.conf ]; then
    log "+" "/etc/inetd.conf found, inetd system"
    if ! grep -q ftncico /etc/inetd.conf ; then
        echo -n "Modifying /etc/inetd.conf"
        log "+" "Modifying /etc/inetd.conf"
        mv /etc/inetd.conf /etc/inetd.conf.ftnd
        cat /etc/inetd.conf.ftnd >/etc/inetd.conf
cat << EOF >>/etc/inetd.conf

#:FTNd: bbs service
binkp   stream  tcp nowait  ftnd    $FTNHOME/bin/ftncico ftncico -t ibn
fido    stream  tcp nowait  ftnd    $FTNHOME/bin/ftncico ftncico -t ifc
tfido   stream  tcp nowait  ftnd    $FTNHOME/bin/ftncico ftncico -t itn
# Example Linux telnet to the BBS
#telnet stream  tcp nowait  root    /usr/sbin/tcpd  in.telnetd -L $FTNHOME/bin/ftnlogin
# Example FreeBSD telnet to the BBS
#telnet  stream  tcp     nowait  root   /usr/libexec/telnetd  telnetd -p $FTNHOME/bin/ftnlogin
# Example OpenBSD telnet to the BBS
#telnet  stream  tcp     nowait  root   /usr/libexec/tcpd     telnetd -L $FTNHOME/bin/ftnlogin
# Example NetBSD telnet to the BBS
#telnet  stream  tcp     nowait  root   /usr/libexec/telnetd  telnetd -g ftnbbs

EOF
    chmod 644 /etc/inetd.conf
    if [ -f /var/run/inetd.pid ]; then
        echo -n ", restarting inetd"
        kill -HUP $( cat /var/run/inetd.pid )
        log "+" "[$?] restarted inetd"
    else
        log "!" "Warning: no inetd.pid file found"
    fi
    echo ", done."
    fi
fi

if [ "$OSTYPE" = "NetBSD" ]; then
  if [ -f /etc/gettytab ]; then
    if ! grep ftnbbs /etc/gettytab ; then
      log "+" "[$?] adding ftnbbs login to /etc/gettytab"
cat << EOF >>/etc/gettytab

#
# Login entry for ftnbbs.
#
ftnbbs:cd:ck:np:lo=$FTNHOME/bin/ftnlogin:sp#38400:
EOF
    fi
  fi
fi

if [ -f /etc/xinetd.conf ]; then
    log "+" "/etc/xinetd.conf found, xinetd system"
    if [ -d /etc/xinetd.d ]; then
        log "+" "has xinetd.d subdir, writing files"
        XINET="/etc/xinetd.d/ftndbbs"
    else
        log "+" "appending to xinetd.conf"
        XINET="/etc/xinetd.conf"
    fi
cat << EOF >> $XINET
#:FTNd services are defined here.
#
# Author: Robert James Clay <jame@rocasa.us>, 01-Apr-2012
# Author: Michiel Broek <ftnd@ftnd.eu>, 27-Sep-2004

service binkp
{
    socket_type     = stream
    protocol        = tcp
    wait            = no
    user            = ftnd
    instances       = 10
    server          = $FTNHOME/bin/ftncico
    server_args     = -t ibn
}

service fido
{
    socket_type     = stream
    protocol        = tcp
    wait            = no
    user            = ftnd
    instances       = 10
    server          = $FTNHOME/bin/ftncico
    server_args     = -t ifc
}

service tfido
{
    socket_type     = stream
    protocol        = tcp
    wait            = no
    user            = ftnd
    instances       = 10
    server          = $FTNHOME/bin/ftncico
    server_args     = -t itn
}

# Telnet to the bbs using ftnlogin, disabled by default.
#
service telnet
{
    disable         = yes
    protocol        = tcp
    instances       = 10
    flags           = REUSE
    log_on_failure += USERID
    socket_type     = stream
    user            = root
    server          = /usr/sbin/telnetd
    server_args     = -L $FTNHOME/bin/ftnlogin
    wait            = no
}

EOF

fi

# We made it, copy the logfile to ftnd's homedir so that when the
# /tmp directory is cleaned, we still have it.
cat SETUP.log >> $FTNHOME/SETUP.log

echo ""
echo -n "Press Enter to continue"
read junk
clear

cat << EOF
     The script made it to the end, that looks good. Before you logout do some
     sanity checks;

     1. Can you still login as a normal user.

     2. Login on another virtual console, network or whatever as user 'ftnd'.
        Then type 'echo \$FTND_ROOT'. Does this show the path to
        '$FTNHOME' or nothing. 

     3. Login on another virtual console as user 'ftnbbs'. It should not ask for
        a password, but should direct try to start the bbs. This is not
        installed yet but you should see error messages and then be logged out.

     If these three tests weren't successfull, restore /etc/passwd and
     or /etc/shadow, the backup copies have the extension '.ftnd'.
     Then issue (as root of course) the following commands:

EOF
if [ "$OSTYPE" = "Linux" ] || [ "$OSTYPE" = "NetBSD" ] || [ "$OSTYPE" = "OpenBSD" ]; then
    if [ "$DISTNAME" = "Debian" ]; then
        echo "     sudo userdel ftnbbs"
        echo "     sudo userdel -r ftnd"
        echo "     sudo groupdel ftnbbs"
    else
        echo "     userdel ftnbbs"
        echo "     userdel -r ftnd"
        echo "     groupdel ftnbbs"
    fi
fi
if [ "$OSTYPE" = "FreeBSD" ]; then
    echo "     pw userdel ftnbbs -r"
    echo "     pw userdel ftnd -r"
    echo "     pw groupdel ftnbbs"
fi

