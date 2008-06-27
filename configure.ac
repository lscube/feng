dnl Process this file with autoconf to produce a configure script.
AC_INIT(feng, 0.1.99)
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE(dist-bzip2 check-news)

AC_PREFIX_DEFAULT("/usr")

AM_CONFIG_HEADER(config.h)

AC_ARG_WITH(rtsp-port,
[  --with-rtsp-port        default RTSP listening port [[default=554, alternative=8554]]],,
    with_rtsp_port=554)

AC_ARG_WITH(config-file,
[  --with-config-file      name for the configuration file in sysconfdir [[default=feng.conf]]],,
    with_config_file="feng.conf")

AC_ARG_WITH(log-file,
[  --with-log-file         name for the log file [[default=feng.log]]],,
    with_log_file="feng.log")

AC_ARG_WITH(max-session,
[  --with-max-session      max number of incoming sessions [[default=100]]],,
    with_max_session=100)

AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           enable IPv6 support [[default=autodetected]]],,
        enable_ipv6="yes")

AC_ARG_ENABLE(libavformat,
[  --enable-libavformat    enable ffmpeg's libavformat support [[default=yes]]],,
        enable_ffmpeg="yes")

AC_ARG_ENABLE(debug,
[  --enable-debug          enable gcc dubugging flags [[default=no]]],,
    enable_debug="no")

AC_ARG_ENABLE(trace,
[  --enable-trace          enable additional informations in log [[default=no]]],,
    enable_trace="no")

AC_ARG_ENABLE(duma,
[  --enable-duma           link against DUMA library to debug memory usage
                          (implies --enable-debug) [[default=no]]],,
    enable_duma="no")

AC_ARG_ENABLE(mudflap,
[  --enable-mudflap        enable mudflap support
                          (implies --enable-debug) [[default=no]]],,
    enable_mudflap="no")

AC_ARG_ENABLE(errors,
[  --enable-errors         make gcc warnings behave like errors: none, normal, pedantic [[default=none]]],,
    enable_errors="none")

AC_ARG_ENABLE(sctp,
[  --enable-sctp           enable SCTP support [[default=autodetected]]],,         enable_sctp="yes")

m4_undefine([AC_PROG_CXX])
m4_defun([AC_PROG_CXX],[])
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
AC_SUBST([LIBTOOL_DEPS])

AC_C_BIGENDIAN([BIGENDIAN="Big Endian"] AC_DEFINE([WORDS_BIGENDIAN], 1, [Define if manchine is big-endian]),[BIGENDIAN="Little Endian"] )

ACX_PTHREAD(,AC_MSG_ERROR([Missing POSIX Threads support]))

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

AC_CHECK_LIBM

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h unistd.h strings.h errno.h sys/errno.h fcntl.h malloc.h sys/ioctl.h getopt.h syslog.h) 

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
AC_CHECK_FUNCS(select socket gettimeofday uname getcwd getwd strcspn strdup strtoul strerror strstr setenv getopt_long uname)
AC_CHECK_FUNC(getaddrinfo)

AC_CHECK_LIB([c], [shm_open], [SHM_LIB=], 
    [AC_CHECK_LIB([rt], [shm_open], [SHM_LIB=-lrt])])
AC_SUBST(SHM_LIB)

fenice_state_dir=$(eval echo ${localstatedir}/feng)
AC_SUBST(fenice_state_dir)

dnl check if we have the generic struct for net addresses the has max possible size
AC_CHECK_TYPES(struct sockaddr_storage,,, [#include <sys/socket.h>])

dnl CFLAGS=""

AM_PATH_GLIB_2_0(2.0.0,CFLAGS="$CFLAGS $GLIB_CFLAGS",AC_MSG_ERROR([Feng uses Glib. Please install it (you need devel files).]), gthread)
AM_PATH_LIBGCRYPT(1.2.0, [CFLAGS="$CFLAGS $LIBGCRYPT_CFLAGS"
                          LIBS="$LIBS $LIBGCRYPT_LIBS"], AC_MSG_ERROR([Feng uses libgcrypts. Please install it.]))
dnl test enable
if test "x$enable_ipv6" = "xyes"; then
    AC_CHECK_TYPE(struct sockaddr_in6,
        AC_DEFINE([IPV6], 1, [Define IPv6 support]),
        ,
        [#include <netinet/in.h>])
else
    AC_MSG_WARN([IPv6 disabled: check skipped.])
fi

PKG_CHECK_MODULES(LIBNETEMBRYO,[libnetembryo >= 0.0.8])
CFLAGS="$CFLAGS $LIBNETEMBRYO_CFLAGS"
LIBS="$LIBS $LIBNETEMBRYO_LIBS"

PKG_CHECK_MODULES(LIBBUFFERPOOL,[libbufferpool >= 0.0.3])
CFLAGS="$CFLAGS $LIBBUFFERPOOL_CFLAGS"
LIBS="$LIBS $LIBBUFFERPOOL_LIBS"

dnl Check for SCTP support
if test "$enable_sctp" = "yes"; then
    if test "${LIBNETEMBRYO_LIBS//sctp}" = "${LIBNETEMBRYO_LIBS}"; then
        AC_MSG_WARN([sctp required but netembryo does not support it, rebuild netembryo first.])
    fi
        AC_CHECK_TYPE(struct sctp_sndrcvinfo, [
         AC_CHECK_LIB(sctp,sctp_recvmsg)],
         [enable_sctp=no,
         AC_MSG_WARN([SCTP disabled: headers not found])],
         [#include <netinet/sctp.h>])
else
    AC_MSG_WARN([SCTP disabled: check skipped.])
fi

avformat_msg="no"
avutil_msg="no"
if test "x$enable_ffmpeg" = "xyes"; then
    PKG_CHECK_MODULES(LIBAVFORMAT,[libavformat >= 52.7.0],
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

if test "$enable_mudflap" = "yes"; then
    CFLAGS="$CFLAGS -fmudflapth"
    LIBS="$LIBS -lmudflapth"
    enable_debug=yes
fi

AM_CONDITIONAL(FENICE_DEBUG, [test "$enable_debug" = "yes"])
if test "$enable_debug" = "yes"; then
    CFLAGS="$CFLAGS -g -ggdb -Wall"
    AC_DEFINE(ENABLE_DEBUG, 1,[Debug enabled])
else
    AC_DEFINE(ENABLE_DEBUG, 0,[Debug disabled])
    CFLAGS="$CFLAGS -O2 -Wall"
fi

if test "$enable_trace" = "yes"; then
    AC_DEFINE(TRACE, 1,[Trace enabled])
fi

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

fenice_avroot_dir=$(eval echo $localstatedir/$PACKAGE_NAME/avroot|sed -e "s:/\+:/:g")
fenice_avroot_dir=${fenice_avroot_dir//NONE}

AC_SUBST(fenice_avroot_dir)

fenice_max_session=$(eval echo $with_max_session)
AC_SUBST(fenice_max_session)

fenice_rtsp_port=$(eval echo $with_rtsp_port)
AC_SUBST(fenice_rtsp_port)

fenice_conf_file=$(eval echo $with_config_file|sed -e "s:/\+:/:g")
AC_SUBST(fenice_conf_file)


feng_basedir=$(eval echo $localstatedir|sed -e "s:/\+:/:g")
feng_basedir=${feng_basedir//NONE}

feng_logdir="${feng_basedir}/log/$PACKAGE_NAME"
AC_SUBST(feng_logdir)

fenice_log_file="${feng_basedir}/log/$with_log_file"
AC_SUBST(fenice_log_file)

feng_statedir="${feng_basedir}/lib/$PACKAGE_NAME"
AC_SUBST(feng_statedir)

feng_basedir="${feng_basedir}/$PACKAGE_NAME"
AC_SUBST(feng_basedir)

fenice_conf_dir=$(eval echo $sysconfdir|sed -e "s:/\+:/:g")
fenice_conf_dir=${fenice_conf_dir//NONE}
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

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([mediathread/Makefile])
AC_CONFIG_FILES([mediathread/parser/Makefile])
AC_CONFIG_FILES([mediathread/demuxer/Makefile])
AC_CONFIG_FILES([mediathread/metadata/Makefile])
AC_CONFIG_FILES([eventloop/Makefile])
AC_CONFIG_FILES([rtp/Makefile])
AC_CONFIG_FILES([rtcp/Makefile])
AC_CONFIG_FILES([rtsp/Makefile])
AC_CONFIG_FILES([sdp/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([schedule/Makefile])
AC_CONFIG_FILES([multicast/Makefile])
AC_CONFIG_FILES([conf/Makefile])
AC_CONFIG_FILES([docs/Makefile])
AC_CONFIG_FILES([etc/Makefile])
AC_CONFIG_FILES([avroot/Makefile])
AC_CONFIG_FILES([etc/feng.conf])
AC_OUTPUT

AC_MSG_NOTICE([config files successfully generated!
 
Feng Install prefix .......... : $prefix
Feng Binary path ............. : $(eval echo $exec_prefix/bin/feng)
Feng Configuration path ...... : $fenice_conf_dir
Feng Log File ................ : $fenice_log_file
Feng Multimedia Files root ... : $fenice_avroot_dir
Feng RTSP listening port ..... : $fenice_rtsp_port
Byte order ................... : this is a $BIGENDIAN machine

sctp support enabled ......... : $enable_sctp
avformat support enabled ..... : $avformat_msg
avutil support enabled ....... : $avutil_msg
debug enabled ................ : $enable_debug
 duma enabled ................ : $enable_duma
 mudflap enabled ............. : $enable_mudflap

 'make' will now compile Feng and 'su -c make install' will install it.
 Note: On non-Linux systems you might need to use 'gmake' instead of 'make'.
])

