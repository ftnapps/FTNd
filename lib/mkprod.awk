BEGIN	{
		print "#include \"libs.h\""
		print "#include \"structs.h\""
		print "#include \"common.h\""
		print ""
		print "struct _ftscprod ftscprod[] = {"
	}
/^[^;]/	{
		if ($2 != "DROPPED")
			print "	{0x" $1 ",\(char \*\)\"" $2 "\"},"
	}
END	{
		print "	{0xff,(char*)0L}"
		print "};"
	}
