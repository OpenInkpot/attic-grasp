AC_INIT([grasp],[0.1])
AM_INIT_AUTOMAKE()

AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_LIB(curl, curl_getenv, [CURL_LIBS="-lcurl"], [echo "$0: libcurl is missing" >&2; exit 2])
AC_SUBST(CURL_LIBS)

BUILD_DATE="`822-date`"
AC_SUBST(BUILD_DATE)

AC_SUBST(AM_CFLAGS, "\
	-g -Wall -std=c89 \
	-I\$(top_srcdir) \
	-I\$(top_srcdir)/include \
	-DSYSCONFDIR=\\\"\$(sysconfdir)\\\" \
	-DLOCALSTATEDIR=\\\"\$(localstatedir)\\\"")

AC_CONFIG_HEADERS([config.h])
AC_OUTPUT([
	Makefile
	src/Makefile
])
