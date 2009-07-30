dnl -*- autoconf -*-

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
