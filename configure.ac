# Process this file with autoconf to produce a configure script.
# $Id: configure.ac,v 1.29 2008/11/29 13:59:33 mbse Exp $

AC_INIT(lib/mbselib.h)
AM_CONFIG_HEADER(config.h)
SUBDIRS="lib mbcico mbfido mbmon mbsebbs mbutils mbnntp mbtask mbsetup unix lang examples html script"
AC_SUBST(SUBDIRS)

#
# General settings for MBSE BBS
# After changeing the version number, run autoconf!
#
PACKAGE="mbsebbs"
MAJOR="0"
MINOR="95"
REVISION="5"
VERSION="$MAJOR.$MINOR.$REVISION"
COPYRIGHT="Copyright (C) 1997-2008 Michiel Broek, All Rights Reserved"
SHORTRIGHT="Copyright (C) 1997-2008 M. Broek"
GROUP="bbs"
OWNER="mbse"
ROWNER="`id -un root`"
RGROUP="`id -gn root`"
AC_SUBST(PACKAGE)
AC_SUBST(MAJOR)
AC_SUBST(MINOR)
AC_SUBST(REVISION)
AC_SUBST(COPYRIGHT)
AC_SUBST(GROUP)
AC_SUBST(OWNER)
AC_SUBST(ROWNER)
AC_SUBST(RGROUP)
AC_PREFIX_DEFAULT(/opt/mbse)
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_DEFINE_UNQUOTED(VERSION_MAJOR, $MAJOR)
AC_DEFINE_UNQUOTED(VERSION_MINOR, $MINOR)
AC_DEFINE_UNQUOTED(VERSION_REVISION, $REVISION)
AC_DEFINE_UNQUOTED(COPYRIGHT, "$COPYRIGHT")
AC_DEFINE_UNQUOTED(SHORTRIGHT, "$SHORTRIGHT")
AC_SUBST(VERSION)
SYSTEM="`uname -s`"
CPU="`uname -m`"
LDFLAGS="-L/usr/local/lib"
CPPFLAGS="-I/usr/local/include"

#
# Checks for programs.
# Try to find GNU make
#
AC_CHECK_PROG(MAKE, gmake, gmake)
AC_CHECK_PROG(MAKE, make, make)
# Alternate awk check, I skip mawk because it doesn't work for MBSE.
AC_CHECK_PROG(AWK, gawk, gawk)
AC_CHECK_PROG(AWK, nawk, nawk)
AC_CHECK_PROG(AWK, awk, awk)
# Try to find GNU install
AC_CHECK_PROG(INSTALL, ginstall, ginstall)
AC_CHECK_PROG(INSTALL, install, install)
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LN_S
AC_CHECK_PROG(TAR, tar, tar)
AC_CHECK_PROG(ZIP, zip, zip)
AC_PATH_PROG(CHOWN, chown, chown, /bin:/sbin:/usr/bin:/usr/sbin:)
AC_C_BIGENDIAN


#
# Additional commandline switches
#
AC_ARG_ENABLE(experiment,  [  --enable-experiment     Compile experimental code],     [ experiment=$enableval ], [ experiment=no ])
if test "$experiment" = "yes"; then
  AC_DEFINE(USE_EXPERIMENT)
fi

AC_ARG_ENABLE(debugging,   [  --enable-debugging      Compile for debugging], [ debugging=$enableval ], [ debugging=no ])
if test "$debugging" = "yes"; then
    CFLAGS="-O -g -Wall -Wshadow -Wwrite-strings -Wstrict-prototypes -D_REENTRANT -I/usr/local/include"
else
    CFLAGS="-O2 -fno-strict-aliasing -Wall -Wshadow -Wwrite-strings -Wstrict-prototypes -D_REENTRANT -I/usr/local/include"
fi 

AC_ARG_ENABLE(optimize,   [  --enable-optimize       Enable CPU optimize], [ optimize=$enableval ], [ optimize=no ])
if test "$optimize" = "yes"; then
AC_MSG_CHECKING(for cpuflags)
    for cpuflagsbin in /usr/bin /usr/local/bin /usr/pkg/bin `pwd` NONE; do
        if test "$cpuflagsbin" = "NONE"; then
            AC_MSG_RESULT(unknown)
        elif test -x $cpuflagsbin/cpuflags; then
	    CPUFLAGS=`$cpuflagsbin/cpuflags`
            AC_MSG_RESULT($CPUFLAGS)
	    CFLAGS="$CPUFLAGS $CFLAGS"
            break
        fi
    done
fi


#
# Enable full newsgate, this was default upto 0.50.0
#
AC_ARG_ENABLE(newsgate,  [  --enable-newsgate       Compile with newsgate],      [ newsgate=$enableval ], [ newsgate=no ])
if test "$newsgate" = "yes"; then
  AC_DEFINE(USE_NEWSGATE)
  NEWSGATE="Yes"
else
  NEWSGATE="No"
fi


#
# Checks for libraries and functions.
#
AC_CHECK_LIB(compat,re_comp,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lcompat"
fi

AC_CACHE_CHECK(for pw_age in struct passwd, ac_cv_struct_passwd_pw_age, AC_TRY_COMPILE([#include <pwd.h>],
[ struct passwd pw;  pw.pw_age = ""; ], ac_cv_struct_passwd_pw_age=yes, ac_cv_struct_passwd_pw_age=no))
if test "$ac_cv_struct_passwd_pw_age" = "yes"; then
        AC_DEFINE(ATT_AGE)
fi

AC_CACHE_CHECK(for pw_comment in struct passwd, ac_cv_struct_passwd_pw_comment, AC_TRY_COMPILE([#include <pwd.h>],
[ struct passwd pw;  pw.pw_comment = ""; ], ac_cv_struct_passwd_pw_comment=yes, ac_cv_struct_passwd_pw_comment=no))
if test "$ac_cv_struct_passwd_pw_comment" = "yes"; then
        AC_DEFINE(ATT_COMMENT)
fi

AC_CHECK_LIB(shadow,setspent,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lshadow"
  SHADOW_PASSWORD=1
  LIBSHADOW=1
else
  AC_CHECK_LIB(shadow,getspnam,result=yes,result=no)
  if test "$result" = "yes"; then
    LIBS="$LIBS -lshadow"
    SHADOW_PASSWORD=1
    LIBSHADOW=1
  else
    # some libc's (glibc 2.x) keep shadow functions in -lc
    AC_CHECK_LIB(c,setspent,result=yes,result=no)
    if test "$result" = "yes"; then
      if test -f /etc/shadow; then
        SHADOW_PASSWORD=1
      fi
    fi
  fi
fi

if test "$SHADOW_PASSWORD" = "1"; then
  if test "$ac_cv_func_fgetspent" != "yes"; then
    AC_CHECK_LIB(shadow,fgetspent,result=yes,result=no)
    if test "$result" = "yes"; then
      if test "$LIBSHADOW" != "1"; then
        LIBS="$LIBS -lshadow"
      fi
    fi
  fi
  AC_DEFINE(SHADOW_PASSWORD)
fi

AC_CHECK_LIB(crypt,crypt,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lcrypt"
fi

AC_CHECK_LIB(util,login,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lutil"
  AC_CHECK_HEADERS(util.h)
  AC_CHECK_HEADERS(libutil.h)
  AC_CHECK_HEADERS(sys/types.h)
fi


#
# Support for zlib and bzlib transfers compression
#
ZLIBSUP=No
AC_CHECK_LIB(z,compress2,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lz"
  AC_CHECK_HEADERS(zlib.h,ZLIBSUP=Yes,ZLIBSUP=No)
fi
BZLIBSUP=No
AC_CHECK_LIB(bz2,BZ2_bzCompressInit,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lbz2"
  AC_CHECK_HEADERS(bzlib.h,BZLIBSUP=Yes,BZLIBSUP=No)
fi

#
# Support for iconv, check libc first.
#
result="no"
AC_CHECK_FUNC(iconv_open,result=yes,
#  AC_SEARCH_LIBS(iconv_open,iconv,result=yes,result=no)
  AC_CHECK_LIB(iconv,iconv_open,result=yes,result=no)
  if test "$result" = "yes"; then
    LIBS="$LIBS -liconv"
  fi
)
if test "$result" = "yes"; then
  AC_CHECK_HEADERS(iconv.h)
fi

#
# Checks for header files.
#
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(termios.h sys/termios.h termio.h sys/termio.h sgtty.h)
AC_CHECK_HEADERS(sys/time.h sys/vfs.h unistd.h netinet/in.h regex.h)
AC_CHECK_HEADERS(sys/resource.h usersec.h gshadow.h shadow.h crypt.h)
AC_CHECK_HEADERS(limits.h utmp.h utmpx.h lastlog.h rpc/key_prot.h)
AC_STRUCT_TIMEZONE

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_STRUCT_ST_BLKSIZE
AC_HEADER_STAT
AC_STRUCT_TM

#
# Checks for library functions.
#
AC_CHECK_FUNCS(c64i a64l fchmod fchown fdatasync fsync lckpwdf strcasestr putpwent)
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS(gettimeofday re_comp select statfs statvfs)
AC_CHECK_FUNCS(getspnam initgroups updwtmp updwtmpx)

#
# Check for usefull external programs
#
AC_PATH_PROG(COMPRESS,compress,no-compress-found-during-configure)
AC_PATH_PROG(GZIP,gzip,no-gzip-found-during-configure)
AC_PATH_PROG(ARC,arc)
AC_PATH_PROG(NOMARCH,nomarch)
AC_PATH_PROG(ARJ,arj)
AC_PATH_PROG(UNARJ,unarj)
AC_PATH_PROG(LHA,lha)
AC_PATH_PROG(RAR,rar)
AC_PATH_PROG(UNRAR,unrar)
AC_PATH_PROG(TAR,tar)
AC_PATH_PROG(ZIP,zip)
AC_PATH_PROG(UNZIP,unzip)
AC_PATH_PROG(ZOO,zoo)
AC_PATH_PROG(HA,ha)
AC_PATH_PROG(ANTIVIR,antivir)
AC_PATH_PROG(FPROT,fpscan)
AC_PATH_PROG(UVSCAN,uvscan)
AC_PATH_PROG(CLAMAV,clamscan)
AC_PATH_PROG(RB,rb)
AC_PATH_PROG(RB,lrb)
AC_PATH_PROG(SB,sb)
AC_PATH_PROG(SB,lsb)
AC_PATH_PROG(RZ,rz)
AC_PATH_PROG(RZ,lrz)
AC_PATH_PROG(SZ,sz)
AC_PATH_PROG(SZ,lsz)
AC_PATH_PROG(GOLDEDBIN,golded)
AC_PATH_PROG(GOLDEDBIN,gedlnx)
AC_PATH_PROG(GOLDNODE,goldnode)
AC_PATH_PROG(GOLDNODE,gnlnx)
AC_PATH_PROG(CONVERT,convert)


#
# Check for GeoIP support, see http://www.maxmind.com/
#
GEOIPSUP=No
AC_CHECK_LIB(GeoIP,GeoIP_new,result=yes,result=no)
if test "$result" = "yes"; then
  LIBS="$LIBS -lGeoIP"
  AC_CHECK_HEADERS(GeoIP.h GeoIPCity.h,GEOIPSUP=Yes,GEOIPSUP=No)
fi


#
# FIX DEL for Del!=207, this was default upto 207 or 126 if no argument 
#
AC_ARG_ENABLE(gbkdel,      [  --enable-gbkdel         Delete key is 126 (default is 207)], [ gbkdel=$enableval ], [ gbkdel=no ])
if test "$gbkdel" = "yes"; then
  GBK_DEL=126
  DEL=$GBK_DEL
  echo "checking delete key... changed into $DEL"
else 
  DEL="default"
  GBK_DEL=0
  echo "checking delete key... default"
fi
AC_DEFINE_UNQUOTED(GBK_DEL,$GBK_DEL)


AC_MSG_CHECKING(location of joe editor)
for joebin in /usr/bin /usr/local/bin /usr/pkg/bin NONE; do
	if test "$joebin" = "NONE"; then
		AC_MSG_RESULT(not found)
	elif test -x $joebin/joe; then
		AC_SUBST(joebin)
		AC_MSG_RESULT($joebin)
		break
	fi
done
AC_MSG_CHECKING(location of joe rc files)
for joelib in /etc/joe /usr/lib/joe /usr/local/etc /usr/local/etc/joe /usr/local/lib/joe /usr/local/lib /usr/pkg/lib /usr/pkg/etc NONE; do
	if test "$joelib" = "NONE"; then
		AC_MSG_RESULT(not found)
	elif test -f $joelib/joerc; then
		AC_SUBST(joelib)
		AC_MSG_RESULT($joelib)
		break
	fi
done

AC_MSG_CHECKING(location of utmp)
for utmpdir in /var/run /var/adm /usr/adm /etc NONE; do
        if test "$utmpdir" = "NONE"; then
                AC_MSG_WARN(utmp file not found)
        elif test -f $utmpdir/utmp; then
                AC_DEFINE_UNQUOTED(_UTMP_FILE, "$utmpdir/utmp")
                AC_MSG_RESULT($utmpdir)
                break
        fi
done

AC_MSG_CHECKING(location of lastlog/wtmp)
for logdir in /var/log /var/adm /usr/adm /etc; do
        if test -d $logdir; then
                AC_DEFINE_UNQUOTED(_WTMP_FILE, "$logdir/wtmp")
                AC_DEFINE_UNQUOTED(LASTLOG_FILE, "$logdir/lastlog")
                AC_MSG_RESULT($logdir)
                break
        fi
done

AC_MSG_CHECKING(location of vpopmail)
for vpop in /var/qmail/vpopmail/bin NONE; do
	if test "$vpop" = "NONE"; then
		AC_MSG_RESULT(not found)
	elif test -x $vpop/vadduser; then
		AC_DEFINE_UNQUOTED(_VPOPMAIL_PATH, "$vpop")
		AC_MSG_RESULT($vpop)
		break
	fi
done


AC_OUTPUT(
	Makefile.global
	paths.h
	unix/login.defs
	INSTALL 
	FILE_ID.DIZ
	script/editor
	html/index.htm
	html/basic.html
	html/upgrade.html
)

#=================================================================================

AC_MSG_RESULT([
-=-=-=-=-=-=-=-=-=-= Configuration Complete =-=-=-=-=-=-=-=-=-=-

  Configuration summary :

    Version : ..................... ${VERSION}
    Hydra/Binkp zlib compression :  ${ZLIBSUP}
    Binkp bzlib compression : ..... ${BZLIBSUP}
    Full newsgate : ............... ${NEWSGATE}
    GeoIP support : ............... ${GEOIPSUP}
    Delete key : .................. ${DEL}

])

if test x$ac_cv_c_compiler_gnu = xyes ; then
        echo -e "  Tools :\n"
        echo "    Compiler is GCC : ............. ${ac_cv_c_compiler_gnu}"
        fi

AC_MSG_RESULT([
  Installation directories :

    Main directory    : ........... ${prefix}
    Owner and group   : ........... ${OWNER}.${GROUP}

  Now type 'make' and as root 'make install'
])
