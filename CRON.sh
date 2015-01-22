#!/bin/sh
#
# CRON.sh
#
# Crontab setup script for FTNd

echo "FTNd for Unix crontab setup. Checking your system..."

# Basic checks.
if [ `whoami` != "ftnd" ]; then
cat << EOF
*** Run $0 as "ftnd" user only! ***

  Because the crontab for ftnd must be changed, you must be ftnd.

*** SETUP aborted ***
EOF
	exit 2
fi

if [ "$FTND_ROOT" = "" ]; then
	echo "*** The FTND_ROOT variable doesn't exist ***"
	echo "*** SETUP aborted ***"
	exit 2
fi

if [ "`grep ftnd: /etc/passwd`" = "" ]; then
	echo "*** User 'ftnd' does not exist on this system ***"
	echo "*** SETUP aborted ***"
	exit 2
fi

if [ "`crontab -l`" != "" ]; then
	echo "*** User 'ftnd' already has a crontab ***"
	echo "*** SETUP aborted ***"
	exit 2
fi

MHOME=$FTND_ROOT

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

echo "Installing FTNd crontab..."

crontab - << EOF
#-------------------------------------------------------------------------
#
#  Crontab for FTNd.
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
#00 02 * * *    export FTND_ROOT=$MHOME; \$FTND_ROOT/bin/mbout poll f544.n120.z1 -quiet
#00 03 * * *    export FTND_ROOT=$MHOME; \$FTND_ROOT/bin/mbout stop f544.n120.z1 -quiet

EOF

echo "Done."

