#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([dlog], [1.0], yk.yun@samsung.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h:config.hin])

AC_CONFIG_MACRO_DIR([m4])
# Init XO
PLATFORM_INIT

# Checks for programs.
dnl AC_PROG_CXX
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_LIBTOOL

# check for libsystemd-journal
AC_ARG_WITH([systemd-journal],
	AS_HELP_STRING([--with-systemd-journal], [check for systemd-journal]),
	[], [with_systemd_journal=check])
have_systemd_journal=no
if test "x$with_systemd_journal" != "xno"; then
	PKG_CHECK_MODULES(systemd_journal, [libsystemd-journal],
		[AC_DEFINE(HAVE_SYSTEMD_JOURNAL, 1, [Define if systemd journal is available])
		have_systemd_journal=yes],
	    have_systemd_journal=no)
	if test "x$have_systemd_journal" = "xno" -a "x$with_systemd_journal" = "xyes"; then
		AC_MSG_ERROR([systemd journal requested but libraries not found])
	fi
fi
AM_CONDITIONAL(HAVE_SYSTEMD_JOURNAL, [test "x$have_systemd_journal" = "xyes"])
if test "x$have_systemd_journal" = "xyes" ; then
	systemd_journal=systemd-journal
fi
AC_SUBST(systemd_journal)

# check binary type for tizen engineer mode
AC_ARG_ENABLE([engineer_mode],
	AS_HELP_STRING([--enable-engineer_mode Turn on engineer_mode]),
		[engineer_mode=yes],
		engineer_mode=no)
if test "x$engineer_mode" = "xyes" ; then
	DEBUG_CFLAGS+=" -DTIZEN_ENGINEER_MODE"
fi
AC_ARG_ENABLE([debug_enable],
	AS_HELP_STRING([--enable-debug_enable Turn on debug_enable]),
		[debug_enable=yes],
	    debug_enable=no)
if test "x$debug_enable" = "xyes" ; then
	DEBUG_CFLAGS+=" -DTIZEN_DEBUG_ENABLE"
fi
# check for fatal_on
AC_ARG_ENABLE([fatal_on],
	AS_HELP_STRING([--enable-fatal_on Turn on fatal assertion]),
		[fatal_on=yes],
	    fatal_on=no)
if test "x$fatal_on" = "xyes" ; then
	DEBUG_CFLAGS+=" -DFATAL_ON"
fi
AC_SUBST(DEBUG_CFLAGS)
dnl AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")
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

# output files
AC_CONFIG_FILES([Makefile dlog.pc])
AC_OUTPUT
