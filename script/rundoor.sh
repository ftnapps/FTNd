#!/bin/bash
#
# $Id: rundoor.sh,v 1.8 2004/11/05 11:53:47 mbse Exp $
#
# Never call this script directly, create a symlink to this file with the 
# name of the door. For example to run the door ilord do:
#   cd /opt/mbse/bin
#   ln -s rundoor.sh ilord
#
# In the bbs menu use the following line for Opt. Data:
#   /opt/mbse/bin/ilord /N [novirtual]
#
# The optional "novirtual" second paramter disables the use of the virtual
# comport in dosemu, this is for dos programs that are not doors.
#
# This is for dosemu 1.2.0 and later and runs the door as user.
#
# by Redy Rodriguez and Michiel Broek.

DOOR=`basename $0`
COMMANDO="\"doors $DOOR $*\r\""
DOSDRIVE=${MBSE_ROOT}/var/dosemu/c

# Prepare users home directory and node directory
if [ "$1" != "" ]; then
    mkdir -p $DOSDRIVE/doors/node$1 >/dev/null 2>&1
    # Copy door*.sys to dos partition
    cat ~/door.sys >$DOSDRIVE/doors/node$1/door.sys
    cat ~/door32.sys >$DOSDRIVE/doors/node$1/door32.sys
    # Create .dosemu directory for the user.
    if [ ! -d $HOME/.dosemu ]; then
	mkdir $HOME/.dosemu
    fi
    # Looks cheap, see above, but this does an upgrade too
    if [ ! -d $HOME/.dosemu/drives ]; then
        mkdir $HOME/.dosemu/drives
    fi
    # Create .dosemu/disclaimer in user home to avoid warning
    if [ ! -f $HOME/.dosemu/disclaimer ]; then
        touch $HOME/.dosemu/disclaimer
    fi
else
    exit 1    
fi

# run the dos emulator with the door.
if [ "$2" == "novirtual" ]; then
    /usr/bin/dosemu.bin -f ${MBSE_ROOT}/etc/dosemu/dosemu.conf -I "`echo -e keystroke $COMMANDO`"
else
    /usr/bin/dosemu.bin -f ${MBSE_ROOT}/etc/dosemu/virtual.conf -I "`echo -e keystroke $COMMANDO`"
fi

