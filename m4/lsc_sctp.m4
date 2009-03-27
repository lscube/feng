AC_DEFUN([LSC_CHECK_SCTP], [
  have_sctp=no

  AC_ARG_ENABLE(sctp,
    AS_HELP_STRING([--enable-sctp], [enable SCTP support [[default=yes]]]),,
	enable_sctp="yes")

  AC_MSG_CHECKING([whether to enable SCTP support])
  AS_IF([test "x$enable_sctp" = "xyes"], [
    AC_MSG_RESULT([yes, checking prerequisites])

    AC_CHECK_HEADERS([sys/socket.h])

    AC_CHECK_TYPE([struct sctp_sndrcvinfo], [
        save_LIBS="$LIBS"
        AC_SEARCH_LIBS([sctp_recvmsg], [sctp], [
          SCTP_LIBS="${LIBS%${save_LIBS}}"
	  have_sctp=yes
	  AC_DEFINE([HAVE_SCTP], [1], [Define this if you have libsctp])
	])
	LIBS="$save_LIBS"
	],
      AC_MSG_WARN([SCTP disabled: headers not found]),
      [#ifdef HAVE_SYS_SOCKET_H
       #include <sys/socket.h>
       #endif
       #include <netinet/sctp.h>
      ])
  ], [
    AC_MSG_RESULT([no, disabled by user])
  ])

  AC_SUBST([SCTP_LIBS])

  AM_CONDITIONAL([HAVE_SCTP], [test "x$have_sctp" = "xyes"])
])
