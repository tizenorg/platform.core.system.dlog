AUTOMAKE_OPTIONS = subdir-objects foreign

AM_CFLAGS = -I$(srcdir)/include \
	$(DEBUG_CFLAGS) \
	-Werror \
	$(CAPI_BASE_COMMON_CFLAGS)

dlog_includedir = $(includedir)/dlog

dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	src/libdlog/log.c \
	include/dlog.h \
	src/libdlog/logconfig.c \
	include/logconfig.h \
	src/libdlog/loglimiter.c \
	include/loglimiter.h

libdlog_la_LIBADD = -lpthread \
	$(systemd_journal_LIBS) \
	$(CAPI_BASE_COMMON_LIBS)

bin_PROGRAMS = dlogutil dlog_logger

dlogutil_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlogutil_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pie

dlogutil_SOURCES = \
	src/shared/logprint.c \
	src/logutil/logutil.c \
	include/logger.h \
	include/logprint.h

dlog_logger_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlog_logger_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pie

dlog_logger_SOURCES = \
	src/shared/logprint.c \
	src/logger/logger.c \
	include/logger.h \
	include/logprint.h

include/%: include/%.m4
	m4 -P $(M4_DEFINES) < $< > $@

# conf file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc

dlogrulesdir=$(libdir)/udev/rules.d
dlogrules_DATA = 01-dlog.rules
