

AM_CFLAGS = @CFLAGS@ -I$(srcdir)/include \
	      -I$(srcdir)/include/internal


dlog_includedir = $(includedir)/dlog
dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	log.c \
	include/dlog.h

libdlog_la_LIBADD = -lpthread -lsystemd-journal

bin_PROGRAMS= dlogutil

dlogutil_SOURCES = \
	logutil.c \
	logprint.c \
	include/logger.h \
	include/logprint.h

# conf file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc

bootscriptdir = /etc/rc.d/init.d
bootscript_SCRIPTS = dlog.sh


