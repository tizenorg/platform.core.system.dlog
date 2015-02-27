AM_CFLAGS = -I$(srcdir)/include

dlog_includedir = $(includedir)/dlog
dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	log.c \
	log-common.c \
	include/dlog.h

libdlog_la_LIBADD = -lpthread $(TZPLATFORM_CONFIG_LIBS) $(systemd_journal_LIBS)

bin_PROGRAMS= dlogutil dlogtests

sbin_PROGRAMS = dlogctrl

dlogutil_SOURCES = \
	logutil.c \
	logprint.c \
	include/logger.h \
	include/logprint.h

dlogtests_SOURCES = \
	logtest.c \
	logprint.c \
	include/logger.h \
	include/logprint.h

dlogctrl_SOURCES = \
	logctrl.c \
	log-common.c

dlogctrl_LDADD = $(TZPLATFORM_CONFIG_LIBS)

dlogctrl_CFLAGS = $(AM_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc

platformlogconfdir=$(sysconfdir)/dlog
platformlogconf_DATA = platformlog.conf
platformlog.conf:
	/bin/echo '0' > platformlog.conf

dlogallconfdir=$(sysconfdir)/dlog
dlogallconf_DATA = dlog.all.conf

dlogradioconfdir=$(sysconfdir)/dlog
dlogradioconf_DATA = dlog.radio.conf

dlogrulesdir=$(libdir)/udev/rules.d
dlogrules_DATA = 01-dlog.rules
