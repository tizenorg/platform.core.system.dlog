AUTOMAKE_OPTIONS = subdir-objects foreign

AM_CFLAGS = -I$(srcdir)/include \
	$(DEBUG_CFLAGS) \
	-Werror \
	$(CAPI_BASE_COMMON_CFLAGS) \
	-D_GNU_SOURCE

dlog_includedir = $(includedir)/dlog

dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	src/libdlog/log.c \
	src/shared/logcommon.c \
	include/dlog.h \
	include/logcommon.h \
	src/libdlog/logconfig.c \
	include/logconfig.h \
	src/libdlog/loglimiter.c \
	include/loglimiter.h

libdlog_la_LIBADD = -lpthread \
	-lsystemd-journal \
	$(systemd_journal_LIBS) \
	$(CAPI_BASE_COMMON_LIBS)

bin_PROGRAMS = dlogutil

dlogutil_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlogutil_LDFLAGS = \
	$(AM_LDFLAGS) \
	-lsystemd-journal \
	-pie

dlogutil_SOURCES = \
	src/shared/kmsg_ioctl.c \
	src/shared/logcommon.c \
	src/shared/logprint.c \
	src/logutil/logutil.c \
	src/shared/queued_entry.c \
	src/shared/log_file.c \
	include/kmsg_ioctl.h \
	include/logcommon.h \
	include/queued_entry.h \
	include/log_file.h \
	include/logprint.h

bin_PROGRAMS += dlog_logger
sbin_PROGRAMS = dloginit

dloginit_CFLAGS =  \
	$(AM_CFLAGS) \
	$(LIBUDEV_CFLAGS) \
	-fPIE

dloginit_LDFLAGS = \
	$(AM_LDFLAGS) \
	$(LIBUDEV_LIBS) \
	-pie

dloginit_SOURCES = \
	src/loginit/loginit.c \
	include/logcommon.h \
	include/dlog.h

dlog_logger_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlog_logger_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pie

dlog_logger_SOURCES = \
	src/shared/kmsg_ioctl.c \
	src/shared/logcommon.c \
	src/shared/logprint.c \
	src/logger/logger.c \
	src/shared/queued_entry.c \
	src/shared/log_file.c \
	include/kmsg_ioctl.h \
	include/logcommon.h \
	include/queued_entry.h \
	include/log_file.h \
	include/logprint.h

include/%: include/%.m4
	m4 -P $(M4_DEFINES) < $< > $@

# conf file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc
