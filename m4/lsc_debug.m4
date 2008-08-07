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

  AS_IF([test "$enable_debug" = "yes"], [
    CFLAGS="$CFLAGS -g -ggdb -Wall"
    AC_DEFINE(ENABLE_DEBUG, 1,[Debug enabled])
  ], [
    AC_DEFINE(ENABLE_DEBUG, 0,[Debug disabled])
    CFLAGS="$CFLAGS -O2 -Wall"
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
