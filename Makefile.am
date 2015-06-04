AUTOMAKE_OPTIONS = subdir-objects
AM_CFLAGS = -I$(srcdir)/include

dlog_includedir = $(includedir)/dlog
dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	src/libdlog/log.c \
	include/dlog.h

libdlog_la_LIBADD = -lpthread $(TZPLATFORM_CONFIG_LIBS) $(systemd_journal_LIBS)

bin_PROGRAMS = dlogutil


dlogutil_SOURCES = \
	src/shared/logprint.c \
	src/logutil/logutil.c \
	include/logger.h \
	include/logprint.h


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc

