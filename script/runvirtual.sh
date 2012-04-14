#!/bin/bash
#
# runvirtual.sh
#
# runvirtual.sh - Never call this script directly, create a symlink
#              to this file with the name of the door. For example
#              tu run the door ilord do:
#              cd /opt/ftnd/bin
#              ln -s runvirtual.sh ilord
#              In the menu use the following line for Optional Data:
#              /opt/ftnd/bin/ilord /N
#
# This version support a virtual COMport, needed by some doors.
#
# by Redy Rodriguez and Michiel Broek.
#
DOOR=`basename $0`
COMMANDO="\"doors $DOOR $*\r\""

/opt/ftnd/bin/bbsdoor.sh $DOOR $1
/usr/bin/sudo /opt/dosemu/bin/dosemu.bin -f /opt/ftnd/etc/dosemu/dosemu.conf \
    -I "`echo -e serial { com 1 virtual }"\n" keystroke $COMMANDO`"
reset
tput reset
stty sane
