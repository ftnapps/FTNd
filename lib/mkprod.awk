# $Id: mkprod.awk,v 1.6 2004/02/21 14:24:04 mbroek Exp $
#

BEGIN	{
		print "#include \"../config.h\""
		print "#include \"mbselib.h\""
		print ""
		print "struct _ftscprod ftscprod[] = {"
	}
/^[^;]/	{
		if ($2 != "DROPPED")
			print "	{0x" $1 ",(char *)\"" $2 "\"},"
	}
END	{
		print "	{0xff,(char*)0L}"
		print "};"
	}
