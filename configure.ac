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

# checks arch
AM_CONDITIONAL(DEVELOP_VER, test $DEVELOP_VER = yes)

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

#AM_CONDITIONAL([ARCH_IS_ARM], [test "x$ARCH" = "xarm"])
if test "x$ARCH" = "xarm" ; then
	DLOG_CFLAGS="-D__arm__"
else
	DLOG_CFLAGS=
fi

AC_SUBST(DLOG_CFLAGS)


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

# output files
AC_CONFIG_FILES([Makefile dlog.pc])
AC_OUTPUT
