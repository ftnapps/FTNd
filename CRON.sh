#!/bin/sh
#
# $Id: CRON.sh,v 1.6 2005/08/26 10:42:43 mbse Exp $
#
# Crontab setup script for MBSE BBS

echo "MBSE BBS for Unix crontab setup. Checking your system..."

# Basic checks.
if [ `whoami` != "mbse" ]; then
cat << EOF
*** Run $0 as "mbse" user only! ***

  Because the crontab for mbse must be changed, you must be mbse.

*** SETUP aborted ***
EOF
	exit 2
fi

if [ "$MBSE_ROOT" = "" ]; then
	echo "*** The MBSE_ROOT variable doesn't exist ***"
	echo "*** SETUP aborted ***"
	exit 2
fi

if [ "`grep mbse: /etc/passwd`" = "" ]; then
	echo "*** User 'mbse' does not exist on this system ***"
	echo "*** SETUP aborted ***"
	exit 2
fi

if [ "`crontab -l`" != "" ]; then
	echo "*** User 'mbse' already has a crontab ***"
	echo "*** SETUP aborted ***"
	exit 2
fi

MHOME=$MBSE_ROOT

clear
cat << EOF
    Everything looks allright to install the default crontab now.
    If you didn't install all bbs programs yet, you better hit
    Control-C now and run this script when everything is installed.
    If you insist on installing the crontab without the bbs is complete
    you might get a lot of mail from cron complaining about errors.

    The default crontab will have entries for regular maintenance.
    You need to add entries to start and stop polling fidonet uplinks.
    There is a example at the bottom of the crontab which is commented
    out of course.

    On most systems you can edit the crontab by typing "crontab -e".

EOF

echo -n "Hit Return to continue or Control-C to abort: "
read junk

echo "Installing MBSE BBS crontab..."

crontab - << EOF
#-------------------------------------------------------------------------
#
#  Crontab for mbse bbs.
#
#-------------------------------------------------------------------------

# User maintenance etc. Just do it sometime when it's quiet.
00 09 * * *	$MHOME/etc/maint

# Midnight event at 00:00.
00 00 * * *	$MHOME/etc/midnight

# Weekly event at Sunday 00:15.
15 00 * * 0	$MHOME/etc/weekly

# Monthly event at the 1st day of the month at 01:30.
30 00 1 * *	$MHOME/etc/monthly

#-----------------------------------------------------------------------------
#
#  From here you should enter your outgoing mailslots, when to send mail etc.

# Mail slot example.
#00 02 * * *	export MBSE_ROOT=$MHOME; \$MBSE_ROOT/bin/mbout poll f16.n2801.z2 -quiet
#00 03 * * *	export MBSE_ROOT=$MHOME; \$MBSE_ROOT/bin/mbout stop f16.n2801.z2 -quiet

EOF

echo "Done."

