#!/bin/bash
#
# $Id$
#
# rundoor.sh - Never call this script directly, create a symlink
#              to this file with the name of the door. For example
#              tu run the door ilord do:
#              cd /opt/mbse/bin
#              ln -s rundoor.sh ilord
#
# by Redy Rodriguez and Michiel Broek.
#
DOOR=`basename $0`
COMMANDO="\"doors $DOOR $*\r\""

/usr/bin/sudo /opt/mbse/bin/bbsdoor.sh $DOOR $1
/usr/bin/sudo /opt/dosemu/bin/dosemu.bin \
	-F /var/lib/dosemu/global.conf \
	-I "`echo -e serial { com 1 virtual }"\n" keystroke $COMMANDO`"
reset
tput reset
stty sane
