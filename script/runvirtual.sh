#!/bin/bash
#
# $Id: runvirtual.sh,v 1.2 2004/08/09 21:30:30 mbse Exp $
#
# runvirtual.sh - Never call this script directly, create a symlink
#              to this file with the name of the door. For example
#              tu run the door ilord do:
#              cd /opt/mbse/bin
#              ln -s runvirtual.sh ilord
#              In the menu use the following line for Optional Data:
#              /opt/mbse/bin/ilord /N
#
# This version support a virtual COMport, needed by some doors.
#
# by Redy Rodriguez and Michiel Broek.
#
DOOR=`basename $0`
COMMANDO="\"doors $DOOR $*\r\""

/opt/mbse/bin/bbsdoor.sh $DOOR $1
/usr/bin/sudo /opt/dosemu/bin/dosemu.bin -f /opt/mbse/etc/dosemu/dosemu.conf \
	-I "`echo -e serial { com 1 virtual }"\n" keystroke $COMMANDO`"
reset
tput reset
stty sane
