#!/bin/sh
#
#  Initialize DOS environment before starting a bbs door.
#  Parameters: $1 = name of the door
#	       $2 = the nodenumber for this session
#
#  by Redy Rodriguez, 22-Oct-2001

if [ "$1" != "" ]; then
    if [ "$2" != "" ]; then
	mkdir -p /dos/c/doors/node$2 >/dev/null 2>&1
	# Copy door.sys to dos partition
	cat ~/door.sys >/dos/c/doors/node$2/door.sys
	# Create .dosemu/disclaimer in user home to avoid warning
	if [ ! -d $HOME/.dosemu ]; then
	    mkdir $HOME/.dosemu
	fi
	if [ ! -f $HOME/.dosemu/disclaimer ]; then
	    touch $HOME/.dosemu/disclaimer
	fi
    fi
fi
