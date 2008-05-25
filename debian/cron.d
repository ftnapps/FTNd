#-------------------------------------------------------------------------
#
#  Crontab for MBSE BBS.
#
#-------------------------------------------------------------------------

# User maintenance etc. Just do it sometime when it's quiet.
00 09 * * *	/opt/mbse/bin/adm/maint

# Midnight event at 00:00.
00 00 * * *	/opt/mbse/bin/adm/midnight

# Weekly event at Sunday 00:15.
15 00 * * 0	/opt/mbse/bin/adm/weekly

# Monthly event at the 1st day of the month at 01:30.
30 00 1 * *	/opt/mbse/bin/adm/monthly

#-----------------------------------------------------------------------------
#
#  From here you should enter your outgoing mailslots, when to send mail etc.

# Mail slot example.
#00 02 * * *	export MBSE_ROOT=/opt/mbse; \$MBSE_ROOT/bin/mbout poll f16.n2801.z2 -quiet
#00 03 * * *	export MBSE_ROOT=/opt/mbse; \$MBSE_ROOT/bin/mbout stop f16.n2801.z2 -quiet

