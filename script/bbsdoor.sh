#!/bin/sh
#
#  Initialize DOS environment before starting a bbs door.
#
#  by Redy Rodriguez, 21-Oct-2001

if [ "$1" != "" ]; then
    if [ "$2" != "" ]; then
	mkdir -p /dos/c/doors/node$2 >/dev/null 2>&1
	# Copy door.sys to dos partition
	cat ~/door.sys >/dos/c/doors/node$2/door.sys
	# Create .dosemu/disclaimer in user home to avoid warning
	if [ ! -d ~/.dosemu ]; then
	    mkdir ~/.dosemu
	fi
	if [ ! -f ~/.dosemu/disclaimer ]; then
	    touch ~/.dosmenu/disclaimer
	fi
    fi
fi
