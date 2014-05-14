AM_CFLAGS = -I$(srcdir)/include

dlog_includedir = $(includedir)/dlog
dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	log.c \
	log-common.c

libdlog_la_LIBADD = -ltzplatform-config-1.0 -lsystemd-journal

bin_PROGRAMS= dlogctrl

dlogctrl_SOURCES = \
	logctrl.c \
	log-common.c

dlogctrl_LDADD = -ltzplatform-config-1.0

dlogctrl_CFLAGS = $(AM_CFLAGS)

# conf file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc
