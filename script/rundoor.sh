#!/bin/bash
#
# $Id$
#
# rundoor.sh - Never call this script directly, create a symlink
#              to this file with the name of the door. For example
#              to run the door ilord do:
#              cd /opt/mbse/bin
#              ln -s rundoor.sh ilord
#              In the menu use the following line for Optional Data:
#              /opt/mbse/bin/ilord /N
#
# This version DOES NOT have virtual COMport support, see runvirtual.sh
#
# by Redy Rodriguez and Michiel Broek.
#
DOOR=`basename $0`
COMMANDO="\"doors $DOOR $*\r\""

/opt/mbse/bin/bbsdoor.sh $DOOR $1
/usr/bin/sudo /usr/bin/dosemu.bin -f /opt/mbse/etc/dosemu/dosemu.conf -I "`echo -e keystroke $COMMANDO`"
reset
tput reset
stty sane
