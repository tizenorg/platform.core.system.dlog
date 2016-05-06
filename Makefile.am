AUTOMAKE_OPTIONS = subdir-objects foreign

BUILT_SOURCES = include/dlog.h

AM_CFLAGS = -I$(srcdir)/include \
	$(DEBUG_CFLAGS) \
	-Werror \
	$(CAPI_BASE_COMMON_CFLAGS) \
	-D_GNU_SOURCE

dlog_includedir = $(includedir)/dlog

dlog_include_HEADERS = \
	include/dlog.h \
	include/dlog-internal.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	src/libdlog/log.c \
	src/shared/logcommon.c \
	include/dlog.h \
	include/dlog-internal.h \
	src/shared/logconfig.c \
	include/logconfig.h \
	src/libdlog/loglimiter.c \
	include/loglimiter.h

if WITH_PIPE
libdlog_la_SOURCES += \
	src/libdlog/log_pipe.c
endif

if WITH_ANDROID_LOGGER
libdlog_la_SOURCES += \
	src/libdlog/log_android.c
endif

if WITH_KMSG
libdlog_la_SOURCES += \
	src/libdlog/log_kmsg.c
endif

if WITH_SYSTEMD_JOURNAL
libdlog_la_SOURCES += \
	src/libdlog/log_journal.c
endif

libdlog_la_LIBADD = -lpthread \
	$(CAPI_BASE_COMMON_LIBS)

if WITH_SYSTEMD_JOURNAL
libdlog_la_LIBADD += \
	$(systemd_journal_LIBS)
endif

bin_PROGRAMS = dlogutil

dlogutil_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlogutil_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pie

if WITH_SYSTEMD_JOURNAL
dlogutil_LDFLAGS += \
	$(systemd_journal_LIBS)
endif

dlogutil_SOURCES = \
	src/shared/dlog_ioctl.c \
	src/shared/logcommon.c \
	src/shared/logprint.c \
	src/shared/queued_entry.c \
	src/shared/log_file.c \
	src/shared/logconfig.c \
	include/dlog_ioctl.h \
	include/logcommon.h \
	include/queued_entry.h \
	include/log_file.h \
	include/logprint.h

if WITH_ANDROID_LOGGER
dlogutil_SOURCES += src/logutil/logutil_kmsg_logger.c
endif

if WITH_KMSG
dlogutil_SOURCES += src/logutil/logutil_kmsg_logger.c
endif

if WITH_SYSTEMD_JOURNAL
dlogutil_SOURCES += src/logutil/logutil_journal.c
endif

if WITH_PIPE
dlogutil_SOURCES += src/logutil/logutil_pipe.c
endif

if !WITH_SYSTEMD_JOURNAL

bin_PROGRAMS += dlog_logger

dlog_logger_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlog_logger_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pie

dlog_logger_SOURCES = \
	src/shared/dlog_ioctl.c \
	src/shared/logcommon.c \
	src/shared/logconfig.c \
	src/shared/logprint.c \
	src/shared/queued_entry.c \
	src/shared/log_file.c \
	include/dlog_ioctl.h \
	include/logcommon.h \
	include/queued_entry.h \
	include/log_file.h \
	include/logprint.h

if WITH_PIPE
dlog_logger_SOURCES += src/logger/logger_pipe.c
else
dlog_logger_SOURCES += src/logger/logger.c
endif

endif

if WITH_KMSG
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
	src/shared/logconfig.c \
	src/shared/logcommon.c \
	include/logcommon.h \
	include/dlog.h
endif

bin_PROGRAMS += dlogctrl
dlogctrl_CFLAGS =  \
	$(AM_CFLAGS) \
	-fPIE

dlogctrl_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pie

dlogctrl_SOURCES = \
	src/shared/logconfig.c \
	src/logctrl/logctrl.c \
	include/logconfig.h

# conf file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc
