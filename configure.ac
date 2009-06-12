dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
AC_INIT([feng], [2.1.0_rc1])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT("/usr")

AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], [
  AC_MSG_ERROR([no C99 compiler found, feng requires a C99 compiler.])
])

AC_PROG_RANLIB

AC_USE_SYSTEM_EXTENSIONS
dnl Force usage of POSIX.1-2001 (clock_gettime)
CPPFLAGS="${CPPFLAGS} -D_POSIX_C_SOURCE=200112L"

dnl Enable some further OS-specific extensions that AC_USE_SYSTEM_EXTENSIONS
dnl does not enable at least up to autoconf 2.63.
dnl These extensions are needed to have struct ip_mreq for multicast support.
AC_CANONICAL_HOST
AS_CASE([$host_os],
	[darwin*], [CPPFLAGS="${CPPFLAGS} -D_DARWIN_C_SOURCE"],
	[freebsd7*], [CPPFLAGS="${CPPFLAGS} -D__BSD_VISIBLE=1"])

AC_CONFIG_HEADER([config.h])

AC_ARG_WITH(rtsp-port,
    AS_HELP_STRING([--with-rtsp-port], [default RTSP listening port (default=554, alternative=8554)]),,
    with_rtsp_port=554)

AC_ARG_WITH(config-file,
    AS_HELP_STRING([--with-config-file], [name for the configuration file in sysconfdir (default=feng.conf)]),,
    with_config_file="feng.conf")

AC_ARG_WITH(log-file,
    AS_HELP_STRING([--with-log-file], [name for the log file (default=feng.log)]),,
    with_log_file="feng.log")

AC_ARG_WITH(max-session,
    AS_HELP_STRING([--with-max-session], [max number of incoming sessions (default=100)]),,
    with_max_session=100)

AC_ARG_ENABLE(libavformat,
    AS_HELP_STRING([--enable-libavformat], [enable ffmpeg's libavformat support (default=yes)]),,
        enable_ffmpeg="yes")

AC_CHECK_HEADERS([ev.h], [
  AC_CHECK_LIB([ev], [ev_io_start],, [have_libev=no])], [have_libev=no])

AS_IF([test "x$have_libev" = "xno"],
      [AC_MSG_ERROR([libev not found, feng requires libev])])

AC_ARG_ENABLE(trace,
    AS_HELP_STRING([--enable-trace], [enable additional informations in log [[default=no]]]),,
    enable_trace="no")

AC_ARG_ENABLE(metadata,
    AS_HELP_STRING([--enable-metadata], [enable metadata support [[default=no]]]),,
    enable_metadata="no")

dnl Checks used by feng itself
AC_CHECK_HEADERS_ONCE([syslog.h])

AC_SEARCH_LIBS([clock_gettime], [rt],
  [AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [Define this if you have clock_gettime])])

CC_ATTRIBUTE_DESTRUCTOR

AH_BOTTOM([#if !defined(NDEBUG) && defined(SUPPORT_ATTRIBUTE_DESTRUCTOR)
	   # define CLEANUP_DESTRUCTOR __attribute__((__destructor__))
	   #endif
	  ])

dnl check if we have the generic struct for net addresses the has max
dnl possible size
AC_CHECK_TYPES(struct sockaddr_storage,,, [#include <sys/socket.h>])

dnl Checks used by conf/*
dnl AC_CHECK_HEADERS([pcre.h fam.h])
dnl AC_CHECK_FUNCS([lstat])

CHECK_RAGEL_AM([src/network/ragel_range.c])

fenice_state_dir=$(eval echo ${localstatedir}/feng)
AC_SUBST(fenice_state_dir)

LSC_CHECK_IPV6
LSC_CHECK_SCTP
LIBS="$LIBS $SCTP_LIBS"

AS_IF([test "x$have_sctp" = "xyes"],
  [LIBNETEMBRYO_REQUIRES="libnetembryo-sctp >= 0.1.0"],
  [LIBNETEMBRYO_REQUIRES="libnetembryo >= 0.1.0"])

PKG_CHECK_MODULES(LIBNETEMBRYO,[$LIBNETEMBRYO_REQUIRES])
AC_SUBST(LIBNETEMBRYO_REQUIRES)
CPPFLAGS="$CPPFLAGS $LIBNETEMBRYO_CFLAGS"
LIBS="$LIBS $LIBNETEMBRYO_LIBS"

dnl Check for prerequisites to enable live streaming with flux.
live_streaming=yes

AC_SEARCH_LIBS([mq_open], [rt],
               [AC_DEFINE(LIVE_STREAMING, [1],
                          [Define this if live streaming is supported])],
               [live_streaming=no])

AM_CONDITIONAL([LIVE_STREAMING], [test "x$live_streaming" = "xyes"])

if test "x$enable_metadata" = "xyes"; then
dnl MYSQL
AX_LIB_MYSQL([5.0])
LIBS="$LIBS $MYSQL_LDFLAGS"
CFLAGS="$CFLAGS $MYSQL_CFLAGS"
AC_DEFINE(HAVE_METADATA, [1], [Define this if metadata support must be included])
fi

AM_CONDITIONAL(FENG_CPD, test "x$enable_metadata" = "xyes")

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.16 gthread-2.0])
CFLAGS="$CFLAGS $GLIB_CFLAGS"
LIBS="$LIBS $GLIB_LIBS"

avformat_msg="no"
avutil_msg="no"
if test "x$enable_ffmpeg" = "xyes"; then
    PKG_CHECK_MODULES(LIBAVFORMAT,[libavformat >= 52.7.0 libavcodec >= 51],
        [AC_DEFINE(HAVE_AVFORMAT,,[Define if libavformat support is available])
         avformat_msg="yes"
         ])
         CFLAGS="$CFLAGS $LIBAVFORMAT_CFLAGS"
         LIBS="$LIBS $LIBAVFORMAT_LIBS"
        PKG_CHECK_MODULES(LIBAVUTIL,[libavutil >= 49.6.0],
        [AC_DEFINE(HAVE_AVUTIL,,[Define if libavutil support is available])
         avutil_msg="yes"],
         [avutil_msg="no, using internal"])
         CFLAGS="$CFLAGS $LIBAVUTIL_CFLAGS"
         LIBS="$LIBS $LIBAVUTIL_LIBS"
fi

AM_CONDITIONAL(FENICE_FFMPEG, [test "$enable_ffmpeg" = "yes"])

if test "$enable_duma" = "yes"; then
    AC_DEFINE(ENABLE_DUMA, 1,[Debug enabled])
    enable_debug=yes
    LIBS="$LIBS -lduma"
fi

CC_ATTRIBUTE_UNUSED(
  [AC_DEFINE([ATTR_UNUSED], [__attribute__((__unused__))], [Define this if ATTR_UNUSED is supported])],
  [AC_DEFINE([ATTR_UNUSED], [], [Define this if ATTR_UNUSED is not supported])])

LSC_ERRORS
LSC_DEBUG

if test "$enable_trace" = "yes"; then
    AC_DEFINE(TRACE, 1,[Trace enabled])
fi

fenice_avroot_dir=$(eval echo $localstatedir/$PACKAGE_NAME/avroot|sed -e "s:/\+:/:g; s:NONE::g")

AC_SUBST(fenice_avroot_dir)

fenice_max_session=$(eval echo $with_max_session)
AC_SUBST(fenice_max_session)

fenice_rtsp_port=$(eval echo $with_rtsp_port)
AC_SUBST(fenice_rtsp_port)

fenice_conf_file=$(eval echo $with_config_file|sed -e "s:/\+:/:g")
AC_SUBST(fenice_conf_file)


feng_basedir=$(eval echo $localstatedir|sed -e "s:/\+:/:g; s:NONE::g")

feng_logdir="${feng_basedir}/log/$PACKAGE_NAME"
AC_SUBST(feng_logdir)

fenice_log_file="${feng_basedir}/log/$with_log_file"
AC_SUBST(fenice_log_file)

feng_statedir="${feng_basedir}/lib/$PACKAGE_NAME"
AC_SUBST(feng_statedir)

feng_basedir="${feng_basedir}/$PACKAGE_NAME"
AC_SUBST(feng_basedir)

fenice_conf_dir=$(eval echo $sysconfdir|sed -e "s:/\+:/:g; s:NONE::g")
AC_SUBST(fenice_conf_dir)

fenice_docs_dir=$(eval echo $datadir/doc/$PACKAGE_NAME|sed -e "s:/\+:/:g")
AC_SUBST(fenice_docs_dir)

AC_DEFINE_UNQUOTED(FENICE_MAX_SESSION_DEFAULT,
                    $fenice_max_session,
                    [Define max number of RTSP incoming sessions for Feng])

AC_DEFINE_UNQUOTED(FENICE_RTSP_PORT_DEFAULT,
                    $fenice_rtsp_port,
                    [Define default RTSP listening port])

AC_DEFINE_UNQUOTED(FENICE_AVROOT_DIR_DEFAULT,
                    $fenice_avroot_dir,
                    [Define default directory for Feng A/V resources])

AC_DEFINE_UNQUOTED(FENICE_AVROOT_DIR_DEFAULT_STR,
                    "$fenice_avroot_dir",
                    [Define default directory string for Feng A/V resources])

AC_DEFINE_UNQUOTED(FENICE_CONF_FILE_DEFAULT,
                    $fenice_conf_file,
                    [Define default file for Feng configuration])

AC_DEFINE_UNQUOTED(FENICE_LOG_FILE_DEFAULT,
                    $fenice_log_file,
                    [Define default file for Feng logger])

AC_DEFINE_UNQUOTED(FENICE_LOG_FILE_DEFAULT_STR,
                    "$fenice_log_file",
                    [Define default string for Feng log file])

AC_DEFINE_UNQUOTED(FENICE_CONF_DIR_DEFAULT,
                    $fenice_conf_dir,
                    [Define default directory for Feng configuration])

AC_DEFINE_UNQUOTED(FENICE_CONF_PATH_DEFAULT_STR,
                    "$(eval echo ${fenice_conf_dir}/${fenice_conf_file}|sed -e s:/\+:/:g)",
                    [Define absolute path string for Feng configuration file])

AC_DEFINE_UNQUOTED(FENICE_STATE_DIR,
                    $fenice_state_dir,
                    [Define default dir for Certificate (pem format)])

AC_DEFINE_UNQUOTED(FENIC_STATE_DIR_STR,
                    "$fenice_state_dir",
                    [Define default string dir for Certificate (pem format)])

AC_MSG_CHECKING([whether we need git to generate ChangeLog])
AS_IF([test -f "$srcdir/ChangeLog"],
  [AC_MSG_RESULT([no, we're good])],
  [AC_MSG_RESULT([yes])
   AC_CHECK_PROG([GIT], [git], [yes], [no])
   AS_IF([test "x$GIT" = "xno"],
     [AC_MSG_ERROR([You need git to use a git snapshot!])])
  ])

AM_CONDITIONAL([CHANGELOG_REGEN], [test "x$GIT" = "xyes"])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([etc/feng.conf])
AC_CONFIG_FILES([etc/feng-uninstalled.conf])
AC_CONFIG_FILES([Doxyfile])
AC_OUTPUT

LSC_DEBUG_STATUS
AC_MSG_NOTICE([config files successfully generated!

Feng Install prefix .......... : $prefix
Feng Binary path ............. : $(eval echo $exec_prefix/bin/feng)
Feng Configuration path ...... : $fenice_conf_dir
Feng Log File ................ : $fenice_log_file
Feng Multimedia Files root ... : $fenice_avroot_dir
Feng RTSP listening port ..... : $fenice_rtsp_port

live streaming supported...... : $live_streaming
sctp support enabled ......... : $enable_sctp
avformat support enabled ..... : $avformat_msg
avutil support enabled ....... : $avutil_msg
 duma enabled ................ : $enable_duma

metadata support enabled...... : $enable_metadata

 'make' will now compile Feng and 'su -c make install' will install it.
 Note: On non-Linux systems you might need to use 'gmake' instead of 'make'.
])

