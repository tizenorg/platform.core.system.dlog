#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([dlog], [1.0], m.niesluchow@samsung.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_MACRO_DIR([m4])

PLATFORM_INIT
AC_CONFIG_SRCDIR([log.c])
AC_CONFIG_HEADERS([config.h])

PKG_CHECK_MODULES([TZPLATFORM_CONFIG], [libtzplatform-config])

# Checks for programs.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strtol])

AC_CONFIG_FILES([Makefile dlog.pc])
AC_OUTPUT
