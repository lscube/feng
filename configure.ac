dnl Process this file with autoconf to produce a configure script.
AC_INIT(fenice, 1.5)
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

AC_PREFIX_PROGRAM(fenice)

test "$prefix" = . || test "$prefix" = NONE && prefix=/usr/local
test "$exec_prefix" = NONE && exec_prefix=$prefix
test "x$sysconfdir" = 'x${prefix}/etc' && sysconfdir="/etc"
test "x$localstatedir" = 'x${prefix}/var' && localstatedir="/var"

AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(debug,
[  --enable-debug          enable gcc dubugging flags [default=no]],,
	enable_debug="no")
AC_ARG_ENABLE(warnings,
[  --enable-warnings       enable/choose gcc warnings level: none, normal, pedantic [default=none]],,
	enable_warnings="none")
AC_ARG_ENABLE(errors,
[  --enable-errors         make gcc warnings behave like errors: none, normal, pedantic [default=none]],,
	enable_errors="none")
AC_ARG_ENABLE(optimize,
[  --enable-optimize       enable/chose optimization level: none, poor, normal, full, super [default=normal]],,
	enable_optimize="normal")

AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Check for LibTool Dynamic Loading
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

dnl Check for Endianess
AC_C_BIGENDIAN

dnl Checks for libraries.
AC_CHECK_LIB(pthread,pthread_create)
dnl AC_CHECK_LIB(avcodec,avcodec_open)

dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, main)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h unistd.h strings.h errno.h fcntl.h malloc.h sys/ioctl.h) 
dnl AC_CHECK_HEADERS(ffmpeg/common.h ffmpeg/avcodec.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_C_INLINE
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(select socket gettimeofday uname getcwd getwd strcspn strdup strtoul strerror strstr setenv)
AC_CHECK_FUNC(getaddrinfo)

CFLAGS=""


AM_CONDITIONAL(FENICE_DEBUG, /bin/false)
if test "$enable_debug" = "yes"; then
	CFLAGS="$CFLAGS -g -ggdb"
	AM_CONDITIONAL(FENICE_DEBUG, /bin/true)
	AC_DEFINE(ENABLE_DEBUG, 1,[Debug enabled])
fi

case "$enable_warnings" in
	pedantic)
		CFLAGS="$CFLAGS -W -Wall -pedantic"
	;;
	normal | yes)
		CFLAGS="$CFLAGS -W -Wall"
	;;
	none | *)
		CFLAGS="$CFLAGS"
	;;
esac

case "$enable_errors" in
	pedantic)
		CFLAGS="$CFLAGS -pedantic-errors -Werror"
	;;
	normal | yes)
		CFLAGS="$CFLAGS -Werror"
	;;
	none | *)
		CFLAGS="$CFLAGS"
	;;
esac

case "$enable_optimize" in
	poor)
		CFLAGS="$CFLAGS -O1" 
	;;
	full)
		CFLAGS="$CFLAGS -O3"
	;;
	super)
		CFLAGS="$CFLAGS -O3 -funroll-loops -fstrength-reduce -fomit-frame-pointer -ffast-math -malign-functions=2 -malign-jumps=2 -malign-loops=2"
	;;
	none)
		CFLAGS="$CFLAGS"
	;;
	normal | *)
		CFLAGS="$CFLAGS -O2"
	;;
esac


ARCH_DEFINES=""
case "$host" in 
	i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
		CFLAGS="$CFLAGS -mcpu=i486"
		ARCH_DEFINES="-DI386_ASSEM"
		arch_type=ix86
	;;
	ppc-*-*)
		arch_type=ppc
	;;
	*-hpux-*)
		ARCH_DEFINES="-DHPUX"
	;;
	*-sun-*)
		have_solaris=yes
	;;
esac


AC_SUBST(ARCH_DEFINES)
AM_CONDITIONAL(ARCH_X86, test "x$arch_type" = "xix86")
AM_CONDITIONAL(ARCH_PPC, test "x$arch_type" = "xppc")
AM_CONDITIONAL(HAVE_SOLARIS, test "x$have_solaris" = xyes)

fenice_avroot_dir=$localstatedir/$PACKAGE_NAME/avroot
AC_SUBST(fenice_avroot_dir)
fenice_conf_dir=$sysconfdir
AC_SUBST(fenice_conf_dir)
fenice_docs_dir=$datadir/doc/$PACKAGE_NAME-$PACKAGE_VERSION
AC_SUBST(fenice_docs_dir)

AC_DEFINE_UNQUOTED(FENICE_AVROOT_DIR_DEFAULT, "$fenice_avroot_dir", [Define default directory for Fenice A/V resources])
AC_DEFINE(FENICE_AVROOT_DIR_ENV, "FENICE_AVROOT_DIR",[Define the name for the ENV variable for Fenice avroot dir])

AC_DEFINE_UNQUOTED(FENICE_CONF_DIR_DEFAULT, "$fenice_conf_dir", [Define default directory for Fenice configuration])
AC_DEFINE(FENICE_CONF_DIR_ENV, "FENICE_CONF_DIR",[Define the name for the ENV variable for Fenice configuration dir])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([eventloop/Makefile])
AC_CONFIG_FILES([rtp/Makefile])
AC_CONFIG_FILES([rtcp/Makefile])
AC_CONFIG_FILES([intnet/Makefile])
AC_CONFIG_FILES([rtsp/Makefile])
AC_CONFIG_FILES([mediainfo/Makefile])
AC_CONFIG_FILES([sdp/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([md5/Makefile])
AC_CONFIG_FILES([socket/Makefile])
AC_CONFIG_FILES([schedule/Makefile])
AC_CONFIG_FILES([xmlparser/Makefile])
AC_CONFIG_FILES([command_environment/Makefile])
AC_CONFIG_FILES([docs/Makefile])
AC_CONFIG_FILES([docs/en/Makefile])
AC_CONFIG_FILES([etc/Makefile])
AC_CONFIG_FILES([avroot/Makefile])
AC_CONFIG_FILES([bufferpool/Makefile])
AC_OUTPUT

AC_MSG_NOTICE([config files successfully generated!
 
Fenice Install prefix .......... : $prefix
Fenice Binary path ............. : $exec_prefix/bin/fenice
Fenice Configuration path .......: $fenice_conf_dir
Fenice A/V Resources path .......: $fenice_avroot_dir
 
 
 'make' will now compile Fenice and 'su -c make install' will install it.
 Note: On non-Linux systems you might need to use 'gmake' instead of 'make'.
])
dnl

