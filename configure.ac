AC_INIT([grasp],[0.1])
AM_INIT_AUTOMAKE()

AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_LIB(popt, poptGetArg, [POPT_LIBS="-lpopt"], [echo "$0: libpopt is missing" >&2; exit 2])
AC_SUBST(POPT_LIBS)

AC_CHECK_LIB(curl, curl_getenv, [CURL_LIBS="-lcurl"], [echo "$0: libcurl is missing" >&2; exit 2])
AC_SUBST(CURL_LIBS)

AC_PATH_PROG(GIT_BIN_PATH, git)
AC_PATH_PROG(TAR_BIN_PATH, tar)
AC_PATH_PROG(CP_BIN_PATH, cp)
AC_PATH_PROG(MV_BIN_PATH, mv)
AC_PATH_PROG(RM_BIN_PATH, rm)
AC_PATH_PROG(MKDIR_BIN_PATH, mkdir)
AC_PATH_PROG(MD5SUM_BIN_PATH, md5sum)
AC_DEFINE_UNQUOTED([GIT_BIN_PATH],    ["$GIT_BIN_PATH"], ["Path to git executable"])
AC_DEFINE_UNQUOTED([TAR_BIN_PATH],    ["$TAR_BIN_PATH"], ["Path to tar executable"])
AC_DEFINE_UNQUOTED([CP_BIN_PATH],     ["$CP_BIN_PATH"],  ["Path to cp executable"])
AC_DEFINE_UNQUOTED([MV_BIN_PATH],     ["$MV_BIN_PATH"], ["Path to mv executable"])
AC_DEFINE_UNQUOTED([RM_BIN_PATH],     ["$RM_BIN_PATH"], ["Path to rm executable"])
AC_DEFINE_UNQUOTED([MKDIR_BIN_PATH],  ["$MKDIR_BIN_PATH"], ["Path to mkdir executable"])
AC_DEFINE_UNQUOTED([MD5SUM_BIN_PATH], ["$MD5SUM_BIN_PATH"], ["Path to md5sum executable"])

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
