dnl -*- autoconf -*-

AC_DEFUN([LSC_DEBUG_ENABLE], [
  AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug], [enable gcc debugging flags [[default=no]]]),,
    enable_debug="no")
])

AC_DEFUN([LSC_MUDFLAP], [
  AC_REQUIRE([LSC_DEBUG_ENABLE])

  AC_ARG_ENABLE(mudflap,
    AS_HELP_STRING([--enable-mudflap], [enable mudflap support (implies --enable-debug) [[default=no]]]),,
    enable_mudflap="no")

  AS_IF([test "$enable_mudflap" = "yes"], [
    CC_CHECK_CFLAGS([-fmudflapth], ,
      [AC_MSG_ERROR([mudflap support requested, but the compiler does not support it])])
    AC_CHECK_LIB([mudflapth], [main], ,
      [AC_MSG_ERROR([mudflap support requested, but the compiler does not support it])])

    enable_debug=yes
  ])
])

AC_DEFUN([LSC_DEBUG], [
  AC_REQUIRE([LSC_DEBUG_ENABLE])
  AC_REQUIRE([LSC_MUDFLAP])

  dnl Check for warning flags, always
  CC_CHECK_CFLAGS_APPEND([-Wall -Wwrite-strings])
  dnl Only enable the best of the two
  CC_CHECK_CFLAGS_APPEND([-Wformat=2 -Wformat], [break;])
  dnl The new style is likely going to be the only supported one in the future
  CC_CHECK_CFLAGS_APPEND([-Werror=implicit -Werror-implicit-declaration],
                         [break;])
  dnl Make sure that there are no random return values
  CC_CHECK_CFLAGS_APPEND([-Werror=return-type -Wreturn-type], [break;])

  AS_IF([test "$enable_debug" = "yes"], [
    CC_CHECK_CFLAGS_APPEND([-ggdb -g], [break;])
  ], [
    CC_CHECK_CFLAGS_APPEND([-DNDEBUG -DG_DISABLE_ASSERT])
  ])
])

AC_DEFUN([LSC_ERRORS], [
  AC_REQUIRE([CC_CHECK_WERROR])

  AC_ARG_ENABLE(errors,
    AS_HELP_STRING([--enable-errors], [make gcc warnings behave like errors: none, normal, pedantic [[default=none]]]))

  case "$enable_errors" in
    pedantic)
        CFLAGS="$CFLAGS -pedantic-errors $cc_cv_werror"
    ;;
    normal | yes)
        CFLAGS="$CFLAGS $cc_cv_werror"
	enable_errors=normal
    ;;
    none | *)
        enable_errors=none
    ;;
  esac
])

AC_DEFUN([LSC_DEBUG_STATUS], [
  AC_MSG_NOTICE([
debug enabled ................ : $enable_debug
 mudflap enabled ............. : $enable_mudflap
  errors enabled ............. : $enable_errors
  ])
])

AC_DEFUN([LSC_TESTS], [
    AC_ARG_ENABLE([tests],
      [AS_HELP_STRING([--enable-tests], [Enable test building (requires gawk, glib, ctags)])],
      [], [enable_tests=auto])

    AC_ARG_VAR([GAWK], [path to a GNU awk-compatible program])
    AC_ARG_VAR([EXUBERANT_CTAGS], [path to an exuberant ctags-compatible program])
    AS_IF([test "x$enable_tests" != "no"],
      [have_tests=yes
       PKG_CHECK_MODULES([GTESTER], [glib-2.0 >= 2.20], [], [have_tests=no])
       AC_CHECK_PROGS([GAWK], [gawk])
       AS_IF([test "x$GAWK" = "x"], [have_tests=no])
       AC_CHECK_PROGS([EXUBERANT_CTAGS], [exuberant-ctags])
       AS_IF([test "x$EXUBERANT_CTAGS" = "x"], [have_tests=no])
      ])
    AC_SUBST([GAWK])
    AC_SUBST([EXUBERANT_CTAGS])

    AS_IF([test "x$enable_tests" = "xyes" && test "x$have_tests" = "xno"],
      [AC_MSG_ERROR([Required software for testing not found])])

    AM_CONDITIONAL([BUILD_TESTS], [test "x$have_tests" = "xyes"])
])

AC_DEFUN([LSC_SYSTEM_EXTENSIONS], [
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
  AC_REQUIRE([AC_CANONICAL_HOST])

  dnl Force usage of POSIX.1-2001 (clock_gettime)
  CPPFLAGS="${CPPFLAGS} -D_POSIX_C_SOURCE=200112L"

  dnl Enable some further OS-specific extensions that AC_USE_SYSTEM_EXTENSIONS
  dnl does not enable at least up to autoconf 2.64.
  dnl These extensions are needed to have struct ip_mreq for multicast support,
  dnl and others.
  AS_CASE([$host_os],
    [darwin*],        [CPPFLAGS="${CPPFLAGS} -D_DARWIN_C_SOURCE"],
    [freebsd[[78]]*], [CPPFLAGS="${CPPFLAGS} -D__BSD_VISIBLE=1"])
])

AC_DEFUN([LSC_INIT], [
  m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
  AM_MAINTAINER_MODE

  AC_PREFIX_DEFAULT("/usr")

  LSC_SYSTEM_EXTENSIONS

  AC_PROG_CC_C99
  AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], [
    AC_MSG_ERROR([no C99 compiler found, $PACKAGE requires a C99 compiler.])
  ])

  LSC_ERRORS
  LSC_DEBUG
  LSC_TESTS
])
