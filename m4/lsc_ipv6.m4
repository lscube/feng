AC_DEFUN([LSC_CHECK_IPV6], [
  AC_ARG_ENABLE(ipv6,
    AS_HELP_STRING([--enable-ipv6], [enable IPv6 support [[default=yes]]]),,
    enable_ipv6="yes")

  AC_MSG_CHECKING([whether to check for IPv6])
  AS_IF([test "x$enable_ipv6" = "xyes"], [
    AC_MSG_RESULT([yes])
    AC_CHECK_TYPE(struct sockaddr_in6,
      AC_DEFINE([IPV6], 1, [Define IPv6 support]),,
      [
       #include <netinet/in.h>
    ])
  ], [
    AC_MSG_RESULT([no])
  ])
])
