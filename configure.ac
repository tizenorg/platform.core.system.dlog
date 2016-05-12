#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([dlog], [1.0], yk.yun@samsung.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h:config.hin])

# Init XO
PLATFORM_INIT

# Checks for programs.
dnl AC_PROG_CXX
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_LIBTOOL

# backend: journald
AC_ARG_ENABLE(journal, AS_HELP_STRING([--enable-journal], [enable systemd journal]),
    [with_systemd_journal=yes],
    with_systemd_journal=no)
if test "x$with_systemd_journal" != "xno"; then
	PKG_CHECK_MODULES(systemd_journal, [libsystemd-journal],
		[AC_DEFINE(DLOG_BACKEND_JOURNAL, 1, [Define if systemd journal is selected])
		have_systemd_journal=yes],
		have_systemd_journal=no)
	if test "x$have_systemd_journal" = "xno" -a "x$with_systemd_journal" = "xyes"; then
		AC_MSG_ERROR([systemd journal requested but libraries not found])
	fi
fi
AM_CONDITIONAL(WITH_SYSTEMD_JOURNAL, [test "x$have_systemd_journal" = "xyes"])

# backend: pipe
AC_ARG_ENABLE(pipe, AS_HELP_STRING([--enable-pipe], [enable pipe (log to pipe fd received via unix socket)]),
	[with_pipe=yes],
	with_pipe=no)
if test "x$with_pipe" = "xyes"; then
	AC_DEFINE(DLOG_BACKEND_PIPE, 1, [Define if pipe backend is selected])
fi
AM_CONDITIONAL(WITH_PIPE, [test "x$with_pipe" = "xyes"])

# backend: kmsg (requires kernel patch)
AC_ARG_ENABLE(kmsg, AS_HELP_STRING([--enable-kmsg], [enable kmsg (requires kmsg extension patch)]),
	[with_kmsg=yes],
	with_kmsg=no)
if test "x$with_kmsg" = "xyes"; then
	AC_DEFINE(DLOG_BACKEND_KMSG, 1, [Define if kmsg backend is selected])
fi
AM_CONDITIONAL(WITH_KMSG, [test "x$with_kmsg" = "xyes"])

# backend: android logger (requires kernel's CONFIG_ANDROID_LOGGER)
AC_ARG_ENABLE(android_logger, AS_HELP_STRING([--enable-android-logger], [enable android logger (kernels CONFIG_ANDROID_LOGGER)]),
	[with_android_logger=yes],
	with_android_logger=no)
if test "x$with_android_logger" = "xyes"; then
	AC_DEFINE(DLOG_BACKEND_LOGGER, 1, [Define if (legacy) android logger backend is selected])
fi
AM_CONDITIONAL(WITH_ANDROID_LOGGER, [test "x$with_android_logger" = "xyes"])

# check binary type for dlog debug mode
AC_ARG_ENABLE([debug_mode],
	AS_HELP_STRING([--enable-debug_mode Turn on debug_mode]),
		[debug_mode=yes],
		debug_mode=no)
if test "x$debug_mode" = "xyes" ; then
	DEBUG_CFLAGS+=" -DDLOG_DEBUG_ENABLE"
fi
AC_ARG_ENABLE([debug_enable],
	AS_HELP_STRING([--enable-debug_enable Turn on debug_enable]),
		[debug_enable=yes],
	    debug_enable=no)
# check for fatal_on
AC_ARG_ENABLE([fatal_on],
	AS_HELP_STRING([--enable-fatal_on Turn on fatal assertion]),
		[fatal_on=yes],
	    fatal_on=no)
if test "x$fatal_on" = "xyes" ; then
	DEBUG_CFLAGS+=" -DFATAL_ON"
fi
AC_SUBST(DEBUG_CFLAGS)
# Checks for libraries.
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h unistd.h ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_STRUCT_TM
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([memset])

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([CAPI_BASE_COMMON], [capi-base-common])

AC_SUBST(TZ_SYS_ETC)

# output files
AC_CONFIG_FILES([Makefile dlog.pc])
AC_OUTPUT
