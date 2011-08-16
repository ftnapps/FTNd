#!/bin/sh
#
#  $Id: bbsdoor.sh,v 1.7 2004/08/11 14:18:31 mbse Exp $
#
#  Initialize DOS environment before starting a bbs door.
#  Parameters: $1 = name of the door
#	       $2 = the nodenumber for this session
#
#  by Redy Rodriguez, 22-Oct-2001

DOSDRIVE=/opt/mbse/var/dosemu/c

if [ "$1" != "" ]; then
    if [ "$2" != "" ]; then
	mkdir -p $DOSDRIVE/doors/node$2 >/dev/null 2>&1
	# Copy door.sys to dos partition
	cat ~/door.sys >$DOSDRIVE/doors/node$2/door.sys
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
    fi
fi
